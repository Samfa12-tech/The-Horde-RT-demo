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
        $versionRaw = [IO.File]::ReadAllText($versionPath, $utf8)
    } catch {
        throw "VERSION must be valid UTF-8 without a byte-order mark: $versionPath"
    }
    if ($versionRaw -notmatch '\A([0-9]+\.[0-9]+\.[0-9]+)(?:\r?\n)?\z' -or
        $versionRaw -match '[ \t]') {
        throw "VERSION must contain exactly one MAJOR.MINOR.PATCH line without whitespace."
    }
    $version = $matches[1]

    try {
        $versionCodeMap = Get-Content -LiteralPath $versionCodeMapPath -Raw -Encoding utf8 | ConvertFrom-Json
    } catch {
        throw "Android version-code map is not valid JSON: $versionCodeMapPath"
    }
    if ($null -eq $versionCodeMap.androidVersionCodes) {
        throw "Android version-code map has no androidVersionCodes object."
    }
    $entry = $versionCodeMap.androidVersionCodes.PSObject.Properties[$version]
    $versionCodeText = if ($null -eq $entry) { "" } else { [string]$entry.Value }
    if ($null -eq $entry -or $entry.Value -isnot [ValueType] -or $versionCodeText -notmatch '^[1-9][0-9]*$') {
        throw "Android version-code map has no positive integer for $version."
    }
    $versionCode = [int]$versionCodeText

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
