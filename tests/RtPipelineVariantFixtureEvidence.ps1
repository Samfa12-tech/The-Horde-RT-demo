[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Fixture,
    [Parameter(Mandatory = $true)][string]$ProviderObject,
    [Parameter(Mandatory = $true)][string]$OpaqueSha256,
    [Parameter(Mandatory = $true)][string]$GenericSha256,
    [Parameter(Mandatory = $true)][string]$RequiredKey)

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
    $path = Join-Path $repoRoot ([string]::Join([IO.Path]::DirectorySeparatorChar, @('src','vulkan','raytracing','variants', "$key.inc")))
    $words = [regex]::Matches((Get-Content -LiteralPath $path -Raw), '0x([0-9a-fA-F]{8})u') | ForEach-Object { [Convert]::ToUInt32($_.Groups[1].Value, 16) }
    $bytes = New-Object byte[] ($words.Count * 4)
    for ($index = 0; $index -lt $words.Count; ++$index) { [Array]::Copy([BitConverter]::GetBytes([uint32]$words[$index]), 0, $bytes, $index * 4, 4) }
    return $bytes
}
function Test-ByteEqual([byte[]]$left, [byte[]]$right) {
    if ($left.Length -ne $right.Length) { return $false }
    for ($index = 0; $index -lt $left.Length; ++$index) { if ($left[$index] -ne $right[$index]) { return $false } }
    return $true
}
function Assert-ProviderContainment([byte[]]$providerBytes, [byte[]]$selectedOpaque, [byte[]]$selectedGeneric,
    [string]$requiredKey) {
    $allKeys = @('shipping_mobile_opaque_fast','shipping_mobile_generic_dielectric','shipping_high_opaque_fast','shipping_high_generic_dielectric','diagnostic_mobile_opaque_fast','diagnostic_mobile_generic_dielectric','diagnostic_high_opaque_fast','diagnostic_high_generic_dielectric')
    $selectedKeys = @($requiredKey, ($requiredKey -replace 'opaque_fast$', 'generic_dielectric'))
    $opaqueOccurrences = Get-ByteSequenceCount $providerBytes $selectedOpaque
    $genericOccurrences = Get-ByteSequenceCount $providerBytes $selectedGeneric
    Assert-True ($opaqueOccurrences -eq 1 -and $genericOccurrences -eq 1) 'Provider object did not retain exactly one copy of each selected SPIR-V stream.'
    foreach ($key in $selectedKeys) { Assert-True (Test-ContainsAscii $providerBytes $key) "Selected semantic metadata is missing: $key" }
    $forbiddenStreams = @()
    foreach ($key in $allKeys | Where-Object { $_ -notin $selectedKeys }) {
        Assert-True (-not (Test-ContainsAscii $providerBytes $key)) "Non-selected semantic metadata leaked: $key"
        $stream = Get-IncludeBytes $key
        if (-not (Test-ByteEqual $stream $selectedOpaque) -and -not (Test-ByteEqual $stream $selectedGeneric) -and -not (@($forbiddenStreams | Where-Object { Test-ByteEqual $_ $stream }).Count -gt 0)) { $forbiddenStreams += ,$stream }
    }
    foreach ($stream in $forbiddenStreams) { Assert-True (-not (Test-ContainsBytes $providerBytes $stream)) 'Non-selected distinct word stream leaked into provider object.' }
    return $forbiddenStreams
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
    $forbiddenStreams = @(Assert-ProviderContainment $providerBytes $selectedOpaque $selectedGeneric $RequiredKey)
    Assert-True ($forbiddenStreams.Count -gt 0) 'Containment fixture must have distinguishable forbidden streams.'
    # Scanner controls use the exact same predicate.  Metadata and word-stream
    # violations are independent so either detection branch has adversarial proof.
    $forbiddenMetadata = @('shipping_mobile_opaque_fast','shipping_mobile_generic_dielectric','shipping_high_opaque_fast','shipping_high_generic_dielectric','diagnostic_mobile_opaque_fast','diagnostic_mobile_generic_dielectric','diagnostic_high_opaque_fast','diagnostic_high_generic_dielectric' | Where-Object { $_ -notin @($RequiredKey, ($RequiredKey -replace 'opaque_fast$', 'generic_dielectric')) })
    $metadataBytes = [Text.Encoding]::ASCII.GetBytes(($forbiddenMetadata -join '|'))
    $metadataContaminated = New-Object byte[] ($providerBytes.Length + $metadataBytes.Length)
    [Array]::Copy($providerBytes, $metadataContaminated, $providerBytes.Length)
    [Array]::Copy($metadataBytes, 0, $metadataContaminated, $providerBytes.Length, $metadataBytes.Length)
    $rejectedMetadataContamination = $false
    try { Assert-ProviderContainment $metadataContaminated $selectedOpaque $selectedGeneric $RequiredKey | Out-Null } catch { $rejectedMetadataContamination = $true }
    Assert-True $rejectedMetadataContamination 'Forbidden-semantic-metadata scanner control did not reject contamination.'
    # Scanner control: contaminate an inspected-object copy with every distinguishable forbidden stream.
    $forbiddenBytes = @($forbiddenStreams | ForEach-Object { $_ }).Count
    $contaminated = New-Object byte[] ($providerBytes.Length + $forbiddenBytes)
    [Array]::Copy($providerBytes, $contaminated, $providerBytes.Length)
    $cursor = $providerBytes.Length
    foreach ($stream in $forbiddenStreams) { [Array]::Copy($stream, 0, $contaminated, $cursor, $stream.Length); $cursor += $stream.Length }
    $rejectedContamination = $false
    try { Assert-ProviderContainment $contaminated $selectedOpaque $selectedGeneric $RequiredKey | Out-Null } catch { $rejectedContamination = $true }
    Assert-True $rejectedContamination 'Forbidden-stream scanner control did not reject contamination.'
}
finally { if (Test-Path -LiteralPath $temporaryRoot) { Remove-Item -LiteralPath $temporaryRoot -Recurse -Force } }
