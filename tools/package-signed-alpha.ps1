param(
    [Parameter(Mandatory = $true)]
    [string]$KeyStorePath,
    [string]$KeyAlias = "horde-lantern-rt",
    [string]$Version = "0.1.0-alpha.1"
)

$ErrorActionPreference = "Stop"
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$keyStoreFull = [IO.Path]::GetFullPath($KeyStorePath)
if (-not (Test-Path -LiteralPath $keyStoreFull)) {
    throw "Release keystore not found: $keyStoreFull"
}

$storeSecure = Read-Host "Keystore password" -AsSecureString
$keySecure = Read-Host "Key password" -AsSecureString
$storePassword = [Net.NetworkCredential]::new("", $storeSecure).Password
$keyPassword = [Net.NetworkCredential]::new("", $keySecure).Password
try {
    $env:HORDE_RELEASE_STORE_FILE = $keyStoreFull
    $env:HORDE_RELEASE_STORE_PASSWORD = $storePassword
    $env:HORDE_RELEASE_KEY_ALIAS = $KeyAlias
    $env:HORDE_RELEASE_KEY_PASSWORD = $keyPassword

    & (Join-Path $PSScriptRoot "package-alpha.ps1") -Version $Version
    if ($LASTEXITCODE -ne 0) { throw "Signed alpha packaging failed." }

    $safeVersion = $Version -replace '[^0-9A-Za-z.-]', '-'
    $apk = Join-Path $repoRoot "releases\candidates\Horde-Lantern-RT-Alpha-$safeVersion-Android.apk"
    if (-not (Test-Path -LiteralPath $apk)) { throw "Signed candidate was not produced: $apk" }

    $buildTools = Get-ChildItem "$env:LOCALAPPDATA\Android\Sdk\build-tools" -Directory |
        Sort-Object Name -Descending | Select-Object -First 1
    $apksigner = Join-Path $buildTools.FullName "apksigner.bat"
    $verification = & $apksigner verify --verbose --print-certs $apk 2>&1
    if ($LASTEXITCODE -ne 0) { throw "apksigner rejected the signed candidate.`n$verification" }
    if ($verification -match "CN=Android Debug") { throw "Refusing a debug-certificate candidate." }

    Write-Host $verification
    Write-Host "Signed Android candidate ready: $apk"
} finally {
    foreach ($name in @(
        'HORDE_RELEASE_STORE_FILE', 'HORDE_RELEASE_STORE_PASSWORD',
        'HORDE_RELEASE_KEY_ALIAS', 'HORDE_RELEASE_KEY_PASSWORD')) {
        Remove-Item "Env:$name" -ErrorAction SilentlyContinue
    }
    $storePassword = $null
    $keyPassword = $null
}
