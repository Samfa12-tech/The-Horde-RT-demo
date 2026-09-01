param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[0-9]+\.[0-9]+\.[0-9]+$')]
    [string]$Version,

    [Parameter(Mandatory = $true)]
    [string]$ArtifactDirectory,

    [string]$ReportDirectory,
    [string]$RecordPath,

    [ValidatePattern('^[0-9a-fA-F]{40}$')]
    [string]$ExpectedTargetCommit,

    [string]$FixturePublicationStatePath,
    [string]$FixtureSourceSurfacePath,

    # Fixture-only escape hatch. Production invocations must query origin and GitHub.
    [switch]$SkipRemote,

    # Fixture-only escape hatch for synthetic artifact hashes. Never use this for a release decision.
    [switch]$FixtureMode
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if ([string]::IsNullOrWhiteSpace($RecordPath)) {
    $RecordPath = Join-Path $repoRoot "release-provenance\horde-lantern-rt-alpha-$Version.json"
}
if ([string]::IsNullOrWhiteSpace($ReportDirectory)) {
    $ReportDirectory = Join-Path $repoRoot 'reports\github-release-preflight'
}
$RecordPath = [IO.Path]::GetFullPath($RecordPath)
$ArtifactDirectory = [IO.Path]::GetFullPath($ArtifactDirectory)
$ReportDirectory = [IO.Path]::GetFullPath($ReportDirectory)
if ($SkipRemote -and -not $FixtureMode) {
    throw 'SkipRemote is fixture-only and cannot be used for a publication decision.'
}
if (-not [string]::IsNullOrWhiteSpace($FixturePublicationStatePath) -and -not $FixtureMode) {
    throw 'FixturePublicationStatePath is fixture-only.'
}
if (-not [string]::IsNullOrWhiteSpace($FixtureSourceSurfacePath) -and -not $FixtureMode) {
    throw 'FixtureSourceSurfacePath is fixture-only.'
}

$checks = [Collections.Generic.List[object]]::new()
function Add-Check {
    param([string]$Name, [string]$Status, [string]$Detail)
    $checks.Add([PSCustomObject]@{ name = $Name; status = $Status; detail = $Detail })
}

function Require-Property {
    param([object]$Object, [string]$Name, [string]$Context)
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) {
        throw "$Context is missing required property '$Name'."
    }
    return $property.Value
}

function Assert-ExactObjectKeys {
    param([object]$Object, [string[]]$Expected, [string]$Context)
    if ($Object -isnot [PSCustomObject]) { throw "$Context must be an object." }
    $actual = @($Object.PSObject.Properties.Name | Sort-Object)
    $required = @($Expected | Sort-Object)
    if (($actual -join '|') -ne ($required -join '|')) {
        throw "$Context has missing or unknown properties."
    }
}

function Require-String {
    param([object]$Object, [string]$Name, [string]$Context, [string]$Pattern)
    $value = Require-Property -Object $Object -Name $Name -Context $Context
    if ($value -isnot [string] -or $value -notmatch $Pattern) {
        throw "$Context.$Name has an invalid value."
    }
    return $value
}

function Require-PositiveInt {
    param([object]$Object, [string]$Name, [string]$Context)
    $value = Require-Property -Object $Object -Name $Name -Context $Context
    if (($value -isnot [int] -and $value -isnot [long]) -or $value -lt 1) {
        throw "$Context.$Name must be a positive integer."
    }
    return [long]$value
}

