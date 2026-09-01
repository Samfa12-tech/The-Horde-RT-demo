$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$scriptPath = Join-Path $repoRoot 'tools\preflight-github-release.ps1'
$publisherPath = Join-Path $repoRoot 'tools\publish-github-release.ps1'
$powerShellExecutable = if ($PSVersionTable.PSEdition -eq 'Core') { Join-Path $PSHOME 'pwsh.exe' } else { Join-Path $PSHOME 'powershell.exe' }
$fixtureRoot = Join-Path ([IO.Path]::GetTempPath()) ('horde-release-preflight-' + [Guid]::NewGuid().ToString('N'))
$artifactRoot = Join-Path $fixtureRoot 'artifacts'
$reportRoot = Join-Path $fixtureRoot 'reports'
$recordPath = Join-Path $fixtureRoot 'record.json'
$statePath = Join-Path $fixtureRoot 'publication-state.json'
$sourcePath = Join-Path $fixtureRoot 'source-surfaces.json'
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
function Write-Record([object]$Windows, [object]$Android, [object]$Manifest, [switch]$UnknownField) {
    $record = [ordered]@{ schemaVersion = 1; release = @{ version = '1.6.0'; tag = 'v1.6.0'; repository = 'Samfa12-tech/The-Horde-RT-demo' }; source = @{ commit = '57c81b635a6c10e2772283639026936adac80f8b' }; android = @{ versionCode = 8 }; artifacts = @{ checksumManifest = $Manifest; windows = $Windows; android = $Android }; documentation = @{ validationPath = 'docs/SHOWCASE_ALPHA_1_6_0_RELEASE_VALIDATION_2026-08-30.md'; releaseNotesPath = 'docs/SHOWCASE_ALPHA_1_6_0_RELEASE_NOTES_2026-08-30.md' } }
    if ($UnknownField) { $record.unexpected = $true }
    $record | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $recordPath -Encoding utf8
}
function Write-State([string]$LocalState = 'absent', [string]$OriginState = 'absent', [object]$Release = $null, [string]$LocalTarget = '', [string]$OriginTarget = '', [string]$LocalShape = 'lightweight', [string]$OriginShape = 'lightweight') {
    $github = if ($null -eq $Release) { @{ state = 'absent' } } else { @{ state = 'present'; release = $Release } }
    @{ localTag = @{ state = $LocalState; target = $LocalTarget; shape = $LocalShape }; originTag = @{ state = $OriginState; target = $OriginTarget; shape = $OriginShape }; githubRelease = $github } | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath $statePath -Encoding utf8
}
function Write-SourceSurfaces([string]$Cmake = "project(x`n VERSION 1.6.0`n)", [string]$Gradle = "versionCode 8`nversionName '1.6.0'", [string]$Notes = ('Package version: ' + [char]96 + '1.6.0' + [char]96 + "`nAndroid version code: " + [char]96 + '8' + [char]96)) {
    @{ cmake = $Cmake; gradle = $Gradle; notes = $Notes } | ConvertTo-Json | Set-Content -LiteralPath $sourcePath -Encoding utf8
}
function Invoke-Preflight([switch]$SkipRemote, [switch]$UseSourceFixture) {
    $arguments = @('-NoProfile', '-File', $scriptPath, '-Version', '1.6.0', '-ArtifactDirectory', $artifactRoot, '-ReportDirectory', $reportRoot, '-RecordPath', $recordPath, '-FixtureMode')
    if ($SkipRemote) { $arguments += '-SkipRemote' } else { $arguments += @('-FixturePublicationStatePath', $statePath) }
    if ($UseSourceFixture) { $arguments += @('-FixtureSourceSurfacePath', $sourcePath) }
    $script:LastPreflightOutput = (& $powerShellExecutable @arguments | Out-String)
    return $LASTEXITCODE
}
function Require-Exit([int]$Actual, [int]$Expected, [string]$Name) { if ($Actual -ne $Expected) { throw "$Name expected exit $Expected, got ${Actual}: $script:LastPreflightOutput" } }

