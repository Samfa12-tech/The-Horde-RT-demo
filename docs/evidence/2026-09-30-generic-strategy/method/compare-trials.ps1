param(
    [switch]$ValidateOnly,
    [string]$EvidenceRoot = $PSScriptRoot
)
$ErrorActionPreference = 'Stop'
$scriptRoot = [IO.Path]::GetFullPath($PSScriptRoot)
if ($scriptRoot -cne 'C:\Dev\tmp\horde-generic-route-profile-20260930') { throw 'Use the bounded GenericDielectric profile script only.' }
$taskRoot = [IO.Path]::GetFullPath($EvidenceRoot)
if ($taskRoot -cne $scriptRoot) {
    $fixtureParent = [IO.Path]::GetFullPath((Join-Path $scriptRoot 'profile/comparison-fixtures'))
    $fixturePrefix = $fixtureParent.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
    if (-not $ValidateOnly -or -not $taskRoot.StartsWith($fixturePrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Alternate evidence roots are allowed only for ValidateOnly fixtures under profile/comparison-fixtures.'
    }
}
$receiptPath = Join-Path $scriptRoot 'profile/provenance/profile-build-receipt.json'
if ((Get-FileHash -LiteralPath $receiptPath -Algorithm SHA256).Hash.ToLowerInvariant() -cne '2d233deb7694d6aea42a4c2c2cef6eeef368a666d01b2df1293b556f79dfd26d') { throw 'Immutable profile receipt changed.' }
$receipt = Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json
function Get-RequiredJsonPropertyValue($Object, [string]$Name) {
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) { throw "Missing JSON property '$Name'." }
    return $property.Value
}
function Get-JsonPropertyCount($Object) {
    $properties = @($Object.PSObject.Properties)
    return $properties.Count
}
$ids = @('generic-route-control-a1-20260930','generic-route-profile-b1-20260930','generic-route-profile-b2-20260930','generic-route-control-a2-20260930')
$warmupId = 'generic-route-control-warmup-20260930'
$warmupPath = Join-Path $taskRoot "phone/$warmupId/analysis.json"
$warmup = Get-Content -LiteralPath $warmupPath -Raw | ConvertFrom-Json
if ($warmup.runId -cne $warmupId -or $warmup.integrity -cne 'PASS' -or
    $warmup.buildLabel -cne 'restored-control-warmup-context-only' -or
    $warmup.physicalEvidenceClass -cne 'restored-control-warmup-context-only' -or
    $warmup.includedInAbba -ne $false -or
    $warmup.apkSha256 -cne $receipt.control.apkSha256) { throw 'Restored-control warmup context-only admission failed.' }
