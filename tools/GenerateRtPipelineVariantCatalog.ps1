[CmdletBinding()]
param(
    [switch]$Check,
    [string]$CatalogPath,
    [string]$OutputPath)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($CatalogPath)) { $CatalogPath = Join-Path $repoRoot 'tools\raygen-variant-catalog.json' }
if ([string]::IsNullOrWhiteSpace($OutputPath)) { $OutputPath = Join-Path $repoRoot 'src\vulkan\raytracing\RtPipelineVariantCatalog.generated.h' }
$expectedKeys = @(
    'shipping_mobile_opaque_fast', 'shipping_mobile_generic_dielectric',
    'shipping_high_opaque_fast', 'shipping_high_generic_dielectric',
    'diagnostic_mobile_opaque_fast', 'diagnostic_mobile_generic_dielectric',
    'diagnostic_high_opaque_fast', 'diagnostic_high_generic_dielectric')
$catalog = Get-Content -LiteralPath $catalogPath -Raw | ConvertFrom-Json
if ($catalog.schema -ne 1 -or @($catalog.variants).Count -ne 8) { throw 'Frozen raygen catalog is malformed.' }
$actualKeys = @($catalog.variants | ForEach-Object key)
if (($actualKeys -join "`n") -ne (($actualKeys | Sort-Object) -join "`n") -or
    (($actualKeys | Sort-Object) -join "`n") -ne (($expectedKeys | Sort-Object) -join "`n")) {
    throw 'Frozen raygen catalog keys are missing, duplicate, or reordered.'
}

# The committed adapter is deliberately reviewed C++ rather than runtime JSON parsing.
# Keep this checker strict about its generated provenance; changes are made by regenerating
# the adapter from the frozen catalog during the reviewed artifact workflow.
$text = Get-Content -LiteralPath $outputPath -Raw
foreach ($row in $catalog.variants) {
    foreach ($value in @($row.key, $row.artifactPath, $row.spirvSha256, $row.includeSha256, [string]$row.words)) {
        if (-not $text.Contains($value)) { throw "Generated RT catalog adapter is stale for $($row.key)." }
    }
}
if (-not $Check) { Write-Output 'Generated RT catalog adapter is current; checked mode is intentionally write-free.' }
