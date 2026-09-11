Set-StrictMode -Version Latest

function Invoke-HordeGitHubReleasePreflight {
    param(
        [Parameter(Mandatory = $true)][string]$PreflightScript,
        [Parameter(Mandatory = $true)][string]$Version,
        [Parameter(Mandatory = $true)][string]$ArtifactDirectory,
        [string]$ExpectedTargetCommit,
        [string]$RecordPath,
        [string]$ReportDirectory,
        [string]$FixturePublicationStatePath,
        [string]$FixtureSourceSurfacePath,
        [string]$FixtureSourceStatePath,
        [switch]$FixtureMode,
        [switch]$SkipRemote
    )

    if (-not (Test-Path -LiteralPath $PreflightScript -PathType Leaf)) {
        throw "Preflight script not found: $PreflightScript"
    }
    $preflightHostExecutable = if ($PSVersionTable.PSEdition -eq 'Core') { Join-Path $PSHOME 'pwsh.exe' } else { Join-Path $PSHOME 'powershell.exe' }
    $arguments = @('-NoProfile', '-File', $PreflightScript, '-Version', $Version, '-ArtifactDirectory', $ArtifactDirectory)
    foreach ($optional in @(@('ExpectedTargetCommit', $ExpectedTargetCommit), @('RecordPath', $RecordPath), @('ReportDirectory', $ReportDirectory),
                               @('FixturePublicationStatePath', $FixturePublicationStatePath), @('FixtureSourceSurfacePath', $FixtureSourceSurfacePath),
                               @('FixtureSourceStatePath', $FixtureSourceStatePath))) {
        if (-not [string]::IsNullOrWhiteSpace([string]$optional[1])) {
            $arguments += ('-' + $optional[0])
            $arguments += [string]$optional[1]
        }
    }
    if ($FixtureMode) { $arguments += '-FixtureMode' }
    if ($SkipRemote) { $arguments += '-SkipRemote' }
    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $output = & $preflightHostExecutable @arguments 2>&1 | Out-String
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
    return [PSCustomObject]@{ ExitCode = $exitCode; Output = $output }
}
