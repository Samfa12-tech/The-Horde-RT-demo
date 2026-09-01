[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$compiler = Join-Path $repoRoot 'tools\compile-raygen.ps1'
$manifestPath = Join-Path $repoRoot 'tools\raygen-variants.json'
$budgetsPath = Join-Path $repoRoot 'tools\raygen-variant-budgets.json'
$configPath = Join-Path $repoRoot 'shaders\raytracing\include\rt_variant_config.glsl'
$diagnosticsPath = Join-Path $repoRoot 'shaders\raytracing\include\rt_diagnostics.glsl'
$raygenPath = Join-Path $repoRoot 'shaders\raytracing\minimal.rgen'
$diagnosticConsumerPaths = @(
    $raygenPath
    Join-Path $repoRoot 'shaders\raytracing\include\rt_lighting.glsl'
    Join-Path $repoRoot 'shaders\raytracing\include\rt_dielectric_transport.glsl'
)
$temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) ('horde-raygen-variants-' + [guid]::NewGuid().ToString('N'))
$worktreeStatusBefore = (& git -C $repoRoot status --porcelain) -join "`n"

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Invoke-ExpectFailure {
    param([scriptblock]$Action, [string]$Message)
    $failed = $false
    try { & $Action } catch { $failed = $true }
    if (-not $failed) { throw $Message }
}

function Get-CanonicalFileHash {
    param([string]$Path)
    $sha256 = [Security.Cryptography.SHA256]::Create()
    try {
        ([BitConverter]::ToString($sha256.ComputeHash([IO.File]::ReadAllBytes($Path)))).Replace('-', '').ToLowerInvariant()
    }
    finally {
        $sha256.Dispose()
    }
}

function Invoke-InvalidVariantConfigCompile {
    param([string]$MacroName, [string]$Value)

    $vulkanSdk = if ([string]::IsNullOrWhiteSpace($env:VULKAN_SDK)) { 'C:\VulkanSDK\1.4.350.0' } else { $env:VULKAN_SDK }
    $validator = Join-Path $vulkanSdk 'Bin\glslangValidator.exe'
    Assert-True (Test-Path -LiteralPath $validator -PathType Leaf) "glslangValidator was not found at $validator"
    $invalidSource = Join-Path $temporaryRoot ("invalid-{0}.rgen" -f $MacroName.ToLowerInvariant())
    $invalidSpirv = "$invalidSource.spv"
    $macroDefinitions = @(
        '#version 460',
        '#extension GL_EXT_ray_tracing : require',
        '#define HORDE_RT_VARIANT_INSTRUMENTATION 0',
        '#define HORDE_RT_VARIANT_QUALITY 0',
        '#define HORDE_RT_VARIANT_MATERIAL 0',
        ("#undef {0}" -f $MacroName),
        ("#define {0} {1}" -f $MacroName, $Value),
        (Get-Content -LiteralPath $configPath -Raw).TrimEnd("`r", "`n"),
        'void main() {}'
    )
    [IO.File]::WriteAllText($invalidSource, ([string]::Join("`n", $macroDefinitions) + "`n"), [Text.UTF8Encoding]::new($false))
    $validatorOutput = @(& $validator -V --target-env vulkan1.2 -S rgen -o $invalidSpirv $invalidSource 2>&1)
    if ($LASTEXITCODE -eq 0) {
        throw "Invalid GLSL $MacroName=$Value compiled successfully: $($validatorOutput -join ' ')"
    }
}

