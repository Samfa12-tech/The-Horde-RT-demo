$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

. (Join-Path $PSScriptRoot "..\tools\version-contract.ps1")

$fixtureRoot = Join-Path ([IO.Path]::GetTempPath()) ("horde-rt-version-contract-" + [Guid]::NewGuid().ToString("N"))
[IO.Directory]::CreateDirectory($fixtureRoot) | Out-Null
$versionPath = Join-Path $fixtureRoot "VERSION"
$mapPath = Join-Path $fixtureRoot "version-code-map.json"
$utf8 = [Text.UTF8Encoding]::new($false)

function Write-FixtureBytes {
    param([byte[]]$VersionBytes, [byte[]]$MapBytes)

    [IO.File]::WriteAllBytes($versionPath, $VersionBytes)
    [IO.File]::WriteAllBytes($mapPath, $MapBytes)
}

function Assert-Identity {
    param(
        [string]$Name,
        [byte[]]$VersionBytes,
        [byte[]]$MapBytes,
        [bool]$ShouldAccept,
        [string]$ExpectedVersion = "",
        [int]$ExpectedVersionCode = 0
    )

    Write-FixtureBytes -VersionBytes $VersionBytes -MapBytes $MapBytes
    $accepted = $false
    $identity = $null
    $failure = $null
    try {
        $identity = Get-HordeSourceIdentity -RepoRoot $fixtureRoot
        $accepted = $true
    } catch {
        $failure = $_.Exception
    }
    if ($ShouldAccept) {
        if (-not $accepted) {
            throw "$Name was rejected unexpectedly: $($failure.Message)"
        }
        if ($identity.Version -ne $ExpectedVersion -or $identity.VersionCode -ne $ExpectedVersionCode) {
            throw "$Name returned $($identity.Version)/$($identity.VersionCode), expected $ExpectedVersion/$ExpectedVersionCode."
        }
    } elseif ($accepted) {
        throw "$Name was accepted unexpectedly as $($identity.Version)/$($identity.VersionCode)."
    }
}

try {
    $validMap = $utf8.GetBytes('{ "androidVersionCodes": { "1.6.1": 9 } }')
    Assert-Identity -Name "LF authority" -VersionBytes $utf8.GetBytes("1.6.1`n") -MapBytes $validMap `
        -ShouldAccept $true -ExpectedVersion "1.6.1" -ExpectedVersionCode 9
    Assert-Identity -Name "CRLF authority" -VersionBytes $utf8.GetBytes("1.6.1`r`n") -MapBytes $validMap `
        -ShouldAccept $true -ExpectedVersion "1.6.1" -ExpectedVersionCode 9

    $bom = [byte[]](0xef, 0xbb, 0xbf)
    Assert-Identity -Name "VERSION BOM" -VersionBytes ($bom + $utf8.GetBytes("1.6.1`n")) -MapBytes $validMap -ShouldAccept $false
    Assert-Identity -Name "map BOM" -VersionBytes $utf8.GetBytes("1.6.1`n") -MapBytes ($bom + $validMap) -ShouldAccept $false
    Assert-Identity -Name "malformed VERSION UTF-8" -VersionBytes ([byte[]](0xff, 0x0a)) -MapBytes $validMap -ShouldAccept $false
    Assert-Identity -Name "malformed map UTF-8" -VersionBytes $utf8.GetBytes("1.6.1`n") `
        -MapBytes ([byte[]](0xff, 0x0a)) -ShouldAccept $false
    Assert-Identity -Name "leading-zero VERSION" -VersionBytes $utf8.GetBytes("01.6.1`n") -MapBytes $validMap -ShouldAccept $false

    foreach ($invalidMap in @(
        '{ "androidVersionCodes": { "1.6.1": 9.0 } }',
        '{ "androidVersionCodes": { "1.6.1": 9e0 } }',
        '{ "androidVersionCodes": { "1.6.1": "9" } }',
        '{ "androidVersionCodes": { "1.6.1": 2147483648 } }',
        '{ "androidVersionCodes": { "1.6.1": 9, "1.6.1": 10 } }')) {
        Assert-Identity -Name "invalid Android code map" -VersionBytes $utf8.GetBytes("1.6.1`n") `
            -MapBytes $utf8.GetBytes($invalidMap) -ShouldAccept $false
    }
} finally {
    if (Test-Path -LiteralPath $fixtureRoot) {
        Remove-Item -LiteralPath $fixtureRoot -Recurse -Force
    }
}

Write-Output "PowerShell version-contract tests passed."
