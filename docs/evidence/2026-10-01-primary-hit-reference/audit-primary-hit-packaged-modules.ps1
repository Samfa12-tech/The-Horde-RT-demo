[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$auditDir = Join-Path $root 'audit-primary-hit-packaged-20261001-retry2'
if (Test-Path -LiteralPath $auditDir) { throw "Refusing to overwrite audit output: $auditDir" }
$vulkanBin = 'C:\VulkanSDK\1.4.350.0\Bin'
$val = Join-Path $vulkanBin 'spirv-val.exe'
$dis = Join-Path $vulkanBin 'spirv-dis.exe'
foreach ($tool in @($val, $dis)) {
    if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) { throw "Missing SPIR-V tool: $tool" }
}
Add-Type -AssemblyName System.IO.Compression.FileSystem

$expected = @{
    control = @{
        apk = 'artifacts\control\HordeLanternRT-primary-hit-normal-control-arm64.apk'
        apkSha = 'a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033'
        nativeSha = '41b34e601fa486e5cb5eeea24dd31df9771badb485b7cb74530344637644b049'
        containment = 'artifacts\control\containment.json'
        pipelineCatalog = 'artifacts\control\raygen-variant-catalog.json'
        computeCatalog = 'artifacts\control\rayquery-variant-catalog.json'
    }
    reference = @{
        apk = 'artifacts\primary-hit-probe\HordeLanternRT-primary-hit-shipping-mobile-benchmark-arm64.apk'
        apkSha = '40d0f759dca40ff8433ce4aca19156a135a9146a960f115f4cdbb78d3db7cf9e'
        nativeSha = 'f094974a32c6c20a7e3a0407e30635476b7e4f13d8f8de073ab95e4c2b9c3019'
        containment = 'artifacts\primary-hit-probe\package-containment.json'
        pipelineCatalog = 'artifacts\primary-hit-probe\raygen-variant-catalog.json'
        computeCatalog = 'artifacts\primary-hit-probe\rayquery-variant-catalog.json'
    }
}
$expectedKeys = @(
    'shipping_mobile_opaque_fast', 'shipping_mobile_generic_dielectric',
    'rayquery_compute_shipping_mobile_opaque_fast', 'rayquery_compute_shipping_mobile_generic_dielectric'
)
$sourcePatchPath = Join-Path $root 'artifacts\primary-hit-probe\source-diff.patch'
$sourceHeaderPath = Join-Path $root 'artifacts\primary-hit-probe\rt_primary_hit_reference.glsl'
$sourcePatchSha = (Get-FileHash -LiteralPath $sourcePatchPath -Algorithm SHA256).Hash.ToLowerInvariant()
$sourceHeaderSha = (Get-FileHash -LiteralPath $sourceHeaderPath -Algorithm SHA256).Hash.ToLowerInvariant()
if ($sourcePatchSha -cne '4500f823836f19123fd85b38fb4dd0220d11b96c84decb4e7b57280c0c74a277' -or
    $sourceHeaderSha -cne 'bca3e4bb9f7c7c4aec176c38dcef5f988f94434d078db1cc77ca58705ab9cadb') {
    throw 'Frozen primary-hit source patch/header hash mismatch.'
}
$assetComparison = Get-Content -LiteralPath (Join-Path $root 'artifacts\primary-hit-probe\asset-shader-comparison.json') -Raw | ConvertFrom-Json
if ($assetComparison.inputs.baselineApkSha256 -cne $expected.control.apkSha) { throw 'Asset comparison control APK pin mismatch.' }
if ($assetComparison.inputs.candidateApkSha256 -cne $expected.reference.apkSha) { throw 'Asset comparison reference APK pin mismatch.' }
if ($assetComparison.assetSummary.baselineCount -ne 53 -or $assetComparison.assetSummary.candidateCount -ne 53 -or
    $assetComparison.assetSummary.identicalCount -ne 53 -or $assetComparison.assetSummary.changedCount -ne 0) {
    throw 'Frozen asset comparison does not prove 53/53 identical payloads.'
}