try {
    foreach ($path in @($manifestPath, $budgetsPath, $configPath, $diagnosticsPath)) {
        Assert-True (Test-Path -LiteralPath $path -PathType Leaf) "Missing required variant foundation file: $path"
    }

    $diagnosticsSource = Get-Content -LiteralPath $diagnosticsPath -Raw
    Assert-True (([regex]::Matches($diagnosticsSource, '\batomic(?:Add|Or)\s*\(').Count -eq 2)) `
        'The diagnostics helper must own exactly the two raw atomic operations.'
    Assert-True ($diagnosticsSource -match '#define\s+RT_DIAG_ADD\s*\(\s*fieldName\s*,\s*deltaValue\s*\)\s+atomicAdd\s*\(\s*rtDielectricDiagnostics\.value\.fieldName\s*,\s*deltaValue\s*\)') `
        'RT_DIAG_ADD must route a field name and delta through the diagnostics SSBO.'
    Assert-True ($diagnosticsSource -match '#define\s+RT_DIAG_OR\s*\(\s*fieldName\s*,\s*maskValue\s*\)\s+atomicOr\s*\(\s*rtDielectricDiagnostics\.value\.fieldName\s*,\s*maskValue\s*\)') `
        'RT_DIAG_OR must route a field name and mask through the diagnostics SSBO.'
    Assert-True ($diagnosticsSource -notmatch '#define\s+RT_DIAG_(?:ADD|OR)\s*\([^\)]*\bvalue\b') `
        'Diagnostics macro parameter names must not collide with rtDielectricDiagnostics.value.'
    foreach ($consumerPath in $diagnosticConsumerPaths) {
        $consumerSource = Get-Content -LiteralPath $consumerPath -Raw
        Assert-True (([regex]::Matches($consumerSource, '\batomic(?:Add|Or)\s*\(').Count -eq 0)) `
            "Direct diagnostics atomics remain in $consumerPath."
    }
    $raygenSource = Get-Content -LiteralPath $raygenPath -Raw
    $abiInclude = '#include "include/rt_scene_abi.glsl"'
    $diagnosticsInclude = '#include "include/rt_diagnostics.glsl"'
    $lightingInclude = '#include "include/rt_lighting.glsl"'
    $transportInclude = '#include "include/rt_dielectric_transport.glsl"'
    Assert-True (($raygenSource.IndexOf($abiInclude) -ge 0) -and
        ($raygenSource.IndexOf($diagnosticsInclude) -gt $raygenSource.IndexOf($abiInclude)) -and
        ($raygenSource.IndexOf($lightingInclude) -gt $raygenSource.IndexOf($diagnosticsInclude)) -and
        ($raygenSource.IndexOf($transportInclude) -gt $raygenSource.IndexOf($diagnosticsInclude))) `
        'The diagnostics helper must follow the ABI declaration and precede lighting and transport.'

    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    Assert-True ($manifest.schema -eq 1) 'Variant manifest schema must be 1.'
    Assert-True ($manifest.variants.Count -eq 8) 'Variant manifest must contain exactly eight entries.'
    $expected = @(
        'shipping_mobile_opaque_fast', 'shipping_mobile_generic_dielectric',
        'shipping_high_opaque_fast', 'shipping_high_generic_dielectric',
        'diagnostic_mobile_opaque_fast', 'diagnostic_mobile_generic_dielectric',
        'diagnostic_high_opaque_fast', 'diagnostic_high_generic_dielectric')
    $actual = @($manifest.variants | ForEach-Object { $_.name })
    Assert-True ((@($actual | Sort-Object -Unique).Count -eq 8) -and
        ((Compare-Object $expected ($actual | Sort-Object)) -eq $null)) `
        'Variant manifest must be the exact eight-key Cartesian product.'
    foreach ($variant in $manifest.variants) {
        Assert-True ($variant.instrumentation -in @('Shipping', 'Diagnostic')) "Invalid instrumentation for $($variant.name)."
        Assert-True ($variant.quality -in @('Mobile', 'High')) "Invalid quality for $($variant.name)."
        Assert-True ($variant.material -in @('OpaqueFast', 'GenericDielectric')) "Invalid material for $($variant.name)."
        $expectedStrategy = if ($variant.material -eq 'GenericDielectric') { 'GenericRetained' } else { 'LegacyInlined' }
        Assert-True ($variant.strategy -eq $expectedStrategy) "Incorrect strategy mapping for $($variant.name)."
        Assert-True ($variant.PSObject.Properties.Name -contains 'shippingAllowed') "Missing shippingAllowed for $($variant.name)."
        Assert-True ($variant.shippingAllowed -is [bool]) "shippingAllowed must be boolean for $($variant.name)."
    }

    $budgets = Get-Content -LiteralPath $budgetsPath -Raw | ConvertFrom-Json
    Assert-True ($budgets.schema -eq 1 -and $budgets.status -eq 'unfrozen') `
        'Variant budget record must remain schema-1 and explicitly unfrozen.'
    Assert-True ($null -eq $budgets.budgets -or @($budgets.budgets).Count -eq 0) `
        'Unfrozen variant budget record must not invent numeric limits.'
    New-Item -ItemType Directory -Force -Path $temporaryRoot | Out-Null
    Invoke-InvalidVariantConfigCompile -MacroName 'HORDE_RT_VARIANT_INSTRUMENTATION' -Value '2'
    Invoke-InvalidVariantConfigCompile -MacroName 'HORDE_RT_VARIANT_QUALITY' -Value '-1'
    Invoke-InvalidVariantConfigCompile -MacroName 'HORDE_RT_VARIANT_MATERIAL' -Value '7'

    $genericHashBefore = Get-CanonicalFileHash (Join-Path $repoRoot 'src\vulkan\raytracing\MinimalRayGenShader.inc')
    $legacyHashBefore = Get-CanonicalFileHash (Join-Path $repoRoot 'src\vulkan\raytracing\MinimalLegacyRayGenShader.inc')
    Assert-True ($genericHashBefore -eq '2b39f13a9648ad6e14feb1b24309e2bb3d28b0d257954fe23103430438f139c5') 'Generic include hash changed before matrix compilation.'
    Assert-True ($legacyHashBefore -eq '68455959f0770b0dfd03550934193c0e657b808620ef732eae7f565988c06f27') 'Legacy include hash changed before matrix compilation.'

    $matrixOutputRoot = Join-Path $temporaryRoot 'matrix'
    $matrixCompilerOutput = @(& $compiler -Matrix -OutputDirectory $matrixOutputRoot)
    if ($LASTEXITCODE -ne 0) { throw "Matrix compiler failed with exit code $LASTEXITCODE." }
    $reportedOrder = @($matrixCompilerOutput | ForEach-Object {
        if ($_ -match '^Raygen variant ([a-z_]+):') { $Matches[1] }
    } | Where-Object { $_ })
    Assert-True ((@($reportedOrder) -join ',') -eq (@($expected | Sort-Object) -join ',')) `
        'Matrix compiler output order must be deterministic and sorted by stable key.'
    $dependencyHashes = @()
    foreach ($variant in $manifest.variants) {
        $variantRoot = Join-Path $matrixOutputRoot $variant.name
        foreach ($name in @('minimal.rgen.resolved', 'minimal.rgen.spv', 'minimal.rgen.spvasm', 'raygen-stats.json')) {
            Assert-True (Test-Path -LiteralPath (Join-Path $variantRoot $name) -PathType Leaf) "Missing matrix artifact $name for $($variant.name)."
        }
        $stats = Get-Content -LiteralPath (Join-Path $variantRoot 'raygen-stats.json') -Raw | ConvertFrom-Json
        Assert-True ($stats.key -eq $variant.name -and $stats.instrumentation -eq $variant.instrumentation -and
            $stats.quality -eq $variant.quality -and $stats.material -eq $variant.material -and
            $stats.strategy -eq $variant.strategy) "Stats identity mismatch for $($variant.name)."
        Assert-True (-not [string]::IsNullOrWhiteSpace($stats.dependencySha256) -and
            -not [string]::IsNullOrWhiteSpace($stats.manifestSha256) -and
            -not [string]::IsNullOrWhiteSpace($stats.compiledSpirvSha256)) "Stats hashes missing for $($variant.name)."
        $dependencyHashes += $stats.dependencySha256
        if ($variant.strategy -eq 'GenericRetained') {
            $expectedStats = @{ bytes = 224764; instructions = 13244; branchOperations = 683; loops = 10; selectionMerges = 285; functions = 59; functionCalls = 192; rayQueryInitializations = 3; atomicInstructions = 32 }
            $expectedSpirvSha256 = 'e9d4fca05e8c642b6e09251a7c57253124fa6475ab5f81e34d1acb548764c23a'
        } else {
            $expectedStats = @{ bytes = 493244; instructions = 27152; branchOperations = 3677; loops = 49; selectionMerges = 1506; functions = 1; functionCalls = 0; rayQueryInitializations = 23; atomicInstructions = 5 }
            $expectedSpirvSha256 = '870e4ea0c0b24fdcac516fec15343c1531906600d8ec28c9eaebf826a7bd75a0'
        }
        foreach ($property in $expectedStats.Keys) {
            Assert-True ($stats.PSObject.Properties.Name -contains $property) "Missing $property stat for $($variant.name)."
            Assert-True ([int64]$stats.$property -eq [int64]$expectedStats[$property]) "Unexpected $property for $($variant.name)."
        }
        Assert-True ($stats.compiledSpirvSha256 -eq $expectedSpirvSha256) "Compiled SPIR-V hash changed for $($variant.name)."
        Assert-True ([bool]$stats.hasDiagnosticsBinding) "Diagnostics binding 22 is missing for $($variant.name)."
        if ($variant.instrumentation -eq 'Shipping') {
            Assert-True ($stats.atomicInstructions -eq $expectedStats.atomicInstructions -and [bool]$stats.hasDiagnosticsBinding) `
                "Shipping-named baseline artifact changed diagnostics semantics for $($variant.name)."
        }
    }
    Assert-True ((@($dependencyHashes | Sort-Object -Unique).Count -eq 8)) `
        'Every key must have a unique dependency identity.'
    $singleOutputRoot = Join-Path $temporaryRoot 'single-variant'
    $singleCompilerOutput = @(& $compiler -Variant 'shipping_mobile_generic_dielectric' -OutputDirectory $singleOutputRoot)
    if ($LASTEXITCODE -ne 0) { throw "Single variant compiler failed with exit code $LASTEXITCODE." }
    $matrixStatsText = Get-Content -LiteralPath (Join-Path $matrixOutputRoot 'shipping_mobile_generic_dielectric\raygen-stats.json') -Raw
    $singleStatsText = Get-Content -LiteralPath (Join-Path $singleOutputRoot 'shipping_mobile_generic_dielectric\raygen-stats.json') -Raw
    Assert-True ($singleStatsText -eq $matrixStatsText) 'Single-key compilation did not reproduce deterministic stats output.'
    Assert-True ((Get-CanonicalFileHash (Join-Path $repoRoot 'src\vulkan\raytracing\MinimalRayGenShader.inc')) -eq $genericHashBefore) 'Matrix compilation modified the generic include.'
    Assert-True ((Get-CanonicalFileHash (Join-Path $repoRoot 'src\vulkan\raytracing\MinimalLegacyRayGenShader.inc')) -eq $legacyHashBefore) 'Matrix compilation modified the legacy include.'

    Invoke-ExpectFailure { & $compiler -Variant 'unknown_variant' -OutputDirectory (Join-Path $temporaryRoot 'unknown') } 'Unknown variant did not fail closed.'
    Invoke-ExpectFailure { & $compiler -Matrix -OutputDirectory $repoRoot } 'Unsafe repository output directory did not fail closed.'
    Invoke-ExpectFailure { & $compiler -Variant 'shipping_mobile_opaque_fast' -Strategy GenericRetained -OutputDirectory (Join-Path $temporaryRoot 'contradictory') } 'Contradictory strategy did not fail closed.'
    $duplicateManifestPath = Join-Path $temporaryRoot 'duplicate-variants.json'
    $duplicateManifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $duplicateManifest.variants[7].name = $duplicateManifest.variants[0].name
    [IO.File]::WriteAllText($duplicateManifestPath, ($duplicateManifest | ConvertTo-Json -Depth 4), [Text.UTF8Encoding]::new($false))
    Invoke-ExpectFailure {
        & $compiler -Matrix -ManifestPath $duplicateManifestPath -OutputDirectory (Join-Path $temporaryRoot 'duplicate-output')
    } 'Duplicate variant names did not fail closed.'
    $missingManifestPath = Join-Path $temporaryRoot 'missing-variants.json'
    $missingManifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $missingManifest.variants = @($missingManifest.variants | Select-Object -First 7)
    [IO.File]::WriteAllText($missingManifestPath, ($missingManifest | ConvertTo-Json -Depth 4), [Text.UTF8Encoding]::new($false))
    Invoke-ExpectFailure {
        & $compiler -Matrix -ManifestPath $missingManifestPath -OutputDirectory (Join-Path $temporaryRoot 'missing-output')
    } 'Missing variant combinations did not fail closed.'
    $schemaManifestPath = Join-Path $temporaryRoot 'bad-schema-variants.json'
    [IO.File]::WriteAllText($schemaManifestPath,
        ((Get-Content -LiteralPath $manifestPath -Raw).Replace('"schema": 1', '"schema": 2')),
        [Text.UTF8Encoding]::new($false))
    Invoke-ExpectFailure {
        & $compiler -Matrix -ManifestPath $schemaManifestPath -OutputDirectory (Join-Path $temporaryRoot 'schema-output')
    } 'Unsupported manifest schema did not fail closed.'
    $unknownEnumManifestPath = Join-Path $temporaryRoot 'unknown-enum-variants.json'
    $unknownEnumManifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $unknownEnumManifest.variants[0].quality = 'Ultra'
    [IO.File]::WriteAllText($unknownEnumManifestPath, ($unknownEnumManifest | ConvertTo-Json -Depth 4), [Text.UTF8Encoding]::new($false))
    Invoke-ExpectFailure {
        & $compiler -Matrix -ManifestPath $unknownEnumManifestPath -OutputDirectory (Join-Path $temporaryRoot 'unknown-enum-output')
    } 'Unknown variant enum did not fail closed.'
    $wrongCaseNameManifestPath = Join-Path $temporaryRoot 'wrong-case-name-variants.json'
    $wrongCaseNameManifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $wrongCaseNameManifest.variants[0].name = 'Shipping_Mobile_Opaque_Fast'
    [IO.File]::WriteAllText($wrongCaseNameManifestPath, ($wrongCaseNameManifest | ConvertTo-Json -Depth 4), [Text.UTF8Encoding]::new($false))
    Invoke-ExpectFailure {
        & $compiler -Matrix -ManifestPath $wrongCaseNameManifestPath -OutputDirectory (Join-Path $temporaryRoot 'wrong-case-name-output')
    } 'Wrong-case stable key did not fail closed.'
    $wrongCaseEnumManifestPath = Join-Path $temporaryRoot 'wrong-case-enum-variants.json'
    $wrongCaseEnumManifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $wrongCaseEnumManifest.variants[0].instrumentation = 'shipping'
    [IO.File]::WriteAllText($wrongCaseEnumManifestPath, ($wrongCaseEnumManifest | ConvertTo-Json -Depth 4), [Text.UTF8Encoding]::new($false))
    Invoke-ExpectFailure {
        & $compiler -Matrix -ManifestPath $wrongCaseEnumManifestPath -OutputDirectory (Join-Path $temporaryRoot 'wrong-case-enum-output')
    } 'Wrong-case manifest enum did not fail closed.'
    $wrongCaseStrategyManifestPath = Join-Path $temporaryRoot 'wrong-case-strategy-variants.json'
    $wrongCaseStrategyManifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $wrongCaseStrategyManifest.variants[1].strategy = 'genericretained'
    [IO.File]::WriteAllText($wrongCaseStrategyManifestPath, ($wrongCaseStrategyManifest | ConvertTo-Json -Depth 4), [Text.UTF8Encoding]::new($false))
    Invoke-ExpectFailure {
        & $compiler -Matrix -ManifestPath $wrongCaseStrategyManifestPath -OutputDirectory (Join-Path $temporaryRoot 'wrong-case-strategy-output')
    } 'Wrong-case manifest strategy did not fail closed.'
    $wrongShippingRelationshipManifestPath = Join-Path $temporaryRoot 'wrong-shipping-relationship-variants.json'
    $wrongShippingRelationshipManifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $wrongShippingRelationshipManifest.variants[4].shippingAllowed = $true
    [IO.File]::WriteAllText($wrongShippingRelationshipManifestPath, ($wrongShippingRelationshipManifest | ConvertTo-Json -Depth 4), [Text.UTF8Encoding]::new($false))
    Invoke-ExpectFailure {
        & $compiler -Matrix -ManifestPath $wrongShippingRelationshipManifestPath -OutputDirectory (Join-Path $temporaryRoot 'wrong-shipping-relationship-output')
    } 'Invalid Shipping relationship did not fail closed.'
    $wrongCasePropertyManifestPath = Join-Path $temporaryRoot 'wrong-case-property-variants.json'
    [IO.File]::WriteAllText($wrongCasePropertyManifestPath,
        ((Get-Content -LiteralPath $manifestPath -Raw).Replace('"name"', '"Name"')),
        [Text.UTF8Encoding]::new($false))
    Invoke-ExpectFailure {
        & $compiler -Matrix -ManifestPath $wrongCasePropertyManifestPath -OutputDirectory (Join-Path $temporaryRoot 'wrong-case-property-output')
    } 'Wrong-case manifest property did not fail closed.'
    Invoke-ExpectFailure {
        & $compiler -Variant 'SHIPPING_MOBILE_OPAQUE_FAST' -OutputDirectory (Join-Path $temporaryRoot 'wrong-case-key-output')
    } 'Wrong-case requested key did not fail closed.'
    Invoke-ExpectFailure {
        & $compiler -Variant 'shipping_mobile_opaque_fast' -Strategy 'legacyinlined' -OutputDirectory (Join-Path $temporaryRoot 'wrong-case-requested-strategy-output')
    } 'Wrong-case requested strategy did not fail closed.'
    $duplicateCombinationManifestPath = Join-Path $temporaryRoot 'duplicate-combination-variants.json'
    $duplicateCombinationManifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $duplicateCombinationManifest.variants[7].name = 'diagnostic_high_generic_dielectric_duplicate'
    $duplicateCombinationManifest.variants[7].instrumentation = $duplicateCombinationManifest.variants[0].instrumentation
    $duplicateCombinationManifest.variants[7].quality = $duplicateCombinationManifest.variants[0].quality
    $duplicateCombinationManifest.variants[7].material = $duplicateCombinationManifest.variants[0].material
    $duplicateCombinationManifest.variants[7].strategy = $duplicateCombinationManifest.variants[0].strategy
    $duplicateCombinationManifest.variants[7].shippingAllowed = $duplicateCombinationManifest.variants[0].shippingAllowed
    [IO.File]::WriteAllText($duplicateCombinationManifestPath, ($duplicateCombinationManifest | ConvertTo-Json -Depth 4), [Text.UTF8Encoding]::new($false))
    Invoke-ExpectFailure {
        & $compiler -Matrix -ManifestPath $duplicateCombinationManifestPath -OutputDirectory (Join-Path $temporaryRoot 'duplicate-combination-output')
    } 'Duplicate variant combinations with distinct names did not fail closed.'
    $unsupportedFieldManifestPath = Join-Path $temporaryRoot 'unsupported-field-variants.json'
    $unsupportedFieldManifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $unsupportedFieldManifest.variants[0] | Add-Member -NotePropertyName 'absolutePath' -NotePropertyValue 'C:/unsafe'
    [IO.File]::WriteAllText($unsupportedFieldManifestPath, ($unsupportedFieldManifest | ConvertTo-Json -Depth 4), [Text.UTF8Encoding]::new($false))
    Invoke-ExpectFailure {
        & $compiler -Matrix -ManifestPath $unsupportedFieldManifestPath -OutputDirectory (Join-Path $temporaryRoot 'unsupported-field-output')
    } 'Unsupported manifest fields did not fail closed.'
    $unsupportedTopLevelManifestPath = Join-Path $temporaryRoot 'unsupported-top-level-variants.json'
    $unsupportedTopLevelManifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $unsupportedTopLevelManifest | Add-Member -NotePropertyName 'outputPath' -NotePropertyValue 'C:/unsafe'
    [IO.File]::WriteAllText($unsupportedTopLevelManifestPath, ($unsupportedTopLevelManifest | ConvertTo-Json -Depth 4), [Text.UTF8Encoding]::new($false))
    Invoke-ExpectFailure {
        & $compiler -Matrix -ManifestPath $unsupportedTopLevelManifestPath -OutputDirectory (Join-Path $temporaryRoot 'unsupported-top-level-output')
    } 'Unsupported top-level manifest fields did not fail closed.'
    $malformedManifestPath = Join-Path $temporaryRoot 'malformed-variants.json'
    [IO.File]::WriteAllText($malformedManifestPath, '{ "schema": 1, "variants": [', [Text.UTF8Encoding]::new($false))
    Invoke-ExpectFailure {
        & $compiler -Matrix -ManifestPath $malformedManifestPath -OutputDirectory (Join-Path $temporaryRoot 'malformed-output')
    } 'Malformed variant manifest did not fail closed.'

    $lineEndingRoot = Join-Path $temporaryRoot 'line-endings'
    $lfRoot = Join-Path $lineEndingRoot 'lf-checkout'
    $crlfRoot = Join-Path $lineEndingRoot 'crlf-checkout'
    foreach ($checkoutRoot in @($lfRoot, $crlfRoot)) {
        New-Item -ItemType Directory -Force -Path (Join-Path $checkoutRoot 'tools'), (Join-Path $checkoutRoot 'shaders\raytracing') | Out-Null
        Copy-Item -LiteralPath $compiler -Destination (Join-Path $checkoutRoot 'tools\compile-raygen.ps1')
        Copy-Item -LiteralPath $manifestPath -Destination (Join-Path $checkoutRoot 'tools\raygen-variants.json')
        Copy-Item -Path (Join-Path $repoRoot 'shaders\raytracing\*') -Destination (Join-Path $checkoutRoot 'shaders\raytracing') -Recurse
    }
    foreach ($shaderPath in Get-ChildItem -LiteralPath (Join-Path $lfRoot 'shaders\raytracing') -File -Recurse) {
        $text = [IO.File]::ReadAllText($shaderPath.FullName).Replace("`r`n", "`n")
        [IO.File]::WriteAllText($shaderPath.FullName, $text, [Text.UTF8Encoding]::new($false))
    }
    foreach ($shaderPath in Get-ChildItem -LiteralPath (Join-Path $crlfRoot 'shaders\raytracing') -File -Recurse) {
        $text = [IO.File]::ReadAllText($shaderPath.FullName).Replace("`r`n", "`n").Replace("`n", "`r`n")
        [IO.File]::WriteAllText($shaderPath.FullName, $text, [Text.UTF8Encoding]::new($false))
    }
    $lfOutput = Join-Path $temporaryRoot 'lf-variant-output'
    $crlfOutput = Join-Path $temporaryRoot 'crlf-variant-output'
    & (Join-Path $lfRoot 'tools\compile-raygen.ps1') -Variant 'shipping_mobile_generic_dielectric' -OutputDirectory $lfOutput
    if ($LASTEXITCODE -ne 0) { throw "LF variant compilation failed with exit code $LASTEXITCODE." }
    & (Join-Path $crlfRoot 'tools\compile-raygen.ps1') -Variant 'shipping_mobile_generic_dielectric' -OutputDirectory $crlfOutput
    if ($LASTEXITCODE -ne 0) { throw "CRLF variant compilation failed with exit code $LASTEXITCODE." }
    $lfStats = Get-Content -LiteralPath (Join-Path $lfOutput 'shipping_mobile_generic_dielectric\raygen-stats.json') -Raw | ConvertFrom-Json
    $crlfStats = Get-Content -LiteralPath (Join-Path $crlfOutput 'shipping_mobile_generic_dielectric\raygen-stats.json') -Raw | ConvertFrom-Json
    Assert-True ($lfStats.dependencySha256 -eq $crlfStats.dependencySha256 -and
        $lfStats.compiledSpirvSha256 -eq $crlfStats.compiledSpirvSha256) 'LF/CRLF variant compilation must retain identical dependency and SPIR-V hashes.'
    Assert-True ((& git -C $repoRoot status --porcelain) -join "`n" -eq $worktreeStatusBefore) 'Temporary variant compilation modified the worktree.'
    Write-Output 'Raygen variant manifest and temporary compiler matrix passed.'
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) { Remove-Item -LiteralPath $temporaryRoot -Recurse -Force }
}
