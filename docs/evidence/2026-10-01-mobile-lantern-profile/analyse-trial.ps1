[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$RunId,
    [string]$PhoneRoot = 'C:\Dev\tmp\horde-mobile-lantern-profile-20261001\phone',
    [string]$PackageReceipt = 'C:\Dev\tmp\horde-mobile-lantern-profile-20261001\audit-pair-v3-3a4faf72923e-bac94c45bcfe\actual-apk-package-audit.json'
)
$ErrorActionPreference = 'Stop'

function Assert-Equal($Actual, $Expected, [string]$Label) {
    if ($Actual -cne $Expected) { throw "$Label expected '$Expected', found '$Actual'" }
}
function Assert-True([bool]$Value, [string]$Label) {
    if (-not $Value) { throw $Label }
}
function Get-Median([double[]]$Values) {
    $sorted = @($Values | Sort-Object)
    if ($sorted.Count -eq 0) { throw 'Cannot summarize an empty sample set.' }
    $middle = [int][Math]::Floor($sorted.Count / 2)
    if ($sorted.Count % 2) { return [double]$sorted[$middle] }
    return ([double]$sorted[$middle - 1] + [double]$sorted[$middle]) / 2.0
}
function Get-NearestRank([double[]]$Values, [double]$Percentile) {
    $sorted = @($Values | Sort-Object)
    if ($sorted.Count -eq 0) { throw 'Cannot summarize an empty sample set.' }
    $rank = [int][Math]::Ceiling($sorted.Count * $Percentile)
    return [double]$sorted[$rank - 1]
}
function Read-MemorySummary([string]$Path) {
    $result = [ordered]@{ available = $false; totalPssKb = $null; totalRssKb = $null; totalSwapPssKb = $null; nativeHeapPssKb = $null }
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $result }
    $contents = [IO.File]::ReadAllText($Path)
    $total = [regex]::Match($contents, '(?m)^\s*TOTAL PSS:\s*(\d+)\s+TOTAL RSS:\s*(\d+)\s+TOTAL SWAP PSS:\s*(\d+)')
    if ($total.Success) {
        $result.totalPssKb = [long]$total.Groups[1].Value
        $result.totalRssKb = [long]$total.Groups[2].Value
        $result.totalSwapPssKb = [long]$total.Groups[3].Value
        $result.available = $true
    }
    $native = [regex]::Match($contents, '(?m)^\s*Native Heap\s+(\d+)\s+')
    if ($native.Success) { $result.nativeHeapPssKb = [long]$native.Groups[1].Value }
    return $result
}

$runDirectory = Join-Path ([IO.Path]::GetFullPath($PhoneRoot)) $RunId
if (-not (Test-Path -LiteralPath $runDirectory -PathType Container)) { throw "Trial directory not found: $runDirectory" }
$benchmarkPath = Join-Path (Join-Path $runDirectory $RunId) 'benchmark.json'
$markerPath = Join-Path (Join-Path $runDirectory $RunId) 'result.json'
$trialPath = Join-Path $runDirectory 'trial.json'
$contextPath = Join-Path $runDirectory 'context-samples.jsonl'
foreach ($path in @($benchmarkPath, $markerPath, $trialPath)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required trial artifact is missing: $path" }
}
$report = Get-Content -LiteralPath $benchmarkPath -Raw | ConvertFrom-Json -DateKind String
$marker = Get-Content -LiteralPath $markerPath -Raw | ConvertFrom-Json -DateKind String
$trial = Get-Content -LiteralPath $trialPath -Raw | ConvertFrom-Json -DateKind String
Assert-Equal $trial.runId $RunId 'trial identity'
Assert-Equal $trial.status 'complete' 'trial status'
Assert-Equal $marker.runId $RunId 'result marker identity'
Assert-Equal $marker.status 'complete' 'result marker status'
Assert-Equal $report.runId $RunId 'benchmark identity'
Assert-Equal $report.status 'complete' 'benchmark status'
Assert-Equal $report.result 'complete' 'benchmark result'
Assert-Equal $report.workloadComplete $true 'workload completion'
Assert-Equal $report.presentedEveryFrame $true 'present-every-frame'
Assert-Equal $trial.instrumentation 'Shipping' 'instrumentation'
Assert-Equal $trial.quality 'Mobile' 'quality'
Assert-Equal $trial.requestedScalePercent 75 'requested render scale'
Assert-Equal $trial.renderScalePercent 75 'actual render scale'
Assert-Equal $report.renderScalePercent 75 'reported render scale'
Assert-Equal $trial.recordingEnabled $false 'recording disabled'
Assert-Equal $report.legacyFrameTimingScope 'android-render-entry-through-present' 'native-cycle timing scope'
Assert-Equal $report.materialEncoding 'ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2) + strict ASTC 6x6 lich' 'material encoding'
Assert-Equal $report.presentMode 'MAILBOX' 'present mode'

