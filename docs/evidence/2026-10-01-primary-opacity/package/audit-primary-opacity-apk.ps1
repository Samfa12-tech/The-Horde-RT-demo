[CmdletBinding()]
param(
    [string]$ControlApk = 'C:\Dev\tmp\horde-mobile-lantern-profile-20261001\control-complete-assets-shipping-mobile.apk',
    [string]$ControlApkSha256 = '3a4faf72923e24da9da859ef6cd71c221b818b97d7418a75b1c8dd0ca0540378',
    [string]$CandidateApk,
    [string]$CandidateApkSha256,
    [string]$RepoRoot = 'C:\Users\sam_s\.codex\worktrees\horde-mobile-lantern-profile\the Horde RT Demo'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Add-Type -AssemblyName System.IO.Compression.FileSystem

$vulkanBin = 'C:\VulkanSDK\1.4.350.0\Bin'
$spirvVal = Join-Path $vulkanBin 'spirv-val.exe'
$spirvDis = Join-Path $vulkanBin 'spirv-dis.exe'
foreach ($toolPath in @($spirvVal, $spirvDis)) {
    if (-not (Test-Path -LiteralPath $toolPath -PathType Leaf)) {
        throw "Missing SPIR-V tool: $toolPath"
    }
}

if (([string]::IsNullOrWhiteSpace($CandidateApk)) -ne
    ([string]::IsNullOrWhiteSpace($CandidateApkSha256))) {
    throw 'Provide both CandidateApk and CandidateApkSha256, or neither.'
}
$hasCandidate = -not [string]::IsNullOrWhiteSpace($CandidateApk)
$runLabel = if ($hasCandidate) {
    'pair-v3-' + $ControlApkSha256.Substring(0, 12) + '-' + $CandidateApkSha256.Substring(0, 12)
} else {
    'control-' + $ControlApkSha256.Substring(0, 12)
}
$auditDir = Join-Path $PSScriptRoot ('audit-' + $runLabel)
if (Test-Path -LiteralPath $auditDir) { throw "Refusing to overwrite audit output: $auditDir" }

$raygenCatalogPath = Join-Path $RepoRoot 'tools/raygen-variant-catalog.json'
$computeCatalogPath = Join-Path $RepoRoot 'tools/rayquery-variant-catalog.json'
$catalogPins = @{
    raygen = 'b6228ddbfd01f91db8badfce1b25c016325580c9f8b7f1ca99e6ca4a5d52516a'
    compute = 'ce42a8455cd065b59f6a6ed5602241b667094a17c59639bde862eb8bf0728af3'
}
foreach ($entry in @(
    @{ name = 'raygen'; path = $raygenCatalogPath },
    @{ name = 'compute'; path = $computeCatalogPath }
)) {
    if (-not (Test-Path -LiteralPath $entry.path -PathType Leaf)) {
        throw "Missing current frozen catalog: $($entry.path)"
    }
    $actualHash = (Get-FileHash -LiteralPath $entry.path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actualHash -cne $catalogPins[$entry.name]) {
        throw "$($entry.name) frozen catalog hash changed: $actualHash"
    }
}
$raygenCatalog = Get-Content -LiteralPath $raygenCatalogPath -Raw | ConvertFrom-Json
$computeCatalog = Get-Content -LiteralPath $computeCatalogPath -Raw | ConvertFrom-Json
if ($raygenCatalog.status -cne 'frozen' -or $computeCatalog.status -cne 'frozen') {
    throw 'The current raygen and compute catalogs must both be frozen.'
}
$catalogs = @{
    RayTracingPipeline = $raygenCatalog.variants
    RayQueryCompute = $computeCatalog.variants
}
$expectedKeys = @(
    'shipping_mobile_opaque_fast', 'shipping_mobile_generic_dielectric',
    'rayquery_compute_shipping_mobile_opaque_fast',
    'rayquery_compute_shipping_mobile_generic_dielectric'
)
$expectedRows = @()
foreach ($backend in @('RayTracingPipeline', 'RayQueryCompute')) {
    $rows = @($catalogs[$backend] | Where-Object {
        $_.instrumentation -ceq 'Shipping' -and $_.quality -ceq 'Mobile'
    })
    if ($rows.Count -ne 2) { throw "$backend catalog must contain exactly two Shipping/Mobile strategies." }
    $expectedRows += $rows
}
if (@($expectedRows.key | Sort-Object -Unique).Count -ne 4 -or
    @($expectedRows.key | Where-Object { $_ -notin $expectedKeys }).Count -ne 0) {
    throw 'Frozen catalogs do not contain exactly the four expected Shipping/Mobile keys.'
}
foreach ($row in $expectedRows) {
    if (-not $row.shippingAllowed -or $row.atomicInstructions -ne 0 -or
        $row.hasDiagnosticsBinding -ne $false -or [int64]$row.bytes -ne ([int64]$row.words * 4)) {
        throw "Frozen catalog entry is not an admitted Shipping module: $($row.key)"
    }
}

function Get-ZipEntrySha256([System.IO.Compression.ZipArchiveEntry]$Entry) {
    $stream = $Entry.Open()
    try {
        $sha = [Security.Cryptography.SHA256]::Create()
        try { return ([Convert]::ToHexString($sha.ComputeHash($stream))).ToLowerInvariant() }
        finally { $sha.Dispose() }
    } finally { $stream.Dispose() }
}

function Get-SpirvFunctionEndLength([byte[]]$Bytes, [int]$StartOffset) {
    $wordIndex = 5
    $maximumWords = [int][Math]::Floor(($Bytes.Length - $StartOffset) / 4)
    while ($wordIndex -lt $maximumWords) {
        $instruction = [BitConverter]::ToUInt32($Bytes, $StartOffset + ($wordIndex * 4))
        $wordCount = [int]($instruction -shr 16)
        $opcode = [int]($instruction -band 0xffff)
        if ($wordCount -lt 1 -or ($wordIndex + $wordCount) -gt $maximumWords) {
            throw "Malformed SPIR-V instruction while locating module end at offset $StartOffset."
        }
        $wordIndex += $wordCount
        if ($opcode -eq 56) { return $wordIndex * 4 }
    }
    throw "No OpFunctionEnd found while locating SPIR-V module end at offset $StartOffset."
}

function Get-ApkPackageFacts([string]$Role, [string]$Path, [string]$ExpectedSha, [string]$Destination) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Missing $Role APK: $Path" }
    $apkSha = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($apkSha -cne $ExpectedSha.ToLowerInvariant()) { throw "$Role APK SHA-256 mismatch: $apkSha" }

    $zip = [IO.Compression.ZipFile]::OpenRead($Path)
    try {
        $assetEntries = @($zip.Entries | Where-Object {
            $_.FullName.StartsWith('assets/', [StringComparison]::Ordinal) -and
            -not $_.FullName.EndsWith('/', [StringComparison]::Ordinal)
        })
        if ($assetEntries.Count -eq 0) { throw "$Role APK contains no assets/ payload entries." }
        $duplicateAssetNames = @($assetEntries | Group-Object FullName | Where-Object Count -ne 1)
        if ($duplicateAssetNames.Count -ne 0) { throw "$Role APK contains duplicate asset paths." }

        $assets = @()
        foreach ($entry in $assetEntries) {
            $stream = $entry.Open()
            try {
                $prefix = [byte[]]::new([int][Math]::Min(64, $entry.Length))
                if ($prefix.Length -gt 0) {
                    $read = $stream.Read($prefix, 0, $prefix.Length)
                    if ($read -ne $prefix.Length) { throw "Could not read asset prefix: $($entry.FullName)" }
                }
            } finally { $stream.Dispose() }
            $prefixText = [Text.Encoding]::ASCII.GetString($prefix)
            if ($prefixText.StartsWith('version https://git-lfs.github.com/spec/v1', [StringComparison]::Ordinal)) {
                throw "$Role APK contains a Git LFS pointer instead of an asset: $($entry.FullName)"
            }
            $assets += [pscustomobject]@{
                path = $entry.FullName
                bytes = [int64]$entry.Length
                sha256 = Get-ZipEntrySha256 $entry
            }
        }

        $nativeEntryName = 'lib/arm64-v8a/libhorde_rt_probe_android.so'
        $nativeEntries = @($zip.Entries | Where-Object FullName -CEQ $nativeEntryName)
        if ($nativeEntries.Count -ne 1) { throw "$Role APK has $($nativeEntries.Count) ARM64 native entries." }
        $nativeStream = $nativeEntries[0].Open()
        try {
            $memory = [IO.MemoryStream]::new()
            try { $nativeStream.CopyTo($memory); $nativeBytes = $memory.ToArray() }
            finally { $memory.Dispose() }
        } finally { $nativeStream.Dispose() }
    } finally { $zip.Dispose() }

    $nativeSha = ([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($nativeBytes))).ToLowerInvariant()
    $magicOffsets = [System.Collections.Generic.List[int]]::new()
    for ($offset = 0; $offset -le $nativeBytes.Length - 4; $offset += 4) {
        if ($nativeBytes[$offset] -eq 0x03 -and $nativeBytes[$offset + 1] -eq 0x02 -and
            $nativeBytes[$offset + 2] -eq 0x23 -and $nativeBytes[$offset + 3] -eq 0x07) {
            $magicOffsets.Add($offset)
        }
    }
    $modules = @()
    foreach ($row in $expectedRows) {
        $moduleLength = [int64]$row.bytes
        $matches = @()
        foreach ($offset in $magicOffsets) {
            if ($offset + $moduleLength -gt $nativeBytes.LongLength) { continue }
            $moduleBytes = [byte[]]::new([int]$moduleLength)
            [Array]::Copy($nativeBytes, $offset, $moduleBytes, 0, [int]$moduleLength)
            $moduleSha = ([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($moduleBytes))).ToLowerInvariant()
            if ($moduleSha -ceq [string]$row.spirvSha256) {
                $matches += [pscustomobject]@{ offset = $offset; bytes = $moduleBytes; sha256 = $moduleSha }
            }
        }
        if ($matches.Count -ne 1) { throw "$Role actual binary has $($matches.Count) exact catalog matches for $($row.key)." }

        $match = $matches[0]
        $spvPath = Join-Path $Destination ($Role + '-' + [string]$row.key + '.spv')
        [IO.File]::WriteAllBytes($spvPath, $match.bytes)
        $asmPath = Join-Path $Destination ($Role + '-' + [string]$row.key + '.spvasm.tmp')
        $valOutput = & $spirvVal --target-env vulkan1.2 $spvPath 2>&1
        $valExit = $LASTEXITCODE
        if ($valExit -ne 0) { throw "spirv-val failed for $Role/$($row.key): $($valOutput | Out-String)" }
        $disOutput = & $spirvDis $spvPath -o $asmPath 2>&1
        $disExit = $LASTEXITCODE
        if ($disExit -ne 0 -or -not (Test-Path -LiteralPath $asmPath -PathType Leaf)) {
            throw "spirv-dis failed for $Role/$($row.key): $($disOutput | Out-String)"
        }
        $assembly = [IO.File]::ReadAllText($asmPath)
        Remove-Item -LiteralPath $asmPath
        $atomics = [regex]::Matches($assembly, '(?m)^\s*(?:%\S+\s*=\s*)?OpAtomic\w+\b').Count
        $imageReads = [regex]::Matches($assembly, '(?m)^\s*(?:%\S+\s*=\s*)?OpImageRead\b').Count
        $binding22 = [regex]::IsMatch($assembly, '(?m)^\s*OpDecorate\s+%\S+\s+Binding\s+22\s*$')
        $queryInitializations = [regex]::Matches($assembly, '\bOpRayQueryInitializeKHR\b').Count
        $instructionCount = [regex]::Matches($assembly, '(?m)^\s*(?:%\S+\s*=\s*)?Op\w+\b').Count
        if ($atomics -ne 0 -or $binding22 -or $imageReads -ne 0) {
            throw "$Role/$($row.key) violates Shipping diagnostic containment."
        }
        if ($queryInitializations -ne [int]$row.rayQueryInitializations) {
            throw "$Role/$($row.key) query initialization count differs from the frozen catalog."
        }
        $backendProperty = $row.PSObject.Properties['executionBackend']
        $modelProperty = $row.PSObject.Properties['executionModel']
        $moduleBackend = if ($null -ne $backendProperty) { [string]$backendProperty.Value } else { 'RayTracingPipeline' }
        $moduleExecutionModel = if ($null -ne $modelProperty) { [string]$modelProperty.Value } else { 'RayGenerationKHR' }
        $modules += [pscustomobject]@{
            key = [string]$row.key
            backend = $moduleBackend
            executionModel = $moduleExecutionModel
            offsetInArm64Library = [int64]$match.offset
            bytes = [int64]$row.bytes
            sha256 = $match.sha256
            catalogKey = [string]$row.key
            words = [int]$row.words
            instructions = $instructionCount
            rayQueryInitializations = $queryInitializations
            atomicsIncludingNoResultOpcodes = $atomics
            imageReads = $imageReads
            binding22 = $binding22
            spirvVal = 'passed'
            spirvDis = 'passed'
            extractedPath = [IO.Path]::GetFileName($spvPath)
        }
    }
    if ($modules.Count -ne 4 -or @($modules.key | Sort-Object -Unique).Count -ne 4) {
        throw "$Role audit did not extract four unique Shipping/Mobile catalog modules."
    }
    $variableOffsets = @($modules | ForEach-Object { [int]$_.offsetInArm64Library })
    $sharedOffsets = @($magicOffsets | Where-Object { $_ -notin $variableOffsets } | Sort-Object)
    if ($magicOffsets.Count -ne 6 -or $sharedOffsets.Count -ne 2) {
        throw "$Role ARM64 library has $($magicOffsets.Count) aligned SPIR-V markers; expected four catalog modules and two shared RT stages."
    }
    $sharedStages = @()
    foreach ($stageIndex in 0..1) {
        $startOffset = [int]$sharedOffsets[$stageIndex]
        $moduleLength = if ($stageIndex -eq 0) {
            [int]($sharedOffsets[$stageIndex + 1] - $startOffset)
        } else {
            Get-SpirvFunctionEndLength $nativeBytes $startOffset
        }
        if ($moduleLength -le 20 -or ($moduleLength % 4) -ne 0 -or
            ($startOffset + $moduleLength) -gt $nativeBytes.Length) {
            throw "$Role shared RT stage bounds are invalid at offset $startOffset, bytes=$moduleLength."
        }
        $moduleBytes = [byte[]]::new($moduleLength)
        [Array]::Copy($nativeBytes, $startOffset, $moduleBytes, 0, $moduleLength)
        $spvPath = Join-Path $Destination ("$Role-shared-stage-$stageIndex.spv")
        [IO.File]::WriteAllBytes($spvPath, $moduleBytes)
        $asmPath = Join-Path $Destination ("$Role-shared-stage-$stageIndex.spvasm.tmp")
        $valOutput = & $spirvVal --target-env vulkan1.2 $spvPath 2>&1
        if ($LASTEXITCODE -ne 0) { throw "spirv-val failed for $Role shared RT stage ${stageIndex}: $($valOutput | Out-String)" }
        $disOutput = & $spirvDis $spvPath -o $asmPath 2>&1
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $asmPath -PathType Leaf)) {
            throw "spirv-dis failed for $Role shared RT stage ${stageIndex}: $($disOutput | Out-String)"
        }
        $assembly = [IO.File]::ReadAllText($asmPath)
        Remove-Item -LiteralPath $asmPath
        $model = if ($assembly -match '(?m)^\s*OpEntryPoint\s+MissKHR\b') {
            'MissKHR'
        } elseif ($assembly -match '(?m)^\s*OpEntryPoint\s+ClosestHitKHR\b') {
            'ClosestHitKHR'
        } else {
            throw "$Role extra embedded module at offset $startOffset is not a shared Miss/ClosestHit stage."
        }
        $atomics = [regex]::Matches($assembly, '(?m)^\s*(?:%\S+\s*=\s*)?OpAtomic\w+\b').Count
        $binding22 = [regex]::IsMatch($assembly, '(?m)^\s*OpDecorate\s+%\S+\s+Binding\s+22\s*$')
        if ($atomics -ne 0 -or $binding22) { throw "$Role shared $model stage contains Diagnostic instrumentation." }
        $sharedStages += [pscustomobject]@{
            executionModel = $model
            offsetInArm64Library = $startOffset
            bytes = $moduleLength
            sha256 = ([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($moduleBytes))).ToLowerInvariant()
            atomicsIncludingNoResultOpcodes = $atomics
            binding22 = $binding22
            spirvVal = 'passed'
            spirvDis = 'passed'
            extractedPath = [IO.Path]::GetFileName($spvPath)
        }
    }
    if (@($sharedStages.executionModel | Sort-Object -Unique) -join ',' -cne 'ClosestHitKHR,MissKHR') {
        throw "$Role did not contain exactly one shared Miss and one shared ClosestHit stage."
    }
    return [pscustomobject]@{
        role = $Role
        apkPath = [IO.Path]::GetFullPath($Path)
        apkSha256 = $apkSha
        arm64LibrarySha256 = $nativeSha
        arm64LibraryBytes = $nativeBytes.LongLength
        alignedSpirvMagicOffsetsFound = $magicOffsets.Count
        frozenCatalogHashes = [pscustomobject]$catalogPins
        assetEntryCount = $assets.Count
        assets = $assets
        modules = $modules
        sharedStages = $sharedStages
    }
}

