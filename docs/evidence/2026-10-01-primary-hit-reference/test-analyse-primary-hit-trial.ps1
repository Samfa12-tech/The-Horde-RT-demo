param()
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
$fixtureRoot=Join-Path $root 'parser-fixtures\synthetic-only-v4'
if(Test-Path -LiteralPath $fixtureRoot){throw "Refusing existing synthetic fixture root: $fixtureRoot"}
$harness=Join-Path $fixtureRoot 'harness'
$null=New-Item -ItemType Directory -Path (Join-Path $harness 'artifacts\control') -Force
$null=New-Item -ItemType Directory -Path (Join-Path $harness 'artifacts\primary-hit-probe') -Force
$analyzer=Join-Path $harness 'analyse-primary-hit-trial.ps1'
Copy-Item -LiteralPath (Join-Path $root 'analyse-primary-hit-trial.ps1') -Destination $analyzer
Copy-Item -LiteralPath (Join-Path $root 'build-receipt.json') -Destination (Join-Path $harness 'build-receipt.json')
Copy-Item -LiteralPath (Join-Path $root 'artifacts\control\containment.json') -Destination (Join-Path $harness 'artifacts\control\containment.json')
Copy-Item -LiteralPath (Join-Path $root 'artifacts\control\all-shadow-build-receipt.json') -Destination (Join-Path $harness 'artifacts\control\all-shadow-build-receipt.json')
Copy-Item -LiteralPath (Join-Path $root 'artifacts\control\raygen-variant-catalog.json') -Destination (Join-Path $harness 'artifacts\control\raygen-variant-catalog.json')
Copy-Item -LiteralPath (Join-Path $root 'artifacts\control\rayquery-variant-catalog.json') -Destination (Join-Path $harness 'artifacts\control\rayquery-variant-catalog.json')
Copy-Item -LiteralPath (Join-Path $root 'artifacts\primary-hit-probe\package-containment.json') -Destination (Join-Path $harness 'artifacts\primary-hit-probe\package-containment.json')
Copy-Item -LiteralPath (Join-Path $root 'artifacts\primary-hit-probe\build-receipt.json') -Destination (Join-Path $harness 'artifacts\primary-hit-probe\build-receipt.json')
Copy-Item -LiteralPath (Join-Path $root 'artifacts\primary-hit-probe\raygen-variant-catalog.json') -Destination (Join-Path $harness 'artifacts\primary-hit-probe\raygen-variant-catalog.json')
Copy-Item -LiteralPath (Join-Path $root 'artifacts\primary-hit-probe\rayquery-variant-catalog.json') -Destination (Join-Path $harness 'artifacts\primary-hit-probe\rayquery-variant-catalog.json')
Copy-Item -LiteralPath (Join-Path $root 'artifacts\primary-hit-probe\asset-shader-comparison.json') -Destination (Join-Path $harness 'artifacts\primary-hit-probe\asset-shader-comparison.json')

$historical='C:\Dev\tmp\horde-all-shadow-profile-20261001\phone\all-shadow-c1-route-20261001'
$historicalId=Split-Path $historical -Leaf
$historicalNested=Join-Path $historical $historicalId

$controlApk='a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033'
$referenceApk='40d0f759dca40ff8433ce4aca19156a135a9146a960f115f4cdbb78d3db7cf9e'
$controlOpaque='66e39df9f53b058fb62cbfa913d424b161c93be4aff59a1685cf8b7e54bb9c4b'
$controlGeneric='ce2302811cb2cb8bbff706fd54cd7f48705e4a5e8b2cda744a7d999574f84532'
$referenceOpaque='5c03eed845bea5fe0c0a654f613694ca428e97b61cdb940da90133309156e928'
$referenceGeneric='0e5a658a94e2dd26d6e959aea90eca21cd52867baa2e734f3a2f7cb2dff2c92b'
function Write-Json([string]$Path,$Value){[IO.File]::WriteAllText($Path,($Value|ConvertTo-Json -Depth 100),[Text.UTF8Encoding]::new($false))}
function Reset-Fixture([string]$Role){
    $runId=if($Role -ceq 'control'){'primary-hit-c1-route-20261001'}else{'primary-hit-p1-route-20261001'}
    $runDir=Join-Path $fixtureRoot "$Role-run"
    $nested=Join-Path $runDir $runId
    $null=New-Item -ItemType Directory -Path $nested -Force
    Copy-Item -LiteralPath (Join-Path $historical 'trial.json') -Destination (Join-Path $runDir 'trial.json') -Force
    $trial=Get-Content -LiteralPath (Join-Path $historical 'trial.json') -Raw|ConvertFrom-Json
    $report=Get-Content -LiteralPath (Join-Path $historicalNested 'benchmark.json') -Raw|ConvertFrom-Json
    $marker=Get-Content -LiteralPath (Join-Path $historicalNested 'result.json') -Raw|ConvertFrom-Json
    $trial.runId=$runId;$trial.sourceCommit='71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95';$trial.workload='showcase-route-v1';$trial.deviceModel='SM-S948B';$trial.instrumentation='Shipping';$trial.quality='Mobile'
    $trial.buildLabel=if($Role -ceq 'control'){'primary-hit-control'}else{'primary-hit-reference'}
    $trial.installedApkSha256=if($Role -ceq 'control'){$controlApk}else{$referenceApk}
    $report.runId=$runId;$report.workload='showcase-route-v1'
    $opaque=if($Role -ceq 'control'){$controlOpaque}else{$referenceOpaque}
    $generic=if($Role -ceq 'control'){$controlGeneric}else{$referenceGeneric}
    $report.shader="opaqueFast:shipping_mobile_opaque_fast@$opaque|genericDielectric:shipping_mobile_generic_dielectric@$generic"
    $marker.runId=$runId;$marker.status='complete'
    Write-Json (Join-Path $runDir 'trial.json') $trial
    Write-Json (Join-Path $nested 'benchmark.json') $report
    Write-Json (Join-Path $nested 'result.json') $marker
    Copy-Item -LiteralPath (Join-Path $historical 'context-samples.jsonl') -Destination (Join-Path $runDir 'context-samples.jsonl') -Force
    return @{path=$runDir;trialPath=(Join-Path $runDir 'trial.json');reportPath=(Join-Path $nested 'benchmark.json')}
}
function Run-Expected([string]$Name,[string]$Path,[string]$Role,[bool]$ShouldPass,[string]$Diagnostic){
    $actual='';$passed=$false
    try{$actual=(& $analyzer -RunPath $Path -ArtifactRole $Role 2>&1|Out-String);$passed=$true}catch{$actual=$_.Exception.Message}
    if($ShouldPass -and -not $passed){throw "$Name expected PASS, got: $actual"}
    if(-not $ShouldPass -and $passed){throw "$Name unexpectedly passed"}
    if(-not $ShouldPass -and $actual -notmatch [regex]::Escape($Diagnostic)){throw "$Name expected '$Diagnostic', got '$actual'"}
    return [ordered]@{name=$Name;expected=if($ShouldPass){'PASS'}else{'REJECT'};observed=if($passed){'PASS'}else{'REJECT'};diagnostic=if($ShouldPass){'synthetic fixture accepted'}else{$actual.Trim()}}
}

