$script:HordeMusicManifestSha256 = '672f2321b2fb429cffe9d61119103a1aa3565f355391a162497f741eec9d6e29'
$script:HordeMusicAssetRelativeRoot = 'assets/audio/music/what-the-dark-keeps'
$script:HordeMusicAssetRootRelativeToAssets = 'audio/music/what-the-dark-keeps'
$script:HordeMusicRights = 'Owner-supplied; authorised for Horde use only'
$script:HordeOutdoorMusicManifestSha256 = '5318990386a47c7ef815cfcd5165e779bd01b0994cdca3f76926882d1d4e1657'
$script:HordeOutdoorMusicAssetRelativeRoot = 'assets/audio/music/beneath-the-same-moon'
$script:HordeOutdoorMusicAssetRootRelativeToAssets = 'audio/music/beneath-the-same-moon'
$script:HordeOutdoorMusicWaveSha256 = '14fe0a94e665cf25b4fb4aa08a45e26e18990794740b670906adfecfaf6aee15'

function Get-HordeMusicCueSpecification {
    @(
        [pscustomobject]@{ Cue = 'A'; Looping = $true; BodyFrames = 576000 },
        [pscustomobject]@{ Cue = 'B'; Looping = $true; BodyFrames = 576000 },
        [pscustomobject]@{ Cue = 'C'; Looping = $false; BodyFrames = 144000 },
        [pscustomobject]@{ Cue = 'D'; Looping = $true; BodyFrames = 576000 },
        [pscustomobject]@{ Cue = 'E'; Looping = $true; BodyFrames = 576000 },
        [pscustomobject]@{ Cue = 'F'; Looping = $true; BodyFrames = 576000 },
        [pscustomobject]@{ Cue = 'G'; Looping = $false; BodyFrames = 288000 },
        [pscustomobject]@{ Cue = 'H'; Looping = $true; BodyFrames = 576000 }
    )
}

function Get-HordeMusicExpectedSources {
    @(
        'source/What_the_Dark_Keeps_Pocket_Chordsmith.json',
        'source/What_the_Dark_Keeps_PCS1.txt'
    )
}

function Get-HordeMusicExpectedRuntimeFiles {
    $files = [Collections.Generic.List[string]]::new()
    $files.Add('asset.manifest.json')
    foreach ($cue in (Get-HordeMusicCueSpecification)) {
        $files.Add("runtime/$($cue.Cue)-body.wav")
        $files.Add("runtime/$($cue.Cue)-tail.wav")
    }
    return @($files)
}

function Get-HordeMusicFileSha256 {
    param([Parameter(Mandatory = $true)][string]$Path)
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Join-HordeMusicRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$BasePath,
        [Parameter(Mandatory = $true)][string]$RelativePath
    )
    $path = [IO.Path]::GetFullPath($BasePath)
    foreach ($segment in ($RelativePath -split '[/\\]')) {
        if ($segment.Length -gt 0) { $path = Join-Path $path $segment }
    }
    return $path
}

function Assert-HordeMusicWave {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][int]$ExpectedFrames,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $expectedDataBytes = [long]$ExpectedFrames * 4
    $expectedWaveBytes = $expectedDataBytes + 44
    $stream = [IO.File]::Open($Path, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
    try {
        if ($stream.Length -ne $expectedWaveBytes) {
            throw "$Label WAV size is $($stream.Length), expected $expectedWaveBytes bytes."
        }
        $reader = [IO.BinaryReader]::new($stream, [Text.Encoding]::ASCII, $true)
        try {
            $riff = [Text.Encoding]::ASCII.GetString($reader.ReadBytes(4))
            $riffSize = $reader.ReadUInt32()
            $wave = [Text.Encoding]::ASCII.GetString($reader.ReadBytes(4))
            $fmt = [Text.Encoding]::ASCII.GetString($reader.ReadBytes(4))
            $fmtSize = $reader.ReadUInt32()
            $encoding = $reader.ReadUInt16()
            $channels = $reader.ReadUInt16()
            $sampleRate = $reader.ReadUInt32()
            $byteRate = $reader.ReadUInt32()
            $blockAlign = $reader.ReadUInt16()
            $bitsPerSample = $reader.ReadUInt16()
            $dataId = [Text.Encoding]::ASCII.GetString($reader.ReadBytes(4))
            $dataBytes = $reader.ReadUInt32()
        } finally { $reader.Dispose() }
    } finally { $stream.Dispose() }

    if ($riff -cne 'RIFF' -or $riffSize -ne ($expectedWaveBytes - 8) -or $wave -cne 'WAVE' -or
        $fmt -cne 'fmt ' -or $fmtSize -ne 16 -or $encoding -ne 1 -or $channels -ne 2 -or
        $sampleRate -ne 48000 -or $byteRate -ne 192000 -or $blockAlign -ne 4 -or
        $bitsPerSample -ne 16 -or $dataId -cne 'data' -or $dataBytes -ne $expectedDataBytes) {
        throw "$Label WAV header is not the exact PCM16_LE stereo 48 kHz format and frame count."
    }
}

