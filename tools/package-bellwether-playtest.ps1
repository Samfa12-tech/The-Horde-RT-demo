[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$WindowsExecutable,
    [Parameter(Mandatory)][string]$BaseWindowsZip,
    [Parameter(Mandatory)][string]$ResidentDirectory,
    [Parameter(Mandatory)][string]$VoiceDirectory
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
$voices = (Resolve-Path -LiteralPath $VoiceDirectory).Path
$revision = (& git -C $repo rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $revision -notmatch '^[0-9a-f]{40}$') { throw 'Could not identify the playtest source.' }
$executableIdentity = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($executable))
if ($executableIdentity -cnotmatch "g:$revision;i:[0-9a-f]{64};c:D;s:C") {
    throw 'Build the Windows playtest in Debug from the current clean source commit.'
}
$voiceLines = @(
    'kit.accept_base.limit','kit.accept_base.work','kit.accept_base.sit','hub.kit_introduce','hub.hoard_answer',
    'kit.approach_injury.stop','kit.approach_injury.ankle','kit.approach_injury.slow','hub.arrival_chair','hub.horde_army',
    'access.body_task','access.body_progress','hub.passing.learned_retreat'
)
$followOn = @('hub.two_routes','hub.routes_independent','kit.voice_reaction','kit.first_direct_voice.finish',
    'kit.royal_claim.help','hub.ordinary_sale')
$presentFollowOn = @($followOn | Where-Object { Test-Path -LiteralPath (Join-Path $voices "$_.wav") })
if ($presentFollowOn.Count -gt 0) {
    if ($presentFollowOn.Count -ne $followOn.Count) { throw 'The follow-on audition needs all six prepared WAVs.' }
    $voiceLines += $followOn
}
$hearth = @('hub.kit_stake','kit.personal_stake.key')
$presentHearth = @($hearth | Where-Object { Test-Path -LiteralPath (Join-Path $voices "$_.wav") })
if ($presentHearth.Count -gt 0) {
    if ($presentHearth.Count -ne $hearth.Count) { throw 'The Kit hearth audition needs both prepared WAVs.' }
    $hearthHashes = @{
        'hub.kit_stake' = '9f7c15c99489614579fc6f7e69845a2d3d83d46f5d75845d55464d3795c26b82'
        'kit.personal_stake.key' = 'd82e1198d35124ffb23c25633141597f1c2bb4ee431a1ee300923b124d6caeb0'
    }
    foreach ($line in $hearth) {
        if ((Get-FileHash -LiteralPath (Join-Path $voices "$line.wav")).Hash.ToLowerInvariant() -ne $hearthHashes[$line]) {
            throw "The Kit hearth audition differs from the exact reviewed take: $line"
        }
    }
    $voiceLines += $hearth
}

$optionalVoices = [ordered]@{
    'hub.first_errand' = '7ddf64e532156ee4d46f50858f18c9362a748b7c98c67d8c564c9fcbe66291d8'
    'hub.shrine_direction' = '25f310416c92fd6011550960bda57fd4c2070ba92beb18cfd0dc6e17cecb4e44'
}
foreach ($line in $optionalVoices.Keys) {
    $path = Join-Path $voices "$line.wav"
    if (Test-Path -LiteralPath $path) {
        if ((Get-FileHash -LiteralPath $path).Hash -ne $optionalVoices[$line]) {
            throw "The private voice differs from its reviewed take: $line"
        }
        $voiceLines += $line
    }
}

$output = (New-Item -ItemType Directory -Path (Join-Path $repo (
    'reports/horde-bellwether-playtest-' + (Get-Date -Format 'yyyyMMdd-HHmmss')))).FullName
$package = (New-Item -ItemType Directory -Path (Join-Path $output 'Bellwether Playtest')).FullName
$assetRoot = (New-Item -ItemType Directory -Path (Join-Path $package 'assets')).FullName

function Copy-PlaytestInput([string]$source, [string]$target) {
    $null = New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force
    Copy-Item -LiteralPath $source -Destination $target
}

