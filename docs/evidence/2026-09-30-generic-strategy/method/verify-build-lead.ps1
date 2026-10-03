$ErrorActionPreference = 'Stop'
$taskRoot = 'C:/Dev/tmp/horde-generic-route-profile-20260930'
$receipt = Get-Content -LiteralPath "$taskRoot/profile/provenance/profile-build-receipt.json" -Raw | ConvertFrom-Json
function Hash-Bytes([byte[]]$bytes) {
    [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()
}
function Verify-Hash([string]$path, [string]$expected) {
    if ((Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant() -cne $expected) { throw "Hash mismatch: $path" }
}
Verify-Hash $receipt.source.externalPatch $receipt.source.patchSha256
Verify-Hash "$taskRoot/source-eaf/src/vulkan/raytracing/PresentableTinyRtScene.cpp" $receipt.source.modifiedSourceSha256
Verify-Hash $receipt.build.gradleLog $receipt.build.gradleLogSha256
if ($receipt.source.head -cne 'eafbf8262442a82e0835edf5cf5306d718e64633' -or $receipt.build.exitCode -ne 0) { throw 'Incorrect source/build receipt' }
foreach ($apk in $receipt.apks) { Verify-Hash $apk.path $apk.sha256 }
Verify-Hash $receipt.control.apk $receipt.control.apkSha256
Add-Type -AssemblyName System.IO.Compression.FileSystem
$control = [IO.Compression.ZipFile]::OpenRead($receipt.control.apk)
$candidate = [IO.Compression.ZipFile]::OpenRead($receipt.apks[0].path)
function Entry-Bytes($entry) {
    $stream = $entry.Open(); $buffer = [IO.MemoryStream]::new()
    try { $stream.CopyTo($buffer); return ,$buffer.ToArray() } finally { $stream.Dispose(); $buffer.Dispose() }
}
$comparisons = @(); $moduleChecks = @()
try {
    $controlNames = @($control.Entries | Where-Object FullName -CLike 'assets/*' | ForEach-Object FullName | Sort-Object)
    $candidateNames = @($candidate.Entries | Where-Object FullName -CLike 'assets/*' | ForEach-Object FullName | Sort-Object)
    if ($controlNames.Count -ne 53 -or ($controlNames -join "`n") -cne ($candidateNames -join "`n")) { throw 'Asset name set differs' }
    foreach ($name in $controlNames) {
        $a = Entry-Bytes $control.GetEntry($name); $b = Entry-Bytes $candidate.GetEntry($name)
        $aHash = Hash-Bytes $a; $bHash = Hash-Bytes $b
        $classification = 'exact-byte-equality'
        if ($aHash -cne $bHash) {
            $aText = [Text.Encoding]::UTF8.GetString($a); $bText = [Text.Encoding]::UTF8.GetString($b)
            if ($name -ceq 'assets/ASSET_LICENSES.md') {
                if ($aText.Replace("`r`n", "`n") -cne $bText.Replace("`r`n", "`n")) { throw 'Licence text changed' }
                $classification = 'line-ending-only'
            } elseif ($name -cin @('assets/models/player/runtime/clip-manifest.json', 'assets/models/player/viewmodel/runtime/asset.manifest.json')) {
                $aJson = $aText | ConvertFrom-Json | ConvertTo-Json -Depth 100 -Compress
                $bJson = $bText | ConvertFrom-Json | ConvertTo-Json -Depth 100 -Compress
                if ($aJson -cne $bJson) { throw "Manifest contents changed: $name" }
                $classification = 'json-formatting-only'
            } else { throw "Unexpected changed asset: $name" }
        }
        $comparisons += [ordered]@{name=$name;controlSha256=$aHash;profileSha256=$bHash;classification=$classification}
    }
    $native = Entry-Bytes $candidate.GetEntry('lib/arm64-v8a/libhorde_rt_probe_android.so')
    $containment = Get-Content -LiteralPath $receipt.containment.benchmark -Raw | ConvertFrom-Json
    if ((Hash-Bytes $native) -cne $containment.packaged.targetSha256 -or $containment.packaged.spirvVal -cne 'passed' -or $containment.packaged.spirvDis -cne 'passed') { throw 'Actual native/validation mismatch' }
    if ($containment.packaged.modules.Count -ne 4) { throw 'Expected four packaged modules' }
    foreach ($module in $containment.packaged.modules) {
        if ($module.atomicInstructions -ne 0 -or $module.binding22 -ne $false) { throw 'Shipping diagnostic containment violation' }
        $bytes = [byte[]]::new($module.words * 4)
        [Array]::Copy($native, $module.offset, $bytes, 0, $bytes.Length)
        if ((Hash-Bytes $bytes) -cne $module.sha256 -or [BitConverter]::ToUInt32($bytes,0) -ne 0x07230203) { throw 'Actual packaged SPIR-V mismatch' }
        $claimed = @($receipt.shippingPackagedModules | Where-Object candidateSha256 -CEQ $module.sha256)
        if ($claimed.Count -ne 1 -or -not $claimed[0].exactEqual -or $claimed[0].controlSha256 -cne $module.sha256 -or $claimed[0].imageReads -ne 0) { throw 'Control module equality or image read policy failed' }
        $moduleChecks += [ordered]@{backend=$module.backend;sha256=$module.sha256;words=$module.words;nativeBytesVerified=$true;shippingPolicy='PASS'}
    }
} finally { $control.Dispose(); $candidate.Dispose() }
$result = [ordered]@{schema=1;status='PASS';sourceBase=$receipt.source.head;sourcePatchSha256=$receipt.source.patchSha256;sourceClaim='base-plus-reviewed-investigation-patch, not clean engineering HEAD';assets=$comparisons;modules=$moduleChecks;scope='Lead independently verified APK/source/patch/log bytes, 53 asset comparisons and four actual packaged SPIR-V hashes; compiler val/dis diagnostics independently inspected, no device or performance acceptance'}
$output = "$taskRoot/profile/provenance/lead-byte-verification.json"
if (Test-Path -LiteralPath $output) { throw 'Do not overwrite frozen receipt' }
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $output -Encoding utf8NoBOM
Write-Output 'Lead byte verification PASS: 53 assets (50 exact, three formatting only), four actual packaged Shipping SPIR-V modules, patch/source/APK/log identity.'
