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

. (Join-Path $PSScriptRoot "..\tools\release-version-policy.ps1")

function Assert-PolicyFailure {
    param([scriptblock]$Action, [string]$Expected)

    $failureMessage = $null
    try { & $Action | Out-Null } catch { $failureMessage = $_.Exception.Message }
    if ($null -eq $failureMessage -or $failureMessage -notmatch [regex]::Escape($Expected)) {
        throw "Expected release-policy rejection '$Expected', received '$failureMessage'."
    }
}

foreach ($publishedVersion in @('1.6.0', '1.6.1', '1.6.1-alpha.1', '1.6.1+rebuild', '1.6.1.preview')) {
    if (-not (Test-HordeReleaseVersionIsPublished -Version $publishedVersion)) {
        throw "Published line was not classified as immutable: $publishedVersion"
    }
    Assert-PolicyFailure { Assert-HordeReleaseVersionIsMutable -Version $publishedVersion -VersionCode 10 } 'immutable'
}
if (Test-HordeReleaseVersionIsPublished -Version '1.6.2') { throw 'An unpublished version was classified as published.' }
Assert-PolicyFailure { Assert-HordeReleaseVersionIsMutable -Version '1.6.2' -VersionCode 9 } 'greater than'
$activeIdentity = Get-HordeSourceIdentity -RepoRoot (Join-Path $PSScriptRoot '..')
$unmatchedVersion = if ($activeIdentity.Version -eq '1.6.2') { '1.6.3' } else { '1.6.2' }
Assert-PolicyFailure { Assert-HordeReleaseVersionIsMutable -Version $unmatchedVersion -VersionCode 10 } 'active root contract'
# A current unpublished identity is a policy query, without packaging or signing.
if (-not (Test-HordeReleaseVersionIsPublished -Version $activeIdentity.Version)) {
    Assert-HordeReleaseVersionIsMutable -Version $activeIdentity.Version -VersionCode $activeIdentity.VersionCode
}
# These real entry points must reject before build, key access/prompt, Butler
# lookup, or output cleanup. Deliberately missing paths must never be reached.
Assert-PolicyFailure { & (Join-Path $PSScriptRoot '..\tools\package-alpha.ps1') -Version '1.6.1' -VersionCode 9 } 'immutable'
Assert-PolicyFailure { & (Join-Path $PSScriptRoot '..\tools\package-signed-alpha.ps1') -Version '1.6.1' -VersionCode 9 -KeyStorePath (Join-Path $fixtureRoot 'missing.jks') } 'immutable'
Assert-PolicyFailure { & (Join-Path $PSScriptRoot '..\tools\push-alpha-to-itch.ps1') -Version '1.6.1' -VersionCode 9 -ButlerPath (Join-Path $fixtureRoot 'missing-butler.exe') } 'immutable'

Write-Output "PowerShell version-contract and published-release policy tests passed."