$deviceSpec = switch -Exact ([string]$trial.deviceModel) {
    'SM-S948B' { [ordered]@{ backend='RayTracingPipeline'; rtMode='RayTracingPipeline'; internalWidth=1080; internalHeight=2235; presentWidth=1440; presentHeight=2980; raygen='shipping_mobile_'; strategyOpaque='opaque-fast'; strategyGlass='generic-dielectric'; serial='R5GL219SZGK' } }
    'SM-S928B' { [ordered]@{ backend='RayQueryCompute'; rtMode='RayQuery'; internalWidth=810; internalHeight=1682; presentWidth=1080; presentHeight=2243; raygen='rayquery_compute_shipping_mobile_'; strategyOpaque='opaque-fast'; strategyGlass='generic-dielectric'; serial='R5CXC0G9GBW' } }
    default { throw "Unadmitted exact device model '$($trial.deviceModel)'" }
}
Assert-Equal $trial.requestedBackend $deviceSpec.backend 'requested device backend'
Assert-Equal $report.executionBackend $deviceSpec.backend 'actual execution backend'
Assert-Equal $report.rtMode $deviceSpec.rtMode 'actual RT mode'
Assert-Equal $report.internalExtent.width $deviceSpec.internalWidth 'internal extent width'
Assert-Equal $report.internalExtent.height $deviceSpec.internalHeight 'internal extent height'
Assert-Equal $trial.internalExtent.width $deviceSpec.internalWidth 'trial internal extent width'
Assert-Equal $trial.internalExtent.height $deviceSpec.internalHeight 'trial internal extent height'
Assert-Equal $report.presentationExtent.width $deviceSpec.presentWidth 'presentation extent width'
Assert-Equal $report.presentationExtent.height $deviceSpec.presentHeight 'presentation extent height'
Assert-Equal $trial.presentationExtent.width $deviceSpec.presentWidth 'trial presentation extent width'
Assert-Equal $trial.presentationExtent.height $deviceSpec.presentHeight 'trial presentation extent height'
Assert-Equal $trial.serial $deviceSpec.serial 'exact Android device serial'

