[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) `
    ("horde-rt-raygen-strategy-" + [guid]::NewGuid().ToString("N"))

try {
    & (Join-Path $repoRoot "tools\compile-raygen.ps1") `
        -Check -OutputDirectory $temporaryRoot
    $stats = Get-Content -LiteralPath `
        (Join-Path $temporaryRoot "raygen-stats.json") -Raw | ConvertFrom-Json
    if ($stats.variant -ne "generic" -or $stats.functions -le 1 -or
        $stats.functionCalls -le 0 -or $stats.rayQueryInitializations -gt 3 -or
        -not $stats.boundedGenericFunctionsRetained -or
        $stats.driverSafeFullyInlined) {
        throw "Generic raygen no longer matches the exact-phone-validated bounded function strategy."
    }
    $legacyStatsPath = Join-Path $temporaryRoot "legacy\raygen-stats.json"
    if (-not (Test-Path -LiteralPath $legacyStatsPath)) {
        throw "Raygen compiler did not check the separately embedded legacy variant."
    }
    $legacyStats = Get-Content -LiteralPath $legacyStatsPath -Raw | ConvertFrom-Json
    if ($legacyStats.variant -ne "legacy" -or -not $legacyStats.matchesEmbeddedWords -or
        -not $legacyStats.driverSafeFullyInlined -or $legacyStats.functionCalls -ne 0 -or
        $legacyStats.functions -ne 1 -or $legacyStats.rayQueryInitializations -gt 29) {
        throw "Legacy raygen is stale or no longer matches the audited driver-safe shape."
    }
    Write-Output "Raygen compiler retained the measured generic function strategy ($($stats.bytes) bytes, $($stats.instructions) instructions, $($stats.rayQueryInitializations) ray-query sites) and the audited fully inlined legacy path."
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}
