param([Parameter(Mandatory=$true)][string]$OutputPath)
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
$analyses=Join-Path $root 'analyses'
$out=[IO.Path]::GetFullPath($OutputPath)
if(Test-Path -LiteralPath $out){throw "Refusing to overwrite aggregate output: $out"}
$expected=@(
    @{id='primary-hit-c1-route-20261001';role='control';workload='showcase-route-v1';lap='control-1'},
    @{id='primary-hit-p1-route-20261001';role='reference';workload='showcase-route-v1';lap='reference-1'},
    @{id='primary-hit-p2-route-20261001';role='reference';workload='showcase-route-v1';lap='reference-2'},
    @{id='primary-hit-c2-route-20261001';role='control';workload='showcase-route-v1';lap='control-2'},
    @{id='primary-hit-c1-high-20261001';role='control';workload='lantern-held-high-v1';lap='control-1'},
    @{id='primary-hit-p1-high-20261001';role='reference';workload='lantern-held-high-v1';lap='reference-1'},
    @{id='primary-hit-p2-high-20261001';role='reference';workload='lantern-held-high-v1';lap='reference-2'},
    @{id='primary-hit-c2-high-20261001';role='control';workload='lantern-held-high-v1';lap='control-2'},
    @{id='primary-hit-c1-live-20261001';role='control';workload='lantern-reveal-sequence-v1';lap='control-1'},
    @{id='primary-hit-p1-live-20261001';role='reference';workload='lantern-reveal-sequence-v1';lap='reference-1'},
    @{id='primary-hit-p2-live-20261001';role='reference';workload='lantern-reveal-sequence-v1';lap='reference-2'},
    @{id='primary-hit-c2-live-20261001';role='control';workload='lantern-reveal-sequence-v1';lap='control-2'}
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
        $openingCounts=$a.owningActiveStrategy.openingCounts
        $strategyProperties=@($openingCounts.PSObject.Properties)
        $opaqueProperty=$openingCounts.PSObject.Properties['opaque-fast']
        if($null -eq $opaqueProperty -or $opaqueProperty.Value -ne 160 -or $strategyProperties.Count -ne 1){throw "Opening strategy admission failed: $($item.id)"}
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
    $references=@($available|Where-Object artifactRole -CEQ 'reference')
    $complete=($available.Count -eq 4 -and $controls.Count -eq 2 -and $references.Count -eq 2)
    $controlMedian=if($complete){Median @($controls|ForEach-Object {[double]$_.cycleMedianMs})}else{$null}
    $referenceMedian=if($complete){Median @($references|ForEach-Object {[double]$_.cycleMedianMs})}else{$null}
    $controlP95=if($complete){Median @($controls|ForEach-Object {[double]$_.cycleP95Ms})}else{$null}
    $referenceP95=if($complete){Median @($references|ForEach-Object {[double]$_.cycleP95Ms})}else{$null}
    $relativePct=if($complete -and $controlMedian -ne 0){[math]::Round(100*($referenceMedian-$controlMedian)/$controlMedian,2)}else{$null}
    $focusName=if($workload -ceq 'showcase-route-v1'){'opening'}else{'yellow-torch-bay'}
    $controlFocus=if($complete){Median @($controls|ForEach-Object {[double]$_.focusGpuZone.medianMs})}else{$null}
    $referenceFocus=if($complete){Median @($references|ForEach-Object {[double]$_.focusGpuZone.medianMs})}else{$null}
    $controlFocusP95=if($complete){Median @($controls|ForEach-Object {[double]$_.focusGpuZone.p95NearestRankMs})}else{$null}
    $referenceFocusP95=if($complete){Median @($references|ForEach-Object {[double]$_.focusGpuZone.p95NearestRankMs})}else{$null}
    $controlPassFrames=if($complete){[int](($controls|ForEach-Object {[int]$_.focusGpuZone.framesAtOrBelow33_333Ms}|Measure-Object -Sum).Sum)}else{$null}
    $referencePassFrames=if($complete){[int](($references|ForEach-Object {[int]$_.focusGpuZone.framesAtOrBelow33_333Ms}|Measure-Object -Sum).Sum)}else{$null}
    $pairs=@()
    if($complete){
        foreach($pair in @(@('control-1','reference-1'),@('reference-2','control-2'))){
            $left=@($available|Where-Object lap -CEQ $pair[0])[0];$right=@($available|Where-Object lap -CEQ $pair[1])[0]
            $pairs+=,[ordered]@{leftRun=$left.runId;rightRun=$right.runId;cycleMedianMs=@($left.cycleMedianMs,$right.cycleMedianMs);cycleP95Ms=@($left.cycleP95Ms,$right.cycleP95Ms);focusZoneMedianMs=@($left.focusGpuZone.medianMs,$right.focusGpuZone.medianMs);focusZoneP95Ms=@($left.focusGpuZone.p95NearestRankMs,$right.focusGpuZone.p95NearestRankMs);focusZone33_333PassFrames=@($left.focusGpuZone.framesAtOrBelow33_333Ms,$right.focusGpuZone.framesAtOrBelow33_333Ms);focusZoneFrameDenominators=@($left.focusGpuZone.frames,$right.focusGpuZone.frames)}
        }
    }
    $groupSummaries.Add([ordered]@{
        workload=$workload;status=if($complete){'complete-4-runs'}else{'partial-not-comparable'};expectedRuns=4;availableRuns=$available.Count
        controls=@($controls|ForEach-Object runId);references=@($references|ForEach-Object runId)
        cycle=[ordered]@{controlMedianOfRunMediansMs=$controlMedian;referenceMedianOfRunMediansMs=$referenceMedian;controlMedianOfRunP95Ms=$controlP95;referenceMedianOfRunP95Ms=$referenceP95;descriptiveMedianDeltaPercent=$relativePct}
        focusGpuZone=$focusName;zoneGpu=[ordered]@{controlMedianOfRunMediansMs=$controlFocus;referenceMedianOfRunMediansMs=$referenceFocus;controlMedianOfRunP95Ms=$controlFocusP95;referenceMedianOfRunP95Ms=$referenceFocusP95;controlFramesAtOrBelow33_333Ms=$controlPassFrames;referenceFramesAtOrBelow33_333Ms=$referencePassFrames;controlDenominator=if($complete){[int](($controls|ForEach-Object {[int]$_.focusGpuZone.frames}|Measure-Object -Sum).Sum)}else{$null};referenceDenominator=if($complete){[int](($references|ForEach-Object {[int]$_.focusGpuZone.frames}|Measure-Object -Sum).Sum)}else{$null}}
        orderedPairs=$pairs
        interpretation='Descriptive ABBA grouping only; no causal optimization, additive ray-cost, or production-gain acceptance.'
    })
}
$completeOverall=($records.Count -eq 12)
$summary=[ordered]@{
    schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');status=if($completeOverall){'all-12-analyzed'}else{'partial-waiting-for-completed-analyses'}
    classification='nonphysical-primary-hit-reference-investigation-only';optimizationClaim='none';causalAcceptance='not-evaluated';productionPromotion='not-authorized'
    controlApkSha256='a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033';referenceApkSha256='40d0f759dca40ff8433ce4aca19156a135a9146a960f115f4cdbb78d3db7cf9e'
    analysisCount=$records.Count;expectedAnalysisCount=12;individualRunStates=@($states);perRunTable=@($records);perWorkloadSummaries=@($groupSummaries)
    comparisonOrder='C1, P1, P2, C2 within each workload; control/reference groups are summarized only when all four strict individual analyses are present.'
    comparisonMethod='Report median across two per-run medians for each group, median across per-run p95 values, exact ordered C1/P1 and P2/C2 pairs, and summed per-zone frames at or below 33.333 ms. These summaries are descriptive and are not pooled-frame estimates.'
    contextInterpretation='Per-run process IDs, battery temperatures, thermal statuses, and GPU thermal power levels are retained. P1/P2 are intended to share the reference installation/process; A1 and A2 are control installs. Context samples are not frame-aligned and any observed context/order differences remain relevant.'
    admissionChecks=[ordered]@{routeOpeningFramesOwnedByOpaqueFast=160;allExistingIndividualAnalysesStrictlyParsed=$true;perRunGpuAndCpuTimingsKeptSeparate=$true}
    limitations=@(
        'The primary-hit reference is intentionally nonphysical: it keeps primary traversal but replaces radiance with a base/normal reference on hits or sky on misses; it omits ordinary direct lighting, secondary reflection/refraction, and fire/atmosphere contributions.',
        'This is not full primary-hit cost, full-material cost, a production-gain result, causal estimate, or additive ray-cost decomposition.',
        'GPU command-buffer duration includes GPU AS work, RT dispatch, and copy; GPU AS and dispatch are not separately reported. CPU BLAS/TLAS stage values are CPU record timings.',
        'The held/live focus-zone GPU statistic is the complete GPU command-buffer interval while the workload is in yellow-torch-bay; it is not isolated torch work or torch-ray cost. Route opening uses the ordinary opening zone interval.',
        'Unused hit attributes may compile away; the reference is primary traversal plus only its consumed base/normal path, not an additive decomposition or lower bound for the full primary material path.',
        'Thermal/power context is sampled outside the measured frame ledger and is descriptive, not frame-aligned.'
    )
}
$parent=Split-Path -Parent $out
if(-not(Test-Path -LiteralPath $parent)){throw "Output parent does not exist: $parent"}
$summary|ConvertTo-Json -Depth 16|Set-Content -LiteralPath $out -Encoding utf8
Write-Output "Aggregate $($summary.status): $($records.Count)/12 strict analyses; $out"
