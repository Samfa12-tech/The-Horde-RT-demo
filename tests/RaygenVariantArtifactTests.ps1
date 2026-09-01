[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$compiler = Join-Path $repoRoot 'tools\compile-raygen.ps1'
$manifestPath = Join-Path $repoRoot 'tools\raygen-variants.json'
$budgetPath = Join-Path $repoRoot 'tools\raygen-variant-budgets.json'
$abiDefinitionPath = Join-Path $repoRoot 'src\vulkan\raytracing\RtSceneAbi.def'
$generatedAbiPath = Join-Path $repoRoot 'shaders\raytracing\include\rt_scene_abi.generated.glsl'
$diagnosticsPath = Join-Path $repoRoot 'shaders\raytracing\include\rt_diagnostics.glsl'
$variantConfigPath = Join-Path $repoRoot 'shaders\raytracing\include\rt_variant_config.glsl'
$transportPath = Join-Path $repoRoot 'shaders\raytracing\include\rt_dielectric_transport.glsl'
$lightingPath = Join-Path $repoRoot 'shaders\raytracing\include\rt_lighting.glsl'
$temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) ('horde-raygen-artifacts-' + [guid]::NewGuid().ToString('N'))
$worktreeStatusBefore = (& git -C $repoRoot status --porcelain) -join "`n"

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Get-CanonicalFileHash {
    param([string]$Path)

    $sha256 = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString($sha256.ComputeHash([IO.File]::ReadAllBytes($Path)))).Replace('-', '').ToLowerInvariant()
    }
    finally {
        $sha256.Dispose()
    }
}

