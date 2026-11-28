[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$WindowsExecutable,
    [Parameter(Mandatory)][string]$BaseWindowsZip,
    [Parameter(Mandatory)][string]$ResidentDirectory,
    [string]$VoiceDirectory,
    [switch]$OptimizedCandidate,
    [switch]$ArchiveOnly
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'horde-1.6.2-asset-policy.ps1')
. (Join-Path $PSScriptRoot 'horde-1.7-release-asset-policy.ps1')
. (Join-Path $PSScriptRoot 'horde-night-audio-policy.ps1')
. (Join-Path $PSScriptRoot 'music-asset-policy.ps1')
. (Join-Path $PSScriptRoot 'third-party-notice-policy.ps1')

# This immutable input supplies the foundation roster and the three existing
# game-package derivatives. The published executable never enters the preview.
$expectedBase = 'e06a7a9d67607586447fbaaa46e60d5465de3c19fc99d22aefffd5d1f28dd277'
$base = (Resolve-Path -LiteralPath $BaseWindowsZip).Path
if ((Get-FileHash -LiteralPath $base).Hash.ToLowerInvariant() -ne $expectedBase) {
    throw 'Expected the exact published Windows 1.7.0 archive as the foundation input.'
}
$executable = (Resolve-Path -LiteralPath $WindowsExecutable).Path
$residents = (Resolve-Path -LiteralPath $ResidentDirectory).Path
if (!$OptimizedCandidate -and !$VoiceDirectory) { throw 'The Debug playtest requires its prepared voice directory.' }
$voices = if ($VoiceDirectory) { (Resolve-Path -LiteralPath $VoiceDirectory).Path }
$revision = (& git -C $repo rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $revision -notmatch '^[0-9a-f]{40}$') { throw 'Could not identify the playtest source.' }
$executableIdentity = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($executable))
if ($OptimizedCandidate) {
    if ($executableIdentity -cnotmatch "g:$revision;i:[0-9a-f]{64};c:R;s:C" -or
        !$executableIdentity.Contains('horde-bellwether-optimized-v1')) {
        throw 'Build the optimized Windows candidate in Release from the current clean source commit.'
    }
} elseif ($executableIdentity -cnotmatch "g:$revision;i:[0-9a-f]{64};c:D;s:C") {
    throw 'Build the Windows playtest in Debug from the current clean source commit.'
}
$voiceLines = @()
if ($voices) {
    $voiceLines = @('kit.accept_base.limit','kit.accept_base.work','kit.accept_base.sit','hub.kit_introduce','hub.hoard_answer')
    $voiceGroups = [ordered]@{
        arrival = @('kit.approach_injury.stop','kit.approach_injury.ankle','kit.approach_injury.slow','hub.arrival_chair','hub.horde_army')
        shore = @('access.body_task','access.body_progress','hub.passing.learned_retreat')
        'follow-on' = @('hub.two_routes','hub.routes_independent','kit.voice_reaction','kit.first_direct_voice.finish',
            'kit.royal_claim.help','hub.ordinary_sale')
        hearth = @('hub.kit_stake','kit.personal_stake.key')
    }
    foreach ($name in $voiceGroups.Keys) {
        $group = $voiceGroups[$name]
        $present = @($group | Where-Object { Test-Path -LiteralPath (Join-Path $voices "$_.wav") -PathType Leaf })
        if ($present.Count -gt 0 -or (!$OptimizedCandidate -and $name -in 'arrival','shore')) {
            if ($present.Count -ne $group.Count) { throw "The $name audition needs every prepared WAV in its group." }
            $voiceLines += $group
        }
    }

    $pinnedWavs = [ordered]@{
        'hub.kit_stake' = '9f7c15c99489614579fc6f7e69845a2d3d83d46f5d75845d55464d3795c26b82'
        'kit.personal_stake.key' = 'd82e1198d35124ffb23c25633141597f1c2bb4ee431a1ee300923b124d6caeb0'
        'hub.first_errand' = '7ddf64e532156ee4d46f50858f18c9362a748b7c98c67d8c564c9fcbe66291d8'
        'hub.shrine_direction' = '25f310416c92fd6011550960bda57fd4c2070ba92beb18cfd0dc6e17cecb4e44'
        'lantern.first_identity' = '6aae2d8fbc39f78ff6da80c9ef3ebd647822f750673d6c206cdc505b5a8cde6f'
        'lantern.first_request' = '7865d1ebaf81329338bee3d06961c98750b7f651e86a23ad01c85c9a2dd69083'
        'lantern.town_seals' = '78580a62a2969870b0820f976753df70e1888e66ac8ab01633585dd4d2e1dd88'
    }
    foreach ($line in $pinnedWavs.Keys) {
        $path = Join-Path $voices "$line.wav"
        if (Test-Path -LiteralPath $path) {
            if ((Get-FileHash -LiteralPath $path).Hash -ne $pinnedWavs[$line]) {
                throw "The private voice differs from its reviewed take: $line"
            }
            if ($line -notin $voiceLines) { $voiceLines += $line }
        }
    }
}

$profile = if ($OptimizedCandidate) { 'Candidate' } else { 'Playtest' }
$output = (New-Item -ItemType Directory -Path (Join-Path $repo (
    "reports/horde-bellwether-$($profile.ToLowerInvariant())-" + (Get-Date -Format 'yyyyMMdd-HHmmss')))).FullName
$archive = Join-Path $output "Horde-Bellwether-Private-$profile-Windows-x64.zip"
$pending = "$archive.pending"
$entryNames = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$compression = [IO.Compression.CompressionLevel]::Fastest

