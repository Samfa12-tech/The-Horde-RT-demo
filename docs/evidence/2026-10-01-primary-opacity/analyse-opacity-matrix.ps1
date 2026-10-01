[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][ValidatePattern('^[0-9a-f]{40}$')][string]$CandidateSourceCommit,
    [string]$ControlPackageAudit = 'C:\Dev\tmp\horde-mobile-lantern-profile-20261001\audit-control-3cb84efb2f71\actual-apk-package-audit.json',
    [string]$CandidatePackageAudit = 'C:\Dev\tmp\horde-primary-opacity-20261001\audit-control-b9d69ff43c13\actual-apk-package-audit.json',
    [string]$PhoneRoot = 'C:\Dev\tmp\horde-primary-opacity-20261001\performance\phone',
    [string]$OutputPath = 'C:\Dev\tmp\horde-primary-opacity-20261001\performance\opacity-matrix-analysis.json'
)
$ErrorActionPreference = 'Stop'

# Fixed identities and row plan are deliberately narrow. Artifact/module hashes are
# read from the supplied actual package-audit receipts, never copied into this adapter.
$controlCommit = '6fa1c53d1f0e4ec3938983f2cad7bd2ece233f4a'
$controlHash = '3cb84efb2f71b1c96b8a562ae6a272569e3c76315053144582a617be75bf30eb'
$candidateHash = 'b9d69ff43c13b0d84ff8fca11132578a27ae710ac2946546677d707655b9a188'
$backend = 'RayTracingPipeline'
$workloads = @('showcase-route-v1','lantern-held-high-v1')
$routeZones = @(
    [ordered]@{name='opening';frames=160}, [ordered]@{name='skeleton-room';frames=98},
    [ordered]@{name='shadow-corridor';frames=599}, [ordered]@{name='skylight-chamber';frames=189},
    [ordered]@{name='yellow-torch-bay';frames=157}, [ordered]@{name='blue-torch-bay';frames=157},
    [ordered]@{name='red-torch-bay';frames=157}, [ordered]@{name='green-torch-bay';frames=157},
    [ordered]@{name='transmission-threshold';frames=63}, [ordered]@{name='finale';frames=101}
)
$cpuStageNames = @('simulationStepCpuMs','skinCpuMs','playerSkinCpuMs','characterSkinCpuMs','dynamicUploadCpuMs','blasRefitRecordCpuMs','tlasUpdateRecordCpuMs','traceCopyRecordCpuMs','frameFenceWaitCpuMs','imageAcquireCpuMs','queueSubmitCpuMs','presentCallCpuMs','wholeFrameCycleCpuMs')

