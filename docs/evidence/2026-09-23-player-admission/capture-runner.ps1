param([Parameter(Mandatory=$true)][string]$OutputRoot)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$root = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $root) { throw 'Choose a new output root' }
$exe = 'C:\Dev\tmp\horde-phase2-images-20260920\Debug\HordeLanternRT.exe'
$generated = Join-Path $repo 'reports/player-regeneration-proof-da931ee918ba455c9a7defe85c292d0c/attempt-1.glb'
if ((Get-FileHash $exe).Hash -cne '9D26F20DCDFD4AF90031B977CF3EB0203D2AE3B221A1C6D7DD5804A399B9368D') { throw 'Unexpected executable' }
if ((Get-FileHash $generated).Hash -cne 'E8737F10E7669B284E04109D9C3ACDF537DF284093A21511656BF191A70450FD') { throw 'Unexpected generated player' }
$files = @([regex]::Matches((Get-Content (Join-Path $repo 'tools/run-foundation-validation.ps1') -Raw), 'Source = "(assets\\[^"\r\n]+)"') | ForEach-Object { $_.Groups[1].Value })
if ($files.Count -ne 29) { throw 'Review changed foundation runtime staging roster' }
foreach ($audio in @('assets/audio/filmcow','assets/audio/pixabay')) {
    $files += @(Get-ChildItem -LiteralPath (Join-Path $repo $audio) -Filter '*.wav' -File | ForEach-Object { $_.FullName.Substring($repo.Length + 1) })
}
New-Item -ItemType Directory -Path $root | Out-Null
foreach ($side in @('baseline','candidate')) {
    $stage = Join-Path $root $side
    New-Item -ItemType Directory -Path $stage | Out-Null
    Copy-Item -LiteralPath $exe -Destination (Join-Path $stage 'HordeLanternRT.exe')
    Copy-Item -LiteralPath (Join-Path $repo 'ASSET_LICENSES.md') -Destination $stage
    foreach ($file in $files) {
        $destination = Join-Path $stage $file
        New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
        Copy-Item -LiteralPath (Join-Path $repo $file) -Destination $destination
    }
    if ($side -eq 'candidate') {
        Copy-Item -LiteralPath $generated -Destination (Join-Path $stage 'assets/models/player/runtime/gothic-traveller-lod0.runtime.glb')
    }
}
$differences = @($files | Where-Object { (Get-FileHash -LiteralPath (Join-Path "$root/baseline" $_)).Hash -ne (Get-FileHash -LiteralPath (Join-Path "$root/candidate" $_)).Hash })
if ($differences.Count -ne 1 -or $differences[0] -notlike '*gothic-traveller-lod0.runtime.glb') { throw 'Stages differ outside the candidate GLB' }
$checkpoints = @('player-body-grips','player-body-forward','player-body-owner-feedback','player-body-downward-cut','player-body-upward-slice')
foreach ($checkpoint in $checkpoints) {
    foreach ($side in @('baseline','candidate')) {
        $stage = Join-Path $root $side
        $capture = Join-Path "$root/captures/$side" $checkpoint
        New-Item -ItemType Directory -Path $capture -Force | Out-Null
        $process = Start-Process -FilePath (Join-Path $stage 'HordeLanternRT.exe') -WorkingDirectory $stage -WindowStyle Hidden -ArgumentList @('--capture-showcase', $capture, '--development-checkpoint', $checkpoint) -RedirectStandardOutput (Join-Path $capture 'stdout.log') -RedirectStandardError (Join-Path $capture 'stderr.log') -PassThru
        $deadline = [DateTime]::UtcNow.AddMinutes(2)
        while (-not $process.WaitForExit(1000)) {
            if ([DateTime]::UtcNow -gt $deadline) { $process.Kill(); throw "Capture timed out: $side/$checkpoint" }
        }
        $process.Refresh()
        if ($process.ExitCode -ne 0) { throw "Capture failed with $($process.ExitCode): $side/$checkpoint" }
        $manifest = Get-Content -LiteralPath (Join-Path $capture 'capture-manifest.json') -Raw | ConvertFrom-Json
        if (-not $manifest.complete -or $manifest.source -cne 'rt-storage-image' -or
            $manifest.executionBackend -cne 'RayTracingPipeline' -or
            $manifest.captures.Count -ne 1 -or $manifest.captures[0].checkpoint -cne $checkpoint) { throw 'Incomplete or mismatched native capture' }
        Write-Output "Passed native capture $side/$checkpoint"
    }
}
Write-Output $root
