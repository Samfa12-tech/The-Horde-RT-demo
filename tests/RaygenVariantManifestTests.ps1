[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$compiler = Join-Path $repoRoot 'tools\compile-raygen.ps1'
$manifestPath = Join-Path $repoRoot 'tools\raygen-variants.json'
$budgetsPath = Join-Path $repoRoot 'tools\raygen-variant-budgets.json'
$configPath = Join-Path $repoRoot 'shaders\raytracing\include\rt_variant_config.glsl'
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

try {
    foreach ($path in @($manifestPath, $budgetsPath, $configPath)) {
        Assert-True (Test-Path -LiteralPath $path -PathType Leaf) "Missing required variant foundation file: $path"
    }

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

    $genericHashBefore = Get-CanonicalFileHash (Join-Path $repoRoot 'src\vulkan\raytracing\MinimalRayGenShader.inc')
    $legacyHashBefore = Get-CanonicalFileHash (Join-Path $repoRoot 'src\vulkan\raytracing\MinimalLegacyRayGenShader.inc')
    Assert-True ($genericHashBefore -eq 'd0c8cccea28d6c4deb02256dd4be0e828416a1f4087d64dc505bcacb0485fa56') 'Generic include hash changed before matrix compilation.'
    Assert-True ($legacyHashBefore -eq '100109525677b2d6b3327cecd1128ecd999bf556d46c30f9c1c925d0349e8300') 'Legacy include hash changed before matrix compilation.'

    $matrixCompilerOutput = @(& $compiler -Matrix -OutputDirectory $temporaryRoot)
    if ($LASTEXITCODE -ne 0) { throw "Matrix compiler failed with exit code $LASTEXITCODE." }
    $reportedOrder = @($matrixCompilerOutput | ForEach-Object {
        if ($_ -match '^Raygen variant ([a-z_]+):') { $Matches[1] }
    } | Where-Object { $_ })
    Assert-True ((@($reportedOrder) -join ',') -eq (@($expected | Sort-Object) -join ',')) `
        'Matrix compiler output order must be deterministic and sorted by stable key.'
    $dependencyHashes = @()
    foreach ($variant in $manifest.variants) {
        $variantRoot = Join-Path $temporaryRoot $variant.name
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
            Assert-True ($stats.functions -gt 1 -and $stats.functionCalls -gt 0 -and $stats.rayQueryInitializations -le 3) "Generic strategy shape regressed for $($variant.name)."
        } else {
            Assert-True ($stats.functions -eq 1 -and $stats.functionCalls -eq 0 -and $stats.rayQueryInitializations -le 29) "Opaque-fast strategy shape regressed for $($variant.name)."
        }
    }
    Assert-True ((@($dependencyHashes | Sort-Object -Unique).Count -eq 8)) `
        'Every key must have a unique dependency identity.'
    $singleOutputRoot = Join-Path $temporaryRoot 'single-variant'
    $singleCompilerOutput = @(& $compiler -Variant 'shipping_mobile_generic_dielectric' -OutputDirectory $singleOutputRoot)
    if ($LASTEXITCODE -ne 0) { throw "Single variant compiler failed with exit code $LASTEXITCODE." }
    $matrixStatsText = Get-Content -LiteralPath (Join-Path $temporaryRoot 'shipping_mobile_generic_dielectric\raygen-stats.json') -Raw
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
