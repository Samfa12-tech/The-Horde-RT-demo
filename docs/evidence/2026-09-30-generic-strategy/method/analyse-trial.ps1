param([Parameter(Mandatory=$true)][string]$RunId)
$ErrorActionPreference='Stop'
$directory=Join-Path $PSScriptRoot "phone/$RunId"
$report=Get-Content (Join-Path $directory "$RunId/benchmark.json") -Raw | ConvertFrom-Json -DateKind String
$marker=Get-Content (Join-Path $directory "$RunId/result.json") -Raw | ConvertFrom-Json -DateKind String
$trial=Get-Content (Join-Path $directory 'trial.json') -Raw | ConvertFrom-Json -DateKind String
$context=@(Get-Content (Join-Path $directory 'context-samples.jsonl') | ForEach-Object {$_ | ConvertFrom-Json -DateKind String})
function Equal($actual,$expected,[string]$label) {if($actual -cne $expected){throw "$label expected '$expected', found '$actual'"}}
Equal $marker.runId $RunId 'marker identity'; Equal $marker.status 'complete' 'marker status'
Equal $report.runId $RunId 'report identity'; Equal $report.status 'complete' 'report status'
Equal $report.result 'complete' 'report result'; Equal $report.workload $trial.workload 'workload'
Equal $report.workloadComplete $true 'workload completion'; Equal $report.presentedEveryFrame $true 'presentation'
Equal $report.renderScalePercent 75 'scale'; Equal $report.executionBackend 'RayTracingPipeline' 'backend'
Equal $report.rtMode 'RayTracingPipeline' 'RT mode'; Equal $report.presentMode 'MAILBOX' 'present mode'
Equal $report.internalExtent.width 1080 'internal width'; Equal $report.internalExtent.height 2235 'internal height'
Equal $report.presentationExtent.width 1440 'present width'; Equal $report.presentationExtent.height 2980 'present height'
Equal $report.materialEncoding 'ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2) + strict ASTC 6x6 lich' 'material encoding'
Equal $report.legacyFrameTimingScope 'android-render-entry-through-present' 'timing scope'
Equal $report.gpu 'Adreno (TM) 840' 'GPU'; Equal $trial.deviceModel 'SM-S948B' 'exact device'
Equal $trial.instrumentation 'Shipping' 'instrumentation'; Equal $trial.quality 'Mobile' 'quality'
Equal $trial.sourceCommit 'eafbf8262442a82e0835edf5cf5306d718e64633' 'trial source commit provenance'
$artifactSpec=switch -Exact ($trial.buildLabel) {
    'control' {@{containment='profile/provenance/containment-control-benchmark-shipping-mobile-eaf-scanner.json';containmentSha256='5bde1466796ef6020962cd9486235dd86a35d1e7802ce792b0f661f0f3f05f87';artifactKind='control';expectedStrategy='opaque-fast';contextOnly=$false}}
    'generic-profile' {@{containment='profile/provenance/containment-generic-benchmark-shipping-mobile.json';containmentSha256='75dd2814c23cc4e4430bd27c0b6f642d113228f4f02a5846d655150eddd34ce6';artifactKind='generic-profile';expectedStrategy='generic-dielectric';contextOnly=$false}}
    'restored-control-warmup-context-only' {@{containment='profile/provenance/containment-control-benchmark-shipping-mobile-eaf-scanner.json';containmentSha256='5bde1466796ef6020962cd9486235dd86a35d1e7802ce792b0f661f0f3f05f87';artifactKind='control';expectedStrategy='opaque-fast';contextOnly=$true}}
    default {throw 'Unknown exact artifact label'}
}
$profileRoot=Join-Path $PSScriptRoot 'profile'
$buildReceiptPath=Join-Path $profileRoot 'provenance/profile-build-receipt.json'
$buildReceipt=Get-Content $buildReceiptPath -Raw | ConvertFrom-Json
Equal $buildReceipt.source.head 'eafbf8262442a82e0835edf5cf5306d718e64633' 'profile source provenance'
Equal $buildReceipt.source.patchSha256 '0775f22d28e91966707db8e62407eb5738c1654e97da30fa239792f8dbd4c474' 'investigation patch identity'
Equal $buildReceipt.moduleEqualityAllFour $true 'four packaged modules match control'
Equal (Get-FileHash -LiteralPath $buildReceiptPath -Algorithm SHA256).Hash.ToLowerInvariant() '2d233deb7694d6aea42a4c2c2cef6eeef368a666d01b2df1293b556f79dfd26d' 'immutable profile build receipt hash'
if($artifactSpec.artifactKind -ceq 'control'){
    $artifactPath=$buildReceipt.control.apk
    $artifactSha=$buildReceipt.control.apkSha256
} else {
    $artifactRows=@($buildReceipt.apks | Where-Object kind -CEQ 'Shipping/Mobile benchmark')
    Equal $artifactRows.Count 1 'profile benchmark artifact receipt count'
    $artifactPath=$artifactRows[0].path
    $artifactSha=$artifactRows[0].sha256
}
Equal (Get-FileHash -LiteralPath $artifactPath -Algorithm SHA256).Hash.ToLowerInvariant() $artifactSha 'immutable APK file versus build receipt'
Equal $trial.installedApkSha256 $artifactSha 'installed APK versus immutable build receipt'
$containmentPath=Join-Path $PSScriptRoot $artifactSpec.containment
Equal (Get-FileHash -LiteralPath $containmentPath -Algorithm SHA256).Hash.ToLowerInvariant() $artifactSpec.containmentSha256 'immutable package containment receipt hash'
$containment=Get-Content $containmentPath -Raw | ConvertFrom-Json
Equal $containment.instrumentation 'Shipping' 'actual packaged instrumentation'
Equal $containment.quality 'Mobile' 'actual packaged quality'
Equal $containment.apk $artifactPath 'containment APK identity path'
Equal $containment.packaged.targetSha256 $containment.stripped.targetSha256 'stripped versus packaged native library'
$modules=$containment.packaged.modules
$pipeline=@($modules | Where-Object backend -CEQ 'RayTracingPipeline')
Equal $pipeline.Count 2 'actual APK pipeline module count'
$modulesByVariant=[ordered]@{}
foreach($variantKey in @('shipping_mobile_opaque_fast','shipping_mobile_generic_dielectric')){
    $receiptRows=@($buildReceipt.shippingPackagedModules | Where-Object semanticKey -CEQ $variantKey)
    Equal $receiptRows.Count 1 "immutable module receipt $variantKey"
    $expectedHash=if($artifactSpec.artifactKind -ceq 'control'){$receiptRows[0].controlSha256}else{$receiptRows[0].candidateSha256}
    $matchingModule=@($pipeline | Where-Object sha256 -CEQ $expectedHash)
    Equal $matchingModule.Count 1 "actual APK module for $variantKey"
    $modulesByVariant[$variantKey]=$matchingModule[0].sha256
}
Equal $report.shader "opaqueFast:shipping_mobile_opaque_fast@$($modulesByVariant['shipping_mobile_opaque_fast'])|genericDielectric:shipping_mobile_generic_dielectric@$($modulesByVariant['shipping_mobile_generic_dielectric'])" 'loaded artifact pair versus actual APK shader identity'
$expected=if($report.workload -eq 'showcase-route-v1'){1838}else{600}
Equal $report.measuredFrames $expected 'measured count'
if($report.shader -notmatch '^opaqueFast:shipping_mobile_opaque_fast@[a-f0-9]{64}\|genericDielectric:shipping_mobile_generic_dielectric@[a-f0-9]{64}$'){throw 'Not fixed Shipping/Mobile pipeline variant identity'}
$evidence=$report.completedFrameEvidence
Equal $evidence.status 'complete' 'ledger status'; Equal $evidence.invalidRun $false 'invalid run'
foreach($name in @('expected','completed','cpuAccepted')){Equal $evidence.counts.$name $expected "count $name"}
foreach($name in @('rejected','cancelled','cpuRejected','outstanding')){Equal $evidence.counts.$name 0 "count $name"}
foreach($property in $evidence.failureReasonCounts.PSObject.Properties){Equal $property.Value 0 "failure $($property.Name)"}
Equal $evidence.rows.Count $expected 'row count'; Equal $evidence.gpuStatusCounts.valid $expected 'GPU valid count'
foreach($property in $evidence.gpuStatusCounts.PSObject.Properties){if($property.Name -notin @('valid','denominator')){Equal $property.Value 0 "GPU status $($property.Name)"}}
$lastCompletion=0L
for($index=0;$index -lt $expected;++$index){
    $row=$evidence.rows[$index]
    Equal $row.index $index 'row index'; Equal $row.cpuSampleIndex $index 'CPU join'; Equal $row.lap 2 'measured lap'
    Equal $row.disposition 'completed' 'disposition'; Equal $row.failure 'none' 'row failure'
    Equal $row.presentationOutcome 'presented' 'presentation outcome'; Equal $row.cpuStageStatus 'valid' 'CPU status'
    Equal $row.diagnosticStatus 'compiled-out' 'diagnostic status'; Equal $row.diagnosticCounters $null 'no diagnostic readback'
    Equal $row.gpuStatus 'valid' 'GPU status'; Equal $row.cpuAccepted $true 'CPU accepted'
    foreach($name in @('sceneEpoch','measurementGeneration','recordAttemptSerial','recordSerial','simulationTick','frameSlot','submissionSerial')){
        if($null -eq $row.submittedIdentity.$name -or $null -eq $row.completionIdentity.$name){throw "Missing identity $name at row $index"}
        Equal $row.submittedIdentity.$name $row.completionIdentity.$name "identity $name at row $index"
    }
    $serial=[long]$row.completionIdentity.completionSerial
    if($serial -le $lastCompletion -or $row.gpuDurationNanoseconds -le 0){throw "Bad serial/timestamp at row $index"}
    $lastCompletion=$serial
}
if($report.workload -eq 'showcase-route-v1'){
    Equal $report.routeTraversalComplete $true 'route complete'; Equal $report.waypointsReached 26 'route waypoint count'
    $zones=[ordered]@{'opening'=160;'skeleton-room'=98;'shadow-corridor'=599;'skylight-chamber'=189;'yellow-torch-bay'=157;'blue-torch-bay'=157;'red-torch-bay'=157;'green-torch-bay'=157;'transmission-threshold'=63;'finale'=101}
    Equal $report.zones.Count $zones.Count 'zone count'
    foreach($name in $zones.Keys){$zone=@($report.zones | Where-Object name -CEQ $name); Equal $zone.Count 1 "zone $name"; Equal $zone[0].frames $zones[$name] "zone $name frames"}
}
$gpu=@($evidence.rows | ForEach-Object {[double]$_.gpuDurationNanoseconds/1e6} | Sort-Object)
$gpuMedian=if($gpu.Count%2){$gpu[[int][math]::Floor($gpu.Count/2)]}else{($gpu[$gpu.Count/2-1]+$gpu[$gpu.Count/2])/2}
if([math]::Abs($gpuMedian-$evidence.gpuRtDurationMs.medianMilliseconds) -gt 0.0001){throw 'GPU median not reproduced from completion-owned rows'}
$strategyExported=$null -ne $evidence.rows[0].PSObject.Properties['activeStrategy']
$strategyCounts=[ordered]@{}
$openingStrategyCounts=[ordered]@{}
$zoneGpu=@()
foreach($zoneName in @($evidence.rows.zoneName | Sort-Object -Unique)){
    $zoneRows=@($evidence.rows | Where-Object zoneName -CEQ $zoneName)
    $durations=@($zoneRows | ForEach-Object {[double]$_.gpuDurationNanoseconds/1e6} | Sort-Object)
    $middle=[int][math]::Floor($durations.Count/2)
    $median=if($durations.Count%2){$durations[$middle]}else{($durations[$middle-1]+$durations[$middle])/2}
    $zoneGpu+=[ordered]@{zone=$zoneName;frames=$durations.Count;medianMs=$median;p95NearestRankMs=$durations[[int][math]::Ceiling($durations.Count*0.95)-1];within33_333Ms=@($durations | Where-Object {$_ -le 33.333}).Count}
}
if($strategyExported){
    foreach($row in $evidence.rows){
        if($row.activeStrategy -cnotin @('opaque-fast','generic-dielectric')){throw "Invalid owning strategy at row $($row.index)"}
        if(-not $strategyCounts.Contains($row.activeStrategy)){$strategyCounts[$row.activeStrategy]=0}
        ++$strategyCounts[$row.activeStrategy]
        if($row.zoneName -ceq 'opening'){
            if(-not $openingStrategyCounts.Contains($row.activeStrategy)){$openingStrategyCounts[$row.activeStrategy]=0}
            ++$openingStrategyCounts[$row.activeStrategy]
        }
    }
}
if($trial.buildLabel -cin @('control','generic-profile','restored-control-warmup-context-only')){
    Equal $strategyExported $true 'owning strategy export'
    Equal $strategyCounts[$artifactSpec.expectedStrategy] $expected "all rows selected $($artifactSpec.expectedStrategy)"
    Equal $strategyCounts.Count 1 'single actual strategy across full measured route'
    if($report.workload -ceq 'showcase-route-v1'){
        Equal $openingStrategyCounts[$artifactSpec.expectedStrategy] 160 "opening $($artifactSpec.expectedStrategy) completions"
        Equal $openingStrategyCounts.Count 1 'single actual opening strategy'
    }
}
$summary=[ordered]@{
    runId=$RunId;integrity='PASS';sourceCommit=$trial.sourceCommit;apkSha256=$trial.installedApkSha256;buildLabel=$trial.buildLabel
    workload=$report.workload;deviceModel=$trial.deviceModel;shader=$report.shader;frames=$expected;presentMode=$report.presentMode
    internalExtent=$report.internalExtent;overall=$report.overall;medianDerivedFps=[math]::Round(1000/$report.overall.medianMs,3)
    gpuRtDurationMs=$evidence.gpuRtDurationMs;cpuStages=$evidence.cpuStages;zones=$report.zones
    actualPipelineModuleSha256=$modulesByVariant
    owningActiveStrategy=[ordered]@{exported=$strategyExported;counts=$strategyCounts;openingCounts=$openingStrategyCounts}
    zoneGpuCommandBufferTiming=$zoneGpu
    diagnosticRowsCompiledOut=$expected;firstMeasuredSimulationTick=$evidence.rows[0].submittedIdentity.simulationTick;lastMeasuredSimulationTick=$evidence.rows[-1].submittedIdentity.simulationTick
    context=[ordered]@{samples=$context.Count;firstUtc=$context[0].utc;lastUtc=$context[-1].utc;batteryStartC=$context[0].batteryC;batteryEndC=$context[-1].batteryC;thermalStatuses=@($context.thermalStatus | Sort-Object -Unique);gpuThermalPowerLevels=@($context.gpuThermalPowerLevel | Sort-Object -Unique)}
    limitations=@('Thermal context starts after am-start observation and is not frame-aligned measured-lap telemetry.','Render-entry-through-present timing is not display pacing or input latency.','GPU command-buffer timestamps include AS work/RT/copy, not isolated shader timing.','Shipping compiled-out counters do not prove glass correctness.','No causal A/B acceptance from this individual trial.')
}
if($artifactSpec.artifactKind -ceq 'generic-profile'){
    $summary['physicalEvidenceClass']='investigation-only-whole-strategy'
    $summary['gainClaim']='none'
    $summary['physicalAcceptance']='not-evaluated'
    $summary['optimizationAcceptance']='none'
    $summary.limitations+=@('Generic Dielectric is the whole selected strategy on the same scene, including its compile-time opaque spawn and ordered-shadow algorithm; this does not isolate compiler occupancy or glass-only cost.','No strict pixel-equivalence or performance gain is claimed; inspect ordinary opening captures before comparing timing.')
} elseif($artifactSpec.contextOnly){
    $summary['physicalEvidenceClass']='restored-control-warmup-context-only'
    $summary['gainClaim']='none'
    $summary['physicalAcceptance']='not-evaluated'
    $summary['includedInAbba']=$false
    $summary.limitations+=@('This restored-control warmup is context only and is excluded from all ABBA metrics; no warmup frames or statistics are pooled.')
} else {
    $summary['physicalEvidenceClass']='exact-control-reference'
    $summary['gainClaim']='none'
    $summary['physicalAcceptance']='not-evaluated'
}
$summaryPath=Join-Path $directory 'analysis.json'
if(Test-Path -LiteralPath $summaryPath){throw 'Refuse to overwrite existing trial analysis.'}
$summary | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $summaryPath -Encoding utf8NoBOM
Write-Output "$RunId integrity PASS: $expected rows; median $($report.overall.medianMs) ms ($($summary.medianDerivedFps) FPS), p95 $($report.overall.p95Ms) ms; GPU median $gpuMedian ms."
