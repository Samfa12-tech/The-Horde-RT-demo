$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$scriptPath = Join-Path $repoRoot 'tools\preflight-github-release.ps1'
$powerShellExecutable = if ($PSVersionTable.PSEdition -eq 'Core') {
    Join-Path $PSHOME 'pwsh.exe'
} else {
    Join-Path $PSHOME 'powershell.exe'
}
$fixtureRoot = Join-Path ([IO.Path]::GetTempPath()) ('horde-release-preflight-' + [Guid]::NewGuid().ToString('N'))
$artifactRoot = Join-Path $fixtureRoot 'artifacts'
$reportRoot = Join-Path $fixtureRoot 'reports'
[IO.Directory]::CreateDirectory($artifactRoot) | Out-Null

function Write-Record {
    param([string]$Path, [string]$Commit, [string]$WindowsHash, [long]$WindowsBytes)
    $zeroHash = '6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d'
    @{
        schemaVersion = 1
        release = @{ version = '1.6.0'; tag = 'v1.6.0'; repository = 'Samfa12-tech/The-Horde-RT-demo' }
        source = @{ commit = $Commit }
        android = @{ versionCode = 8 }
        artifacts = @{
            checksumManifest = 'SHA256SUMS.txt'
            windows = @{ fileName = 'Horde-Lantern-RT-Alpha-1.6.0-Windows-x64.zip'; bytes = $WindowsBytes; sha256 = $WindowsHash }
            android = @{ fileName = 'Horde-Lantern-RT-Alpha-1.6.0-Android.apk'; bytes = 1; sha256 = $zeroHash }
        }
        documentation = @{
            validationPath = 'docs/SHOWCASE_ALPHA_1_6_0_RELEASE_VALIDATION_2026-08-30.md'
            releaseNotesPath = 'docs/SHOWCASE_ALPHA_1_6_0_RELEASE_NOTES_2026-08-30.md'
        }
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $Path -Encoding utf8
}

function Invoke-Preflight {
    param([string]$RecordPath)
    $script:LastPreflightOutput = (& $powerShellExecutable -NoProfile -File $scriptPath -Version '1.6.0' -ArtifactDirectory $artifactRoot -ReportDirectory $reportRoot -RecordPath $RecordPath -SkipRemote -FixtureMode | Out-String)
    return $LASTEXITCODE
}

try {
    $workingTreeBefore = (& git -C $repoRoot status --porcelain | Out-String)
    $windowsPath = Join-Path $artifactRoot 'Horde-Lantern-RT-Alpha-1.6.0-Windows-x64.zip'
    [IO.File]::WriteAllBytes($windowsPath, [byte[]](1, 2, 3, 4))
    $windowsHash = (Get-FileHash -LiteralPath $windowsPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $recordPath = Join-Path $fixtureRoot 'record.json'
    $sourceCommit = '57c81b635a6c10e2772283639026936adac80f8b'
    $publisher = Get-Content -LiteralPath (Join-Path $repoRoot 'tools\publish-github-release.ps1') -Raw -Encoding utf8
    if ($publisher -notmatch 'TargetCommit' -or $publisher -notmatch 'ExpectedTargetCommit' -or
        $publisher -match "--target', 'main'" -or $publisher -notmatch '--target'', \$TargetCommit') {
        throw 'publisher must require the preflight-bound explicit target commit and must not target main'
    }

    Write-Record -Path $recordPath -Commit $sourceCommit -WindowsHash $windowsHash -WindowsBytes 4
    $exitCode = Invoke-Preflight -RecordPath $recordPath
    if ($exitCode -ne 2) { throw "missing Android fixture must be incomplete (exit 2), got $exitCode" }

    [IO.File]::WriteAllBytes((Join-Path $artifactRoot 'Horde-Lantern-RT-Alpha-1.6.0-Android.apk'), [byte[]](0))
    @("$windowsHash  Horde-Lantern-RT-Alpha-1.6.0-Windows-x64.zip", ('6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d  Horde-Lantern-RT-Alpha-1.6.0-Android.apk')) |
        Set-Content -LiteralPath (Join-Path $artifactRoot 'SHA256SUMS.txt') -Encoding ascii
    $exitCode = Invoke-Preflight -RecordPath $recordPath
    if ($exitCode -ne 0) { throw "exact fixture size/hash/manifest preflight must pass, got ${exitCode}: $script:LastPreflightOutput" }

    & $powerShellExecutable -NoProfile -File $scriptPath -Version '1.6.0' -ArtifactDirectory $artifactRoot -ReportDirectory $reportRoot -RecordPath $recordPath -ExpectedTargetCommit ('f' * 40) -SkipRemote -FixtureMode | Out-Null
    if ($LASTEXITCODE -ne 1) { throw 'a target commit that disagrees with immutable provenance must fail closed' }

    [IO.File]::WriteAllBytes($windowsPath, [byte[]](9, 9, 9, 9))
    $exitCode = Invoke-Preflight -RecordPath $recordPath
    if ($exitCode -ne 1) { throw "hash mismatch must fail closed (exit 1), got $exitCode" }

    [IO.File]::WriteAllBytes($windowsPath, [byte[]](1, 2, 3, 4))
    $sourceMismatchCommit = (& git -C $repoRoot rev-parse HEAD).Trim()
    Write-Record -Path $recordPath -Commit $sourceMismatchCommit -WindowsHash $windowsHash -WindowsBytes 4
    $exitCode = Invoke-Preflight -RecordPath $recordPath
    if ($exitCode -ne 1) { throw "source mismatch must fail closed (exit 1), got $exitCode" }

    '{"schemaVersion":1}' | Set-Content -LiteralPath $recordPath -Encoding utf8
    $exitCode = Invoke-Preflight -RecordPath $recordPath
    if ($exitCode -ne 1) { throw "malformed schema must fail closed (exit 1), got $exitCode" }
    $workingTreeAfter = (& git -C $repoRoot status --porcelain | Out-String)
    if ($workingTreeBefore -ne $workingTreeAfter) { throw 'offline preflight fixture changed tracked or untracked repository state' }
} finally {
    if (Test-Path -LiteralPath $fixtureRoot) { Remove-Item -LiteralPath $fixtureRoot -Recurse -Force }
}

Write-Output 'GitHub release preflight offline tests passed.'
