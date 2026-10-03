$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$scriptPath = Join-Path $repoRoot 'tools\preflight-github-release.ps1'
. (Join-Path $repoRoot 'tools\github-release-preflight-invoker.ps1')
$fixtureRoot = Join-Path ([IO.Path]::GetTempPath()) ('horde-release-preflight-' + [Guid]::NewGuid().ToString('N'))
$artifactRoot = Join-Path $fixtureRoot 'artifacts'
$reportRoot = Join-Path $fixtureRoot 'reports'
$recordPath = Join-Path $fixtureRoot 'record.json'
$statePath = Join-Path $fixtureRoot 'publication-state.json'
$sourcePath = Join-Path $fixtureRoot 'source-surfaces.json'
$sourceStatePath = Join-Path $fixtureRoot 'source-state.json'
[IO.Directory]::CreateDirectory($artifactRoot) | Out-Null

function Get-Artifact([string]$Name) {
    $path = Join-Path $artifactRoot $Name
    [PSCustomObject]@{ fileName = $Name; bytes = ([IO.FileInfo]$path).Length; sha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() }
}
function Write-Manifest([object]$Windows, [object]$Android, [switch]$WrongWindowsEntry) {
    $windowsHash = if ($WrongWindowsEntry) { 'f' * 64 } else { $Windows.sha256 }
    @("$windowsHash  $($Windows.fileName)", "$($Android.sha256)  $($Android.fileName)") | Set-Content -LiteralPath (Join-Path $artifactRoot 'SHA256SUMS.txt') -Encoding ascii
    Get-Artifact 'SHA256SUMS.txt'
}
function Write-Record([object]$Windows, [object]$Android, [object]$Manifest, [string]$SourceCommit, [switch]$UnknownField, [string]$Version = '1.6.0', [int]$VersionCode = 8) {
    $validationPath = if ($Version -eq '1.6.0') { 'docs/SHOWCASE_ALPHA_1_6_0_RELEASE_VALIDATION_2026-08-30.md' } else { 'docs/ENGINEERING_1_6_1_FINDING_STATUS.md' }
    $releaseNotesPath = if ($Version -eq '1.6.0') { 'docs/SHOWCASE_ALPHA_1_6_0_RELEASE_NOTES_2026-08-30.md' } else { 'docs/SHOWCASE_ALPHA_1_6_1_RELEASE_NOTES_2026-09-01.md' }
    $record = [ordered]@{ schemaVersion = 1; release = @{ version = $Version; tag = "v$Version"; repository = 'Samfa12-tech/The-Horde-RT-demo' }; source = @{ commit = $SourceCommit }; android = @{ versionCode = $VersionCode }; artifacts = @{ checksumManifest = $Manifest; windows = $Windows; android = $Android }; documentation = @{ validationPath = $validationPath; releaseNotesPath = $releaseNotesPath } }
    if ($UnknownField) { $record.unexpected = $true }
    $record | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $recordPath -Encoding utf8
}
function Write-State([string]$LocalState = 'absent', [string]$OriginState = 'absent', [object]$Release = $null, [string]$LocalTarget = '', [string]$OriginTarget = '', [string]$LocalShape = 'lightweight', [string]$OriginShape = 'lightweight') {
    $github = if ($null -eq $Release) { @{ state = 'absent' } } else { @{ state = 'present'; release = $Release } }
    @{ localTag = @{ state = $LocalState; target = $LocalTarget; shape = $LocalShape }; originTag = @{ state = $OriginState; target = $OriginTarget; shape = $OriginShape }; githubRelease = $github } | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath $statePath -Encoding utf8
}
function Write-SourceSurfaces([string]$Version = '1.6.0', [int]$VersionCode = 8, [string]$Cmake = $null, [string]$CmakeVersionModule = $null, [string]$Gradle = $null, [string]$Notes = $null, [string]$LookupFailurePath = '', [switch]$Legacy) {
    if ($Legacy) {
        if ([string]::IsNullOrEmpty($Cmake)) { $Cmake = "project(HordeLanternRT`n  VERSION 1.6.0`n  LANGUAGES CXX)" }
        if ([string]::IsNullOrEmpty($Gradle)) { $Gradle = "defaultConfig {`n        versionCode 8`n        versionName '1.6.0'`n    }" }
    } else {
        if ([string]::IsNullOrEmpty($Cmake)) { $Cmake = 'include(cmake/HordeRtVersion.cmake)' + "`n" + 'project(HordeLanternRT VERSION ${HORDE_RT_PACKAGE_VERSION} LANGUAGES CXX)' }
        if ([string]::IsNullOrEmpty($CmakeVersionModule)) {
        $CmakeVersionModule = @'
set(_horde_rt_version_file "${HORDE_RT_REPO_ROOT}/VERSION")
file(READ "${_horde_rt_version_file}" _horde_rt_version_raw)
file(STRINGS "${_horde_rt_version_file}" _horde_rt_version_lines ENCODING UTF-8)
list(GET _horde_rt_version_lines 0 HORDE_RT_PACKAGE_VERSION)
set(_horde_rt_version_code_map_file "${HORDE_RT_REPO_ROOT}/version-code-map.json")
file(READ "${_horde_rt_version_code_map_file}" _horde_rt_version_code_map)
string(JSON HORDE_RT_ANDROID_VERSION_CODE ERROR_VARIABLE _horde_rt_map_error GET
    "${_horde_rt_version_code_map}" androidVersionCodes "${HORDE_RT_PACKAGE_VERSION}")
'@
        }
        if ([string]::IsNullOrEmpty($Gradle)) {
        $Gradle = @'
def hordeVersionFile = rootProject.file('../VERSION')
def hordeVersionCodeMapFile = rootProject.file('../version-code-map.json')
def hordeVersionRaw = readHordeUtf8Authority(hordeVersionFile, 'VERSION')
def hordeVersionMatcher = hordeVersionRaw =~ /^([0-9]+\.[0-9]+\.[0-9]+)$/
def hordeVersionName = hordeVersionMatcher.group(1)
def hordeVersionCodeMapRaw = readHordeUtf8Authority(hordeVersionCodeMapFile, 'Android version-code map')
def hordeVersionCodeMap = new JsonSlurper().parseText(hordeVersionCodeMapRaw)
def hordeVersionCode = hordeVersionCodeMap?.androidVersionCodes?[(hordeVersionName)]
        versionCode hordeVersionCode.intValue()
        versionName hordeVersionName
'@
        }
    }
    if ([string]::IsNullOrEmpty($Notes)) { $Notes = 'Package version: ' + [char]96 + $Version + [char]96 + "`nAndroid version code: " + [char]96 + $VersionCode + [char]96 }
    $map = [ordered]@{ androidVersionCodes = [ordered]@{} }
    $map.androidVersionCodes[$Version] = $VersionCode
    $versionCodeMap = $map | ConvertTo-Json -Compress
    @{ version = $Version; versionCodeMap = $versionCodeMap; cmake = $Cmake; cmakeVersionModule = $CmakeVersionModule; gradle = $Gradle; notes = $Notes; lookupFailurePath = $LookupFailurePath } | ConvertTo-Json | Set-Content -LiteralPath $sourcePath -Encoding utf8
}
function Write-SourceState { @{ commitExists = $true; objectType = 'commit'; reachable = $true } | ConvertTo-Json | Set-Content -LiteralPath $sourceStatePath -Encoding utf8 }
function Invoke-Preflight([switch]$SkipRemote, [switch]$UseSourceFixture, [string]$ExpectedTargetCommit, [string]$Version = '1.6.0') {
    $arguments = @{ PreflightScript = $scriptPath; Version = $Version; ArtifactDirectory = $artifactRoot; ReportDirectory = $reportRoot; RecordPath = $recordPath; FixtureMode = $true; FixtureSourceStatePath = $sourceStatePath; FixtureSourceSurfacePath = $sourcePath; ExpectedTargetCommit = $ExpectedTargetCommit }
    if ($SkipRemote) { $arguments.SkipRemote = $true } else { $arguments.FixturePublicationStatePath = $statePath }
    $script:LastPreflightResult = Invoke-HordeGitHubReleasePreflight @arguments
    $script:LastPreflightOutput = $script:LastPreflightResult.Output
    return $script:LastPreflightResult.ExitCode
}
function Require-Exit([int]$Actual, [int]$Expected, [string]$Name) { if ($Actual -ne $Expected) { throw "$Name expected exit $Expected, got ${Actual}: $script:LastPreflightOutput" } }

