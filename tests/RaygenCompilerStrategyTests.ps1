[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) `
    ("horde-rt-raygen-strategy-" + [guid]::NewGuid().ToString("N"))

try {
    $lfRoot = Join-Path $temporaryRoot "lf-checkout"
    $crlfRoot = Join-Path $temporaryRoot "crlf-checkout"
    foreach ($checkoutRoot in @($lfRoot, $crlfRoot)) {
        New-Item -ItemType Directory -Force -Path `
            (Join-Path $checkoutRoot "tools"), `
            (Join-Path $checkoutRoot "shaders\raytracing"), `
            (Join-Path $checkoutRoot "src\vulkan\raytracing") | Out-Null
        Copy-Item -LiteralPath (Join-Path $repoRoot "tools\compile-raygen.ps1") `
            -Destination (Join-Path $checkoutRoot "tools\compile-raygen.ps1")
        Copy-Item -Path (Join-Path $repoRoot "shaders\raytracing\*") `
            -Destination (Join-Path $checkoutRoot "shaders\raytracing") -Recurse
        Copy-Item -LiteralPath `
            (Join-Path $repoRoot "src\vulkan\raytracing\MinimalRayGenShader.inc") `
            -Destination (Join-Path $checkoutRoot "src\vulkan\raytracing\MinimalRayGenShader.inc")
        Copy-Item -LiteralPath `
            (Join-Path $repoRoot "src\vulkan\raytracing\MinimalLegacyRayGenShader.inc") `
            -Destination (Join-Path $checkoutRoot "src\vulkan\raytracing\MinimalLegacyRayGenShader.inc")
    }

    foreach ($shaderPath in Get-ChildItem -LiteralPath `
        (Join-Path $lfRoot "shaders\raytracing") -File -Recurse |
        Where-Object Extension -in ".rgen", ".glsl") {
        $text = [IO.File]::ReadAllText($shaderPath.FullName).Replace("`r`n", "`n")
        [IO.File]::WriteAllText($shaderPath.FullName, $text, [Text.UTF8Encoding]::new($false))
    }
    foreach ($shaderPath in Get-ChildItem -LiteralPath `
        (Join-Path $crlfRoot "shaders\raytracing") -File -Recurse |
        Where-Object Extension -in ".rgen", ".glsl") {
        $text = [IO.File]::ReadAllText($shaderPath.FullName).Replace("`r`n", "`n").Replace("`n", "`r`n")
        [IO.File]::WriteAllText($shaderPath.FullName, $text, [Text.UTF8Encoding]::new($false))
    }

    & (Join-Path $lfRoot "tools\compile-raygen.ps1")
    Copy-Item -LiteralPath `
        (Join-Path $lfRoot "src\vulkan\raytracing\MinimalRayGenShader.inc") `
        -Destination (Join-Path $crlfRoot "src\vulkan\raytracing\MinimalRayGenShader.inc") -Force
    Copy-Item -LiteralPath `
        (Join-Path $lfRoot "src\vulkan\raytracing\MinimalLegacyRayGenShader.inc") `
        -Destination (Join-Path $crlfRoot "src\vulkan\raytracing\MinimalLegacyRayGenShader.inc") -Force
    & (Join-Path $crlfRoot "tools\compile-raygen.ps1") -Check `
        -OutputDirectory (Join-Path $temporaryRoot "crlf-output")

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
