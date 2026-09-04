[CmdletBinding()]
param([switch]$Check, [switch]$Write, [string]$CatalogPath, [string]$OutputPath)

$ErrorActionPreference = 'Stop'
if ($Check -and $Write) { throw 'Use either -Check or -Write, not both.' }
$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($CatalogPath)) { $CatalogPath = Join-Path $repoRoot 'tools\raygen-variant-catalog.json' }
if ([string]::IsNullOrWhiteSpace($OutputPath)) { $OutputPath = Join-Path $repoRoot 'src\vulkan\raytracing\RtPipelineVariantCatalog.generated.h' }
function Assert-True([bool]$value, [string]$message) { if (-not $value) { throw $message } }
function Assert-Names([object]$value, [string[]]$expected, [string]$message) {
    $actual = @($value.PSObject.Properties.Name | Sort-Object)
    Assert-True (($actual -join "`n") -eq (($expected | Sort-Object) -join "`n")) $message
}
function Read-Utf8NoBom([string]$path, [string]$label) {
    $bytes = [IO.File]::ReadAllBytes($path)
    Assert-True (-not ($bytes.Length -ge 3 -and $bytes[0] -eq 0xef -and $bytes[1] -eq 0xbb -and $bytes[2] -eq 0xbf)) "$label must not contain a UTF-8 BOM."
    try { return [Text.UTF8Encoding]::new($false, $true).GetString($bytes) } catch { throw "$label must be valid UTF-8." }
}
function Get-Sha256Hex([byte[]]$bytes) {
    $sha256 = [Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($sha256.ComputeHash($bytes))).Replace('-', '').ToLowerInvariant() }
    finally { $sha256.Dispose() }
}
function Get-CanonicalTextHash([string]$path) {
    $lines = [IO.File]::ReadAllLines($path)
    $text = if ($lines.Count -eq 0) { '' } else { [string]::Join("`n", $lines) + "`n" }
    Get-Sha256Hex ([Text.Encoding]::UTF8.GetBytes($text))
}
function Get-IncludeWords([string]$path) {
    $text = Get-Content -LiteralPath $path -Raw
    $matches = [regex]::Matches($text, '0x([0-9a-fA-F]{8})u')
    Assert-True ($matches.Count -gt 0) "Frozen include contains no words: $path"
    $bytes = New-Object byte[] ($matches.Count * 4)
    for ($index = 0; $index -lt $matches.Count; ++$index) { [Array]::Copy([BitConverter]::GetBytes([Convert]::ToUInt32($matches[$index].Groups[1].Value, 16)), 0, $bytes, $index * 4, 4) }
    [pscustomobject]@{ Count = $matches.Count; Bytes = $bytes }
}