function Assert-Equal($actual, $expected, [string]$label) { if ($actual -cne $expected) { throw "$label expected '$expected', got '$actual'" } }
function Assert-True([bool]$condition, [string]$label) { if (-not $condition) { throw $label } }
function Get-Median([double[]]$values) {
    $s = @($values | Sort-Object); if (-not $s.Count) { throw 'Empty sample set.' }
    $m = [int][Math]::Floor($s.Count / 2)
    if ($s.Count % 2) { return [double]$s[$m] }
    return ([double]$s[$m - 1] + [double]$s[$m]) / 2.0
}
function Get-P95([double[]]$values) {
    $s = @($values | Sort-Object); if (-not $s.Count) { throw 'Empty sample set.' }
    return [double]$s[[int][Math]::Ceiling($s.Count * 0.95) - 1]
}
function Read-OneAudit([string]$path, [string]$apkHash) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Package audit missing: $path" }
    $audit = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
    $a = @($audit.artifacts | Where-Object { $_.apkSha256 -ceq $apkHash })
    if ($a.Count -ne 1) { throw "Audit must contain exactly one artifact matching APK ${apkHash}: $path" }
    if ($a[0].assetEntryCount -ne 70 -or $a[0].assets.Count -ne 70 -or $a[0].modules.Count -ne 4 -or $a[0].sharedStages.Count -ne 2) {
        throw "Actual APK receipt lacks the expected asset/module inventory: $path"
    }
    return [ordered]@{audit=$audit; artifact=$a[0]; receiptSha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()}
}
function Get-Map($rows, [string]$keyName, [string]$valueName) {
    $map = @{}
    foreach ($row in $rows) { $key = [string]$row.$keyName; if ($map.ContainsKey($key)) { throw "Duplicate inventory key $key" }; $map[$key] = [string]$row.$valueName }
    return $map
}
function Get-ArtifactShader($artifact) {
    $map = Get-Map $artifact.modules 'key' 'sha256'
    $opaque = $map['shipping_mobile_opaque_fast']; $generic = $map['shipping_mobile_generic_dielectric']
    if (-not $opaque -or -not $generic) { throw 'RayTracingPipeline module pair absent from actual audit receipt.' }
    return "opaqueFast:shipping_mobile_opaque_fast@$opaque|genericDielectric:shipping_mobile_generic_dielectric@$generic"
}
function Get-Memory([string]$path) {
    $r = [ordered]@{available=$false;totalPssKb=$null;totalRssKb=$null;totalSwapPssKb=$null;nativeHeapPssKb=$null}
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { return $r }
    $t = [regex]::Match([IO.File]::ReadAllText($path),'(?m)^\s*TOTAL PSS:\s*(\d+)\s+TOTAL RSS:\s*(\d+)\s+TOTAL SWAP PSS:\s*(\d+)')
    if ($t.Success) { $r.available=$true;$r.totalPssKb=[long]$t.Groups[1].Value;$r.totalRssKb=[long]$t.Groups[2].Value;$r.totalSwapPssKb=[long]$t.Groups[3].Value }
    $n = [regex]::Match([IO.File]::ReadAllText($path),'(?m)^\s*Native Heap\s+(\d+)\s+')
    if ($n.Success) { $r.nativeHeapPssKb=[long]$n.Groups[1].Value }
    return $r
}

$control = Read-OneAudit $ControlPackageAudit $controlHash
$candidate = Read-OneAudit $CandidatePackageAudit $candidateHash
$controlAssets = Get-Map $control.artifact.assets 'path' 'sha256'
$candidateAssets = Get-Map $candidate.artifact.assets 'path' 'sha256'
if ($controlAssets.Count -ne $candidateAssets.Count) { throw 'Control/candidate packaged asset inventory count differs.' }
$changedAssets = @($controlAssets.Keys | Where-Object { -not $candidateAssets.ContainsKey($_) -or $candidateAssets[$_] -cne $controlAssets[$_] })
if ($changedAssets.Count) { throw "Control/candidate packaged assets differ: $($changedAssets -join ', ')" }
$controlModules = Get-Map $control.artifact.modules 'key' 'sha256'
$candidateModules = Get-Map $candidate.artifact.modules 'key' 'sha256'
if ($controlModules.Count -ne 4 -or $candidateModules.Count -ne 4) { throw 'Shipping/Mobile module inventory is incomplete.' }
$changedModules = @($controlModules.Keys | Where-Object { -not $candidateModules.ContainsKey($_) -or $candidateModules[$_] -cne $controlModules[$_] } | Sort-Object)
$expectedChangedModules = @('rayquery_compute_shipping_mobile_generic_dielectric','rayquery_compute_shipping_mobile_opaque_fast','shipping_mobile_generic_dielectric','shipping_mobile_opaque_fast')
if (($changedModules -join '|') -cne ($expectedChangedModules -join '|')) { throw "Unexpected actual module delta: $($changedModules -join ', ')" }
$controlStages = Get-Map $control.artifact.sharedStages 'executionModel' 'sha256'
$candidateStages = Get-Map $candidate.artifact.sharedStages 'executionModel' 'sha256'
foreach ($k in @('MissKHR','ClosestHitKHR')) { Assert-Equal $candidateStages[$k] $controlStages[$k] "unchanged shared stage $k" }
foreach ($receipt in @($control.artifact,$candidate.artifact)) {
    foreach ($module in $receipt.modules) {
        Assert-Equal $module.spirvVal 'passed' "SPIR-V validation $($module.key)"
        Assert-Equal $module.spirvDis 'passed' "SPIR-V disassembly $($module.key)"
        Assert-Equal $module.atomicsIncludingNoResultOpcodes 0 "atomic instructions $($module.key)"
        Assert-Equal $module.imageReads 0 "image reads $($module.key)"
        Assert-Equal $module.binding22 $false "diagnostic binding $($module.key)"
    }
    foreach ($stage in $receipt.sharedStages) {
        Assert-Equal $stage.spirvVal 'passed' "shared-stage validation $($stage.executionModel)"
        Assert-Equal $stage.spirvDis 'passed' "shared-stage disassembly $($stage.executionModel)"
        Assert-Equal $stage.atomicsIncludingNoResultOpcodes 0 "shared-stage atomics $($stage.executionModel)"
        Assert-Equal $stage.binding22 $false "shared-stage diagnostic binding $($stage.executionModel)"
    }
}
$controlShader = Get-ArtifactShader $control.artifact
$candidateShader = Get-ArtifactShader $candidate.artifact

