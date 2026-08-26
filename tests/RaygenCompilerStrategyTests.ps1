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
    if ($stats.functionCalls -ne 0 -or $stats.functions -ne 1) {
        throw "Raygen compiler retained helper calls that are unsafe around driver ray-query traversal."
    }
    if ($stats.rayQueryInitializations -gt 29) {
        throw "Raygen compiler cloned $($stats.rayQueryInitializations) static ray-query sites; expected at most the audited 29-site fully inlined shape."
    }
    if (-not $stats.driverSafeFullyInlined) {
        throw "Raygen compiler did not report the driver-safe fully inlined strategy."
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
    Write-Output "Raygen compiler retained the audited driver-safe inlined traversal ($($stats.bytes) bytes, $($stats.instructions) instructions, $($stats.rayQueryInitializations) ray-query sites)."
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}