$expectedKeys = @('diagnostic_high_generic_dielectric', 'diagnostic_high_opaque_fast', 'diagnostic_mobile_generic_dielectric', 'diagnostic_mobile_opaque_fast', 'shipping_high_generic_dielectric', 'shipping_high_opaque_fast', 'shipping_mobile_generic_dielectric', 'shipping_mobile_opaque_fast')
$catalog = (Read-Utf8NoBom $CatalogPath 'Frozen raygen catalog') | ConvertFrom-Json
Assert-True ($catalog.schema -is [int] -or $catalog.schema -is [long]) 'Frozen raygen catalog schema must be an integer.'
Assert-True ($catalog.schema -eq 1 -and $catalog.status -eq 'frozen') 'Frozen raygen catalog schema/status is invalid.'
Assert-Names $catalog @('authorities', 'generator', 'schema', 'status', 'target', 'toolchain', 'variants') 'Frozen raygen catalog root schema is invalid.'
$rows = @($catalog.variants)
Assert-True ($rows.Count -eq 8) 'Frozen raygen catalog must contain exactly eight rows.'
Assert-True ((@($rows | ForEach-Object key) -join "`n") -eq ($expectedKeys -join "`n")) 'Frozen raygen catalog keys are missing, duplicate, or reordered.'
$rowFields = @('artifactPath', 'atomicInstructions', 'boundedGenericFunctionsRetained', 'branchOperations', 'bytes', 'compiler', 'dependencies', 'dependencySha256', 'driverSafeFullyInlined', 'functionCalls', 'functions', 'hasDiagnosticsBinding', 'includeSha256', 'instructions', 'instrumentation', 'key', 'loops', 'material', 'quality', 'rayQueryInitializations', 'selectionMerges', 'shippingAllowed', 'spirvSha256', 'strategy', 'words')
foreach ($row in $rows) {
    Assert-Names $row $rowFields "Frozen raygen catalog row schema is invalid: $($row.key)"
    foreach ($name in @('key','instrumentation','quality','material','artifactPath','spirvSha256','includeSha256')) { Assert-True ($row.$name -is [string] -and -not [string]::IsNullOrWhiteSpace($row.$name)) "Frozen catalog row has invalid ${name}: $($row.key)" }
    Assert-True ($row.words -is [int] -or $row.words -is [long]) "Frozen catalog words must be an integer: $($row.key)"
    Assert-True ($row.key -cmatch '^[a-z_]+$' -and $row.artifactPath -ceq "src/vulkan/raytracing/variants/$($row.key).inc") "Frozen catalog artifact path/key mapping is invalid: $($row.key)"
    Assert-True ($row.spirvSha256 -cmatch '^[0-9a-f]{64}$' -and $row.includeSha256 -cmatch '^[0-9a-f]{64}$') "Frozen catalog hash lexeme is invalid: $($row.key)"
    Assert-True (@('Shipping','Diagnostic') -ccontains $row.instrumentation -and @('Mobile','High') -ccontains $row.quality -and @('OpaqueFast','GenericDielectric') -ccontains $row.material) "Frozen catalog enum is invalid: $($row.key)"
    $includePath = Join-Path $repoRoot ([string]::Join([IO.Path]::DirectorySeparatorChar, [string[]]$row.artifactPath.Split('/')))
    Assert-True (Test-Path -LiteralPath $includePath -PathType Leaf) "Frozen catalog include is missing: $($row.key)"
    $words = Get-IncludeWords $includePath
    Assert-True ($words.Count -eq $row.words -and (Get-Sha256Hex $words.Bytes) -ceq $row.spirvSha256 -and (Get-CanonicalTextHash $includePath) -ceq $row.includeSha256) "Frozen catalog include is stale: $($row.key)"
}
function Render-Row([object]$row) {
    $hasDiagnosticsBinding = if ($row.hasDiagnosticsBinding) { 'true' } else { 'false' }
    "    RtPipelineCatalogRecord{{RtInstrumentation::$($row.instrumentation), DielectricQuality::$($row.quality), RtMaterialStrategy::$($row.material)}, `"$($row.key)`", `"$($row.artifactPath)`", `"$($row.spirvSha256)`", `"$($row.includeSha256)`", $($row.words), $($row.atomicInstructions), $hasDiagnosticsBinding}"
}
function Render-Pair([string]$instrumentation, [string]$quality, [int]$i, [int]$q) {
    $pair = @($rows | Where-Object { $_.instrumentation -eq $instrumentation -and $_.quality -eq $quality } | Sort-Object material)
    Assert-True ($pair.Count -eq 2 -and $pair[0].material -eq 'GenericDielectric' -and $pair[1].material -eq 'OpaqueFast') "Frozen pair is incomplete: $instrumentation/$quality"
    @("#if HORDE_RT_SELECTED_INSTRUMENTATION == $i && HORDE_RT_SELECTED_DIELECTRIC_QUALITY == $q", 'inline constexpr std::array<RtPipelineCatalogRecord, 2> kSelectedRtPipelineCatalog{', ((Render-Row $pair[1]) + ','), ((Render-Row $pair[0]) + '};'))
}

$lines = @('// Generated by tools/GenerateRtPipelineVariantCatalog.ps1. Do not edit.', '#pragma once', '', '#include "vulkan/raytracing/RtPipelineVariants.h"', '', '#include <array>', '#include <cstddef>', '#include <string_view>', '', '#if !defined(HORDE_RT_SELECTED_INSTRUMENTATION) || !defined(HORDE_RT_SELECTED_DIELECTRIC_QUALITY)', '#error "The RT raygen bundle policy must define both selected dimensions."', '#endif', '', 'namespace horde::vulkan::raytracing::detail {', '', 'struct RtPipelineCatalogRecord {', '    RtPipelineVariantKey key;', '    std::string_view canonicalKey;', '    std::string_view artifactPath;', '    std::string_view spirvSha256;', '    std::string_view includeSha256;', '    std::size_t words;', '    std::size_t atomicInstructions;', '    bool hasDiagnosticsBinding;', '};', '')
$pairs = @(@('Shipping','Mobile',0,0), @('Shipping','High',0,1), @('Diagnostic','Mobile',1,0), @('Diagnostic','High',1,1))
for ($index = 0; $index -lt $pairs.Count; ++$index) {
    $pairLines = @(Render-Pair $pairs[$index][0] $pairs[$index][1] $pairs[$index][2] $pairs[$index][3])
    if ($index -gt 0) { $pairLines[0] = $pairLines[0].Replace('#if', '#elif') }
    $lines += $pairLines
}
$lines += @('#else', '#error "Unsupported exact RT raygen bundle policy."', '#endif', '', '} // namespace horde::vulkan::raytracing::detail', '')
$rendered = $lines -join "`n"
$renderedBytes = [Text.UTF8Encoding]::new($false, $true).GetBytes($rendered)
if ($Write) { [IO.File]::WriteAllBytes($OutputPath, $renderedBytes); exit 0 }
if (-not (Test-Path -LiteralPath $OutputPath) -or -not [Linq.Enumerable]::SequenceEqual([byte[]][IO.File]::ReadAllBytes($OutputPath), [byte[]]$renderedBytes)) { throw 'Generated RT catalog adapter is stale, malformed, or hand-edited.' }
Write-Output 'Generated RT catalog adapter is current.'
