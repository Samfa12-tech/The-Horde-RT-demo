param([string]$Root = $PSScriptRoot)
$ErrorActionPreference = 'Stop'
foreach ($variant in @('debug', 'unsigned-shipping')) {
    $directory = Join-Path $Root ('apk-' + $variant)
    $summary = Get-Content -LiteralPath (Join-Path $directory 'containment.log') -Raw | ConvertFrom-Json
    $elf = [IO.File]::ReadAllBytes($summary.target)
    $records = foreach ($module in $summary.modules) {
        $stem = $module.backend + '-' + $module.sha256.Substring(0, 12)
        $spvPath = Join-Path $directory ($stem + '.spv')
        $disPath = Join-Path $directory ($stem + '.spvasm')
        $bytes = [byte[]]::new($module.words * 4)
        [Array]::Copy($elf, $module.offset, $bytes, 0, $bytes.Length)
        [IO.File]::WriteAllBytes($spvPath, $bytes)
        $hash = (Get-FileHash -LiteralPath $spvPath -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($hash -cne $module.sha256) { throw 'Extracted module hash mismatch.' }
        & 'C:/VulkanSDK/1.4.350.0/Bin/spirv-dis.exe' $spvPath -o $disPath
        if ($LASTEXITCODE -ne 0) { throw 'Actual packaged module disassembly failed.' }
        $disassembly = Get-Content -LiteralPath $disPath -Raw
        $atomics = [regex]::Matches($disassembly, '\bOpAtomic\w+\b').Count
        $imageReads = [regex]::Matches($disassembly, '\bOpImageRead\b').Count
        $binding22 = [regex]::IsMatch($disassembly, '\bBinding 22\b')
        if ($atomics -ne $module.atomicInstructions -or $binding22 -ne $module.binding22) {
            throw 'Independent disassembly differs from containment summary.'
        }
        if ($variant -eq 'unsigned-shipping' -and ($atomics -ne 0 -or $imageReads -ne 0 -or $binding22)) {
            throw 'Shipping diagnostic/readback path found in actual APK.'
        }
        [ordered]@{
            sha256 = $hash; backend = $module.backend; words = $module.words
            atomicInstructions = $atomics; imageReadInstructions = $imageReads
            binding22 = $binding22; spirvVal = $summary.spirvVal; spirvDis = 'passed'
        }
    }
    [ordered]@{
        apkSha256 = (Get-FileHash -LiteralPath (Join-Path $Root ('HordeLanternRT-derived-normal-bias-01-' + $variant + '-arm64.apk')) -Algorithm SHA256).Hash.ToLowerInvariant()
        nativeLibrarySha256 = $summary.targetSha256
        modules = @($records)
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $directory 'embedded-module-disassembly-summary.json') -Encoding utf8
    Write-Output "$variant`: four actual packaged modules validated/disassembled, Shipping readback guard checked."
}