try {
    $abi = Get-Content -LiteralPath $abiDefinitionPath -Raw | ConvertFrom-Json
    Assert-True ($abi.schema -eq 1 -and $abi.bindings.dielectricDiagnostics -eq 22) `
        'RT scene ABI must retain schema 1 and diagnostics binding 22.'
    $diagnosticRecord = @($abi.records | Where-Object name -eq 'RtDielectricDiagnostics')
    Assert-True ($diagnosticRecord.Count -eq 1 -and $diagnosticRecord[0].size -eq 176 -and
        @($diagnosticRecord[0].fields).Count -eq 41) `
        'RtDielectricDiagnostics must remain the exact 176-byte / 41-field ABI record.'

    $generatedAbi = Get-Content -LiteralPath $generatedAbiPath -Raw
    Assert-True ($generatedAbi -match '#if !defined\(HORDE_RT_VARIANT_INSTRUMENTATION\) \|\| HORDE_RT_VARIANT_INSTRUMENTATION == 1' -and
        $generatedAbi -match 'binding = 22\) restrict buffer RtDielectricDiagnosticsBuffer' -and
        $generatedAbi -match '\} rtDielectricDiagnostics;\s*#endif') `
        'Generated GLSL must conditionally retain the canonical diagnostics block for compatibility/Diagnostic shaders only.'

    $diagnostics = Get-Content -LiteralPath $diagnosticsPath -Raw
    Assert-True (([regex]::Matches($diagnostics, '\batomic(?:Add|Or)\s*\(').Count -eq 2) -and
        $diagnostics -match '#if !defined\(HORDE_RT_VARIANT_INSTRUMENTATION\)' -and
        $diagnostics -match '(?m)^#define\s+RT_DIAG_ADD\(fieldName, deltaValue\)\s*$' -and
        $diagnostics -match '(?m)^#define\s+RT_DIAG_OR\(fieldName, maskValue\)\s*$') `
        'Diagnostic helpers must retain exactly two Diagnostic atomics and expression-free Shipping no-ops.'

    $variantConfig = Get-Content -LiteralPath $variantConfigPath -Raw
    foreach ($expectedConstant in @(
        'kRtVariantDielectricInterfaceBudget = 4;', 'kRtVariantDielectricVolumeBudget = 2;',
        'kRtVariantShadowInterfaceBudget = 4;', 'kRtVariantShadowVolumeBudget = 2;',
        'kRtVariantDielectricInterfaceBudget = 8;', 'kRtVariantDielectricVolumeBudget = 4;',
        'kRtVariantShadowInterfaceBudget = 8;', 'kRtVariantShadowVolumeBudget = 4;')) {
        Assert-True ($variantConfig.Contains($expectedConstant)) "Missing compile-time matrix budget: $expectedConstant"
    }
    $transport = Get-Content -LiteralPath $transportPath -Raw
    $lighting = Get-Content -LiteralPath $lightingPath -Raw
    Assert-True ($transport.Contains('HORDE_RT_DIELECTRIC_INTERFACE_CEILING') -and
        $transport.Contains('HORDE_RT_DIELECTRIC_VOLUME_CAPACITY') -and
        $lighting.Contains('HORDE_RT_SHADOW_INTERFACE_CEILING') -and
        $lighting.Contains('HORDE_RT_SHADOW_VOLUME_CAPACITY') -and
        $transport.Contains('controls.waterQuality >= 1.5') -and
        $lighting.Contains('controls.waterQuality >= 1.5')) `
        'Matrix budgets must specialise all bounded dielectric/shadow routes while macro-absent compatibility retains runtime WaterQuality selection.'

    $budgets = Get-Content -LiteralPath $budgetPath -Raw | ConvertFrom-Json
    Assert-True ($budgets.schema -eq 1 -and $budgets.status -eq 'unfrozen' -and
        ($null -eq $budgets.budgets -or @($budgets.budgets).Count -eq 0)) `
        'Task 3c must not freeze temporary matrix budgets.'

    $genericInclude = Join-Path $repoRoot 'src\vulkan\raytracing\MinimalRayGenShader.inc'
    $legacyInclude = Join-Path $repoRoot 'src\vulkan\raytracing\MinimalLegacyRayGenShader.inc'
    Assert-True ((Get-CanonicalFileHash $genericInclude) -eq 'fd534d390fc5d73aa65fb291fc847dde6d94a70da4d611eb274c4739d65c7087') `
        'Compatibility generic include changed unexpectedly.'
    Assert-True ((Get-CanonicalFileHash $legacyInclude) -eq 'b8ec454582c7b7e0a4f0734286b3475596b817b785b857ddc2c563e40a70bb82') `
        'Compatibility legacy include changed unexpectedly.'

    New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
    $matrixOutput = Join-Path $temporaryRoot 'matrix'
    & $compiler -Matrix -OutputDirectory $matrixOutput
    if ($LASTEXITCODE -ne 0) { throw "Matrix compiler failed with exit code $LASTEXITCODE." }

    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $expectedKeys = @(
        'shipping_mobile_opaque_fast', 'shipping_mobile_generic_dielectric',
        'shipping_high_opaque_fast', 'shipping_high_generic_dielectric',
        'diagnostic_mobile_opaque_fast', 'diagnostic_mobile_generic_dielectric',
        'diagnostic_high_opaque_fast', 'diagnostic_high_generic_dielectric')
    Assert-True ((Compare-Object ($expectedKeys | Sort-Object) (@($manifest.variants | ForEach-Object name | Sort-Object))).Count -eq 0) `
        'Matrix manifest no longer contains the exact eight approved keys.'

    foreach ($variant in $manifest.variants) {
        $variantRoot = Join-Path $matrixOutput $variant.name
        $statsPath = Join-Path $variantRoot 'raygen-stats.json'
        $assemblyPath = Join-Path $variantRoot 'minimal.rgen.spvasm'
        $resolvedPath = Join-Path $variantRoot 'minimal.rgen.resolved'
        Assert-True ((Test-Path -LiteralPath $statsPath) -and (Test-Path -LiteralPath $assemblyPath) -and
            (Test-Path -LiteralPath $resolvedPath)) "Missing compiled artifact for $($variant.name)."
        $stats = Get-Content -LiteralPath $statsPath -Raw | ConvertFrom-Json
        $assembly = Get-Content -LiteralPath $assemblyPath -Raw
        Assert-True ($stats.schema -eq 1 -and $stats.key -eq $variant.name -and
            $stats.instrumentation -eq $variant.instrumentation -and $stats.quality -eq $variant.quality -and
            $stats.material -eq $variant.material -and $stats.strategy -eq $variant.strategy -and
            -not [string]::IsNullOrWhiteSpace($stats.dependencySha256) -and
            -not [string]::IsNullOrWhiteSpace($stats.compiledSpirvSha256)) "Artifact identity mismatch for $($variant.name)."
        if ($variant.strategy -eq 'GenericRetained') {
            Assert-True ($stats.boundedGenericFunctionsRetained -and $stats.functions -gt 1 -and
                $stats.functionCalls -gt 0 -and $stats.rayQueryInitializations -le 3) "Generic strategy shape changed for $($variant.name)."
        } else {
            Assert-True ($stats.driverSafeFullyInlined -and $stats.functions -eq 1 -and
                $stats.functionCalls -eq 0 -and $stats.rayQueryInitializations -le 29) "Legacy strategy shape changed for $($variant.name)."
        }
        $hasBinding22 = $assembly -match '\bOpDecorate\s+%\S+\s+Binding\s+22\b'
        if ($variant.instrumentation -eq 'Shipping') {
            Assert-True ($stats.atomicInstructions -eq 0 -and -not $stats.hasDiagnosticsBinding -and -not $hasBinding22 -and
                $assembly -notmatch '\bOpAtomic\w+\b') "Shipping artifact retained diagnostic SSBO work: $($variant.name)."
        } else {
            $expectedAtomics = if ($variant.material -eq 'GenericDielectric') { 32 } else { 5 }
            Assert-True ($stats.atomicInstructions -eq $expectedAtomics -and $stats.hasDiagnosticsBinding -and $hasBinding22 -and
                ([regex]::Matches($assembly, '\bOpAtomic\w+\b').Count -eq $expectedAtomics)) `
                "Diagnostic artifact lost its expected atomic/binding shape: $($variant.name)."
        }
        if ($variant.material -eq 'GenericDielectric') {
            $expectedBound = if ($variant.quality -eq 'Mobile') { 4 } else { 8 }
            Assert-True ($assembly -match ("\bOp(?:SLessThanEqual|SGreaterThanEqual)\s+%bool\s+%\S+\s+%int_{0}\b" -f $expectedBound)) `
                "Generic artifact did not compile the expected interface ceiling ${expectedBound}: $($variant.name)."
        }
    }

    $compatibilityGenericOutput = Join-Path $temporaryRoot 'compatibility-generic'
    & $compiler -Check -OutputDirectory $compatibilityGenericOutput
    if ($LASTEXITCODE -ne 0) { throw "Generic compatibility freshness failed with exit code $LASTEXITCODE." }
    Assert-True ((Get-CanonicalFileHash (Join-Path $compatibilityGenericOutput 'minimal.rgen.spv')) -eq
        'e9d4fca05e8c642b6e09251a7c57253124fa6475ab5f81e34d1acb548764c23a') `
        'Compatibility generic SPIR-V words changed.'
    $compatibilityLegacyOutput = Join-Path $temporaryRoot 'compatibility-legacy'
    & $compiler -Legacy -Check -OutputDirectory $compatibilityLegacyOutput
    if ($LASTEXITCODE -ne 0) { throw "Legacy compatibility freshness failed with exit code $LASTEXITCODE." }
    Assert-True ((Get-CanonicalFileHash (Join-Path $compatibilityLegacyOutput 'minimal.legacy.rgen.spv')) -eq
        '870e4ea0c0b24fdcac516fec15343c1531906600d8ec28c9eaebf826a7bd75a0') `
        'Compatibility legacy SPIR-V words changed.'
    Assert-True ((& git -C $repoRoot status --porcelain) -join "`n" -eq $worktreeStatusBefore) `
        'Temporary artifact compilation modified the worktree.'
    Write-Output 'Raygen variant artifact specialization and compatibility contracts passed.'
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) { Remove-Item -LiteralPath $temporaryRoot -Recurse -Force }
}