try {
    $worktreeBefore = (& git -C $repoRoot status --porcelain | Out-String)
    $refsBefore = (& git -C $repoRoot show-ref --head | Out-String)
    $windowsName = 'Horde-Lantern-RT-Alpha-1.6.0-Windows-x64.zip'; $androidName = 'Horde-Lantern-RT-Alpha-1.6.0-Android.apk'
    [IO.File]::WriteAllBytes((Join-Path $artifactRoot $windowsName), [byte[]](1, 2, 3, 4)); [IO.File]::WriteAllBytes((Join-Path $artifactRoot $androidName), [byte[]](0))
    $windows = Get-Artifact $windowsName; $android = Get-Artifact $androidName; $manifest = Write-Manifest $windows $android
    $fixtureSourceCommit = (& git -C $repoRoot rev-parse HEAD).Trim()
    Write-Record $windows $android $manifest $fixtureSourceCommit; Write-State; Write-SourceSurfaces -Legacy; Write-SourceState
    $productionRecordOverride = Invoke-HordeGitHubReleasePreflight -PreflightScript $scriptPath -Version '1.6.0' -ArtifactDirectory $artifactRoot -ReportDirectory $reportRoot -RecordPath $recordPath
    $script:LastPreflightOutput = $productionRecordOverride.Output
    Require-Exit $productionRecordOverride.ExitCode 1 'production RecordPath override'
    Remove-Item -LiteralPath (Join-Path $artifactRoot $androidName) -Force; Require-Exit (Invoke-Preflight) 2 'missing artifact'
    [IO.File]::WriteAllBytes((Join-Path $artifactRoot $androidName), [byte[]](0)); Require-Exit (Invoke-Preflight) 0 'exact fixture'
    if ($script:LastPreflightOutput -notmatch '"status":"fixture-pass"' -or $script:LastPreflightOutput -match '"publicationReady":true') { throw 'fixture success must not be production-ready' }
    Require-Exit (Invoke-Preflight -SkipRemote) 0 'skipped remote fixture'; if ($script:LastPreflightOutput -match '"publicationReady":true') { throw 'skipped remote checks must not be production-ready' }

    [IO.File]::WriteAllBytes((Join-Path $artifactRoot $windowsName), [byte[]](1, 2, 3, 4, 5)); $changedWindows = Get-Artifact $windowsName
    $manifest = Write-Manifest $changedWindows $android; Write-Record ([PSCustomObject]@{ fileName = $windowsName; bytes = 4; sha256 = $changedWindows.sha256 }) $android $manifest $fixtureSourceCommit; Require-Exit (Invoke-Preflight) 1 'size-only mismatch'
    Write-Record ([PSCustomObject]@{ fileName = $windowsName; bytes = $changedWindows.bytes; sha256 = $windows.sha256 }) $android $manifest $fixtureSourceCommit; Require-Exit (Invoke-Preflight) 1 'hash-only mismatch'
    $manifest = Write-Manifest $changedWindows $android -WrongWindowsEntry; Write-Record $changedWindows $android $manifest $fixtureSourceCommit; Require-Exit (Invoke-Preflight) 1 'manifest-entry mismatch'
    $manifest = Write-Manifest $changedWindows $android; Write-Record $changedWindows $android $manifest $fixtureSourceCommit -UnknownField; Require-Exit (Invoke-Preflight) 1 'unknown schema field'
    Write-Record $changedWindows ([PSCustomObject]@{ fileName = 'Horde-Lantern-RT-Alpha-1.6.0-Android-debug.apk'; bytes = $android.bytes; sha256 = $android.sha256 }) $manifest $fixtureSourceCommit; Require-Exit (Invoke-Preflight) 1 'unsafe Android name'
    Write-SourceSurfaces -Legacy -Cmake "project(HordeLanternRT`n  VERSION 1.6.1`n  LANGUAGES CXX)"; Require-Exit (Invoke-Preflight -UseSourceFixture) 1 'legacy CMake literal version mismatch'
    Write-SourceSurfaces -Legacy -Gradle "defaultConfig {`n        versionCode 9`n        versionName '1.6.1'`n    }"; Require-Exit (Invoke-Preflight -UseSourceFixture) 1 'legacy Gradle literal version mismatch'
    Write-SourceSurfaces -Legacy -Notes 'Package version: `1.6.1`'; Require-Exit (Invoke-Preflight -UseSourceFixture) 1 'legacy release-note marker mismatch'

    # 1.6.0 predates the root VERSION/map authorities and retains its immutable literal-source contract.
    # Generated-authority fixtures apply only to the later 1.6.1 source snapshot.
    Write-Record $changedWindows $android $manifest $fixtureSourceCommit -Version '1.6.1' -VersionCode 9
    Write-SourceSurfaces -Version '1.6.1' -VersionCode 9
    Require-Exit (Invoke-Preflight -UseSourceFixture -Version '1.6.1') 0 'generated-source 1.6.1/code 9 authorities'
    Write-SourceSurfaces -Version '1.6.00' -VersionCode 9
    Require-Exit (Invoke-Preflight -UseSourceFixture -Version '1.6.1') 1 'generated-source near-match VERSION'
    Write-SourceSurfaces -Version '1.6.1' -VersionCode 8
    Require-Exit (Invoke-Preflight -UseSourceFixture -Version '1.6.1') 1 'generated-source mismatched code map'
    Write-SourceSurfaces -Version '1.6.1' -VersionCode 9 -Cmake 'project(HordeLanternRT VERSION 1.6.1 LANGUAGES CXX)'
    Require-Exit (Invoke-Preflight -UseSourceFixture -Version '1.6.1') 1 'generated-source CMake literal bypass'
    Write-SourceSurfaces -Version '1.6.1' -VersionCode 9 -Gradle "versionCode 9`nversionName '1.6.1'"
    Require-Exit (Invoke-Preflight -UseSourceFixture -Version '1.6.1') 1 'generated-source Gradle literal bypass'
    Write-SourceSurfaces -Version '1.6.1' -VersionCode 9 -CmakeVersionModule 'set(HORDE_RT_PACKAGE_VERSION "1.6.1")'
    Require-Exit (Invoke-Preflight -UseSourceFixture -Version '1.6.1') 1 'generated-source CMake helper bypass'
    Write-SourceSurfaces -Version '1.6.1' -VersionCode 9 -LookupFailurePath 'VERSION'
    Require-Exit (Invoke-Preflight -UseSourceFixture -Version '1.6.1') 1 'generated-source VERSION lookup failure'
    Write-Record $changedWindows $android $manifest $fixtureSourceCommit
    Write-SourceSurfaces -Legacy

    $release = [PSCustomObject]@{ tagName = 'v1.6.0'; targetCommitish = $fixtureSourceCommit; assets = @([PSCustomObject]@{ name = $changedWindows.fileName; state = 'uploaded'; size = $changedWindows.bytes; digest = 'sha256:' + $changedWindows.sha256 }, [PSCustomObject]@{ name = $android.fileName; state = 'uploaded'; size = $android.bytes; digest = 'sha256:' + $android.sha256 }, [PSCustomObject]@{ name = $manifest.fileName; state = 'uploaded'; size = $manifest.bytes; digest = 'sha256:' + $manifest.sha256 }) }
    Write-Record $changedWindows $android $manifest $fixtureSourceCommit; Write-State -LocalState present -OriginState present -LocalTarget $fixtureSourceCommit -OriginTarget $fixtureSourceCommit -Release $release; Require-Exit (Invoke-Preflight) 0 'matched lightweight tag and release'
    Write-State -LocalState present -OriginState present -LocalTarget $fixtureSourceCommit -OriginTarget $fixtureSourceCommit -LocalShape annotated -OriginShape annotated -Release $release; Require-Exit (Invoke-Preflight) 0 'matched annotated tag and release'
    $release.assets += [PSCustomObject]@{ name = 'unexpected.txt'; state = 'uploaded'; size = 1; digest = 'sha256:' + ('0' * 64) }; Write-State -LocalState present -OriginState present -LocalTarget $fixtureSourceCommit -OriginTarget $fixtureSourceCommit -Release $release; Require-Exit (Invoke-Preflight) 1 'mismatched release asset set'
    Write-State -LocalState present -OriginState present -LocalTarget ('1' * 40) -OriginTarget $fixtureSourceCommit; Require-Exit (Invoke-Preflight) 1 'mismatched tag target'
    Require-Exit (Invoke-Preflight -ExpectedTargetCommit ('f' * 40)) 1 'expected target/provenance mismatch'
    if ($worktreeBefore -ne (& git -C $repoRoot status --porcelain | Out-String) -or $refsBefore -ne (& git -C $repoRoot show-ref --head | Out-String)) { throw 'offline fixtures changed repository worktree or refs' }
} finally { if (Test-Path -LiteralPath $fixtureRoot) { Remove-Item -LiteralPath $fixtureRoot -Recurse -Force } }
Write-Output 'GitHub release preflight offline tests passed.'