$artifactSpec = switch -Exact ([string]$trial.member) {
    'physical-control' { [ordered]@{ apk='3a4faf72923e24da9da859ef6cd71c221b818b97d7418a75b1c8dd0ca0540378'; label='control'; source='9f4f439e40a9477efe0bac42ee41b74de64b3b1b'; native='216382290a62f587e7e3063370d1cc2da59109eb2646bf4840e8d2c2e5d719de' } }
    'open-aperture-candidate' { [ordered]@{ apk='bac94c45bcfe3f8fcc367f21439735387d188959005b5ee873d02f0f1e9cf142'; label='candidate'; source='1da483d01124ca673badf6c13939d99736c4a658'; native='17a1bad6a83fac40d6fdbb991bb7e983c087a815434ef1edd10a576517bf4adc' } }
    default { throw "Unknown exact treatment member '$($trial.member)'" }
}
Assert-Equal $trial.installedApkSha256 $artifactSpec.apk 'installed APK package identity'
Assert-Equal $trial.sourceCommit $artifactSpec.source 'member source receipt identity'
Assert-Equal (Get-FileHash -LiteralPath $PackageReceipt -Algorithm SHA256).Hash.ToLowerInvariant() '67c1ae7225c57bd1fa27cab6ab934b910d5d8f2dd993c460c1ba74917b44ddaa' 'actual package audit receipt hash'
$packageAudit = Get-Content -LiteralPath $PackageReceipt -Raw | ConvertFrom-Json
Assert-Equal $packageAudit.assetComparison.controlEntryCount 70 'actual control asset inventory'
Assert-Equal $packageAudit.assetComparison.candidateEntryCount 70 'actual candidate asset inventory'
Assert-Equal $packageAudit.assetComparison.identicalEntries 70 'byte-identical asset inventory'
Assert-Equal $packageAudit.assetComparison.changedEntries 0 'changed package assets'
Assert-Equal $packageAudit.assetComparison.lfsPointers 0 'LFS pointer count'
Assert-Equal $packageAudit.moduleComparison.actualShippingMobileModulesEach 4 'actual Shipping/Mobile module count'
Assert-Equal $packageAudit.moduleComparison.unchangedSharedRtStagesEach 2 'shared RT stage count'
Assert-Equal $packageAudit.moduleComparison.result 'all four exact frozen Shipping/Mobile modules and both shared RT stages are byte-identical to control' 'module containment result'
$expectedModuleHashes = @{
    shipping_mobile_opaque_fast='66e39df9f53b058fb62cbfa913d424b161c93be4aff59a1685cf8b7e54bb9c4b'
    shipping_mobile_generic_dielectric='ce2302811cb2cb8bbff706fd54cd7f48705e4a5e8b2cda744a7d999574f84532'
    rayquery_compute_shipping_mobile_opaque_fast='5002afa7ac6ce0d03fc749742bb0832a1006b9f01bb1c6ab8490710790a79133'
    rayquery_compute_shipping_mobile_generic_dielectric='c2bd389812841161951f819e0f12e95c929aa3f49ec87d9849393a687d20ebde'
}
$receiptArtifacts = @($packageAudit.artifacts | Where-Object role -CEQ $artifactSpec.label)
Assert-Equal $receiptArtifacts.Count 1 'actual APK binary receipt count'
$artifact = $receiptArtifacts[0]
Assert-Equal $artifact.apkSha256 $artifactSpec.apk 'actual APK receipt identity'
Assert-Equal $artifact.arm64LibrarySha256 $artifactSpec.native 'actual APK native-library identity'
Assert-Equal $artifact.assetEntryCount 70 'actual APK asset count'
Assert-Equal $artifact.alignedSpirvMagicOffsetsFound 6 'all embedded SPIR-V module count'
Assert-Equal $artifact.modules.Count 4 'catalog module receipt count'
Assert-Equal $artifact.sharedStages.Count 2 'shared RT-stage receipt count'
foreach ($key in $expectedModuleHashes.Keys) {
    $module = @($artifact.modules | Where-Object key -CEQ $key)
    Assert-Equal $module.Count 1 "actual embedded module $key"
    Assert-Equal $module[0].sha256 $expectedModuleHashes[$key] "frozen module bytes $key"
    Assert-Equal $module[0].spirvVal 'passed' "fresh SPIR-V validation $key"
    Assert-Equal $module[0].spirvDis 'passed' "fresh SPIR-V disassembly $key"
    Assert-Equal $module[0].atomicsIncludingNoResultOpcodes 0 "atomic opcodes $key"
    Assert-Equal $module[0].imageReads 0 "image-read opcodes $key"
    Assert-Equal $module[0].binding22 $false "diagnostic binding $key"
}
foreach ($stage in $artifact.sharedStages) {
    Assert-Equal $stage.spirvVal 'passed' "fresh shared stage validation $($stage.executionModel)"
    Assert-Equal $stage.spirvDis 'passed' "fresh shared stage disassembly $($stage.executionModel)"
    Assert-Equal $stage.atomicsIncludingNoResultOpcodes 0 "shared stage atomics $($stage.executionModel)"
    Assert-Equal $stage.binding22 $false "shared stage diagnostic binding $($stage.executionModel)"
}
$opaqueKey = $deviceSpec.raygen + 'opaque_fast'
$glassKey = $deviceSpec.raygen + 'generic_dielectric'
$expectedOpaqueHash = $expectedModuleHashes[$opaqueKey]
$expectedGlassHash = $expectedModuleHashes[$glassKey]
$expectedShader = "opaqueFast:$($opaqueKey)@$expectedOpaqueHash|genericDielectric:$($glassKey)@$expectedGlassHash"
Assert-Equal $report.shader $expectedShader 'loaded backend-specific Shipping/Mobile shader identities'

