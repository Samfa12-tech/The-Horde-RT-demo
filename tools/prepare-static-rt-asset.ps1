[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$SourceGlb,
    [Parameter(Mandatory = $true)][string]$Manifest,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [string[]]$BaseColorImage = @(),
    [string[]]$NormalImage = @(),
    [string[]]$RoughnessImage = @(),
    [string[]]$MetallicImage = @(),
    [string[]]$EmissiveImage = @(),
    [string[]]$ReviewedValidatorIssueCodes = @(),
    [int]$TextureResolution = 512,
    [switch]$PreserveAuthoredSockets,
    [string]$RuntimeStatus = "development-only-license-unresolved"
)

$ErrorActionPreference = "Stop"
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$sourcePath = [IO.Path]::GetFullPath((Join-Path $repoRoot $SourceGlb))
$manifestPath = [IO.Path]::GetFullPath((Join-Path $repoRoot $Manifest))
$outputRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot $OutputDirectory))
if (-not (Test-Path -LiteralPath $sourcePath)) { throw "Static RT source GLB is missing: $sourcePath" }
if (-not (Test-Path -LiteralPath $manifestPath)) { throw "Static RT asset manifest is missing: $manifestPath" }
if ($TextureResolution -lt 1) { throw "TextureResolution must be greater than zero." }
$manifestData = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if (@($manifestData.lods).Count -lt 1) { throw "Asset manifest must define at least one LOD budget." }
foreach ($lod in @($manifestData.lods)) {
    if ([int64]$lod.maxTriangles -le 0) {
        throw "Asset manifest LOD '$($lod.name)' maxTriangles must be greater than zero."
    }
}
function Test-ExactLodIdentitySuffix([string]$assetIdentity, [string]$lodName) {
    if ([string]::IsNullOrWhiteSpace($lodName)) { return $false }
    if ($assetIdentity.EndsWith(".runtime", [StringComparison]::Ordinal)) {
        $assetIdentity = $assetIdentity.Substring(0, $assetIdentity.Length - ".runtime".Length)
    }
    if ($assetIdentity.Equals($lodName, [StringComparison]::Ordinal)) { return $true }
    foreach ($delimiter in @(".", "-", "_")) {
        if ($assetIdentity.EndsWith("$delimiter$lodName", [StringComparison]::Ordinal)) { return $true }
    }
    return $false
}

$sourceStem = [IO.Path]::GetFileNameWithoutExtension($sourcePath)
$selectedLods = @($manifestData.lods | Where-Object {
    Test-ExactLodIdentitySuffix $sourceStem ([string]$_.name)
})
if ($selectedLods.Count -eq 0 -and @($manifestData.lods).Count -eq 1) {
    $selectedLods = @($manifestData.lods[0])
}
if ($selectedLods.Count -ne 1) { throw "Static RT asset filename must select exactly one manifest LOD identity." }
$selectedLod = $selectedLods[0]
New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null

