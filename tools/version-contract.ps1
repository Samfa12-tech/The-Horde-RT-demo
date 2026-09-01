Set-StrictMode -Version Latest

function Get-HordeSourceIdentity {
    param(
        [string]$RepoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
    )

    $versionPath = Join-Path $RepoRoot "VERSION"
    $versionCodeMapPath = Join-Path $RepoRoot "version-code-map.json"
    if (-not (Test-Path -LiteralPath $versionPath -PathType Leaf)) {
        throw "Horde RT semantic version source is missing: $versionPath"
    }
    if (-not (Test-Path -LiteralPath $versionCodeMapPath -PathType Leaf)) {
        throw "Horde RT Android version-code map is missing: $versionCodeMapPath"
    }

    $utf8 = [Text.UTF8Encoding]::new($false, $true)
    try {
        $versionBytes = [IO.File]::ReadAllBytes($versionPath)
        if ($versionBytes.Length -ge 3 -and $versionBytes[0] -eq 0xEF -and
            $versionBytes[1] -eq 0xBB -and $versionBytes[2] -eq 0xBF) {
            throw "VERSION has a byte-order mark."
        }
        $versionRaw = $utf8.GetString($versionBytes)
    } catch {
        throw "VERSION must be valid UTF-8 without a byte-order mark: $versionPath"
    }
    if ($versionRaw -notmatch '\A((?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*))(?:\r?\n)?\z' -or
        $versionRaw -match '[ \t]') {
        throw "VERSION must contain exactly one MAJOR.MINOR.PATCH line without whitespace."
    }
    $version = $matches[1]

    try {
        $versionCodeMapBytes = [IO.File]::ReadAllBytes($versionCodeMapPath)
        if ($versionCodeMapBytes.Length -ge 3 -and $versionCodeMapBytes[0] -eq 0xEF -and
            $versionCodeMapBytes[1] -eq 0xBB -and $versionCodeMapBytes[2] -eq 0xBF) {
            throw "Android version-code map has a byte-order mark."
        }
        $versionCodeMapRaw = $utf8.GetString($versionCodeMapBytes)
        $versionCodeMap = $versionCodeMapRaw | ConvertFrom-Json
    } catch {
        throw "Android version-code map must be valid UTF-8 JSON without a byte-order mark: $versionCodeMapPath"
    }
    if ($versionCodeMap -isnot [PSCustomObject]) {
        throw "Android version-code map must be a JSON object."
    }
    if ($null -eq $versionCodeMap.androidVersionCodes) {
        throw "Android version-code map has no androidVersionCodes object."
    }
    if ($versionCodeMap.androidVersionCodes -isnot [PSCustomObject]) {
        throw "Android version-code map androidVersionCodes must be an object."
    }
    $activeAssignmentPattern = '"' + [regex]::Escape($version) + '"\s*:'
    $activeAssignmentCount = [regex]::Matches($versionCodeMapRaw, $activeAssignmentPattern).Count
    if ($activeAssignmentCount -ne 1) {
        throw "Android version-code map must contain exactly one active assignment for $version."
    }
    $entry = $versionCodeMap.androidVersionCodes.PSObject.Properties[$version]
    if ($null -eq $entry -or $entry.Value -isnot [long] -or
        $entry.Value -lt 1 -or $entry.Value -gt [int]::MaxValue) {
        throw "Android version-code map has no positive integer for $version."
    }
    $versionCode = [int]$entry.Value

    return [PSCustomObject]@{
        Version = $version
        VersionCode = $versionCode
        VersionPath = $versionPath
        VersionCodeMapPath = $versionCodeMapPath
    }
}

function Assert-HordeSourceIdentityMatches {
    param(
        [Parameter(Mandatory = $true)][string]$Version,
        [Parameter(Mandatory = $true)][int]$VersionCode,
        [string]$RepoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
    )

    $identity = Get-HordeSourceIdentity -RepoRoot $RepoRoot
    if ($Version -ne $identity.Version) {
        throw "Version must match the active root contract $($identity.Version); received $Version."
    }
    if ($VersionCode -ne $identity.VersionCode) {
        throw "VersionCode must match the active root contract $($identity.VersionCode) for $($identity.Version); received $VersionCode."
    }
    return $identity
}
