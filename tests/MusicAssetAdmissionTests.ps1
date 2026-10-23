$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $repoRoot 'tools/music-asset-policy.ps1')

$musicSource = Join-Path $repoRoot 'assets/audio/music/what-the-dark-keeps'
$outdoorMusicSource = Join-Path $repoRoot 'assets/audio/music/beneath-the-same-moon'
$scratchRoot = Join-Path ([IO.Path]::GetTempPath()) ('Horde-MusicAssetAdmission-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $scratchRoot | Out-Null
$script:passed = 0
$script:failed = 0

function Join-TestRelativePath {
    param([Parameter(Mandatory = $true)][string]$BasePath, [Parameter(Mandatory = $true)][string]$RelativePath)
    $path = [IO.Path]::GetFullPath($BasePath)
    foreach ($segment in ($RelativePath -split '[/\\]')) {
        if ($segment.Length -gt 0) { $path = Join-Path $path $segment }
    }
    return $path
}

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Copy-MusicFixture {
    param([Parameter(Mandatory = $true)][string]$Name)
    $fixtureRepo = Join-Path (Join-Path $scratchRoot $Name) 'repo'
    $fixtureMusic = Join-TestRelativePath -BasePath $fixtureRepo -RelativePath 'assets/audio/music/what-the-dark-keeps'
    New-Item -ItemType Directory -Path $fixtureMusic -Force | Out-Null
    foreach ($directory in Get-ChildItem -LiteralPath $musicSource -Recurse -Directory -Force) {
        $relative = $directory.FullName.Substring($musicSource.Length + 1)
        New-Item -ItemType Directory -Path (Join-Path $fixtureMusic $relative) -Force | Out-Null
    }
    foreach ($file in Get-ChildItem -LiteralPath $musicSource -Recurse -File -Force) {
        $relative = $file.FullName.Substring($musicSource.Length + 1)
        $destination = Join-Path $fixtureMusic $relative
        $parent = Split-Path -Parent $destination
        if (-not (Test-Path -LiteralPath $parent -PathType Container)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
        Copy-Item -LiteralPath $file.FullName -Destination $destination
    }
    return [pscustomobject]@{ RepositoryRoot = $fixtureRepo; MusicRoot = $fixtureMusic }
}

function Expect-Failure {
    param(
        [Parameter(Mandatory = $true)][scriptblock]$Action,
        [Parameter(Mandatory = $true)][string]$Label,
        [string]$ExpectedText
    )
    $message = ''
    try { & $Action | Out-Null }
    catch { $message = $_.Exception.Message }
    if ([string]::IsNullOrWhiteSpace($message)) { throw "$Label unexpectedly passed." }
    if ($ExpectedText -and $message -notlike "*$ExpectedText*") { throw "$Label failed for an unexpected reason: $message" }
    $script:passed++
    Write-Output "PASS negative: $Label"
}

function Copy-OutdoorMusicFixture {
    param([Parameter(Mandatory = $true)][string]$Name)
    $root = Join-Path $scratchRoot $Name
    New-Item -ItemType Directory -Path $root | Out-Null
    Copy-Item -LiteralPath (Join-Path $outdoorMusicSource 'asset.manifest.json') -Destination $root
    Copy-Item -LiteralPath (Join-Path $outdoorMusicSource 'METADATA.md') -Destination $root
    Copy-Item -LiteralPath (Join-Path $outdoorMusicSource 'runtime') -Destination $root -Recurse
    return $root
}

function Test-AssetPolicy {
    param([scriptblock]$Action, [string]$Label)
    try { & $Action | Out-Null; $script:passed++; Write-Output "PASS: $Label" }
    catch { $script:failed++; Write-Output "FAIL: $Label — $($_.Exception.Message)" }
}

function Add-ZipEntryFromFile {
    param([IO.Compression.ZipArchive]$Archive, [string]$EntryName, [string]$SourcePath, [switch]$Corrupt)
    $entry = $Archive.CreateEntry($EntryName, [IO.Compression.CompressionLevel]::Optimal)
    $destination = $entry.Open()
    try {
        if ($Corrupt) {
            $bytes = [IO.File]::ReadAllBytes($SourcePath)
            if ($bytes.Length -lt 45) { throw "Test WAV too short to corrupt: $SourcePath" }
            $bytes[44] = $bytes[44] -bxor 1
            $destination.Write($bytes, 0, $bytes.Length)
        } else {
            $source = [IO.File]::OpenRead($SourcePath)
            try { $source.CopyTo($destination) } finally { $source.Dispose() }
        }
    } finally { $destination.Dispose() }
}

function New-MusicTestArchive {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$AssetRoot,
        [string]$OmitPath,
        [string]$ExtraPath,
        [string]$DuplicatePath,
        [string]$CorruptPath
    )
    if (Test-Path -LiteralPath $Path) { throw "Refusing to overwrite test archive: $Path" }
    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $file = [IO.File]::Open($Path, [IO.FileMode]::CreateNew, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    $zip = [IO.Compression.ZipArchive]::new($file, [IO.Compression.ZipArchiveMode]::Create, $false)
    try {
        foreach ($relative in (Get-HordeMusicRuntimeFiles -RepositoryRoot $repoRoot)) {
            if ($relative -ceq $OmitPath) { continue }
            $source = Join-TestRelativePath -BasePath $AssetRoot -RelativePath $relative
            $corrupt = $relative -ceq $CorruptPath
            Add-ZipEntryFromFile -Archive $zip -EntryName "assets/$relative" -SourcePath $source -Corrupt:$corrupt
            if ($relative -ceq $DuplicatePath) {
                Add-ZipEntryFromFile -Archive $zip -EntryName "assets/$relative" -SourcePath $source
            }
        }
        if ($ExtraPath) {
            $entry = $zip.CreateEntry("assets/$ExtraPath", [IO.Compression.CompressionLevel]::Optimal)
            $stream = $entry.Open()
            try { $data = [Text.Encoding]::UTF8.GetBytes('source is never packaged'); $stream.Write($data, 0, $data.Length) }
            finally { $stream.Dispose() }
        }
    } finally { $zip.Dispose() }
}

try {
    Test-AssetPolicy -Label 'canonical source/runtime admission and manifest pin' -Action {
        $manifest = Assert-HordeMusicAssets -RepositoryRoot $repoRoot
        Assert-True ($manifest.assetId -ceq 'what-the-dark-keeps') 'Unexpected admitted music asset ID.'
    }
    Test-AssetPolicy -Label 'strict-mode legacy seventeen, outdoor two and aggregate nineteen-file runtime rosters' -Action {
        # Release/version tools enable strict mode before calling this helper.
        # A clean stage has zero reparse points and must not dereference null.
        Set-StrictMode -Version Latest
        $paths = @(Get-HordeLegacyMusicRuntimeFiles -RepositoryRoot $repoRoot)
        Assert-True ($paths.Count -eq 17) "Runtime roster count was $($paths.Count), expected 17."
        $outdoorPaths = @(Get-HordeOutdoorMusicRuntimeFiles -RepositoryRoot $repoRoot)
        Assert-True ($outdoorPaths.Count -eq 2) 'Outdoor runtime roster must contain only its manifest and full main WAV.'
        $allPaths = @(Get-HordeMusicRuntimeFiles -RepositoryRoot $repoRoot)
        Assert-True ($allPaths.Count -eq 19) 'Aggregate runtime music roster must contain exactly nineteen files.'
        Assert-True (@($allPaths | Where-Object { $_ -match 'source|\.flac$|\.zip$|dialogue-bed|quiet' }).Count -eq 0) 'Runtime roster contains a source or quiet variant.'
        $assets = Join-Path (Join-Path $scratchRoot 'stage-positive') 'assets'
        Copy-HordeMusicRuntimeAssets -RepositoryRoot $repoRoot -AssetRoot $assets | Out-Null
        $musicRoot = Join-TestRelativePath -BasePath $assets -RelativePath 'audio/music/what-the-dark-keeps'
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $musicRoot 'source'))) 'Staging leaked canonical sources.'
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $musicRoot 'METADATA.md'))) 'Staging copied project metadata.'
        $actual = @(Get-ChildItem -LiteralPath $musicRoot -Recurse -File)
        Assert-True ($actual.Count -eq 17) "Staged music file count was $($actual.Count), expected 17."
        $outdoorRoot = Join-TestRelativePath -BasePath $assets -RelativePath 'audio/music/beneath-the-same-moon'
        Assert-True (@(Get-ChildItem -LiteralPath $outdoorRoot -Recurse -File).Count -eq 2) 'Staged outdoor tree must contain only two runtime entries.'
        Assert-True (-not (Test-Path -LiteralPath (Join-Path $outdoorRoot 'METADATA.md'))) 'Staging leaked outdoor source metadata.'
    }
    Test-AssetPolicy -Label 'positive Windows-style staged ZIP and Android-style APK validation' -Action {
        $assets = Join-Path (Join-Path $scratchRoot 'archive-positive') 'assets'
        Copy-HordeMusicRuntimeAssets -RepositoryRoot $repoRoot -AssetRoot $assets | Out-Null
        $zipPath = Join-Path (Join-Path $scratchRoot 'archive-positive') 'windows.zip'
        $apkPath = Join-Path (Join-Path $scratchRoot 'archive-positive') 'android.apk'
        New-MusicTestArchive -Path $zipPath -AssetRoot $assets
        New-MusicTestArchive -Path $apkPath -AssetRoot $assets
        Assert-HordeMusicPackage -RepositoryRoot $repoRoot -ArchivePath $zipPath | Out-Null
        Assert-HordeMusicPackage -RepositoryRoot $repoRoot -ArchivePath $apkPath | Out-Null
    }

    $fixture = Copy-MusicFixture -Name 'source-corrupt'
    $sourceMutation = [IO.File]::Open((Join-TestRelativePath $fixture.MusicRoot 'source/What_the_Dark_Keeps_Pocket_Chordsmith.json'), [IO.FileMode]::Open, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    try { $sourceMutation.Position = 24; $byte = $sourceMutation.ReadByte(); $sourceMutation.Position = 24; $sourceMutation.WriteByte([byte]($byte -bxor 1)) } finally { $sourceMutation.Dispose() }
    Expect-Failure -Label 'independent source-byte mutation' -ExpectedText 'source SHA-256 mismatch' -Action {
        Assert-HordeMusicAssets -RepositoryRoot $fixture.RepositoryRoot
    }

    $fixture = Copy-MusicFixture -Name 'pcm-corrupt'
    $pcmPath = Join-TestRelativePath $fixture.MusicRoot 'runtime/A-body.wav'
    $pcm = [IO.File]::Open($pcmPath, [IO.FileMode]::Open, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    try { $pcm.Position = 44; $value = $pcm.ReadByte(); $pcm.Position = 44; $pcm.WriteByte([byte]($value -bxor 1)) } finally { $pcm.Dispose() }
    Expect-Failure -Label 'independent PCM sample-byte mutation' -ExpectedText 'runtime SHA-256 mismatch' -Action {
        Assert-HordeMusicAssets -RepositoryRoot $fixture.RepositoryRoot
    }

    $fixture = Copy-MusicFixture -Name 'manifest-repin'
    $manifestPath = Join-Path $fixture.MusicRoot 'asset.manifest.json'
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $manifest.title = 'Mutated title with attempted manifest repin'
    $manifest | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $manifestPath -Encoding utf8
    Expect-Failure -Label 'manifest-content repin attempt' -ExpectedText 'manifest SHA-256 does not match the reviewed pin' -Action {
        Assert-HordeMusicAssets -RepositoryRoot $fixture.RepositoryRoot
    }

    $fixture = Copy-MusicFixture -Name 'looping-policy'
    $manifestPath = Join-Path $fixture.MusicRoot 'asset.manifest.json'
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $manifest.cues[2].looping = $true
    $manifest | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $manifestPath -Encoding utf8
    Expect-Failure -Label 'one-shot looping-policy mutation' -ExpectedText 'Cue C order or looping policy' -Action {
        Assert-HordeMusicAssets -RepositoryRoot $fixture.RepositoryRoot
    }

    $fixture = Copy-MusicFixture -Name 'missing-file'
    Remove-Item -LiteralPath (Join-TestRelativePath $fixture.MusicRoot 'runtime/H-tail.wav')
    Expect-Failure -Label 'missing runtime wave' -ExpectedText 'runtime wave is missing' -Action {
        Assert-HordeMusicAssets -RepositoryRoot $fixture.RepositoryRoot
    }

    $fixture = Copy-MusicFixture -Name 'extra-file'
    Set-Content -LiteralPath (Join-TestRelativePath $fixture.MusicRoot 'runtime/unrecognised.wav') -Value 'fixture only' -Encoding ascii
    Expect-Failure -Label 'extra runtime file' -ExpectedText 'closed inventory' -Action {
        Assert-HordeMusicAssets -RepositoryRoot $fixture.RepositoryRoot
    }

    $outdoor = Copy-OutdoorMusicFixture -Name 'outdoor-pcm-corrupt'
    $wave = [IO.File]::Open((Join-TestRelativePath $outdoor 'runtime/outdoor-main.wav'), [IO.FileMode]::Open, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    try { $wave.Position = 44; $value = $wave.ReadByte(); $wave.Position = 44; $wave.WriteByte([byte]($value -bxor 1)) } finally { $wave.Dispose() }
    Expect-Failure -Label 'outdoor independent PCM sample mutation' -ExpectedText 'runtime SHA-256 mismatch' -Action {
        Assert-HordeOutdoorMusicAssetTree -MusicRoot $outdoor
    }
    $outdoor = Copy-OutdoorMusicFixture -Name 'outdoor-tail-policy'
    $manifestPath = Join-Path $outdoor 'asset.manifest.json'
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $manifest.cues[0].tail = [pscustomobject]@{ path = 'runtime/outdoor-tail.wav'; frames = 144000 }
    $manifest | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $manifestPath -Encoding utf8
    Expect-Failure -Label 'outdoor separate-tail policy mutation' -ExpectedText 'without a separate tail or quiet mix' -Action {
        Assert-HordeOutdoorMusicAssetTree -MusicRoot $outdoor
    }
    $outdoor = Copy-OutdoorMusicFixture -Name 'outdoor-manifest-repin'
    $manifestPath = Join-Path $outdoor 'asset.manifest.json'
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $manifest.title = 'Unreviewed outdoor title'
    $manifest | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $manifestPath -Encoding utf8
    Expect-Failure -Label 'outdoor independent manifest pin' -ExpectedText 'manifest SHA-256 does not match the reviewed pin' -Action {
        Assert-HordeOutdoorMusicAssetTree -MusicRoot $outdoor
    }
    $outdoor = Copy-OutdoorMusicFixture -Name 'outdoor-quiet-extra'
    Set-Content -LiteralPath (Join-TestRelativePath $outdoor 'runtime/dialogue-bed.wav') -Value 'unapproved quiet file' -Encoding ascii
    Expect-Failure -Label 'outdoor quiet variant excluded from asset admission' -ExpectedText 'closed inventory' -Action {
        Assert-HordeOutdoorMusicAssetTree -MusicRoot $outdoor
    }
    $outdoor = Copy-OutdoorMusicFixture -Name 'outdoor-source-extra'
    Set-Content -LiteralPath (Join-Path $outdoor 'source.zip') -Value 'source is never runtime' -Encoding ascii
    Expect-Failure -Label 'outdoor source ZIP excluded from asset admission' -ExpectedText 'closed inventory' -Action {
        Assert-HordeOutdoorMusicAssetTree -MusicRoot $outdoor
    }
    $outdoor = Copy-OutdoorMusicFixture -Name 'outdoor-missing-wave'
    Remove-Item -LiteralPath (Join-TestRelativePath $outdoor 'runtime/outdoor-main.wav')
    Expect-Failure -Label 'outdoor missing main rejected' -ExpectedText 'runtime wave is missing' -Action {
        Assert-HordeOutdoorMusicAssetTree -MusicRoot $outdoor
    }

    $positiveStage = Join-Path (Join-Path $scratchRoot 'archive-negative') 'assets'
    Copy-HordeMusicRuntimeAssets -RepositoryRoot $repoRoot -AssetRoot $positiveStage | Out-Null
    $sourceZip = Join-Path (Join-Path $scratchRoot 'archive-negative') 'source-entry.zip'
    New-MusicTestArchive -Path $sourceZip -AssetRoot $positiveStage -ExtraPath 'audio/music/what-the-dark-keeps/source/score.json'
    Expect-Failure -Label 'archive source-entry rejection' -ExpectedText 'exactly 17 entries' -Action {
        Assert-HordeMusicPackage -RepositoryRoot $repoRoot -ArchivePath $sourceZip
    }
    $missingZip = Join-Path (Join-Path $scratchRoot 'archive-negative') 'missing-entry.zip'
    New-MusicTestArchive -Path $missingZip -AssetRoot $positiveStage -OmitPath 'audio/music/what-the-dark-keeps/runtime/H-tail.wav'
    Expect-Failure -Label 'archive missing-entry rejection' -ExpectedText 'exactly 17 entries' -Action {
        Assert-HordeMusicPackage -RepositoryRoot $repoRoot -ArchivePath $missingZip
    }
    $duplicateZip = Join-Path (Join-Path $scratchRoot 'archive-negative') 'duplicate-entry.zip'
    New-MusicTestArchive -Path $duplicateZip -AssetRoot $positiveStage -DuplicatePath 'audio/music/what-the-dark-keeps/runtime/A-body.wav'
    Expect-Failure -Label 'archive duplicate-entry rejection' -ExpectedText 'exactly 17 entries' -Action {
        Assert-HordeMusicPackage -RepositoryRoot $repoRoot -ArchivePath $duplicateZip
    }
    $sameCountDuplicateZip = Join-Path (Join-Path $scratchRoot 'archive-negative') 'same-count-duplicate-entry.zip'
    New-MusicTestArchive -Path $sameCountDuplicateZip -AssetRoot $positiveStage `
        -OmitPath 'audio/music/what-the-dark-keeps/runtime/H-tail.wav' `
        -DuplicatePath 'audio/music/what-the-dark-keeps/runtime/A-body.wav'
    Expect-Failure -Label 'archive same-count duplicate-entry rejection' -ExpectedText 'duplicate entry names' -Action {
        Assert-HordeMusicPackage -RepositoryRoot $repoRoot -ArchivePath $sameCountDuplicateZip
    }
    $corruptZip = Join-Path (Join-Path $scratchRoot 'archive-negative') 'corrupt-entry.zip'
    New-MusicTestArchive -Path $corruptZip -AssetRoot $positiveStage -CorruptPath 'audio/music/what-the-dark-keeps/runtime/A-body.wav'
    Expect-Failure -Label 'archive corrupted PCM rejection' -ExpectedText 'entry byte/hash mismatch' -Action {
        Assert-HordeMusicPackage -RepositoryRoot $repoRoot -ArchivePath $corruptZip
    }
    $outdoorMissingZip = Join-Path (Join-Path $scratchRoot 'archive-negative') 'outdoor-missing.zip'
    New-MusicTestArchive -Path $outdoorMissingZip -AssetRoot $positiveStage -OmitPath 'audio/music/beneath-the-same-moon/runtime/outdoor-main.wav'
    Expect-Failure -Label 'archive outdoor missing-main rejection' -ExpectedText 'exactly two main-only runtime entries' -Action {
        Assert-HordeMusicPackage -RepositoryRoot $repoRoot -ArchivePath $outdoorMissingZip
    }
    $outdoorCorruptZip = Join-Path (Join-Path $scratchRoot 'archive-negative') 'outdoor-corrupt.zip'
    New-MusicTestArchive -Path $outdoorCorruptZip -AssetRoot $positiveStage -CorruptPath 'audio/music/beneath-the-same-moon/runtime/outdoor-main.wav'
    Expect-Failure -Label 'archive outdoor corrupted-main rejection' -ExpectedText 'Outdoor music package entry byte/hash mismatch' -Action {
        Assert-HordeMusicPackage -RepositoryRoot $repoRoot -ArchivePath $outdoorCorruptZip
    }
    $outdoorQuietZip = Join-Path (Join-Path $scratchRoot 'archive-negative') 'outdoor-quiet.zip'
    New-MusicTestArchive -Path $outdoorQuietZip -AssetRoot $positiveStage -ExtraPath 'audio/music/beneath-the-same-moon/runtime/dialogue-bed.wav'
    Expect-Failure -Label 'archive outdoor quiet-copy rejection' -ExpectedText 'exactly two main-only runtime entries' -Action {
        Assert-HordeMusicPackage -RepositoryRoot $repoRoot -ArchivePath $outdoorQuietZip
    }
    $outdoorSourceZip = Join-Path (Join-Path $scratchRoot 'archive-negative') 'outdoor-source-original.zip'
    New-MusicTestArchive -Path $outdoorSourceZip -AssetRoot $positiveStage -ExtraPath 'source/beyond_the_tomb/music/beneath-the-same-moon-outdoor-r2/source.zip'
    Expect-Failure -Label 'archive outdoor original-source rejection outside runtime root' -ExpectedText 'unapproved music/source content' -Action {
        Assert-HordeMusicPackage -RepositoryRoot $repoRoot -ArchivePath $outdoorSourceZip
    }
    $outdoorSourceElsewhereZip = Join-Path (Join-Path $scratchRoot 'archive-negative') 'outdoor-source-elsewhere.zip'
    New-MusicTestArchive -Path $outdoorSourceElsewhereZip -AssetRoot $positiveStage -ExtraPath 'unrelated/Beneath-the-Same-Moon-Outdoor-r2-loop.flac'
    Expect-Failure -Label 'archive original FLAC rejection even under an unrelated package folder' -ExpectedText 'unapproved music/source content' -Action {
        Assert-HordeMusicPackage -RepositoryRoot $repoRoot -ArchivePath $outdoorSourceElsewhereZip
    }
} catch {
    $script:failed++
    Write-Output "FAIL: $($_.Exception.Message)"
} finally {
    Write-Output "Music asset admission tests: $script:passed passed, $script:failed failed."
    Write-Output "Independent copies and ZIP fixtures retained under: $scratchRoot"
}

if ($script:failed -ne 0) { exit 1 }