$missing = @($ids | Where-Object { -not (Test-Path -LiteralPath (Join-Path $taskRoot "phone/$_/analysis.json")) })
if ($missing.Count -gt 0) { throw "ABBA admission requires all four exact runs; missing analysis: $($missing -join ', ')" }
$rows = foreach ($id in $ids) {
    $path = Join-Path $taskRoot "phone/$id/analysis.json"
    $value = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
    $candidate = $id -match '^generic-route-profile-'
    $expectedBuild = if ($candidate) { 'generic-profile' } else { 'control' }
    $expectedStrategy = if ($candidate) { 'generic-dielectric' } else { 'opaque-fast' }
    $expectedApk = if ($candidate) { ($receipt.apks | Where-Object kind -CEQ 'Shipping/Mobile benchmark' | Select-Object -First 1).sha256 } else { $receipt.control.apkSha256 }
    $routeCounts = $value.owningActiveStrategy.counts
    $openingCounts = $value.owningActiveStrategy.openingCounts
    $routeStrategyCount = Get-RequiredJsonPropertyValue $routeCounts $expectedStrategy
    $openingStrategyCount = Get-RequiredJsonPropertyValue $openingCounts $expectedStrategy
    if ($value.runId -cne $id -or $value.integrity -cne 'PASS' -or $value.apkSha256 -cne $expectedApk -or
        $value.buildLabel -cne $expectedBuild -or $value.sourceCommit -cne $receipt.source.head -or
        $value.deviceModel -cne 'SM-S948B' -or $value.workload -cne 'showcase-route-v1' -or
        $value.frames -ne 1838 -or $value.diagnosticRowsCompiledOut -ne 1838 -or $value.presentMode -cne 'MAILBOX' -or
        $value.internalExtent.width -ne 1080 -or $value.internalExtent.height -ne 2235 -or
        $routeStrategyCount -ne 1838 -or (Get-JsonPropertyCount $routeCounts) -ne 1 -or
        $openingStrategyCount -ne 160 -or (Get-JsonPropertyCount $openingCounts) -ne 1) { throw "Comparison admission failed: $id" }
    if ($candidate -and ($value.physicalEvidenceClass -cne 'investigation-only-whole-strategy' -or
        $value.gainClaim -cne 'none' -or $value.physicalAcceptance -cne 'not-evaluated' -or
        $value.optimizationAcceptance -cne 'none')) { throw "Profile evidence classification failed: $id" }
    if (-not $candidate -and $value.physicalEvidenceClass -cne 'exact-control-reference') { throw "Control evidence classification failed: $id" }
    foreach ($moduleKey in @('shipping_mobile_opaque_fast','shipping_mobile_generic_dielectric')) {
        $moduleReceipt = @($receipt.shippingPackagedModules | Where-Object semanticKey -CEQ $moduleKey)
        if ($moduleReceipt.Count -ne 1) { throw "Module receipt cardinality failed: $id $moduleKey" }
        $expectedModuleSha = if ($candidate) { $moduleReceipt[0].candidateSha256 } else { $moduleReceipt[0].controlSha256 }
        $actualModuleSha = $value.actualPipelineModuleSha256.PSObject.Properties[$moduleKey].Value
        if ($actualModuleSha -cne $expectedModuleSha) { throw "Module/APK receipt join failed: $id $moduleKey" }
    }
    $opening = @($value.zoneGpuCommandBufferTiming | Where-Object zone -CEQ 'opening')
    if ($opening.Count -ne 1 -or $opening[0].frames -ne 160) { throw "Opening ownership count failed: $id" }
    [ordered]@{
        runId = $id; apkSha256 = $value.apkSha256
        analysisSha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
        routeMedianMs = $value.overall.medianMs; routeP95Ms = $value.overall.p95Ms
        medianDerivedFps = $value.medianDerivedFps; routeGpuMedianMs = $value.gpuRtDurationMs.medianMilliseconds
        gpuRtDurationMs = $value.gpuRtDurationMs; cpuStages = $value.cpuStages
        openingGpuMedianMs = $opening[0].medianMs; openingGpuP95Ms = $opening[0].p95NearestRankMs
        openingGpuWithin33_333Ms = $opening[0].within33_333Ms; context = $value.context
        activeStrategy = $value.owningActiveStrategy; actualPipelineModuleSha256 = $value.actualPipelineModuleSha256
    }
}
$control = @($rows[0],$rows[3]); $candidate = @($rows[1],$rows[2])
$metrics = foreach ($field in @('routeMedianMs','routeP95Ms','routeGpuMedianMs','openingGpuMedianMs','openingGpuP95Ms')) {
    $a = ($control[0][$field] + $control[1][$field]) / 2.0
    $b = ($candidate[0][$field] + $candidate[1][$field]) / 2.0
    [ordered]@{ metric=$field; controlMeanOfRunStatistics=$a; candidateMeanOfRunStatistics=$b; descriptiveChangePercent=100.0*($b/$a-1.0) }
}
$result = [ordered]@{
    schema=1; evidenceClass='investigation-only-whole-strategy'; gainClaim='none'
    optimizationAcceptance='none'; physicalAcceptance='not-evaluated'; causalConclusion='none'
    sourceCommit=$receipt.source.head; investigationPatchSha256=$receipt.source.patchSha256
    phaseAcceptance='not-complete'; target30Fps='not-demonstrated'; externallyCooled=$false
    fullRouteRows=7352; openingRows=640; runs=$rows; contrasts=$metrics
    contextOnlyExcludedFromAbba=[ordered]@{runId=$warmup.runId;apkSha256=$warmup.apkSha256;context=$warmup.context;usedForRunStatistics=$false;pooledWithAbba=$false}
    limitations=@(
        'Same exact phone, Shipping/Mobile, 75%, scene, and strict completed-frame ledgers; the Generic profile selects a different whole shader strategy, including opaque spawn and ordered-shadow paths.',
        'This is not isolated compiler occupancy, pure glass cost, or guaranteed pixel-equivalent rendering; opening captures must be reviewed separately.',
        'Battery temperatures and GPU thermal power levels vary; context is not aligned to individual measured frames. This is not a causal effect estimate or steady-state thermal certification.',
        'Mean of per-run statistics is descriptive, not a pooled-frame median or display-pacing measurement.',
        'The restored-control warmup context-only run is excluded from ABBA statistics; no warmup frames or statistics are pooled.',
        'GPU command duration includes AS work, RT dispatch and copy; CPU skinning remains separate.',
        'This ordinary opening route does not isolate reward-lantern/glass cost and does not replace glass-heavy performance/correctness or backend/device acceptance.'
    )
}
if ($ValidateOnly) { $result | ConvertTo-Json -Depth 8; return }
$output = Join-Path $taskRoot 'comparison.json'
if (Test-Path -LiteralPath $output) { throw 'Refuse to overwrite the frozen comparison receipt.' }
$result | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $output -Encoding utf8NoBOM
Write-Output 'Four-run whole-strategy comparison admitted; investigation only, no optimisation/gain or 30 FPS acceptance.'