$rows = [Collections.Generic.List[object]]::new()
$order = @(
    [ordered]@{block='A1';role='normal-control';member='normal-control'},
    [ordered]@{block='B1';role='primary-opacity-candidate';member='primary-opacity-candidate'},
    [ordered]@{block='B2';role='primary-opacity-candidate';member='primary-opacity-candidate'},
    [ordered]@{block='A2';role='normal-control';member='normal-control'}
)
foreach ($block in $order) {
    foreach ($workload in $workloads) {
        $runId = "opacity-$($block.block)-$workload"
        $dir = Join-Path ([IO.Path]::GetFullPath($PhoneRoot)) $runId
        $reportPath = Join-Path (Join-Path $dir $runId) 'benchmark.json'
        $markerPath = Join-Path (Join-Path $dir $runId) 'result.json'
        $trialPath = Join-Path $dir 'trial.json'
        foreach ($p in @($reportPath,$markerPath,$trialPath)) { if (-not (Test-Path -LiteralPath $p -PathType Leaf)) { throw "Missing required retained row artifact: $p" } }
        $trial = Get-Content -LiteralPath $trialPath -Raw | ConvertFrom-Json
        $report = Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json
        $marker = Get-Content -LiteralPath $markerPath -Raw | ConvertFrom-Json
        $expectedSource = if ($block.role -ceq 'normal-control') { $controlCommit } else { $CandidateSourceCommit }
        $expectedApk = if ($block.role -ceq 'normal-control') { $controlHash } else { $candidateHash }
        $expectedShader = if ($block.role -ceq 'normal-control') { $controlShader } else { $candidateShader }
        $expectedCount = if ($workload -ceq 'showcase-route-v1') { 1838 } else { 600 }
        Assert-Equal $trial.runId $runId 'trial run identity'; Assert-Equal $trial.status 'complete' 'trial completion'
        Assert-Equal $trial.sourceCommit $expectedSource 'trial source commit'; Assert-Equal $trial.member $block.member 'harness member identity'
        Assert-Equal $trial.workload $workload 'trial workload'; Assert-Equal $trial.deviceModel 'SM-S948B' 'exact model'
        Assert-Equal $trial.serial 'R5GL219SZGK' 'exact serial'; Assert-Equal $trial.package 'com.samfa12.hordelanternrt.benchmark' 'package'
        Assert-Equal $trial.installedApkSha256 $expectedApk 'full installed APK hash'; Assert-Equal $trial.requestedScalePercent 75 'requested scale'
        Assert-Equal $trial.renderScalePercent 75 'actual scale'; Assert-Equal $trial.requestedBackend $backend 'requested backend'
        Assert-Equal $trial.instrumentation 'Shipping' 'instrumentation'; Assert-Equal $trial.quality 'Mobile' 'quality'
        Assert-Equal $trial.recordingEnabled $false 'recording disabled'; Assert-Equal $trial.appDataCleared $false 'app data preserved'
        Assert-Equal $marker.status 'complete' 'result status'; Assert-Equal $marker.runId $runId 'result identity'
        Assert-Equal $report.status 'complete' 'benchmark status'; Assert-Equal $report.result 'complete' 'benchmark result'
        Assert-Equal $report.workloadComplete $true 'workload completion'; Assert-Equal $report.presentedEveryFrame $true 'present denominator'
        Assert-Equal $report.renderScalePercent 75 'reported scale'; Assert-Equal $report.executionBackend $backend 'actual backend'
        Assert-Equal $report.rtMode 'RayTracingPipeline' 'actual RT mode'; Assert-Equal $report.presentMode 'MAILBOX' 'present mode'
        Assert-Equal $report.materialEncoding 'ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2) + strict ASTC 6x6 lich' 'asset encoding'
        Assert-Equal $report.internalExtent.width 1080 'internal width'; Assert-Equal $report.internalExtent.height 2235 'internal height'
        Assert-Equal $report.presentationExtent.width 1440 'presentation width'; Assert-Equal $report.presentationExtent.height 2980 'presentation height'
        Assert-Equal $report.shader $expectedShader 'actual loaded Shipping/Mobile module identities'
        Assert-Equal $report.measuredFrames $expectedCount 'measured denominator'; Assert-Equal $report.lapsRequested 2 'requested laps'
        Assert-Equal $report.lapsCompleted 2 'completed laps'; Assert-Equal $trial.recordingEnabled $false 'recording disabled'
        $e = $report.completedFrameEvidence
        Assert-Equal $e.status 'complete' 'completion ledger'; Assert-Equal $e.invalidRun $false 'invalid-run marker'
        foreach ($n in @('expected','completed','cpuAccepted')) { Assert-Equal $e.counts.$n $expectedCount "ledger $n count" }
        foreach ($n in @('rejected','cancelled','cpuRejected','outstanding')) { Assert-Equal $e.counts.$n 0 "ledger $n count" }
        Assert-Equal $e.gpuStatusCounts.denominator $expectedCount 'GPU denominator'; Assert-Equal $e.gpuStatusCounts.valid $expectedCount 'GPU valid count'
        foreach ($p in $e.gpuStatusCounts.PSObject.Properties) { if ($p.Name -notin @('denominator','valid')) { Assert-Equal $p.Value 0 "GPU status $($p.Name)" } }
        foreach ($p in $e.failureReasonCounts.PSObject.Properties) { Assert-Equal $p.Value 0 "failure reason $($p.Name)" }
        $expectedStrategy = 'opaque-fast' # Both members use open-aperture geometry and this same selected route.
        $gpuAll = [Collections.Generic.List[double]]::new(); $openingGpu = [Collections.Generic.List[double]]::new()
        $cpuZone = @{}
        $zonePlan = if ($workload -ceq 'showcase-route-v1') { $routeZones } else { @(
            [ordered]@{name='opening';frames=0}, [ordered]@{name='skeleton-room';frames=0},
            [ordered]@{name='shadow-corridor';frames=0}, [ordered]@{name='skylight-chamber';frames=0},
            [ordered]@{name='yellow-torch-bay';frames=600}, [ordered]@{name='blue-torch-bay';frames=0},
            [ordered]@{name='red-torch-bay';frames=0}, [ordered]@{name='green-torch-bay';frames=0},
            [ordered]@{name='transmission-threshold';frames=0}, [ordered]@{name='finale';frames=0}) }
        $expectedZones = @{}
        foreach ($z in $zonePlan) { $expectedZones[$z.name] = [int]$z.frames; $cpuZone[$z.name] = 0 }
        Assert-Equal $report.zones.Count $zonePlan.Count 'zone inventory count'
        foreach ($z in $zonePlan) {
            $rz = @($report.zones | Where-Object { $_.name -ceq $z.name })
            if ($rz.Count -ne 1) { throw "Missing/duplicate zone $($z.name) in $runId" }
            Assert-Equal $rz[0].frames $z.frames "zone frames $($z.name)"
        }
        $lastSerial = 0L
        for ($i=0; $i -lt $expectedCount; $i++) {
            $r = $e.rows[$i]
            Assert-Equal $r.index $i "row index $i"; Assert-Equal $r.cpuSampleIndex $i "CPU join $i"
            Assert-Equal $r.lap 2 "measured lap $i"; Assert-Equal $r.disposition 'completed' "completion $i"
            Assert-Equal $r.failure 'none' "failure $i"; Assert-Equal $r.presentationOutcome 'presented' "presentation $i"
            Assert-Equal $r.cpuStageStatus 'valid' "CPU stages $i"; Assert-Equal $r.diagnosticStatus 'compiled-out' "diagnostic state $i"
            Assert-Equal $r.diagnosticCounters $null "diagnostic counters $i"; Assert-Equal $r.gpuStatus 'valid' "GPU status $i"
            Assert-Equal $r.cpuAccepted $true "CPU admission $i"; Assert-Equal $r.activeStrategy $expectedStrategy "active strategy $i"
            $zoneName = if ($workload -ceq 'showcase-route-v1') {
                $n=$i; $chosen=$null; foreach ($z in $routeZones) { if ($n -lt $z.frames) { $chosen=$z.name; break }; $n -= $z.frames }; $chosen
            } else { 'yellow-torch-bay' }
            Assert-Equal $r.zoneName $zoneName "named zone $i"
            foreach ($name in @('sceneEpoch','measurementGeneration','recordAttemptSerial','recordSerial','simulationTick','frameSlot','submissionSerial')) {
                Assert-True ($null -ne $r.submittedIdentity.$name -and $null -ne $r.completionIdentity.$name) "missing frame identity $name row $i"
                Assert-Equal $r.submittedIdentity.$name $r.completionIdentity.$name "frame join $name row $i"
            }
            Assert-Equal $r.submittedIdentity.frameSlot 0 "single in-flight slot $i"
            $serialValue=[long]$r.completionIdentity.completionSerial
            if ($serialValue -le $lastSerial) { throw "Non-monotonic completion serial in $runId at row $i" }; $lastSerial=$serialValue
            $ms=[double]$r.gpuDurationNanoseconds/1000000.0
            if ($ms -le 0 -or [double]::IsNaN($ms) -or [double]::IsInfinity($ms)) { throw "Invalid GPU duration in $runId row $i" }
            $gpuAll.Add($ms); if ($workload -ceq 'showcase-route-v1' -and $i -lt 160) { $openingGpu.Add($ms) }
            if (-not $cpuZone.ContainsKey($r.zoneName)) { $cpuZone[$r.zoneName]=0 }; $cpuZone[$r.zoneName]++
        }
        if ($workload -ceq 'showcase-route-v1') {
            Assert-Equal $report.routeTraversalComplete $true 'route traversal'; Assert-Equal $report.waypointsReached 26 'waypoint count'
            Assert-Equal $openingGpu.Count 160 'opening slice GPU denominator'
        } else { Assert-Equal $report.routeTraversalComplete $false 'non-route marker' }
        foreach ($n in $zonePlan) { Assert-Equal ([int]$cpuZone[$n.name]) $n.frames "observed zone denominator $($n.name)" }
        $cpuStages=[ordered]@{}
        foreach ($name in $cpuStageNames) {
            $st=$e.cpuStages.PSObject.Properties[$name].Value
            if ($null -eq $st) { throw "Missing CPU stage inventory '$name' in $runId" }
            Assert-Equal $st.sampleCount $expectedCount "CPU stage sample count $name"
            $cpuStages[$name]=[ordered]@{sampleCount=$st.sampleCount;meanMs=$st.meanMilliseconds;medianMs=$st.medianMilliseconds;p95Ms=$st.p95Milliseconds;slowestOnePercentMeanMs=$st.slowestOnePercentMeanMilliseconds}
        }
        if ([Math]::Abs([double]$e.cpuStages.wholeFrameCycleCpuMs.medianMilliseconds-[double]$report.overall.medianMs)-gt 0.002 -or
            [Math]::Abs([double]$e.cpuStages.wholeFrameCycleCpuMs.p95Milliseconds-[double]$report.overall.p95Ms)-gt 0.002) { throw "Native full-cycle summary mismatch: $runId" }
        $openingSlice = $null
        if ($workload -ceq 'showcase-route-v1') {
            $nativeOpening = @($report.zones | Where-Object { $_.name -ceq 'opening' })
            $ledgerOpening = @($e.zones | Where-Object { $_.name -ceq 'opening' })
            if ($nativeOpening.Count -ne 1 -or $ledgerOpening.Count -ne 1) { throw "Opening160 summaries missing in $runId" }
            Assert-Equal $nativeOpening[0].frames 160 'opening160 native denominator'
            Assert-Equal $ledgerOpening[0].counts.completed 160 'opening160 completion denominator'
            Assert-Equal $ledgerOpening[0].gpuStatusCounts.valid 160 'opening160 GPU denominator'
            $openingCpuStages = [ordered]@{}
            foreach ($name in $cpuStageNames) {
                $stage = $ledgerOpening[0].cpuStages.PSObject.Properties[$name].Value
                if ($null -eq $stage) { throw "Opening CPU stage '$name' missing in $runId" }
                Assert-Equal $stage.sampleCount 160 "opening CPU stage count $name"
                $openingCpuStages[$name] = [ordered]@{sampleCount=$stage.sampleCount;meanMs=$stage.meanMilliseconds;medianMs=$stage.medianMilliseconds;p95Ms=$stage.p95Milliseconds;slowestOnePercentMeanMs=$stage.slowestOnePercentMeanMilliseconds}
            }
            $openingSlice = [ordered]@{
                denominator=160
                nativeCycle=[ordered]@{scope='android-render-entry-through-present';medianMs=$nativeOpening[0].medianMs;p95Ms=$nativeOpening[0].p95Ms;displayPacingProof=$false}
                gpuRt=[ordered]@{sampleCount=$openingGpu.Count;medianMs=[Math]::Round((Get-Median $openingGpu.ToArray()),4);p95NearestRankMs=[Math]::Round((Get-P95 $openingGpu.ToArray()),4);includesAsBuildUpdateRtCopy=$true}
                cpuStageInventory=$openingCpuStages
            }
        }
        $ctx=@(); $ctxPath=Join-Path $dir 'context-samples.jsonl'
        if (Test-Path -LiteralPath $ctxPath -PathType Leaf) { $ctx=@(Get-Content -LiteralPath $ctxPath | Where-Object {$_} | ForEach-Object {$_|ConvertFrom-Json}) }
        $thermal=[ordered]@{sampleCount=$ctx.Count;firstUtc=$null;lastUtc=$null;batteryStartC=$null;batteryEndC=$null;thermalStatuses=@();gpuThermalPowerLevels=@()}
        if ($ctx.Count) { $thermal.firstUtc=$ctx[0].utc;$thermal.lastUtc=$ctx[-1].utc;$thermal.batteryStartC=$ctx[0].batteryC;$thermal.batteryEndC=$ctx[-1].batteryC;$thermal.thermalStatuses=@($ctx.thermalStatus|Where-Object {$null-ne $_}|Sort-Object -Unique);$thermal.gpuThermalPowerLevels=@($ctx.gpuThermalPowerLevel|Where-Object {$null-ne $_}|Sort-Object -Unique) }
        $rows.Add([ordered]@{
            order=$block.block;role=$block.role;runId=$runId;workload=$workload;sourceCommit=$trial.sourceCommit;apkSha256=$trial.installedApkSha256
            actualShaderIdentity=$report.shader;scalePercent=$report.renderScalePercent;internalExtent=$report.internalExtent;presentationExtent=$report.presentationExtent
            denominator=[ordered]@{expected=$expectedCount;completed=$e.counts.completed;presented=$expectedCount;gpuValid=$gpuAll.Count;diagnostic='compiled-out'}
            nativeCycle=[ordered]@{scope='android-render-entry-through-present';sampleCount=$expectedCount;medianMs=$report.overall.medianMs;p95Ms=$report.overall.p95Ms;displayPacingProof=$false}
            gpuRt=[ordered]@{sampleCount=$gpuAll.Count;medianMs=[Math]::Round((Get-Median $gpuAll.ToArray()),4);p95NearestRankMs=[Math]::Round((Get-P95 $gpuAll.ToArray()),4);includesAsBuildUpdateRtCopy=$true}
            opening160=$openingSlice
            cpuStageInventory=$cpuStages;thermalContext=$thermal;ramBefore=(Get-Memory (Join-Path $dir 'memory-before.txt'));ramAfter=(Get-Memory (Join-Path $dir 'memory-after.txt'))
            ramActive=(Get-Memory (Join-Path $dir 'memory-active.txt'));activeRamScope='one timed snapshot; not frame aligned; pressure/system-memory raw files retained separately'
        })
    }
}

