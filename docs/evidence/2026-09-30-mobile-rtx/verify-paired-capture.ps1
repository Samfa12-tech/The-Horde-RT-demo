$ErrorActionPreference = 'Stop'
$assessmentRoot = 'C:/Dev/tmp/horde-mobile-assessment-20260930'
$receipt = Get-Content -Raw -LiteralPath "$assessmentRoot/artifact-cmake-module-receipt.json" | ConvertFrom-Json
function Assert-Sha([string]$path, [string]$expected) {
    $actual = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $expected.ToLowerInvariant()) { throw "SHA mismatch: $path" }
}
Assert-Sha $receipt.configure.cachePath $receipt.configure.cacheSha256
Assert-Sha $receipt.configure.configureLog $receipt.configure.configureLogSha256
Assert-Sha $receipt.build.exePath $receipt.build.exeSha256
Assert-Sha $receipt.build.buildLog $receipt.build.buildLogSha256
if ($receipt.build.exitCode -ne 0) { throw 'Native build failed.' }
$containment = Get-Content -Raw -LiteralPath "$assessmentRoot/containment-debug-mobile.json" | ConvertFrom-Json
if ($containment.spirvVal -ne 'passed' -or $containment.spirvDis -ne 'passed' -or
    $containment.instrumentation -ne 'Diagnostic' -or $containment.quality -ne 'Mobile' -or
    $containment.modules.Count -ne 4 -or $containment.targetSha256 -ne $receipt.build.exeSha256) {
    throw 'Wrong actual native modules/validation receipt.'
}
$manifests = @()
foreach ($capture in $receipt.captures) {
    Assert-Sha $capture.manifestPath $capture.manifestSha256
    Assert-Sha $capture.pngPath $capture.pngSha256
    $captureManifest = Get-Content -Raw -LiteralPath $capture.manifestPath | ConvertFrom-Json
    if (-not $captureManifest.complete -or $captureManifest.error -or $captureManifest.captures.Count -ne 1 -or
        $captureManifest.executionBackend -ne $capture.executionBackend -or
        -not $captureManifest.captures[0].honestlyPresentedRtFrame -or
        $captureManifest.captures[0].checkpoint -ne 'lantern-glass-production' -or
        $captureManifest.device.gpuName -ne 'NVIDIA GeForce RTX 5050 Laptop GPU' -or
        $captureManifest.presentation.dispatchWidth -ne 540 -or $captureManifest.presentation.dispatchHeight -ne 960 -or
        $captureManifest.presentation.renderScale -ne 1 -or
        -not $captureManifest.dielectricReasonDiagnostics.available) { throw 'Invalid paired native capture identity.' }
    foreach ($strategy in @('opaqueFast','genericDielectric')) {
        $moduleHash = $captureManifest.selectedRtPipelineBundle.$strategy.sha256
        if (@($containment.modules | Where-Object { $_.sha256 -eq $moduleHash -and $_.backend -eq $capture.executionBackend }).Count -ne 1) {
            throw 'Capture does not name the packaged backend module.'
        }
    }
    $manifests += $captureManifest
}
if ($manifests.Count -ne 2 -or $manifests[0].executionBackend -ne 'RayTracingPipeline' -or $manifests[1].executionBackend -ne 'RayQueryCompute') {
    throw 'Expected exactly one native capture per backend.'
}
$counterParity = ($manifests[0].dielectricDiagnostics | ConvertTo-Json -Compress) -eq ($manifests[1].dielectricDiagnostics | ConvertTo-Json -Compress) -and
    ($manifests[0].dielectricReasonDiagnostics | ConvertTo-Json -Compress) -eq ($manifests[1].dielectricReasonDiagnostics | ConvertTo-Json -Compress)
if (-not $counterParity) { throw 'Paired counter objects differ.' }
Add-Type -AssemblyName System.Drawing
$pipeline = [Drawing.Bitmap]::new($receipt.captures[0].pngPath)
$compute = [Drawing.Bitmap]::new($receipt.captures[1].pngPath)
$different = 0L
$overOne = 0L
$maximumDifference = 0
$channelsAbsoluteSum = 0L
try {
    if ($pipeline.Width -ne 540 -or $pipeline.Height -ne 960 -or $compute.Width -ne 540 -or $compute.Height -ne 960) { throw 'PNG extent mismatch.' }
    for ($row = 0; $row -lt 960; ++$row) {
        for ($column = 0; $column -lt 540; ++$column) {
            $a = $pipeline.GetPixel($column, $row)
            $b = $compute.GetPixel($column, $row)
            $redDelta = [Math]::Abs([int]$a.R - [int]$b.R)
            $greenDelta = [Math]::Abs([int]$a.G - [int]$b.G)
            $blueDelta = [Math]::Abs([int]$a.B - [int]$b.B)
            $delta = [Math]::Max($redDelta, [Math]::Max($greenDelta, $blueDelta))
            $channelsAbsoluteSum += $redDelta + $greenDelta + $blueDelta
            if ($delta -gt 0) { ++$different }
            if ($delta -gt 1) { ++$overOne }
            if ($delta -gt $maximumDifference) { $maximumDifference = $delta }
        }
    }
} finally {
    $pipeline.Dispose()
    $compute.Dispose()
}
$pixelCount = 540L * 960L
$result = [ordered]@{
    schema = 1
    artifactManifestModuleHashesVerified = $true
    sourceCommit = $receipt.source.commit
    exeSha256 = $receipt.build.exeSha256
    counterObjectsEqual = $counterParity
    subset = 'One RTX diagnostic Mobile isolated-lantern checkpoint; not the 13-capture foundation gate'
    pixelToleranceSource = 'tools/compare-foundation-captures.ps1 (unchanged limits; pixel-only subset assessment)'
    pixelTolerance = [ordered]@{maximumChannelDifference = 3; maximumFractionOverOne = 0.001}
    pixels = $pixelCount
    pixelsDifferent = $different
    pixelsDifferentByMoreThanOne = $overOne
    differentFraction = $overOne / $pixelCount
    maximumChannelDifference = $maximumDifference
    meanAbsoluteRgbChannelDifference = $channelsAbsoluteSum / ($pixelCount * 3)
    pixelSubsetPassed = $maximumDifference -le 3 -and ($overOne / $pixelCount) -le 0.001
    phoneFailureReproduced = $false
    performanceAcceptance = $false
    phase4Accepted = $false
}
$result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath "$assessmentRoot/paired-capture-verification.json" -Encoding utf8
$result | ConvertTo-Json -Depth 6