try {
    $worktreeBefore = (& git -C $repoRoot status --porcelain | Out-String)
    $refsBefore = (& git -C $repoRoot show-ref --head | Out-String)
    $windowsName = 'Horde-Lantern-RT-Alpha-1.6.0-Windows-x64.zip'; $androidName = 'Horde-Lantern-RT-Alpha-1.6.0-Android.apk'
    [IO.File]::WriteAllBytes((Join-Path $artifactRoot $windowsName), [byte[]](1, 2, 3, 4)); [IO.File]::WriteAllBytes((Join-Path $artifactRoot $androidName), [byte[]](0))
    $windows = Get-Artifact $windowsName; $android = Get-Artifact $androidName; $manifest = Write-Manifest $windows $android
    Write-Record $windows $android $manifest; Write-State; Write-SourceSurfaces
    Remove-Item -LiteralPath (Join-Path $artifactRoot $androidName) -Force; Require-Exit (Invoke-Preflight) 2 'missing artifact'
    [IO.File]::WriteAllBytes((Join-Path $artifactRoot $androidName), [byte[]](0)); Require-Exit (Invoke-Preflight) 0 'exact fixture'
    if ($script:LastPreflightOutput -notmatch '"status":"fixture-pass"' -or $script:LastPreflightOutput -match '"publicationReady":true') { throw 'fixture success must not be production-ready' }
    Require-Exit (Invoke-Preflight -SkipRemote) 0 'skipped remote fixture'; if ($script:LastPreflightOutput -match '"publicationReady":true') { throw 'skipped remote checks must not be production-ready' }

    [IO.File]::WriteAllBytes((Join-Path $artifactRoot $windowsName), [byte[]](1, 2, 3, 4, 5)); $changedWindows = Get-Artifact $windowsName
    $manifest = Write-Manifest $changedWindows $android; Write-Record ([PSCustomObject]@{ fileName = $windowsName; bytes = 4; sha256 = $changedWindows.sha256 }) $android $manifest; Require-Exit (Invoke-Preflight) 1 'size-only mismatch'
    Write-Record ([PSCustomObject]@{ fileName = $windowsName; bytes = $changedWindows.bytes; sha256 = $windows.sha256 }) $android $manifest; Require-Exit (Invoke-Preflight) 1 'hash-only mismatch'
    $manifest = Write-Manifest $changedWindows $android -WrongWindowsEntry; Write-Record $changedWindows $android $manifest; Require-Exit (Invoke-Preflight) 1 'manifest-entry mismatch'
    $manifest = Write-Manifest $changedWindows $android; Write-Record $changedWindows $android $manifest -UnknownField; Require-Exit (Invoke-Preflight) 1 'unknown schema field'
    Write-Record $changedWindows ([PSCustomObject]@{ fileName = 'Horde-Lantern-RT-Alpha-1.6.0-Android-debug.apk'; bytes = $android.bytes; sha256 = $android.sha256 }) $manifest; Require-Exit (Invoke-Preflight) 1 'unsafe Android name'
    Write-Record $changedWindows $android $manifest; Write-SourceSurfaces -Cmake "project(x`n VERSION 1.6.00`n)"; Require-Exit (Invoke-Preflight -UseSourceFixture) 1 'near-match source version'
    Write-SourceSurfaces -Gradle "versionCode 80`nversionName '1.6.0'"; Require-Exit (Invoke-Preflight -UseSourceFixture) 1 'near-match Android code'
    Write-SourceSurfaces

    $release = [PSCustomObject]@{ tagName = 'v1.6.0'; targetCommitish = '57c81b635a6c10e2772283639026936adac80f8b'; assets = @([PSCustomObject]@{ name = $changedWindows.fileName; state = 'uploaded'; size = $changedWindows.bytes; digest = 'sha256:' + $changedWindows.sha256 }, [PSCustomObject]@{ name = $android.fileName; state = 'uploaded'; size = $android.bytes; digest = 'sha256:' + $android.sha256 }, [PSCustomObject]@{ name = $manifest.fileName; state = 'uploaded'; size = $manifest.bytes; digest = 'sha256:' + $manifest.sha256 }) }
    Write-Record $changedWindows $android $manifest; Write-State -LocalState present -OriginState present -LocalTarget '57c81b635a6c10e2772283639026936adac80f8b' -OriginTarget '57c81b635a6c10e2772283639026936adac80f8b' -Release $release; Require-Exit (Invoke-Preflight) 0 'matched lightweight tag and release'
    Write-State -LocalState present -OriginState present -LocalTarget '57c81b635a6c10e2772283639026936adac80f8b' -OriginTarget '57c81b635a6c10e2772283639026936adac80f8b' -LocalShape annotated -OriginShape annotated -Release $release; Require-Exit (Invoke-Preflight) 0 'matched annotated tag and release'
    $release.assets += [PSCustomObject]@{ name = 'unexpected.txt'; state = 'uploaded'; size = 1; digest = 'sha256:' + ('0' * 64) }; Write-State -LocalState present -OriginState present -LocalTarget '57c81b635a6c10e2772283639026936adac80f8b' -OriginTarget '57c81b635a6c10e2772283639026936adac80f8b' -Release $release; Require-Exit (Invoke-Preflight) 1 'mismatched release asset set'
    Write-State -LocalState present -OriginState present -LocalTarget ('1' * 40) -OriginTarget '57c81b635a6c10e2772283639026936adac80f8b'; Require-Exit (Invoke-Preflight) 1 'mismatched tag target'

    $publisher = Get-Content -LiteralPath $publisherPath -Raw -Encoding utf8; if ($publisher -notmatch 'preflightExitCode' -or $publisher -match "--target', 'main'") { throw 'publisher must use an explicit preflight process contract and never target main' }
    $realCandidates = 'C:\Users\sam_s\Documents\the Horde RT Demo\releases\candidates'
    & $powerShellExecutable -NoProfile -File $publisherPath -Version 1.6.0 -TargetCommit 57c81b635a6c10e2772283639026936adac80f8b -ArtifactDirectory $realCandidates -PreflightOnly | Out-Null; if ($LASTEXITCODE -ne 0) { throw 'publisher-facing preflight-only contract must succeed without gh release create' }
    if ($worktreeBefore -ne (& git -C $repoRoot status --porcelain | Out-String) -or $refsBefore -ne (& git -C $repoRoot show-ref --head | Out-String)) { throw 'offline fixtures changed repository worktree or refs' }
} finally { if (Test-Path -LiteralPath $fixtureRoot) { Remove-Item -LiteralPath $fixtureRoot -Recurse -Force } }
Write-Output 'GitHub release preflight offline tests passed.'
