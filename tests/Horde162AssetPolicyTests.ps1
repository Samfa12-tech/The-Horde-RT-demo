$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $repo 'tools/horde-1.6.2-asset-policy.ps1')
Add-Type -AssemblyName System.IO.Compression.FileSystem
$temp=[IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$scratch=Join-Path $temp ('Horde162AssetPolicy-'+[Guid]::NewGuid().ToString('N'))
$null=New-Item -ItemType Directory -Path $scratch
$script:checks=0
function Require([bool]$Value,[string]$Message) { if(-not $Value){throw $Message} }
function Expect-Failure([scriptblock]$Action,[string]$Message) {
    $observed=''; try { & $Action | Out-Null } catch { $observed=$_.Exception.Message }
    Require (-not [string]::IsNullOrWhiteSpace($observed)) 'Negative asset fixture unexpectedly passed.'
    Require ($observed -like "*$Message*") "Negative fixture failed for an unrelated reason: $observed"
    ++$script:checks
}
function New-FixtureArchive([string]$Name,[string]$Platform,[string]$Omit='', [string]$Extra='', [string]$Duplicate='', [string]$Corrupt='', [string[]]$Directories=@(), [string]$EmptyFile='') {
    $path=Join-Path $scratch $Name
    $zip=[IO.Compression.ZipFile]::Open($path,[IO.Compression.ZipArchiveMode]::Create)
    try {
        foreach($relative in Get-Horde162RuntimeFiles $repo $Platform) {
            $entryName="assets/$relative"
            if($entryName -ceq $Omit){continue}
            $source=Join-Horde162Path $repo $entryName
            $bytes=[IO.File]::ReadAllBytes($source)
            if($entryName -ceq $Corrupt){$bytes[$bytes.Length-1]=$bytes[$bytes.Length-1] -bxor 1}
            $copies=if($entryName -ceq $Duplicate){2}else{1}
            for($copy=0;$copy -lt $copies;++$copy){
                $entry=$zip.CreateEntry($entryName);$stream=$entry.Open()
                try{$stream.Write($bytes,0,$bytes.Length)}finally{$stream.Dispose()}
            }
        }
        if($Extra){
            $entry=$zip.CreateEntry($Extra);$stream=$entry.Open()
            try{$bytes=[Text.Encoding]::ASCII.GetBytes('synthetic foreign source, never admitted');$stream.Write($bytes,0,$bytes.Length)}finally{$stream.Dispose()}
        }
        foreach($directory in $Directories){$null=$zip.CreateEntry($directory)}
        if($EmptyFile){$null=$zip.CreateEntry($EmptyFile)}
        # A Debug candidate may contain unrelated native/signing metadata. The
        # asset validator intentionally does not mistake this for Release proof.
        $entry=$zip.CreateEntry('META-INF/debug-fixture.txt');$stream=$entry.Open()
        try{$stream.WriteByte(1)}finally{$stream.Dispose()}
    }finally{$zip.Dispose()}
    return $path
}
try {
    $null=Assert-Horde162Assets $repo
    ++$script:checks
    $windows=@(Get-Horde162RuntimeFiles $repo Windows);$android=@(Get-Horde162RuntimeFiles $repo Android)
    Require ($windows.Count -eq 11 -and $android.Count -eq 12) 'Closed runtime roster count changed.'
    Require ($windows -ccontains 'audio/pixabay/waterfall_loop.wav' -and $windows -cnotcontains 'audio/pixabay/waterfall_core_loop.wav') 'Windows must preserve accepted full waterfall only.'
    Require ($android -ccontains 'audio/pixabay/waterfall_core_loop.wav' -and $android -cnotcontains 'audio/pixabay/waterfall_loop.wav') 'Android must use admitted Core loop only.'
    ++$script:checks
    foreach($platform in @('Windows','Android')) {
        $archive=New-FixtureArchive "$platform-positive.zip" $platform
        Assert-Horde162Package $repo $archive $platform
        $stage=Join-Path $scratch "$platform-stage/assets"
        Copy-Horde162RuntimeAssets $repo $stage $platform
        Assert-Horde162StagedAssets $repo $stage $platform
        ++$script:checks
    }
    $directories=@('assets/audio/pixabay/','assets/textures/environment/','assets/textures/environment/runtime/')
    $archive=New-FixtureArchive 'explicit-directory-entries.zip' Windows -Directories $directories
    Assert-Horde162Package $repo $archive Windows
    ++$script:checks
    $archive=Join-Path $scratch 'actual-compress-archive.zip'
    Compress-Archive -Path (Join-Path $scratch 'Windows-stage/*') -DestinationPath $archive
    Assert-Horde162Package $repo $archive Windows
    ++$script:checks
    $archive=New-FixtureArchive 'directory-plus-foreign-file.zip' Windows -Directories $directories -Extra 'assets/textures/environment/source/foreign.png'
    Expect-Failure {Assert-Horde162Package $repo $archive Windows} 'closed runtime roster'
    $archive=New-FixtureArchive 'nonempty-directory.zip' Windows -Extra 'assets/textures/environment/runtime/'
    Expect-Failure {Assert-Horde162Package $repo $archive Windows} 'malformed nonempty directory'
    $archive=New-FixtureArchive 'zero-byte-foreign-file.zip' Windows -EmptyFile 'assets/audio/pixabay/foreign.wav'
    Expect-Failure {Assert-Horde162Package $repo $archive Windows} 'closed runtime roster'
    $archive=New-FixtureArchive 'foreign-empty-directory.zip' Windows -Directories @('assets/textures/environment/source/')
    Expect-Failure {Assert-Horde162Package $repo $archive Windows} 'closed runtime roster'
    foreach($case in @(
        @{Name='missing-cue';Platform='Windows';Omit='assets/audio/pixabay/keeper_i_sense_you.wav'},
        @{Name='missing-core-manifest';Platform='Android';Omit='assets/audio/pixabay/waterfall-core.manifest.json'},
        @{Name='foreign-source';Platform='Windows';Extra='assets/textures/environment/source/foreign-original.png'},
        @{Name='foreign-cue';Platform='Android';Extra='assets/audio/pixabay/unreviewed.wav'},
        @{Name='windows-extra-core';Platform='Windows';Extra='assets/audio/pixabay/waterfall_core_loop.wav'},
        @{Name='android-extra-full-loop';Platform='Android';Extra='assets/audio/pixabay/waterfall_loop.wav'},
        @{Name='other-platform-env';Platform='Windows';Extra='assets/textures/environment/runtime/night-storm.android.ktx2'},
        @{Name='noncanonical-path';Platform='Windows';Extra='assets\audio\pixabay\foreign.wav'}
    )) {
        $archiveArguments=@{Name=$case.Name+'.zip';Platform=$case.Platform}
        if($case.ContainsKey('Omit')){$archiveArguments.Omit=$case.Omit}
        if($case.ContainsKey('Extra')){$archiveArguments.Extra=$case.Extra}
        $archive=New-FixtureArchive @archiveArguments
        Expect-Failure {Assert-Horde162Package $repo $archive $case.Platform} 'closed runtime roster'
    }
    $archive=New-FixtureArchive 'duplicate.zip' Windows -Duplicate 'assets/audio/pixabay/keeper_i_sense_you.wav'
    Expect-Failure {Assert-Horde162Package $repo $archive Windows} 'duplicate runtime entry'
    foreach($case in @(
        @{Name='corrupt-cue';Platform='Windows';Path='assets/audio/pixabay/keeper_come_closer.wav'},
        @{Name='corrupt-env';Platform='Android';Path='assets/textures/environment/runtime/night-storm.android.ktx2'},
        @{Name='corrupt-manifest';Platform='Windows';Path='assets/audio/pixabay/keeper-asset.manifest.json'}
    )) {
        $archive=New-FixtureArchive ($case.Name+'.zip') $case.Platform -Corrupt $case.Path
        Expect-Failure {Assert-Horde162Package $repo $archive $case.Platform} 'byte/hash mismatch'
    }
    $fixtureRepo=Join-Path $scratch 'manifest-fixture'
    foreach($relative in (@(Get-Horde162AssetSpecification | ForEach-Object Path)+@(Get-Horde162ManifestSpecification | ForEach-Object Path))) {
        $source=Join-Horde162Path $repo "assets/$relative";$target=Join-Horde162Path $fixtureRepo "assets/$relative"
        $null=New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force
        Copy-Item -LiteralPath $source -Destination $target
    }
    $manifestPath=Join-Horde162Path $fixtureRepo 'assets/audio/pixabay/keeper-asset.manifest.json'
    $manifest=Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $manifest.assets[0].channels=2
    $manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $manifestPath
    Expect-Failure {Assert-Horde162Assets $fixtureRepo} 'keeper manifest runtime mismatch'
    $stage=Join-Path $scratch 'Windows-stage/assets'
    $foreign=Join-Horde162Path $stage 'textures/environment/source/unreviewed.png'
    $null=New-Item -ItemType Directory -Path (Split-Path -Parent $foreign) -Force
    [IO.File]::WriteAllText($foreign,'synthetic source')
    Expect-Failure {Assert-Horde162StagedAssets $repo $stage Windows} 'closed runtime roster'
    Write-Output "PASS: $script:checks 1.6.2 admission/staging/archive cases; no build, package, signing or device action."
} finally {
    $resolved=[IO.Path]::GetFullPath($scratch)
    $prefix=$temp.TrimEnd([IO.Path]::DirectorySeparatorChar)+[IO.Path]::DirectorySeparatorChar
    if(-not $resolved.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase)){throw 'Fixture cleanup target escaped its verified temp root.'}
    if(Test-Path -LiteralPath $resolved){Remove-Item -LiteralPath $resolved -Recurse -Force}
}