if (Test-Path -LiteralPath $OutputPath) { throw "Refusing to overwrite existing analysis: $OutputPath" }
$comparison=@()
foreach ($workload in $workloads) {
    $c=@($rows|Where-Object {$_.workload -ceq $workload -and $_.role -ceq 'normal-control'})
    $b=@($rows|Where-Object {$_.workload -ceq $workload -and $_.role -ceq 'primary-opacity-candidate'})
    if ($c.Count -ne 2 -or $b.Count -ne 2) { throw "ABBA row count incomplete for $workload" }
    $comparison += [ordered]@{workload=$workload;controlRows=@($c.runId);candidateRows=@($b.runId);sameQualityBackendScale=$true;performanceDecision='none'}
}
$analysis=[ordered]@{
    schema=1;status='integrity-only';candidateSourceCommit=$CandidateSourceCommit;controlSourceCommit=$controlCommit
    device=[ordered]@{model='SM-S948B';serial='R5GL219SZGK';backend=$backend;scalePercent=75;quality='Mobile';instrumentation='Shipping'}
    workloadPlan='A1/B1/B2/A2; showcase-route-v1 (1838 measured rows, opening160 separately) and lantern-held-high-v1 (600 measured rows) per member; two laps with measured lap 2 only.'
    packageAudits=[ordered]@{controlPath=$ControlPackageAudit;controlReceiptSha256=$control.receiptSha256;candidatePath=$CandidatePackageAudit;candidateReceiptSha256=$candidate.receiptSha256;identicalAssetEntries=$controlAssets.Count;changedAssetEntries=0;changedModuleKeys=$changedModules;controlShader=$controlShader;candidateShader=$candidateShader}
    rows=$rows.ToArray();matchedWorkloads=$comparison
    interpretation=[ordered]@{performanceAcceptance='none';nativeCycleMeaning='render-entry through present-call CPU cycle; median-derived FPS is not display pacing';gpuMeaning='owning submitted/completed command-buffer GPU duration includes AS build/update, RT and copy, not isolated shader time';ramMeaning='before/after values are lifecycle-boundary snapshots, not frame-aligned; benchmark completion hides SurfaceView and destroys RT scene';openingSlice='first 160 measured lap-2 opening rows in showcase-route-v1; per-zone native, CPU-stage, and GPU summaries are reported';diagnostics='Shipping counters are compiled out and null; device GPU memory counters are not collected and do not imply success';decision='No before/after pass, FPS claim, bandwidth claim, or optimization admission is produced.'}
}
$analysis | ConvertTo-Json -Depth 14 | Set-Content -LiteralPath $OutputPath -Encoding utf8NoBOM
Write-Output "Opacity matrix integrity analysis written: $OutputPath (8 row slots; performance acceptance remains none)."
