param([Parameter(Mandatory)][string]$OutputRoot,[switch]$HighFixtures,[switch]$ControlHighComputeOnly,[switch]$Shipping,[switch]$EdgeWitness,[switch]$OpeningOnly)
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
if ($OpeningOnly -and ($HighFixtures -or $ControlHighComputeOnly -or $EdgeWitness)) {
    throw 'Opening-only compatibility captures are Mobile, without the High observer.'
}
if($EdgeWitness -and (-not $HighFixtures -or $Shipping -or $ControlHighComputeOnly)) {
    throw 'The isolated edge witness is High Diagnostic candidate only.'
}
if ((& git -C $repoRoot branch --show-current) -cne 'codex/horde-rtx-corrections') {
    throw 'Run only in the isolated clean-corrections worktree.'
}
$source = (& git -C $repoRoot rev-parse HEAD).Trim()
$exe = Join-Path $repoRoot 'build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'
if($ControlHighComputeOnly) {
    if($Shipping){throw 'The retained normal control is Diagnostic, not Shipping.'}
    if(-not $HighFixtures){throw 'The missing-control row is only the three High compute fixtures.'}
    $normalRoot='C:/Users/sam_s/.codex/worktrees/horde-mobile-lantern-profile/the Horde RT Demo'
    $source=(& git -C $normalRoot rev-parse HEAD).Trim()
    $exe=Join-Path $normalRoot 'build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'
    if((Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant() -cne
        '3526bc9d5727cf91f21785e91740bce55de0ea61ab5def68ec5174a68eb73a19') {
        throw 'Existing normal High executable changed; do not silently rebuild or substitute it.'
    }
}
$exeHash = (Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant()
$root = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $root) { throw 'Use a new output root; do not overwrite or repeat retained captures.' }
New-Item -ItemType Directory -Path $root | Out-Null
$matrix = foreach ($backend in @('pipeline', 'compute')) {
    if($ControlHighComputeOnly -and $backend -cne 'compute'){continue}
    if ($HighFixtures) {
        foreach ($checkpoint in @('glass-edge-fresnel','lantern-held-high','lantern-held-low')) {
            if($EdgeWitness -and $checkpoint -cne 'glass-edge-fresnel'){continue}
            [pscustomobject]@{backend=$backend;checkpoint=$checkpoint;directory="$backend/$checkpoint";label="$backend-$checkpoint"}
        }
    } else {
        [pscustomobject]@{backend=$backend;checkpoint=$(if($OpeningOnly){'opening'}else{$null});directory=$backend;label=$backend}
    }
}
$quality = if($HighFixtures){'High'}else{'Mobile'}
$instrumentation = if($Shipping){'Shipping'}else{'Diagnostic'}
$runs = foreach ($row in $matrix) {
    $backend = $row.backend
    $destination = Join-Path $root $row.directory
    $start = [Diagnostics.ProcessStartInfo]::new($exe)
    $start.WorkingDirectory = $repoRoot
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.WindowStyle = [Diagnostics.ProcessWindowStyle]::Hidden
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $start.ArgumentList.Add('--capture-showcase')
    $start.ArgumentList.Add($destination)
    if ($backend -ceq 'compute') { $start.ArgumentList.Add('--require-rayquery-compute') }
    if ($null -ne $row.checkpoint) {
        $start.ArgumentList.Add('--development-checkpoint')
        $start.ArgumentList.Add($row.checkpoint)
    }
    $process = [Diagnostics.Process]::Start($start)
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    if (-not $process.WaitForExit(60000)) {
        $process.Kill($true)
        $process.WaitForExit()
        throw "Owned $backend capture exceeded its finite 60-second limit."
    }
    $stdout.GetAwaiter().GetResult() | Set-Content (Join-Path $root "$($row.label)-stdout.log")
    $stderr.GetAwaiter().GetResult() | Set-Content (Join-Path $root "$($row.label)-stderr.log")
    $exitCode = $process.ExitCode
    $process.Dispose()
    if ($exitCode -ne 0) { throw "Capture $backend failed with exit $exitCode; preserve its logs." }
    $manifest = Get-Content (Join-Path $destination 'capture-manifest.json') -Raw|ConvertFrom-Json
    if(-not $manifest.complete -or $null -ne $manifest.error -or
        $manifest.selectedRtPipelineBundle.genericDielectric.key -notmatch "$($instrumentation.ToLowerInvariant())_$($quality.ToLowerInvariant())_generic_dielectric$") {
        throw "Unexpected completed module/quality: $($row.label)"
    }
    [ordered]@{ backend = $backend; checkpoint = $row.checkpoint; exitCode = $exitCode; arguments = @($start.ArgumentList) }
}
[ordered]@{ schema = 1; sourceCommit = $source; executableSha256 = $exeHash;
    instrumentation = $instrumentation; quality = $quality; payloadRows = $(if($EdgeWitness){1}else{0});
    existingNormalBinary = [bool]$ControlHighComputeOnly;
    investigationOnly = $true; performanceEvidence = $false; runs = @($runs) } |
    ConvertTo-Json -Depth 6 | Set-Content (Join-Path $root 'run-receipt.json')
Write-Output "Completed finite clean capture pair: $source / $exeHash"