$nodeToolRoot = Join-Path $repoRoot "build\tools\static-rt-asset-node"
$validatorModule = Join-Path $nodeToolRoot "node_modules\gltf-validator"
if (-not (Test-Path -LiteralPath $validatorModule)) {
    npm install --prefix $nodeToolRoot --no-save --ignore-scripts --silent `
        gltf-validator@2.0.0-dev.3.10
    if ($LASTEXITCODE -ne 0) { throw "Failed to provision the pinned static asset preparation tools." }
}

function Invoke-Validator([string]$assetPath, [string]$reportPath, [string[]]$reviewedCodes) {
    node (Join-Path $PSScriptRoot "run-gltf-validator.mjs") $assetPath $reportPath $nodeToolRoot
    if ($LASTEXITCODE -ne 0) { throw "Khronos glTF Validator invocation failed for $assetPath" }
    $report = Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json
    if ($report.issues.numErrors -ne 0) {
        $codes = @($report.issues.messages | Where-Object severity -eq 0 | ForEach-Object code) -join ", "
        throw "Khronos glTF Validator rejected $assetPath with error code(s): $codes"
    }
    $unreviewed = @($report.issues.messages | Where-Object {
        $_.severity -eq 1 -and $reviewedCodes -notcontains $_.code
    })
    if ($unreviewed.Count -ne 0) {
        throw "Khronos glTF Validator found unreviewed warning code(s): $(@($unreviewed | ForEach-Object code) -join ', ')"
    }
    return $report
}

function Read-GlbJson([string]$path) {
    $bytes = [IO.File]::ReadAllBytes($path)
    if ($bytes.Length -lt 20 -or [BitConverter]::ToUInt32($bytes, 0) -ne 0x46546c67 -or
        [BitConverter]::ToUInt32($bytes, 4) -ne 2 -or [BitConverter]::ToUInt32($bytes, 16) -ne 0x4e4f534a) {
        throw "Runtime GLB header is malformed while deriving texture layers."
    }
    $jsonLength = [BitConverter]::ToUInt32($bytes, 12)
    if (20 + $jsonLength -gt $bytes.Length) { throw "Runtime GLB JSON chunk is truncated while deriving texture layers." }
    return [Text.Encoding]::UTF8.GetString($bytes, 20, $jsonLength).TrimEnd([char]0, ' ') | ConvertFrom-Json
}

function Get-TextureIndex($view) {
    if ($null -eq $view -or $null -eq $view.index) { return -1 }
    return [int]$view.index
}

function Get-RequiredTextureLayerCounts([string]$runtimeGlb) {
    $document = Read-GlbJson $runtimeGlb
    $baseColor = [Collections.Generic.HashSet[string]]::new()
    $normal = [Collections.Generic.HashSet[string]]::new()
    $orm = [Collections.Generic.HashSet[string]]::new()
    $emissive = [Collections.Generic.HashSet[string]]::new()
    foreach ($material in @($document.materials)) {
        $baseIndex = Get-TextureIndex $material.pbrMetallicRoughness.baseColorTexture
        $normalIndex = Get-TextureIndex $material.normalTexture
        $metallicRoughnessIndex = Get-TextureIndex $material.pbrMetallicRoughness.metallicRoughnessTexture
        $occlusionIndex = Get-TextureIndex $material.occlusionTexture
        $emissiveIndex = Get-TextureIndex $material.emissiveTexture
        if ($baseIndex -ge 0) { $null = $baseColor.Add([string]$baseIndex) }
        if ($normalIndex -ge 0) { $null = $normal.Add([string]$normalIndex) }
        if ($metallicRoughnessIndex -ge 0 -or $occlusionIndex -ge 0) {
            $null = $orm.Add("$metallicRoughnessIndex/$occlusionIndex")
        }
        if ($emissiveIndex -ge 0) { $null = $emissive.Add([string]$emissiveIndex) }
    }
    return [ordered]@{
        BaseColor = [Math]::Max($baseColor.Count, 1)
        Normal = [Math]::Max($normal.Count, 1)
        Orm = [Math]::Max($orm.Count, 1)
        Emissive = [Math]::Max($emissive.Count, 1)
    }
}

function Resolve-TextureInputs([string[]]$values, [int]$requiredCount, [string]$category) {
    $provided = @($values | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($provided.Count -ne $requiredCount) {
        throw "Static RT $category texture input count $($provided.Count) does not match required array layer count $requiredCount."
    }
    $resolved = @($provided | ForEach-Object { [IO.Path]::GetFullPath((Join-Path $repoRoot $_)) })
    foreach ($path in $resolved) {
        if (-not (Test-Path -LiteralPath $path)) { throw "Static RT texture source is missing: $path" }
    }
    return $resolved
}

$sourceReportPath = Join-Path $outputRoot "validator-source.json"
$sourceReport = Invoke-Validator $sourcePath $sourceReportPath $ReviewedValidatorIssueCodes

$runtimeName = ([IO.Path]::GetFileNameWithoutExtension($sourcePath)) + ".runtime.glb"
$runtimePath = Join-Path $outputRoot $runtimeName
$blender = "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe"
if (-not (Test-Path -LiteralPath $blender)) { throw "Blender 5.2 is required for audited tangent/socket conversion." }
$temporaryPaths = [Collections.Generic.List[string]]::new()
if ($PreserveAuthoredSockets) {
    Copy-Item -LiteralPath $sourcePath -Destination $runtimePath -Force
} else {
    $socketGlb = Join-Path $outputRoot "conversion-with-socket.glb"
    $temporaryPaths.Add($socketGlb)
    # Legacy development sword path retained for the Task 2 proof. Production
    # held items arrive with reviewed exact-case sockets and use the generic path.
    & $blender --background --python (Join-Path $PSScriptRoot "add-gltf-socket.py") -- `
        $sourcePath $socketGlb grip 0 0 -0.78
    if ($LASTEXITCODE -ne 0) { throw "Blender tangent/socket conversion failed." }
    Copy-Item -LiteralPath $socketGlb -Destination $runtimePath -Force
}

