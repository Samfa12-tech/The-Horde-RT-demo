[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Root,[string]$VulkanBin='C:\VulkanSDK\1.4.350.0\Bin')
$ErrorActionPreference='Stop'
$rootPath=(Resolve-Path -LiteralPath $Root -ErrorAction Stop).Path
function Eq($Actual,$Expected,[string]$Label){if($Actual -cne $Expected){throw "$Label expected '$Expected', found '$Actual'"}}
function Read-Json([string]$Rel){Get-Content -LiteralPath (Join-Path $rootPath $Rel) -Raw|ConvertFrom-Json -DateKind String}
function Hash([string]$Path){(Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()}
function Assert-FiniteNonnegative($Value,[string]$Label){
    if($null -eq $Value){throw "Missing numeric value: $Label"}
    $n=0.0
    if(-not [double]::TryParse([string]$Value,[ref]$n) -or [double]::IsNaN($n) -or [double]::IsInfinity($n) -or $n -lt 0){throw "Nonfinite/negative numeric value: $Label"}
}
function Assert-FiniteRange($Value,[double]$Minimum,[double]$Maximum,[string]$Label){
    if($null -eq $Value){throw "Missing numeric value: $Label"}
    $n=0.0
    if(-not [double]::TryParse([string]$Value,[ref]$n) -or [double]::IsNaN($n) -or [double]::IsInfinity($n) -or $n -lt $Minimum -or $n -gt $Maximum){throw "Invalid finite numeric range: $Label"}
}
function Assert-CpuStages($Stages,[string]$Label,[int]$Rows){
    $keys=@($Stages.PSObject.Properties.Name|Sort-Object)
    $expectedKeys=@($script:cpuKeys|Sort-Object)
    if(($keys -join "`n") -cne ($expectedKeys -join "`n")){throw "CPU-stage key mismatch: $Label"}
    foreach($key in $script:cpuKeys){
        $stage=$Stages.$key
        $statKeys=@($stage.PSObject.Properties.Name|Sort-Object)
        $expected=@('sampleCount','meanMilliseconds','medianMilliseconds','p90Milliseconds','p95Milliseconds','slowestOnePercentMeanMilliseconds')
        if($key -ceq 'wholeFrameCycleCpuMs'){$expected+= 'onePercentLowFps'}
        if(($statKeys -join "`n") -cne (@($expected|Sort-Object) -join "`n")){throw "CPU statistic key mismatch: $Label.$key"}
        Eq $stage.sampleCount $Rows "$Label.$key sample count"
        foreach($stat in $statKeys){if($stat -ne 'sampleCount'){Assert-FiniteNonnegative $stage.$stat "$Label.$key.$stat"}}
    }
}
$manifest=Read-Json 'manifest.json'
Eq $manifest.schema 1 'manifest schema'
Eq $manifest.classification 'nonphysical-opaque-primary-secondary-omission-cost-bound-investigation-only' 'archive classification'
$listed=@($manifest.files|ForEach-Object {[string]$_.path})
if(@($listed|Sort-Object -Unique).Count -ne $listed.Count){throw 'Manifest contains duplicate file paths'}
$actualFiles=@(Get-ChildItem -LiteralPath $rootPath -Recurse -File|ForEach-Object {$_.FullName.Substring($rootPath.Length+1).Replace('\','/')})
$expectedFiles=@($listed+'manifest.json'|Sort-Object -Unique)
if(($actualFiles|Sort-Object) -join "`n" -cne ($expectedFiles -join "`n")){throw 'Curated file set differs from SHA manifest'}
foreach($entry in $manifest.files){
    $path=Join-Path $rootPath ([string]$entry.path.Replace('/','\'))
    if(-not(Test-Path -LiteralPath $path -PathType Leaf)){throw "Missing manifest file: $($entry.path)"}
    Eq (Get-Item -LiteralPath $path).Length $entry.bytes "file byte length $($entry.path)"
    Eq (Hash $path) $entry.sha256 "file SHA $($entry.path)"
}
$build=Read-Json 'build-receipt.json'
$source='71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95'
$apks=[ordered]@{control='a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033';isolate='ce1c1548ed3937d1ab99b57c523b62a8a1e244538c01e9075bb2f7e56e766cce'}
Eq $build.control.apkSha256 $apks.control 'control APK receipt pin'
Eq $build.isolate.apkSha256 $apks.isolate 'isolate APK receipt pin'
Eq $build.classification 'nonphysical-opaque-primary-secondary-omission-investigation-only' 'build classification'
$pins=Read-Json 'module-pins.json'
Eq $pins.control.apkSha256 $apks.control 'control module receipt APK pin'
Eq $pins.isolate.apkSha256 $apks.isolate 'isolate module receipt APK pin'
$expectedKeys=@('shipping_mobile_opaque_fast','shipping_mobile_generic_dielectric','rayquery_compute_shipping_mobile_opaque_fast','rayquery_compute_shipping_mobile_generic_dielectric')
if(@($pins.artifacts).Count -ne 2){throw 'Expected exactly control and isolate module artifacts'}
$control=@($pins.artifacts|Where-Object role -CEQ 'control');$isolate=@($pins.artifacts|Where-Object role -CEQ 'isolate')
Eq $control.Count 1 'control artifact count';Eq $isolate.Count 1 'isolate artifact count'
foreach($artifact in @($control[0],$isolate[0])){
    if(@($artifact.modules).Count -ne 4){throw "$($artifact.role) must contain four modules"}
    $keys=@($artifact.modules|ForEach-Object semanticKey)
    if(@($keys|Sort-Object -Unique).Count -ne 4 -or ($keys|Sort-Object) -join ',' -cne (@($expectedKeys|Sort-Object) -join ',')){throw "$($artifact.role) semantic key set mismatch"}
    $catalogDirectory=if($artifact.role -ceq 'control'){'artifacts/control'}else{'artifacts/revised-candidate'}
    $raygen=Read-Json "$catalogDirectory/raygen-variant-catalog.json"
    $compute=Read-Json "$catalogDirectory/rayquery-variant-catalog.json"
    foreach($module in $artifact.modules){
        $catalog=if($module.backend -ceq 'RayTracingPipeline'){$raygen}else{$compute}
        $variant=@($catalog.variants|Where-Object key -CEQ $module.semanticKey)
        Eq $variant.Count 1 "$($artifact.role) catalog key $($module.semanticKey)"
        Eq $variant[0].spirvSha256 $module.sha256 "$($artifact.role) catalog/SPIR-V pin $($module.semanticKey)"
        $spv=Join-Path $rootPath ([string]$module.spvPath.Replace('/','\'))
        Eq (Hash $spv) $module.sha256 "$($artifact.role) actual SPIR-V bytes $($module.semanticKey)"
        Eq ((Get-Item -LiteralPath $spv).Length/4) $module.words "$($artifact.role) SPIR-V word count $($module.semanticKey)"
        Eq $module.spirvVal 'PASS' "$($artifact.role) spirv-val proof $($module.semanticKey)"
        Eq $module.spirvDis 'PASS' "$($artifact.role) spirv-dis proof $($module.semanticKey)"
        Eq $module.atomicInstructions 0 "$($artifact.role) atomics $($module.semanticKey)"
        Eq $module.opImageReadCount 0 "$($artifact.role) OpImageRead $($module.semanticKey)"
        Eq $module.binding22 $false "$($artifact.role) binding 22 $($module.semanticKey)"
        foreach($logKey in @('valLog','disLog')){
            $log=Join-Path $rootPath ([string]$module.$logKey.Replace('/','\'))
            $text=[IO.File]::ReadAllText($log)
            if($text -notmatch '(?m)^exitCode=0\r?$' -or $text -notmatch '(?m)^result=passed(?:;[^\r\n]*)?\r?$'){throw "Stored SPIR-V tool proof failed: $($module.$logKey)"}
        }
    }
}
$asset=Read-Json 'receipts/assets-and-package-module-comparison.json'
Eq $asset.assetSummary.baselineCount 53 'asset control count';Eq $asset.assetSummary.candidateCount 53 'asset isolate count'
Eq $asset.assetSummary.identicalCount 53 'byte-identical asset count';Eq $asset.assetSummary.changedCount 0 'asset changes'
$cpuKeys=@('simulationStepCpuMs','skinCpuMs','playerSkinCpuMs','characterSkinCpuMs','dynamicUploadCpuMs','blasRefitRecordCpuMs','tlasUpdateRecordCpuMs','traceCopyRecordCpuMs','frameFenceWaitCpuMs','imageAcquireCpuMs','queueSubmitCpuMs','presentCallCpuMs','wholeFrameCycleCpuMs')
$expectedRuns=@()
foreach($slot in @('c1','s1','s2','c2')){foreach($work in @('route','live')){$expectedRuns+=,"opaque-secondary-$slot-$work-20261001"}}
foreach($runId in $expectedRuns){
    $role=if($runId -match '-(c1|c2)-'){'control'}else{'isolate'}
    $workload=if($runId -match '-route-'){'showcase-route-v1'}else{'lantern-reveal-sequence-v1'}
    $base="phone/$runId";$trial=Read-Json "$base/trial.json";$report=Read-Json "$base/$runId/benchmark.json";$result=Read-Json "$base/$runId/result.json"
    Eq $trial.runId $runId 'trial run ID';Eq $trial.status 'complete' 'trial completion status';Eq $result.status 'complete' 'result status';Eq $report.status 'complete' 'report status';Eq $report.workload $workload 'workload'
    Eq $trial.sourceCommit $source 'source commit';Eq $trial.deviceModel 'SM-S948B' 'device model';Eq $trial.serial 'R5GL219SZGK' 'phone serial';Eq $trial.package 'com.samfa12.hordelanternrt.benchmark' 'installed package'
    Eq $trial.instrumentation 'Shipping' 'instrumentation';Eq $trial.quality 'Mobile' 'quality';Eq $trial.requestedScalePercent 75 'requested scale';Eq $trial.requestedBackend 'RayTracingPipeline' 'requested backend'
    Eq $trial.installedApkSha256 $apks[$role] 'installed APK receipt pin'
    $frames=if($workload -ceq 'showcase-route-v1'){1838}else{600};Eq $report.measuredFrames $frames 'measured frame count';Eq $report.completedFrameEvidence.rows.Count $frames 'owning row count'
    Eq $report.renderScalePercent 75 'report render scale';Eq $report.executionBackend 'RayTracingPipeline' 'report backend';Eq $report.rtMode 'RayTracingPipeline' 'report RT mode';Eq $report.presentMode 'MAILBOX' 'report present mode'
    Eq $report.materialEncoding 'ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2) + strict ASTC 6x6 lich' 'report material encoding'
    Eq $report.internalExtent.width 1080 'report internal width';Eq $report.internalExtent.height 2235 'report internal height';Eq $report.presentationExtent.width 1440 'report presentation width';Eq $report.presentationExtent.height 2980 'report presentation height'
    Eq $report.completedFrameEvidence.counts.completed $frames 'completed denominator';Eq $report.completedFrameEvidence.gpuStatusCounts.valid $frames 'GPU valid denominator';Eq $report.completedFrameEvidence.gpuStatusCounts.denominator $frames 'GPU timing denominator'
    Eq $report.completedFrameEvidence.counts.rejected 0 'rejected rows';Eq $report.completedFrameEvidence.counts.cancelled 0 'cancelled rows';Eq $report.completedFrameEvidence.counts.cpuRejected 0 'CPU rejected rows';Eq $report.completedFrameEvidence.counts.outstanding 0 'outstanding rows'
    Assert-CpuStages $report.completedFrameEvidence.cpuStages 'whole run CPU stage set' $frames
    $zones=if($workload -ceq 'showcase-route-v1'){@{opening=160;'skeleton-room'=98;'shadow-corridor'=599;'skylight-chamber'=189;'yellow-torch-bay'=157;'blue-torch-bay'=157;'red-torch-bay'=157;'green-torch-bay'=157;'transmission-threshold'=63;finale=101}}else{@{opening=0;'skeleton-room'=0;'shadow-corridor'=0;'skylight-chamber'=0;'yellow-torch-bay'=600;'blue-torch-bay'=0;'red-torch-bay'=0;'green-torch-bay'=0;'transmission-threshold'=0;finale=0}}
    foreach($name in $zones.Keys){$legacy=@($report.zones|Where-Object name -CEQ $name);Eq $legacy.Count 1 "legacy zone $name";Eq $legacy[0].frames $zones[$name] "legacy zone frame count $name";Eq @($report.completedFrameEvidence.rows|Where-Object zoneName -CEQ $name).Count $zones[$name] "owning zone row count $name"}
    foreach($zone in $report.completedFrameEvidence.zones){$n=[int]$zone.counts.completed;Eq $n $zones[$zone.name] "completion zone row count $($zone.name)";if($n -gt 0){Assert-CpuStages $zone.cpuStages "completion zone $($zone.name)" $n}}
    $seen=0L
    foreach($i in 0..($frames-1)){$row=$report.completedFrameEvidence.rows[$i];Eq $row.index $i 'row index';Eq $row.cpuSampleIndex $i 'CPU row join';Eq $row.disposition 'completed' 'frame disposition';Eq $row.failure 'none' 'frame failure';Eq $row.presentationOutcome 'presented' 'presentation';Eq $row.cpuStageStatus 'valid' 'CPU stage state';Eq $row.gpuStatus 'valid' 'GPU state';Eq $row.diagnosticStatus 'compiled-out' 'diagnostic state';Eq $row.diagnosticCounters $null 'diagnostic payload';if($row.activeStrategy -cnotin @('opaque-fast','generic-dielectric')){throw "Invalid active strategy at $runId row $i"};Assert-FiniteNonnegative $row.gpuDurationNanoseconds "GPU row duration $i";foreach($key in @('sceneEpoch','measurementGeneration','recordAttemptSerial','recordSerial','simulationTick','frameSlot','submissionSerial')){Eq $row.submittedIdentity.$key $row.completionIdentity.$key "joined $key row $i"};$serial=[long]$row.completionIdentity.completionSerial;if($serial -le $seen){throw "Completion serial not monotonic in $runId"};$seen=$serial}
    if($workload -ceq 'showcase-route-v1'){$opening=@($report.completedFrameEvidence.rows|Where-Object zoneName -CEQ 'opening');Eq $opening.Count 160 'opening OpaqueFast rows';if(@($opening|Where-Object activeStrategy -CEQ 'opaque-fast').Count -ne 160){throw "Opening strategy mismatch in $runId"};$summaryPath="analyses-v2/$runId-$role.v2.analysis.json";$analysis=Read-Json $summaryPath;Eq $analysis.owningActiveStrategy.openingCounts.'opaque-fast' 160 'analysis opening strategy'}else{$summaryPath="analyses-v2/$runId-$role.v2.analysis.json";$analysis=Read-Json $summaryPath}
    Eq $analysis.parserVersion 2 'V2 analysis parser pin';Eq $analysis.integrity 'PASS' 'V2 analysis integrity';Eq $analysis.runId $runId 'V2 analysis run ID';Eq $analysis.apkSha256 $apks[$role] 'V2 analysis APK pin';Eq $analysis.shader $report.shader 'analysis/raw loaded shader'
    $moduleArtifact=@($pins.artifacts|Where-Object role -CEQ $role)[0]
    $opaqueModule=@($moduleArtifact.modules|Where-Object semanticKey -CEQ 'shipping_mobile_opaque_fast')[0]
    $genericModule=@($moduleArtifact.modules|Where-Object semanticKey -CEQ 'shipping_mobile_generic_dielectric')[0]
    $expectedShader="opaqueFast:shipping_mobile_opaque_fast@$($opaqueModule.sha256)|genericDielectric:shipping_mobile_generic_dielectric@$($genericModule.sha256)"
    Eq $report.shader $expectedShader 'loaded shader pair versus exact sealed module hashes'
    $rawPaths=[ordered]@{trialJson=Join-Path $rootPath "$base/trial.json";resultJson=Join-Path $rootPath "$base/$runId/result.json";benchmarkJson=Join-Path $rootPath "$base/$runId/benchmark.json";contextSamplesJsonl=Join-Path $rootPath "$base/context-samples.jsonl"}
    $relativeRaw=[ordered]@{trialJson="$base/trial.json";resultJson="$base/$runId/result.json";benchmarkJson="$base/$runId/benchmark.json";contextSamplesJsonl="$base/context-samples.jsonl"}
    foreach($key in $rawPaths.Keys){
        $rawHash=Hash $rawPaths[$key]
        Eq $rawHash $analysis.sourceHashes.$key "analysis raw hash $runId/$key"
        $manifestEntry=@($manifest.files|Where-Object path -CEQ $relativeRaw[$key])
        Eq $manifestEntry.Count 1 "manifest raw entry $runId/$key"
        Eq $rawHash $manifestEntry[0].sha256 "manifest/raw hash $runId/$key"
    }
    $context=@(Get-Content -LiteralPath $rawPaths.contextSamplesJsonl|ForEach-Object {$_|ConvertFrom-Json -DateKind String})
    if($context.Count -eq 0){throw "Empty context for $runId"}
    $contextProcess=[string]$trial.processId
    if([string]::IsNullOrWhiteSpace($contextProcess)){throw "Missing trial process ID for $runId"}
    foreach($sample in $context){
        Eq $sample.processId $contextProcess 'context process ID'
        Assert-FiniteRange $sample.batteryC -20 100 "battery temperature $runId"
        foreach($field in @(@{name='thermalStatus';max=6},@{name='gpuThermalPowerLevel';max=7})){
            $v=$sample.($field.name)
            if($null -ne $v){$n=[double]$v;if($n -lt 0 -or $n -gt $field.max -or $n -ne [math]::Truncate($n)){throw "Invalid context thermal/power field $runId/$($field.name)"}}
        }
    }
}
$toolMode='stored-val/dis-proofs-only'
$val=Join-Path $VulkanBin 'spirv-val.exe';$dis=Join-Path $VulkanBin 'spirv-dis.exe'
if((Test-Path -LiteralPath $val -PathType Leaf) -and (Test-Path -LiteralPath $dis -PathType Leaf)){
    foreach($artifact in $pins.artifacts){foreach($module in $artifact.modules){$path=Join-Path $rootPath ([string]$module.spvPath.Replace('/','\'));$null=& $val --target-env vulkan1.2 $path 2>&1;if($LASTEXITCODE -ne 0){throw "spirv-val failed: $($module.spvPath)"};$temp=Join-Path ([IO.Path]::GetTempPath()) ("secondary-"+[guid]::NewGuid().ToString('N'));try{$null=& $dis $path -o $temp 2>&1;if($LASTEXITCODE -ne 0 -or -not(Test-Path -LiteralPath $temp -PathType Leaf)){throw "spirv-dis failed: $($module.spvPath)"}}finally{if(Test-Path -LiteralPath $temp){Remove-Item -LiteralPath $temp -Force}}}}
    $toolMode='SPIR-V tools independently rerun and passed'
}
Write-Output "Curated archive verified: $($manifest.files.Count) manifest files; eight strict raw reports; eight pinned SPIR-V modules; $toolMode. APK/native binaries were not rehashed because excluded."
