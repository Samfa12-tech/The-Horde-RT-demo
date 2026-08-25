[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$SourceGlb,
    [Parameter(Mandatory = $true)][string]$Manifest,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [string]$BaseColorImage = "",
    [string]$NormalImage = "",
    [string]$RoughnessImage = "",
    [string]$MetallicImage = "",
    [string]$EmissiveImage = "",
    [string[]]$ReviewedValidatorIssueCodes = @(),
    [int]$TextureResolution = 512,
    [switch]$SkipTextureCompilation
)

$ErrorActionPreference = "Stop"
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$sourcePath = [IO.Path]::GetFullPath((Join-Path $repoRoot $SourceGlb))
$manifestPath = [IO.Path]::GetFullPath((Join-Path $repoRoot $Manifest))
$outputRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot $OutputDirectory))
if (-not (Test-Path -LiteralPath $sourcePath)) { throw "Static RT source GLB is missing: $sourcePath" }
if (-not (Test-Path -LiteralPath $manifestPath)) { throw "Static RT asset manifest is missing: $manifestPath" }
if ($TextureResolution -lt 1) { throw "TextureResolution must be greater than zero." }
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

$sourceReportPath = Join-Path $outputRoot "validator-source.json"
$sourceReport = Invoke-Validator $sourcePath $sourceReportPath $ReviewedValidatorIssueCodes

$runtimeName = ([IO.Path]::GetFileNameWithoutExtension($sourcePath)) + ".runtime.glb"
$runtimePath = Join-Path $outputRoot $runtimeName
if ($SkipTextureCompilation) {
    Copy-Item -LiteralPath $sourcePath -Destination $runtimePath -Force
} else {
    $blender = "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe"
    if (-not (Test-Path -LiteralPath $blender)) { throw "Blender 5.2 is required for audited tangent/socket conversion." }
    $socketGlb = Join-Path $outputRoot "conversion-with-socket.glb"
    # Blender Z-up -0.78 exports as glTF +Y-up -0.78, matching the hilt.
    & $blender --background --python (Join-Path $PSScriptRoot "add-gltf-socket.py") -- `
        $sourcePath $socketGlb grip 0 0 -0.78
    if ($LASTEXITCODE -ne 0) { throw "Blender tangent/socket conversion failed." }
    Copy-Item -LiteralPath $socketGlb -Destination $runtimePath -Force
}

$runtimeReportPath = Join-Path $outputRoot "validator-runtime.json"
$runtimeReport = Invoke-Validator $runtimePath $runtimeReportPath $ReviewedValidatorIssueCodes
$manifestData = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
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
$sourceResolutionImages = @($runtimeReport.info.resources | Where-Object {
    $_.image -and ($_.image.width -gt 4 -or $_.image.height -gt 4)
})
if (-not $SkipTextureCompilation -and $sourceResolutionImages.Count -ne 0) {
    throw "Runtime GLB still contains source-resolution image payloads."
}

$textureRecords = @()
if (-not $SkipTextureCompilation) {
    $textureInputs = @($BaseColorImage, $NormalImage, $RoughnessImage, $MetallicImage, $EmissiveImage)
    if (@($textureInputs | Where-Object { [string]::IsNullOrWhiteSpace($_) }).Count -ne 0) {
        throw "BaseColorImage, NormalImage, RoughnessImage, MetallicImage, and EmissiveImage are required."
    }
    $resolvedInputs = @($textureInputs | ForEach-Object { [IO.Path]::GetFullPath((Join-Path $repoRoot $_)) })
    foreach ($path in $resolvedInputs) {
        if (-not (Test-Path -LiteralPath $path)) { throw "Static RT texture source is missing: $path" }
    }
    $ormPng = Join-Path $outputRoot "conversion-orm.png"
    & $blender --background --python (Join-Path $PSScriptRoot "compile-orm-texture.py") -- `
        $resolvedInputs[2] $resolvedInputs[3] $ormPng
    if ($LASTEXITCODE -ne 0) { throw "Blender failed to compile the ORM source image." }

    $ktx = (Get-Command ktx -ErrorAction SilentlyContinue).Source
    if ([string]::IsNullOrWhiteSpace($ktx)) {
        $ktx = "C:\Users\sam_s\Documents\Codex\shared-tools\KTX-Software-4.4.2-src\build-local\Release\ktx.exe"
    }
    if (-not (Test-Path -LiteralPath $ktx)) { throw "KTX-Software 4.4.2 is required for runtime texture compilation." }
    $categories = @(
        @{ Name = "base-color"; Input = $resolvedInputs[0]; Transfer = "srgb"; Windows = "R8G8B8A8_SRGB"; Android = "ASTC_6x6_SRGB_BLOCK" },
        @{ Name = "normal"; Input = $resolvedInputs[1]; Transfer = "linear"; Windows = "R8G8B8A8_UNORM"; Android = "ASTC_4x4_UNORM_BLOCK" },
        @{ Name = "orm"; Input = $ormPng; Transfer = "linear"; Windows = "R8G8B8A8_UNORM"; Android = "ASTC_6x6_UNORM_BLOCK" },
        @{ Name = "emissive"; Input = $resolvedInputs[4]; Transfer = "srgb"; Windows = "R8G8B8A8_SRGB"; Android = "ASTC_6x6_SRGB_BLOCK" }
    )
    foreach ($category in $categories) {
        foreach ($platform in @("windows", "android")) {
            $format = if ($platform -eq "windows") { $category.Windows } else { $category.Android }
            $path = Join-Path $outputRoot "$($category.Name).$platform.ktx2"
            & $ktx create --testrun --format $format --layers 1 --width $TextureResolution --height $TextureResolution `
                --generate-mipmap --assign-tf $category.Transfer $category.Input $path
            if ($LASTEXITCODE -ne 0) { throw "KTX2 compilation failed for $($category.Name) $platform." }
            & $ktx validate $path
            if ($LASTEXITCODE -ne 0) { throw "KTX2 validation failed for $path" }
            $file = Get-Item -LiteralPath $path
            $textureRecords += [ordered]@{
                category = $category.Name
                platform = $platform
                format = $format
                layers = 1
                width = $TextureResolution
                height = $TextureResolution
                mipmapped = $true
                bytes = $file.Length
                sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
                file = $file.Name
            }
        }
    }
}

$sourceFile = Get-Item -LiteralPath $sourcePath
$runtimeFile = Get-Item -LiteralPath $runtimePath
$budgetReport = [ordered]@{
    schema = 1
    status = "development-only-license-unresolved"
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
Copy-Item -LiteralPath $manifestPath -Destination (Join-Path $outputRoot "asset.manifest.json") -Force
foreach ($temporaryName in @("conversion-with-socket.glb", "conversion-orm.png")) {
    $temporaryPath = Join-Path $outputRoot $temporaryName
    if (Test-Path -LiteralPath $temporaryPath) { Remove-Item -LiteralPath $temporaryPath -Force }
}
Write-Output "Prepared development-only static RT asset: $runtimePath"
Write-Output "Validator reports: $sourceReportPath ; $runtimeReportPath"
Write-Output "Runtime budget report: $budgetPath"