$candidateStrategy = if ($trial.member -ceq 'open-aperture-candidate') { $deviceSpec.strategyOpaque } else { $deviceSpec.strategyGlass }
$workloadSpec = switch -Exact ([string]$trial.workload) {
    'showcase-route-v1' {
        [ordered]@{ measured=1838; expectedStrategy=$deviceSpec.strategyOpaque; rowPlan=@(
            [ordered]@{name='opening';frames=160}, [ordered]@{name='skeleton-room';frames=98},
            [ordered]@{name='shadow-corridor';frames=599}, [ordered]@{name='skylight-chamber';frames=189},
            [ordered]@{name='yellow-torch-bay';frames=157}, [ordered]@{name='blue-torch-bay';frames=157},
            [ordered]@{name='red-torch-bay';frames=157}, [ordered]@{name='green-torch-bay';frames=157},
            [ordered]@{name='transmission-threshold';frames=63}, [ordered]@{name='finale';frames=101}); zonePlan=@(
            [ordered]@{name='opening';frames=160}, [ordered]@{name='skeleton-room';frames=98},
            [ordered]@{name='shadow-corridor';frames=599}, [ordered]@{name='skylight-chamber';frames=189},
            [ordered]@{name='yellow-torch-bay';frames=157}, [ordered]@{name='blue-torch-bay';frames=157},
            [ordered]@{name='red-torch-bay';frames=157}, [ordered]@{name='green-torch-bay';frames=157},
            [ordered]@{name='transmission-threshold';frames=63}, [ordered]@{name='finale';frames=101}) }
    }
    'lantern-held-high-v1' {
        $zones = @([ordered]@{name='opening';frames=0},[ordered]@{name='skeleton-room';frames=0},[ordered]@{name='shadow-corridor';frames=0},[ordered]@{name='skylight-chamber';frames=0},[ordered]@{name='yellow-torch-bay';frames=600},[ordered]@{name='blue-torch-bay';frames=0},[ordered]@{name='red-torch-bay';frames=0},[ordered]@{name='green-torch-bay';frames=0},[ordered]@{name='transmission-threshold';frames=0},[ordered]@{name='finale';frames=0})
        [ordered]@{ measured=600; expectedStrategy=$candidateStrategy; rowPlan=@([ordered]@{name='yellow-torch-bay';frames=600}); zonePlan=$zones }
    }
    'lantern-reveal-sequence-v1' {
        $zones = @([ordered]@{name='opening';frames=0},[ordered]@{name='skeleton-room';frames=0},[ordered]@{name='shadow-corridor';frames=0},[ordered]@{name='skylight-chamber';frames=0},[ordered]@{name='yellow-torch-bay';frames=600},[ordered]@{name='blue-torch-bay';frames=0},[ordered]@{name='red-torch-bay';frames=0},[ordered]@{name='green-torch-bay';frames=0},[ordered]@{name='transmission-threshold';frames=0},[ordered]@{name='finale';frames=0})
        [ordered]@{ measured=600; expectedStrategy=$candidateStrategy; rowPlan=@([ordered]@{name='yellow-torch-bay';frames=600}); zonePlan=$zones }
    }
    default { throw "Unadmitted workload '$($trial.workload)'" }
}
Assert-Equal $report.workload $trial.workload 'trial workload'
Assert-Equal $report.measuredFrames $workloadSpec.measured 'measured frame count'
Assert-Equal $report.lapsRequested 2 'requested lap count'
Assert-Equal $report.lapsCompleted 2 'completed lap count'
$evidence = $report.completedFrameEvidence
Assert-Equal $evidence.status 'complete' 'completion ledger status'
Assert-Equal $evidence.invalidRun $false 'completion ledger invalid-run marker'
foreach ($name in @('expected','completed','cpuAccepted')) { Assert-Equal $evidence.counts.$name $workloadSpec.measured "ledger count $name" }
foreach ($name in @('rejected','cancelled','cpuRejected','outstanding')) { Assert-Equal $evidence.counts.$name 0 "ledger count $name" }
Assert-Equal $evidence.rows.Count $workloadSpec.measured 'completion row count'
Assert-Equal $evidence.gpuStatusCounts.denominator $workloadSpec.measured 'GPU completion denominator'
Assert-Equal $evidence.gpuStatusCounts.valid $workloadSpec.measured 'valid GPU completion count'
foreach ($property in $evidence.gpuStatusCounts.PSObject.Properties) {
    if ($property.Name -notin @('denominator','valid')) { Assert-Equal $property.Value 0 "GPU status $($property.Name)" }
}
foreach ($property in $evidence.failureReasonCounts.PSObject.Properties) { Assert-Equal $property.Value 0 "failure reason $($property.Name)" }