function Assert-HordeMusicAssetTree {
    param([Parameter(Mandatory = $true)][string]$MusicRoot)

    $musicRootFull = [IO.Path]::GetFullPath($MusicRoot)
    if (-not (Test-Path -LiteralPath $musicRootFull -PathType Container)) {
        throw "Horde music asset root is missing: $musicRootFull"
    }
    $rootItem = Get-Item -LiteralPath $musicRootFull -Force
    if (($rootItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'Horde music asset root must not be a symlink or reparse point.'
    }
    foreach ($item in Get-ChildItem -LiteralPath $musicRootFull -Recurse -Force) {
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Horde music asset tree contains a symlink or reparse point: $($item.FullName)"
        }
    }

    $manifestPath = Join-Path $musicRootFull 'asset.manifest.json'
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw 'Horde music manifest is missing.' }
    $manifestHash = Get-HordeMusicFileSha256 -Path $manifestPath
    try { $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json }
    catch { throw "Horde music manifest JSON is invalid: $($_.Exception.Message)" }

    if ($manifest.schemaVersion -ne 1 -or $manifest.assetId -cne 'what-the-dark-keeps') {
        throw 'Horde music manifest schema or asset ID is not the reviewed value.'
    }
    if ($manifest.rights -cne $script:HordeMusicRights) {
        throw "Horde music rights must be exactly '$($script:HordeMusicRights)'."
    }
    if ($manifest.format.encoding -cne 'PCM16_LE' -or $manifest.format.sampleRate -ne 48000 -or
        $manifest.format.channels -ne 2) {
        throw 'Horde music format must be PCM16_LE, 48000 Hz, stereo.'
    }
    if ($manifest.tailFrames -ne 144000 -or $manifest.crossfadeFrames -ne 12000 -or
        $manifest.totalWaveBytes -ne 20160704 -or $manifest.totalPcmBytes -ne 20160000) {
        throw 'Horde music tail, crossfade, or aggregate PCM sizes differ from the reviewed contract.'
    }

    $expectedSourcePaths = @(Get-HordeMusicExpectedSources)
    if (@($manifest.sources).Count -ne $expectedSourcePaths.Count) { throw 'Horde music manifest must list exactly two canonical sources.' }
    $observedSourcePaths = @($manifest.sources | ForEach-Object { [string]$_.path })
    $observedSourcesSorted = [string]::Join("`n", [string[]]@($observedSourcePaths | Sort-Object -CaseSensitive))
    $expectedSourcesSorted = [string]::Join("`n", [string[]]@($expectedSourcePaths | Sort-Object -CaseSensitive))
    if ($observedSourcesSorted -cne $expectedSourcesSorted) {
        throw 'Horde music canonical source roster is not the reviewed JSON plus PCS1 pair.'
    }
    foreach ($source in $manifest.sources) {
        $path = Join-HordeMusicRelativePath -BasePath $musicRootFull -RelativePath ([string]$source.path)
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Horde music canonical source is missing: $($source.path)" }
        if ((Get-Item -LiteralPath $path).Length -ne [long]$source.bytes) { throw "Horde music source size mismatch: $($source.path)" }
        if ((Get-HordeMusicFileSha256 -Path $path) -cne [string]$source.sha256) { throw "Horde music source SHA-256 mismatch: $($source.path)" }
    }

    $specifications = @(Get-HordeMusicCueSpecification)
    if (@($manifest.cues).Count -ne $specifications.Count) { throw 'Horde music manifest must contain exactly cues A-H.' }
    $runtimeWaveBytes = 0L
    $runtimePcmBytes = 0L
    for ($index = 0; $index -lt $specifications.Count; $index++) {
        $spec = $specifications[$index]
        $cue = $manifest.cues[$index]
        if ($cue.cue -cne $spec.Cue -or [bool]$cue.looping -ne [bool]$spec.Looping) {
            throw "Cue $($spec.Cue) order or looping policy differs from the reviewed A-H contract."
        }
        foreach ($part in @(
            [pscustomobject]@{ Name = 'body'; Frames = [int]$spec.BodyFrames },
            [pscustomobject]@{ Name = 'tail'; Frames = 144000 }
        )) {
            $entry = $cue.($part.Name)
            $expectedPath = "runtime/$($spec.Cue)-$($part.Name).wav"
            $expectedPcmBytes = [long]$part.Frames * 4
            $expectedBytes = $expectedPcmBytes + 44
            if ($entry.path -cne $expectedPath) { throw "Cue $($spec.Cue) $($part.Name) path is not the reviewed runtime path." }
            if ($entry.frames -ne $part.Frames -or $entry.bytes -ne $expectedBytes) {
                throw "Cue $($spec.Cue) $($part.Name) frame or byte count differs from the reviewed contract."
            }
            $wavePath = Join-HordeMusicRelativePath -BasePath $musicRootFull -RelativePath $expectedPath
            if (-not (Test-Path -LiteralPath $wavePath -PathType Leaf)) { throw "Horde music runtime wave is missing: $expectedPath" }
            if ((Get-Item -LiteralPath $wavePath).Length -ne $expectedBytes) { throw "Horde music runtime size mismatch: $expectedPath" }
            if ((Get-HordeMusicFileSha256 -Path $wavePath) -cne [string]$entry.sha256) { throw "Horde music runtime SHA-256 mismatch: $expectedPath" }
            Assert-HordeMusicWave -Path $wavePath -ExpectedFrames $part.Frames -Label "Cue $($spec.Cue) $($part.Name)"
            $runtimeWaveBytes += $expectedBytes
            $runtimePcmBytes += $expectedPcmBytes
        }
    }
    if ($runtimeWaveBytes -ne [long]$manifest.totalWaveBytes -or $runtimePcmBytes -ne [long]$manifest.totalPcmBytes) {
        throw 'Horde music aggregate PCM/WAV byte totals do not equal the exact cue roster.'
    }

    $metadataPath = Join-Path $musicRootFull 'METADATA.md'
    if (-not (Test-Path -LiteralPath $metadataPath -PathType Leaf)) { throw 'Horde music METADATA.md is missing.' }
    $metadataText = Get-Content -LiteralPath $metadataPath -Raw
    if (-not $metadataText.Contains($script:HordeMusicRights) -or
        -not $metadataText.Contains('JSON/PCS1 in `source/`') -or
        -not $metadataText.Contains('deliberately excluded from game packages')) {
        throw 'Horde music metadata must retain its Horde-only rights and source-not-packaged notice.'
    }

    $expectedInventory = @('asset.manifest.json', 'METADATA.md') + $expectedSourcePaths + @(Get-HordeMusicExpectedRuntimeFiles | Where-Object { $_ -cne 'asset.manifest.json' })
    $actualFiles = @(Get-ChildItem -LiteralPath $musicRootFull -Recurse -File -Force | ForEach-Object {
        $_.FullName.Substring($musicRootFull.Length + 1).Replace('\', '/')
    })
    $actualFilesSorted = [string]::Join("`n", [string[]]@($actualFiles | Sort-Object -CaseSensitive))
    $expectedInventorySorted = [string]::Join("`n", [string[]]@($expectedInventory | Sort-Object -CaseSensitive))
    if ($actualFilesSorted -cne $expectedInventorySorted) {
        throw 'Horde music source tree has a missing or unrecognised file; closed inventory is manifest, metadata, two sources, and sixteen runtime WAVs.'
    }
    $actualDirectories = @(Get-ChildItem -LiteralPath $musicRootFull -Recurse -Directory -Force | ForEach-Object {
        $_.FullName.Substring($musicRootFull.Length + 1).Replace('\', '/')
    })
    $actualDirectoriesSorted = [string]::Join("`n", [string[]]@($actualDirectories | Sort-Object -CaseSensitive))
    $expectedDirectoriesSorted = [string]::Join("`n", [string[]]@(@('runtime', 'source') | Sort-Object -CaseSensitive))
    if ($actualDirectoriesSorted -cne $expectedDirectoriesSorted) {
        throw 'Horde music tree may contain only the runtime/ and source/ subdirectories.'
    }

    if ($manifestHash -cne $script:HordeMusicManifestSha256) {
        throw "Horde music manifest SHA-256 does not match the reviewed pin ($script:HordeMusicManifestSha256)."
    }
    return $manifest
}

function Assert-HordeMusicAssets {
    param([Parameter(Mandatory = $true)][string]$RepositoryRoot)
    $repo = [IO.Path]::GetFullPath($RepositoryRoot)
    $musicRoot = Join-HordeMusicRelativePath -BasePath $repo -RelativePath $script:HordeMusicAssetRelativeRoot
    $manifest = Assert-HordeMusicAssetTree -MusicRoot $musicRoot
    $null = Assert-HordeOutdoorMusicAssets -RepositoryRoot $repo
    return $manifest
}

function Get-HordeLegacyMusicRuntimeFiles {
    param([Parameter(Mandatory = $true)][string]$RepositoryRoot)
    $root = Join-HordeMusicRelativePath -BasePath ([IO.Path]::GetFullPath($RepositoryRoot)) -RelativePath $script:HordeMusicAssetRelativeRoot
    $null = Assert-HordeMusicAssetTree -MusicRoot $root
    $files = [Collections.Generic.List[string]]::new()
    $files.Add("$($script:HordeMusicAssetRootRelativeToAssets)/asset.manifest.json")
    foreach ($cue in (Get-HordeMusicCueSpecification)) {
        $files.Add("$($script:HordeMusicAssetRootRelativeToAssets)/runtime/$($cue.Cue)-body.wav")
        $files.Add("$($script:HordeMusicAssetRootRelativeToAssets)/runtime/$($cue.Cue)-tail.wav")
    }
    return @($files)
}

function Get-HordeOutdoorMusicExpectedRuntimeFiles {
    @('asset.manifest.json', 'runtime/outdoor-main.wav')
}

function Assert-HordeOutdoorMusicAssetTree {
    param([Parameter(Mandatory = $true)][string]$MusicRoot)
    $root = [IO.Path]::GetFullPath($MusicRoot)
    if (-not (Test-Path -LiteralPath $root -PathType Container)) { throw 'Outdoor music asset root is missing.' }
    foreach ($item in @((Get-Item -LiteralPath $root -Force)) + @(Get-ChildItem -LiteralPath $root -Recurse -Force)) {
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw 'Outdoor music tree must not contain symlinks or reparse points.'
        }
    }
    $manifestPath = Join-Path $root 'asset.manifest.json'
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw 'Outdoor music manifest is missing.' }
    try { $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json }
    catch { throw 'Outdoor music manifest JSON is invalid.' }
    if ($manifest.schemaVersion -ne 1 -or $manifest.assetId -cne 'beneath-the-same-moon' -or
        $manifest.rights -cne $script:HordeMusicRights -or
        $manifest.format.encoding -cne 'PCM16_LE' -or $manifest.format.sampleRate -ne 48000 -or
        $manifest.format.channels -ne 2 -or $manifest.tailFrames -ne 0 -or
        $manifest.crossfadeFrames -ne 12000 -or $manifest.totalWaveBytes -ne 20480044 -or
        $manifest.totalPcmBytes -ne 20480000 -or @($manifest.cues).Count -ne 1) {
        throw 'Outdoor music manifest rights, format or aggregate contract differs from the reviewed main-only asset.'
    }
    $cue = $manifest.cues[0]
    if ($cue.cue -cne 'Outdoor' -or $cue.looping -ne $true -or $null -ne $cue.tail -or
        $cue.body.path -cne 'runtime/outdoor-main.wav' -or $cue.body.frames -ne 5120000 -or
        $cue.body.bytes -ne 20480044 -or $cue.body.sha256 -cne $script:HordeOutdoorMusicWaveSha256) {
        throw 'Outdoor music cue must be the exact full-period main loop without a separate tail or quiet mix.'
    }
    $wavePath = Join-HordeMusicRelativePath -BasePath $root -RelativePath 'runtime/outdoor-main.wav'
    if (-not (Test-Path -LiteralPath $wavePath -PathType Leaf)) { throw 'Outdoor music runtime wave is missing.' }
    if ((Get-HordeMusicFileSha256 -Path $wavePath) -cne $script:HordeOutdoorMusicWaveSha256) {
        throw 'Outdoor music runtime SHA-256 mismatch.'
    }
    Assert-HordeMusicWave -Path $wavePath -ExpectedFrames 5120000 -Label 'Outdoor main'
    $metadataPath = Join-Path $root 'METADATA.md'
    if (-not (Test-Path -LiteralPath $metadataPath -PathType Leaf)) { throw 'Outdoor music metadata is missing.' }
    $metadata = Get-Content -LiteralPath $metadataPath -Raw
    if (-not $metadata.Contains($script:HordeMusicRights) -or
        -not $metadata.Contains('source originals are excluded from game packages')) {
        throw 'Outdoor music metadata must retain rights and source exclusion.'
    }
    $expected = @('asset.manifest.json', 'METADATA.md', 'runtime/outdoor-main.wav')
    $actual = @(Get-ChildItem -LiteralPath $root -Recurse -File -Force | ForEach-Object {
        $_.FullName.Substring($root.Length + 1).Replace('\', '/')
    })
    if ([string]::Join("`n", [string[]]@($actual | Sort-Object -CaseSensitive)) -cne
        [string]::Join("`n", [string[]]@($expected | Sort-Object -CaseSensitive))) {
        throw 'Outdoor music closed inventory excludes all source originals, quiet mixes and unknown files.'
    }
    $directories = @(Get-ChildItem -LiteralPath $root -Recurse -Directory -Force)
    if ($directories.Count -ne 1 -or $directories[0].Name -cne 'runtime') {
        throw 'Outdoor music tree may contain only its runtime subdirectory.'
    }
    if ((Get-HordeMusicFileSha256 -Path $manifestPath) -cne $script:HordeOutdoorMusicManifestSha256) {
        throw 'Outdoor music manifest SHA-256 does not match the reviewed pin.'
    }
    return $manifest
}

