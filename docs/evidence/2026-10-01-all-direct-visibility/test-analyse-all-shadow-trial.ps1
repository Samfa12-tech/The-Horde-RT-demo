param()
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
$fixtureRoot=Join-Path $root 'parser-fixtures'
$analysisDirectory=Join-Path $root 'analyses'
if(Test-Path -LiteralPath $fixtureRoot){throw "Refusing existing fixture root: $fixtureRoot"}
if(Test-Path -LiteralPath $analysisDirectory){throw "Refusing existing analysis output: $analysisDirectory"}
$null=New-Item -ItemType Directory -Path $fixtureRoot
$analyzer=Join-Path $root 'analyse-all-shadow-trial.ps1'
$historicalRoot='C:\Dev\tmp\horde-shipping-ab-20260930\phone'
$controlRun='shipping-active-strategy-route-20260930'
$liveRun='shipping-a1-live-20260930'
$controlApk='a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033'
$controlOpaque='66e39df9f53b058fb62cbfa913d424b161c93be4aff59a1685cf8b7e54bb9c4b'
$controlGeneric='ce2302811cb2cb8bbff706fd54cd7f48705e4a5e8b2cda744a7d999574f84532'
$shaderPair="opaqueFast:shipping_mobile_opaque_fast@$controlOpaque|genericDielectric:shipping_mobile_generic_dielectric@$controlGeneric"

function Write-Json([string]$Path,$Value){[IO.File]::WriteAllText($Path,($Value | ConvertTo-Json -Depth 100))}
function New-SyntheticFixture([string]$SourceRun,[string]$SyntheticId,[string]$Workload){
    $source=Join-Path $historicalRoot $SourceRun
    $destination=Join-Path $fixtureRoot $SyntheticId
    $nested=Join-Path $destination $SyntheticId
    $null=New-Item -ItemType Directory -Path $nested -Force
    foreach($name in @('trial.json','context-samples.jsonl')){Copy-Item -LiteralPath (Join-Path $source $name) -Destination (Join-Path $destination $name)}
    foreach($name in @('benchmark.json','result.json')){Copy-Item -LiteralPath (Join-Path $source "$SourceRun/$name") -Destination (Join-Path $nested $name)}
    $trial=Get-Content -LiteralPath (Join-Path $destination 'trial.json') -Raw|ConvertFrom-Json
    $report=Get-Content -LiteralPath (Join-Path $nested 'benchmark.json') -Raw|ConvertFrom-Json
    $marker=Get-Content -LiteralPath (Join-Path $nested 'result.json') -Raw|ConvertFrom-Json
    $trial.runId=$SyntheticId; $trial.buildLabel='all-shadow-control'; $trial.sourceCommit='71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95'; $trial.installedApkSha256=$controlApk; $trial.workload=$Workload
    $report.runId=$SyntheticId; $report.workload=$Workload; $report.shader=$shaderPair
    $marker.runId=$SyntheticId; $marker.status='complete'
    foreach($row in $report.completedFrameEvidence.rows){if($null -eq $row.PSObject.Properties['activeStrategy']){$row|Add-Member -NotePropertyName activeStrategy -NotePropertyValue 'opaque-fast'}}
    Write-Json (Join-Path $destination 'trial.json') $trial
    Write-Json (Join-Path $nested 'benchmark.json') $report
    Write-Json (Join-Path $nested 'result.json') $marker
    return @{path=$destination;trialPath=(Join-Path $destination 'trial.json');reportPath=(Join-Path $nested 'benchmark.json');report=$report;trial=$trial}
}
function Run-Expected([string]$Name,[string]$Path,[bool]$ShouldPass,[string]$Diagnostic){
    $actual='';$passed=$false
    try{$actual=(& $analyzer -RunPath $Path -ArtifactRole control 2>&1 | Out-String);$passed=$true}catch{$actual=$_.Exception.Message}
    if($ShouldPass -and -not $passed){throw "$Name expected PASS, got: $actual"}
    if(-not $ShouldPass -and $passed){throw "$Name unexpectedly passed"}
    if(-not $ShouldPass -and $actual -notmatch [regex]::Escape($Diagnostic)){throw "$Name wrong diagnostic; expected '$Diagnostic', got '$actual'"}
    return [ordered]@{name=$Name;expected=if($ShouldPass){'PASS'}else{'REJECT'};observed=if($passed){'PASS'}else{'REJECT'};diagnostic=if($ShouldPass){'synthetic fixture accepted'}else{$actual.Trim()}}
}
function Copy-Case([string]$Source,[string]$CaseName){
    $dest=Join-Path $fixtureRoot $CaseName
    Copy-Item -LiteralPath $Source -Destination $dest -Recurse
    return @{path=$dest;trialPath=(Join-Path $dest 'trial.json');reportPath=(Join-Path $dest "$((Get-Content -LiteralPath (Join-Path $dest 'trial.json') -Raw|ConvertFrom-Json).runId)/benchmark.json")}
}
function Save-Trial($Case){$v=Get-Content -LiteralPath $Case.trialPath -Raw|ConvertFrom-Json;Write-Json $Case.trialPath $v;return $v}
function Save-Report($Case){$v=Get-Content -LiteralPath $Case.reportPath -Raw|ConvertFrom-Json;Write-Json $Case.reportPath $v;return $v}

