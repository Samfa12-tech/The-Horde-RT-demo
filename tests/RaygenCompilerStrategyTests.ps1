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
    if ($stats.functionCalls -lt 1) {
        throw "Raygen compiler eagerly inlined every helper into the entry point."
    }
    if ($stats.rayQueryInitializations -gt 4) {
        throw "Raygen compiler cloned $($stats.rayQueryInitializations) static ray-query sites; expected at most 4 shared sites."
    }
    if (-not $stats.optimizerPreservedFunctions) {
        throw "Raygen compiler did not report the function-preserving optimization strategy."
    }
    Write-Output "Raygen compiler preserved shared bounded traversal functions ($($stats.bytes) bytes, $($stats.instructions) instructions, $($stats.rayQueryInitializations) ray-query sites)."
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}
