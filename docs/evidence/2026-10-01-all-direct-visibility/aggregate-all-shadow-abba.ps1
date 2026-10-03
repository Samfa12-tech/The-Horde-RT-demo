param([Parameter(Mandatory=$true)][string]$OutputPath)
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
$analyses=Join-Path $root 'analyses'
$out=[IO.Path]::GetFullPath($OutputPath)
if(Test-Path -LiteralPath $out){throw "Refusing to overwrite aggregate output: $out"}
$expected=@(
    @{id='all-shadow-c1-route-20261001';role='control';workload='showcase-route-v1';lap='control-1'},
    @{id='all-shadow-s1-route-20261001';role='isolate';workload='showcase-route-v1';lap='isolate-1'},
    @{id='all-shadow-s2-route-20261001';role='isolate';workload='showcase-route-v1';lap='isolate-2'},
    @{id='all-shadow-c2-route-20261001';role='control';workload='showcase-route-v1';lap='control-2'},
    @{id='all-shadow-c1-high-20261001';role='control';workload='lantern-held-high-v1';lap='control-1'},
    @{id='all-shadow-s1-high-20261001';role='isolate';workload='lantern-held-high-v1';lap='isolate-1'},
    @{id='all-shadow-s2-high-20261001';role='isolate';workload='lantern-held-high-v1';lap='isolate-2'},
    @{id='all-shadow-c2-high-20261001';role='control';workload='lantern-held-high-v1';lap='control-2'},
    @{id='all-shadow-c1-live-20261001';role='control';workload='lantern-reveal-sequence-v1';lap='control-1'},
    @{id='all-shadow-s1-live-20261001';role='isolate';workload='lantern-reveal-sequence-v1';lap='isolate-1'},
    @{id='all-shadow-s2-live-20261001';role='isolate';workload='lantern-reveal-sequence-v1';lap='isolate-2'},
    @{id='all-shadow-c2-live-20261001';role='control';workload='lantern-reveal-sequence-v1';lap='control-2'}
)
function Median([double[]]$Values){$a=@($Values|Sort-Object);if($a.Count -eq 0){return $null};if($a.Count%2){return [double]$a[[int][math]::Floor($a.Count/2)]};return ([double]$a[$a.Count/2-1]+[double]$a[$a.Count/2])/2}
function Get-Stage($Stages,[string]$Key){$p=$Stages.PSObject.Properties[$Key];if($null -eq $p){return $null};return $p.Value}
$states=[Collections.Generic.List[object]]::new()
$records=[Collections.Generic.List[object]]::new()
foreach($item in $expected){
    $path=Join-Path $analyses "$($item.id)-$($item.role).analysis.json"
    if(-not(Test-Path -LiteralPath $path)){$states.Add([ordered]@{runId=$item.id;role=$item.role;workload=$item.workload;status='analysis-pending';path=$path});continue}
    $a=Get-Content -LiteralPath $path -Raw|ConvertFrom-Json
    if($a.runId -cne $item.id -or $a.integrity -cne 'PASS' -or $a.artifactRole -cne $item.role -or $a.workload -cne $item.workload){throw "Analysis identity mismatch: $path"}
    if($item.workload -ceq 'showcase-route-v1'){
        if($a.owningActiveStrategy.openingCounts.'opaque-fast' -ne 160 -or $a.owningActiveStrategy.openingCounts.Count -ne 1){throw "Opening strategy admission failed: $($item.id)"}
    }
    $targetZone=if($item.workload -ceq 'showcase-route-v1'){'opening'}else{'yellow-torch-bay'}
    $focus=@($a.zoneGpuCommandBufferTiming|Where-Object zone -CEQ $targetZone)
    if($focus.Count -ne 1){throw "Missing target GPU zone $targetZone in $($item.id)"}
    $cpu=$a.cpuStages
    $openingCpu=$null
    if($item.workload -ceq 'showcase-route-v1'){$openingCpu=$a.openingVsHeavyRouteStages.opening.cpuStages}
    $record=[ordered]@{
        runId=$item.id;lap=$item.lap;artifactRole=$item.role;workload=$item.workload;sourceCommit=$a.sourceCommit;buildLabel=$a.buildLabel;apkSha256=$a.apkSha256;shader=$a.shader;integrity=$a.integrity
        frames=$a.frames;cycleMedianMs=$a.overall.medianMs;cycleP95Ms=$a.overall.p95Ms;medianDerivedFps=$a.medianDerivedFps
        wholeGpuCommandBuffer=[ordered]@{medianMs=$a.gpuRtDurationMs.medianMilliseconds;p95Ms=$a.gpuRtDurationMs.p95Milliseconds;samples=$a.gpuRtDurationMs.sampleCount}
        focusGpuZone=[ordered]@{zone=$targetZone;frames=$focus[0].frames;medianMs=$focus[0].medianMs;p95NearestRankMs=$focus[0].p95NearestRankMs;framesAtOrBelow33_333Ms=$focus[0].within33_333Ms}
        openingCpuStages=$openingCpu
        cpuStageSummaries=[ordered]@{
            wholeFrameCycle=(Get-Stage $cpu 'wholeFrameCycleCpuMs');frameFenceWait=(Get-Stage $cpu 'frameFenceWaitCpuMs')
            blasRefitRecord=(Get-Stage $cpu 'blasRefitRecordCpuMs');tlasUpdateRecord=(Get-Stage $cpu 'tlasUpdateRecordCpuMs');traceCopyRecord=(Get-Stage $cpu 'traceCopyRecordCpuMs')
        }
        allZoneGpuCommandBufferTiming=$a.zoneGpuCommandBufferTiming
        context=[ordered]@{sampleCount=$a.context.samples;firstUtc=$a.context.firstUtc;lastUtc=$a.context.lastUtc;batteryStartC=$a.context.batteryStartC;batteryEndC=$a.context.batteryEndC;thermalStatuses=$a.context.thermalStatuses;gpuThermalPowerLevels=$a.context.gpuThermalPowerLevels;processIds=$a.context.processIds;alignment='Context sampling is outside and not frame-aligned to the measured ledger.'}
        timingNote='Whole GPU command-buffer timestamps and CPU stage summaries are distinct measurements. Current schema does not isolate GPU AS-build from RT dispatch.'
    }
    $records.Add($record)
    $states.Add([ordered]@{runId=$item.id;role=$item.role;workload=$item.workload;status='analyzed';path=$path})
}
$groupSummaries=[Collections.Generic.List[object]]::new()
foreach($workload in @('showcase-route-v1','lantern-held-high-v1','lantern-reveal-sequence-v1')){
    $group=@($expected|Where-Object workload -CEQ $workload)
    $available=@($records|Where-Object workload -CEQ $workload)
    $controls=@($available|Where-Object artifactRole -CEQ 'control')
    $isolates=@($available|Where-Object artifactRole -CEQ 'isolate')
    $complete=($available.Count -eq 4 -and $controls.Count -eq 2 -and $isolates.Count -eq 2)
    $controlMedian=if($complete){Median @($controls|ForEach-Object {[double]$_.cycleMedianMs})}else{$null}
    $isolateMedian=if($complete){Median @($isolates|ForEach-Object {[double]$_.cycleMedianMs})}else{$null}
    $controlP95=if($complete){Median @($controls|ForEach-Object {[double]$_.cycleP95Ms})}else{$null}
    $isolateP95=if($complete){Median @($isolates|ForEach-Object {[double]$_.cycleP95Ms})}else{$null}
    $relativePct=if($complete -and $controlMedian -ne 0){[math]::Round(100*($isolateMedian-$controlMedian)/$controlMedian,2)}else{$null}
    $focusName=if($workload -ceq 'showcase-route-v1'){'opening'}else{'yellow-torch-bay'}
    $controlFocus=if($complete){Median @($controls|ForEach-Object {[double]$_.focusGpuZone.medianMs})}else{$null}
    $isolateFocus=if($complete){Median @($isolates|ForEach-Object {[double]$_.focusGpuZone.medianMs})}else{$null}
    $controlFocusP95=if($complete){Median @($controls|ForEach-Object {[double]$_.focusGpuZone.p95NearestRankMs})}else{$null}
    $isolateFocusP95=if($complete){Median @($isolates|ForEach-Object {[double]$_.focusGpuZone.p95NearestRankMs})}else{$null}
    $controlPassFrames=if($complete){[int](($controls|ForEach-Object {[int]$_.focusGpuZone.framesAtOrBelow33_333Ms}|Measure-Object -Sum).Sum)}else{$null}
    $isolatePassFrames=if($complete){[int](($isolates|ForEach-Object {[int]$_.focusGpuZone.framesAtOrBelow33_333Ms}|Measure-Object -Sum).Sum)}else{$null}
    $pairs=@()
    if($complete){
        foreach($pair in @(@('control-1','isolate-1'),@('isolate-2','control-2'))){
            $left=@($available|Where-Object lap -CEQ $pair[0])[0];$right=@($available|Where-Object lap -CEQ $pair[1])[0]
            $pairs+=,[ordered]@{leftRun=$left.runId;rightRun=$right.runId;cycleMedianMs=@($left.cycleMedianMs,$right.cycleMedianMs);cycleP95Ms=@($left.cycleP95Ms,$right.cycleP95Ms);focusZoneMedianMs=@($left.focusGpuZone.medianMs,$right.focusGpuZone.medianMs);focusZoneP95Ms=@($left.focusGpuZone.p95NearestRankMs,$right.focusGpuZone.p95NearestRankMs);focusZone33_333PassFrames=@($left.focusGpuZone.framesAtOrBelow33_333Ms,$right.focusGpuZone.framesAtOrBelow33_333Ms);focusZoneFrameDenominators=@($left.focusGpuZone.frames,$right.focusGpuZone.frames)}
        }
    }
    $groupSummaries.Add([ordered]@{
        workload=$workload;status=if($complete){'complete-4-runs'}else{'partial-not-comparable'};expectedRuns=4;availableRuns=$available.Count
        controls=@($controls|ForEach-Object runId);isolates=@($isolates|ForEach-Object runId)
        cycle=[ordered]@{controlMedianOfRunMediansMs=$controlMedian;isolateMedianOfRunMediansMs=$isolateMedian;controlMedianOfRunP95Ms=$controlP95;isolateMedianOfRunP95Ms=$isolateP95;descriptiveMedianDeltaPercent=$relativePct}
        focusGpuZone=$focusName;zoneGpu=[ordered]@{controlMedianOfRunMediansMs=$controlFocus;isolateMedianOfRunMediansMs=$isolateFocus;controlMedianOfRunP95Ms=$controlFocusP95;isolateMedianOfRunP95Ms=$isolateFocusP95;controlFramesAtOrBelow33_333Ms=$controlPassFrames;isolateFramesAtOrBelow33_333Ms=$isolatePassFrames;controlDenominator=if($complete){[int](($controls|ForEach-Object {[int]$_.focusGpuZone.frames}|Measure-Object -Sum).Sum)}else{$null};isolateDenominator=if($complete){[int](($isolates|ForEach-Object {[int]$_.focusGpuZone.frames}|Measure-Object -Sum).Sum)}else{$null}}
        orderedPairs=$pairs
        interpretation='Descriptive ABBA grouping only; no causal optimization, additive ray-cost, or production-gain acceptance.'
    })
}
$completeOverall=($records.Count -eq 12)
$summary=[ordered]@{
    schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');status=if($completeOverall){'all-12-analyzed'}else{'partial-waiting-for-completed-analyses'}
    analysisCount=$records.Count;expectedAnalysisCount=12;individualRunStates=@($states);perRunTable=@($records);perWorkloadSummaries=@($groupSummaries)
    comparisonOrder='C1, S1, S2, C2 within each workload; controls and isolates summarized only when all four strict individual analyses are present.'
    comparisonMethod='Report median across two per-run medians for each group, median across per-run p95 values, exact ordered C1/S1 and S2/C2 pairs, and summed per-zone frames at or below 33.333 ms. These summaries are descriptive and are not pooled-frame estimates.'
    contextInterpretation='Per-run process IDs, battery temperatures, thermal statuses, and GPU thermal power levels are retained. Here B1/B2 share a process ID distinct from A1 and A2; the sequence also spans unequal thermal context. Context samples are not frame-aligned.'
    admissionChecks=[ordered]@{routeOpeningFramesOwnedByOpaqueFast=160;allExistingIndividualAnalysesStrictlyParsed=$true;perRunGpuAndCpuTimingsKeptSeparate=$true}
    limitations=@(
        'The all-direct-visibility isolate is intentionally nonphysical: it removes opaque blocker visibility and GenericDielectric shadow-transmittance evaluation, including real glass attenuation.',
        'This is not a production-gain result, causal estimate, isolated shadow-ray cost, or additive ray-cost decomposition.',
        'GPU command-buffer duration includes GPU AS work, RT dispatch, and copy; GPU AS and dispatch are not separately reported. CPU BLAS/TLAS stage values are CPU record timings.',
        'The held/live focus-zone GPU statistic is the complete GPU command-buffer interval while the workload is in yellow-torch-bay; it is not isolated torch work or torch-ray cost. Route opening uses the ordinary opening zone interval.',
        'Persistent residual heavy-route and held/live GPU time after the omitted visibility work bounds the interpretation; see per-run zone timing table.',
        'Thermal/power context is sampled outside the measured frame ledger and is descriptive, not frame-aligned.'
    )
}
$parent=Split-Path -Parent $out
if(-not(Test-Path -LiteralPath $parent)){throw "Output parent does not exist: $parent"}
$summary|ConvertTo-Json -Depth 16|Set-Content -LiteralPath $out -Encoding utf8
Write-Output "Aggregate $($summary.status): $($records.Count)/12 strict analyses; $out"