function Register-PlaytestEntry([string]$relative) {
    if (!$relative -or $relative.Contains('\') -or $relative.Contains(':') -or
        $relative.StartsWith('/') -or $relative.EndsWith('/') -or
        $relative -match '(^|/)(\.|\.\.)(/|$)' -or !$entryNames.Add($relative)) {
        throw "Invalid or duplicate Bellwether archive entry: $relative"
    }
}

function Add-PlaytestFile([string]$source, [string]$relative) {
    Register-PlaytestEntry $relative
    $null = [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $source, $relative, $compression)
}

function Add-PlaytestText([string]$relative, [string]$content) {
    Register-PlaytestEntry $relative
    $entry = $zip.CreateEntry($relative, $compression)
    $writer = [IO.StreamWriter]::new($entry.Open(), [Text.UTF8Encoding]::new($false))
    try { $writer.WriteLine($content) }
    finally { $writer.Dispose() }
}

$activeRoster = @(
    Get-Horde162RuntimeFiles $repo Windows
    Get-HordeNightAudioRuntimeFiles
    Get-HordeMusicRuntimeFiles -RepositoryRoot $repo
)
$zip = [IO.Compression.ZipFile]::Open($pending, [IO.Compression.ZipArchiveMode]::Create)
try {
    $baseArchive = [IO.Compression.ZipFile]::OpenRead($base)
    try {
        foreach ($entry in $baseArchive.Entries) {
            $relative = $entry.FullName
            if ($relative.EndsWith('/')) { continue }
            # The 1.8 town GLB in the current roster replaces this 1.7 skyline.
            if ($relative.StartsWith('assets/models/world/runtime/bellwether-shell-v4/')) { continue }
            if ($relative.StartsWith('assets/')) {
                if ($relative.Substring(7) -in $activeRoster) { continue }
                Add-PlaytestFile (Join-Path $repo $relative) $relative
            } elseif ($relative -in $script:Horde17ReleaseFiles.Keys) {
                Register-PlaytestEntry $relative
                $source = $entry.Open()
                try {
                    $destination = $zip.CreateEntry($relative, $compression).Open()
                    try { $source.CopyTo($destination) }
                    finally { $destination.Dispose() }
                } finally { $source.Dispose() }
            }
        }
    } finally { $baseArchive.Dispose() }

    foreach ($relative in $activeRoster) {
        Add-PlaytestFile (Join-Path $repo "assets/$relative") "assets/$relative"
    }
    foreach ($notice in Get-HordeSoftwarePackageNotices) {
        Add-PlaytestFile (Join-Path $repo $notice.Source) $notice.Destination
    }
    Add-PlaytestFile (Join-Path $repo 'ASSET_LICENSES.md') 'ASSET_LICENSES.md'

    foreach ($role in @('tavern-keeper','record-keeper','artificer','salvager','relic-dealer')) {
        foreach ($leaf in @('asset.manifest.json','resident.glb','base-color.windows.ktx2','normal.windows.ktx2','orm.windows.ktx2')) {
            $relative = "$role/$leaf"
            Add-PlaytestFile (Join-Path $residents $relative) "assets/private_bellwether/$relative"
        }
    }
    foreach ($line in $voiceLines) {
        Add-PlaytestFile (Join-Path $voices "$line.wav") "assets/private_bellwether_voice/$line.wav"
    }
    Add-PlaytestFile $executable 'HordeLanternRT.exe'
    if ($OptimizedCandidate) {
        Add-PlaytestText 'Bellwether.candidate' 'horde-bellwether-optimized-v1'
        $saveLocation = 'The candidate saves in state/campaign-v1 beside the game.'
    } else {
        Add-PlaytestText 'Bellwether.playtest' 'Private Bellwether playtest'
        $saveLocation = 'The playtest saves in Local AppData under HordeLanternRT/bellwether-playtest/campaign-v1.'
    }
    Add-PlaytestText 'PLAYTEST.txt' @"
Horde Lantern RT — private Bellwether 1.8 $($profile.ToLowerInvariant()) (Windows x64)

Extract the whole ZIP to a writable folder, then open HordeLanternRT.exe.
Choose New Game to play from the beginning. $saveLocation
Settings live beside the game.

A Vulkan hardware ray-tracing GPU and driver are required.
This build and its resident and voice assets are for private playtesting.
"@
} finally { $zip.Dispose() }

Assert-Horde17ReleasePackage -ArchivePath $pending -Platform Windows
Assert-Horde162Package -RepositoryRoot $repo -ArchivePath $pending -Platform Windows
$null = Assert-HordeMusicPackage -RepositoryRoot $repo -ArchivePath $pending
Assert-HordeThirdPartyNoticesPackage -RepositoryRoot $repo -ArchivePath $pending -Platform Windows
if (!$ArchiveOnly) {
    [IO.Compression.ZipFile]::ExtractToDirectory($pending, (Join-Path $output "Bellwether $profile"))
}
Move-Item -LiteralPath $pending -Destination $archive
[ordered]@{
    sourceCommit = $revision
    profile = $profile.ToLowerInvariant()
    foundationZipSha256 = $expectedBase
    executableSha256 = (Get-FileHash -LiteralPath $executable).Hash.ToLowerInvariant()
    stagingScriptSha256 = (Get-FileHash -LiteralPath $PSCommandPath).Hash.ToLowerInvariant()
    archive = $archive
    sha256 = (Get-FileHash -LiteralPath $archive).Hash.ToLowerInvariant()
    bytes = (Get-Item -LiteralPath $archive).Length
    residentFiles = 25
    voiceFiles = $voiceLines.Count
    archiveOnly = [bool]$ArchiveOnly
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'windows-package.json')
Write-Output $output