$runtimeReportPath = Join-Path $outputRoot "validator-runtime.json"
$runtimeReport = Invoke-Validator $runtimePath $runtimeReportPath $ReviewedValidatorIssueCodes
& py -3 (Join-Path $PSScriptRoot "validate-dielectric-topology.py") $runtimePath $manifestPath
if ($LASTEXITCODE -ne 0) {
    throw "Offline dielectric topology validation rejected $runtimePath."
}
if ($runtimeReport.info.totalVertexCount -gt $manifestData.budgets.maxVertices) {
    throw "Runtime GLB exceeds manifest maxVertices capacity."
}
$indexCount = [int64]$runtimeReport.info.totalTriangleCount * 3
if ($indexCount -gt $manifestData.budgets.maxIndices) {
    throw "Runtime GLB exceeds manifest maxIndices capacity."
}
if ($runtimeReport.info.drawCallCount -gt $manifestData.budgets.maxPrimitives) {
    throw "Runtime GLB exceeds manifest maxPrimitives capacity."
}
if ($runtimeReport.info.materialCount -gt $manifestData.budgets.maxMaterials) {
    throw "Runtime GLB exceeds manifest maxMaterials capacity."
}
if ([int64]$runtimeReport.info.totalTriangleCount -gt [int64]$selectedLod.maxTriangles) {
    throw "Runtime GLB exceeds selected LOD '$($selectedLod.name)' maxTriangles capacity."
}
$sourceResolutionImages = @($runtimeReport.info.resources | Where-Object {
    $_.image -and ($_.image.width -gt 4 -or $_.image.height -gt 4)
})
if ($sourceResolutionImages.Count -ne 0) {
    throw "Runtime GLB still contains source-resolution image payloads."
}

