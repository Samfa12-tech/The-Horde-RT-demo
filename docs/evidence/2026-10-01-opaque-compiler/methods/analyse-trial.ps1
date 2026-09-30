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
    'control' {@{artifactKind='control';expectedStrategy='opaque-fast'}}
    'opaque-retained-profile' {@{artifactKind='opaque-retained-profile';expectedStrategy='opaque-fast'}}
    default {throw 'Unknown exact artifact label'}
}
$compilerReceiptPath=Join-Path $PSScriptRoot 'opaque-retained-profile.json'
$compilerReceipt=Get-Content -LiteralPath $compilerReceiptPath -Raw | ConvertFrom-Json
Equal (Get-FileHash -LiteralPath $compilerReceiptPath -Algorithm SHA256).Hash.ToLowerInvariant() '23f17fc83520a515bf2bdf4642527910f292e4115ea66ef8eba80167aac91760' 'compiler-treatment receipt identity'
Equal $compilerReceipt.source.head 'eafbf8262442a82e0835edf5cf5306d718e64633' 'compiler source provenance'
Equal $compilerReceipt.source.preprocessedSource.sha256 'eaa0b8a9f7b1a87fce83e34ab8568c524b1a8ed4160009bf35fc1a89e728713f' 'exact OpaqueFast preprocessed source'
Equal $compilerReceipt.source.samePreprocessedFileUsedForBothCompiles $true 'compiler input identity'
Equal $compilerReceipt.controlBinding.packagedRaygenModuleSha256 '66e39df9f53b058fb62cbfa913d424b161c93be4aff59a1685cf8b7e54bb9c4b' 'ordinary packaged control pipeline module'
Equal $compilerReceipt.moduleComparison.retainedPassHypothesis.sha256 '14509c272fa4fa9f92a8d180abf170b2485447798b7940ec98f55150c05249d3' 'retained candidate compiler module'
Equal $compilerReceipt.moduleComparison.retainedPassHypothesis.atomicInstructions 0 'Shipping compiler candidate atomics'
Equal $compilerReceipt.moduleComparison.retainedPassHypothesis.binding22 $false 'Shipping compiler candidate binding 22'
$moduleReceiptPath=Join-Path $PSScriptRoot 'independent-scan-v3/independent-apk-module-receipt.json'
Equal (Get-FileHash -LiteralPath $moduleReceiptPath -Algorithm SHA256).Hash.ToLowerInvariant() '6b2d154668ee1d56a09a9ac8d6c557eaf3a0fbc4353a3c7a7965f5f96d013f7b' 'independent APK module receipt hash'
$moduleReceipt=Get-Content -LiteralPath $moduleReceiptPath -Raw | ConvertFrom-Json
Equal $moduleReceipt.sourceHead 'eafbf8262442a82e0835edf5cf5306d718e64633' 'independent module source base'
Equal $moduleReceipt.androidArtifacts.Count 2 'independent APK receipt count'
Equal $moduleReceipt.assetComparison[0].controlAssetCount 53 'control asset count'
Equal $moduleReceipt.assetComparison[0].candidateAssetCount 53 'candidate asset count'
Equal $moduleReceipt.assetComparison[0].byteIdenticalAssetCount 50 'candidate byte-identical asset count'
Equal $moduleReceipt.assetComparison[0].changedEntryCount 3 'candidate text-only asset differences'
$pixelGatePath=Join-Path $PSScriptRoot 'opening-image-comparison.json'
Equal (Get-FileHash -LiteralPath $pixelGatePath -Algorithm SHA256).Hash.ToLowerInvariant() 'e04b92c263256fc0404534e99f0975c455bbbb0a8f2455293028a1549a2f31d6' 'opening pixel-gate evidence hash'
$pixelGate=Get-Content -LiteralPath $pixelGatePath -Raw | ConvertFrom-Json
Equal $pixelGate.pixelSubsetPassed $false 'opening pixel subset gate'
Equal $pixelGate.maximumChannelDifference 14 'opening maximum channel difference'
Equal $pixelGate.pixelsDifferentByMoreThanOne 125 'opening pixels differing over one'
if($artifactSpec.artifactKind -ceq 'control'){
    $artifactPath=$compilerReceipt.controlBinding.controlApk
    $artifactSha=$compilerReceipt.controlBinding.controlApkSha256
    $containmentPath='C:\Dev\tmp\horde-generic-route-profile-20260930\profile\provenance\containment-control-benchmark-shipping-mobile-eaf-scanner.json'
    Equal (Get-FileHash -LiteralPath $containmentPath -Algorithm SHA256).Hash.ToLowerInvariant() '5bde1466796ef6020962cd9486235dd86a35d1e7802ce792b0f661f0f3f05f87' 'standard control containment receipt hash'
    $containment=Get-Content -LiteralPath $containmentPath -Raw | ConvertFrom-Json
    Equal $containment.apk $artifactPath 'control containment APK path'
    Equal $containment.instrumentation 'Shipping' 'control packaged instrumentation'
    Equal $containment.quality 'Mobile' 'control packaged quality'
    Equal $containment.packaged.targetSha256 $containment.stripped.targetSha256 'control stripped versus packaged native library'
    $modules=$containment.packaged.modules
} else {
    $artifactRows=@($moduleReceipt.androidArtifacts | Where-Object kind -CEQ 'benchmark')
    Equal $artifactRows.Count 1 'candidate benchmark APK receipt count'
    Equal $artifactRows[0].sha256 '4827a3c2e26d3328ed53608e15c77afe12a34077e96dcd0564c8d65532e21ff4' 'immutable candidate APK SHA-256'
    $artifactPath=$artifactRows[0].path
    $artifactSha=$artifactRows[0].sha256
    $modules=$artifactRows[0].modules
}
Equal (Get-FileHash -LiteralPath $artifactPath -Algorithm SHA256).Hash.ToLowerInvariant() $artifactSha 'immutable APK file versus build receipt'
Equal $trial.installedApkSha256 $artifactSha 'installed APK versus immutable build receipt'
$pipeline=@($modules | Where-Object backend -CEQ 'RayTracingPipeline')
Equal $pipeline.Count 2 'actual APK pipeline module count'
$modulesByVariant=[ordered]@{}
foreach($variantKey in @('shipping_mobile_opaque_fast','shipping_mobile_generic_dielectric')){
    $expectedHash=if($variantKey -ceq 'shipping_mobile_opaque_fast') { if($artifactSpec.artifactKind -ceq 'control'){'66e39df9f53b058fb62cbfa913d424b161c93be4aff59a1685cf8b7e54bb9c4b'} else {'14509c272fa4fa9f92a8d180abf170b2485447798b7940ec98f55150c05249d3'} } else {'ce2302811cb2cb8bbff706fd54cd7f48705e4a5e8b2cda744a7d999574f84532'}
    if($artifactSpec.artifactKind -ceq 'opaque-retained-profile'){
        $matchingModule=@($pipeline | Where-Object { $_.catalogKey -CEQ $variantKey -and $_.sha256 -CEQ $expectedHash })
        Equal $matchingModule.Count 1 "actual candidate module for $variantKey"
        if($variantKey -ceq 'shipping_mobile_opaque_fast'){ Equal $matchingModule[0].strategy 'OpaqueRetainedInvestigation' 'experimental catalog-only strategy label' }
        else { Equal $matchingModule[0].strategy 'GenericRetained' 'unchanged Generic pipeline strategy' }
    } else {
        $matchingModule=@($pipeline | Where-Object sha256 -CEQ $expectedHash)
    }
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
if($trial.buildLabel -cin @('control','opaque-retained-profile')){
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
    artifactReceipts=[ordered]@{compilerTreatment='opaque-retained-profile.json';candidateActualModuleScan='independent-scan-v3/independent-apk-module-receipt.json';controlStandardContainment='C:\Dev\tmp\horde-generic-route-profile-20260930\profile\provenance\containment-control-benchmark-shipping-mobile-eaf-scanner.json'}
    compilerTreatment=[ordered]@{classification='compiler-treatment';samePreprocessedSourceSha256='eaa0b8a9f7b1a87fce83e34ab8568c524b1a8ed4160009bf35fc1a89e728713f';ordinaryControlModuleSha256='66e39df9f53b058fb62cbfa913d424b161c93be4aff59a1685cf8b7e54bb9c4b';retainedModuleSha256='14509c272fa4fa9f92a8d180abf170b2485447798b7940ec98f55150c05249d3';runtimeStrategy='opaque-fast';optimizationAdmission='none';gainClaim='none';physicalAcceptance='not-evaluated';separateOpeningPixelGate=[ordered]@{scope='Diagnostic/Mobile opening capture comparison; not the Shipping route timing APK pair';pixelSubsetPassed=$false;maximumChannelDifference=14;pixelsDifferentByMoreThanOne=125;comparisonPath='opening-image-comparison.json'}}
    owningActiveStrategy=[ordered]@{exported=$strategyExported;counts=$strategyCounts;openingCounts=$openingStrategyCounts}
    zoneGpuCommandBufferTiming=$zoneGpu
    diagnosticRowsCompiledOut=$expected;firstMeasuredSimulationTick=$evidence.rows[0].submittedIdentity.simulationTick;lastMeasuredSimulationTick=$evidence.rows[-1].submittedIdentity.simulationTick
    context=[ordered]@{samples=$context.Count;firstUtc=$context[0].utc;lastUtc=$context[-1].utc;batteryStartC=$context[0].batteryC;batteryEndC=$context[-1].batteryC;thermalStatuses=@($context.thermalStatus | Sort-Object -Unique);gpuThermalPowerLevels=@($context.gpuThermalPowerLevel | Sort-Object -Unique)}
    limitations=@('Thermal context starts after am-start observation and is not frame-aligned measured-lap telemetry.','Render-entry-through-present timing is not display pacing or input latency.','GPU command-buffer timestamps include AS work/RT/copy, not isolated shader timing.','Shipping compiled-out counters do not prove glass correctness.','No causal A/B acceptance from this individual trial.')
}
if($artifactSpec.artifactKind -ceq 'opaque-retained-profile'){
    $summary['physicalEvidenceClass']='investigation-only-compiler-treatment'
    $summary['gainClaim']='none'
    $summary['physicalAcceptance']='not-evaluated'
    $summary['optimizationAcceptance']='none'
    $summary.limitations+=@('This is a compiler-treatment group: glslang flags and SPIR-V optimization pass treatment vary together, while both compile the exact same OpaqueFast preprocessed source. It does not isolate any single compiler flag/pass.','The separate Diagnostic/Mobile opening pixel gate failed: maximum channel difference 14 and 125 pixels differ by more than one. It is not physical/runtime or visual acceptance for this Shipping route trial.','Standard package containment remains RED on compute/raygen policy parity; the independent module scan does not replace that guard.','No optimization admission, gain claim, or physical acceptance is made.')
} else {
    $summary['physicalEvidenceClass']='exact-control-reference'
    $summary['gainClaim']='none'
    $summary['physicalAcceptance']='not-evaluated'
}
$summaryPath=Join-Path $directory 'analysis.json'
if(Test-Path -LiteralPath $summaryPath){throw 'Refuse to overwrite existing trial analysis.'}
$summary | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $summaryPath -Encoding utf8NoBOM
Write-Output "$RunId integrity PASS: $expected rows; median $($report.overall.medianMs) ms ($($summary.medianDerivedFps) FPS), p95 $($report.overall.p95Ms) ms; GPU median $gpuMedian ms."