function Read-Record {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Release provenance record not found: $Path"
    }
    try {
        $record = Get-Content -LiteralPath $Path -Raw -Encoding utf8 | ConvertFrom-Json
    } catch {
        throw "Release provenance record must be valid JSON: $Path"
    }
    Assert-ExactObjectKeys -Object $record -Expected @('schemaVersion', 'release', 'source', 'android', 'artifacts', 'documentation') -Context 'record'
    $schemaVersion = Require-PositiveInt -Object $record -Name 'schemaVersion' -Context 'record'
    if ($schemaVersion -ne 1) { throw "Unsupported release provenance schemaVersion $schemaVersion." }

    $release = Require-Property -Object $record -Name 'release' -Context 'record'
    $source = Require-Property -Object $record -Name 'source' -Context 'record'
    $android = Require-Property -Object $record -Name 'android' -Context 'record'
    $artifacts = Require-Property -Object $record -Name 'artifacts' -Context 'record'
    $documentation = Require-Property -Object $record -Name 'documentation' -Context 'record'
    Assert-ExactObjectKeys -Object $release -Expected @('version', 'tag', 'repository') -Context 'record.release'
    Assert-ExactObjectKeys -Object $source -Expected @('commit') -Context 'record.source'
    Assert-ExactObjectKeys -Object $android -Expected @('versionCode') -Context 'record.android'
    Assert-ExactObjectKeys -Object $artifacts -Expected @('checksumManifest', 'windows', 'android') -Context 'record.artifacts'
    Assert-ExactObjectKeys -Object $documentation -Expected @('validationPath', 'releaseNotesPath') -Context 'record.documentation'
    $windows = Require-Property -Object $artifacts -Name 'windows' -Context 'record.artifacts'
    $androidArtifact = Require-Property -Object $artifacts -Name 'android' -Context 'record.artifacts'
    $checksumManifest = Require-Property -Object $artifacts -Name 'checksumManifest' -Context 'record.artifacts'
    Assert-ExactObjectKeys -Object $windows -Expected @('fileName', 'bytes', 'sha256') -Context 'record.artifacts.windows'
    Assert-ExactObjectKeys -Object $androidArtifact -Expected @('fileName', 'bytes', 'sha256') -Context 'record.artifacts.android'
    Assert-ExactObjectKeys -Object $checksumManifest -Expected @('fileName', 'bytes', 'sha256') -Context 'record.artifacts.checksumManifest'
    $androidFileName = Require-String -Object $androidArtifact -Name 'fileName' -Context 'record.artifacts.android' -Pattern '^[A-Za-z0-9_.-]+$'
    if ($androidFileName -match '(?i)(debug|unsigned|do-not-publish)') {
        throw 'record.artifacts.android.fileName is not a publishable Android artifact name.'
    }

    [PSCustomObject]@{
        SchemaVersion = $schemaVersion
        Version = Require-String -Object $release -Name 'version' -Context 'record.release' -Pattern '^[0-9]+\.[0-9]+\.[0-9]+$'
        Tag = Require-String -Object $release -Name 'tag' -Context 'record.release' -Pattern '^v[0-9]+\.[0-9]+\.[0-9]+$'
        Repository = Require-String -Object $release -Name 'repository' -Context 'record.release' -Pattern '^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$'
        SourceCommit = Require-String -Object $source -Name 'commit' -Context 'record.source' -Pattern '^[0-9a-f]{40}$'
        AndroidVersionCode = Require-PositiveInt -Object $android -Name 'versionCode' -Context 'record.android'
        ChecksumManifest = [PSCustomObject]@{
            FileName = Require-String -Object $checksumManifest -Name 'fileName' -Context 'record.artifacts.checksumManifest' -Pattern '^[A-Za-z0-9_.-]+$'
            Bytes = Require-PositiveInt -Object $checksumManifest -Name 'bytes' -Context 'record.artifacts.checksumManifest'
            Sha256 = Require-String -Object $checksumManifest -Name 'sha256' -Context 'record.artifacts.checksumManifest' -Pattern '^[0-9a-f]{64}$'
        }
        Windows = [PSCustomObject]@{
            FileName = Require-String -Object $windows -Name 'fileName' -Context 'record.artifacts.windows' -Pattern '^[A-Za-z0-9_.-]+$'
            Bytes = Require-PositiveInt -Object $windows -Name 'bytes' -Context 'record.artifacts.windows'
            Sha256 = Require-String -Object $windows -Name 'sha256' -Context 'record.artifacts.windows' -Pattern '^[0-9a-f]{64}$'
        }
        AndroidArtifact = [PSCustomObject]@{
            FileName = $androidFileName
            Bytes = Require-PositiveInt -Object $androidArtifact -Name 'bytes' -Context 'record.artifacts.android'
            Sha256 = Require-String -Object $androidArtifact -Name 'sha256' -Context 'record.artifacts.android' -Pattern '^[0-9a-f]{64}$'
        }
        ValidationPath = Require-String -Object $documentation -Name 'validationPath' -Context 'record.documentation' -Pattern '^docs/[A-Za-z0-9_.-]+$'
        ReleaseNotesPath = Require-String -Object $documentation -Name 'releaseNotesPath' -Context 'record.documentation' -Pattern '^docs/[A-Za-z0-9_.-]+$'
    }
}

