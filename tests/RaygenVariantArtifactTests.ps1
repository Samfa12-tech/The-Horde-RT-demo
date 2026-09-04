[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$compiler = Join-Path $repoRoot 'tools\compile-raygen.ps1'
$manifestPath = Join-Path $repoRoot 'tools\raygen-variants.json'
$budgetPath = Join-Path $repoRoot 'tools\raygen-variant-budgets.json'
$catalogPath = Join-Path $repoRoot 'tools\raygen-variant-catalog.json'
$artifactDirectory = Join-Path $repoRoot 'src\vulkan\raytracing\variants'
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

function Assert-Throws {
    param([scriptblock]$Action, [string]$Message)

    $threw = $false
    try { & $Action } catch { $threw = $true }
    Assert-True $threw $Message
}

function Get-CanonicalTextHash {
    param([string]$Path)

    # Match compile-raygen.ps1's logical-source identity: UTF-8 text without a
    # BOM, normalized to LF with one trailing newline.
    $lines = [IO.File]::ReadAllLines($Path)
    $canonicalText = if ($lines.Count -eq 0) {
        ''
    } else {
        [string]::Join("`n", $lines) + "`n"
    }
    $sha256 = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString($sha256.ComputeHash([Text.Encoding]::UTF8.GetBytes($canonicalText)))).Replace('-', '').ToLowerInvariant()
    }
    finally {
        $sha256.Dispose()
    }
}

function Get-RawFileHash {
    param([string]$Path)

    $sha256 = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString($sha256.ComputeHash([IO.File]::ReadAllBytes($Path)))).Replace('-', '').ToLowerInvariant()
    }
    finally {
        $sha256.Dispose()
    }
}

function Get-BracedFunctionBody {
    param([string]$Source, [string]$FunctionName)

    $headers = [regex]::Matches($Source, ('(?m)^\s*(?:[A-Za-z_]\w*\s+)+{0}\s*\(' -f [regex]::Escape($FunctionName)))
    foreach ($header in $headers) {
        $parentheses = 0
        $bodyStart = -1
        $afterSignature = -1
        for ($index = $header.Index; $index -lt $Source.Length; ++$index) {
            $character = $Source[$index]
            if ($character -eq '(') { ++$parentheses; continue }
            if ($character -eq ')') {
                --$parentheses
                if ($parentheses -eq 0) { $afterSignature = $index + 1; break }
            }
        }
        if ($afterSignature -lt 0) { continue }
        while ($afterSignature -lt $Source.Length -and [char]::IsWhiteSpace($Source[$afterSignature])) { ++$afterSignature }
        # The flattened source contains forward declarations before definitions.
        # Only accept the exact signature followed by an opening brace.
        if ($afterSignature -ge $Source.Length -or $Source[$afterSignature] -ne '{') { continue }
        $bodyStart = $afterSignature

        $depth = 0
        $inLineComment = $false
        $inBlockComment = $false
        $inString = $false
        for ($index = $bodyStart; $index -lt $Source.Length; ++$index) {
            $character = $Source[$index]
            $next = if ($index + 1 -lt $Source.Length) { $Source[$index + 1] } else { [char]0 }
            if ($inLineComment) {
                if ($character -eq "`n") { $inLineComment = $false }
                continue
            }
            if ($inBlockComment) {
                if ($character -eq '*' -and $next -eq '/') { $inBlockComment = $false; ++$index }
                continue
            }
            if ($inString) {
                if ($character -eq '\\') { ++$index; continue }
                if ($character -eq '"') { $inString = $false }
                continue
            }
            if ($character -eq '/' -and $next -eq '/') { $inLineComment = $true; ++$index; continue }
            if ($character -eq '/' -and $next -eq '*') { $inBlockComment = $true; ++$index; continue }
            if ($character -eq '"') { $inString = $true; continue }
            if ($character -eq '{') { ++$depth; continue }
            if ($character -eq '}') {
                --$depth
                if ($depth -eq 0) { return $Source.Substring($bodyStart, $index - $bodyStart + 1) }
            }
        }
        throw "Unterminated preprocessed function body: $FunctionName"
    }
    throw "Missing preprocessed function definition: $FunctionName"
}

