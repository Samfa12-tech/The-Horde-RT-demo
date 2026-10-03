[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = 'C:\Dev\tmp\horde-all-shadow-profile-20261001'
$vulkanBin = 'C:\VulkanSDK\1.4.350.0\Bin'
$val = Join-Path $vulkanBin 'spirv-val.exe'
$dis = Join-Path $vulkanBin 'spirv-dis.exe'
foreach ($tool in @($val, $dis)) {
    if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) { throw "Missing SPIR-V tool: $tool" }
}
Add-Type -AssemblyName System.IO.Compression.FileSystem
$all = @()
foreach ($kind in @('control', 'isolate')) {
    $dir = Join-Path $root "artifacts\$kind"
    $apk = Join-Path $dir "HordeLanternRT-all-shadow-$kind-benchmark-arm64.apk"
    $containment = Get-Content (Join-Path $dir 'containment.json') -Raw | ConvertFrom-Json
    $catalogs = @{
        RayTracingPipeline = (Get-Content (Join-Path $dir 'raygen-variant-catalog.json') -Raw | ConvertFrom-Json).variants
        RayQueryCompute = (Get-Content (Join-Path $dir 'rayquery-variant-catalog.json') -Raw | ConvertFrom-Json).variants
    }
    $moduleDir = Join-Path $dir 'actual-packaged-modules-audit2'
    if (Test-Path -LiteralPath $moduleDir) { throw "Refusing to overwrite module audit: $moduleDir" }
    New-Item -ItemType Directory -Path $moduleDir | Out-Null
    $zip = [IO.Compression.ZipFile]::OpenRead($apk)
    try {
        $nativeEntries = @($zip.Entries | Where-Object FullName -CEQ 'lib/arm64-v8a/libhorde_rt_probe_android.so')
        if ($nativeEntries.Count -ne 1) { throw "$kind APK has $($nativeEntries.Count) ARM64 native libraries." }
        $stream = $nativeEntries[0].Open()
        try {
            $memory = [IO.MemoryStream]::new()
            try { $stream.CopyTo($memory); $nativeBytes = $memory.ToArray() }
            finally { $memory.Dispose() }
        } finally { $stream.Dispose() }
    } finally { $zip.Dispose() }
    $nativeHash = ([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($nativeBytes))).ToLowerInvariant()
    if ($nativeHash -cne $containment.arm64Sha256) { throw "$kind extracted package library does not match containment receipt." }
    $nativePath = Join-Path $moduleDir 'libhorde_rt_probe_android.so'
    [IO.File]::WriteAllBytes($nativePath, $nativeBytes)
    $rows = @()
    foreach ($module in $containment.packaged.modules) {
        $offset = [int]$module.offset
        $length = [int]$module.words * 4
        if ($module.words -le 5 -or $offset -lt 0 -or ($offset + $length) -gt $nativeBytes.Length) {
            throw "$kind module bounds invalid at $offset words=$($module.words)."
        }
        $bytes = [byte[]]::new($length)
        [Array]::Copy($nativeBytes, $offset, $bytes, 0, $length)
        $sha = ([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes))).ToLowerInvariant()
        if ($sha -cne $module.sha256) { throw "$kind packaged module hash mismatch at $offset." }
        $backend = if ($module.backend -ceq 'RayTracingPipeline') { 'RayTracingPipeline' } else { 'RayQueryCompute' }
        $matches = @($catalogs[$backend] | Where-Object {
            $_.spirvSha256 -ceq $sha -and $_.key -in $containment.packaged.semanticKeys
        })
        if ($matches.Count -ne 1) { throw "$kind module $sha does not map to exactly one $backend catalog key." }
        $key = [string]$matches[0].key
        $spv = Join-Path $moduleDir "$key.spv"
        $asm = Join-Path $moduleDir "$key.spvasm"
        [IO.File]::WriteAllBytes($spv, $bytes)
        & $val --target-env vulkan1.2 $spv
        if ($LASTEXITCODE -ne 0) { throw "spirv-val failed: $kind/$key exit $LASTEXITCODE" }
        & $dis $spv -o $asm
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $asm)) { throw "spirv-dis failed: $kind/$key exit $LASTEXITCODE" }
        $text = [IO.File]::ReadAllText($asm)
        $atomics = [regex]::Matches($text, '(?m)^\s*%\S+\s*=\s*OpAtomic\w+\b').Count
        $imageReads = [regex]::Matches($text, '(?m)^\s*%\S+\s*=\s*OpImageRead\b').Count
        $binding22 = [regex]::IsMatch($text, '(?m)^\s*OpDecorate\s+%\S+\s+Binding\s+22\s*$')
        $rows += [pscustomobject]@{
            key = $key; backend = $backend; executionModel = $module.executionModel
            sha256 = $sha; words = $module.words; rayQueryInitializations = $module.rayQueryInitializations
            atomics = $atomics; imageReads = $imageReads; binding22 = $binding22
            spirvVal = 'passed'; spirvDis = 'passed'
        }
    }
    $all += [pscustomobject]@{ kind = $kind; apkSha256 = (Get-FileHash $apk -Algorithm SHA256).Hash.ToLowerInvariant(); nativeSha256 = $nativeHash; modules = $rows }
}
$expectedChanged = @(
    'shipping_mobile_opaque_fast', 'shipping_mobile_generic_dielectric',
    'rayquery_compute_shipping_mobile_opaque_fast', 'rayquery_compute_shipping_mobile_generic_dielectric'
)
$controlByKey = @{}; foreach ($m in $all[0].modules) { $controlByKey[$m.key] = $m }
$isolateByKey = @{}; foreach ($m in $all[1].modules) { $isolateByKey[$m.key] = $m }
if ($controlByKey.Count -ne 4 -or $isolateByKey.Count -ne 4) { throw 'Expected exactly four packaged shader modules per APK.' }
foreach ($key in $controlByKey.Keys) {
    $a = $controlByKey[$key]; $b = $isolateByKey[$key]
    if (($a.sha256 -cne $b.sha256) -ne ($key -in $expectedChanged)) { throw "Unexpected module delta: $key" }
    foreach ($m in @($a, $b)) {
        if ($m.atomics -ne 0 -or $m.imageReads -ne 0 -or $m.binding22) { throw "Shipping diagnostic containment violation: $($m.key)" }
    }
}
$receipt = [pscustomobject]@{ schema = 1; sourceHead = '71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95'; expectedChangedKeys = $expectedChanged; artifacts = $all }
$receipt | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $root 'packaged-module-audit.json')
Write-Output (Get-Content (Join-Path $root 'packaged-module-audit.json') -Raw)
