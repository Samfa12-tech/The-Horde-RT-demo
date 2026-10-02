$ErrorActionPreference = 'Stop'
$cuffRepo = 'C:/Users/sam_s/.codex/worktrees/horde-mobile-lantern-profile/the Horde RT Demo'
$cuffStage = 'C:/Dev/tmp/horde-right-cuff-rt-20261002-a'
$cuffOutput = 'C:/Dev/tmp/horde-right-cuff-fit-20261002-guarded'
$worldRelative = 'assets/models/player/runtime/gothic-traveller-lod0.runtime.glb'
$viewRelative = 'assets/models/player/viewmodel/runtime/gothic-traveller-viewmodel.runtime.glb'
function Assert-Hash($path, $sha) {
    if ((Get-FileHash -LiteralPath $path).Hash -ne $sha) { throw "Unexpected artifact: $path" }
}
Assert-Hash "$cuffStage/HordeLanternRT.exe" '5c6a778257b9f6d439905233328b3d193e789d6bcfbf8c6a9769c2b139c78372'
Assert-Hash "$cuffRepo/$worldRelative" 'f2c3f62b2696c4630309b1d0b0ecb366151054fb48f7c0c6bcc6956980ff81eb'
Assert-Hash "$cuffRepo/$viewRelative" '6f06d77e754d7e2d9017b84c1204879aba2be302b69c077c5f84578408d5166b'
Assert-Hash "$cuffOutput/world-seam-reconciled.runtime.glb" '46a88dac9dd569117be5a17a35540fecf3e0e6026d3ad404dd5fdbc85a9ad54f'
Assert-Hash "$cuffOutput/gothic-traveller-viewmodel.runtime.glb" 'eaa0db3abb8ff2dae616247054c190031a08c542dd95396cc6dc1e18a7462488'
try {
    foreach ($variant in @('control-landscape', 'fitted-landscape')) {
        if (Test-Path -LiteralPath "$cuffStage/$variant") { throw "Do not repeat retained capture $variant" }
        $worldSource = if ($variant -eq 'control-landscape') { "$cuffRepo/$worldRelative" } else { "$cuffOutput/world-seam-reconciled.runtime.glb" }
        $viewSource = if ($variant -eq 'control-landscape') { "$cuffRepo/$viewRelative" } else { "$cuffOutput/gothic-traveller-viewmodel.runtime.glb" }
        Copy-Item -LiteralPath $worldSource -Destination "$cuffStage/$worldRelative"
        Copy-Item -LiteralPath $viewSource -Destination "$cuffStage/$viewRelative"
        $capture = Start-Process -FilePath "$cuffStage/HordeLanternRT.exe" -WorkingDirectory $cuffStage -WindowStyle Hidden -ArgumentList @('--capture-showcase', "$cuffStage/$variant", '--development-checkpoint', 'player-viewmodel-grips') -RedirectStandardOutput "$cuffStage/$variant.stdout.txt" -RedirectStandardError "$cuffStage/$variant.stderr.txt" -Wait -PassThru
        Write-Output "$variant exit=$($capture.ExitCode)"
        if ($capture.ExitCode -ne 0) { throw "Native capture failed: $variant" }
        $manifest = Get-Content -LiteralPath "$cuffStage/$variant/capture-manifest.json" -Raw | ConvertFrom-Json
        if (-not $manifest.complete -or -not $manifest.captures[0].honestlyPresentedRtFrame) { throw 'Native RT evidence incomplete' }
    }
} finally {
    Copy-Item -LiteralPath "$cuffRepo/$worldRelative" -Destination "$cuffStage/$worldRelative"
    Copy-Item -LiteralPath "$cuffRepo/$viewRelative" -Destination "$cuffStage/$viewRelative"
    Assert-Hash "$cuffStage/$worldRelative" 'f2c3f62b2696c4630309b1d0b0ecb366151054fb48f7c0c6bcc6956980ff81eb'
    Assert-Hash "$cuffStage/$viewRelative" '6f06d77e754d7e2d9017b84c1204879aba2be302b69c077c5f84578408d5166b'
}