function Invoke-GitText {
    param([string[]]$Arguments)
    $nativeErrorPreference = Get-Variable -Name PSNativeCommandUseErrorActionPreference -Scope Global -ErrorAction SilentlyContinue
    if ($null -ne $nativeErrorPreference) { $global:PSNativeCommandUseErrorActionPreference = $false }
    try {
        try {
            $output = & git @Arguments 2>$null | Out-String
            $exitCode = $LASTEXITCODE
        } catch {
            $output = ($_ | Out-String)
            $exitCode = $LASTEXITCODE
            if ($exitCode -eq 0) { $exitCode = 1 }
        }
    } finally {
        if ($null -ne $nativeErrorPreference) { $global:PSNativeCommandUseErrorActionPreference = $nativeErrorPreference.Value }
    }
    return [PSCustomObject]@{ exitCode = $exitCode; output = $output.Trim() }
}

function Invoke-GhText {
    param([string[]]$Arguments)
    $nativeErrorPreference = Get-Variable -Name PSNativeCommandUseErrorActionPreference -Scope Global -ErrorAction SilentlyContinue
    if ($null -ne $nativeErrorPreference) { $global:PSNativeCommandUseErrorActionPreference = $false }
    try {
        try {
            $output = & gh @Arguments 2>&1 | Out-String
            $exitCode = $LASTEXITCODE
        } catch {
            $output = ($_ | Out-String)
            $exitCode = $LASTEXITCODE
            if ($exitCode -eq 0) { $exitCode = 1 }
        }
    } finally {
        if ($null -ne $nativeErrorPreference) { $global:PSNativeCommandUseErrorActionPreference = $nativeErrorPreference.Value }
    }
    return [PSCustomObject]@{ exitCode = $exitCode; output = $output.Trim() }
}

function Get-ManifestHashes {
    param([string]$Path)
    $entries = @{}
    foreach ($line in Get-Content -LiteralPath $Path -Encoding ascii) {
        if ($line -notmatch '^([0-9a-fA-F]{64})\s+\*?([A-Za-z0-9_.-]+)$') {
            throw "Checksum manifest contains an invalid entry: $line"
        }
        $name = $matches[2]
        if ($entries.ContainsKey($name)) { throw "Checksum manifest duplicates $name." }
        $entries[$name] = $matches[1].ToLowerInvariant()
    }
    return $entries
}

function Test-ReleaseAssetShape {
    param([object]$Release, [object]$Record)
    if ($Release -isnot [PSCustomObject] -or $Release.tagName -ne $Record.Tag -or
        ($Release.targetCommitish -ne $Record.SourceCommit -and $Release.targetCommitish -ne $Record.Tag)) {
        return $false
    }
    $assets = @($Release.assets)
    $planned = @($Record.Windows, $Record.AndroidArtifact, $Record.ChecksumManifest)
    if ($assets.Count -ne 3) { return $false }
    foreach ($asset in $planned) {
        $matches = @($assets | Where-Object { $_.name -eq $asset.FileName })
        if ($matches.Count -ne 1 -or $matches[0].state -ne 'uploaded' -or
            [long]$matches[0].size -ne $asset.Bytes -or $matches[0].digest -ne ('sha256:' + $asset.Sha256)) {
            return $false
        }
    }
    return @($assets | Where-Object { $_.name -notin @($planned | ForEach-Object { $_.FileName }) }).Count -eq 0
}