New-Item -ItemType Directory -Path $auditDir | Out-Null
$artifactRows = @()
foreach ($kind in @('control', 'reference')) {
    $pins = $expected[$kind]
    $artifactRoot = Join-Path $root "artifacts\$kind"
    $apk = Join-Path $root $pins.apk
    if (-not (Test-Path -LiteralPath $apk -PathType Leaf)) { throw "Missing $kind sealed APK: $apk" }
    $apkSha = (Get-FileHash -LiteralPath $apk -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($apkSha -cne $pins.apkSha) { throw "$kind APK SHA pin mismatch: $apkSha" }
    $containment = Get-Content -LiteralPath (Join-Path $root $pins.containment) -Raw | ConvertFrom-Json
    if ($containment.instrumentation -cne 'Shipping' -or $containment.quality -cne 'Mobile' -or
        $containment.arm64Sha256 -cne $pins.nativeSha -or $containment.packaged.modules.Count -ne 4) {
        throw "$kind containment receipt does not match the expected Shipping/Mobile ARM64 package."
    }
    $catalogs = @{
        RayTracingPipeline = (Get-Content -LiteralPath (Join-Path $root $pins.pipelineCatalog) -Raw | ConvertFrom-Json).variants
        RayQueryCompute = (Get-Content -LiteralPath (Join-Path $root $pins.computeCatalog) -Raw | ConvertFrom-Json).variants
    }
    foreach ($backend in @('RayTracingPipeline','RayQueryCompute')) {
        $rows = @($catalogs[$backend] | Where-Object {
            $_.instrumentation -ceq 'Shipping' -and $_.quality -ceq 'Mobile'
        })
        if ($rows.Count -ne 2) { throw "$kind $backend catalog must contain exactly two Shipping/Mobile strategy rows." }
    }

    $zip = [IO.Compression.ZipFile]::OpenRead($apk)
    try {
        $nativeEntries = @($zip.Entries | Where-Object FullName -CEQ 'lib/arm64-v8a/libhorde_rt_probe_android.so')
        if ($nativeEntries.Count -ne 1) { throw "$kind APK has $($nativeEntries.Count) ARM64 native library entries." }
        $stream = $nativeEntries[0].Open()
        try {
            $memory = [IO.MemoryStream]::new()
            try { $stream.CopyTo($memory); $nativeBytes = $memory.ToArray() }
            finally { $memory.Dispose() }
        } finally { $stream.Dispose() }
    } finally { $zip.Dispose() }
    $nativeSha = ([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($nativeBytes))).ToLowerInvariant()
    if ($nativeSha -cne $pins.nativeSha -or $nativeSha -cne $containment.packaged.targetSha256) {
        throw "$kind extracted native module hash does not match the immutable build/containment pins."
    }

    $rows = @()
    foreach ($module in $containment.packaged.modules) {
        $offset = [int64]$module.offset
        $length = [int64]$module.words * 4
        if ($module.words -le 5 -or $offset -lt 0 -or ($offset + $length) -gt $nativeBytes.LongLength) {
            throw "$kind packaged module bounds invalid at offset=$offset words=$($module.words)."
        }
        $bytes = [byte[]]::new([int]$length)
        [Array]::Copy($nativeBytes, [int]$offset, $bytes, 0, [int]$length)
        $sha = ([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes))).ToLowerInvariant()
        if ($sha -cne $module.sha256) { throw "$kind module SHA differs from actual containment at offset=$offset." }
        $backend = [string]$module.backend
        if ($backend -cnotin @('RayTracingPipeline','RayQueryCompute')) { throw "Unknown module backend $backend" }
        $catalogMatch = @($catalogs[$backend] | Where-Object {
            $_.spirvSha256 -ceq $sha -and $_.instrumentation -ceq 'Shipping' -and $_.quality -ceq 'Mobile'
        })
        if ($catalogMatch.Count -ne 1) { throw "$kind module $sha does not resolve uniquely in frozen $backend catalog." }
        $entry = $catalogMatch[0]
        $key = [string]$entry.key
        if ($key -notin $expectedKeys -or $key -notin $containment.packaged.semanticKeys) {
            throw "$kind module maps to unexpected semantic key $key."
        }
        $outSpv = Join-Path $auditDir "$kind-$key.spv"
        $tmpAsm = Join-Path $auditDir "$kind-$key.spvasm.tmp"
        $valLog = Join-Path $auditDir "$kind-$key-spirv-val.log"
        [IO.File]::WriteAllBytes($outSpv, $bytes)
        $valOutput = & $val --target-env vulkan1.2 $outSpv 2>&1
        $valExit = $LASTEXITCODE
        @("command=$val --target-env vulkan1.2 $outSpv", "exitCode=$valExit", 'result=passed') |
            Set-Content -LiteralPath $valLog -Encoding utf8
        if ($valExit -ne 0) { throw "spirv-val failed for $kind/$key exit=$valExit (see $valLog)." }
        $disOutput = & $dis $outSpv -o $tmpAsm 2>&1
        $disExit = $LASTEXITCODE
        if ($disExit -ne 0 -or -not (Test-Path -LiteralPath $tmpAsm -PathType Leaf)) {
            ($disOutput | Out-String) | Set-Content -LiteralPath (Join-Path $auditDir "$kind-$key-spirv-dis.log")
            throw "spirv-dis failed for $kind/$key exit=$disExit."
        }
        @("command=$dis $outSpv -o <temporary-disassembly>", "exitCode=$disExit", 'result=passed; temporary disassembly removed') |
            Set-Content -LiteralPath (Join-Path $auditDir "$kind-$key-spirv-dis.log") -Encoding utf8
        $text = [IO.File]::ReadAllText($tmpAsm)
        # SPIR-V atomic instructions may omit a result ID (e.g. OpAtomicStore/FlagClear).
        $atomics = [regex]::Matches($text, '(?m)^\s*(?:%\S+\s*=\s*)?OpAtomic\w+\b').Count
        $imageReads = [regex]::Matches($text, '(?m)^\s*(?:%\S+\s*=\s*)?OpImageRead\b').Count
        $binding22 = [regex]::IsMatch($text, '(?m)^\s*OpDecorate\s+%\S+\s+Binding\s+22\s*$')
        $queryInitializations = [regex]::Matches($text, '\bOpRayQueryInitializeKHR\b').Count
        $instructionCount = [regex]::Matches($text, '(?m)^\s*(?:%\S+\s*=\s*)?Op\w+\b').Count
        Remove-Item -LiteralPath $tmpAsm
        if ($atomics -ne 0 -or $imageReads -ne 0 -or $binding22) { throw "$kind/$key violates Shipping diagnostic containment." }
        if ($queryInitializations -ne [int]$entry.rayQueryInitializations) { throw "$kind/$key ray-query initialization count differs from frozen catalog." }
        $rows += [pscustomobject]@{
            semanticKey = $key; backend = $backend; executionModel = [string]$module.executionModel
            sha256 = $sha; words = [int]$module.words; disassemblyInstructionCount = $instructionCount
            rayQueryInitializations = $queryInitializations; atomicsIncludingNoResult = $atomics
            imageReads = $imageReads; binding22 = $binding22; spirvValExitCode = $valExit
            spirvDisExitCode = $disExit; spirvValLog = [IO.Path]::GetFileName($valLog)
            catalogFunctions = [int]$entry.functions; catalogFunctionCalls = [int]$entry.functionCalls
            catalogInstructions = [int]$entry.instructions; catalogLoops = [int]$entry.loops
            catalogBranchOperations = [int]$entry.branchOperations
        }
    }
    if ($rows.Count -ne 4 -or @($rows.semanticKey | Sort-Object -Unique).Count -ne 4) {
        throw "$kind audit did not resolve exactly four distinct expected packaged modules."
    }
    $artifactRows += [pscustomobject]@{ role = $kind; apkSha256 = $apkSha; nativeLibrarySha256 = $nativeSha; modules = $rows }
}

