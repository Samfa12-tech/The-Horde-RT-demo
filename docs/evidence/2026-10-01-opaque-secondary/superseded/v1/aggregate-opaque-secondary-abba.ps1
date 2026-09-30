param([Parameter(Mandatory=$true)][string]$OutputPath)
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
$analyses=Join-Path $root 'analyses'
$out=[IO.Path]::GetFullPath($OutputPath)
if(Test-Path -LiteralPath $out){throw "Refusing to overwrite aggregate output: $out"}
$expected=[Collections.Generic.List[object]]::new()
foreach($workload in @(@{key='route';value='showcase-route-v1'},@{key='live';value='lantern-reveal-sequence-v1'})){
    foreach($position in @(@{slot='c1';role='control';lap='control-1'},@{slot='s1';role='isolate';lap='isolate-1'},@{slot='s2';role='isolate';lap='isolate-2'},@{slot='c2';role='control';lap='control-2'})){
        $expected.Add([ordered]@{id="opaque-secondary-$($position.slot)-$($workload.key)-20261001";role=$position.role;workload=$workload.value;lap=$position.lap})
    }
}
function Median([double[]]$Values){$a=@($Values|Sort-Object);if($a.Count -eq 0){throw 'Cannot calculate empty median'};if($a.Count%2){return [double]$a[[int][math]::Floor($a.Count/2)]};return ([double]$a[$a.Count/2-1]+[double]$a[$a.Count/2])/2}
function Verify-RawHashes($Analysis){
    $directory=[IO.Path]::GetFullPath([string]$Analysis.runDirectory)
    $paths=[ordered]@{trialJson=(Join-Path $directory 'trial.json');resultJson=(Join-Path $directory "$($Analysis.runId)/result.json");benchmarkJson=(Join-Path $directory "$($Analysis.runId)/benchmark.json");contextSamplesJsonl=(Join-Path $directory 'context-samples.jsonl')}
    foreach($key in $paths.Keys){
        if(-not(Test-Path -LiteralPath $paths[$key])){throw "Missing raw source $key for $($Analysis.runId)"}
        $actual=(Get-FileHash -LiteralPath $paths[$key] -Algorithm SHA256).Hash.ToLowerInvariant()
        $expectedHash=[string]$Analysis.sourceHashes.$key
        if($actual -cne $expectedHash){throw "Raw SHA mismatch $key for $($Analysis.runId)"}
    }
}
function Stage($Stages,[string]$Key){$p=$Stages.PSObject.Properties[$Key];if($null -eq $p){return $null};return $p.Value}
$records=[Collections.Generic.List[object]]::new()
foreach($item in $expected){
    $path=Join-Path $analyses "$($item.id)-$($item.role).analysis.json"
    if(-not(Test-Path -LiteralPath $path)){throw "ABBA not ready: missing strict analysis $($item.id) ($($item.role))"}
    $a=Get-Content -LiteralPath $path -Raw|ConvertFrom-Json
    if($a.runId -cne $item.id -or $a.integrity -cne 'PASS' -or $a.artifactRole -cne $item.role -or $a.workload -cne $item.workload){throw "Analysis identity mismatch: $path"}
    if($a.sourceCommit -cne '71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95'){throw "Source identity mismatch: $($item.id)"}
    if($a.apkSha256 -cne $(if($item.role -ceq 'control'){'a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033'}else{'ce1c1548ed3937d1ab99b57c523b62a8a1e244538c01e9075bb2f7e56e766cce'})){throw "APK identity mismatch: $($item.id)"}
    Verify-RawHashes $a
    if($item.workload -ceq 'showcase-route-v1'){
        $p=$a.owningActiveStrategy.openingCounts.PSObject.Properties['opaque-fast']
        if($null -eq $p -or $p.Value -ne 160 -or @($a.owningActiveStrategy.openingCounts.PSObject.Properties).Count -ne 1){throw "Opening OpaqueFast admission failed: $($item.id)"}
    }
    $focusName=if($item.workload -ceq 'showcase-route-v1'){'opening'}else{'yellow-torch-bay'}
    $focus=@($a.zoneGpuCommandBufferTiming|Where-Object zone -CEQ $focusName)
    if($focus.Count -ne 1){throw "Missing owning GPU timing zone $focusName in $($item.id)"}
    if($focus[0].frames -ne $(if($item.workload -ceq 'showcase-route-v1'){160}else{600})){throw "Unexpected GPU owning-row denominator in $($item.id)"}
    $focusCpu=@($a.completionOwnedZoneCpuStages|Where-Object zone -CEQ $focusName)
    if($focusCpu.Count -ne 1 -or $focusCpu[0].frames -ne $focus[0].frames -or $null -eq $focusCpu[0].cpuStages){throw "Missing completion-owned CPU stages for $focusName in $($item.id)"}
    $records.Add([ordered]@{
        runId=$item.id;lap=$item.lap;artifactRole=$item.role;workload=$item.workload;sourceCommit=$a.sourceCommit;buildLabel=$a.buildLabel;apkSha256=$a.apkSha256;shader=$a.shader
        reportSha256=$a.sourceHashes.benchmarkJson;trialSha256=$a.sourceHashes.trialJson;resultSha256=$a.sourceHashes.resultJson;contextSha256=$a.sourceHashes.contextSamplesJsonl
        frames=$a.frames;cycleMedianMs=$a.overall.medianMs;cycleP95Ms=$a.overall.p95Ms
        wholeGpuCommandBuffer=[ordered]@{medianMs=$a.gpuRtDurationMs.medianMilliseconds;p95Ms=$a.gpuRtDurationMs.p95Milliseconds;samples=$a.gpuRtDurationMs.sampleCount}
        focusGpuZone=[ordered]@{zone=$focusName;frames=$focus[0].frames;medianMs=$focus[0].medianMs;p95NearestRankMs=$focus[0].p95NearestRankMs;framesAtOrBelow33_333Ms=$focus[0].within33_333Ms}
        focusCompletionOwnedCpuStages=$focusCpu[0].cpuStages
        wholeCpuStages=[ordered]@{wholeFrameCycle=(Stage $a.cpuStages 'wholeFrameCycleCpuMs');frameFenceWait=(Stage $a.cpuStages 'frameFenceWaitCpuMs');blasRefitRecord=(Stage $a.cpuStages 'blasRefitRecordCpuMs');tlasUpdateRecord=(Stage $a.cpuStages 'tlasUpdateRecordCpuMs');traceCopyRecord=(Stage $a.cpuStages 'traceCopyRecordCpuMs')}
        allZoneGpuCommandBufferTiming=$a.zoneGpuCommandBufferTiming
        context=[ordered]@{sampleCount=$a.context.samples;firstUtc=$a.context.firstUtc;lastUtc=$a.context.lastUtc;batteryStartC=$a.context.batteryStartC;batteryEndC=$a.context.batteryEndC;thermalStatuses=$a.context.thermalStatuses;gpuThermalPowerLevels=$a.context.gpuThermalPowerLevels;processIds=$a.context.processIds;alignment='Context sampling is outside and not frame-aligned to the measured ledger.'}
    })
}
if($records.Count -ne 8){throw "Expected exactly eight strict analyses; found $($records.Count)"}
$summaries=[Collections.Generic.List[object]]::new()
foreach($workload in @('showcase-route-v1','lantern-reveal-sequence-v1')){
    $group=@($records|Where-Object workload -CEQ $workload)
    $controls=@($group|Where-Object artifactRole -CEQ 'control')
    $isolates=@($group|Where-Object artifactRole -CEQ 'isolate')
    if($group.Count -ne 4 -or $controls.Count -ne 2 -or $isolates.Count -ne 2){throw "Incomplete ordered ABBA group for $workload"}
    $pairs=@()
    foreach($names in @(@('control-1','isolate-1'),@('isolate-2','control-2'))){
        $left=@($group|Where-Object lap -CEQ $names[0])[0];$right=@($group|Where-Object lap -CEQ $names[1])[0]
        $pairs+=,[ordered]@{leftRun=$left.runId;rightRun=$right.runId;cycleMedianMs=@($left.cycleMedianMs,$right.cycleMedianMs);cycleP95Ms=@($left.cycleP95Ms,$right.cycleP95Ms);focusZoneMedianMs=@($left.focusGpuZone.medianMs,$right.focusGpuZone.medianMs);focusZoneP95Ms=@($left.focusGpuZone.p95NearestRankMs,$right.focusGpuZone.p95NearestRankMs);focusZone33_333PassFrames=@($left.focusGpuZone.framesAtOrBelow33_333Ms,$right.focusGpuZone.framesAtOrBelow33_333Ms);focusZoneDenominators=@($left.focusGpuZone.frames,$right.focusGpuZone.frames)}
    }
    $controlCycle=Median @($controls|ForEach-Object {[double]$_.cycleMedianMs});$isolateCycle=Median @($isolates|ForEach-Object {[double]$_.cycleMedianMs})
    $focusControl=Median @($controls|ForEach-Object {[double]$_.focusGpuZone.medianMs});$focusIsolate=Median @($isolates|ForEach-Object {[double]$_.focusGpuZone.medianMs})
    $summaries.Add([ordered]@{
        workload=$workload;status='complete-4-runs';expectedRuns=4;controlRuns=@($controls|ForEach-Object runId);isolateRuns=@($isolates|ForEach-Object runId)
        cycle=[ordered]@{controlMedianOfRunMediansMs=$controlCycle;isolateMedianOfRunMediansMs=$isolateCycle;controlMedianOfRunP95Ms=(Median @($controls|ForEach-Object {[double]$_.cycleP95Ms}));isolateMedianOfRunP95Ms=(Median @($isolates|ForEach-Object {[double]$_.cycleP95Ms}));descriptiveMedianDeltaPercent=[math]::Round(100*($isolateCycle-$controlCycle)/$controlCycle,2)}
        focusGpuZone=if($workload -ceq 'showcase-route-v1'){'opening'}else{'yellow-torch-bay'}
        focusGpu=[ordered]@{controlMedianOfRunMediansMs=$focusControl;isolateMedianOfRunMediansMs=$focusIsolate;controlMedianOfRunP95Ms=(Median @($controls|ForEach-Object {[double]$_.focusGpuZone.p95NearestRankMs}));isolateMedianOfRunP95Ms=(Median @($isolates|ForEach-Object {[double]$_.focusGpuZone.p95NearestRankMs}));controlFramesAtOrBelow33_333Ms=[int](($controls|ForEach-Object {[int]$_.focusGpuZone.framesAtOrBelow33_333Ms}|Measure-Object -Sum).Sum);isolateFramesAtOrBelow33_333Ms=[int](($isolates|ForEach-Object {[int]$_.focusGpuZone.framesAtOrBelow33_333Ms}|Measure-Object -Sum).Sum);controlDenominator=[int](($controls|ForEach-Object {[int]$_.focusGpuZone.frames}|Measure-Object -Sum).Sum);isolateDenominator=[int](($isolates|ForEach-Object {[int]$_.focusGpuZone.frames}|Measure-Object -Sum).Sum)}
        orderedPairs=$pairs
        interpretation='Descriptive ABBA grouping only; the isolate is a nonphysical omission cost-bound, not an optimization candidate, additive saving, or accepted quality result.'
    })
}
$totals=[ordered]@{route=($records|Where-Object workload -CEQ 'showcase-route-v1'|Measure-Object -Property frames -Sum).Sum;live=($records|Where-Object workload -CEQ 'lantern-reveal-sequence-v1'|Measure-Object -Property frames -Sum).Sum}
if($totals.route -ne 7352 -or $totals.live -ne 2400 -or ($totals.route+$totals.live) -ne 9752){throw "Unexpected total owning rows: route=$($totals.route), live=$($totals.live)"}
$summary=[ordered]@{
    schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');status='all-8-strict-analyses';classification='nonphysical-opaque-primary-secondary-omission-cost-bound-investigation-only';optimizationClaim='none';causalAcceptance='not-evaluated';qualityAcceptance='not-evaluated';productionPromotion='not-authorized'
    sourceCommit='71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95';controlApkSha256='a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033';isolateApkSha256='ce1c1548ed3937d1ab99b57c523b62a8a1e244538c01e9075bb2f7e56e766cce'
    perRunCount=$records.Count;expectedRunCount=8;routeOwningRows=$totals.route;liveOwningRows=$totals.live;totalOwningRows=($totals.route+$totals.live);perRunTable=@($records);perWorkloadSummaries=@($summaries)
    comparisonOrder='C1, S1, S2, C2 separately for route and live; both controls remain separate from the two isolates.'
    comparisonMethod='Two per-run medians per role summarized by median-of-run-medians; median of per-run p95s and ordered C1/S1 and S2/C2 pairs. No pooled-frame estimate.'
    contextInterpretation='Per-run process IDs, battery temperatures, thermal statuses and GPU thermal power are retained. Context is not frame-aligned; order, cooling and workload limits remain material.'
    admissionChecks=[ordered]@{allEightRawHashJoinsRecomputed=$true;allIndividualLedgersStrictlyParsed=$true;routeOpeningOpaqueFastRowsPerRun=160;allGpuOwningRowsWithDenominators=$true;completionOwnedCpuZoneStages=$true}
    limitations=@('The isolate intentionally omits opaque-primary secondary bounce/reflected-fire contributions and is nonphysical; this is a cost-bound investigation only.','No optimization, additive ray-cost, production-gain, causal, or quality acceptance follows.','Whole GPU command-buffer timestamps include GPU AS work, RT dispatch and copy; they do not isolate secondary work or GPU AS versus RT. CPU BLAS/TLAS stages are CPU record timings.','The route opening focus is the exact ordinary opening zone; live focus is the 600-frame yellow-torch-bay whole GPU interval, not isolated torch work.','Thermal/power context is external to the owning frame ledger and not frame-aligned.','This is not sustained 30-FPS or display-pacing evidence.')
}
$parent=Split-Path -Parent $out
if(-not(Test-Path -LiteralPath $parent)){throw "Output parent does not exist: $parent"}
$summary|ConvertTo-Json -Depth 18|Set-Content -LiteralPath $out -Encoding utf8
Write-Output "Aggregate $($summary.status): $($records.Count)/8 strict analyses; $($summary.totalOwningRows) owning rows; $out"