function Read-FixturePublicationState {
    param([string]$Path)
    if ([string]::IsNullOrWhiteSpace($Path)) { return $null }
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Fixture publication state not found: $Path" }
    try { $state = Get-Content -LiteralPath $Path -Raw -Encoding utf8 | ConvertFrom-Json } catch { throw 'Fixture publication state must be valid JSON.' }
    Assert-ExactObjectKeys -Object $state -Expected @('localTag', 'originTag', 'githubRelease') -Context 'fixture publication state'
    return $state
}

function Read-FixtureSourceSurfaces {
    param([string]$Path)
    if ([string]::IsNullOrWhiteSpace($Path)) { return $null }
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Fixture source surfaces not found: $Path" }
    try { $surfaces = Get-Content -LiteralPath $Path -Raw -Encoding utf8 | ConvertFrom-Json } catch { throw 'Fixture source surfaces must be valid JSON.' }
    Assert-ExactObjectKeys -Object $surfaces -Expected @('cmake', 'gradle', 'notes') -Context 'fixture source surfaces'
    foreach ($name in @('cmake', 'gradle', 'notes')) { if ($surfaces.$name -isnot [string]) { throw "fixture source surfaces.$name must be a string." } }
    return $surfaces
}

$record = $null
$fatalError = $null
$fixturePublicationState = $null
$fixtureSourceSurfaces = $null
try {
    $record = Read-Record -Path $RecordPath
    if ($record.Version -ne $Version -or $record.Tag -ne "v$Version") {
        throw "Record identity $($record.Version) / $($record.Tag) does not match requested version $Version."
    }
    if (-not [string]::IsNullOrWhiteSpace($ExpectedTargetCommit) -and
        $record.SourceCommit -ne $ExpectedTargetCommit.ToLowerInvariant()) {
        throw "Requested target $ExpectedTargetCommit does not equal immutable provenance source $($record.SourceCommit)."
    }
    if ($RecordPath -eq (Join-Path $repoRoot "release-provenance\horde-lantern-rt-alpha-1.6.0.json")) {
        $canonicalFields = @(
            @('release.tag', $record.Tag, 'v1.6.0'),
            @('release.repository', $record.Repository, 'Samfa12-tech/The-Horde-RT-demo'),
            @('source.commit', $record.SourceCommit, '57c81b635a6c10e2772283639026936adac80f8b'),
            @('android.versionCode', $record.AndroidVersionCode, 8),
            @('artifacts.checksumManifest.fileName', $record.ChecksumManifest.FileName, 'SHA256SUMS.txt'),
            @('artifacts.checksumManifest.bytes', $record.ChecksumManifest.Bytes, 349),
            @('artifacts.checksumManifest.sha256', $record.ChecksumManifest.Sha256, 'b86650261d7b8cdf000aeaf33f5a0a526b118c55ba59ea2ec2993d4a1bb47a30'),
            @('artifacts.windows.fileName', $record.Windows.FileName, 'Horde-Lantern-RT-Alpha-1.6.0-Windows-x64.zip'),
            @('artifacts.windows.bytes', $record.Windows.Bytes, 105508271),
            @('artifacts.windows.sha256', $record.Windows.Sha256, '7b0dcf24b4a47771a9c3a27cbc52e3899c87781109afcef20f7a9a8472411d77'),
            @('artifacts.android.fileName', $record.AndroidArtifact.FileName, 'Horde-Lantern-RT-Alpha-1.6.0-Android.apk'),
            @('artifacts.android.bytes', $record.AndroidArtifact.Bytes, 82357855),
            @('artifacts.android.sha256', $record.AndroidArtifact.Sha256, '52a64255ad5dec82cc866fb2ea3545be498ca06c73a789019be851c77e5d6c48'),
            @('documentation.validationPath', $record.ValidationPath, 'docs/SHOWCASE_ALPHA_1_6_0_RELEASE_VALIDATION_2026-08-30.md'),
            @('documentation.releaseNotesPath', $record.ReleaseNotesPath, 'docs/SHOWCASE_ALPHA_1_6_0_RELEASE_NOTES_2026-08-30.md')
        )
        if (@($canonicalFields | Where-Object { $_[1] -ne $_[2] }).Count -ne 0) {
            throw 'The checked 1.6.0 provenance record does not retain the immutable release identity.'
        }
    }
    Add-Check 'record.schema' 'pass' "schema $($record.SchemaVersion), $($record.Version), Android code $($record.AndroidVersionCode)"
    if ($FixtureMode) {
        $fixturePublicationState = Read-FixturePublicationState -Path $FixturePublicationStatePath
        $fixtureSourceSurfaces = Read-FixtureSourceSurfaces -Path $FixtureSourceSurfacePath
    }
} catch {
    $fatalError = $_.Exception.Message
    Add-Check 'record.schema' 'fail' $fatalError
}

