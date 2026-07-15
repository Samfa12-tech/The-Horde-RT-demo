param(
    [string]$KeyStorePath = (Join-Path $HOME "Documents\Android Release Keys\Horde Lantern RT\horde-lantern-rt-release.jks"),
    [string]$KeyAlias = "horde-lantern-rt",
    [string]$DistinguishedName = "CN=Samfa12, OU=Games, O=Samfa12, C=AU"
)

$ErrorActionPreference = "Stop"
$keytool = (Get-Command keytool -ErrorAction Stop).Source
$keyStoreFull = [IO.Path]::GetFullPath($KeyStorePath)
if (Test-Path -LiteralPath $keyStoreFull) {
    throw "Refusing to replace an existing signing key: $keyStoreFull"
}

$parent = Split-Path -Parent $keyStoreFull
New-Item -ItemType Directory -Force -Path $parent | Out-Null

function Read-ConfirmedSecret([string]$Label) {
    $first = Read-Host "$Label" -AsSecureString
    $second = Read-Host "Confirm $Label" -AsSecureString
    $firstText = [Net.NetworkCredential]::new("", $first).Password
    $secondText = [Net.NetworkCredential]::new("", $second).Password
    if ($firstText.Length -lt 12) { throw "$Label must be at least 12 characters." }
    if ($firstText -cne $secondText) { throw "$Label values did not match." }
    return $firstText
}

$storePassword = Read-ConfirmedSecret "Keystore password"
$keyPassword = Read-ConfirmedSecret "Key password"
try {
    $env:HORDE_KEYTOOL_STORE_PASSWORD = $storePassword
    $env:HORDE_KEYTOOL_KEY_PASSWORD = $keyPassword
    & $keytool -genkeypair `
        -keystore $keyStoreFull `
        -storetype JKS `
        -storepass:env HORDE_KEYTOOL_STORE_PASSWORD `
        -keypass:env HORDE_KEYTOOL_KEY_PASSWORD `
        -alias $KeyAlias `
        -keyalg RSA `
        -keysize 4096 `
        -validity 10000 `
        -dname $DistinguishedName
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $keyStoreFull)) {
        throw "keytool did not create the release key."
    }

    @"
HORDE LANTERN RT ANDROID SIGNING KEY

Key file: $keyStoreFull
Alias: $KeyAlias

Back up this JKS and both passwords in a secure password manager before publishing.
Never commit the JKS or passwords to Git. Android updates must use this same key.

Use tools/package-signed-alpha.ps1 to build a signed candidate without placing
passwords in the repository or command-line arguments.
"@ | Set-Content -LiteralPath (Join-Path $parent "SIGNING_KEY_BACKUP_REQUIRED.txt") -Encoding utf8

    Write-Host "Created dedicated Horde Lantern RT release key: $keyStoreFull"
    Write-Host "Back it up before the first public push."
} finally {
    Remove-Item Env:HORDE_KEYTOOL_STORE_PASSWORD -ErrorAction SilentlyContinue
    Remove-Item Env:HORDE_KEYTOOL_KEY_PASSWORD -ErrorAction SilentlyContinue
    $storePassword = $null
    $keyPassword = $null
}
