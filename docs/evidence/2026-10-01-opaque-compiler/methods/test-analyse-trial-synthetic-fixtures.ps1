[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath($PSScriptRoot)
$fixtureRoot=Join-Path $root 'phone\fixtures-v2'
if(Test-Path -LiteralPath $fixtureRoot){throw "Refusing to overwrite existing synthetic fixture tree: $fixtureRoot"}
$null=New-Item -ItemType Directory -Path $fixtureRoot
$analyzer=Join-Path $root 'analyse-trial.ps1'
$output=@()
$zonePlan=@(
    @{name='opening';frames=160},@{name='skeleton-room';frames=98},@{name='shadow-corridor';frames=599},
    @{name='skylight-chamber';frames=189},@{name='yellow-torch-bay';frames=157},@{name='blue-torch-bay';frames=157},
    @{name='red-torch-bay';frames=157},@{name='green-torch-bay';frames=157},
    @{name='transmission-threshold';frames=63},@{name='finale';frames=101})
$zones=@($zonePlan | ForEach-Object {[pscustomobject]@{name=$_.name;frames=$_.frames}})
function Write-Json([string]$Path,$Value){$parent=Split-Path -Parent $Path;if(-not(Test-Path -LiteralPath $parent)){New-Item -ItemType Directory -Path $parent -Force|Out-Null};[IO.File]::WriteAllText($Path,(ConvertTo-Json -InputObject $Value -Depth 20)+"`n",[Text.UTF8Encoding]::new($false))}
function New-Fixture([string]$Case,[string]$BuildLabel,[bool]$BadApk=$false,[bool]$BadStrategy=$false,[bool]$MissingIdentity=$false,[bool]$BadShader=$false){
    $runId="fixtures-v2/$Case"
    $directory=Join-Path $root "phone\$runId"
    $nested=Join-Path $directory $runId
    $apkSha=if($BuildLabel -ceq 'control'){'ab3e2261fd081f87e667a6e4967e2476077fa96554702330e6aa49baa8133eae'}else{'4827a3c2e26d3328ed53608e15c77afe12a34077e96dcd0564c8d65532e21ff4'}
    if($BadApk){$apkSha='0'*64}
    $opaqueSha=if($BuildLabel -ceq 'control'){'66e39df9f53b058fb62cbfa913d424b161c93be4aff59a1685cf8b7e54bb9c4b'}else{'14509c272fa4fa9f92a8d180abf170b2485447798b7940ec98f55150c05249d3'}
    $genericSha='ce2302811cb2cb8bbff706fd54cd7f48705e4a5e8b2cda744a7d999574f84532'
    if($BadShader){$opaqueSha='f'*64}
    $shader="opaqueFast:shipping_mobile_opaque_fast@$opaqueSha|genericDielectric:shipping_mobile_generic_dielectric@$genericSha"
    $rows=[Collections.Generic.List[object]]::new();$zoneIndex=0;$zoneRemaining=$zonePlan[0].frames
    for($i=0;$i -lt 1838;$i++){
        while($zoneRemaining -le 0){$zoneIndex++;$zoneRemaining=$zonePlan[$zoneIndex].frames}
        $identity=[ordered]@{sceneEpoch=1;measurementGeneration=1;recordAttemptSerial=($i+1);recordSerial=($i+1);simulationTick=($i+1);frameSlot=0;submissionSerial=($i+1);completionSerial=($i+1)}
        $completion=[ordered]@{sceneEpoch=1;measurementGeneration=1;recordAttemptSerial=($i+1);recordSerial=($i+1);simulationTick=($i+1);frameSlot=0;submissionSerial=($i+1);completionSerial=($i+1)}
        $row=[ordered]@{index=$i;cpuSampleIndex=$i;lap=2;disposition='completed';failure='none';presentationOutcome='presented';cpuStageStatus='valid';diagnosticStatus='compiled-out';diagnosticCounters=$null;gpuStatus='valid';cpuAccepted=$true;gpuDurationNanoseconds=1000000;zoneName=$zonePlan[$zoneIndex].name;activeStrategy='opaque-fast';submittedIdentity=$identity;completionIdentity=$completion}
        if($BadStrategy -and $i -eq 0){$row.activeStrategy='generic-dielectric'}
        if($MissingIdentity -and $i -eq 0){$row.completionIdentity.sceneEpoch=$null}
        $rows.Add([pscustomobject]$row);$zoneRemaining--
    }
    $evidence=[ordered]@{status='complete';invalidRun=$false;counts=[ordered]@{expected=1838;completed=1838;cpuAccepted=1838;rejected=0;cancelled=0;cpuRejected=0;outstanding=0};failureReasonCounts=[ordered]@{};rows=$rows.ToArray();gpuStatusCounts=[ordered]@{valid=1838;denominator=1838};gpuRtDurationMs=[ordered]@{medianMilliseconds=1.0};cpuStages=[ordered]@{totalSamples=1838}}
    $report=[ordered]@{runId=$runId;status='complete';result='complete';workload='showcase-route-v1';workloadComplete=$true;presentedEveryFrame=$true;renderScalePercent=75;executionBackend='RayTracingPipeline';rtMode='RayTracingPipeline';presentMode='MAILBOX';internalExtent=[ordered]@{width=1080;height=2235};presentationExtent=[ordered]@{width=1440;height=2980};materialEncoding='ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2) + strict ASTC 6x6 lich';legacyFrameTimingScope='android-render-entry-through-present';gpu='Adreno (TM) 840';shader=$shader;measuredFrames=1838;routeTraversalComplete=$true;waypointsReached=26;zones=$zones;overall=[ordered]@{medianMs=20.0;p95Ms=25.0};completedFrameEvidence=$evidence}
    $trial=[ordered]@{workload='showcase-route-v1';deviceModel='SM-S948B';instrumentation='Shipping';quality='Mobile';sourceCommit='eafbf8262442a82e0835edf5cf5306d718e64633';buildLabel=$BuildLabel;installedApkSha256=$apkSha}
    $marker=[ordered]@{runId=$runId;status='complete'}
    Write-Json (Join-Path $directory 'trial.json') $trial
    Write-Json (Join-Path $nested 'benchmark.json') $report
    Write-Json (Join-Path $nested 'result.json') $marker
    $context=@([ordered]@{utc='2026-10-01T00:00:00Z';batteryC=32.0;thermalStatus=0;gpuThermalPowerLevel=0},[ordered]@{utc='2026-10-01T00:01:00Z';batteryC=32.1;thermalStatus=0;gpuThermalPowerLevel=0})
    [IO.File]::WriteAllLines((Join-Path $directory 'context-samples.jsonl'),@($context|ForEach-Object {ConvertTo-Json -InputObject $_ -Compress}),[Text.UTF8Encoding]::new($false))
    return $runId
}
function Invoke-Expected([string]$Case,[string]$BuildLabel,[string]$ExpectedError){
    $runId=New-Fixture $Case $BuildLabel ($Case -eq 'wrong-apk-sha') ($Case -eq 'wrong-strategy') ($Case -eq 'missing-completion-identity') ($Case -eq 'wrong-loaded-module-sha')
    try { & $analyzer -RunId $runId *> $null; throw "Expected strict parser rejection for $Case." }
    catch { if($_.Exception.Message -notlike "*$ExpectedError*"){throw};$script:output+=[pscustomobject]@{fixture=$Case;result='expected rejection';diagnostic=$_.Exception.Message} }
}
foreach($pair in @(@('green-control','control'),@('green-candidate','opaque-retained-profile'))){
    $runId=New-Fixture $pair[0] $pair[1]
    try { $message=& $analyzer -RunId $runId 2>&1 | Out-String;if($LASTEXITCODE -and $LASTEXITCODE -ne 0){throw $message};$output+=[pscustomobject]@{fixture=$pair[0];result='PASS';diagnostic=$message.Trim()} }
    catch {throw "Synthetic green fixture $($pair[0]) failed: $($_.Exception.Message)"}
}
Invoke-Expected 'wrong-apk-sha' 'opaque-retained-profile' 'installed APK versus immutable build receipt'
Invoke-Expected 'wrong-loaded-module-sha' 'opaque-retained-profile' 'loaded artifact pair versus actual APK shader identity'
Invoke-Expected 'wrong-strategy' 'opaque-retained-profile' 'all rows selected opaque-fast'
Invoke-Expected 'missing-completion-identity' 'opaque-retained-profile' 'Missing identity sceneEpoch'
$receipt=[ordered]@{schema=1;classification='synthetic analyzer fixtures only; no measured/physical data';analyzerPath=$analyzer;tests=$output}
$receiptPath=Join-Path $fixtureRoot 'synthetic-test-receipt.json'
Write-Json $receiptPath $receipt
Get-FileHash -LiteralPath $receiptPath -Algorithm SHA256 | Select-Object Path,Hash
Write-Output "Synthetic parser tests: $($output.Count) passed; no actual trial reports were read or modified."
