[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$taskRoot = 'C:\Dev\tmp\horde-mobile-lantern-profile-20261001'
$phoneRoot = Join-Path $taskRoot 'phone'
$sourceRun = 'mobile-pane-s26-c1-route-valid-20261001'
$sourceDirectory = Join-Path $phoneRoot $sourceRun
$fixtureRoot = Join-Path $phoneRoot 'parser-fixtures-v9'
$analyzer = 'C:\Users\sam_s\.codex\worktrees\horde-mobile-lantern-profile\the Horde RT Demo\docs\evidence\2026-10-01-mobile-lantern-profile\analyse-trial.ps1'
$receipt = Join-Path $taskRoot 'audit-pair-v3-3a4faf72923e-bac94c45bcfe\actual-apk-package-audit.json'
if (Test-Path -LiteralPath $fixtureRoot) { throw "Refusing to overwrite parser test fixtures: $fixtureRoot" }
if (-not (Test-Path -LiteralPath $sourceDirectory -PathType Container)) { throw "Source control fixture is missing: $sourceDirectory" }
$null = New-Item -ItemType Directory -Path $fixtureRoot

function Write-JsonFile([string]$Path, $Value) {
    [IO.File]::WriteAllText($Path, (ConvertTo-Json -InputObject $Value -Depth 30) + "`n", [Text.UTF8Encoding]::new($false))
}
function New-Fixture([string]$Case, [string]$SeedRun = $sourceRun) {
    $runId = "parser-fixtures-v9/$Case"
    $directory = Join-Path $fixtureRoot (Join-Path 'parser-fixtures-v9' $Case)
    $nested = Join-Path $directory $runId
    $null = New-Item -ItemType Directory -Path $nested -Force
    $seedDirectory = Join-Path $phoneRoot $SeedRun
    Copy-Item -LiteralPath (Join-Path $seedDirectory 'trial.json') -Destination (Join-Path $directory 'trial.json')
    Copy-Item -LiteralPath (Join-Path $seedDirectory 'context-samples.jsonl') -Destination (Join-Path $directory 'context-samples.jsonl')
    Copy-Item -LiteralPath (Join-Path $seedDirectory 'memory-before.txt') -Destination (Join-Path $directory 'memory-before.txt')
    Copy-Item -LiteralPath (Join-Path $seedDirectory 'memory-after.txt') -Destination (Join-Path $directory 'memory-after.txt')
    $sourceNested = Join-Path $seedDirectory $SeedRun
    Copy-Item -LiteralPath (Join-Path $sourceNested 'benchmark.json') -Destination (Join-Path $nested 'benchmark.json')
    Copy-Item -LiteralPath (Join-Path $sourceNested 'result.json') -Destination (Join-Path $nested 'result.json')
    $trialPath = Join-Path $directory 'trial.json'
    $benchmarkPath = Join-Path $nested 'benchmark.json'
    $markerPath = Join-Path $nested 'result.json'
    $trial = Get-Content -Raw -LiteralPath $trialPath | ConvertFrom-Json
    $report = Get-Content -Raw -LiteralPath $benchmarkPath | ConvertFrom-Json
    $marker = Get-Content -Raw -LiteralPath $markerPath | ConvertFrom-Json
    $trial.runId = $runId
    $report.runId = $runId
    $marker.runId = $runId
    Write-JsonFile $trialPath $trial
    Write-JsonFile $benchmarkPath $report
    Write-JsonFile $markerPath $marker
    return [pscustomobject]@{ runId=$runId; directory=$directory; trialPath=$trialPath; benchmarkPath=$benchmarkPath; markerPath=$markerPath; seedRun=$SeedRun }
}
function Invoke-Reject([string]$Name, [string]$Expected, [scriptblock]$Mutation, [string]$SeedRun = $sourceRun) {
    $fixture = New-Fixture $Name $SeedRun
    $trial = Get-Content -Raw -LiteralPath $fixture.trialPath | ConvertFrom-Json
    $report = Get-Content -Raw -LiteralPath $fixture.benchmarkPath | ConvertFrom-Json
    $marker = Get-Content -Raw -LiteralPath $fixture.markerPath | ConvertFrom-Json
    & $Mutation $trial $report $marker
    Write-JsonFile $fixture.trialPath $trial
    Write-JsonFile $fixture.benchmarkPath $report
    Write-JsonFile $fixture.markerPath $marker
    try {
        $null = & $analyzer -RunId $fixture.runId -PhoneRoot $fixtureRoot -PackageReceipt $receipt 2>&1
        throw "Expected '$Name' fixture to fail closed."
    } catch {
        $message = $_.Exception.Message
        if ($message -like "Expected '$Name' fixture to fail closed.") { throw $message }
    }
    if ($message -notlike "*$Expected*") { throw "Fixture '$Name' failed for the wrong reason. Expected '$Expected'; got: $message" }
    return [pscustomobject]@{ fixture=$Name; result='expected rejection'; reason=$Expected }
}

$results = [Collections.Generic.List[object]]::new()
$green = New-Fixture 'green-control'
try { $null = & $analyzer -RunId $green.runId -PhoneRoot $fixtureRoot -PackageReceipt $receipt 2>&1 }
catch { throw "Green synthetic control failed: $($_.Exception.Message)" }
$results.Add([pscustomobject]@{ fixture='green-control'; result='PASS'; reason='synthetic clone only' })
$greenCandidate = New-Fixture 'green-candidate' 'mobile-pane-s26-p1-high-20261001'
try { $null = & $analyzer -RunId $greenCandidate.runId -PhoneRoot $fixtureRoot -PackageReceipt $receipt 2>&1 }
catch { throw "Green synthetic open-aperture candidate failed: $($_.Exception.Message)" }
$results.Add([pscustomobject]@{ fixture='green-candidate'; result='PASS'; reason='completed P1 high clone; opaque-fast selected' })

$results.Add((Invoke-Reject 'identity' 'result marker identity' { param($t,$r,$m) $m.runId='wrong-run' }))
$results.Add((Invoke-Reject 'gpu-gap' 'GPU status at row 0' { param($t,$r,$m) $r.completedFrameEvidence.rows[0].gpuStatus='missing' }))
$results.Add((Invoke-Reject 'diagnostics' 'Shipping diagnostic status at row 0' { param($t,$r,$m) $r.completedFrameEvidence.rows[0].diagnosticStatus='valid' }))
$results.Add((Invoke-Reject 'measured-count' 'measured frame count expected' { param($t,$r,$m) $r.measuredFrames=1837 }))
$results.Add((Invoke-Reject 'artifact-identity' 'installed APK package identity' { param($t,$r,$m) $t.installedApkSha256=('0'*64) }))
$results.Add((Invoke-Reject 'shader-identity' 'loaded backend-specific Shipping/Mobile shader identities' { param($t,$r,$m) $r.shader='unrecognized' }))
$results.Add((Invoke-Reject 'source-identity' 'member source receipt identity' { param($t,$r,$m) $t.sourceCommit=('0'*40) } 'mobile-pane-s26-p1-high-20261001'))
$results.Add((Invoke-Reject 'device-serial' 'exact Android device serial' { param($t,$r,$m) $t.serial='wrong-device' } 'mobile-pane-s26-p1-high-20261001'))
$results.Add((Invoke-Reject 'ledger-owning-identity' 'ledger scene epoch at row 0' { param($t,$r,$m) $r.completedFrameEvidence.rows[0].submittedIdentity.sceneEpoch=999; $r.completedFrameEvidence.rows[0].completionIdentity.sceneEpoch=999 } 'mobile-pane-s26-p1-high-20261001'))

$summary = [ordered]@{ schema=1; classification='synthetic parser fixtures only; not physical evidence'; analyzerPath=$analyzer; testedSourceRun=$sourceRun; cases=$results.ToArray() }
$receiptPath = Join-Path $fixtureRoot 'parser-test-receipt.json'
Write-JsonFile $receiptPath $summary
Get-FileHash -LiteralPath $receiptPath -Algorithm SHA256 | Select-Object Path,Hash
Write-Output "Strict parser tests: $($results.Count) passed (two green, nine fail-closed); all source trials remained untouched."
