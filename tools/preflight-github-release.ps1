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
if ($FixtureMode -and -not $SkipRemote) {
    throw 'FixtureMode requires SkipRemote and cannot query live publication state.'
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
    if ($record -isnot [PSCustomObject]) { throw 'Release provenance record must be a JSON object.' }
    $schemaVersion = Require-PositiveInt -Object $record -Name 'schemaVersion' -Context 'record'
    if ($schemaVersion -ne 1) { throw "Unsupported release provenance schemaVersion $schemaVersion." }

    $release = Require-Property -Object $record -Name 'release' -Context 'record'
    $source = Require-Property -Object $record -Name 'source' -Context 'record'
    $android = Require-Property -Object $record -Name 'android' -Context 'record'
    $artifacts = Require-Property -Object $record -Name 'artifacts' -Context 'record'
    $documentation = Require-Property -Object $record -Name 'documentation' -Context 'record'
    foreach ($namedObject in @(@($release, 'record.release'), @($source, 'record.source'), @($android, 'record.android'),
                               @($artifacts, 'record.artifacts'), @($documentation, 'record.documentation'))) {
        if ($namedObject[0] -isnot [PSCustomObject]) { throw "$($namedObject[1]) must be an object." }
    }
    $windows = Require-Property -Object $artifacts -Name 'windows' -Context 'record.artifacts'
    $androidArtifact = Require-Property -Object $artifacts -Name 'android' -Context 'record.artifacts'
    if ($windows -isnot [PSCustomObject] -or $androidArtifact -isnot [PSCustomObject]) {
        throw 'record.artifacts.windows and record.artifacts.android must be objects.'
    }

    [PSCustomObject]@{
        SchemaVersion = $schemaVersion
        Version = Require-String -Object $release -Name 'version' -Context 'record.release' -Pattern '^[0-9]+\.[0-9]+\.[0-9]+$'
        Tag = Require-String -Object $release -Name 'tag' -Context 'record.release' -Pattern '^v[0-9]+\.[0-9]+\.[0-9]+$'
        Repository = Require-String -Object $release -Name 'repository' -Context 'record.release' -Pattern '^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$'
        SourceCommit = Require-String -Object $source -Name 'commit' -Context 'record.source' -Pattern '^[0-9a-f]{40}$'
        AndroidVersionCode = Require-PositiveInt -Object $android -Name 'versionCode' -Context 'record.android'
        ChecksumManifest = Require-String -Object $artifacts -Name 'checksumManifest' -Context 'record.artifacts' -Pattern '^[A-Za-z0-9_.-]+$'
        Windows = [PSCustomObject]@{
            FileName = Require-String -Object $windows -Name 'fileName' -Context 'record.artifacts.windows' -Pattern '^[A-Za-z0-9_.-]+$'
            Bytes = Require-PositiveInt -Object $windows -Name 'bytes' -Context 'record.artifacts.windows'
            Sha256 = Require-String -Object $windows -Name 'sha256' -Context 'record.artifacts.windows' -Pattern '^[0-9a-f]{64}$'
        }
        AndroidArtifact = [PSCustomObject]@{
            FileName = Require-String -Object $androidArtifact -Name 'fileName' -Context 'record.artifacts.android' -Pattern '^[A-Za-z0-9_.-]+$'
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

$record = $null
$fatalError = $null
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
        if ($record.SourceCommit -ne '57c81b635a6c10e2772283639026936adac80f8b' -or $record.AndroidVersionCode -ne 8 -or
            $record.Repository -ne 'Samfa12-tech/The-Horde-RT-demo') {
            throw 'The checked 1.6.0 provenance record does not retain the immutable release identity.'
        }
    }
    Add-Check 'record.schema' 'pass' "schema $($record.SchemaVersion), $($record.Version), Android code $($record.AndroidVersionCode)"
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

        $sourceCmake = Invoke-GitText -Arguments @('show', ($record.SourceCommit + ':CMakeLists.txt'))
        $sourceGradle = Invoke-GitText -Arguments @('show', ($record.SourceCommit + ':android/app/build.gradle'))
        $sourceNotes = Invoke-GitText -Arguments @('show', ($record.SourceCommit + ':' + $record.ReleaseNotesPath))
        $sourceSurfaceValid = $sourceCmake.exitCode -eq 0 -and $sourceGradle.exitCode -eq 0 -and $sourceNotes.exitCode -eq 0 -and
            $sourceCmake.output -match ('VERSION\s+' + [regex]::Escape($record.Version)) -and
            $sourceGradle.output -match ('versionCode\s+' + $record.AndroidVersionCode) -and
            $sourceGradle.output -match ("versionName\s+['`"]" + [regex]::Escape($record.Version) + "['`"]") -and
            $sourceNotes.output.Contains("Package version: ``$($record.Version)``") -and
            $sourceNotes.output.Contains("Android version code: ``$($record.AndroidVersionCode)``")
        if ($sourceSurfaceValid) { Add-Check 'source.release-surfaces' 'pass' 'source-time CMake, Gradle, and release notes match 1.6.0/code 8' }
        else { Add-Check 'source.release-surfaces' 'fail' 'source-time version, Android code, or release-note marker disagrees with the record' }

        $validationPath = Join-Path $repoRoot $record.ValidationPath
        $notesPath = Join-Path $repoRoot $record.ReleaseNotesPath
        if ($FixtureMode) {
            Add-Check 'documentation.immutable-validation' 'skipped' 'offline fixture mode'
        } elseif ((Test-Path -LiteralPath $validationPath -PathType Leaf) -and (Test-Path -LiteralPath $notesPath -PathType Leaf)) {
            $validation = Get-Content -LiteralPath $validationPath -Raw -Encoding utf8
            $notes = Get-Content -LiteralPath $notesPath -Raw -Encoding utf8
            $validationValid = $validation.Contains($record.SourceCommit) -and $validation.Contains("versionCode $($record.AndroidVersionCode)") -and
                $validation.Contains($record.Windows.FileName) -and $validation.Contains($record.Windows.Sha256) -and
                $validation.Contains($record.AndroidArtifact.FileName) -and $validation.Contains($record.AndroidArtifact.Sha256) -and
                $notes.Contains("Package version: ``$($record.Version)``") -and $notes.Contains("Android version code: ``$($record.AndroidVersionCode)``")
            if ($validationValid) { Add-Check 'documentation.immutable-validation' 'pass' 'current validation record and release notes agree with the provenance record' }
            else { Add-Check 'documentation.immutable-validation' 'fail' 'current validation record or release notes disagrees with the provenance record' }
        } else { Add-Check 'documentation.immutable-validation' 'fail' 'validation record or release notes path is missing' }

        $manifestPath = Join-Path $ArtifactDirectory $record.ChecksumManifest
        $manifestHashes = $null
        if (-not (Test-Path -LiteralPath $ArtifactDirectory -PathType Container)) {
            Add-Check 'artifacts.directory' 'unavailable' "artifact directory is unavailable: $ArtifactDirectory"
        } elseif (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
            Add-Check 'artifacts.manifest' 'unavailable' "checksum manifest is unavailable: $manifestPath"
        } else {
            try {
                $manifestHashes = Get-ManifestHashes -Path $manifestPath
                Add-Check 'artifacts.manifest' 'pass' "checksum manifest parsed: $($record.ChecksumManifest)"
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

        $localTag = Invoke-GitText -Arguments @('show-ref', '--verify', '--quiet', ('refs/tags/' + $record.Tag))
        if ($localTag.exitCode -ne 0) { Add-Check 'tag.local' 'absent' "local tag $($record.Tag) is absent (expected before publication)" }
        else {
            $localTarget = Invoke-GitText -Arguments @('rev-list', '-n', '1', $record.Tag)
            if ($localTarget.exitCode -eq 0 -and $localTarget.output -eq $record.SourceCommit) { Add-Check 'tag.local' 'present-matched' "local tag points to $($record.SourceCommit)" }
            else { Add-Check 'tag.local' 'fail' "local tag does not resolve to $($record.SourceCommit)" }
        }

        if ($SkipRemote) {
            Add-Check 'tag.origin' 'skipped' 'offline fixture mode'
            Add-Check 'github.release' 'skipped' 'offline fixture mode'
        } else {
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
                        $releaseAssets = @($release.assets)
                        $assetChecks = foreach ($artifact in @($record.Windows, $record.AndroidArtifact)) {
                            $matches = @($releaseAssets | Where-Object { $_.name -eq $artifact.FileName })
                            $matches.Count -eq 1 -and [long]$matches[0].size -eq $artifact.Bytes
                        }
                        $manifestAsset = @($releaseAssets | Where-Object { $_.name -eq $record.ChecksumManifest }).Count -eq 1
                        $targetMatches = $release.tagName -eq $record.Tag -and
                            ($release.targetCommitish -eq $record.SourceCommit -or $release.targetCommitish -eq $record.Tag)
                        if ($targetMatches -and -not ($assetChecks -contains $false) -and $manifestAsset) {
                            Add-Check 'github.release' 'present-matched' 'existing GitHub Release target and required asset names/sizes match the record'
                        } else { Add-Check 'github.release' 'fail' 'existing GitHub Release target or required assets disagree with the record' }
                    } catch { Add-Check 'github.release' 'fail' 'GitHub Release response was not valid expected JSON' }
                }
            }
        }
    } finally { Pop-Location }
}

$failed = @($checks | Where-Object { $_.status -eq 'fail' }).Count -gt 0
$unavailable = @($checks | Where-Object { $_.status -eq 'unavailable' }).Count -gt 0
$publicationReady = -not $failed -and -not $unavailable -and $null -ne $record
$status = if ($failed) { 'failed' } elseif ($publicationReady) { 'pass' } else { 'incomplete' }

[IO.Directory]::CreateDirectory($ReportDirectory) | Out-Null
$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$result = [PSCustomObject]@{
    schemaVersion = 1
    status = $status
    publicationReady = $publicationReady
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
if (-not $publicationReady) { exit 2 }
