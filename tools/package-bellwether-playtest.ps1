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
if (!$executableIdentity.Contains("g:$revision")) { throw 'Build the Windows playtest from the current source commit.' }

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
foreach ($line in @(
    'kit.accept_base.limit','kit.accept_base.work','kit.accept_base.sit','hub.kit_introduce','hub.hoard_answer',
    'kit.approach_injury.stop','kit.approach_injury.ankle','kit.approach_injury.slow','hub.arrival_chair','hub.horde_army',
    'access.body_task','access.body_progress','hub.passing.learned_retreat'
)) {
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
    archive = $archive
    sha256 = (Get-FileHash -LiteralPath $archive).Hash.ToLowerInvariant()
    bytes = (Get-Item -LiteralPath $archive).Length
    residentFiles = 25
    voiceFiles = 13
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'windows-package.json')
Write-Output $output