if ($null -ne $record) {
    Push-Location $repoRoot
    try {
        $commitType = Invoke-GitText -Arguments @('cat-file', '-t', $record.SourceCommit)
        if ($commitType.exitCode -eq 0 -and $commitType.output -eq 'commit') {
            $reachable = Invoke-GitText -Arguments @('merge-base', '--is-ancestor', $record.SourceCommit, 'HEAD')
            if ($reachable.exitCode -eq 0) { Add-Check 'source.commit' 'pass' 'exact commit exists, is a commit, and is reachable from HEAD' }
            else { Add-Check 'source.commit' 'fail' 'exact source commit is not reachable from HEAD' }
        } else { Add-Check 'source.commit' 'fail' 'exact source commit is missing or is not a commit object' }

        if ($null -ne $fixtureSourceSurfaces) {
            $sourceCmake = [PSCustomObject]@{ exitCode = 0; output = $fixtureSourceSurfaces.cmake }
            $sourceGradle = [PSCustomObject]@{ exitCode = 0; output = $fixtureSourceSurfaces.gradle }
            $sourceNotes = [PSCustomObject]@{ exitCode = 0; output = $fixtureSourceSurfaces.notes }
        } else {
            $sourceCmake = Invoke-GitText -Arguments @('show', ($record.SourceCommit + ':CMakeLists.txt'))
            $sourceGradle = Invoke-GitText -Arguments @('show', ($record.SourceCommit + ':android/app/build.gradle'))
            $sourceNotes = Invoke-GitText -Arguments @('show', ($record.SourceCommit + ':' + $record.ReleaseNotesPath))
        }
        $sourceSurfaceValid = $sourceCmake.exitCode -eq 0 -and $sourceGradle.exitCode -eq 0 -and $sourceNotes.exitCode -eq 0 -and
            [regex]::IsMatch($sourceCmake.output, ('(?m)^[ \t]*VERSION[ \t]+' + [regex]::Escape($record.Version) + '[ \t]*\r?$')) -and
            [regex]::IsMatch($sourceGradle.output, ('(?m)^[ \t]*versionCode[ \t]+' + $record.AndroidVersionCode + '[ \t]*\r?$')) -and
            [regex]::IsMatch($sourceGradle.output, ("(?m)^[ \t]*versionName[ \t]+['`"]" + [regex]::Escape($record.Version) + "['`"][ \t]*\r?$")) -and
            [regex]::IsMatch($sourceNotes.output, ('(?m)^Package version: ' + [regex]::Escape('`' + $record.Version + '`') + '[ \t]*\r?$')) -and
            [regex]::IsMatch($sourceNotes.output, ('(?m)^Android version code: ' + [regex]::Escape('`' + $record.AndroidVersionCode + '`') + '[ \t]*\r?$'))
        if ($sourceSurfaceValid) { Add-Check 'source.release-surfaces' 'pass' 'source-time CMake, Gradle, and release notes match 1.6.0/code 8' }
        else { Add-Check 'source.release-surfaces' 'fail' 'source-time version, Android code, or release-note marker disagrees with the record' }

        $validationPath = Join-Path $repoRoot $record.ValidationPath
        $notesPath = Join-Path $repoRoot $record.ReleaseNotesPath
        if ($FixtureMode) {
            Add-Check 'documentation.immutable-validation' 'skipped' 'offline fixture mode'
        } elseif ((Test-Path -LiteralPath $validationPath -PathType Leaf) -and (Test-Path -LiteralPath $notesPath -PathType Leaf)) {
            $validation = Get-Content -LiteralPath $validationPath -Raw -Encoding utf8
            $notes = Get-Content -LiteralPath $notesPath -Raw -Encoding utf8
            $windowsBytes = $record.Windows.Bytes.ToString('N0', [Globalization.CultureInfo]::InvariantCulture)
            $androidBytes = $record.AndroidArtifact.Bytes.ToString('N0', [Globalization.CultureInfo]::InvariantCulture)
            $validationValid = $validation.Contains($record.SourceCommit) -and $validation.Contains("versionCode $($record.AndroidVersionCode)") -and
                [regex]::IsMatch($validation, ([regex]::Escape($record.Windows.FileName) + '\s*`?\s*\|\s*' + [regex]::Escape($windowsBytes) + '\s*\|\s*`' + [regex]::Escape($record.Windows.Sha256) + '`')) -and
                [regex]::IsMatch($validation, ([regex]::Escape($record.AndroidArtifact.FileName) + '\s*`?\s*\|\s*' + [regex]::Escape($androidBytes) + '\s*\|\s*`' + [regex]::Escape($record.AndroidArtifact.Sha256) + '`')) -and
                [regex]::IsMatch($notes, ('(?m)^Package version: ' + [regex]::Escape('`' + $record.Version + '`') + '[ \t]*\r?$')) -and
                [regex]::IsMatch($notes, ('(?m)^Android version code: ' + [regex]::Escape('`' + $record.AndroidVersionCode + '`') + '[ \t]*\r?$'))
            if ($validationValid) { Add-Check 'documentation.immutable-validation' 'pass' 'current validation record and release notes agree with the provenance record' }
            else { Add-Check 'documentation.immutable-validation' 'fail' 'current validation record or release notes disagrees with the provenance record' }
        } else { Add-Check 'documentation.immutable-validation' 'fail' 'validation record or release notes path is missing' }

        $manifestPath = Join-Path $ArtifactDirectory $record.ChecksumManifest.FileName
        $manifestHashes = $null
        if (-not (Test-Path -LiteralPath $ArtifactDirectory -PathType Container)) {
            Add-Check 'artifacts.directory' 'unavailable' "artifact directory is unavailable: $ArtifactDirectory"
        } elseif (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
            Add-Check 'artifacts.manifest' 'unavailable' "checksum manifest is unavailable: $manifestPath"
        } else {
            try {
                $manifestSize = ([IO.FileInfo]$manifestPath).Length
                $manifestHash = (Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash.ToLowerInvariant()
                $manifestHashes = Get-ManifestHashes -Path $manifestPath
                if ($manifestSize -ne $record.ChecksumManifest.Bytes -or $manifestHash -ne $record.ChecksumManifest.Sha256) {
                    throw "checksum manifest size/hash mismatch (size $manifestSize, hash $manifestHash)"
                }
                Add-Check 'artifacts.manifest' 'pass' "exact checksum manifest $($record.ChecksumManifest.FileName) verified"
            } catch { Add-Check 'artifacts.manifest' 'fail' $_.Exception.Message }
        }
        foreach ($artifact in @($record.Windows, $record.AndroidArtifact)) {
            $artifactPath = Join-Path $ArtifactDirectory $artifact.FileName
            if (-not (Test-Path -LiteralPath $artifactPath -PathType Leaf)) {
                Add-Check ("artifacts." + $artifact.FileName) 'unavailable' "artifact is unavailable: $artifactPath"
                continue
            }
            if ($null -eq $manifestHashes) {
                Add-Check ("artifacts." + $artifact.FileName) 'unavailable' 'checksum manifest is unavailable; exact artifact verification cannot complete'
                continue
            }
            $size = [IO.FileInfo]$artifactPath
            $hash = (Get-FileHash -LiteralPath $artifactPath -Algorithm SHA256).Hash.ToLowerInvariant()
            $manifestMatches = $null -ne $manifestHashes -and $manifestHashes.ContainsKey($artifact.FileName) -and
                $manifestHashes[$artifact.FileName] -eq $artifact.Sha256
            if ($size.Length -eq $artifact.Bytes -and $hash -eq $artifact.Sha256 -and $manifestMatches) {
                Add-Check ("artifacts." + $artifact.FileName) 'pass' "exact size $($artifact.Bytes), SHA-256, and manifest entry verified"
            } else {
                Add-Check ("artifacts." + $artifact.FileName) 'fail' "size/hash/manifest mismatch (size $($size.Length), hash $hash)"
            }
        }

        if ($null -ne $fixturePublicationState) {
            foreach ($tagFixture in @(@('tag.local', $fixturePublicationState.localTag), @('tag.origin', $fixturePublicationState.originTag))) {
                Assert-ExactObjectKeys -Object $tagFixture[1] -Expected @('state', 'target', 'shape') -Context $tagFixture[0]
                if ($tagFixture[1].state -eq 'absent' -and [string]::IsNullOrWhiteSpace([string]$tagFixture[1].target)) {
                    Add-Check $tagFixture[0] 'absent' "$($tagFixture[0]) is absent in fixture state"
                } elseif ($tagFixture[1].state -eq 'present' -and $tagFixture[1].shape -in @('lightweight', 'annotated') -and $tagFixture[1].target -eq $record.SourceCommit) {
                    Add-Check $tagFixture[0] 'present-matched' "$($tagFixture[0]) points to the immutable source in fixture state"
                } else { Add-Check $tagFixture[0] 'fail' "$($tagFixture[0]) fixture state disagrees with immutable provenance" }
            }
            $releaseFixture = $fixturePublicationState.githubRelease
            if ($releaseFixture -isnot [PSCustomObject]) { Add-Check 'github.release' 'fail' 'github.release fixture state is invalid' }
            elseif ($releaseFixture.state -eq 'absent') { Add-Check 'github.release' 'absent' 'GitHub Release is absent in fixture state' }
            elseif ($releaseFixture.state -eq 'present' -and (Test-ReleaseAssetShape -Release $releaseFixture.release -Record $record)) {
                Add-Check 'github.release' 'present-matched' 'GitHub Release fixture has exact uploaded immutable assets'
            } else { Add-Check 'github.release' 'fail' 'GitHub Release fixture state disagrees with immutable provenance' }
        } elseif ($SkipRemote) {
            $localTag = Invoke-GitText -Arguments @('show-ref', '--verify', '--quiet', ('refs/tags/' + $record.Tag))
            if ($localTag.exitCode -ne 0) { Add-Check 'tag.local' 'absent' "local tag $($record.Tag) is absent (expected before publication)" }
            else { Add-Check 'tag.local' 'fail' 'fixture SkipRemote cannot accept a present local tag' }
            Add-Check 'tag.origin' 'skipped' 'offline fixture mode'
            Add-Check 'github.release' 'skipped' 'offline fixture mode'
        } else {
            $localTag = Invoke-GitText -Arguments @('show-ref', '--verify', '--quiet', ('refs/tags/' + $record.Tag))
            if ($localTag.exitCode -ne 0) { Add-Check 'tag.local' 'absent' "local tag $($record.Tag) is absent (expected before publication)" }
            else {
                $localTarget = Invoke-GitText -Arguments @('rev-list', '-n', '1', $record.Tag)
                if ($localTarget.exitCode -eq 0 -and $localTarget.output -eq $record.SourceCommit) { Add-Check 'tag.local' 'present-matched' "local tag points to $($record.SourceCommit)" }
                else { Add-Check 'tag.local' 'fail' "local tag does not resolve to $($record.SourceCommit)" }
            }
            $remoteTag = Invoke-GitText -Arguments @('ls-remote', '--tags', 'origin', ('refs/tags/' + $record.Tag), ('refs/tags/' + $record.Tag + '^{}'))
            if ($remoteTag.exitCode -ne 0) { Add-Check 'tag.origin' 'fail' 'origin tag query failed' }
            elseif ([string]::IsNullOrWhiteSpace($remoteTag.output)) { Add-Check 'tag.origin' 'absent' "origin tag $($record.Tag) is absent (expected before publication)" }
            else {
                $remoteTarget = (($remoteTag.output -split "`r?`n") | Where-Object { $_ -match ('refs/tags/' + [regex]::Escape($record.Tag) + '(?:\^\{\})?$') } | Select-Object -Last 1) -split '\s+' | Select-Object -First 1
                if ($remoteTarget -eq $record.SourceCommit) { Add-Check 'tag.origin' 'present-matched' "origin tag points to $($record.SourceCommit)" }
                else { Add-Check 'tag.origin' 'fail' "origin tag does not resolve to $($record.SourceCommit)" }
            }

            if (-not (Get-Command gh -ErrorAction SilentlyContinue)) { Add-Check 'github.release' 'fail' 'GitHub CLI is unavailable' }
            else {
                $releaseQuery = Invoke-GhText -Arguments @('release', 'view', $record.Tag, '--repo', $record.Repository, '--json', 'tagName,targetCommitish,assets')
                if ($releaseQuery.exitCode -ne 0) {
                    if ($releaseQuery.output -match '(?i)(not found|could not resolve|release not found)') { Add-Check 'github.release' 'absent' "GitHub Release $($record.Tag) is absent (expected before publication)" }
                    else { Add-Check 'github.release' 'fail' 'GitHub Release query failed' }
                } else {
                    try {
                        $release = $releaseQuery.output | ConvertFrom-Json
                        if (Test-ReleaseAssetShape -Release $release -Record $record) {
                            Add-Check 'github.release' 'present-matched' 'existing GitHub Release has exactly the planned uploaded assets, sizes, and SHA-256 digests'
                        } else { Add-Check 'github.release' 'fail' 'existing GitHub Release target or required assets disagree with the record' }
                    } catch { Add-Check 'github.release' 'fail' 'GitHub Release response was not valid expected JSON' }
                }
            }
        }
    } finally { Pop-Location }
}

$failed = @($checks | Where-Object { $_.status -eq 'fail' }).Count -gt 0
$unavailable = @($checks | Where-Object { $_.status -eq 'unavailable' }).Count -gt 0
$skipped = @($checks | Where-Object { $_.status -eq 'skipped' }).Count -gt 0
$fixtureValidated = $FixtureMode -and -not $failed -and -not $unavailable
$publicationReady = -not $failed -and -not $unavailable -and -not $skipped -and -not $FixtureMode -and $null -ne $record
$status = if ($failed) { 'failed' } elseif ($fixtureValidated) { 'fixture-pass' } elseif ($publicationReady) { 'pass' } else { 'incomplete' }

[IO.Directory]::CreateDirectory($ReportDirectory) | Out-Null
$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$result = [PSCustomObject]@{
    schemaVersion = 1
    status = $status
    publicationReady = $publicationReady
    fixtureValidated = $fixtureValidated
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString('o')
    recordPath = $RecordPath
    artifactDirectory = $ArtifactDirectory
    checks = @($checks)
}
$jsonPath = Join-Path $ReportDirectory "github-release-preflight-$Version-$timestamp.json"
$markdownPath = Join-Path $ReportDirectory "github-release-preflight-$Version-$timestamp.md"
$result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $jsonPath -Encoding utf8
$markdown = @("# GitHub release preflight: $Version", "", "Status: **$status**", "", "Publication ready: **$publicationReady**", "", "| Check | Status | Detail |", "|---|---|---|")
foreach ($check in $checks) {
    $markdownDetail = $check.detail -replace '\|', '\\|'
    $markdown += "| $($check.name) | $($check.status) | $markdownDetail |"
}
$markdown | Set-Content -LiteralPath $markdownPath -Encoding utf8
Write-Output ($result | ConvertTo-Json -Depth 6 -Compress)
if ($failed) { exit 1 }
if ($publicationReady -or $fixtureValidated) { exit 0 }
exit 2