$zoneRowsExpected = [ordered]@{}
foreach ($zone in $workloadSpec.zonePlan) { $zoneRowsExpected[$zone.name] = [int]$zone.frames }
$routeRows = @()
foreach ($zone in $workloadSpec.rowPlan) { for ($n=0; $n -lt $zone.frames; $n++) { $routeRows += [string]$zone.name } }
$lastCompletion = 0L
$lastAttempt = 0L
$lastRecord = 0L
$lastSubmission = 0L
$strategies = [ordered]@{}
$allGpuMs = [Collections.Generic.List[double]]::new()
$gpuByZone = [ordered]@{}
for ($index=0; $index -lt $workloadSpec.measured; $index++) {
    $row = $evidence.rows[$index]
    Assert-Equal $row.index $index 'completion row index'
    Assert-Equal $row.cpuSampleIndex $index "owning CPU sample join at row $index"
    Assert-Equal $row.lap 2 "measured lap at row $index"
    Assert-Equal $row.disposition 'completed' "completion disposition at row $index"
    Assert-Equal $row.failure 'none' "completion failure at row $index"
    Assert-Equal $row.presentationOutcome 'presented' "presentation outcome at row $index"
    Assert-Equal $row.cpuStageStatus 'valid' "CPU stage status at row $index"
    Assert-Equal $row.diagnosticStatus 'compiled-out' "Shipping diagnostic status at row $index"
    Assert-Equal $row.diagnosticCounters $null "Shipping diagnostic counters at row $index"
    Assert-Equal $row.gpuStatus 'valid' "GPU status at row $index"
    Assert-Equal $row.cpuAccepted $true "CPU admission at row $index"
    Assert-Equal $row.zoneName $routeRows[$index] "named workload zone at row $index"
    $row.zoneName = [string]$row.zoneName
    Assert-Equal $row.activeStrategy $workloadSpec.expectedStrategy "actual active ray strategy at row $index"
    if (-not $strategies.Contains($row.activeStrategy)) { $strategies[$row.activeStrategy] = 0 }
    ++$strategies[$row.activeStrategy]
    foreach ($name in @('sceneEpoch','measurementGeneration','recordAttemptSerial','recordSerial','simulationTick','frameSlot','submissionSerial')) {
        if ($null -eq $row.submittedIdentity.$name -or $null -eq $row.completionIdentity.$name) { throw "Missing submitted/completed identity $name at row $index" }
        Assert-Equal $row.submittedIdentity.$name $row.completionIdentity.$name "owning-frame identity $name at row $index"
    }
    Assert-Equal $row.submittedIdentity.sceneEpoch $evidence.sceneEpoch "ledger scene epoch at row $index"
    Assert-Equal $row.completionIdentity.sceneEpoch $evidence.sceneEpoch "completed ledger scene epoch at row $index"
    Assert-Equal $row.submittedIdentity.measurementGeneration $evidence.measurementGeneration "ledger measurement generation at row $index"
    Assert-Equal $row.completionIdentity.measurementGeneration $evidence.measurementGeneration "completed ledger measurement generation at row $index"
    Assert-Equal $row.submittedIdentity.frameSlot 0 "one-frame-in-flight slot at row $index"
    Assert-Equal $row.completionIdentity.frameSlot 0 "completed one-frame-in-flight slot at row $index"
    $attempt = [long]$row.submittedIdentity.recordAttemptSerial
    $record = [long]$row.submittedIdentity.recordSerial
    $submission = [long]$row.submittedIdentity.submissionSerial
    if ($attempt -le 0 -or $record -le 0 -or $submission -le 0 -or $attempt -le $lastAttempt -or $record -le $lastRecord -or $submission -le $lastSubmission) {
        throw "Non-positive or non-monotonic attempt/record/submission serial at row $index"
    }
    $lastAttempt = $attempt; $lastRecord = $record; $lastSubmission = $submission
    if ($null -eq $row.completionIdentity.completionSerial) { throw "Missing completion serial at row $index" }
    $completion = [long]$row.completionIdentity.completionSerial
    if ($completion -le $lastCompletion) { throw "Non-monotonic or duplicate completion serial at row $index" }
    $lastCompletion = $completion
    $gpuMs = [double]$row.gpuDurationNanoseconds / 1000000.0
    if ($gpuMs -le 0 -or [double]::IsNaN($gpuMs) -or [double]::IsInfinity($gpuMs)) { throw "Invalid owning GPU duration at row $index" }
    $allGpuMs.Add($gpuMs)
    if (-not $gpuByZone.Contains($row.zoneName)) { $gpuByZone[$row.zoneName] = [Collections.Generic.List[double]]::new() }
    $gpuByZone[$row.zoneName].Add($gpuMs)
}
if ($trial.workload -ceq 'showcase-route-v1') {
    Assert-Equal $report.routeTraversalComplete $true 'route traversal completion'
    Assert-Equal $report.waypointsReached 26 'route waypoint denominator'
} else {
    Assert-Equal $report.routeTraversalComplete $false 'non-route workload route marker'
}
Assert-Equal $report.zones.Count $zoneRowsExpected.Count 'named zone denominator count'
foreach ($name in $zoneRowsExpected.Keys) {
    $reportedZone = @($report.zones | Where-Object name -CEQ $name)
    Assert-Equal $reportedZone.Count 1 "reported named zone $name"
    Assert-Equal $reportedZone[0].frames $zoneRowsExpected[$name] "reported denominator for zone $name"
    $observedZoneCount = if ($gpuByZone.Contains($name)) { $gpuByZone[$name].Count } else { 0 }
    Assert-Equal $observedZoneCount $zoneRowsExpected[$name] "owning completion denominator for zone $name"
}