$requiredTextureLayers = Get-RequiredTextureLayerCounts $runtimePath
$baseColorInputs = @(Resolve-TextureInputs $BaseColorImage $requiredTextureLayers.BaseColor "baseColor")
$normalInputs = @(Resolve-TextureInputs $NormalImage $requiredTextureLayers.Normal "normal")
$roughnessInputs = @(Resolve-TextureInputs $RoughnessImage $requiredTextureLayers.Orm "roughness/ORM")
$metallicInputs = @(Resolve-TextureInputs $MetallicImage $requiredTextureLayers.Orm "metallic/ORM")
$emissiveInputs = @(Resolve-TextureInputs $EmissiveImage $requiredTextureLayers.Emissive "emissive")
$ormInputs = @()
for ($layer = 0; $layer -lt $requiredTextureLayers.Orm; ++$layer) {
    $ormPng = Join-Path $outputRoot "conversion-orm-$layer.png"
    $temporaryPaths.Add($ormPng)
    & $blender --background --python (Join-Path $PSScriptRoot "compile-orm-texture.py") -- `
        $roughnessInputs[$layer] $metallicInputs[$layer] $ormPng
    if ($LASTEXITCODE -ne 0) { throw "Blender failed to compile ORM source layer $layer." }
    $ormInputs += $ormPng
}

$textureRecords = @()
$categories = @(
    @{ Name = "base-color"; Inputs = $baseColorInputs; Transfer = "srgb"; Windows = "R8G8B8A8_SRGB"; Android = "ASTC_6x6_SRGB_BLOCK" },
    @{ Name = "normal"; Inputs = $normalInputs; Transfer = "linear"; Windows = "R8G8B8A8_UNORM"; Android = "ASTC_4x4_UNORM_BLOCK" },
    @{ Name = "orm"; Inputs = $ormInputs; Transfer = "linear"; Windows = "R8G8B8A8_UNORM"; Android = "ASTC_6x6_UNORM_BLOCK" },
    @{ Name = "emissive"; Inputs = $emissiveInputs; Transfer = "srgb"; Windows = "R8G8B8A8_SRGB"; Android = "ASTC_6x6_SRGB_BLOCK" }
)
foreach ($category in $categories) {
    foreach ($platform in @("windows", "android")) {
        $format = if ($platform -eq "windows") { $category.Windows } else { $category.Android }
        $path = Join-Path $outputRoot "$($category.Name).$platform.ktx2"
        & (Join-Path $PSScriptRoot "compile-static-texture-array.ps1") `
            -InputPaths $category.Inputs -OutputPath $path -Format $format `
            -Transfer $category.Transfer -Width $TextureResolution -Height $TextureResolution
        $file = Get-Item -LiteralPath $path
        $textureRecords += [ordered]@{
            category = $category.Name
            platform = $platform
            format = $format
            layers = @($category.Inputs).Count
            width = $TextureResolution
            height = $TextureResolution
            mipmapped = $true
            bytes = $file.Length
            sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
            file = $file.Name
        }
    }
}

$sourceFile = Get-Item -LiteralPath $sourcePath
$runtimeFile = Get-Item -LiteralPath $runtimePath
$budgetReport = [ordered]@{
    schema = 1
    status = $RuntimeStatus
    source = [ordered]@{
        file = $sourceFile.Name
        bytes = $sourceFile.Length
        sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $sourcePath).Hash.ToLowerInvariant()
        validatorVersion = $sourceReport.validatorVersion
        reviewedWarningCodes = @($ReviewedValidatorIssueCodes)
    }
    runtime = [ordered]@{
        file = $runtimeFile.Name
        bytes = $runtimeFile.Length
        sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $runtimePath).Hash.ToLowerInvariant()
        vertices = [int64]$runtimeReport.info.totalVertexCount
        indices = $indexCount
        triangles = [int64]$runtimeReport.info.totalTriangleCount
        primitives = [int64]$runtimeReport.info.drawCallCount
        materials = [int64]$runtimeReport.info.materialCount
        maxAttributes = [int64]$runtimeReport.info.maxAttributes
        sourceResolutionImagePayloads = $sourceResolutionImages.Count
        validatorVersion = $runtimeReport.validatorVersion
    }
    textures = $textureRecords
}
$budgetPath = Join-Path $outputRoot "runtime-budget.json"
$budgetReport | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $budgetPath -Encoding utf8
$manifestDestination = Join-Path $outputRoot "asset.manifest.json"
if ([IO.Path]::GetFullPath($manifestPath) -ne [IO.Path]::GetFullPath($manifestDestination)) {
    Copy-Item -LiteralPath $manifestPath -Destination $manifestDestination -Force
}
foreach ($temporaryPath in $temporaryPaths) {
    if (Test-Path -LiteralPath $temporaryPath) { Remove-Item -LiteralPath $temporaryPath -Force }
}
Write-Output "Prepared static RT asset ($RuntimeStatus): $runtimePath"
Write-Output "Validator reports: $sourceReportPath ; $runtimeReportPath"
Write-Output "Runtime budget report: $budgetPath"
