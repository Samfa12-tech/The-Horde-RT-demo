param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[0-9A-Za-z][0-9A-Za-z.+-]*$')]
    [string]$Version,

    [string]$ReleaseName,
    [string]$NotesFile,
    [switch]$Draft,
    [switch]$Prerelease = $true
)

$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$candidateRoot = Join-Path $repoRoot 'releases\candidates'
$safeVersion = $Version -replace '[^0-9A-Za-z.-]', '-'
$tag = "v$Version"
$baseName = "Horde-Lantern-RT-Alpha-$safeVersion"
$windowsZip = Join-Path $candidateRoot "$baseName-Windows-x64.zip"
$androidApk = Join-Path $candidateRoot "$baseName-Android.apk"
$hashFile = Join-Path $candidateRoot 'SHA256SUMS.txt'

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

Require-File $windowsZip 'Windows release ZIP'
Require-File $androidApk 'Signed Android release APK'
Require-File $hashFile 'Release hash manifest'

if ($androidApk -match '(?i)(debug|unsigned|do-not-publish)') {
    throw "Refusing unsafe Android candidate name: $androidApk"
}

$expectedHashes = Read-ExpectedHashes $hashFile
Assert-Hash $windowsZip $expectedHashes
Assert-Hash $androidApk $expectedHashes

& gh auth status
if ($LASTEXITCODE -ne 0) {
    throw 'GitHub CLI is not authenticated. Run: gh auth login'
}

Push-Location $repoRoot
try {
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

    $generatedNotes = Join-Path $env:TEMP "horde-github-release-$safeVersion.md"
    if ([string]::IsNullOrWhiteSpace($NotesFile)) {
        @"
# Horde Lantern RT $Version

Native Vulkan hardware-ray-tracing technology demo for Windows and compatible Android devices.

## Downloads

- Windows x64 ZIP
- Signed Android APK
- SHA-256 checksum manifest

The same release is also available from itch.io:
https://samfa12.itch.io/the-horde

Hardware support is intentionally narrow. Unsupported devices show explicit diagnostics rather than using a raster or simulated ray-tracing fallback.
"@ | Set-Content -LiteralPath $generatedNotes -Encoding utf8
        $NotesFile = $generatedNotes
    } else {
        $NotesFile = [IO.Path]::GetFullPath($NotesFile)
        Require-File $NotesFile 'Release notes file'
    }

    $arguments = @(
        'release', 'create', $tag,
        '--repo', 'Samfa12-tech/The-Horde-RT-demo',
        '--target', 'main',
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
