#requires -Version 7.0
param(
    [Parameter(Mandatory = $true)][string]$StageRoot
)

$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$stage = [IO.Path]::GetFullPath($StageRoot)

function Get-PinnedHash {
    param([string]$Source, [string]$Symbol)
    $contents = Get-Content -LiteralPath (Join-Path $repoRoot $Source) -Raw
    $matches = [regex]::Matches($contents, [regex]::Escape($Symbol) + '="([0-9a-f]{64})"')
    if ($matches.Count -ne 1) { throw "Expected exactly one pinned $Symbol in $Source." }
    return $matches[0].Groups[1].Value
}

$required = @(
    @{ Path = 'private_flask/player.glb'; Source = 'src/gameplay/simulation/PrivateRevivalApplication.h'; Symbol = 'kFlaskCandidateSha256' },
    @{ Path = 'private_revival/skeleton.glb'; Source = 'src/gameplay/simulation/PrivateRevivalApplication.h'; Symbol = 'kPrivateStandSha256' },
    @{ Path = 'private_revival/lich_revival.wav'; Source = 'src/audio/LichRevivalAudio.h'; Symbol = 'kLichRevivalWaveSha256' }
)

if (-not (Test-Path -LiteralPath (Join-Path $stage 'HordeLanternRT.exe') -PathType Leaf)) {
    throw "Private acceptance stage has no HordeLanternRT.exe: $stage"
}
foreach ($entry in $required) {
    $relative = $entry.Path.Replace('/', [IO.Path]::DirectorySeparatorChar)
    $path = Join-Path $stage $relative
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Private acceptance stage is missing $($entry.Path): $stage"
    }
    $expected = Get-PinnedHash -Source $entry.Source -Symbol $entry.Symbol
    $actual = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $expected) {
        throw "Private acceptance stage has an unapproved $($entry.Path) (SHA-256 mismatch)."
    }
}
Write-Output 'Private Windows acceptance stage has the three pinned flask/revival payloads.'