$controlModules = @{}; foreach ($m in $artifactRows[0].modules) { $controlModules[$m.semanticKey] = $m }
$referenceModules = @{}; foreach ($m in $artifactRows[1].modules) { $referenceModules[$m.semanticKey] = $m }
if ($controlModules.Count -ne 4 -or $referenceModules.Count -ne 4) { throw 'Expected four module mappings for both artifacts.' }
$changed = @()
foreach ($key in $expectedKeys) {
    if (-not $controlModules.ContainsKey($key) -or -not $referenceModules.ContainsKey($key)) { throw "Missing packaged module $key." }
    if ($controlModules[$key].sha256 -cne $referenceModules[$key].sha256) { $changed += $key }
}
$changedSet = @($changed | Sort-Object) -join ','
$expectedSet = @($expectedKeys | Sort-Object) -join ','
if ($changedSet -cne $expectedSet) {
    throw "Unexpected pipeline/compute module delta set: $($changed -join ',')."
}

$footprint = @()
foreach ($key in @('shipping_mobile_opaque_fast','rayquery_compute_shipping_mobile_opaque_fast')) {
    $a = $controlModules[$key]; $b = $referenceModules[$key]
    $footprint += [pscustomobject]@{
        semanticKey = $key; control = [pscustomobject]@{
            sha256 = $a.sha256; words = $a.words; functions = $a.catalogFunctions; calls = $a.catalogFunctionCalls
            instructions = $a.catalogInstructions; loops = $a.catalogLoops; branchOperations = $a.catalogBranchOperations
            rayQueryInitializations = $a.rayQueryInitializations
        }; primaryHitReference = [pscustomobject]@{
            sha256 = $b.sha256; words = $b.words; functions = $b.catalogFunctions; calls = $b.catalogFunctionCalls
            instructions = $b.catalogInstructions; loops = $b.catalogLoops; branchOperations = $b.catalogBranchOperations
            rayQueryInitializations = $b.rayQueryInitializations
        }; interpretation = 'Static packaged-module footprint only; not dynamic work or runtime cost.'
    }
}