$results=[Collections.Generic.List[object]]::new()
$case=Reset-Fixture 'control'
$results.Add((Run-Expected 'synthetic-control-positive' $case.path 'control' $true ''))
$case=Reset-Fixture 'reference'
$results.Add((Run-Expected 'synthetic-reference-positive' $case.path 'reference' $true ''))

$case=Reset-Fixture 'control';$t=Get-Content $case.trialPath -Raw|ConvertFrom-Json;$t.buildLabel='wrong-label';Write-Json $case.trialPath $t
$results.Add((Run-Expected 'forged-build-label' $case.path 'control' $false 'primary-hit build label'))
$case=Reset-Fixture 'control';$t=Get-Content $case.trialPath -Raw|ConvertFrom-Json;$t.installedApkSha256=('0'*64);Write-Json $case.trialPath $t
$results.Add((Run-Expected 'forged-installed-apk' $case.path 'control' $false 'installed APK versus exact primary-hit artifact'))
$case=Reset-Fixture 'reference';$r=Get-Content $case.reportPath -Raw|ConvertFrom-Json;$r.shader='opaqueFast:shipping_mobile_opaque_fast@'+('0'*64);Write-Json $case.reportPath $r
$results.Add((Run-Expected 'wrong-loaded-module-sha' $case.path 'reference' $false 'loaded artifact pair versus actual APK shader identity'))
$case=Reset-Fixture 'control';$r=Get-Content $case.reportPath -Raw|ConvertFrom-Json;$r.measuredFrames=1837;Write-Json $case.reportPath $r
$results.Add((Run-Expected 'forged-completion-denominator' $case.path 'control' $false 'measured count'))
$case=Reset-Fixture 'control';$r=Get-Content $case.reportPath -Raw|ConvertFrom-Json;$row=@($r.completedFrameEvidence.rows|Where-Object zoneName -CEQ 'opening'|Select-Object -First 1)[0];$row.activeStrategy='generic-dielectric';Write-Json $case.reportPath $r
$results.Add((Run-Expected 'invalid-opening-strategy' $case.path 'control' $false 'actual opening OpaqueFast completions'))
$case=Reset-Fixture 'control';$r=Get-Content $case.reportPath -Raw|ConvertFrom-Json;$r.completedFrameEvidence.rows[0].completionIdentity.submissionSerial=[long]$r.completedFrameEvidence.rows[0].submittedIdentity.submissionSerial+1;Write-Json $case.reportPath $r
$results.Add((Run-Expected 'mismatched-completion-identity' $case.path 'control' $false 'identity submissionSerial at row 0'))

$receipt=[ordered]@{schema=1;syntheticRegressionOnly=$true;phoneEvidence=$false;sourceFixture='completed historical all-shadow control report copied and relabelled only inside this isolated test fixture';testCount=$results.Count;expectedRejections=@($results|Where-Object observed -EQ 'REJECT').Count;results=@($results);limitations=@('Positive parser fixtures only exercise parsing/identity joins; they are not primary-hit measurements.','No synthetic report is in the live phone directory or actual analysis directory.')}
$receiptPath=Join-Path $fixtureRoot 'synthetic-regression-receipt.json'
Write-Json $receiptPath $receipt
Write-Output "Synthetic primary-hit parser PASS: $($results.Count) cases, $($receipt.expectedRejections) expected rejects. Receipt: $receiptPath"
