[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [string]$BlenderExecutable = 'blender',
    [string]$AcceptedPairDirectory = ''
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $outputRoot) { throw 'Choose a new validation directory; existing outputs are never overwritten.' }
$processor = Join-Path $PSScriptRoot 'process-player-rig-runtime.py'
$rig = Join-Path $repoRoot 'assets/models/player/source/meshy-2026-08-26-gothic-traveller-candidate-2/player-rigged.glb'
$walk = Join-Path $repoRoot 'assets/models/player/source/meshy-2026-08-26-gothic-traveller-candidate-2/player-walking.glb'
$pbr = Join-Path $repoRoot 'assets/textures/player/source'
$gauntlet = Join-Path $repoRoot 'assets/models/player/source/meshy-2026-08-30-viewmodel-gauntlet/right-gauntlet-5k-stripped.glb'
foreach ($path in @($processor, $rig, $walk, $pbr, $gauntlet)) {
    if (-not (Test-Path -LiteralPath $path)) { throw "Required existing input missing: $path" }
}
$blender = (Get-Command $BlenderExecutable -ErrorAction Stop).Source
$version = @(& $blender --version)
if ($LASTEXITCODE -ne 0) { throw 'Blender version check failed' }
New-Item -ItemType Directory -Path $outputRoot | Out-Null
$inputFiles = @($processor, $rig, $walk, $gauntlet) + @(Get-ChildItem -LiteralPath $pbr -File | ForEach-Object { $_.FullName })
$inputs = @($inputFiles | ForEach-Object {
    [ordered]@{ path = $_.Substring($repoRoot.Length + 1).Replace('\', '/'); sha256 = (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash.ToLowerInvariant() }
})
$outputs = @()
$pairReference = $null
if ($AcceptedPairDirectory) {
    $pairReference = [IO.Path]::GetFullPath($AcceptedPairDirectory)
    $processor = Join-Path $PSScriptRoot 'process-player-viewmodel-runtime.py'
    $pairFiles = @('world-seam-reconciled.runtime.glb', 'gothic-traveller-viewmodel.runtime.glb', 'asset.manifest.json')
    foreach ($name in $pairFiles) {
        if (-not (Test-Path -LiteralPath (Join-Path $pairReference $name) -PathType Leaf)) {
            throw "Accepted pair reference missing: $name"
        }
    }
    $recipe = @('--gauntlet-source-hand', 'Right', '--grip-roll-degrees', '105', '145',
        '--blend-elbows', '--gauntlet-scale', '.099', '--reconcile-sleeve-seams',
        '--body-remainder', '--retain-upper-torso', '--reconcile-segmented-seams')
    foreach ($path in @($processor,
            (Join-Path $PSScriptRoot 'player_body_partition.py'),
            (Join-Path $PSScriptRoot 'player_sleeve_seams.py'),
            (Join-Path $PSScriptRoot 'player_segmented_seams_blender.py'),
            (Join-Path $repoRoot 'assets/models/player/runtime/asset.manifest.json'),
            (Join-Path $repoRoot 'assets/models/player/viewmodel/runtime/viewmodel-processing.json'))) {
        $inputs += [ordered]@{ path = $path.Substring($repoRoot.Length + 1).Replace('\', '/');
            sha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() }
    }
}
foreach ($attempt in 1..2) {
    $destination = Join-Path $outputRoot ("attempt-$attempt.glb")
    $log = Join-Path $outputRoot ("attempt-$attempt.log")
    # Blender's parallel export changed tangent rounding and unique-vertex order
    # between clean runs. Pin the observed repeatable single-thread invocation.
    if ($pairReference) {
        $destination = Join-Path $outputRoot "pair-$attempt"
        & $blender --background --threads 1 --python-exit-code 1 --python $processor -- $destination @recipe > $log 2>&1
    } else {
        & $blender --background --threads 1 --python-exit-code 1 --python $processor -- $rig $walk $pbr $gauntlet $destination > $log 2>&1
    }
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $destination)) {
        throw "Player regeneration attempt $attempt failed; inspect $log"
    }
    if ($pairReference) {
        foreach ($name in $pairFiles) {
            $actual = Join-Path $destination $name
            $reference = Join-Path $pairReference $name
            $digest = (Get-FileHash -LiteralPath $actual -Algorithm SHA256).Hash.ToLowerInvariant()
            $expectedDigest = (Get-FileHash -LiteralPath $reference -Algorithm SHA256).Hash.ToLowerInvariant()
            $outputs += [ordered]@{ attempt = $attempt; file = $name;
                bytes = (Get-Item -LiteralPath $actual).Length; sha256 = $digest;
                referenceSha256 = $expectedDigest; matchesAcceptedReference = $digest -ceq $expectedDigest }
        }
    } else {
        $outputs += [ordered]@{
            file = [IO.Path]::GetFileName($destination)
            bytes = (Get-Item -LiteralPath $destination).Length
            sha256 = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    }
}
$repeatable = if ($pairReference) {
    @((0..($pairFiles.Count - 1)) | Where-Object {
        $outputs[$_].sha256 -cne $outputs[$_ + $pairFiles.Count].sha256
    }).Count -eq 0
} else { $outputs[0].sha256 -ceq $outputs[1].sha256 }
$matchesReference = if ($pairReference) {
    @($outputs | Where-Object { -not $_.matchesAcceptedReference }).Count -eq 0
} else { $null }
$report = [ordered]@{
    schema = 1; blender = $version; threads = 1; inputs = $inputs; outputs = $outputs
    byteRepeatable = $repeatable
    matchesAcceptedReference = $matchesReference
    acceptedPairReference = $pairReference
    scope = 'Offline regeneration only. Does not replace runtime assets or establish new visual/device acceptance.'
}
$report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $outputRoot 'regeneration.json') -Encoding utf8
if (-not $repeatable) { throw 'Clean generated GLBs differ; deterministic regeneration gate remains open.' }
if ($pairReference -and -not $matchesReference) { throw 'Repeatable generated pair differs from the accepted reference.' }
Write-Output $(if ($pairReference) { 'Both regenerated world/view/manifest sets exactly match the accepted reference.' }
              else { "Two clean player exports match: $($outputs[0].sha256)" })
Write-Output (Join-Path $outputRoot 'regeneration.json')