$live=New-SyntheticFixture $liveRun 'all-shadow-c1-live-positive-synthetic' 'lantern-reveal-sequence-v1'
$route=New-SyntheticFixture $controlRun 'all-shadow-c1-route-positive-synthetic' 'showcase-route-v1'
$results=[Collections.Generic.List[object]]::new()
$results.Add((Run-Expected 'synthetic-live-positive' $live.path $true ''))
$results.Add((Run-Expected 'synthetic-route-positive' $route.path $true ''))

$case=Copy-Case $live.path 'all-shadow-c1-forged-apk-synthetic'
$t=Get-Content -LiteralPath $case.trialPath -Raw|ConvertFrom-Json; $t.installedApkSha256=('0'*64); Write-Json $case.trialPath $t
$results.Add((Run-Expected 'forged-installed-apk' $case.path $false 'installed APK versus exact all-shadow artifact'))

$case=Copy-Case $live.path 'all-shadow-c1-wrong-loaded-shader-synthetic'
$r=Get-Content -LiteralPath $case.reportPath -Raw|ConvertFrom-Json; $r.shader='opaqueFast:shipping_mobile_opaque_fast@' + ('0'*64); Write-Json $case.reportPath $r
$results.Add((Run-Expected 'wrong-loaded-shader-pair' $case.path $false 'loaded artifact pair versus actual APK shader identity'))

$case=Copy-Case $route.path 'all-shadow-c1-invalid-opening-strategy-synthetic'
$r=Get-Content -LiteralPath $case.reportPath -Raw|ConvertFrom-Json; $row=@($r.completedFrameEvidence.rows|Where-Object zoneName -CEQ 'opening'|Select-Object -First 1)[0]; $row.activeStrategy='generic-dielectric'; Write-Json $case.reportPath $r
$results.Add((Run-Expected 'invalid-opening-strategy' $case.path $false 'actual opening OpaqueFast completions'))

$case=Copy-Case $live.path 'all-shadow-c1-mismatched-completion-synthetic'
$r=Get-Content -LiteralPath $case.reportPath -Raw|ConvertFrom-Json; $r.completedFrameEvidence.rows[0].completionIdentity.submissionSerial=[long]$r.completedFrameEvidence.rows[0].submittedIdentity.submissionSerial+1; Write-Json $case.reportPath $r
$results.Add((Run-Expected 'mismatched-completion-identity' $case.path $false 'identity submissionSerial at row 0'))

$case=Copy-Case $live.path 'all-shadow-c1-diagnostic-not-compiled-out-synthetic'
$r=Get-Content -LiteralPath $case.reportPath -Raw|ConvertFrom-Json; $r.completedFrameEvidence.rows[0].diagnosticStatus='valid'; Write-Json $case.reportPath $r
$results.Add((Run-Expected 'diagnostic-not-compiled-out' $case.path $false 'diagnostic status'))

$receipt=[ordered]@{
    schema=1;syntheticRegressionOnly=$true;phoneEvidence=$false
    description='Copies of completed earlier trial JSON were altered only inside this fixture root to exercise parser logic. These are not all-shadow runs, phone measurements, or validation of the forthcoming artifact pair.'
    sourceFixtures=@{route=$controlRun;live=$liveRun}
    expectedControlApkSha256=$controlApk;expectedControlPipelineModules=@{opaqueFast=$controlOpaque;genericDielectric=$controlGeneric}
    results=@($results);passedTests=$results.Count;rejectedTests=@($results|Where-Object observed -EQ 'REJECT').Count
}
$receiptPath=Join-Path $fixtureRoot 'synthetic-regression-receipt.json'
Write-Json $receiptPath $receipt
Write-Output "Synthetic parser regression PASS: $($results.Count) cases ($($receipt.rejectedTests) expected rejections). No phone data created or modified. Receipt: $receiptPath"
