param([ValidateSet('v2','v3')][string]$Version='v3')
$ErrorActionPreference='Stop'
$id="shipping-opening-gpu-stages-$Version-route-20260930"
$root=Join-Path $PSScriptRoot "phone/$id"
$report=Get-Content (Join-Path $root "$id/benchmark.json") -Raw | ConvertFrom-Json
$trial=Get-Content (Join-Path $root 'trial.json') -Raw | ConvertFrom-Json
$analysis=Get-Content (Join-Path $root 'analysis.json') -Raw | ConvertFrom-Json
if($analysis.integrity -cne 'PASS' -or $report.completedFrameEvidence.rows.Count -ne 1838){throw 'Full benchmark integrity must pass first'}
$stages=@{}
$retained=[Collections.Generic.List[string]]::new()
$warnings=[Collections.Generic.List[string]]::new()
$logName=if($Version -ceq 'v2'){'phone-stage-logcat.txt'}else{'phone-stage-logcat-v3.txt'}
foreach($line in Get-Content (Join-Path $PSScriptRoot "opening-stage-proposal/$logName")){
    if($line -notmatch '^I/HordeRtProbeBridge\(\s*(\d+)\): OPENING_GPU_STAGE stage=(geometry-as|rt-dispatch) epoch=(\d+) generation=(\d+) attempt=(\d+) record=(\d+) tick=(\d+) slot=(\d+) submission=(\d+) submitted=(\d+) joined=(\d+) valid=(\d+) ms=(-?[\d.]+) status=(\d+) vk=(-?\d+) timer=(\S+)$'){
        if($line -match 'OPENING_GPU_STAGE_(UNAVAILABLE|INIT)'){$warnings.Add($line)}
        continue
    }
    if($Matches[1] -cne $trial.processId){continue}
    $key=(@(3..9 | ForEach-Object {$Matches[$_]}) -join ':')
    $stage=$Matches[2]
    $stageKey="$key/$stage"
    if($stages.ContainsKey($stageKey)){throw "Duplicate probe stage identity: $stageKey"}
    if($Matches[10] -ne '1' -or $Matches[11] -ne '1' -or $Matches[12] -ne '1' -or $Matches[14] -ne '1' -or $Matches[15] -ne '0' -or $Matches[16] -cne 'available'){throw 'Invalid probe result; do not fabricate a duration'}
    $duration=[double]::Parse($Matches[13],[Globalization.CultureInfo]::InvariantCulture)
    if(-not [double]::IsFinite($duration) -or $duration -le 0){throw 'Invalid GPU-stage duration'}
    $stages[$stageKey]=$duration
}
$joinedRows=@(foreach($row in $report.completedFrameEvidence.rows){
    $identity=$row.completionIdentity
    $key=(@($identity.sceneEpoch,$identity.measurementGeneration,$identity.recordAttemptSerial,$identity.recordSerial,$identity.simulationTick,$identity.frameSlot,$identity.submissionSerial) -join ':')
    foreach($stage in @('geometry-as','rt-dispatch')){if(-not $stages.ContainsKey("$key/$stage")){throw "Missing exact measured-stage join: $key/$stage"}}
    [ordered]@{index=$row.index;zone=$row.zoneName;activeStrategy=$row.activeStrategy;completedIdentity=$identity;geometryAsGpuMs=$stages["$key/geometry-as"];rtDispatchGpuMs=$stages["$key/rt-dispatch"];fullCommandBufferGpuMs=[double]$row.gpuDurationNanoseconds/1e6}
})
function Stats($values){
    $sorted=@($values | Sort-Object)
    $middle=[int][math]::Floor($sorted.Count/2)
    $median=if($sorted.Count%2){$sorted[$middle]}else{($sorted[$middle-1]+$sorted[$middle])/2}
    [ordered]@{frames=$sorted.Count;medianMs=$median;p95NearestRankMs=$sorted[[int][math]::Ceiling($sorted.Count*.95)-1];meanMs=($sorted | Measure-Object -Average).Average}
}
$zones=@(foreach($zone in @($joinedRows.zone | Sort-Object -Unique)){
    $rows=@($joinedRows | Where-Object zone -CEQ $zone)
    if(@($rows | Where-Object activeStrategy -CNE 'opaque-fast').Count){throw 'Unexpected non-OpaqueFast route row'}
    [ordered]@{zone=$zone;frames=$rows.Count;activeStrategy='opaque-fast';geometryAsGpu=Stats $rows.geometryAsGpuMs;rtDispatchGpu=Stats $rows.rtDispatchGpuMs;fullCommandBufferGpu=Stats $rows.fullCommandBufferGpuMs}
})
[ordered]@{runId=$id;apkSha256=$trial.installedApkSha256;processId=$trial.processId;matchedCompletedRows=$joinedRows.Count;identityFieldsMatched=7;zoneStrategyAuthority='completed benchmark row, not fence-time mutable state';stageTimestampSemantics='AS-build at both AS endpoints; selected RT/compute stage at both dispatch endpoints';zones=$zones;initializationAndUnavailableMessages=$warnings.ToArray();limitations=@('Temporary opt-in probe, not frozen A/B performance.','Stage-limited timestamps may occur at a logically later stage; no isolated-engine or additive claim.','AS interval excludes CPU skinning/preparation.','Dispatch includes all shading/lighting/shadow/ray traversal; does not isolate torch cost.','Cooled observation is not uncooled sustained or actual display-pacing acceptance.')} | ConvertTo-Json -Depth 10 > (Join-Path $root 'stage-analysis.json')
$joinedRows | ConvertTo-Json -Depth 8 > (Join-Path $root 'joined-stage-rows.json')
$zones | Where-Object zone -CEQ opening | ConvertTo-Json -Depth 7
