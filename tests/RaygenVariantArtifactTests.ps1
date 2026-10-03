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
$dielectricCommonPath = Join-Path $repoRoot 'shaders\raytracing\include\rt_dielectric_common.glsl'
$transportPath = Join-Path $repoRoot 'shaders\raytracing\include\rt_dielectric_transport.glsl'
$dielectricSpawnPath = Join-Path $repoRoot 'shaders\raytracing\include\rt_dielectric_spawn.glsl'
$hitDecodePath = Join-Path $repoRoot 'shaders\raytracing\include\rt_hit_decode.glsl'
$staticMeshAssetPath = Join-Path $repoRoot 'src\scene\assets\StaticMeshAsset.cpp'
$staticMeshSlotPath = Join-Path $repoRoot 'src\vulkan\raytracing\RtStaticMeshSlot.cpp'
$dielectricMathPath = Join-Path $repoRoot 'src\vulkan\raytracing\DielectricMath.h'
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

function Assert-FrozenCatalogShape {
    param([pscustomobject]$Catalog)

    $rootFields = 'authorities,generator,schema,status,target,toolchain,variants'
    Assert-True ((@($Catalog.PSObject.Properties.Name | Sort-Object) -join ',') -eq $rootFields) `
        'Catalog root has a missing, reordered, or unknown field.'
    Assert-True ($Catalog.schema -eq 1 -and $Catalog.status -eq 'frozen' -and @($Catalog.variants).Count -eq 8) `
        'Catalog must remain a frozen schema-1 eight-key record.'
    $rowFields = 'artifactPath,atomicInstructions,boundedGenericFunctionsRetained,branchOperations,bytes,compiler,dependencies,dependencySha256,driverSafeFullyInlined,functionCalls,functions,hasDiagnosticsBinding,includeSha256,instructions,instrumentation,key,loops,material,quality,rayQueryInitializations,selectionMerges,shippingAllowed,spirvSha256,strategy,words'
    $compilerFields = 'glslangCompileArguments,spirvDisArguments,spirvOptArguments,spirvValArguments'
    foreach ($row in @($Catalog.variants)) {
        Assert-True ((@($row.PSObject.Properties.Name | Sort-Object) -join ',') -eq $rowFields) `
            "Catalog row is malformed or has an extra field: $($row.key)"
        Assert-True ((@($row.compiler.PSObject.Properties.Name | Sort-Object) -join ',') -eq $compilerFields) `
            "Catalog compiler invocation is incomplete or malformed: $($row.key)"
        Assert-True ($row.artifactPath -match '^src/vulkan/raytracing/variants/[a-z_]+\.inc$' -and
            -not $row.artifactPath.Contains('..') -and -not $row.artifactPath.Contains('\')) `
            "Catalog artifact path escaped the dedicated artifact directory: $($row.key)"
    }
}

function New-CatalogFixture {
    param([string]$FixtureRoot, [string]$Name, [string]$ExpectedDiagnostic, [scriptblock]$Mutate)

    $fixture = Join-Path $FixtureRoot $Name
    $fixtureVariants = Join-Path $fixture 'variants'
    New-Item -ItemType Directory -Force -Path $fixtureVariants | Out-Null
    Copy-Item -LiteralPath $catalogPath -Destination (Join-Path $fixture 'raygen-variant-catalog.json')
    Copy-Item -LiteralPath $budgetPath -Destination (Join-Path $fixture 'raygen-variant-budgets.json')
    Get-ChildItem -LiteralPath $artifactDirectory -Filter '*.inc' -File | Copy-Item -Destination $fixtureVariants
    [IO.File]::WriteAllText((Join-Path $fixture 'expected-diagnostic.txt'), $ExpectedDiagnostic, [Text.UTF8Encoding]::new($false))
    & $Mutate $fixture $fixtureVariants
}

function Write-FixtureJson {
    param([string]$Path, [object]$Value)
    [IO.File]::WriteAllText($Path, (($Value | ConvertTo-Json -Depth 24).Replace("`r`n", "`n") + "`n"), [Text.UTF8Encoding]::new($false))
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

function Get-DielectricTransportQueryArguments {
    param([string]$TransportSource)

    $queries = [System.Collections.Generic.List[object]]::new()
    foreach ($functionName in @('shadeBoundedDielectric', 'shadeProductionBoundedDielectric')) {
        $body = Get-BracedFunctionBody -Source $TransportSource -FunctionName $functionName
        $bodyOffset = $TransportSource.IndexOf($body, [StringComparison]::Ordinal)
        $traceQueryCount = [regex]::Matches($body, '\btraceScene\s*\(').Count
        Assert-True ($traceQueryCount -eq 2) "$functionName must retain exactly its reflection and continuation scene queries."
        Assert-True ($body -match '(?s)float\s+reflectionEpsilon\s*=\s*dielectricSpawnEpsilon\s*\(\s*firstHit\s*,\s*firstHit\.t\s*\)\s*;') `
            "$functionName reflection must carry the matching hit's proven minimum normal bias."
        Assert-True ($body -match '(?s)float\s+epsilon\s*=\s*dielectricSpawnEpsilon\s*\(\s*currentHit\s*,\s*totalDistance\s*\)\s*;') `
            "$functionName continuation must carry the matching hit's proven minimum normal bias."
        Assert-True ($body -match '(?s)\badvanceDielectricRayOrigin\s*\(\s*dielectricSpawnPoint\s*\(\s*firstHit\s*\)') `
            "$functionName reflection query must use the guarded first-hit spawn point."
        Assert-True ($body -match '(?s)\bvec3\s+nextOrigin\s*=\s*advanceDielectricRayOrigin\s*\(\s*dielectricSpawnPoint\s*\(\s*currentHit\s*\)') `
            "$functionName continuation query must use the guarded current-hit spawn point."
        Assert-True ($body -match '(?s)\bHitInfo\s+reflectedHit\s*=\s*traceScene\s*\(\s*advanceDielectricRayOrigin\s*\(\s*dielectricSpawnPoint\s*\(\s*firstHit\s*\)') `
            "$functionName reflection query must pass the guarded first-hit origin to traceScene."
        Assert-True ($body -match '(?s)\bHitInfo\s+nextHit\s*=\s*traceScene\s*\(\s*nextOrigin\s*,\s*transmissionDirection\s*,') `
            "$functionName continuation query must pass nextOrigin and transmissionDirection to traceScene."
        Assert-True ($body -match '(?s)advanceDielectricRayOrigin\s*\(\s*dielectricSpawnPoint\s*\(\s*firstHit\s*\)\s*,\s*firstOutward\s*,\s*reflectionDirection\s*,\s*reflectionEpsilon\s*\)') `
            "$functionName actual reflection origin must use the matching guarded epsilon."
        Assert-True ($body -match '(?s)vec3\s+nextOrigin\s*=\s*advanceDielectricRayOrigin\s*\(\s*dielectricSpawnPoint\s*\(\s*currentHit\s*\)\s*,\s*outwardNormal\s*,\s*transmissionDirection\s*,\s*epsilon\s*\)') `
            "$functionName actual continuation origin must use the matching guarded epsilon."

        foreach ($query in @(
            @{ Name = 'reflection'; Pattern = '(?s)\bHitInfo\s+reflectedHit\s*=\s*traceScene\s*\(\s*advanceDielectricRayOrigin\s*\(\s*dielectricSpawnPoint\s*\(\s*firstHit\s*\).*?\)\s*,\s*reflectionDirection\s*,\s*[^,]+,\s*[^,]+,\s*(?<minimum>[^,]+),' },
            @{ Name = 'continuation'; Pattern = '(?s)\bHitInfo\s+nextHit\s*=\s*traceScene\s*\(\s*nextOrigin\s*,\s*transmissionDirection\s*,\s*[^,]+,\s*[^,]+,\s*(?<minimum>[^,]+),' })) {
            $matches = [regex]::Matches($body, $query.Pattern)
            Assert-True ($matches.Count -eq 1) "$functionName must have exactly one structurally matched $($query.Name) query."
            $minimum = $matches[0].Groups['minimum']
            $queries.Add([pscustomobject]@{
                FunctionName = $functionName
                QueryName = $query.Name
                MinimumArgument = $minimum.Value.Trim()
                MinimumOffset = $bodyOffset + $minimum.Index
                MinimumLength = $minimum.Length
            })
        }
    }
    return @($queries)
}

function Assert-DielectricRayMinimumDistanceContract {
    param(
        [string]$TransportSource,
        [string]$GlslCommonSource,
        [string]$CppMathSource,
        [string]$SpawnSource,
        [string]$HitDecodeSource,
        [string]$AssetSource,
        [string]$StaticMeshSlotSource,
        [string]$GeneratedAbiSource
    )

    Assert-True ($GlslCommonSource -match '(?m)^const\s+float\s+kDielectricRayMinimumDistance\s*=\s*0\.000001\s*;') `
        'GLSL dielectric minimum query distance must remain the independent 1 µm constant.'
    Assert-True ($CppMathSource -match '(?m)^inline\s+constexpr\s+float\s+kDielectricRayMinimumDistance\s*=\s*0\.000001f\s*;') `
        'CPU dielectric minimum query distance must remain the independent 1 µm constant.'

    $spawnPointBody = Get-BracedFunctionBody -Source $GlslCommonSource -FunctionName 'dielectricSpawnPoint'
    Assert-True ($spawnPointBody -match '(?s)if\s*\(\s*hit\.dielectricSpawnGuarded\s*\)\s*return\s+hit\.dielectricSpawnPosition\s*;\s*#endif\s*return\s+hit\.position\s*;') `
        'Dielectric spawn selection must use the guarded point only for proven hits and retain the exact physical hit-point fallback.'
    $minimumBody = Get-BracedFunctionBody -Source $GlslCommonSource -FunctionName 'dielectricQueryMinimum'
    Assert-True ($minimumBody -match '(?s)if\s*\(\s*hit\.dielectricSpawnGuarded\s*\)\s*return\s+0\.0\s*;\s*#endif\s*return\s+kDielectricRayMinimumDistance\s*;') `
        'Only a proven guarded spawn may use zero minimum; all fallback queries must retain the independent 1 µm minimum.'
    $epsilonBody = Get-BracedFunctionBody -Source $GlslCommonSource -FunctionName 'dielectricSpawnEpsilon'
    Assert-True ($epsilonBody -match '(?s)float\s+epsilon\s*=\s*dielectricRayEpsilon\s*\(\s*hit\.position\s*,\s*interfaceDistance\s*\)\s*;.*if\s*\(\s*hit\.dielectricSpawnGuarded\s*\)\s*return\s+max\s*\(\s*epsilon\s*,\s*hit\.dielectricSpawnMinimumNormalBias\s*\)\s*;\s*#endif\s*return\s+epsilon\s*;') `
        'Only an admitted guard may raise the origin offset to its calculated normal-separation minimum; fallback epsilon stays unchanged.'

    $guardBody = Get-BracedFunctionBody -Source $SpawnSource -FunctionName 'guardedRectangularDielectricSpawn'
    foreach ($finiteValue in @('localPosition', 'surfacePosition', 'geometricNormal', 'objectOffsetDirection', 'guardedWorld')) {
        Assert-True ($guardBody.Contains("any(isnan($finiteValue))") -and $guardBody.Contains("any(isinf($finiteValue))")) `
            "The rectangular spawn guard must fail closed on non-finite $finiteValue."
    }
    Assert-True ($guardBody -match '(?s)spawnPosition\s*=\s*surfacePosition\s*;') `
        'The rectangular spawn helper must initialize its output to the unmodified surface fallback before any rejection.'
    Assert-True ($guardBody.Contains('minimumNormalBias = 0.0;') -and
        $guardBody.Contains('isnan(separationScaleLower)') -and $guardBody.Contains('isinf(separationScaleLower)') -and
        $guardBody.Contains('separationScaleLower <= 0.0') -and
        $guardBody.Contains('gamma16 * dot(abs(objectNormal), absoluteW2o * abs(geometricNormal))') -and
        $guardBody.Contains('requiredBias = uintBitsToFloat(floatBitsToUint(requiredBias) + 1u);') -and
        $guardBody.Contains('requiredBias > 0.00025') -and
        $guardBody.Contains('if (separationScaleLower * lowerNormalBias <= separationErrorUpper ||') -and
        $guardBody -match '(?s)spawnPosition\s*=\s*guardedWorld\s*;\s*minimumNormalBias\s*=\s*lowerNormalBias\s*;\s*return\s+true\s*;') `
        'Per-hit normal bias must use an absolute projection bound, upward rounding, strict finite separation and the unchanged 250 µm cap, publishing only on guard success.'
    Assert-True ($guardBody -match '(?s)\+\s*vec3\s*\(\s*geometryError\s*\)') `
        'The conservative loader-derived geometry error must contribute to the spawn uncertainty bound.'
    Assert-True ($guardBody -match '(?s)vec3\s+candidate\s*=\s*start\s*\+\s*delta\s*\*\s*clamp\s*\(\s*dot\s*\(\s*localPosition\s*-\s*start\s*,\s*delta\s*\)\s*/\s*squaredLength\s*,\s*0\.0\s*,\s*1\.0\s*\)\s*;\s*float\s+squared\s*=\s*dot\s*\(\s*candidate\s*-\s*localPosition\s*,\s*candidate\s*-\s*localPosition\s*\)\s*;\s*if\s*\(\s*squared\s*<\s*nearestSquared\s*\)') `
        'The inset must select the metric-nearest point on the admissible triangle, not an authored barycentric slide.'
    Assert-True ($guardBody -match '(?s)length\s*\(\s*guardedLocal\s*-\s*localPosition\s*\)\s*\*\s*\(1\.0\s*\+\s*gamma8\)\s*>\s*2\.0\s*\*\s*length\s*\(\s*objectError\s*\)\s*\*\s*\(1\.0\s*-\s*gamma8\)\s*\)\s*return\s+false\s*;') `
        'Triangle inset beyond twice the derived object-space error bound must reject guard eligibility.'
    Assert-True ($guardBody.Contains('objectError += gamma8 * (abs(v0) + abs(e1) + abs(e2));') -and
        $guardBody.Contains('floatBitsToUint(objectError * (1.0 + gamma32)) + uvec3(1u)') -and
        $guardBody -notmatch '\bgamma4\b') `
        'Spawn construction and affine bounds must retain conservative operation counts and upward-rounded uncertainty.'
    Assert-True ($HitDecodeSource -match '(?s)h\.dielectricSpawnPosition\s*=\s*surfacePosition\s*;\s*if\s*\(\s*\(h\.materialFlags\s*&\s*kRtMaterialFlagCertifiedRectangularVolume\)\s*!=\s*0u\s*\)\s*h\.dielectricSpawnGuarded\s*=\s*guardedRectangularDielectricSpawn\s*\(') `
        'Hit decoding must preserve the surface fallback and invoke the guard only for loader-certified rectangular-volume material.'
    Assert-True ($HitDecodeSource -match '(?s)h\.thickness\s*=\s*max\s*\(\s*staticMaterial\.iorThicknessAttenuationDistance\.y\s*,\s*0\.0\s*\)\s*;') `
        'The physical dielectric thickness must remain sourced from the authored material value.'
    Assert-True ($HitDecodeSource -match '(?s)staticMaterial\.iorThicknessAttenuationDistance\.w\s*,\s*staticMaterial\.attenuationColor\.w\s*,\s*h\.dielectricSpawnPosition') `
        'Spawn clearance width and geometry error must use their loader-derived fields, independently of authored thickness.'
    Assert-True ($HitDecodeSource -match '(?s)h\.dielectricSpawnGuarded\s*=\s*false\s*;') `
        'Every trace result must default to unguarded so a rejected or ineligible hit cannot inherit zero-minimum eligibility.'
    Assert-True ($HitDecodeSource.Contains('h.dielectricSpawnMinimumNormalBias = 0.0;') -and
        $HitDecodeSource.Contains('h.dielectricSpawnPosition, h.dielectricSpawnMinimumNormalBias);')) `
        'Hit decoding must initialize and carry the guard minimum, not discard its source-face separation result.'
    Assert-True ($GeneratedAbiSource -match '\bkRtMaterialFlagCertifiedRectangularVolume\s*=\s*4096u\s*;') `
        'The guarded spawn eligibility bit must remain the canonical 4096 ABI flag.'
    Assert-True ($AssetSource -match '(?s)if\s*\(everyComponentIsRectangular\)\s*\{\s*material\.flags\s*\|=\s*certifiedRectangularVolumeFlag\s*;') `
        'The rectangular eligibility bit must only be set after every closed component passes the loader certification.'
    Assert-True ($AssetSource -match '(?s)material\.numericalSpawnMinimumWidth\s*=\s*std::nextafter\s*\(' -and
        $AssetSource -match '(?s)material\.numericalSpawnGeometryError\s*=\s*std::nextafter\s*\(' -and
        $StaticMeshSlotSource -match '(?s)iorThicknessAttenuationDistance\s*=\s*\{\{\s*source\.ior\s*,\s*source\.thicknessFactor\s*,\s*source\.attenuationDistance\s*,\s*source\.numericalSpawnMinimumWidth\s*\}\}' -and
        $StaticMeshSlotSource -match '(?s)attenuationColor\s*=\s*\{\{\s*source\.attenuationColor\[0\]\s*,\s*source\.attenuationColor\[1\]\s*,\s*source\.attenuationColor\[2\]\s*,\s*source\.numericalSpawnGeometryError\s*\}\}') `
        'Loader-derived width and geometry error must be conservatively rounded and carried in their documented shader-material lanes.'

    $queries = @(Get-DielectricTransportQueryArguments -TransportSource $TransportSource)
    Assert-True ($queries.Count -eq 4) 'Both dielectric transport routes must retain all four independently bounded ray queries.'
    foreach ($query in $queries) {
        $expectedHit = if ($query.QueryName -eq 'reflection') { 'firstHit' } else { 'currentHit' }
        Assert-True ($query.MinimumArgument -match ('^dielectricQueryMinimum\s*\(\s*{0}\s*\)$' -f $expectedHit)) `
            "$($query.FunctionName) $($query.QueryName) query must select its minimum through the matching guarded helper."
    }
    return $queries
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
    $dielectricCommon = Get-Content -LiteralPath $dielectricCommonPath -Raw
    $dielectricSpawn = Get-Content -LiteralPath $dielectricSpawnPath -Raw
    $hitDecode = Get-Content -LiteralPath $hitDecodePath -Raw
    $staticMeshAsset = Get-Content -LiteralPath $staticMeshAssetPath -Raw
    $staticMeshSlot = Get-Content -LiteralPath $staticMeshSlotPath -Raw
    $generatedAbi = Get-Content -LiteralPath $generatedAbiPath -Raw
    $dielectricMath = Get-Content -LiteralPath $dielectricMathPath -Raw
    $lighting = Get-Content -LiteralPath $lightingPath -Raw
    $dielectricQueries = @(Assert-DielectricRayMinimumDistanceContract -TransportSource $transport `
        -GlslCommonSource $dielectricCommon -CppMathSource $dielectricMath -SpawnSource $dielectricSpawn `
        -HitDecodeSource $hitDecode -AssetSource $staticMeshAsset -StaticMeshSlotSource $staticMeshSlot `
        -GeneratedAbiSource $generatedAbi
    )
    foreach ($dielectricQuery in $dielectricQueries) {
        $mutatedTransport = $transport.Substring(0, $dielectricQuery.MinimumOffset) +
            'epsilon * 0.5' +
            $transport.Substring($dielectricQuery.MinimumOffset + $dielectricQuery.MinimumLength)
        Assert-Throws {
            Assert-DielectricRayMinimumDistanceContract -TransportSource $mutatedTransport `
                -GlslCommonSource $dielectricCommon -CppMathSource $dielectricMath -SpawnSource $dielectricSpawn `
                -HitDecodeSource $hitDecode -AssetSource $staticMeshAsset -StaticMeshSlotSource $staticMeshSlot `
                -GeneratedAbiSource $generatedAbi
        } "A $($dielectricQuery.FunctionName) $($dielectricQuery.QueryName) query using epsilon/2 must fail the independent-minimum contract."

        $unconditionalQueryZero = $transport.Substring(0, $dielectricQuery.MinimumOffset) +
            '0.0' +
            $transport.Substring($dielectricQuery.MinimumOffset + $dielectricQuery.MinimumLength)
        Assert-Throws {
            Assert-DielectricRayMinimumDistanceContract -TransportSource $unconditionalQueryZero `
                -GlslCommonSource $dielectricCommon -CppMathSource $dielectricMath -SpawnSource $dielectricSpawn `
                -HitDecodeSource $hitDecode -AssetSource $staticMeshAsset -StaticMeshSlotSource $staticMeshSlot `
                -GeneratedAbiSource $generatedAbi
        } "An unconditional zero at the actual $($dielectricQuery.FunctionName) $($dielectricQuery.QueryName) query site must fail even while unused helper calls remain."
    }
    $unconditionalZeroMinimum = [regex]::Replace(
        $dielectricCommon,
        '(?s)(if\s*\(\s*hit\.dielectricSpawnGuarded\s*\)\s*return\s+)0\.0\s*;',
        '${1}0.0; return 0.0;')
    Assert-Throws {
        Assert-DielectricRayMinimumDistanceContract -TransportSource $transport `
            -GlslCommonSource $unconditionalZeroMinimum -CppMathSource $dielectricMath -SpawnSource $dielectricSpawn `
            -HitDecodeSource $hitDecode -AssetSource $staticMeshAsset -StaticMeshSlotSource $staticMeshSlot `
            -GeneratedAbiSource $generatedAbi
    } 'An unconditional zero query minimum must fail the guarded-minimum contract.'
    $bypassedEligibility = $hitDecode.Replace(
        'if ((h.materialFlags & kRtMaterialFlagCertifiedRectangularVolume) != 0u)',
        'if ((h.materialFlags & kRtMaterialFlagTransmission) != 0u)')
    Assert-Throws {
        Assert-DielectricRayMinimumDistanceContract -TransportSource $transport `
            -GlslCommonSource $dielectricCommon -CppMathSource $dielectricMath -SpawnSource $dielectricSpawn `
            -HitDecodeSource $bypassedEligibility -AssetSource $staticMeshAsset -StaticMeshSlotSource $staticMeshSlot `
            -GeneratedAbiSource $generatedAbi
    } 'A material-eligibility bypass must fail the loader-certified rectangular guard contract.'
    foreach ($finiteValue in @('localPosition', 'surfacePosition', 'geometricNormal', 'objectOffsetDirection', 'guardedWorld')) {
        $missingFiniteCheck = $dielectricSpawn.Replace("any(isnan($finiteValue))", 'false')
        Assert-Throws {
            Assert-DielectricRayMinimumDistanceContract -TransportSource $transport `
                -GlslCommonSource $dielectricCommon -CppMathSource $dielectricMath -SpawnSource $missingFiniteCheck `
                -HitDecodeSource $hitDecode -AssetSource $staticMeshAsset -StaticMeshSlotSource $staticMeshSlot `
                -GeneratedAbiSource $generatedAbi
        } "A missing $finiteValue finite check must reject the guard source contract."
    }
    Assert-True ($transport.Contains('HORDE_RT_DIELECTRIC_INTERFACE_CEILING') -and
        $transport.Contains('HORDE_RT_DIELECTRIC_VOLUME_CAPACITY') -and
        $lighting.Contains('HORDE_RT_SHADOW_INTERFACE_CEILING') -and
        $lighting.Contains('HORDE_RT_SHADOW_VOLUME_CAPACITY') -and
        $transport.Contains('controls.waterQuality >= 1.5') -and
        $lighting.Contains('controls.waterQuality >= 1.5')) `
        'Matrix budgets must specialise all bounded dielectric/shadow routes while macro-absent compatibility retains runtime WaterQuality selection.'
    foreach ($brokenGuard in @(
        $dielectricSpawn.Replace('requiredBias > 0.00025', 'false'),
        $dielectricSpawn.Replace('floatBitsToUint(requiredBias) + 1u', 'floatBitsToUint(requiredBias) + 0u'),
        $dielectricSpawn.Replace('gamma16 * dot(abs(objectNormal), absoluteW2o * abs(geometricNormal))', '0.0'),
        $dielectricSpawn.Replace('isnan(separationScaleLower)', 'false'))) {
        Assert-Throws {
            Assert-DielectricRayMinimumDistanceContract -TransportSource $transport `
                -GlslCommonSource $dielectricCommon -CppMathSource $dielectricMath -SpawnSource $brokenGuard `
                -HitDecodeSource $hitDecode -AssetSource $staticMeshAsset -StaticMeshSlotSource $staticMeshSlot `
                -GeneratedAbiSource $generatedAbi
        } 'Removing a per-hit normal-separation cap, upward rounding, absolute projection bound or finite check must fail.'
    }
    $discardedBias = $dielectricCommon.Replace('max(epsilon, hit.dielectricSpawnMinimumNormalBias)', 'epsilon')
    Assert-Throws {
        Assert-DielectricRayMinimumDistanceContract -TransportSource $transport `
            -GlslCommonSource $discardedBias -CppMathSource $dielectricMath -SpawnSource $dielectricSpawn `
            -HitDecodeSource $hitDecode -AssetSource $staticMeshAsset -StaticMeshSlotSource $staticMeshSlot `
            -GeneratedAbiSource $generatedAbi
    } 'A calculated but discarded guarded minimum must fail the origin-selection contract.'
    foreach ($offsetMatch in [regex]::Matches($transport, 'dielectricSpawnEpsilon\((firstHit, firstHit\.t|currentHit, totalDistance)\)')) {
        $wrongOffset = $transport.Substring(0, $offsetMatch.Index) + 'dielectricRayEpsilon(vec3(0.0), 0.0)' +
            $transport.Substring($offsetMatch.Index + $offsetMatch.Length)
        Assert-Throws {
            Assert-DielectricRayMinimumDistanceContract -TransportSource $wrongOffset `
                -GlslCommonSource $dielectricCommon -CppMathSource $dielectricMath -SpawnSource $dielectricSpawn `
                -HitDecodeSource $hitDecode -AssetSource $staticMeshAsset -StaticMeshSlotSource $staticMeshSlot `
                -GeneratedAbiSource $generatedAbi
        } 'Each actual reflection/continuation offset must carry its matching hit minimum; unused helper calls are insufficient.'
    }
    foreach ($offsetUse in [regex]::Matches($transport, '(reflectionDirection, reflectionEpsilon|transmissionDirection, epsilon)\)')) {
        $wrongUse = $transport.Substring(0, $offsetUse.Index) + 'vec3(0.0), 0.00002)' +
            $transport.Substring($offsetUse.Index + $offsetUse.Length)
        Assert-Throws {
            Assert-DielectricRayMinimumDistanceContract -TransportSource $wrongUse `
                -GlslCommonSource $dielectricCommon -CppMathSource $dielectricMath -SpawnSource $dielectricSpawn `
                -HitDecodeSource $hitDecode -AssetSource $staticMeshAsset -StaticMeshSlotSource $staticMeshSlot `
                -GeneratedAbiSource $generatedAbi
        } 'Discarding the calculated epsilon at an actual origin call must fail even with an unused helper assignment.'
    }

    $budgets = Get-Content -LiteralPath $budgetPath -Raw | ConvertFrom-Json
    Assert-True ($budgets.schema -eq 1 -and $budgets.status -eq 'frozen' -and
        @($budgets.budgets).Count -eq 8 -and @($budgets.metrics).Count -eq 10) `
        'Task 3d must retain the reviewed frozen eight-key budget set.'

    New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
    $genericInclude = Join-Path $repoRoot 'src\vulkan\raytracing\MinimalRayGenShader.inc'
    $legacyInclude = Join-Path $repoRoot 'src\vulkan\raytracing\MinimalLegacyRayGenShader.inc'
    # Compatibility pins restore the measured September 30 control traversal;
    # the independent fresh compiler check below still validates source identity.
    Assert-True ((Get-CanonicalTextHash $genericInclude) -eq 'f4170abbf7f68364d7eeb5d9d01baeed0efacb2fc6db7ab4d1f4c0075dddacd4') `
        'Compatibility generic include changed unexpectedly.'
    Assert-True ((Get-CanonicalTextHash $legacyInclude) -eq 'a3b32262260a25e31fbc880f851baabc5df14bbbb7995fd532e5696afd3e06bb') `
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

    $catalog = Get-Content -LiteralPath $catalogPath -Raw | ConvertFrom-Json
    Assert-FrozenCatalogShape -Catalog $catalog
    Assert-True ((@($catalog.variants | ForEach-Object key | Sort-Object) -join ',') -eq ((@($expectedKeys | Sort-Object)) -join ',')) `
        'Catalog keys must remain the exact approved set without duplicates or reordering.'
    Assert-True ((@($catalog.variants | Where-Object { $_.material -eq 'OpaqueFast' } | Group-Object spirvSha256 | Where-Object Count -eq 2).Count -eq 2)) `
        'Distinct OpaqueFast artifact paths must permit their reviewed equal raw SPIR-V pairs.'
    Assert-Throws {
        $unknownFieldCatalog = Get-Content -LiteralPath $catalogPath -Raw | ConvertFrom-Json
        $unknownFieldCatalog.variants[0] | Add-Member -NotePropertyName unexpected -NotePropertyValue 'reject'
        Assert-FrozenCatalogShape -Catalog $unknownFieldCatalog
    } 'Catalog schema must reject unknown row fields.'
    Assert-Throws {
        $escapeCatalog = Get-Content -LiteralPath $catalogPath -Raw | ConvertFrom-Json
        $escapeCatalog.variants[0].artifactPath = '../escape.inc'
        Assert-FrozenCatalogShape -Catalog $escapeCatalog
    } 'Catalog schema must reject artifact path escape.'
    Assert-Throws {
        $malformedCatalog = Get-Content -LiteralPath $catalogPath -Raw | ConvertFrom-Json
        $malformedCatalog.variants[0].compiler.PSObject.Properties.Remove('spirvValArguments')
        Assert-FrozenCatalogShape -Catalog $malformedCatalog
    } 'Catalog schema must reject missing compiler arguments.'
    Assert-Throws {
        & $compiler -CheckCatalog -ArtifactDirectory $temporaryRoot -CatalogPath $catalogPath -BudgetPath $budgetPath
        if ($LASTEXITCODE -ne 0) { throw 'expected path containment failure' }
    } 'Catalog checker must reject a shared or escaping artifact directory.'
    Assert-Throws {
        & $compiler -CheckCatalog -ArtifactDirectory $artifactDirectory -CatalogPath (Join-Path $temporaryRoot 'catalog.json') -BudgetPath $budgetPath
        if ($LASTEXITCODE -ne 0) { throw 'expected path containment failure' }
    } 'Catalog checker must reject a substituted catalog path.'
    $includeFixture = Join-Path $temporaryRoot 'one-byte-spirv-mutation.inc'
    Copy-Item -LiteralPath (Join-Path $artifactDirectory "$($catalog.variants[0].key).inc") -Destination $includeFixture
    $includeText = Get-Content -LiteralPath $includeFixture -Raw
    $firstWord = [regex]::Match($includeText, '0x([0-9a-fA-F])')
    Assert-True $firstWord.Success 'Frozen include must contain a mutable raw SPIR-V word.'
    $replacementNibble = if ($firstWord.Groups[1].Value -eq '0') { '1' } else { '0' }
    $mutatedIncludeText = $includeText.Remove($firstWord.Index + 2, 1).Insert($firstWord.Index + 2, $replacementNibble)
    [IO.File]::WriteAllText($includeFixture, $mutatedIncludeText, [Text.UTF8Encoding]::new($false))
    Assert-True ((Get-RawFileHash $includeFixture) -ne (Get-RawFileHash (Join-Path $artifactDirectory "$($catalog.variants[0].key).inc"))) `
        'A one-byte SPIR-V-word mutation must not retain the frozen include identity.'

    $fixtureRoot = Join-Path $temporaryRoot 'checker-fixtures'
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'pass-reviewed-equal-opaque-hashes' -ExpectedDiagnostic '' -Mutate { param($fixture, $variants) }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-reordered-keys' -ExpectedDiagnostic 'stale or malformed' -Mutate {
        param($fixture, $variants)
        $value = Get-Content (Join-Path $fixture 'raygen-variant-catalog.json') -Raw | ConvertFrom-Json
        [array]::Reverse($value.variants); Write-FixtureJson (Join-Path $fixture 'raygen-variant-catalog.json') $value
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-shared-artifact-path' -ExpectedDiagnostic 'stale or malformed' -Mutate {
        param($fixture, $variants)
        $value = Get-Content (Join-Path $fixture 'raygen-variant-catalog.json') -Raw | ConvertFrom-Json
        $value.variants[1].artifactPath = $value.variants[0].artifactPath; Write-FixtureJson (Join-Path $fixture 'raygen-variant-catalog.json') $value
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-stale-generator-hash' -ExpectedDiagnostic 'stale or malformed' -Mutate {
        param($fixture, $variants)
        $value = Get-Content (Join-Path $fixture 'raygen-variant-catalog.json') -Raw | ConvertFrom-Json
        $value.generator.sha256 = '0' * 64
        Write-FixtureJson (Join-Path $fixture 'raygen-variant-catalog.json') $value
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-numeric-type-drift' -ExpectedDiagnostic 'stale or malformed' -Mutate {
        param($fixture, $variants)
        $value = Get-Content (Join-Path $fixture 'raygen-variant-catalog.json') -Raw | ConvertFrom-Json
        $value.variants[0].bytes = [string]$value.variants[0].bytes
        Write-FixtureJson (Join-Path $fixture 'raygen-variant-catalog.json') $value
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-boolean-type-drift' -ExpectedDiagnostic 'stale or malformed' -Mutate {
        param($fixture, $variants)
        $value = Get-Content (Join-Path $fixture 'raygen-variant-catalog.json') -Raw | ConvertFrom-Json
        $value.variants[0].shippingAllowed = [string]$value.variants[0].shippingAllowed
        Write-FixtureJson (Join-Path $fixture 'raygen-variant-catalog.json') $value
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-comma-bearing-extra-field' -ExpectedDiagnostic 'invalid schema' -Mutate {
        param($fixture, $variants)
        $value = Get-Content (Join-Path $fixture 'raygen-variant-catalog.json') -Raw | ConvertFrom-Json
        $value | Add-Member -NotePropertyName 'extra,field' -NotePropertyValue 'must not collide with ordered fields'
        Write-FixtureJson (Join-Path $fixture 'raygen-variant-catalog.json') $value
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-budget-schema-type-drift' -ExpectedDiagnostic 'budgets must use frozen schema 1' -Mutate {
        param($fixture, $variants)
        $budget = Get-Content (Join-Path $fixture 'raygen-variant-budgets.json') -Raw | ConvertFrom-Json
        $budget.schema = [string]$budget.schema
        Write-FixtureJson (Join-Path $fixture 'raygen-variant-budgets.json') $budget
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-budget-metrics-container-drift' -ExpectedDiagnostic 'unsupported schema or metric set' -Mutate {
        param($fixture, $variants)
        $budget = Get-Content (Join-Path $fixture 'raygen-variant-budgets.json') -Raw | ConvertFrom-Json
        $budget.metrics = @($budget.metrics) -join ','
        Write-FixtureJson (Join-Path $fixture 'raygen-variant-budgets.json') $budget
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-budget-case-drift' -ExpectedDiagnostic 'exact budget invariant changed' -Mutate {
        param($fixture, $variants)
        $budget = Get-Content (Join-Path $fixture 'raygen-variant-budgets.json') -Raw | ConvertFrom-Json
        $budget.budgets[0].exact.instrumentation = 'diagnostic'
        Write-FixtureJson (Join-Path $fixture 'raygen-variant-budgets.json') $budget
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-budget-boolean-type-drift' -ExpectedDiagnostic 'exact budget invariant changed' -Mutate {
        param($fixture, $variants)
        $budget = Get-Content (Join-Path $fixture 'raygen-variant-budgets.json') -Raw | ConvertFrom-Json
        $budget.budgets[0].exact.shippingAllowed = [string]$budget.budgets[0].exact.shippingAllowed
        Write-FixtureJson (Join-Path $fixture 'raygen-variant-budgets.json') $budget
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-budget-integer-type-drift' -ExpectedDiagnostic 'exact budget invariant changed' -Mutate {
        param($fixture, $variants)
        $budget = Get-Content (Join-Path $fixture 'raygen-variant-budgets.json') -Raw | ConvertFrom-Json
        $budget.budgets[0].exact.atomicInstructions = [string]$budget.budgets[0].exact.atomicInstructions
        Write-FixtureJson (Join-Path $fixture 'raygen-variant-budgets.json') $budget
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-stale-source-dependency-toolchain' -ExpectedDiagnostic 'stale or malformed' -Mutate {
        param($fixture, $variants)
        $value = Get-Content (Join-Path $fixture 'raygen-variant-catalog.json') -Raw | ConvertFrom-Json
        $value.authorities.source.sha256 = '1' * 64; $value.variants[0].dependencies[0].sha256 = '2' * 64; $value.toolchain.glslangValidator.version = 'stale'
        Write-FixtureJson (Join-Path $fixture 'raygen-variant-catalog.json') $value
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-missing-stats' -ExpectedDiagnostic 'stale or malformed' -Mutate {
        param($fixture, $variants)
        $value = Get-Content (Join-Path $fixture 'raygen-variant-catalog.json') -Raw | ConvertFrom-Json
        $value.variants[0].PSObject.Properties.Remove('instructions')
        Write-FixtureJson (Join-Path $fixture 'raygen-variant-catalog.json') $value
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-diagnostic-counter-drift' -ExpectedDiagnostic 'stale or malformed' -Mutate {
        param($fixture, $variants)
        $value = Get-Content (Join-Path $fixture 'raygen-variant-catalog.json') -Raw | ConvertFrom-Json
        $value.variants[0].hasDiagnosticsBinding = $false; Write-FixtureJson (Join-Path $fixture 'raygen-variant-catalog.json') $value
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-shipping-atomic-drift' -ExpectedDiagnostic 'stale or malformed' -Mutate {
        param($fixture, $variants)
        $value = Get-Content (Join-Path $fixture 'raygen-variant-catalog.json') -Raw | ConvertFrom-Json
        $shipping = @($value.variants | Where-Object instrumentation -eq 'Shipping')[0]
        $shipping.atomicInstructions = 1
        Write-FixtureJson (Join-Path $fixture 'raygen-variant-catalog.json') $value
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-strategy-drift' -ExpectedDiagnostic 'stale or malformed' -Mutate {
        param($fixture, $variants)
        $value = Get-Content (Join-Path $fixture 'raygen-variant-catalog.json') -Raw | ConvertFrom-Json
        $value.variants[0].strategy = 'LegacyInlined'; Write-FixtureJson (Join-Path $fixture 'raygen-variant-catalog.json') $value
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-over-budget' -ExpectedDiagnostic 'exceeds frozen budget' -Mutate {
        param($fixture, $variants)
        $budget = Get-Content (Join-Path $fixture 'raygen-variant-budgets.json') -Raw | ConvertFrom-Json
        $budget.budgets[0].max.bytes = 1
        Write-FixtureJson (Join-Path $fixture 'raygen-variant-budgets.json') $budget
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-widened-metric-set' -ExpectedDiagnostic 'unsupported schema or metric set' -Mutate {
        param($fixture, $variants)
        $budget = Get-Content (Join-Path $fixture 'raygen-variant-budgets.json') -Raw | ConvertFrom-Json
        $budget.metrics += 'bindingDecorations'; Write-FixtureJson (Join-Path $fixture 'raygen-variant-budgets.json') $budget
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-mutated-include-words' -ExpectedDiagnostic 'include is stale' -Mutate {
        param($fixture, $variants)
        $path = Get-ChildItem -LiteralPath $variants -Filter '*.inc' -File | Select-Object -First 1 -ExpandProperty FullName
        $text = Get-Content $path -Raw; $word = [regex]::Match($text, '0x([0-9a-fA-F])')
        $replacement = if ($word.Groups[1].Value -eq '0') { '1' } else { '0' }
        [IO.File]::WriteAllText($path, $text.Remove($word.Index + 2, 1).Insert($word.Index + 2, $replacement), [Text.UTF8Encoding]::new($false))
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-extra-include' -ExpectedDiagnostic 'unexpected or missing files' -Mutate {
        param($fixture, $variants)
        [IO.File]::WriteAllText((Join-Path $variants 'stale.inc'), '// stale artifact' + "`n", [Text.UTF8Encoding]::new($false))
    }
    New-CatalogFixture -FixtureRoot $fixtureRoot -Name 'reject-bom-prefixed-include' -ExpectedDiagnostic 'must not contain a UTF-8 BOM' -Mutate {
        param($fixture, $variants)
        $path = Get-ChildItem -LiteralPath $variants -Filter '*.inc' -File | Select-Object -First 1 -ExpandProperty FullName
        $bytes = [IO.File]::ReadAllBytes($path)
        $bomBytes = New-Object byte[] ($bytes.Length + 3)
        $bomBytes[0] = 0xef; $bomBytes[1] = 0xbb; $bomBytes[2] = 0xbf
        [Array]::Copy($bytes, 0, $bomBytes, 3, $bytes.Length)
        [IO.File]::WriteAllBytes($path, $bomBytes)
    }

    # CheckCatalog is the sole matrix compilation in this test. It replays all
    # eight compile/preprocess/validate/disassemble paths into temporary storage
    # and compares catalog, include words, raw SPIR-V, budgets, and toolchain.
    & $compiler -CheckCatalog -ArtifactDirectory $artifactDirectory -CatalogPath $catalogPath -BudgetPath $budgetPath `
        -CheckCatalogFixtureRoot $fixtureRoot -TestPublicationFaultAfter 5
    if ($LASTEXITCODE -ne 0) { throw "Frozen catalog check failed with exit code $LASTEXITCODE." }

    $compatibilityGenericOutput = Join-Path $temporaryRoot 'compatibility-generic'
    & $compiler -Check -OutputDirectory $compatibilityGenericOutput
    if ($LASTEXITCODE -ne 0) { throw "Generic compatibility freshness failed with exit code $LASTEXITCODE." }
    Assert-True ((Get-RawFileHash (Join-Path $compatibilityGenericOutput 'minimal.rgen.spv')) -eq
        '03526a113daed58e6b9dc3565e7a0837c2040acdb685339dcd48b9d7219de658') `
        'Compatibility generic SPIR-V words changed.'
    $compatibilityLegacyOutput = Join-Path $temporaryRoot 'compatibility-legacy'
    & $compiler -Legacy -Check -OutputDirectory $compatibilityLegacyOutput
    if ($LASTEXITCODE -ne 0) { throw "Legacy compatibility freshness failed with exit code $LASTEXITCODE." }
    Assert-True ((Get-RawFileHash (Join-Path $compatibilityLegacyOutput 'minimal.legacy.rgen.spv')) -eq
        'dcf51ae9a3a19689d7b71bf0e15cc0a00c07947d1417657301b196926b21665b') `
        'Compatibility legacy SPIR-V words changed.'
    Assert-True ((& git -C $repoRoot status --porcelain) -join "`n" -eq $worktreeStatusBefore) `
        'Temporary artifact compilation modified the worktree.'
    Write-Output 'Raygen variant artifact specialization and compatibility contracts passed.'
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) { Remove-Item -LiteralPath $temporaryRoot -Recurse -Force }
}
