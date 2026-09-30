param()
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
$fixtureRoot=Join-Path $root 'parser-fixtures\opaque-secondary-negative-v1'
if(Test-Path -LiteralPath $fixtureRoot){throw "Refusing existing fixture root: $fixtureRoot"}
$null=New-Item -ItemType Directory -Path $fixtureRoot
$source=Join-Path $root 'phone\opaque-secondary-c1-route-20261001'
$runId='opaque-secondary-c1-route-20261001'
$analyzer=Join-Path $root 'analyse-opaque-secondary-trial.ps1'
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
$aggregateOutput=Join-Path $fixtureRoot 'must-not-exist-partial-aggregate.json'
$aggregateOutputCreated=$false;$aggregateError=''
try{& (Join-Path $root 'aggregate-opaque-secondary-abba.ps1') -OutputPath $aggregateOutput 2>&1|Out-Null;$aggregateOutputCreated=Test-Path -LiteralPath $aggregateOutput}catch{$aggregateError=$_.Exception.Message}
if($aggregateOutputCreated -or $aggregateError -notmatch 'ABBA not ready'){throw "Incomplete aggregate fail-closed test failed: $aggregateError"}
$results.Add([ordered]@{name='aggregate-waits-for-all-eight';status='expected-reject';diagnostic=$aggregateError.Trim();outputCreated=$aggregateOutputCreated})
$receipt=[ordered]@{schema=1;syntheticRegressionOnly=$true;phoneEvidence=$false;source='isolated copies of completed C1 route report; actual run files unchanged';testCount=$results.Count;expectedRejections=$results.Count;results=@($results);limitations=@('Synthetic negatives only verify parser refusal paths. They are not additional benchmark runs or performance evidence.','The positive parser result is the separate strict analysis of the actual completed C1 route/live runs.')}
$receiptPath=Join-Path $fixtureRoot 'negative-test-receipt.json'
Write-Json $receiptPath $receipt
Write-Output "Opaque-secondary parser negative tests PASS: $($results.Count)/$($results.Count) expected rejects; receipt $receiptPath"
