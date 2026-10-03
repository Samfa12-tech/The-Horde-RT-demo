param(
    [Parameter(Mandatory=$true)][string]$RunPath,
    [Parameter(Mandatory=$true)][ValidateSet('control','isolate')][string]$ArtifactRole
)
$ErrorActionPreference='Stop'
$directory=(Resolve-Path -LiteralPath $RunPath -ErrorAction Stop).Path
$trial=Get-Content (Join-Path $directory 'trial.json') -Raw | ConvertFrom-Json -DateKind String
$RunId=[string]$trial.runId
if($RunId -notmatch '^[A-Za-z0-9_-]+$'){throw "Unsafe run identity: $RunId"}
$reportPath=Join-Path $directory "$RunId/benchmark.json"
$markerPath=Join-Path $directory "$RunId/result.json"
$trialPath=Join-Path $directory 'trial.json'
$contextPath=Join-Path $directory 'context-samples.jsonl'
$report=Get-Content $reportPath -Raw | ConvertFrom-Json -DateKind String
$marker=Get-Content $markerPath -Raw | ConvertFrom-Json -DateKind String
$context=@(Get-Content (Join-Path $directory 'context-samples.jsonl') | ForEach-Object {$_ | ConvertFrom-Json -DateKind String})
function Equal($actual,$expected,[string]$label) {if($actual -cne $expected){throw "$label expected '$expected', found '$actual'"}}
Equal $trial.runId $RunId 'trial identity'
Equal $marker.runId $RunId 'marker identity'; Equal $marker.status 'complete' 'marker status'
Equal $report.runId $RunId 'report identity'; Equal $report.status 'complete' 'report status'
Equal $report.result 'complete' 'report result'; Equal $report.workload $trial.workload 'workload'
if($report.workload -notin @('showcase-route-v1','lantern-reveal-sequence-v1')){throw "Unsupported opaque-secondary workload: $($report.workload)"}
if($RunId -match '^opaque-secondary-c[12]-'){Equal $ArtifactRole 'control' 'ABBA control position'}
if($RunId -match '^opaque-secondary-s[12]-'){Equal $ArtifactRole 'isolate' 'ABBA isolate position'}
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
$expectedApk=if($ArtifactRole -ceq 'control'){'a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033'}else{'ce1c1548ed3937d1ab99b57c523b62a8a1e244538c01e9075bb2f7e56e766cce'}
Equal $artifactSpec.apkSha256 $expectedApk "sealed $ArtifactRole APK receipt identity"
Equal $trial.installedApkSha256 $expectedApk 'installed APK versus exact opaque-secondary artifact'
if($RunId -notmatch '^opaque-secondary-(c1|s1|s2|c2)-(route|live)-20261001$'){throw "Unexpected opaque-secondary run identity: $RunId"}
if($RunId -match '^opaque-secondary-c[12]-'){Equal $ArtifactRole 'control' 'ABBA control position'}
if($RunId -match '^opaque-secondary-s[12]-'){Equal $ArtifactRole 'isolate' 'ABBA isolate position'}
if($RunId -match '-route-'){Equal $report.workload 'showcase-route-v1' 'route run identity/workload'}
if($RunId -match '-live-'){Equal $report.workload 'lantern-reveal-sequence-v1' 'live reveal run identity/workload'}
Equal $trial.sourceCommit '71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95' 'source commit'
$expectedBuildLabel=if($ArtifactRole -ceq 'control'){'opaque-secondary-control'}else{'opaque-secondary-isolate'}
Equal $trial.buildLabel $expectedBuildLabel 'opaque-secondary build label'
$artifactDirectory=if($ArtifactRole -ceq 'control'){Join-Path $PSScriptRoot 'artifacts/control'}else{Join-Path $PSScriptRoot 'artifacts/revised-candidate'}
$apkPath=[IO.Path]::GetFullPath([string]$artifactSpec.apkPath)
Equal (Get-FileHash -LiteralPath $apkPath -Algorithm SHA256).Hash.ToLowerInvariant() $expectedApk 'sealed APK bytes'
$packageProof=Get-Content (Join-Path $PSScriptRoot 'logs/apk-assets-module-comparison-02.json') -Raw | ConvertFrom-Json
Equal $packageProof.assetSummary.baselineCount 53 'control asset count'
Equal $packageProof.assetSummary.candidateCount 53 'isolate asset count'
Equal $packageProof.assetSummary.identicalCount 53 'byte-identical asset count'
Equal $packageProof.assetSummary.changedCount 0 'changed asset count'
Equal $packageProof.packagedShippingModules.Count 2 'exact control/isolate package pair'
$proofLabel=if($ArtifactRole -ceq 'control'){'baseline'}else{'candidate'}
$proofSide=@($packageProof.packagedShippingModules|Where-Object label -CEQ $proofLabel)
Equal $proofSide.Count 1 'exact packaged module proof side'
Equal $proofSide[0].apkSha256 $expectedApk 'asset/module proof APK identity'
$expectedNative=if($ArtifactRole -ceq 'control'){'41b34e601fa486e5cb5eeea24dd31df9771badb485b7cb74530344637644b049'}else{'75c7410f7ef30cc0e9f2ccb231c7792cbf2947907006a37f8b7bc2ae1f3d51f3'}
Equal $proofSide[0].librarySha256 $expectedNative 'actual APK native library identity'
Equal $proofSide[0].modules.Count 4 'exact actual package module count'
Equal @($proofSide[0].modules|Where-Object { $_.atomicInstructions -ne 0 }).Count 0 'no package module atomics'
Equal @($proofSide[0].modules|Where-Object { $_.binding22 }).Count 0 'no package module diagnostic binding 22'
Equal @($proofSide[0].modules|Where-Object { $_.opImageReadCount -ne 0 }).Count 0 'no package image read instructions'
$candidateDetail=Get-Content (Join-Path $PSScriptRoot 'artifacts/revised-candidate/build-receipt.json') -Raw|ConvertFrom-Json
Equal $candidateDetail.build.apkSha256 'ce1c1548ed3937d1ab99b57c523b62a8a1e244538c01e9075bb2f7e56e766cce' 'revised isolate detailed build receipt'
Equal $candidateDetail.source.head '71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95' 'revised isolate source head'
Equal $candidateDetail.build.nativeArm64Sha256 '75c7410f7ef30cc0e9f2ccb231c7792cbf2947907006a37f8b7bc2ae1f3d51f3' 'revised isolate native library pin'
$candidateContainment=Get-Content (Join-Path $PSScriptRoot 'logs/candidate-package-containment-02.json') -Raw|ConvertFrom-Json
Equal $candidateContainment.packaged.targetSha256 '75c7410f7ef30cc0e9f2ccb231c7792cbf2947907006a37f8b7bc2ae1f3d51f3' 'candidate containment native identity'
Equal $candidateContainment.packaged.modules.Count 4 'candidate containment module count'
if($ArtifactRole -ceq 'control'){
    $controlDetail=Get-Content 'C:\Dev\tmp\horde-primary-hit-profile-20261001\artifacts\control\all-shadow-build-receipt.json' -Raw|ConvertFrom-Json
    Equal $controlDetail.control.apkSha256 $expectedApk 'normal control build receipt'
    Equal $controlDetail.control.nativeLibrarySha256 $expectedNative 'normal control native pin'
}
$raygenCatalogPath=Join-Path $artifactDirectory 'raygen-variant-catalog.json'
$computeCatalogPath=Join-Path $artifactDirectory 'rayquery-variant-catalog.json'
$expectedCatalogHashes=if($ArtifactRole -ceq 'control'){
    @{raygen='4330a0711733a3c73b49eef350c4ed7a1672abe4bc4510daa1f767ef5c529bd1';compute='20b98150b58653b471415ef78e9f95c2065ef2e003e97754be8785a84d9353d6'}
}else{
    @{raygen='beb135d9bec31aafea824f941432f8c1a21c5447e526297504028bf45de6e585';compute='24ba01cb34fde9c4f04669f795c3b70b2e01e1ada769a7aeb94dbf88fbba46ee4'}
}
Equal (Get-FileHash -LiteralPath $raygenCatalogPath -Algorithm SHA256).Hash.ToLowerInvariant() $expectedCatalogHashes.raygen 'frozen raygen catalog file hash'
Equal (Get-FileHash -LiteralPath $computeCatalogPath -Algorithm SHA256).Hash.ToLowerInvariant() $expectedCatalogHashes.compute 'frozen compute catalog file hash'
$raygenSnapshot=Get-Content -LiteralPath $raygenCatalogPath -Raw|ConvertFrom-Json
$computeSnapshot=Get-Content -LiteralPath $computeCatalogPath -Raw|ConvertFrom-Json
$modulePins=[ordered]@{}
foreach($entry in @(
    @{catalog=$raygenSnapshot;key='shipping_mobile_opaque_fast';backend='RayTracingPipeline'},
    @{catalog=$raygenSnapshot;key='shipping_mobile_generic_dielectric';backend='RayTracingPipeline'},
    @{catalog=$computeSnapshot;key='rayquery_compute_shipping_mobile_opaque_fast';backend='RayQueryCompute'},
    @{catalog=$computeSnapshot;key='rayquery_compute_shipping_mobile_generic_dielectric';backend='RayQueryCompute'}
)){
    $variant=@($entry.catalog.variants|Where-Object key -CEQ $entry.key)
    Equal $variant.Count 1 "frozen catalog key $($entry.key)"
    Equal $entry.catalog.status 'frozen' "frozen catalog status $($entry.key)"
    Equal $variant[0].instrumentation 'Shipping' "frozen catalog instrumentation $($entry.key)"
    Equal $variant[0].quality 'Mobile' "frozen catalog quality $($entry.key)"
    if($entry.backend -ceq 'RayQueryCompute'){
        Equal $variant[0].executionBackend $entry.backend "frozen catalog backend $($entry.key)"
        Equal $entry.catalog.target.stage 'comp' "frozen catalog stage $($entry.key)"
    }else{
        Equal $entry.catalog.target.stage 'rgen' "frozen catalog stage $($entry.key)"
    }
    $modulePins[$entry.key]=[string]$variant[0].spirvSha256
}
foreach($moduleKey in $modulePins.Keys){
    $proofModule=@($proofSide[0].modules|Where-Object semanticKey -CEQ $moduleKey)
    Equal $proofModule.Count 1 "packaged module $moduleKey"
    Equal $proofModule[0].sha256 $modulePins[$moduleKey] "pinned packaged module $moduleKey"
    Equal $proofModule[0].atomicInstructions 0 "atomic count $moduleKey"
    Equal $proofModule[0].opImageReadCount 0 "image read count $moduleKey"
    Equal $proofModule[0].binding22 $false "binding 22 $moduleKey"
    Equal $proofModule[0].spirvVal 'passed' "spirv-val $moduleKey"
    Equal $proofModule[0].spirvDis 'passed' "spirv-dis $moduleKey"
}
$modules=$proofSide[0].modules
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
$completedZoneCpuSummaries=@()
foreach($legacyZone in $report.zones){
    $zoneName=[string]$legacyZone.name
    $completedZone=@($evidence.zones | Where-Object name -CEQ $zoneName)
    if($legacyZone.frames -eq 0){
        if($completedZone.Count -eq 0){$completedZoneCpuSummaries+=,[ordered]@{zone=$zoneName;frames=0;cpuStages=$null};continue}
    }
    Equal $completedZone.Count 1 "completion-owned zone $zoneName"
    $zoneEvidence=$completedZone[0]
    Equal $zoneEvidence.counts.intended $legacyZone.frames "zone $zoneName intended rows"
    Equal $zoneEvidence.counts.completed $legacyZone.frames "zone $zoneName completed rows"
    Equal $zoneEvidence.counts.cpuAccepted $legacyZone.frames "zone $zoneName CPU accepted rows"
    foreach($name in @('rejected','cancelled','cpuRejected','outstanding')){Equal $zoneEvidence.counts.$name 0 "zone $zoneName $name"}
    Equal $zoneEvidence.gpuStatusCounts.denominator $legacyZone.frames "zone $zoneName GPU denominator"
    Equal $zoneEvidence.gpuStatusCounts.valid $legacyZone.frames "zone $zoneName valid GPU count"
    foreach($property in $zoneEvidence.gpuStatusCounts.PSObject.Properties){if($property.Name -notin @('valid','denominator')){Equal $property.Value 0 "zone $zoneName GPU status $($property.Name)"}}
    foreach($stage in $zoneEvidence.cpuStages.PSObject.Properties){
        Equal $stage.Value.sampleCount $legacyZone.frames "zone $zoneName CPU stage sample count $($stage.Name)"
        foreach($stat in $stage.Value.PSObject.Properties){if($stat.Name -ne 'sampleCount' -and [double]$stat.Value -lt 0){throw "Negative completion-owned zone CPU statistic $zoneName.$($stage.Name).$($stat.Name)"}}
    }
    $completedZoneCpuSummaries+=,[ordered]@{zone=$zoneName;frames=$legacyZone.frames;cpuStages=$zoneEvidence.cpuStages}
}
foreach($zoneEvidence in $evidence.zones){
    $legacy=@($report.zones|Where-Object name -CEQ $zoneEvidence.name)
    Equal $legacy.Count 1 "completion-owned zone has legacy zone $($zoneEvidence.name)"
    if($legacy[0].frames -eq 0){throw "Unexpected completion-owned rows in empty zone $($zoneEvidence.name)"}
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
    $openingZone=@($completedZoneCpuSummaries | Where-Object zone -CEQ 'opening')
    Equal $openingZone.Count 1 'opening zone CPU stage summary'
    $routeStageSplit=[ordered]@{
        opening=[ordered]@{frames=$openingGpu[0].frames;gpuCommandBuffer=$openingGpu[0];cpuStages=$openingZone[0].cpuStages}
        heavyZones=@($zoneGpu | Where-Object zone -CNE 'opening')
        heavyZoneCpuStages=@($completedZoneCpuSummaries | Where-Object zone -CNE 'opening')
        gpuStageBoundary='The current benchmark schema emits a completion-owned GPU command-buffer interval per frame/zone, not separate GPU AS-build and RT-dispatch durations. CPU BlasRefitRecord/TlasUpdateRecord/TraceCopyRecord are CPU record-stage timings, not GPU-stage timings.'
    }
}
if($report.workload -ceq 'lantern-reveal-sequence-v1'){
    $liveGpu=@($zoneGpu | Where-Object zone -CEQ 'yellow-torch-bay')
    Equal $liveGpu.Count 1 'live reveal zone GPU timing'
    $liveZone=@($completedZoneCpuSummaries | Where-Object zone -CEQ 'yellow-torch-bay')
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
$sourceHashes=[ordered]@{
    trialJson=(Get-FileHash -LiteralPath $trialPath -Algorithm SHA256).Hash.ToLowerInvariant()
    resultJson=(Get-FileHash -LiteralPath $markerPath -Algorithm SHA256).Hash.ToLowerInvariant()
    benchmarkJson=(Get-FileHash -LiteralPath $reportPath -Algorithm SHA256).Hash.ToLowerInvariant()
    contextSamplesJsonl=(Get-FileHash -LiteralPath $contextPath -Algorithm SHA256).Hash.ToLowerInvariant()
}
$summary=[ordered]@{
    runId=$RunId;integrity='PASS';artifactRole=$ArtifactRole;runDirectory=$directory;sourceCommit=$trial.sourceCommit;apkSha256=$trial.installedApkSha256;buildLabel=$trial.buildLabel
    sourceHashes=$sourceHashes
    workload=$report.workload;deviceModel=$trial.deviceModel;shader=$report.shader;frames=$expected;presentMode=$report.presentMode
    internalExtent=$report.internalExtent;overall=$report.overall;medianDerivedFps=[math]::Round(1000/$report.overall.medianMs,3)
    gpuRtDurationMs=$evidence.gpuRtDurationMs;cpuStages=$evidence.cpuStages;zones=$report.zones
    owningActiveStrategy=[ordered]@{exported=$strategyExported;counts=$strategyCounts;openingCounts=$openingStrategyCounts}
    zoneGpuCommandBufferTiming=$zoneGpu
    openingVsHeavyRouteStages=$routeStageSplit
    liveRevealZoneStages=$liveSequenceStages
    completionOwnedZoneCpuStages=$completedZoneCpuSummaries
    diagnosticRowsCompiledOut=$expected;firstMeasuredSimulationTick=$evidence.rows[0].submittedIdentity.simulationTick;lastMeasuredSimulationTick=$evidence.rows[-1].submittedIdentity.simulationTick
    context=[ordered]@{samples=$context.Count;firstUtc=$context[0].utc;lastUtc=$context[-1].utc;batteryStartC=$context[0].batteryC;batteryEndC=$context[-1].batteryC;thermalStatuses=@($context.thermalStatus | Sort-Object -Unique);gpuThermalPowerLevels=@($context.gpuThermalPowerLevel | Sort-Object -Unique);processIds=@($context.processId | Sort-Object -Unique)}
    classification='nonphysical-opaque-primary-secondary-omission-cost-bound-investigation-only';optimizationClaim='none';causalAcceptance='not-evaluated';qualityAcceptance='not-evaluated';productionPromotion='not-authorized'
    limitations=@('Thermal context starts after am-start observation and is not frame-aligned measured-lap telemetry.','Render-entry-through-present timing is not display pacing or input latency.','GPU command-buffer timestamps include AS work/RT/copy, not isolated shader timing.','The investigation removes opaque-primary secondary bounce/reflected-fire contributions; the isolate is deliberately nonphysical and not a candidate.','The whole-GPU interval includes all work in the frame; it is not additive secondary-ray cost or an isolated shader measurement.','CPU BLAS/TLAS record timings do not establish GPU AS cost; current GPU schema does not separate AS from RT dispatch.','This parser validates individual runs only; it does not calculate an ABBA effect or accept a performance or quality claim.')
}
$summary | ConvertTo-Json -Depth 9 > $analysisPath
Write-Output "$RunId integrity PASS: $expected rows; median $($report.overall.medianMs) ms ($($summary.medianDerivedFps) FPS), p95 $($report.overall.p95Ms) ms; GPU median $gpuMedian ms."
