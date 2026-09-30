param(
    [Parameter(Mandatory=$true)][string]$RunPath,
    [Parameter(Mandatory=$true)][ValidateSet('control','isolate')][string]$ArtifactRole
)
$ErrorActionPreference='Stop'
$directory=(Resolve-Path -LiteralPath $RunPath -ErrorAction Stop).Path
$trial=Get-Content (Join-Path $directory 'trial.json') -Raw | ConvertFrom-Json -DateKind String
$RunId=[string]$trial.runId
if($RunId -notmatch '^[A-Za-z0-9_-]+$'){throw "Unsafe run identity: $RunId"}
$report=Get-Content (Join-Path $directory "$RunId/benchmark.json") -Raw | ConvertFrom-Json -DateKind String
$marker=Get-Content (Join-Path $directory "$RunId/result.json") -Raw | ConvertFrom-Json -DateKind String
$context=@(Get-Content (Join-Path $directory 'context-samples.jsonl') | ForEach-Object {$_ | ConvertFrom-Json -DateKind String})
function Equal($actual,$expected,[string]$label) {if($actual -cne $expected){throw "$label expected '$expected', found '$actual'"}}
Equal $trial.runId $RunId 'trial identity'
Equal $marker.runId $RunId 'marker identity'; Equal $marker.status 'complete' 'marker status'
Equal $report.runId $RunId 'report identity'; Equal $report.status 'complete' 'report status'
Equal $report.result 'complete' 'report result'; Equal $report.workload $trial.workload 'workload'
if($report.workload -notin @('showcase-route-v1','lantern-held-high-v1','lantern-reveal-sequence-v1')){throw "Unsupported all-shadow workload: $($report.workload)"}
if($RunId -match '^all-shadow-c[12](-|$)'){Equal $ArtifactRole 'control' 'ABBA control position'}
if($RunId -match '^all-shadow-s[12](-|$)'){Equal $ArtifactRole 'isolate' 'ABBA isolate position'}
Equal $report.workloadComplete $true 'workload completion'; Equal $report.presentedEveryFrame $true 'presentation'
Equal $report.renderScalePercent 75 'scale'; Equal $report.executionBackend 'RayTracingPipeline' 'backend'
Equal $report.rtMode 'RayTracingPipeline' 'RT mode'; Equal $report.presentMode 'MAILBOX' 'present mode'
Equal $report.internalExtent.width 1080 'internal width'; Equal $report.internalExtent.height 2235 'internal height'
Equal $report.presentationExtent.width 1440 'present width'; Equal $report.presentationExtent.height 2980 'present height'
Equal $report.materialEncoding 'ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2) + strict ASTC 6x6 lich' 'material encoding'
Equal $report.legacyFrameTimingScope 'android-render-entry-through-present' 'timing scope'
Equal $report.gpu 'Adreno (TM) 840' 'GPU'; Equal $trial.deviceModel 'SM-S948B' 'exact device'
Equal $trial.instrumentation 'Shipping' 'instrumentation'; Equal $trial.quality 'Mobile' 'quality'
$buildReceipt=Get-Content (Join-Path $PSScriptRoot 'build-receipt.json') -Raw | ConvertFrom-Json
$artifactSpec=$buildReceipt.$ArtifactRole
$expectedApk=if($ArtifactRole -ceq 'control'){'a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033'}else{'b57abb9127929665102a9de70b44085d6c469aeeee10fb016f09d6ffbd6d4829'}
Equal $artifactSpec.apkSha256 $expectedApk "sealed $ArtifactRole APK receipt identity"
Equal $trial.installedApkSha256 $expectedApk 'installed APK versus exact all-shadow artifact'
if($RunId -notmatch '^all-shadow-(c1|s1|s2|c2)(-[A-Za-z0-9_]+)*$'){throw "Unexpected all-shadow run identity: $RunId"}
if($RunId -match '^all-shadow-c[12](-|$)'){Equal $ArtifactRole 'control' 'ABBA control position'}
if($RunId -match '^all-shadow-s[12](-|$)'){Equal $ArtifactRole 'isolate' 'ABBA isolate position'}
Equal $trial.sourceCommit '71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95' 'source commit'
$expectedBuildLabel=if($ArtifactRole -ceq 'control'){'all-shadow-control'}else{'all-shadow-isolate'}
Equal $trial.buildLabel $expectedBuildLabel 'all-shadow build label'
$artifactDirectory=Join-Path $PSScriptRoot "artifacts/$ArtifactRole"
$containmentPath=Join-Path $artifactDirectory 'containment.json'
$containment=Get-Content $containmentPath -Raw | ConvertFrom-Json
Equal $containment.arm64Sha256 $artifactSpec.nativeLibrarySha256 'actual packaged native library identity'
$modules=$containment.packaged.modules
$pipeline=@($modules | Where-Object backend -CEQ 'RayTracingPipeline')
Equal $pipeline.Count 2 'actual APK pipeline module count'
$catalog=Get-Content (Join-Path $artifactDirectory 'raygen-variant-catalog.json') -Raw | ConvertFrom-Json
$modulesByVariant=[ordered]@{}
foreach($variantKey in @('shipping_mobile_opaque_fast','shipping_mobile_generic_dielectric')){
    $variant=@($catalog.variants | Where-Object key -CEQ $variantKey)
    Equal $variant.Count 1 "catalog variant $variantKey"
    Equal $variant[0].instrumentation 'Shipping' "catalog instrumentation $variantKey"
    Equal $variant[0].quality 'Mobile' "catalog quality $variantKey"
    $matchingModule=@($pipeline | Where-Object sha256 -CEQ $variant[0].spirvSha256)
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
Equal $evidence.rows.Count $expected 'row count'; Equal $evidence.gpuStatusCounts.valid $expected 'GPU valid count'; Equal $evidence.gpuStatusCounts.denominator $expected 'GPU denominator'
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
        if([long]$row.submittedIdentity.$name -lt 0 -or [long]$row.completionIdentity.$name -lt 0){throw "Negative identity $name at row $index"}
        Equal $row.submittedIdentity.$name $row.completionIdentity.$name "identity $name at row $index"
    }
    $serial=[long]$row.completionIdentity.completionSerial
    if($null -eq $row.completionIdentity.completionSerial -or $serial -le $lastCompletion -or [long]$row.gpuDurationNanoseconds -le 0){throw "Bad serial/timestamp at row $index"}
    $lastCompletion=$serial
}
if($report.workload -eq 'showcase-route-v1'){
    Equal $report.routeTraversalComplete $true 'route complete'; Equal $report.waypointsReached 26 'route waypoint count'
    $zones=[ordered]@{'opening'=160;'skeleton-room'=98;'shadow-corridor'=599;'skylight-chamber'=189;'yellow-torch-bay'=157;'blue-torch-bay'=157;'red-torch-bay'=157;'green-torch-bay'=157;'transmission-threshold'=63;'finale'=101}
    Equal $report.zones.Count $zones.Count 'zone count'
    foreach($name in $zones.Keys){$zone=@($report.zones | Where-Object name -CEQ $name); Equal $zone.Count 1 "zone $name"; Equal $zone[0].frames $zones[$name] "zone $name frames"}
}
if($report.workload -eq 'lantern-reveal-sequence-v1'){
    # This exact sequence was previously measured as 600 frames entirely in
    # yellow-torch-bay; the complete ten-zone schema remains present.
    $liveZones=[ordered]@{'opening'=0;'skeleton-room'=0;'shadow-corridor'=0;'skylight-chamber'=0;'yellow-torch-bay'=600;'blue-torch-bay'=0;'red-torch-bay'=0;'green-torch-bay'=0;'transmission-threshold'=0;'finale'=0}
    Equal $report.zones.Count $liveZones.Count 'live sequence zone count'
    foreach($name in $liveZones.Keys){$zone=@($report.zones | Where-Object name -CEQ $name); Equal $zone.Count 1 "live zone $name"; Equal $zone[0].frames $liveZones[$name] "live zone $name frames"}
}
$gpuStats=$evidence.gpuRtDurationMs
Equal $gpuStats.sampleCount $expected 'GPU timing sample count'
foreach($stat in @('meanMilliseconds','medianMilliseconds','p90Milliseconds','p95Milliseconds','slowestOnePercentMeanMilliseconds')){if([double]$gpuStats.$stat -lt 0){throw "Negative GPU timing statistic $stat"}}
foreach($stage in $evidence.cpuStages.PSObject.Properties){
    Equal $stage.Value.sampleCount $expected "CPU stage sample count $($stage.Name)"
    foreach($stat in $stage.Value.PSObject.Properties){
        if($stat.Name -ne 'sampleCount' -and [double]$stat.Value -lt 0){throw "Negative CPU timing statistic $($stage.Name).$($stat.Name)"}
    }
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
Equal $strategyExported $true 'owning strategy export'
Equal (($strategyCounts.Values | Measure-Object -Sum).Sum) $expected 'owning strategy row denominator'
if($report.workload -ceq 'showcase-route-v1'){
    Equal $openingStrategyCounts['opaque-fast'] 160 'actual opening OpaqueFast completions'
    Equal $openingStrategyCounts.Count 1 'no unexpected opening strategy'
}
$routeStageSplit=$null
$liveSequenceStages=$null
if($report.workload -ceq 'showcase-route-v1'){
    $openingGpu=@($zoneGpu | Where-Object zone -CEQ 'opening')
    Equal $openingGpu.Count 1 'opening zone GPU timing'
    $openingZone=@($report.zones | Where-Object name -CEQ 'opening')
    Equal $openingZone.Count 1 'opening zone CPU stage summary'
    $routeStageSplit=[ordered]@{
        opening=[ordered]@{frames=$openingGpu[0].frames;gpuCommandBuffer=$openingGpu[0];cpuStages=$openingZone[0].cpuStages}
        heavyZones=@($zoneGpu | Where-Object zone -CNE 'opening')
        heavyZoneCpuStages=@($report.zones | Where-Object name -CNE 'opening' | ForEach-Object {[ordered]@{zone=$_.name;frames=$_.frames;cpuStages=$_.cpuStages}})
        gpuStageBoundary='The current benchmark schema emits a completion-owned GPU command-buffer interval per frame/zone, not separate GPU AS-build and RT-dispatch durations. CPU BlasRefitRecord/TlasUpdateRecord/TraceCopyRecord are CPU record-stage timings, not GPU-stage timings.'
    }
}
if($report.workload -ceq 'lantern-reveal-sequence-v1'){
    $liveGpu=@($zoneGpu | Where-Object zone -CEQ 'yellow-torch-bay')
    Equal $liveGpu.Count 1 'live reveal zone GPU timing'
    $liveZone=@($report.zones | Where-Object name -CEQ 'yellow-torch-bay')
    Equal $liveZone.Count 1 'live reveal zone CPU stage summary'
    $liveSequenceStages=[ordered]@{
        zone='yellow-torch-bay';frames=$liveGpu[0].frames;gpuCommandBuffer=$liveGpu[0];cpuStages=$liveZone[0].cpuStages
        gpuStageBoundary='The current benchmark schema emits a completion-owned GPU command-buffer interval, not separate GPU AS-build and RT-dispatch durations. CPU BLAS/TLAS record timings are CPU-only.'
    }
}
$analysisDirectory=Join-Path $PSScriptRoot 'analyses'
if(-not (Test-Path -LiteralPath $analysisDirectory)){New-Item -ItemType Directory -Path $analysisDirectory | Out-Null}
$analysisPath=Join-Path $analysisDirectory "$RunId-$ArtifactRole.analysis.json"
if(Test-Path -LiteralPath $analysisPath){throw "Refusing to overwrite analysis: $analysisPath"}
$summary=[ordered]@{
    runId=$RunId;integrity='PASS';artifactRole=$ArtifactRole;sourceCommit=$trial.sourceCommit;apkSha256=$trial.installedApkSha256;buildLabel=$trial.buildLabel
    workload=$report.workload;deviceModel=$trial.deviceModel;shader=$report.shader;frames=$expected;presentMode=$report.presentMode
    internalExtent=$report.internalExtent;overall=$report.overall;medianDerivedFps=[math]::Round(1000/$report.overall.medianMs,3)
    gpuRtDurationMs=$evidence.gpuRtDurationMs;cpuStages=$evidence.cpuStages;zones=$report.zones
    owningActiveStrategy=[ordered]@{exported=$strategyExported;counts=$strategyCounts;openingCounts=$openingStrategyCounts}
    zoneGpuCommandBufferTiming=$zoneGpu
    openingVsHeavyRouteStages=$routeStageSplit
    liveRevealZoneStages=$liveSequenceStages
    diagnosticRowsCompiledOut=$expected;firstMeasuredSimulationTick=$evidence.rows[0].submittedIdentity.simulationTick;lastMeasuredSimulationTick=$evidence.rows[-1].submittedIdentity.simulationTick
    context=[ordered]@{samples=$context.Count;firstUtc=$context[0].utc;lastUtc=$context[-1].utc;batteryStartC=$context[0].batteryC;batteryEndC=$context[-1].batteryC;thermalStatuses=@($context.thermalStatus | Sort-Object -Unique);gpuThermalPowerLevels=@($context.gpuThermalPowerLevel | Sort-Object -Unique);processIds=@($context.processId | Sort-Object -Unique)}
    classification='all-direct-visibility-investigation';optimizationClaim='none';causalAcceptance='not-evaluated';productionPromotion='not-authorized'
    limitations=@('Thermal context starts after am-start observation and is not frame-aligned measured-lap telemetry.','Render-entry-through-present timing is not display pacing or input latency.','GPU command-buffer timestamps include AS work/RT/copy, not isolated shader timing.','This all-direct-visibility isolate removes real geometric blockers and dielectric/glass shadow attenuation; it is nonphysical.','CPU BLAS/TLAS record timings do not establish GPU AS cost; current GPU schema does not separate AS from RT dispatch.','This parser validates individual runs only; it does not calculate an ABBA effect or accept a performance claim.')
}
$summary | ConvertTo-Json -Depth 9 > $analysisPath
Write-Output "$RunId integrity PASS: $expected rows; median $($report.overall.medianMs) ms ($($summary.medianDerivedFps) FPS), p95 $($report.overall.p95Ms) ms; GPU median $gpuMedian ms."