function Assert-MatrixRouteBudget {
    param(
        [string]$PreprocessedSource,
        [string]$FunctionName,
        [string]$InterfaceConstant,
        [string]$GuardVariable,
        [switch]$RequireStaticCeiling,
        [string]$VolumeConstant = ''
    )

    $body = Get-BracedFunctionBody -Source $PreprocessedSource -FunctionName $FunctionName
    Assert-True ($body -notmatch 'controls\.waterQuality') "$FunctionName retained runtime WaterQuality selection in a matrix compile."
    Assert-True ($body -match ("\bconst\s+int\s+interfaceBudget\s*=\s*{0}\s*;" -f [regex]::Escape($InterfaceConstant))) `
        "$FunctionName did not receive $InterfaceConstant as its matrix interface budget."
    $hasStaticCeiling = $body -match ("\bfor\s*\([^;]*;\s*\w+\s*<=\s*{0}\s*;" -f [regex]::Escape($InterfaceConstant))
    $hasCounterGuard = $body -match ("\b{0}\s*>=\s*interfaceBudget\b" -f [regex]::Escape($GuardVariable))
    if ($RequireStaticCeiling) {
        Assert-True $hasStaticCeiling `
            "$FunctionName did not retain its required static interface ceiling through $InterfaceConstant."
    }
    Assert-True $hasCounterGuard `
        "$FunctionName did not retain its $GuardVariable budget guard."
    if (-not [string]::IsNullOrWhiteSpace($VolumeConstant)) {
        Assert-True ($body -match ("\bconst\s+int\s+volumeBudget\s*=\s*{0}\s*;" -f [regex]::Escape($VolumeConstant))) `
            "$FunctionName did not receive $VolumeConstant as its matrix volume budget."
        Assert-True ($body -match ("\[\s*{0}\s*\]" -f [regex]::Escape($VolumeConstant))) `
            "$FunctionName did not compile fixed volume capacity $VolumeConstant."
        Assert-True ($body -match ("\bfor\s*\([^;]*;\s*\w+\s*<\s*{0}\s*;" -f [regex]::Escape($VolumeConstant))) `
            "$FunctionName did not retain a volume-capacity loop through $VolumeConstant."
    }
}

function Assert-PreprocessedBudgetConstants {
    param([string]$PreprocessedSource, [int]$ExpectedInterfaceBudget, [int]$ExpectedVolumeBudget)

    foreach ($constant in @(
        @{ name = 'kRtVariantDielectricInterfaceBudget'; value = $ExpectedInterfaceBudget },
        @{ name = 'kRtVariantDielectricVolumeBudget'; value = $ExpectedVolumeBudget },
        @{ name = 'kRtVariantShadowInterfaceBudget'; value = $ExpectedInterfaceBudget },
        @{ name = 'kRtVariantShadowVolumeBudget'; value = $ExpectedVolumeBudget })) {
        $declarations = @([regex]::Matches($PreprocessedSource,
            ("\bconst\s+int\s+{0}\s*=\s*(\d+)\s*;" -f [regex]::Escape($constant.name))))
        Assert-True ($declarations.Count -eq 1) `
            "Preprocessed matrix source must contain exactly one literal declaration of $($constant.name)."
        Assert-True ($declarations[0].Groups[1].Value -eq [string]$constant.value) `
            "Preprocessed matrix source did not define $($constant.name) as literal $($constant.value)."
    }
}