$receipt = [pscustomobject]@{
    schema = 1; audit = 'actual-sealed-APK-packaged-SPIR-V'; sourceHead = '71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95'
    classification = 'nonphysical-primary-hit-reference-investigation-only'
    scriptSha256 = (Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256).Hash.ToLowerInvariant()
    sourceFreeze = [pscustomobject]@{ patchSha256 = $sourcePatchSha; referenceHeaderSha256 = $sourceHeaderSha }
    apkZipAssetComparison = [pscustomobject]@{ payloadEntriesEach = 53; identicalPayloads = 53; changedPayloads = 0 }
    auditRules = [pscustomobject]@{
        atomicMatcherIncludesNoResultOpcodes = $true; shippingAtomicCount = 0; opImageReadCount = 0
        binding22 = $false; moduleCountEach = 4; spirvTargetEnv = 'vulkan1.2'
        disassemblyTempFilesRemoved = $true; nativeELFRetained = $false
    }
    changedSemanticKeys = @($changed | Sort-Object); artifacts = $artifactRows; staticFootprint = $footprint
    limitation = 'Footprint comparisons are static counts only; they do not estimate dynamic work, time, throughput, or additive decomposition. The primary-hit reference is nonphysical and excludes substantial shading paths.'
}
$receiptPath = Join-Path $auditDir 'packaged-primary-hit-module-audit.json'
$receipt | ConvertTo-Json -Depth 9 | Set-Content -LiteralPath $receiptPath -Encoding utf8
Write-Output "Packaged primary-hit audit passed: 2 APKs, 8 actual SPIR-V modules, 4 expected changed keys."
Write-Output "Receipt: $receiptPath"
