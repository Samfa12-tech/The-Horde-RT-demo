[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
$output=Join-Path $root 'supplemental-completed-zone-cpu-20261001.json'
if(Test-Path -LiteralPath $output){throw "Refusing to overwrite supplementary receipt: $output"}
$sourceCommit='71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95'
$apks=@{control='a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033';reference='40d0f759dca40ff8433ce4aca19156a135a9146a960f115f4cdbb78d3db7cf9e'}
$runs=@(
    @{id='primary-hit-c1-route-20261001';role='control';workload='showcase-route-v1';targetZone='opening';frames=160},
    @{id='primary-hit-p1-route-20261001';role='reference';workload='showcase-route-v1';targetZone='opening';frames=160},
    @{id='primary-hit-p2-route-20261001';role='reference';workload='showcase-route-v1';targetZone='opening';frames=160},
    @{id='primary-hit-c2-route-20261001';role='control';workload='showcase-route-v1';targetZone='opening';frames=160},
    @{id='primary-hit-c1-high-20261001';role='control';workload='lantern-held-high-v1';targetZone='yellow-torch-bay';frames=600},
    @{id='primary-hit-p1-high-20261001';role='reference';workload='lantern-held-high-v1';targetZone='yellow-torch-bay';frames=600},
    @{id='primary-hit-p2-high-20261001';role='reference';workload='lantern-held-high-v1';targetZone='yellow-torch-bay';frames=600},
    @{id='primary-hit-c2-high-20261001';role='control';workload='lantern-held-high-v1';targetZone='yellow-torch-bay';frames=600},
    @{id='primary-hit-c1-live-20261001';role='control';workload='lantern-reveal-sequence-v1';targetZone='yellow-torch-bay';frames=600},
    @{id='primary-hit-p1-live-20261001';role='reference';workload='lantern-reveal-sequence-v1';targetZone='yellow-torch-bay';frames=600},
    @{id='primary-hit-p2-live-20261001';role='reference';workload='lantern-reveal-sequence-v1';targetZone='yellow-torch-bay';frames=600},
    @{id='primary-hit-c2-live-20261001';role='control';workload='lantern-reveal-sequence-v1';targetZone='yellow-torch-bay';frames=600}
)
$stageNames=@('simulationStepCpuMs','skinCpuMs','playerSkinCpuMs','characterSkinCpuMs','dynamicUploadCpuMs',
    'blasRefitRecordCpuMs','tlasUpdateRecordCpuMs','traceCopyRecordCpuMs','frameFenceWaitCpuMs',
    'imageAcquireCpuMs','queueSubmitCpuMs','presentCallCpuMs','wholeFrameCycleCpuMs')
function Assert-Equal($Actual,$Expected,[string]$Label){if($Actual -cne $Expected){throw "$Label expected '$Expected', found '$Actual'"}}
function Assert-Near([double]$Actual,[double]$Expected,[double]$Tolerance,[string]$Label){if([math]::Abs($Actual-$Expected) -gt $Tolerance){throw "$Label differs: $Actual vs $Expected"}}
function Read-Json([string]$Path){Get-Content -LiteralPath $Path -Raw|ConvertFrom-Json -DateKind String}
function Project-Stats($Stats){
    $result=[ordered]@{}
    foreach($property in $Stats.PSObject.Properties){
        if($property.Name -eq 'sampleCount'){$result.sampleCount=[int]$property.Value;continue}
        if($property.Value -is [ValueType]){$result[$property.Name]=[double]$property.Value}
    }
    return $result
}

$records=[Collections.Generic.List[object]]::new()
foreach($item in $runs){
    $id=[string]$item.id
    $runDir=Join-Path $root "phone\$id"
    $reportPath=Join-Path $runDir "$id\benchmark.json"
    $trialPath=Join-Path $runDir 'trial.json'
    $resultPath=Join-Path $runDir "$id\result.json"
    $analysisPath=Join-Path $root "analyses\$id-$($item.role).analysis.json"
    foreach($path in @($reportPath,$trialPath,$resultPath,$analysisPath)){if(-not(Test-Path -LiteralPath $path -PathType Leaf)){throw "Missing exact source input: $path"}}

    $report=Read-Json $reportPath
    $trial=Read-Json $trialPath
    $result=Read-Json $resultPath
    $analysis=Read-Json $analysisPath
    $apk=$apks[[string]$item.role]
    Assert-Equal $report.runId $id "$id report identity"
    Assert-Equal $trial.runId $id "$id trial identity"
    Assert-Equal $result.runId $id "$id result identity"
    Assert-Equal $result.status 'complete' "$id completion status"
    Assert-Equal $report.status 'complete' "$id report status"
    Assert-Equal $report.result 'complete' "$id report result"
    Assert-Equal $report.workload $item.workload "$id workload"
    Assert-Equal $trial.sourceCommit $sourceCommit "$id trial source commit"
    Assert-Equal $analysis.sourceCommit $sourceCommit "$id analysis source commit"
    Assert-Equal $trial.installedApkSha256 $apk "$id installed APK identity"
    Assert-Equal $analysis.apkSha256 $apk "$id analysis APK identity"
    Assert-Equal $analysis.buildLabel "primary-hit-$($item.role)" "$id analysis artifact label"
    Assert-Equal $analysis.artifactRole $item.role "$id analysis role"
    Assert-Equal $analysis.runId $id "$id analysis identity"
    Assert-Equal $analysis.integrity 'PASS' "$id strict analysis integrity"
    Assert-Equal $analysis.workload $item.workload "$id analysis workload"

    $evidenceZones=@($report.completedFrameEvidence.zones|Where-Object name -CEQ $item.targetZone)
    Assert-Equal $evidenceZones.Count 1 "$id actual completed-evidence zone count"
    $zone=$evidenceZones[0]
    Assert-Equal $zone.counts.intended $item.frames "$id zone intended frame count"
    Assert-Equal $zone.counts.completed $item.frames "$id zone completed frame count"
    Assert-Equal $zone.counts.cpuAccepted $item.frames "$id zone CPU accepted frame count"
    Assert-Equal $zone.counts.rejected 0 "$id zone rejected count"
    Assert-Equal $zone.counts.cancelled 0 "$id zone cancelled count"
    Assert-Equal $zone.counts.outstanding 0 "$id zone outstanding count"
    Assert-Equal $zone.gpuStatusCounts.denominator $item.frames "$id GPU denominator"
    Assert-Equal $zone.gpuStatusCounts.valid $item.frames "$id valid GPU interval count"
    foreach($status in @('not-ready','compiled-out','unknown','disabled','unsupported','pending','error','missing')){
        Assert-Equal $zone.gpuStatusCounts.$status 0 "$id GPU status $status"
    }
    Assert-Equal @($zone.cpuStages.PSObject.Properties).Count $stageNames.Count "$id CPU stage set size"
    $cpu=[ordered]@{}
    foreach($name in $stageNames){
        $stageProperty=$zone.cpuStages.PSObject.Properties[$name]
        if($null -eq $stageProperty){throw "$id missing completed-zone CPU stage $name"}
        $stage=$stageProperty.Value
        Assert-Equal $stage.sampleCount $item.frames "$id $name sample count"
        foreach($metric in $stage.PSObject.Properties){
            if($metric.Name -ne 'sampleCount' -and [double]$metric.Value -lt 0){throw "$id negative CPU stage value $name.$($metric.Name)"}
        }
        $cpu[$name]=Project-Stats $stage
    }
    $gpu=Project-Stats $zone.gpuRtDurationMs
    Assert-Equal $gpu.sampleCount $item.frames "$id completed-zone GPU sample count"

    $legacyZones=@($report.zones|Where-Object name -CEQ $item.targetZone)
    Assert-Equal $legacyZones.Count 1 "$id legacy timing-only zone count"
    $legacy=$legacyZones[0]
    Assert-Equal $legacy.frames $item.frames "$id legacy zone frame count"
    $analysisZones=@($analysis.zoneGpuCommandBufferTiming|Where-Object zone -CEQ $item.targetZone)
    Assert-Equal $analysisZones.Count 1 "$id strict analysis focus zone count"
    Assert-Near ([double]$analysisZones[0].medianMs) ([double]$gpu.medianMilliseconds) 0.0002 "$id analyzer versus completed-zone GPU median"
    Assert-Near ([double]$analysisZones[0].p95NearestRankMs) ([double]$gpu.p95Milliseconds) 0.0002 "$id analyzer versus completed-zone GPU p95"

    $reportHash=(Get-FileHash -LiteralPath $reportPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $reportBytes=(Get-Item -LiteralPath $reportPath).Length
    $records.Add([ordered]@{
        runId=$id;artifactRole=$item.role;buildLabel=$analysis.buildLabel;workload=$item.workload;zone=$item.targetZone
        sourceCommit=$sourceCommit;installedApkSha256=$apk;reportBytes=$reportBytes;rawBenchmarkJsonSha256=$reportHash
        completedFrames=$zone.counts.completed;gpuValidFrames=$zone.gpuStatusCounts.valid
        legacyZoneCycleMs=[ordered]@{mean=[double]$legacy.averageMs;median=[double]$legacy.medianMs;p95=[double]$legacy.p95Ms}
        completedZoneGpuMs=$gpu;completedZoneCpuStages=$cpu
        parserSelectionNote='Supplement reads report.completedFrameEvidence.zones for completed CPU stages. Frozen parser selected CPU stages from report.zones, a timing-only projection that has no cpuStages property; its null stage fields mean selector mismatch, not missing or zero cost.'
    })
}
Assert-Equal $records.Count 12 'supplementary record count'
$receipt=[ordered]@{
    schema=1;classification='supplementary-completed-zone-cpu-evidence';sourceHead=$sourceCommit
    sourceParserSha256=(Get-FileHash -LiteralPath (Join-Path $root 'analyse-primary-hit-trial.ps1') -Algorithm SHA256).Hash.ToLowerInvariant()
    sourceAggregateSha256=(Get-FileHash -LiteralPath (Join-Path $root 'aggregate-primary-hit-abba.ps1') -Algorithm SHA256).Hash.ToLowerInvariant()
    generatedUtc=[DateTime]::UtcNow.ToString('o');runCount=$records.Count;records=@($records)
    scopeNote='Raw completion-owned zone reports only; no benchmark schema/parser/aggregate rewrite, no ADB or production edits. CPU stage values are host-side record/submit/present measurements as named, not GPU substage costs.'
}
$receipt|ConvertTo-Json -Depth 12|Set-Content -LiteralPath $output -Encoding utf8
Write-Output "Supplementary completed-zone CPU receipt PASS: $($records.Count)/12 runs."
Write-Output "Receipt: $output"
