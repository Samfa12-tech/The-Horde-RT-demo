# Runtime-only 1.6.2 audio/environment/world admission. This does not certify device,
# listening, signing or release acceptance and works with Debug ZIP/APK files.
function Get-Horde162AssetSpecification {
    @(
        [pscustomobject]@{ Path='audio/menu/menu_room.wav'; Bytes=384044; Sha256='9aa9a9631f7dbaf724ecd46bfbbea17142af20497299a83db08055751c7cc361'; Platform='Both'; Kind='Wave'; Channels=1; Frames=192000 },
        [pscustomobject]@{ Path='audio/menu/menu_chain.wav'; Bytes=96044; Sha256='6c85d6c97a21e9de69100b9969dd8f844af927b229ee91bf585c42b4d0ae146d'; Platform='Both'; Kind='Wave'; Channels=1; Frames=48000 },
        [pscustomobject]@{ Path='audio/menu/asset.manifest.json'; Bytes=4459; Sha256='93b4711ebb1bd768c09ba3c0166aa5ed7e5e06adcaf11bb2758b0aab47af5f94'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/props/runtime/player-sword-scabbard/asset.manifest.json'; Bytes=758; Sha256='75e030f75e5dea9baa1732ce7efed67a1dd40e79c7203a2319d3f0e08044319e'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/props/runtime/player-sword-scabbard/processing-receipt.json'; Bytes=10052; Sha256='3d257b3cd2cfcd4f169ae962ef86e28d8b252351a1fbccd29a06d03d35e56541'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/props/runtime/player-sword-scabbard/player-sword-scabbard-lod0.runtime.glb'; Bytes=15128; Sha256='d23c2b0711f53ce7608e68945ee7c38dd4eb1dc06ad5e33e58acc26c5bc0eaf5'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='audio/filmcow/equipment/sword_draw.wav'; Bytes=41972; Sha256='a70dcbcf7d627f8ebec641a33c144f64682771c715f0bfc819fd2f8d981713ce'; Platform='Both'; Kind='Wave'; Channels=1; Frames=20964 },
        [pscustomobject]@{ Path='audio/filmcow/equipment/sword_sheath.wav'; Bytes=67558; Sha256='0f38fe0560c91aa1b97de84a01787d70d490832f727684ca402e9dfcf4ff2aaf'; Platform='Both'; Kind='Wave'; Channels=1; Frames=33757 },
        [pscustomobject]@{ Path='audio/filmcow/equipment/asset.manifest.json'; Bytes=1503; Sha256='b84eeef9402cebc0140996e25a819d8b47a1032b1eb5691fc95a926d09daa28d'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='audio/pixabay/chest_unlock.wav'; Bytes=36908; Sha256='a1dae01adb6534a2e1504f0d5bc41abff0305e619264dff0339577d4d5b2d808'; Platform='Both'; Kind='Wave'; Channels=1; Frames=18432 },
        [pscustomobject]@{ Path='audio/pixabay/chest_open.wav'; Bytes=479276; Sha256='788b1c31c504c8eea9e5316c87859013aba6a49f205e0e4a66b181aafbdfb4a6'; Platform='Both'; Kind='Wave'; Channels=1; Frames=239616 },
        [pscustomobject]@{ Path='audio/pixabay/torch_extinguish.wav'; Bytes=100268; Sha256='ef2e85d4219cce2e5789d6326755909f59e302bd26bc2dbd8f3e39c270614ab8'; Platform='Both'; Kind='Wave'; Channels=1; Frames=50112 },
        [pscustomobject]@{ Path='audio/pixabay/keeper_i_sense_you.wav'; Bytes=248684; Sha256='b476d7c672cbfce30a38c02f31978d0024f16c658ce2fd8cb716cbf91a994418'; Platform='Both'; Kind='Wave'; Channels=1; Frames=124320 },
        [pscustomobject]@{ Path='audio/pixabay/keeper_come_closer.wav'; Bytes=275418; Sha256='7a79e79aa4ebee4920d11f28f9ca140ac1e22b3c5cec2352ee159e0ba117a719'; Platform='Both'; Kind='Wave'; Channels=1; Frames=137687 },
        [pscustomobject]@{ Path='audio/pixabay/skeleton_idle_rattle.wav'; Bytes=120044; Sha256='8ac138628c744ab3a3df130abb415b11803145063932b4e6956414d010b504ea'; Platform='Both'; Kind='Wave'; Channels=1; Frames=60000 },
        [pscustomobject]@{ Path='audio/pixabay/skeleton_falling_bones.wav'; Bytes=193964; Sha256='66ee82dcab110f14ff7685eb60fa3b0caed32765c6b07f8b654433cd5c97efc9'; Platform='Both'; Kind='Wave'; Channels=1; Frames=96960 },
        [pscustomobject]@{ Path='audio/pixabay/waterfall_loop.wav'; Bytes=2047132; Sha256='de7711f0e6ef9cf0bdd3d04ba7a1b713ce09ef18169bd994182b09ab2017ee62'; Platform='Windows'; Kind='Wave'; Channels=1; Frames=1023527 },
        [pscustomobject]@{ Path='audio/pixabay/waterfall_core_loop.wav'; Bytes=2208044; Sha256='5bb82801f0cff75c56f07993fad128bf1dee2a9876d032c808b8033a135fbad3'; Platform='Android'; Kind='Wave'; Channels=2; Frames=552000 },
        [pscustomobject]@{ Path='textures/environment/runtime/night-storm.windows.ktx2'; Bytes=699532; Sha256='a5e274bfafae55efa4a80840208a39fc7f24c6cf5fe69c744c9c2cd7d60420b0'; Platform='Windows'; Kind='Ktx'; Format=43 },
        [pscustomobject]@{ Path='textures/environment/runtime/night-storm.android.ktx2'; Bytes=80128; Sha256='f833b77867e7b3c01659f5e9ef38834e4a0e5b967cd077023157eb682feb7cec'; Platform='Android'; Kind='Ktx'; Format=166 },
        [pscustomobject]@{ Path='models/world/runtime/collapsed-entry/collapsed-entry-lod0.runtime.glb'; Bytes=652980; Sha256='c67471b522c92f354d9a5880dea6e48b4f22b5095207841c35c95b5cfc75c4d4'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/world/runtime/collapsed-entry/asset.manifest.json'; Bytes=742; Sha256='952e1920fbc60646154d0424556c2935a18e7cf882866a9fe9f4f45ef38be623'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='models/props/runtime/player-rag-torch/rag-torch-player-lod0.runtime.glb'; Bytes=267672; Sha256='476cb05a8f11f1af917a0cfa29610cc03f23522478a366f6cf2cf49b906db74d'; Platform='Both'; Kind='Glb' },
        [pscustomobject]@{ Path='models/props/runtime/player-rag-torch/asset.manifest.json'; Bytes=830; Sha256='5316b2072008da441024cda1153f725f5afcd7d366f3a5b89dece98ec74617aa'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='textures/props/runtime/asset.manifest.json'; Bytes=12356; Sha256='d01d4be1c68b24c340fe930e23cf0af3197a00f93e86c98010f5fc38d45f619e'; Platform='Both'; Kind='Json' },
        [pscustomobject]@{ Path='textures/props/runtime/base-color.android.ktx2'; Bytes=8768496; Sha256='8ec835e988441ab533b77cc25ac9f2dbd83e91a36f614c0cdbed21a035e7c856'; Platform='Android'; Kind='PropsKtx'; Format=166; Layers=14 },
        [pscustomobject]@{ Path='textures/props/runtime/normal.android.ktx2'; Bytes=19574256; Sha256='8b830b88a95b8a3eedbbb7555c5ef61046bd3280730fd25dc817b217decfdf3c'; Platform='Android'; Kind='PropsKtx'; Format=157; Layers=14 },
        [pscustomobject]@{ Path='textures/props/runtime/orm.android.ktx2'; Bytes=8768496; Sha256='81ef2a8bb533d27f5609a6e36852a7c5d31a9d51a4b7ccf6d1622a31ba0d8744'; Platform='Android'; Kind='PropsKtx'; Format=165; Layers=14 },
        [pscustomobject]@{ Path='textures/props/runtime/emissive.android.ktx2'; Bytes=626752; Sha256='da5ee0baa4e6b84280a1b9ae077954c4658a4e7fff1da03212d220d282f9542a'; Platform='Android'; Kind='PropsKtx'; Format=166; Layers=1 },
        [pscustomobject]@{ Path='textures/props/runtime/base-color.windows.ktx2'; Bytes=78294160; Sha256='849103d8a02c3f947549eb4c5cdebe40d6f671522d68a28af16264a5e25caa8b'; Platform='Windows'; Kind='PropsKtx'; Format=43; Layers=14 },
        [pscustomobject]@{ Path='textures/props/runtime/normal.windows.ktx2'; Bytes=78294160; Sha256='ebbc7dfae582c0efe91c37f9ab9108d33aecacdc88071d797b37e55b42601c65'; Platform='Windows'; Kind='PropsKtx'; Format=37; Layers=14 },
        [pscustomobject]@{ Path='textures/props/runtime/orm.windows.ktx2'; Bytes=78294160; Sha256='43b10b4f1684ce3269ebedf096d426b679d12e442d5567c2f02b99d619720e82'; Platform='Windows'; Kind='PropsKtx'; Format=37; Layers=14 },
        [pscustomobject]@{ Path='textures/props/runtime/emissive.windows.ktx2'; Bytes=5592908; Sha256='c642d3972a14bf477d88f2ecb34ffc177ad4dc6308dfe8f507af6033c292dd56'; Platform='Windows'; Kind='PropsKtx'; Format=43; Layers=1 }
    )
}

function Get-Horde162ManifestSpecification {
    @(
        [pscustomobject]@{ Path='audio/pixabay/keeper-asset.manifest.json'; Platform='Both' },
        [pscustomobject]@{ Path='audio/pixabay/waterfall-core.manifest.json'; Platform='Android' },
        [pscustomobject]@{ Path='textures/environment/runtime/asset.manifest.json'; Platform='Both' }
    )
}

function Join-Horde162Path {
    param([string]$Root, [string]$Relative)
    [IO.Path]::Combine([IO.Path]::GetFullPath($Root), ($Relative.Replace('/', [IO.Path]::DirectorySeparatorChar)))
}

function Get-Horde162Sha256 {
    param([string]$Path)
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Assert-Horde162Wave {
    param([string]$Path, [int]$Channels, [long]$Frames)
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 44 -or [Text.Encoding]::ASCII.GetString($bytes,0,4) -cne 'RIFF' -or
        [BitConverter]::ToUInt32($bytes,4) -ne $bytes.Length-8 -or [Text.Encoding]::ASCII.GetString($bytes,8,4) -cne 'WAVE') {
        throw "1.6.2 WAV RIFF header mismatch: $Path"
    }
    $formatCount=0; $dataCount=0; $offset=12L
    while ($offset -lt $bytes.Length) {
        if ($offset+8 -gt $bytes.Length) { throw "1.6.2 WAV truncated chunk: $Path" }
        $id=[Text.Encoding]::ASCII.GetString($bytes,[int]$offset,4)
        $size=[long][BitConverter]::ToUInt32($bytes,[int]$offset+4)
        $body=$offset+8
        if ($body+$size -gt $bytes.Length) { throw "1.6.2 WAV invalid chunk size: $Path" }
        if ($id -ceq 'fmt ') {
            ++$formatCount
            if ($size -ne 16 -or [BitConverter]::ToUInt16($bytes,[int]$body) -ne 1 -or
                [BitConverter]::ToUInt16($bytes,[int]$body+2) -ne $Channels -or
                [BitConverter]::ToUInt32($bytes,[int]$body+4) -ne 48000 -or
                [BitConverter]::ToUInt32($bytes,[int]$body+8) -ne 48000*$Channels*2 -or
                [BitConverter]::ToUInt16($bytes,[int]$body+12) -ne $Channels*2 -or
                [BitConverter]::ToUInt16($bytes,[int]$body+14) -ne 16) { throw "1.6.2 WAV must be PCM16 LE 48 kHz with $Channels channels: $Path" }
        }
        if ($id -ceq 'data') { ++$dataCount; if ($size -ne $Frames*$Channels*2) { throw "1.6.2 WAV frame count mismatch: $Path" } }
        $offset=$body+$size+($size%2)
    }
    if ($offset -ne $bytes.Length -or $formatCount -ne 1 -or $dataCount -ne 1) { throw "1.6.2 WAV requires exactly one format/data chunk: $Path" }
}

function Assert-Horde162EnvironmentKtx {
    param([string]$Path, [uint32]$Format)
    $bytes=[IO.File]::ReadAllBytes($Path)
    $signature=[byte[]]@(0xab,0x4b,0x54,0x58,0x20,0x32,0x30,0xbb,0x0d,0x0a,0x1a,0x0a)
    if ($bytes.Length -lt 80 -or [Convert]::ToBase64String($bytes,0,12) -cne [Convert]::ToBase64String($signature) -or
        [BitConverter]::ToUInt32($bytes,12) -ne $Format -or [BitConverter]::ToUInt32($bytes,20) -ne 512 -or
        [BitConverter]::ToUInt32($bytes,24) -ne 256 -or [BitConverter]::ToUInt32($bytes,28) -ne 0 -or
        [BitConverter]::ToUInt32($bytes,32) -ne 0 -or [BitConverter]::ToUInt32($bytes,36) -ne 1 -or
        [BitConverter]::ToUInt32($bytes,40) -ne 10 -or [BitConverter]::ToUInt32($bytes,44) -ne 0) {
        throw "1.6.2 environment must be the admitted 512x256, ten-mip, native-format KTX2: $Path"
    }
}

function Assert-Horde162Assets {
    param([Parameter(Mandatory=$true)][string]$RepositoryRoot)
    $assets=Join-Horde162Path $RepositoryRoot 'assets'
    foreach ($spec in Get-Horde162AssetSpecification) {
        $path=Join-Horde162Path $assets $spec.Path
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "1.6.2 runtime asset missing: $($spec.Path)" }
        if ((Get-Item -LiteralPath $path).Length -ne $spec.Bytes -or (Get-Horde162Sha256 $path) -cne $spec.Sha256) { throw "1.6.2 runtime byte/hash mismatch: $($spec.Path)" }
        if ($spec.Kind -ceq 'Wave') { Assert-Horde162Wave $path $spec.Channels $spec.Frames }
        elseif ($spec.Kind -ceq 'Ktx') { Assert-Horde162EnvironmentKtx $path $spec.Format }
        elseif ($spec.Kind -ceq 'PropsKtx') {
            $bytes=[byte[]]::new(80); $stream=[IO.File]::OpenRead($path)
            try { $read=$stream.Read($bytes,0,$bytes.Length) } finally { $stream.Dispose() }
            if ($read -ne 80 -or [BitConverter]::ToString($bytes,0,12) -cne 'AB-4B-54-58-20-32-30-BB-0D-0A-1A-0A' -or
                [BitConverter]::ToUInt32($bytes,12) -ne $spec.Format -or [BitConverter]::ToUInt32($bytes,20) -ne 1024 -or
                [BitConverter]::ToUInt32($bytes,24) -ne 1024 -or [BitConverter]::ToUInt32($bytes,28) -ne 0 -or
                [BitConverter]::ToUInt32($bytes,32) -ne $spec.Layers -or [BitConverter]::ToUInt32($bytes,36) -ne 1 -or
                [BitConverter]::ToUInt32($bytes,40) -ne 11 -or [BitConverter]::ToUInt32($bytes,44) -ne 0) {
                throw "1.6.2 props KTX must match the admitted 1K/full-mip/layer/native-format profile: $path"
            }
        }
        elseif ($spec.Kind -ceq 'Glb') {
            $bytes=[IO.File]::ReadAllBytes($path)
            if ($bytes.Length -lt 20 -or [Text.Encoding]::ASCII.GetString($bytes,0,4) -cne 'glTF' -or
                [BitConverter]::ToUInt32($bytes,4) -ne 2 -or [BitConverter]::ToUInt32($bytes,8) -ne $bytes.Length -or
                [Text.Encoding]::ASCII.GetString($bytes,16,4) -cne 'JSON') { throw "1.6.2 runtime GLB header/length mismatch: $path" }
        }
        elseif ($spec.Kind -ceq 'Json') { $null=Get-Content -LiteralPath $path -Raw | ConvertFrom-Json }
        else { throw "1.6.2 runtime asset has an unknown admission type: $($spec.Path)" }
    }
    $keeper=Get-Content -LiteralPath (Join-Horde162Path $assets 'audio/pixabay/keeper-asset.manifest.json') -Raw | ConvertFrom-Json
    if ($keeper.schema -ne 1 -or $keeper.target -cne '1.6.2' -or $keeper.license -cne 'Pixabay Content License; no CC0 claim' -or
        $keeper.licenseTerms -cne 'https://pixabay.com/service/terms/' -or @($keeper.assets).Count -ne 4) { throw '1.6.2 keeper manifest admission mismatch.' }
    $keeperPaths=@('keeper_i_sense_you.wav','keeper_come_closer.wav','skeleton_idle_rattle.wav','skeleton_falling_bones.wav')
    foreach ($name in $keeperPaths) {
        $entries=@($keeper.assets | Where-Object { $_.runtimePath -ceq $name })
        $spec=@(Get-Horde162AssetSpecification | Where-Object { $_.Path -ceq "audio/pixabay/$name" })[0]
        if ($entries.Count -ne 1 -or $entries[0].sha256 -cne $spec.Sha256 -or $entries[0].bytes -ne $spec.Bytes -or
            $entries[0].sampleRate -ne 48000 -or $entries[0].channels -ne 1 -or $entries[0].bitsPerSample -ne 16 -or
            $entries[0].frames -ne $spec.Frames) { throw "1.6.2 keeper manifest runtime mismatch: $name" }
    }
    $core=Get-Content -LiteralPath (Join-Horde162Path $assets 'audio/pixabay/waterfall-core.manifest.json') -Raw | ConvertFrom-Json
    $coreSpec=@(Get-Horde162AssetSpecification | Where-Object { $_.Path -ceq 'audio/pixabay/waterfall_core_loop.wav' })[0]
    $fullSpec=@(Get-Horde162AssetSpecification | Where-Object { $_.Path -ceq 'audio/pixabay/waterfall_loop.wav' })[0]
    if ($core.schema -ne 1 -or $core.runtimePath -cne 'waterfall_core_loop.wav' -or $core.sha256 -cne $coreSpec.Sha256 -or
        $core.bytes -ne $coreSpec.Bytes -or $core.frames -ne $coreSpec.Frames -or $core.channels -ne 2 -or
        $core.sampleRate -ne 48000 -or $core.bitsPerSample -ne 16 -or $core.source -cne 'waterfall_loop.wav' -or
        $core.sourceSha256 -cne $fullSpec.Sha256 -or $core.sourceFrames -ne $fullSpec.Frames) { throw '1.6.2 waterfall manifest admission mismatch.' }
    $environment=Get-Content -LiteralPath (Join-Horde162Path $assets 'textures/environment/runtime/asset.manifest.json') -Raw | ConvertFrom-Json
    if ($environment.schema -ne 1 -or $environment.rights -cne 'Project-owned generated asset.' -or @($environment.runtime).Count -ne 2) { throw '1.6.2 environment manifest admission mismatch.' }
    foreach ($platform in @('windows','android')) {
        $entry=@($environment.runtime | Where-Object { $_.platform -ceq $platform })
        $spec=@(Get-Horde162AssetSpecification | Where-Object { $_.Path -ceq "textures/environment/runtime/night-storm.$platform.ktx2" })[0]
        $format=if($platform -ceq 'windows'){'R8G8B8A8_SRGB'}else{'ASTC_6x6_SRGB_BLOCK'}
        if ($entry.Count -ne 1 -or $entry[0].path -cne "night-storm.$platform.ktx2" -or $entry[0].sha256 -cne $spec.Sha256 -or
            $entry[0].bytes -ne $spec.Bytes -or $entry[0].format -cne $format -or @($entry[0].dimensions).Count -ne 2 -or
            $entry[0].dimensions[0] -ne 512 -or $entry[0].dimensions[1] -ne 256 -or $entry[0].mipLevels -ne 10) { throw "1.6.2 environment manifest runtime mismatch: $platform" }
    }
    return [pscustomobject]@{ Keeper=$keeper; Waterfall=$core; Environment=$environment }
}

function Get-Horde162RuntimeFiles {
    param([Parameter(Mandatory=$true)][string]$RepositoryRoot, [Parameter(Mandatory=$true)][ValidateSet('Windows','Android')][string]$Platform)
    $null=Assert-Horde162Assets $RepositoryRoot
    @(Get-Horde162AssetSpecification | Where-Object { $_.Platform -ceq 'Both' -or $_.Platform -ceq $Platform } | ForEach-Object Path)
    @(Get-Horde162ManifestSpecification | Where-Object { $_.Platform -ceq 'Both' -or $_.Platform -ceq $Platform } | ForEach-Object Path)
}

function Assert-Horde162StagedAssets {
    param([string]$RepositoryRoot, [string]$AssetRoot, [ValidateSet('Windows','Android')][string]$Platform)
    $expected=@(Get-Horde162RuntimeFiles $RepositoryRoot $Platform)
    $observed=@(foreach($root in @('audio/menu','audio/filmcow/equipment','audio/pixabay','textures/environment','textures/props','models/world','models/props/runtime/player-rag-torch','models/props/runtime/player-sword-scabbard','models/props/source')) {
        $path=Join-Horde162Path $AssetRoot $root
        if (Test-Path -LiteralPath $path -PathType Container) {
            foreach($item in Get-ChildItem -LiteralPath $path -Recurse -Force) {
                if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw '1.6.2 staged assets contain a reparse point.' }
                if (-not $item.PSIsContainer) { $item.FullName.Substring([IO.Path]::GetFullPath($AssetRoot).Length+1).Replace('\','/') }
            }
        }
    })
    if ([string]::Join("`n",[string[]]@($expected | Sort-Object -CaseSensitive)) -cne
        [string]::Join("`n",[string[]]@($observed | Sort-Object -CaseSensitive))) { throw "1.6.2 $Platform staged inventory is not the closed runtime roster." }
    foreach($relative in $expected) {
        $source=Join-Horde162Path $RepositoryRoot "assets/$relative"; $staged=Join-Horde162Path $AssetRoot $relative
        if ((Get-Item -LiteralPath $source).Length -ne (Get-Item -LiteralPath $staged).Length -or
            (Get-Horde162Sha256 $source) -cne (Get-Horde162Sha256 $staged)) { throw "1.6.2 staged byte/hash mismatch: $relative" }
    }
}

function Copy-Horde162RuntimeAssets {
    param([Parameter(Mandatory=$true)][string]$RepositoryRoot, [Parameter(Mandatory=$true)][string]$AssetRoot,
          [Parameter(Mandatory=$true)][ValidateSet('Windows','Android')][string]$Platform)
    foreach($relative in Get-Horde162RuntimeFiles $RepositoryRoot $Platform) {
        $source=Join-Horde162Path $RepositoryRoot "assets/$relative"; $target=Join-Horde162Path $AssetRoot $relative
        if (Test-Path -LiteralPath $target) {
            if ((Get-Horde162Sha256 $target) -cne (Get-Horde162Sha256 $source)) { throw "Refusing to overwrite a different staged asset: $relative" }
        } else {
            $null=New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force
            Copy-Item -LiteralPath $source -Destination $target
        }
    }
    Assert-Horde162StagedAssets $RepositoryRoot $AssetRoot $Platform
}

function Assert-Horde162Package {
    param([Parameter(Mandatory=$true)][string]$RepositoryRoot, [Parameter(Mandatory=$true)][string]$ArchivePath,
          [Parameter(Mandatory=$true)][ValidateSet('Windows','Android')][string]$Platform)
    $expected=@(Get-Horde162RuntimeFiles $RepositoryRoot $Platform | ForEach-Object { "assets/$_" })
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip=[IO.Compression.ZipFile]::OpenRead([IO.Path]::GetFullPath($ArchivePath))
    try {
        foreach ($entry in $zip.Entries) {
            if ($entry.FullName.Contains('\') -or $entry.FullName.StartsWith('/', [StringComparison]::Ordinal) -or
                $entry.FullName -match '(^|/)(\.|\.\.)(/|$)') { throw '1.6.2 package contains a noncanonical or traversing entry path.' }
        }
        $entries=@($zip.Entries | Where-Object { $_.FullName -match '(?i)^assets/(audio/menu|audio/filmcow/equipment|audio/pixabay|textures/environment|textures/props|models/world|models/props/runtime/player-rag-torch|models/props/runtime/player-sword-scabbard|models/props/source)(/|$)' })
        # ZIP directory entries are metadata, distinguished by their trailing
        # slash, not by byte length (a zero-byte foreign file is still a file).
        foreach($entry in $entries) {
            if ($entry.FullName.EndsWith('/', [StringComparison]::Ordinal)) {
                if ($entry.Length -ne 0) { throw '1.6.2 package has a malformed nonempty directory entry.' }
                if ($entry.FullName.Contains('\') -or @($expected | Where-Object {
                    $_.StartsWith($entry.FullName, [StringComparison]::Ordinal)
                }).Count -eq 0) { throw '1.6.2 package directory is outside the closed runtime roster.' }
            }
        }
        $names=@($entries | ForEach-Object FullName)
        $unique=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
        foreach($name in $names) { if (-not $unique.Add($name)) { throw '1.6.2 package has duplicate runtime entry names.' } }
        $entries=@($entries | Where-Object { -not $_.FullName.EndsWith('/', [StringComparison]::Ordinal) })
        $names=@($entries | ForEach-Object FullName)
        if ([string]::Join("`n",[string[]]@($expected | Sort-Object -CaseSensitive)) -cne
            [string]::Join("`n",[string[]]@($names | Sort-Object -CaseSensitive))) { throw "1.6.2 $Platform package inventory is not the closed runtime roster (missing, source, foreign or other-platform entry)." }
        foreach($entry in $entries) {
            $source=Join-Horde162Path $RepositoryRoot $entry.FullName
            if ($entry.Length -ne (Get-Item -LiteralPath $source).Length) { throw "1.6.2 package byte/hash mismatch: $($entry.FullName)" }
            $sha=[Security.Cryptography.SHA256]::Create(); $stream=$entry.Open()
            try { $actual=[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','').ToLowerInvariant() }
            finally { $stream.Dispose(); $sha.Dispose() }
            if ($actual -cne (Get-Horde162Sha256 $source)) { throw "1.6.2 package byte/hash mismatch: $($entry.FullName)" }
        }
    } finally { $zip.Dispose() }
}
