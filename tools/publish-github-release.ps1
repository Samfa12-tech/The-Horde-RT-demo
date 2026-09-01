param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[0-9A-Za-z][0-9A-Za-z.+-]*$')]
    [string]$Version,

    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[0-9a-fA-F]{40}$')]
    [string]$TargetCommit,

    [string]$ReleaseName,
    [string]$NotesFile,
    [string]$ArtifactDirectory,
    [switch]$Draft,
    [switch]$Prerelease = $true,
    [switch]$PreflightOnly
)

$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if ([string]::IsNullOrWhiteSpace($ArtifactDirectory)) { $candidateRoot = Join-Path $repoRoot 'releases\candidates' }
else { $candidateRoot = [IO.Path]::GetFullPath($ArtifactDirectory) }
$safeVersion = $Version -replace '[^0-9A-Za-z.-]', '-'
$tag = "v$Version"
$baseName = "Horde-Lantern-RT-Alpha-$safeVersion"
$windowsZip = Join-Path $candidateRoot "$baseName-Windows-x64.zip"
$androidApk = Join-Path $candidateRoot "$baseName-Android.apk"
$hashFile = Join-Path $candidateRoot 'SHA256SUMS.txt'
$preflightScript = Join-Path $PSScriptRoot 'preflight-github-release.ps1'
$preflightHost = if ($PSVersionTable.PSEdition -eq 'Core') { Join-Path $PSHOME 'pwsh.exe' } else { Join-Path $PSHOME 'powershell.exe' }

function Require-File([string]$Path, [string]$Description) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description not found: $Path"
    }
}

function Read-ExpectedHashes([string]$Path) {
    $map = @{}
    foreach ($line in Get-Content -LiteralPath $Path) {
        if ($line -match '^\s*([0-9a-fA-F]{64})\s+\*?(.+?)\s*$') {
            $map[$matches[2]] = $matches[1].ToLowerInvariant()
        }
    }
    return $map
}

function Assert-Hash([string]$Path, [hashtable]$Expected) {
    $name = [IO.Path]::GetFileName($Path)
    if (-not $Expected.ContainsKey($name)) {
        throw "SHA256SUMS.txt has no entry for $name"
    }
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash.ToLowerInvariant()
    if ($actual -ne $Expected[$name]) {
        throw "SHA-256 mismatch for $name. Expected $($Expected[$name]); got $actual"
    }
}

foreach ($command in @('git', 'gh')) {
    if (-not (Get-Command $command -ErrorAction SilentlyContinue)) {
        throw "Required command '$command' is not installed or not on PATH."
    }
}

Require-File $preflightScript 'Exact release-provenance preflight'
& $preflightHost -NoProfile -File $preflightScript -Version $Version -ArtifactDirectory $candidateRoot -ExpectedTargetCommit $TargetCommit
$preflightExitCode = $LASTEXITCODE
if ($preflightExitCode -ne 0) {
    throw "Exact release-provenance preflight did not pass for $Version."
}

Require-File $windowsZip 'Windows release ZIP'
Require-File $androidApk 'Signed Android release APK'
Require-File $hashFile 'Release hash manifest'

if ($androidApk -match '(?i)(debug|unsigned|do-not-publish)') {
    throw "Refusing unsafe Android candidate name: $androidApk"
}

$expectedHashes = Read-ExpectedHashes $hashFile
Assert-Hash $windowsZip $expectedHashes
Assert-Hash $androidApk $expectedHashes

if ($PreflightOnly) {
    Write-Host "Exact release-provenance preflight passed for $Version / $TargetCommit."
    return
}

& gh auth status
if ($LASTEXITCODE -ne 0) {
    throw 'GitHub CLI is not authenticated. Run: gh auth login'
}

Push-Location $repoRoot
try {
    $resolvedTarget = (& git rev-parse "$TargetCommit^{commit}").Trim().ToLowerInvariant()
    if ($LASTEXITCODE -ne 0 -or $resolvedTarget -ne $TargetCommit.ToLowerInvariant()) {
        throw "TargetCommit must resolve to the exact supplied full commit: $TargetCommit"
    }
    $remote = (& git remote get-url origin).Trim()
    if ($remote -notmatch 'Samfa12-tech/The-Horde-RT-demo(?:\.git)?$') {
        throw "Unexpected origin remote: $remote"
    }

    & gh release view $tag --repo Samfa12-tech/The-Horde-RT-demo *> $null
    if ($LASTEXITCODE -eq 0) {
        throw "GitHub Release $tag already exists. Refusing to overwrite it."
    }

    if ([string]::IsNullOrWhiteSpace($ReleaseName)) {
        $ReleaseName = "Horde Lantern RT $Version"
    }

    $provenancePath = Join-Path $repoRoot "release-provenance\horde-lantern-rt-alpha-$Version.json"
    Require-File $provenancePath 'Exact release-provenance record'
    $provenance = Get-Content -LiteralPath $provenancePath -Raw -Encoding utf8 | ConvertFrom-Json
    $plannedNotesFile = [IO.Path]::GetFullPath((Join-Path $repoRoot $provenance.documentation.releaseNotesPath))
    Require-File $plannedNotesFile 'Planned release notes file'
    if ([string]::IsNullOrWhiteSpace($NotesFile)) { $NotesFile = $plannedNotesFile }
    else {
        $NotesFile = [IO.Path]::GetFullPath($NotesFile)
        Require-File $NotesFile 'Release notes file'
        if ($NotesFile -ne $plannedNotesFile) { throw "Release notes must match the provenance attachment plan: $plannedNotesFile" }
    }

    $arguments = @(
        'release', 'create', $tag,
        '--repo', 'Samfa12-tech/The-Horde-RT-demo',
        '--target', $TargetCommit,
        '--title', $ReleaseName,
        '--notes-file', $NotesFile
    )
    if ($Draft) { $arguments += '--draft' }
    if ($Prerelease) { $arguments += '--prerelease' }
    $arguments += @(
        "$windowsZip#Windows x64",
        "$androidApk#Android APK",
        "$hashFile#SHA-256 checksums"
    )

    & gh @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "GitHub Release publication failed for $tag"
    }

    Write-Host "Published GitHub Release $tag with Windows, Android, and checksum assets."
} finally {
    Pop-Location
}