New-Item -ItemType Directory -Path $auditDir | Out-Null
$control = Get-ApkPackageFacts 'control' $ControlApk $ControlApkSha256 $auditDir
$candidate = $null
$assetComparison = $null
$moduleComparison = $null
if ($hasCandidate) {
    $candidate = Get-ApkPackageFacts 'candidate' $CandidateApk $CandidateApkSha256 $auditDir
    if ($control.assetEntryCount -ne $candidate.assetEntryCount) {
        throw "APK asset counts differ: control=$($control.assetEntryCount), candidate=$($candidate.assetEntryCount)."
    }
    $controlAssets = @{}; foreach ($asset in $control.assets) { $controlAssets[$asset.path] = $asset }
    $changedAssets = @()
    foreach ($asset in $candidate.assets) {
        if (-not $controlAssets.ContainsKey($asset.path)) { $changedAssets += $asset.path; continue }
        $baseline = $controlAssets[$asset.path]
        if ($baseline.bytes -ne $asset.bytes -or $baseline.sha256 -cne $asset.sha256) {
            $changedAssets += $asset.path
        }
    }
    if ($changedAssets.Count -ne 0) { throw "APK assets differ: $($changedAssets -join ', ')" }
    $assetComparison = [pscustomobject]@{
        controlEntryCount = $control.assetEntryCount
        candidateEntryCount = $candidate.assetEntryCount
        identicalEntries = $control.assetEntryCount
        changedEntries = 0
        lfsPointers = 0
    }
    $controlModules = @{}; foreach ($module in $control.modules) { $controlModules[$module.key] = $module }
    $moduleChanges = @()
    foreach ($module in $candidate.modules) {
        if (-not $controlModules.ContainsKey($module.key) -or
            $controlModules[$module.key].sha256 -cne $module.sha256) {
            $moduleChanges += $module.key
        }
    }
    if ($moduleChanges.Count -ne 0) {
        throw "Shipping/Mobile shader modules changed between profile artifacts: $($moduleChanges -join ', ')"
    }
    $controlStages = @{}; foreach ($stage in $control.sharedStages) { $controlStages[$stage.executionModel] = $stage }
    $stageChanges = @()
    foreach ($stage in $candidate.sharedStages) {
        if (-not $controlStages.ContainsKey($stage.executionModel) -or
            $controlStages[$stage.executionModel].sha256 -cne $stage.sha256) {
            $stageChanges += $stage.executionModel
        }
    }
    if ($stageChanges.Count -ne 0) { throw "Shared RT stage modules changed: $($stageChanges -join ', ')" }
    $moduleComparison = [pscustomobject]@{
        actualShippingMobileModulesEach = 4
        unchangedSharedRtStagesEach = 2
        changedSemanticKeys = @()
        result = 'all four exact frozen Shipping/Mobile modules and both shared RT stages are byte-identical to control'
    }
}

$receipt = [pscustomobject]@{
    schema = 1
    audit = 'actual-APK-assets-and-embedded-ARM64-Shipping-Mobile-SPIR-V'
    repoRoot = [IO.Path]::GetFullPath($RepoRoot)
    currentCatalogStatus = 'frozen'
    catalogPins = [pscustomobject]$catalogPins
    assetComparison = $assetComparison
    moduleComparison = $moduleComparison
    artifacts = @($control) + @($candidate | Where-Object { $null -ne $_ })
    limits = 'Asset equality covers all non-directory APK assets/ ZIP entries. The four variable RT modules are byte-matched against frozen Shipping/Mobile catalog rows and freshly validated/disassembled from extracted ARM64-library bytes.'
}
$receiptPath = Join-Path $auditDir 'actual-apk-package-audit.json'
$receipt | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $receiptPath -Encoding utf8
if ($hasCandidate) {
    Write-Output "APK audit passed: asset entries=$($control.assetEntryCount) identical, 4 current Shipping/Mobile shader modules each, zero shader-module changes."
} else {
    Write-Output "Control APK audit passed: actual asset entries=$($control.assetEntryCount), four exact current Shipping/Mobile catalog modules validated and disassembled."
}
Write-Output "Receipt: $receiptPath"