$activeRoster = @(
    Get-Horde162RuntimeFiles $repo Windows
    Get-HordeNightAudioRuntimeFiles
    Get-HordeMusicRuntimeFiles -RepositoryRoot $repo
)
$baseArchive = [IO.Compression.ZipFile]::OpenRead($base)
try {
    foreach ($entry in $baseArchive.Entries) {
        $relative = $entry.FullName
        if ($relative.EndsWith('/')) { continue }
        # The 1.8 town GLB in the current roster replaces this 1.7 skyline.
        if ($relative.StartsWith('assets/models/world/runtime/bellwether-shell-v4/')) { continue }
        if ($relative.StartsWith('assets/')) {
            if ($relative.Substring(7) -in $activeRoster) { continue }
            Copy-PlaytestInput (Join-Path $repo $relative) (Join-Path $package $relative)
        } elseif ($relative -in $script:Horde17ReleaseFiles.Keys) {
            $target = Join-Path $package $relative
            $null = New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force
            [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $target)
        }
    }
} finally {
    $baseArchive.Dispose()
}

$null = Copy-Horde162RuntimeAssets -RepositoryRoot $repo -AssetRoot $assetRoot -Platform Windows
Copy-HordeNightAudioRuntimeAssets -RepositoryRoot $repo -AssetRoot $assetRoot
$null = Copy-HordeMusicRuntimeAssets -RepositoryRoot $repo -AssetRoot $assetRoot
Copy-HordeThirdPartyNotices -RepositoryRoot $repo -PackageRoot $package
Copy-PlaytestInput (Join-Path $repo 'ASSET_LICENSES.md') (Join-Path $package 'ASSET_LICENSES.md')

foreach ($role in @('tavern-keeper','record-keeper','artificer','salvager','relic-dealer')) {
    foreach ($leaf in @('asset.manifest.json','resident.glb','base-color.windows.ktx2','normal.windows.ktx2','orm.windows.ktx2')) {
        $relative = "$role/$leaf"
        Copy-PlaytestInput (Join-Path $residents $relative) (Join-Path $assetRoot "private_bellwether/$relative")
    }
}
foreach ($line in $voiceLines) {
    Copy-PlaytestInput (Join-Path $voices "$line.wav") (Join-Path $assetRoot "private_bellwether_voice/$line.wav")
}
Copy-PlaytestInput $executable (Join-Path $package 'HordeLanternRT.exe')
Set-Content -LiteralPath (Join-Path $package 'Bellwether.playtest') -Value 'Private Bellwether playtest'
@'
Horde Lantern RT — private Bellwether 1.8 playtest (Windows x64)

Extract the whole ZIP to a writable folder, then open HordeLanternRT.exe.
Choose New Game to play from the beginning. Load shows only the Windows
Bellwether playtest slots. Saves live in Local AppData under
HordeLanternRT/bellwether-playtest/campaign-v1. Settings live beside the game.

A Vulkan hardware ray-tracing GPU and driver are required.
This build and its resident and voice assets are for private playtesting.
'@ | Set-Content -LiteralPath (Join-Path $package 'PLAYTEST.txt') -Encoding utf8

$archive = Join-Path $output 'Horde-Bellwether-Private-Playtest-Windows-x64.zip'
[IO.Compression.ZipFile]::CreateFromDirectory($package, $archive, [IO.Compression.CompressionLevel]::Fastest, $false)
Assert-Horde17ReleasePackage -ArchivePath $archive -Platform Windows
Assert-Horde162Package -RepositoryRoot $repo -ArchivePath $archive -Platform Windows
$null = Assert-HordeMusicPackage -RepositoryRoot $repo -ArchivePath $archive
Assert-HordeThirdPartyNoticesPackage -RepositoryRoot $repo -ArchivePath $archive -Platform Windows
[ordered]@{
    sourceCommit = $revision
    foundationZipSha256 = $expectedBase
    executableSha256 = (Get-FileHash -LiteralPath $executable).Hash.ToLowerInvariant()
    stagingScriptSha256 = (Get-FileHash -LiteralPath $PSCommandPath).Hash.ToLowerInvariant()
    archive = $archive
    sha256 = (Get-FileHash -LiteralPath $archive).Hash.ToLowerInvariant()
    bytes = (Get-Item -LiteralPath $archive).Length
    residentFiles = 25
    voiceFiles = $voiceLines.Count
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'windows-package.json')
Write-Output $output
