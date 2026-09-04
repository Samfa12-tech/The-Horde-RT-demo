[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Fixture,
    [Parameter(Mandatory = $true)][string]$ProviderObject,
    [Parameter(Mandatory = $true)][string]$OpaqueSha256,
    [Parameter(Mandatory = $true)][string]$GenericSha256,
    [Parameter(Mandatory = $true)][string]$RequiredKey,
    [Parameter(Mandatory = $true)][string]$ForbiddenKey,
    [Parameter(Mandatory = $true)][string]$ForbiddenGenericKey)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
function Assert-True([bool]$condition, [string]$message) { if (-not $condition) { throw $message } }
function Test-ContainsAscii([byte[]]$bytes, [string]$text) {
    $needle = [Text.Encoding]::ASCII.GetBytes($text)
    for ($offset = 0; $offset -le $bytes.Length - $needle.Length; ++$offset) {
        $matched = $true
        for ($index = 0; $index -lt $needle.Length; ++$index) { if ($bytes[$offset + $index] -ne $needle[$index]) { $matched = $false; break } }
        if ($matched) { return $true }
    }
    return $false
}
function Test-ContainsBytes([byte[]]$bytes, [byte[]]$needle) {
    for ($offset = 0; $offset -le $bytes.Length - $needle.Length; ++$offset) {
        $matched = $true
        for ($index = 0; $index -lt $needle.Length; ++$index) { if ($bytes[$offset + $index] -ne $needle[$index]) { $matched = $false; break } }
        if ($matched) { return $true }
    }
    return $false
}
function Get-ByteSequenceCount([byte[]]$bytes, [byte[]]$needle) {
    $count = 0
    for ($offset = 0; $offset -le $bytes.Length - $needle.Length; ++$offset) {
        $matched = $true
        for ($index = 0; $index -lt $needle.Length; ++$index) { if ($bytes[$offset + $index] -ne $needle[$index]) { $matched = $false; break } }
        if ($matched) { ++$count }
    }
    return $count
}
function Get-IncludeBytes([string]$key) {
    $path = Join-Path $repoRoot ("src\vulkan\raytracing\variants\$key.inc")
    $words = [regex]::Matches((Get-Content -LiteralPath $path -Raw), '0x([0-9a-fA-F]{8})u') | ForEach-Object { [Convert]::ToUInt32($_.Groups[1].Value, 16) }
    $bytes = New-Object byte[] ($words.Count * 4)
    for ($index = 0; $index -lt $words.Count; ++$index) { [Array]::Copy([BitConverter]::GetBytes([uint32]$words[$index]), 0, $bytes, $index * 4, 4) }
    return $bytes
}
function Assert-ProviderContainment([byte[]]$providerBytes, [byte[]]$selectedOpaque, [byte[]]$selectedGeneric,
    [byte[]]$forbiddenGeneric, [string]$requiredKey, [string]$forbiddenKey) {
    $opaqueOccurrences = Get-ByteSequenceCount $providerBytes $selectedOpaque
    $genericOccurrences = Get-ByteSequenceCount $providerBytes $selectedGeneric
    Assert-True ($opaqueOccurrences -eq 1 -and $genericOccurrences -eq 1) 'Provider object did not retain exactly one copy of each selected SPIR-V stream.'
    Assert-True (Test-ContainsAscii $providerBytes $requiredKey) 'Selected semantic metadata is missing from provider object.'
    Assert-True (-not (Test-ContainsAscii $providerBytes $forbiddenKey)) 'Non-selected semantic metadata leaked into provider object.'
    Assert-True (-not (Test-ContainsBytes $providerBytes $forbiddenGeneric)) 'Non-selected distinct generic word stream leaked into provider object.'
}

$temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) ('horde-rt-provider-fixture-' + [guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
    & $Fixture $temporaryRoot
    if ($LASTEXITCODE -ne 0) { throw 'Provider fixture did not reconstruct its selected word streams.' }
    Assert-True ((Get-FileHash (Join-Path $temporaryRoot 'opaque.spv') -Algorithm SHA256).Hash.ToLowerInvariant() -eq $OpaqueSha256) 'Opaque stream raw SHA-256 changed.'
    Assert-True ((Get-FileHash (Join-Path $temporaryRoot 'generic.spv') -Algorithm SHA256).Hash.ToLowerInvariant() -eq $GenericSha256) 'Generic stream raw SHA-256 changed.'
    $providerBytes = [IO.File]::ReadAllBytes($ProviderObject)
    $selectedOpaque = Get-IncludeBytes -key $RequiredKey
    $selectedGeneric = Get-IncludeBytes -key ($RequiredKey -replace 'opaque_fast$', 'generic_dielectric')
    $forbiddenGeneric = Get-IncludeBytes -key $ForbiddenGenericKey
    Assert-ProviderContainment $providerBytes $selectedOpaque $selectedGeneric $forbiddenGeneric $RequiredKey $ForbiddenKey
    # Scanner control: contaminate an inspected-object copy with a real forbidden stream.
    $contaminated = New-Object byte[] ($providerBytes.Length + $forbiddenGeneric.Length)
    [Array]::Copy($providerBytes, $contaminated, $providerBytes.Length)
    [Array]::Copy($forbiddenGeneric, 0, $contaminated, $providerBytes.Length, $forbiddenGeneric.Length)
    $rejectedContamination = $false
    try { Assert-ProviderContainment $contaminated $selectedOpaque $selectedGeneric $forbiddenGeneric $RequiredKey $ForbiddenKey } catch { $rejectedContamination = $true }
    Assert-True $rejectedContamination 'Forbidden-stream scanner control did not reject contamination.'
}
finally { if (Test-Path -LiteralPath $temporaryRoot) { Remove-Item -LiteralPath $temporaryRoot -Recurse -Force } }
