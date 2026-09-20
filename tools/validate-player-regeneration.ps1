[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [string]$BlenderExecutable = 'blender'
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
foreach ($attempt in 1..2) {
    $destination = Join-Path $outputRoot ("attempt-$attempt.glb")
    $log = Join-Path $outputRoot ("attempt-$attempt.log")
    # Blender's parallel export changed tangent rounding and unique-vertex order
    # between clean runs. Pin the observed repeatable single-thread invocation.
    & $blender --background --threads 1 --python $processor -- $rig $walk $pbr $gauntlet $destination > $log 2>&1
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $destination)) {
        throw "Player regeneration attempt $attempt failed; inspect $log"
    }
    $outputs += [ordered]@{
        file = [IO.Path]::GetFileName($destination)
        bytes = (Get-Item -LiteralPath $destination).Length
        sha256 = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}
$repeatable = $outputs[0].sha256 -ceq $outputs[1].sha256
$report = [ordered]@{
    schema = 1; blender = $version; threads = 1; inputs = $inputs; outputs = $outputs
    byteRepeatable = $repeatable
    scope = 'Offline regeneration only. Does not replace runtime assets or prove visual/device acceptance.'
}
$report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $outputRoot 'regeneration.json') -Encoding utf8
if (-not $repeatable) { throw 'Clean generated GLBs differ; deterministic regeneration gate remains open.' }
Write-Output ("Two clean player exports match: " + $outputs[0].sha256)
Write-Output (Join-Path $outputRoot 'regeneration.json')
