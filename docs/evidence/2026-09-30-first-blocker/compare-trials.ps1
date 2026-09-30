param([switch]$ValidateOnly)
$ErrorActionPreference = 'Stop'
$taskRoot = [IO.Path]::GetFullPath($PSScriptRoot)
if ($taskRoot -cne 'C:\Dev\tmp\horde-first-blocker-20260930') { throw 'Use the bounded first-blocker scratch root only.' }
$ids = @('first-blocker-control-a1-route-20260930','first-blocker-candidate-b1-route-20260930','first-blocker-candidate-b2-route-20260930','first-blocker-control-a2-route-20260930')
$rows = foreach ($id in $ids) {
    $path = Join-Path $taskRoot "phone/$id/analysis.json"
    $value = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
    $candidate = $id -match '-candidate-'
    $expectedApk = if ($candidate) { '0383edcad0ffb0ab9e116ff5df7f8e20bbdb4cdca1f06a30986709257e42387e' } else { 'ab3e2261fd081f87e667a6e4967e2476077fa96554702330e6aa49baa8133eae' }
    if ($value.runId -cne $id -or $value.integrity -cne 'PASS' -or $value.apkSha256 -cne $expectedApk -or
        $value.deviceModel -cne 'SM-S948B' -or $value.workload -cne 'showcase-route-v1' -or
        $value.frames -ne 1838 -or $value.diagnosticRowsCompiledOut -ne 1838 -or $value.presentMode -cne 'MAILBOX' -or
        $value.internalExtent.width -ne 1080 -or $value.internalExtent.height -ne 2235 -or
        $value.owningActiveStrategy.counts.'opaque-fast' -ne 1838 -or
        $value.owningActiveStrategy.openingCounts.'opaque-fast' -ne 160) { throw "Comparison admission failed: $id" }
    if ($candidate -and ($value.physicalEvidenceClass -cne 'quality-preserving-candidate-unaccepted' -or
        $value.gainClaim -cne 'none' -or $value.physicalAcceptance -cne 'not-evaluated')) { throw "Candidate evidence classification failed: $id" }
    $opening = @($value.zoneGpuCommandBufferTiming | Where-Object zone -CEQ 'opening')
    if ($opening.Count -ne 1 -or $opening[0].frames -ne 160) { throw "Opening ownership count failed: $id" }
    [ordered]@{
        runId = $id; apkSha256 = $value.apkSha256
        analysisSha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
        routeMedianMs = $value.overall.medianMs; routeP95Ms = $value.overall.p95Ms
        medianDerivedFps = $value.medianDerivedFps; routeGpuMedianMs = $value.gpuRtDurationMs.medianMilliseconds
        openingGpuMedianMs = $opening[0].medianMs; openingGpuP95Ms = $opening[0].p95NearestRankMs
        openingGpuWithin33_333Ms = $opening[0].within33_333Ms; context = $value.context
    }
}
$control = @($rows[0],$rows[3]); $candidate = @($rows[1],$rows[2])
$metrics = foreach ($field in @('routeMedianMs','routeP95Ms','routeGpuMedianMs','openingGpuMedianMs','openingGpuP95Ms')) {
    $a = ($control[0][$field] + $control[1][$field]) / 2.0
    $b = ($candidate[0][$field] + $candidate[1][$field]) / 2.0
    [ordered]@{ metric=$field; controlMeanOfRunStatistics=$a; candidateMeanOfRunStatistics=$b; descriptiveChangePercent=100.0*($b/$a-1.0) }
}
$result = [ordered]@{
    schema=1; evidenceClass='descriptive-uncooled-abba-with-power-drift'; gainClaim='none'
    phaseAcceptance='not-complete'; target30Fps='not-demonstrated'; externallyCooled=$false
    fullRouteRows=7352; openingRows=640; runs=$rows; contrasts=$metrics
    limitations=@(
        'Same exact phone, Shipping/Mobile, pipeline, 75%, scene, compiler options and admitted complete ledgers; no quality reduction.',
        'Battery temperatures and GPU thermal power levels vary; context is not aligned to individual measured frames. This is not a causal effect estimate or steady-state thermal certification.',
        'Mean of per-run statistics is descriptive, not a pooled-frame median or display-pacing measurement.',
        'GPU command duration includes AS work, RT dispatch and copy; CPU skinning remains separate.',
        'This route contains no reward lantern; it does not replace glass-heavy performance/correctness or backend/device acceptance.'
    )
}
if ($ValidateOnly) { $result | ConvertTo-Json -Depth 8; return }
$output = Join-Path $taskRoot 'comparison.json'
if (Test-Path -LiteralPath $output) { throw 'Refuse to overwrite the frozen comparison receipt.' }
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $output -Encoding utf8NoBOM
Write-Output 'Four-run exact uncooled comparison admitted; descriptive statistics only, no gain or 30 FPS claim.'