$gpuMedian = Get-Median $allGpuMs.ToArray()
$gpuP95 = Get-NearestRank $allGpuMs.ToArray() 0.95
if ([Math]::Abs($gpuMedian - [double]$evidence.gpuRtDurationMs.medianMilliseconds) -gt 0.002) { throw 'GPU median does not reproduce from owning completion rows.' }
$zoneGpu = @()
foreach ($name in $zoneRowsExpected.Keys) {
    $values = if ($gpuByZone.Contains($name)) { $gpuByZone[$name].ToArray() } else { @() }
    $zoneGpu += [ordered]@{ zone=$name; denominator=$values.Count; medianMs=if($values.Count){[Math]::Round((Get-Median $values),4)}else{$null}; p95NearestRankMs=if($values.Count){[Math]::Round((Get-NearestRank $values 0.95),4)}else{$null} }
}
$cpuStageNames = @('simulationStepCpuMs','skinCpuMs','playerSkinCpuMs','characterSkinCpuMs','dynamicUploadCpuMs','blasRefitRecordCpuMs','tlasUpdateRecordCpuMs','traceCopyRecordCpuMs','frameFenceWaitCpuMs','imageAcquireCpuMs','queueSubmitCpuMs','presentCallCpuMs','wholeFrameCycleCpuMs')
$cpuStages = [ordered]@{}
foreach ($name in $cpuStageNames) {
    $stage = $evidence.cpuStages.PSObject.Properties[$name].Value
    if ($null -eq $stage) { throw "Missing native CPU stage summary '$name'." }
    Assert-Equal $stage.sampleCount $workloadSpec.measured "native CPU stage sample count $name"
    $cpuStages[$name] = [ordered]@{ sampleCount=$stage.sampleCount; medianMs=$stage.medianMilliseconds; p95Ms=$stage.p95Milliseconds; slowestOnePercentMeanMs=$stage.slowestOnePercentMeanMilliseconds }
}
if ([Math]::Abs([double]$evidence.cpuStages.wholeFrameCycleCpuMs.medianMilliseconds - [double]$report.overall.medianMs) -gt 0.002 -or
    [Math]::Abs([double]$evidence.cpuStages.wholeFrameCycleCpuMs.p95Milliseconds - [double]$report.overall.p95Ms) -gt 0.002) {
    throw 'Native full-cycle timing does not agree with top-level measured summary.'
}
$context = @()
if (Test-Path -LiteralPath $contextPath -PathType Leaf) {
    $context = @(Get-Content -LiteralPath $contextPath | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | ForEach-Object { $_ | ConvertFrom-Json -DateKind String })
}
$contextSummary = [ordered]@{ sampleCount=$context.Count; firstUtc=$null; lastUtc=$null; batteryStartC=$null; batteryEndC=$null; thermalStatuses=@(); gpuThermalPowerLevels=@() }
if ($context.Count -gt 0) {
    $contextSummary.firstUtc = $context[0].utc
    $contextSummary.lastUtc = $context[-1].utc
    $contextSummary.batteryStartC = $context[0].batteryC
    $contextSummary.batteryEndC = $context[-1].batteryC
    $contextSummary.thermalStatuses = @($context.thermalStatus | Where-Object { $null -ne $_ } | Sort-Object -Unique)
    $contextSummary.gpuThermalPowerLevels = @($context.gpuThermalPowerLevel | Where-Object { $null -ne $_ } | Sort-Object -Unique)
}
$memoryBefore = Read-MemorySummary (Join-Path $runDirectory 'memory-before.txt')
$memoryAfter = Read-MemorySummary (Join-Path $runDirectory 'memory-after.txt')
$medianMs = [double]$report.overall.medianMs
$summary = [ordered]@{
    runId=$RunId; integrity='PASS'; analyzerScriptSha256=(Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256).Hash.ToLowerInvariant(); member=$trial.member; workload=$trial.workload
    sourceCommit=$trial.sourceCommit; deviceModel=$trial.deviceModel; requestedBackend=$trial.requestedBackend
    actualBackend=$report.executionBackend; apkSha256=$trial.installedApkSha256; packageReceiptSha256=(Get-FileHash -LiteralPath $PackageReceipt -Algorithm SHA256).Hash.ToLowerInvariant()
    scalePercent=$report.renderScalePercent; internalExtent=$report.internalExtent; presentationExtent=$report.presentationExtent
    measuredFrames=$workloadSpec.measured; namedZoneDenominators=$report.zones
    shaderIdentity=$report.shader; actualActiveStrategyCounts=$strategies
    gpuOwningCompletion=[ordered]@{ sampleCount=$allGpuMs.Count; medianMs=[Math]::Round($gpuMedian,4); p95NearestRankMs=[Math]::Round($gpuP95,4); byZone=$zoneGpu }
    nativeCpuStages=$cpuStages
    nativeCycle=[ordered]@{ scope='android-render-entry-through-present'; medianMs=$medianMs; p95Ms=$report.overall.p95Ms; medianDerivedFps=[Math]::Round(1000.0/$medianMs,3); displayPacingProof=$false }
    thermalContext=$contextSummary
    ramBefore=$memoryBefore; ramAfter=$memoryAfter
    diagnosticReadback=[ordered]@{ status='compiled-out'; nullCounters=$true; provesPhysicalGlassCorrectness=$false }
    performanceAcceptance='none'; optimizationAdmission='none'; visualAcceptance='pending owner review';
    limitations=@('Native-cycle median-derived FPS is not display-pacing proof.','GPU command-buffer timing includes AS build/update, RT and copy work; it is not isolated shader timing.','Context samples are not aligned to individual measured frames.','RAM before/after are non-frame-aligned boundary samples from different lifecycle states: benchmark completion hides the SurfaceView and destroys the RT scene, so a lower after reading is not a memory/performance improvement. Null fields mean the source did not provide parseable values.','Physical timing does not prove scene correctness. The open-aperture visual blocker remains open until owner review, including S24 hands, one-skeleton, and yellow-water concerns.','No matched repeated S24 candidate/control performance acceptance is implied.')
}
$analysisPath = Join-Path $runDirectory 'analysis.json'
if (Test-Path -LiteralPath $analysisPath) { throw "Refusing to overwrite existing trial analysis: $analysisPath" }
$summary | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $analysisPath -Encoding utf8NoBOM
Write-Output "$RunId integrity PASS: $($workloadSpec.measured) owning rows; GPU median/p95 $([Math]::Round($gpuMedian,4))/$([Math]::Round($gpuP95,4)) ms; native-cycle median/p95 $($report.overall.medianMs)/$($report.overall.p95Ms) ms; performance acceptance remains none."
