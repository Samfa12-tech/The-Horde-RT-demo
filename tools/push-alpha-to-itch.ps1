param(
    [string]$Version = "0.1.0-alpha.1",
    [ValidateSet("Both", "Windows", "Android")]
    [string]$Channels = "Both",
    [switch]$ConfirmPush,
    [string]$ButlerPath = "C:\Dev\tools\butler\butler.exe"
)

$ErrorActionPreference = "Stop"
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$candidateRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot "releases\candidates"))
$safeVersion = $Version -replace '[^0-9A-Za-z.-]', '-'
$baseName = "Horde-Lantern-RT-Alpha-$safeVersion"
$windowsZip = Join-Path $candidateRoot "$baseName-Windows-x64.zip"
$androidApk = Join-Path $candidateRoot "$baseName-Android.apk"
$hashFile = Join-Path $candidateRoot "SHA256SUMS.txt"
$windowsTarget = "samfa12/the-horde:windows-x64"
$androidTarget = "samfa12/the-horde:android"

if (-not (Test-Path -LiteralPath $ButlerPath)) { throw "Butler not found: $ButlerPath" }
if (-not (Test-Path -LiteralPath $hashFile)) { throw "Candidate hashes are missing: $hashFile" }
if ($Channels -in @("Both", "Windows") -and -not (Test-Path -LiteralPath $windowsZip)) {
    throw "Windows candidate is missing: $windowsZip"
}
if ($Channels -in @("Both", "Android") -and -not (Test-Path -LiteralPath $androidApk)) {
    throw "A stable-key-signed Android candidate is missing: $androidApk"
}

if ($Channels -in @("Both", "Android")) {
    $buildTools = Get-ChildItem "$env:LOCALAPPDATA\Android\Sdk\build-tools" -Directory |
        Sort-Object Name -Descending | Select-Object -First 1
    $apksigner = Join-Path $buildTools.FullName "apksigner.bat"
    $verification = & $apksigner verify --verbose --print-certs $androidApk 2>&1
    if ($LASTEXITCODE -ne 0) { throw "apksigner rejected the Android candidate.`n$verification" }
    if ($verification -match "CN=Android Debug") { throw "Refusing to publish an Android Debug certificate." }
}

$hashLines = Get-Content -LiteralPath $hashFile
$selectedArtifacts = @()
if ($Channels -in @("Both", "Windows")) { $selectedArtifacts += $windowsZip }
if ($Channels -in @("Both", "Android")) { $selectedArtifacts += $androidApk }
foreach ($artifact in $selectedArtifacts) {
    $expectedLine = $hashLines | Where-Object { $_ -match "  $([regex]::Escape([IO.Path]::GetFileName($artifact)))$" } | Select-Object -First 1
    if (-not $expectedLine) { throw "No SHA-256 record for $artifact" }
    $expected = ($expectedLine -split '\s+')[0]
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $artifact).Hash.ToLowerInvariant()
    if ($actual -ne $expected) { throw "SHA-256 mismatch for $artifact" }
}

& $ButlerPath status samfa12/the-horde
if ($LASTEXITCODE -ne 0) { throw "Butler could not access samfa12/the-horde." }

Write-Host "Preflight passed."
if ($Channels -in @("Both", "Windows")) { Write-Host "Windows -> $windowsTarget" }
if ($Channels -in @("Both", "Android")) { Write-Host "Android -> $androidTarget" }
if (-not $ConfirmPush) {
    Write-Host "No upload performed. Re-run with -ConfirmPush after final approval."
    exit 0
}

$stage = [IO.Path]::GetFullPath((Join-Path $candidateRoot ".butler-windows-stage"))
if (-not $stage.StartsWith($candidateRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Unsafe Butler staging path: $stage"
}
try {
    if ($Channels -in @("Both", "Windows")) {
        if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
        Expand-Archive -LiteralPath $windowsZip -DestinationPath $stage
        & $ButlerPath push $stage $windowsTarget --userversion $Version
        if ($LASTEXITCODE -ne 0) { throw "Windows Butler push failed." }
    }
    if ($Channels -in @("Both", "Android")) {
        & $ButlerPath push $androidApk $androidTarget --userversion $Version
        if ($LASTEXITCODE -ne 0) { throw "Android Butler push failed." }
    }
} finally {
    if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
}

Write-Host "Requested itch channel push completed."
