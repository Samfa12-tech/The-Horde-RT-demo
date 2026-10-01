[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][ValidatePattern('^[0-9a-f]{40}$')][string]$CandidateSourceCommit,
    [string]$RepoRoot = 'C:\Users\sam_s\.codex\worktrees\horde-mobile-lantern-profile\the Horde RT Demo',
    [string]$ControlApk = 'C:\Dev\tmp\horde-primary-opacity-20261001\control-shipping.apk',
    [string]$CandidateApk = 'C:\Dev\tmp\horde-primary-opacity-20261001\candidate-shipping.apk',
    [string]$OutputRoot = 'C:\Dev\tmp\horde-primary-opacity-20261001\performance\phone',
    [switch]$Execute
)
$ErrorActionPreference = 'Stop'

$controlCommit = '6fa1c53d1f0e4ec3938983f2cad7bd2ece233f4a'
$controlHash = '3cb84efb2f71b1c96b8a562ae6a272569e3c76315053144582a617be75bf30eb'
$candidateHash = 'b9d69ff43c13b0d84ff8fca11132578a27ae710ac2946546677d707655b9a188'
$serial = 'R5GL219SZGK'
$model = 'SM-S948B'
$backend = 'RayTracingPipeline'
$runner = 'C:\Dev\tmp\horde-primary-opacity-20261001\performance\run-opacity-trial.ps1'
if (-not (Test-Path -LiteralPath $runner -PathType Leaf)) { throw "Existing validated collector not found: $runner" }
if (-not (Test-Path -LiteralPath $ControlApk -PathType Leaf) -or
    (Get-FileHash -LiteralPath $ControlApk -Algorithm SHA256).Hash.ToLowerInvariant() -cne $controlHash) {
    throw 'Control APK missing or does not match the supplied full SHA-256.'
}
if (-not (Test-Path -LiteralPath $CandidateApk -PathType Leaf) -or
    (Get-FileHash -LiteralPath $CandidateApk -Algorithm SHA256).Hash.ToLowerInvariant() -cne $candidateHash) {
    throw 'Candidate APK missing or does not match the supplied full SHA-256.'
}

# Fixed ABBA order; within each member, run the route before the authored high pose.
$plan = @(
    [ordered]@{ block='A1'; role='normal-control'; member='normal-control'; workload='showcase-route-v1'; source=$controlCommit; apk=$ControlApk; hash=$controlHash },
    [ordered]@{ block='A1'; role='normal-control'; member='normal-control'; workload='lantern-held-high-v1'; source=$controlCommit; apk=$ControlApk; hash=$controlHash },
    [ordered]@{ block='B1'; role='primary-opacity-candidate'; member='primary-opacity-candidate'; workload='showcase-route-v1'; source=$CandidateSourceCommit; apk=$CandidateApk; hash=$candidateHash },
    [ordered]@{ block='B1'; role='primary-opacity-candidate'; member='primary-opacity-candidate'; workload='lantern-held-high-v1'; source=$CandidateSourceCommit; apk=$CandidateApk; hash=$candidateHash },
    [ordered]@{ block='B2'; role='primary-opacity-candidate'; member='primary-opacity-candidate'; workload='showcase-route-v1'; source=$CandidateSourceCommit; apk=$CandidateApk; hash=$candidateHash },
    [ordered]@{ block='B2'; role='primary-opacity-candidate'; member='primary-opacity-candidate'; workload='lantern-held-high-v1'; source=$CandidateSourceCommit; apk=$CandidateApk; hash=$candidateHash },
    [ordered]@{ block='A2'; role='normal-control'; member='normal-control'; workload='showcase-route-v1'; source=$controlCommit; apk=$ControlApk; hash=$controlHash },
    [ordered]@{ block='A2'; role='normal-control'; member='normal-control'; workload='lantern-held-high-v1'; source=$controlCommit; apk=$ControlApk; hash=$controlHash }
)
for ($i = 0; $i -lt $plan.Count; $i++) {
    $plan[$i].runId = "opacity-$($plan[$i].block)-$($plan[$i].workload)"
    $plan[$i].order = $i + 1
}

Write-Output "Bounded plan: 8 rows, 75%, $model/$serial, $backend, Shipping/Mobile; no image recapture."
$plan | ForEach-Object { '{0}. {1} {2} {3} {4} ({5})' -f $_.order,$_.block,$_.role,$_.workload,$_.runId,$_.hash }
if (-not $Execute) {
    Write-Output 'Plan only. After the candidate commit is final, execute with -CandidateSourceCommit <40-hex-commit> -Execute.'
    return
}

New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null
# Preflight every existing row before any install/launch. Complete rows are skipped;
# any incomplete row halts the matrix for manual recovery without overwriting it.
foreach ($row in $plan) {
    $dir = Join-Path $OutputRoot $row.runId
    if (-not (Test-Path -LiteralPath $dir)) { continue }
    $meta = Join-Path $dir 'trial.json'
    if (-not (Test-Path -LiteralPath $meta -PathType Leaf)) { throw "Existing incomplete row must be retained/reviewed: $dir" }
    $saved = Get-Content -LiteralPath $meta -Raw | ConvertFrom-Json
    if ($saved.status -cne 'complete' -or $saved.runId -cne $row.runId -or
        $saved.installedApkSha256 -cne $row.hash -or $saved.sourceCommit -cne $row.source -or
        $saved.workload -cne $row.workload) { throw "Existing row is incomplete or identity-mismatched; retain it: $dir" }
    $reportPath = Join-Path (Join-Path $dir $row.runId) 'benchmark.json'
    $markerPath = Join-Path (Join-Path $dir $row.runId) 'result.json'
    if (-not (Test-Path -LiteralPath $reportPath -PathType Leaf) -or -not (Test-Path -LiteralPath $markerPath -PathType Leaf)) {
        throw "Existing row lacks its complete retained reports; do not restart: $dir"
    }
    $marker = Get-Content -LiteralPath $markerPath -Raw | ConvertFrom-Json
    if ($marker.status -cne 'complete' -or $marker.runId -cne $row.runId) { throw "Existing row result marker is not complete: $dir" }
    $row.skip = $true
    Write-Output "Skipping already complete row $($row.runId)."
}

foreach ($row in $plan) {
    if ($row.skip) { continue }
    Write-Output "Starting $($row.order)/8: $($row.runId) [$($row.role)]"
    & $runner -RunId $row.runId -SourceCommit $row.source -ApkPath $row.apk `
        -ApkSha256 $row.hash -DeviceSerial $serial -DeviceModel $model `
        -Backend $backend -Member $row.member -Workload $row.workload `
        -OutputRoot $OutputRoot -Install
    if ($LASTEXITCODE -ne 0) { throw "Existing collector failed for $($row.runId); retain partial evidence and do not restart it." }
}
Write-Output 'All planned rows are complete or retained as complete. Analyze only after both package audits are available.'