function Assert-HordeOutdoorMusicAssets {
    param([Parameter(Mandatory = $true)][string]$RepositoryRoot)
    $root = Join-HordeMusicRelativePath -BasePath ([IO.Path]::GetFullPath($RepositoryRoot)) -RelativePath $script:HordeOutdoorMusicAssetRelativeRoot
    Assert-HordeOutdoorMusicAssetTree -MusicRoot $root
}

function Get-HordeOutdoorMusicRuntimeFiles {
    param([Parameter(Mandatory = $true)][string]$RepositoryRoot)
    $null = Assert-HordeOutdoorMusicAssets -RepositoryRoot $RepositoryRoot
    @(Get-HordeOutdoorMusicExpectedRuntimeFiles | ForEach-Object { "$($script:HordeOutdoorMusicAssetRootRelativeToAssets)/$_" })
}

function Get-HordeMusicRuntimeFiles {
    param([Parameter(Mandatory = $true)][string]$RepositoryRoot)
    @(Get-HordeLegacyMusicRuntimeFiles -RepositoryRoot $RepositoryRoot) + @(Get-HordeOutdoorMusicRuntimeFiles -RepositoryRoot $RepositoryRoot)
}

function Copy-HordeMusicRuntimeAssets {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)][string]$AssetRoot
    )
    $manifest = Assert-HordeMusicAssets -RepositoryRoot $RepositoryRoot
    $repo = [IO.Path]::GetFullPath($RepositoryRoot)
    $assetRootFull = [IO.Path]::GetFullPath($AssetRoot)
    if (-not (Test-Path -LiteralPath $assetRootFull -PathType Container)) {
        New-Item -ItemType Directory -Path $assetRootFull -Force | Out-Null
    }
    $musicDestination = Join-HordeMusicRelativePath -BasePath $assetRootFull -RelativePath 'audio/music'
    if (Test-Path -LiteralPath $musicDestination) {
        throw "Refusing to overwrite existing staged Horde music tree: $musicDestination"
    }
    $destinationParent = Split-Path -Parent $musicDestination
    New-Item -ItemType Directory -Path $destinationParent -Force | Out-Null
    foreach ($relative in (Get-HordeMusicRuntimeFiles -RepositoryRoot $repo)) {
        $source = Join-HordeMusicRelativePath -BasePath $repo -RelativePath "assets/$relative"
        $destination = Join-HordeMusicRelativePath -BasePath $assetRootFull -RelativePath $relative
        $parent = Split-Path -Parent $destination
        if (-not (Test-Path -LiteralPath $parent -PathType Container)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
        Copy-Item -LiteralPath $source -Destination $destination
        if ((Get-Item -LiteralPath $destination).Length -ne (Get-Item -LiteralPath $source).Length -or
            (Get-HordeMusicFileSha256 -Path $destination) -cne (Get-HordeMusicFileSha256 -Path $source)) {
            throw "Staged Horde music file failed byte verification: $relative"
        }
    }
    $expected = @(Get-HordeMusicRuntimeFiles -RepositoryRoot $repo | ForEach-Object { $_.Substring('audio/music/'.Length) })
    $actual = @(Get-ChildItem -LiteralPath $musicDestination -Recurse -File -Force | ForEach-Object {
        $_.FullName.Substring($musicDestination.Length + 1).Replace('\', '/')
    })
    $actualSorted = [string]::Join("`n", [string[]]@($actual | Sort-Object -CaseSensitive))
    $expectedSorted = [string]::Join("`n", [string[]]@($expected | Sort-Object -CaseSensitive))
    if ($actualSorted -cne $expectedSorted) {
        throw 'Staged Horde music tree is not the exact legacy seventeen plus outdoor two-file roster.'
    }
    if (@(Get-ChildItem -LiteralPath $musicDestination -Recurse -Force | Where-Object { ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 }).Count -ne 0) {
        throw 'Staged Horde music tree contains a symlink or reparse point.'
    }
    return ,$manifest
}

function Get-HordeMusicZipEntrySha256 {
    param([Parameter(Mandatory = $true)][IO.Compression.ZipArchiveEntry]$Entry)
    $hash = [Security.Cryptography.SHA256]::Create()
    $stream = $Entry.Open()
    try { ([BitConverter]::ToString($hash.ComputeHash($stream))).Replace('-', '').ToLowerInvariant() }
    finally { $stream.Dispose(); $hash.Dispose() }
}

function Assert-HordeMusicPackage {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)][string]$ArchivePath
    )
    $manifest = Assert-HordeMusicAssets -RepositoryRoot $RepositoryRoot
    $archiveFull = [IO.Path]::GetFullPath($ArchivePath)
    if (-not (Test-Path -LiteralPath $archiveFull -PathType Leaf)) { throw "Horde music package archive is missing: $archiveFull" }
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [IO.Compression.ZipFile]::OpenRead($archiveFull)
    try {
        $prefix = "assets/$($script:HordeMusicAssetRootRelativeToAssets)/"
        $musicEntries = @($zip.Entries | Where-Object { $_.FullName.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase) })
        $expectedRelative = @(Get-HordeLegacyMusicRuntimeFiles -RepositoryRoot $RepositoryRoot)
        $expectedNames = @($expectedRelative | ForEach-Object { "assets/$_" })
        if ($musicEntries.Count -ne $expectedNames.Count) {
            throw "Horde music package must contain exactly $($expectedNames.Count) entries under $prefix; found $($musicEntries.Count)."
        }
        $observedNames = @($musicEntries | ForEach-Object FullName)
        if (@($observedNames | Sort-Object -Unique -CaseSensitive).Count -ne $observedNames.Count) {
            throw 'Horde music package contains duplicate entry names.'
        }
        $observedNamesSorted = [string]::Join("`n", [string[]]@($observedNames | Sort-Object -CaseSensitive))
        $expectedNamesSorted = [string]::Join("`n", [string[]]@($expectedNames | Sort-Object -CaseSensitive))
        if ($observedNamesSorted -cne $expectedNamesSorted) {
            throw 'Horde music package contains missing, source, or unrecognised entries under the music asset root.'
        }
        $musicRoot = Join-HordeMusicRelativePath -BasePath ([IO.Path]::GetFullPath($RepositoryRoot)) -RelativePath $script:HordeMusicAssetRelativeRoot
        foreach ($entry in $musicEntries) {
            $relative = $entry.FullName.Substring('assets/'.Length)
            $source = Join-HordeMusicRelativePath -BasePath ([IO.Path]::GetFullPath($RepositoryRoot)) -RelativePath "assets/$relative"
            if ($entry.Length -ne (Get-Item -LiteralPath $source).Length -or
                (Get-HordeMusicZipEntrySha256 -Entry $entry) -cne (Get-HordeMusicFileSha256 -Path $source)) {
                throw "Horde music package entry byte/hash mismatch: $($entry.FullName)"
            }
        }
        $outdoorPrefix = "assets/$($script:HordeOutdoorMusicAssetRootRelativeToAssets)/"
        $outdoorEntries = @($zip.Entries | Where-Object { $_.FullName.StartsWith($outdoorPrefix, [StringComparison]::OrdinalIgnoreCase) })
        $outdoorNames = @(Get-HordeOutdoorMusicRuntimeFiles -RepositoryRoot $RepositoryRoot | ForEach-Object { "assets/$_" })
        if ($outdoorEntries.Count -ne 2) { throw 'Outdoor music package must contain exactly two main-only runtime entries.' }
        $actualOutdoorNames = @($outdoorEntries | ForEach-Object FullName)
        if (@($actualOutdoorNames | Sort-Object -Unique -CaseSensitive).Count -ne 2) {
            throw 'Outdoor music package contains duplicate entry names.'
        }
        if ([string]::Join("`n", [string[]]@($actualOutdoorNames | Sort-Object -CaseSensitive)) -cne
            [string]::Join("`n", [string[]]@($outdoorNames | Sort-Object -CaseSensitive))) {
            throw 'Outdoor music package contains missing, source, quiet or unrecognised entries.'
        }
        foreach ($entry in $outdoorEntries) {
            $source = Join-HordeMusicRelativePath -BasePath ([IO.Path]::GetFullPath($RepositoryRoot)) -RelativePath $entry.FullName
            if ($entry.Length -ne (Get-Item -LiteralPath $source).Length -or
                (Get-HordeMusicZipEntrySha256 -Entry $entry) -cne (Get-HordeMusicFileSha256 -Path $source)) {
                throw "Outdoor music package entry byte/hash mismatch: $($entry.FullName)"
            }
        }
        $allowedMusicNames = $expectedNames + $outdoorNames
        $sourceNames = @('What_the_Dark_Keeps_Pocket_Chordsmith.json', 'What_the_Dark_Keeps_PCS1.txt')
        foreach ($entry in $zip.Entries) {
            if (($entry.FullName.StartsWith('assets/audio/music/', [StringComparison]::OrdinalIgnoreCase) -and
                 $entry.FullName -cnotin $allowedMusicNames) -or
                $entry.FullName.StartsWith('assets/source/beyond_the_tomb/music/', [StringComparison]::OrdinalIgnoreCase) -or
                $entry.FullName.IndexOf('beneath-the-same-moon-outdoor-r2', [StringComparison]::OrdinalIgnoreCase) -ge 0) {
                throw "Horde music package contains unapproved music/source content: $($entry.FullName)"
            }
            if ($entry.FullName.StartsWith("assets/$($script:HordeMusicAssetRootRelativeToAssets)/source/", [StringComparison]::OrdinalIgnoreCase) -or
                ($entry.FullName.StartsWith('assets/audio/music/', [StringComparison]::Ordinal) -and
                    [IO.Path]::GetFileName($entry.FullName) -cin $sourceNames)) {
                throw "Horde music package contains a canonical/source document: $($entry.FullName)"
            }
        }
    } finally { $zip.Dispose() }
    return ,$manifest
}
