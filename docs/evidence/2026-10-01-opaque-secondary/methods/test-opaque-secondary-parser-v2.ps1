param()
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
$fixtureRoot=Join-Path $root 'parser-fixtures\opaque-secondary-negative-v2d'
if(Test-Path -LiteralPath $fixtureRoot){throw "Refusing existing fixture root: $fixtureRoot"}
$null=New-Item -ItemType Directory -Path $fixtureRoot
$harness=Join-Path $fixtureRoot 'harness'
$null=New-Item -ItemType Directory -Path (Join-Path $harness 'artifacts\control') -Force
$null=New-Item -ItemType Directory -Path (Join-Path $harness 'artifacts\revised-candidate') -Force
$null=New-Item -ItemType Directory -Path (Join-Path $harness 'logs') -Force
$analyzer=Join-Path $harness 'analyse-opaque-secondary-trial-v2.ps1'
Copy-Item -LiteralPath (Join-Path $root 'analyse-opaque-secondary-trial-v2.ps1') -Destination $analyzer
Copy-Item -LiteralPath (Join-Path $root 'aggregate-opaque-secondary-abba-v2.ps1') -Destination (Join-Path $harness 'aggregate-opaque-secondary-abba-v2.ps1')
Copy-Item -LiteralPath (Join-Path $root 'build-receipt.json') -Destination (Join-Path $harness 'build-receipt.json')
Copy-Item -LiteralPath (Join-Path $root 'artifacts\control\raygen-variant-catalog.json') -Destination (Join-Path $harness 'artifacts\control\raygen-variant-catalog.json')
Copy-Item -LiteralPath (Join-Path $root 'artifacts\control\rayquery-variant-catalog.json') -Destination (Join-Path $harness 'artifacts\control\rayquery-variant-catalog.json')
Copy-Item -LiteralPath (Join-Path $root 'artifacts\revised-candidate\raygen-variant-catalog.json') -Destination (Join-Path $harness 'artifacts\revised-candidate\raygen-variant-catalog.json')
Copy-Item -LiteralPath (Join-Path $root 'artifacts\revised-candidate\rayquery-variant-catalog.json') -Destination (Join-Path $harness 'artifacts\revised-candidate\rayquery-variant-catalog.json')
Copy-Item -LiteralPath (Join-Path $root 'artifacts\revised-candidate\build-receipt.json') -Destination (Join-Path $harness 'artifacts\revised-candidate\build-receipt.json')
Copy-Item -LiteralPath (Join-Path $root 'logs\apk-assets-module-comparison-02.json') -Destination (Join-Path $harness 'logs\apk-assets-module-comparison-02.json')
Copy-Item -LiteralPath (Join-Path $root 'logs\candidate-package-containment-02.json') -Destination (Join-Path $harness 'logs\candidate-package-containment-02.json')
$source=Join-Path $root 'phone\opaque-secondary-c1-route-20261001'
$runId='opaque-secondary-c1-route-20261001'
function Write-Json([string]$Path,$Value){[IO.File]::WriteAllText($Path,($Value|ConvertTo-Json -Depth 100),[Text.UTF8Encoding]::new($false))}
function New-Fixture([string]$Name){
    $dir=Join-Path $fixtureRoot $Name
    $nested=Join-Path $dir $runId
    $null=New-Item -ItemType Directory -Path $nested -Force
    Copy-Item -LiteralPath (Join-Path $source 'trial.json') -Destination (Join-Path $dir 'trial.json')
    Copy-Item -LiteralPath (Join-Path $source 'context-samples.jsonl') -Destination (Join-Path $dir 'context-samples.jsonl')
    Copy-Item -LiteralPath (Join-Path $source "$runId\benchmark.json") -Destination (Join-Path $nested 'benchmark.json')
    Copy-Item -LiteralPath (Join-Path $source "$runId\result.json") -Destination (Join-Path $nested 'result.json')
    return [ordered]@{directory=$dir;trial=Join-Path $dir 'trial.json';report=Join-Path $nested 'benchmark.json'}
}
function Expect-Reject([string]$Name,[string]$Diagnostic){
    $actual='';$accepted=$false
    try{& $analyzer -RunPath $script:case.directory -ArtifactRole control 2>&1|Out-Null;$accepted=$true}catch{$actual=$_.Exception.Message}
    if($accepted){throw "$Name unexpectedly accepted"}
    if($actual -notmatch [regex]::Escape($Diagnostic)){throw "$Name rejected for wrong reason: $actual"}
    return [ordered]@{name=$Name;status='expected-reject';diagnostic=$actual.Trim()}
}
$results=[Collections.Generic.List[object]]::new()
$case=New-Fixture 'forged-apk';$t=Get-Content $case.trial -Raw|ConvertFrom-Json;$t.installedApkSha256=('0'*64);Write-Json $case.trial $t
$results.Add((Expect-Reject 'forged-installed-apk-sha' 'installed APK versus exact opaque-secondary artifact'))
$case=New-Fixture 'forged-shader';$r=Get-Content $case.report -Raw|ConvertFrom-Json;$r.shader='opaqueFast:shipping_mobile_opaque_fast@'+('0'*64);Write-Json $case.report $r
$results.Add((Expect-Reject 'forged-loaded-shader-sha' 'loaded artifact pair versus actual APK shader identity'))
$case=New-Fixture 'bad-completion-identity';$r=Get-Content $case.report -Raw|ConvertFrom-Json;$r.completedFrameEvidence.rows[0].completionIdentity.submissionSerial=[long]$r.completedFrameEvidence.rows[0].submittedIdentity.submissionSerial+1;Write-Json $case.report $r
$results.Add((Expect-Reject 'mismatched-completion-identity' 'identity submissionSerial at row 0'))
$case=New-Fixture 'bad-opening-strategy';$r=Get-Content $case.report -Raw|ConvertFrom-Json;$row=@($r.completedFrameEvidence.rows|Where-Object zoneName -CEQ 'opening'|Select-Object -First 1)[0];$row.activeStrategy='generic-dielectric';Write-Json $case.report $r
$results.Add((Expect-Reject 'invalid-opening-strategy' 'actual opening OpaqueFast completions'))
$case=New-Fixture 'diagnostics-present';$r=Get-Content $case.report -Raw|ConvertFrom-Json;$r.completedFrameEvidence.rows[0].diagnosticStatus='available';Write-Json $case.report $r
$results.Add((Expect-Reject 'noncompiled-out-diagnostic' 'diagnostic status'))
$case=New-Fixture 'missing-cpu-stage';$r=Get-Content $case.report -Raw|ConvertFrom-Json;$r.completedFrameEvidence.cpuStages.PSObject.Properties.Remove('presentCallCpuMs');Write-Json $case.report $r
$results.Add((Expect-Reject 'missing-cpu-stage' 'CPU stage keys missing or unexpected'))
$case=New-Fixture 'nonfinite-cpu-timing';$r=Get-Content $case.report -Raw|ConvertFrom-Json;$r.completedFrameEvidence.cpuStages.presentCallCpuMs.medianMilliseconds='NaN';Write-Json $case.report $r
$results.Add((Expect-Reject 'nonfinite-cpu-timing' 'CPU timing value is nonfinite/negative'))
$case=New-Fixture 'missing-cpu-stat';$r=Get-Content $case.report -Raw|ConvertFrom-Json;$r.completedFrameEvidence.cpuStages.presentCallCpuMs.PSObject.Properties.Remove('meanMilliseconds');Write-Json $case.report $r
$results.Add((Expect-Reject 'missing-cpu-statistic' 'CPU timing statistic keys missing or unexpected'))
$case=New-Fixture 'wrong-row-zone';$r=Get-Content $case.report -Raw|ConvertFrom-Json;$r.completedFrameEvidence.rows[0].zoneName='finale';Write-Json $case.report $r
$results.Add((Expect-Reject 'owning-row-zone-count-drift' 'owning-row zone count opening'))
$case=New-Fixture 'wrong-trial-status';$t=Get-Content $case.trial -Raw|ConvertFrom-Json;$t.status='failed';Write-Json $case.trial $t
$results.Add((Expect-Reject 'mismatched-trial-status' 'trial status'))
$case=New-Fixture 'wrong-context-pid';$lines=Get-Content -LiteralPath (Join-Path $case.directory 'context-samples.jsonl');$first=$lines[0]|ConvertFrom-Json;$first.processId='99999';$lines[0]=($first|ConvertTo-Json -Compress);Set-Content -LiteralPath (Join-Path $case.directory 'context-samples.jsonl') -Value $lines -Encoding utf8
$results.Add((Expect-Reject 'mismatched-context-process' 'context process ID versus trial process'))
$case=New-Fixture 'invalid-thermal-state';$lines=Get-Content -LiteralPath (Join-Path $case.directory 'context-samples.jsonl');$first=$lines[0]|ConvertFrom-Json;$first.gpuThermalPowerLevel=8;$lines[0]=($first|ConvertTo-Json -Compress);Set-Content -LiteralPath (Join-Path $case.directory 'context-samples.jsonl') -Value $lines -Encoding utf8
$results.Add((Expect-Reject 'invalid-gpu-thermal-power-state' 'Context gpuThermalPowerLevel is not a valid nullable state'))
$aggregateOutput=Join-Path $fixtureRoot 'must-not-exist-partial-aggregate.json'
$aggregateOutputCreated=$false;$aggregateError=''
try{& (Join-Path $harness 'aggregate-opaque-secondary-abba-v2.ps1') -OutputPath $aggregateOutput 2>&1|Out-Null;$aggregateOutputCreated=Test-Path -LiteralPath $aggregateOutput}catch{$aggregateError=$_.Exception.Message}
if($aggregateOutputCreated -or $aggregateError -notmatch 'ABBA not ready'){throw "Incomplete aggregate fail-closed test failed: $aggregateError"}
$results.Add([ordered]@{name='aggregate-waits-for-all-eight';status='expected-reject';diagnostic=$aggregateError.Trim();outputCreated=$aggregateOutputCreated})
$receipt=[ordered]@{schema=2;parserVersion=2;syntheticRegressionOnly=$true;phoneEvidence=$false;source='isolated copies of completed C1 route report; actual run files unchanged';testCount=$results.Count;expectedRejections=$results.Count;results=@($results);limitations=@('Synthetic negatives only verify parser refusal paths. They are not additional benchmark runs or performance evidence.','The positive parser results are separate strict version-2 analyses of the actual completed C1 route/live runs.')}
$receiptPath=Join-Path $fixtureRoot 'negative-test-receipt.json'
Write-Json $receiptPath $receipt
Write-Output "Opaque-secondary parser negative tests PASS: $($results.Count)/$($results.Count) expected rejects; receipt $receiptPath"