try {
    Assert-True (Test-Path -LiteralPath $catalogPath -PathType Leaf) `
        'Frozen raygen variant catalog is missing.'
    Assert-True (Test-Path -LiteralPath $artifactDirectory -PathType Container) `
        'Frozen raygen variant artifact directory is missing.'
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
    Assert-True ($budgets.schema -eq 1 -and $budgets.status -eq 'frozen' -and
        @($budgets.budgets).Count -eq 8 -and @($budgets.metrics).Count -eq 10) `
        'Task 3d must retain the reviewed frozen eight-key budget set.'

    New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
    $genericInclude = Join-Path $repoRoot 'src\vulkan\raytracing\MinimalRayGenShader.inc'
    $legacyInclude = Join-Path $repoRoot 'src\vulkan\raytracing\MinimalLegacyRayGenShader.inc'
    Assert-True ((Get-CanonicalTextHash $genericInclude) -eq 'fd534d390fc5d73aa65fb291fc847dde6d94a70da4d611eb274c4739d65c7087') `
        'Compatibility generic include changed unexpectedly.'
    Assert-True ((Get-CanonicalTextHash $legacyInclude) -eq 'b8ec454582c7b7e0a4f0734286b3475596b817b785b857ddc2c563e40a70bb82') `
        'Compatibility legacy include changed unexpectedly.'

    $lfFixture = Join-Path $temporaryRoot 'canonical-lf-fixture.txt'
    $crlfFixture = Join-Path $temporaryRoot 'canonical-crlf-fixture.txt'
    [IO.File]::WriteAllText($lfFixture, "generated include`nline two`n", [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText($crlfFixture, "generated include`r`nline two`r`n", [Text.UTF8Encoding]::new($false))
    Assert-True ((Get-CanonicalTextHash $lfFixture) -eq (Get-CanonicalTextHash $crlfFixture)) `
        'Canonical generated-text hashing must treat LF and CRLF forms identically.'
    Assert-True ((Get-RawFileHash $lfFixture) -ne (Get-RawFileHash $crlfFixture)) `
        'The LF/CRLF fixture must prove canonical text hashing is not raw-byte hashing.'

    $staleCeilingFixture = @'
vec3 shadeBoundedDielectric(HitInfo firstHit, vec3 rayDirection)
{
    const int interfaceBudget = kRtVariantDielectricInterfaceBudget;
    if (interfaceIndex >= interfaceBudget) return vec3(0.0);
    return vec3(1.0);
}
'@
    Assert-Throws {
        Assert-MatrixRouteBudget -PreprocessedSource $staleCeilingFixture -FunctionName 'shadeBoundedDielectric' `
            -InterfaceConstant 'kRtVariantDielectricInterfaceBudget' -GuardVariable 'interfaceIndex' -RequireStaticCeiling
    } 'A dielectric route with only a stale/missing static ceiling must fail the route contract.'

    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $expectedKeys = @(
        'shipping_mobile_opaque_fast', 'shipping_mobile_generic_dielectric',
        'shipping_high_opaque_fast', 'shipping_high_generic_dielectric',
        'diagnostic_mobile_opaque_fast', 'diagnostic_mobile_generic_dielectric',
        'diagnostic_high_opaque_fast', 'diagnostic_high_generic_dielectric')
    Assert-True ((Compare-Object ($expectedKeys | Sort-Object) (@($manifest.variants | ForEach-Object name | Sort-Object))).Count -eq 0) `
        'Matrix manifest no longer contains the exact eight approved keys.'

    # CheckCatalog is the sole matrix compilation in this test. It replays all
    # eight compile/preprocess/validate/disassemble paths into temporary storage
    # and compares catalog, include words, raw SPIR-V, budgets, and toolchain.
    & $compiler -CheckCatalog -ArtifactDirectory $artifactDirectory -CatalogPath $catalogPath -BudgetPath $budgetPath
    if ($LASTEXITCODE -ne 0) { throw "Frozen catalog check failed with exit code $LASTEXITCODE." }

    $compatibilityGenericOutput = Join-Path $temporaryRoot 'compatibility-generic'
    & $compiler -Check -OutputDirectory $compatibilityGenericOutput
    if ($LASTEXITCODE -ne 0) { throw "Generic compatibility freshness failed with exit code $LASTEXITCODE." }
    Assert-True ((Get-RawFileHash (Join-Path $compatibilityGenericOutput 'minimal.rgen.spv')) -eq
        'e9d4fca05e8c642b6e09251a7c57253124fa6475ab5f81e34d1acb548764c23a') `
        'Compatibility generic SPIR-V words changed.'
    $compatibilityLegacyOutput = Join-Path $temporaryRoot 'compatibility-legacy'
    & $compiler -Legacy -Check -OutputDirectory $compatibilityLegacyOutput
    if ($LASTEXITCODE -ne 0) { throw "Legacy compatibility freshness failed with exit code $LASTEXITCODE." }
    Assert-True ((Get-RawFileHash (Join-Path $compatibilityLegacyOutput 'minimal.legacy.rgen.spv')) -eq
        '870e4ea0c0b24fdcac516fec15343c1531906600d8ec28c9eaebf826a7bd75a0') `
        'Compatibility legacy SPIR-V words changed.'
    Assert-True ((& git -C $repoRoot status --porcelain) -join "`n" -eq $worktreeStatusBefore) `
        'Temporary artifact compilation modified the worktree.'
    Write-Output 'Raygen variant artifact specialization and compatibility contracts passed.'
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) { Remove-Item -LiteralPath $temporaryRoot -Recurse -Force }
}
