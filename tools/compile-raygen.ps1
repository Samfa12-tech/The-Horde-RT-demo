param(
    [string]$VulkanSdk = $env:VULKAN_SDK,
    [switch]$Check,
    [string]$OutputDirectory,
    [string]$EmbeddedIncludePath,
    [switch]$Legacy,
    [string]$Variant,
    [switch]$Matrix,
    [string]$Strategy,
    [string]$ManifestPath
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($VulkanSdk))
{
    $VulkanSdk = 'C:\VulkanSDK\1.4.350.0'
}

$validator = Join-Path $VulkanSdk 'Bin\glslangValidator.exe'
if (-not (Test-Path -LiteralPath $validator))
{
    throw "glslangValidator was not found at $validator"
}

$disassembler = Join-Path $VulkanSdk 'Bin\spirv-dis.exe'
if (-not (Test-Path -LiteralPath $disassembler))
{
    throw "spirv-dis was not found at $disassembler"
}
$optimizer = Join-Path $VulkanSdk 'Bin\spirv-opt.exe'
if (-not (Test-Path -LiteralPath $optimizer))
{
    throw "spirv-opt was not found at $optimizer"
}
$source = Join-Path $repoRoot 'shaders\raytracing\minimal.rgen'
$includeName = if ($Legacy) { 'MinimalLegacyRayGenShader.inc' } else { 'MinimalRayGenShader.inc' }
$include = Join-Path $repoRoot (Join-Path 'src\vulkan\raytracing' $includeName)
$variantName = if ($Legacy) { 'legacy' } else { 'generic' }
$outputStem = if ($Legacy) { 'minimal.legacy.rgen' } else { 'minimal.rgen' }

function Resolve-RaygenIncludes
{
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.HashSet[string]]$ActivePaths,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Dependencies
    )

    $fullPath = [IO.Path]::GetFullPath($Path)
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf))
    {
        throw "Raygen source dependency was not found: $fullPath"
    }
    if (-not $ActivePaths.Add($fullPath))
    {
        throw "Raygen source include cycle detected at $fullPath"
    }

    $Dependencies.Add($fullPath)

    $output = [Text.StringBuilder]::new()
    $directory = Split-Path -Parent $fullPath
    foreach ($line in [IO.File]::ReadAllLines($fullPath))
    {
        if ($line -match '^\s*#include\s+"([^"]+)"\s*$')
        {
            [void]$output.Append((Resolve-RaygenIncludes -Path (Join-Path $directory $Matches[1]) -ActivePaths $ActivePaths -Dependencies $Dependencies))
            continue
        }
        [void]$output.AppendLine($line)
    }

    [void]$ActivePaths.Remove($fullPath)
    return $output.ToString()
}

function Get-RaygenDependencyHash
{
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    # Resolve-RaygenIncludes compiles logical text lines, so checkout-specific
    # CRLF/LF bytes must not make an otherwise identical embedded shader stale.
    $lines = [IO.File]::ReadAllLines($Path)
    $canonicalText = if ($lines.Count -eq 0) {
        ''
    } else {
        [string]::Join("`n", $lines) + "`n"
    }
    return Get-RaygenSha256 -Text $canonicalText
}

function Get-RaygenSha256
{
    param([string]$Text)

    # Windows PowerShell 5.1 uses .NET Framework, where SHA256.HashData and
    # Convert.ToHexString are unavailable. Keep the dependency identity stable
    # across both engines with the older instance API.
    $sha256 = [Security.Cryptography.SHA256]::Create()
    try
    {
        return ([BitConverter]::ToString($sha256.ComputeHash(
            [Text.Encoding]::UTF8.GetBytes($Text)))).Replace('-', '').ToLowerInvariant()
    }
    finally
    {
        $sha256.Dispose()
    }
}

function Get-RaygenRelativePath
{
    param([string]$Path)

    $fullPath = [IO.Path]::GetFullPath($Path)
    $repoPrefix = $repoRoot.TrimEnd('\') + '\'
    if (-not $fullPath.StartsWith($repoPrefix, [StringComparison]::OrdinalIgnoreCase))
    {
        throw "Raygen dependency is outside the repository: $fullPath"
    }
    return $fullPath.Substring($repoPrefix.Length).Replace('\', '/')
}

function Get-RaygenFileSha256
{
    param([string]$Path)

    $sha256 = [Security.Cryptography.SHA256]::Create()
    try
    {
        return ([BitConverter]::ToString($sha256.ComputeHash([IO.File]::ReadAllBytes($Path)))).Replace('-', '').ToLowerInvariant()
    }
    finally
    {
        $sha256.Dispose()
    }
}

function Test-RaygenExactString
{
    param([string]$Left, [string]$Right)

    return $null -ne $Left -and $null -ne $Right -and
        [string]::Equals($Left, $Right, [StringComparison]::Ordinal)
}

function Test-RaygenExactOneOf
{
    param([string]$Value, [string[]]$ExpectedValues)

    foreach ($expectedValue in $ExpectedValues)
    {
        if (Test-RaygenExactString -Left $Value -Right $expectedValue)
        {
            return $true
        }
    }
    return $false
}

function Get-RaygenVariantManifest
{
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path))
    {
        $Path = Join-Path $repoRoot 'tools\raygen-variants.json'
    }
    $fullPath = [IO.Path]::GetFullPath($Path)
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf))
    {
        throw "Raygen variant manifest was not found: $fullPath"
    }
    $manifest = Get-Content -LiteralPath $fullPath -Raw | ConvertFrom-Json
    if ($manifest.schema -ne 1 -or $null -eq $manifest.variants)
    {
        throw 'Raygen variant manifest must use schema 1 and contain variants.'
    }
    if (-not (Test-RaygenExactString -Left (@($manifest.PSObject.Properties.Name | Sort-Object) -join ',') -Right 'schema,variants'))
    {
        throw 'Raygen variant manifest has unsupported top-level fields.'
    }
    $expected = @()
    foreach ($instrumentation in @('Shipping', 'Diagnostic'))
    {
        foreach ($quality in @('Mobile', 'High'))
        {
            foreach ($material in @('OpaqueFast', 'GenericDielectric'))
            {
                $materialName = if ($material -eq 'OpaqueFast') { 'opaque_fast' } else { 'generic_dielectric' }
                $expected += [pscustomobject]@{
                    name = ('{0}_{1}_{2}' -f $instrumentation.ToLowerInvariant(), $quality.ToLowerInvariant(), $materialName)
                    instrumentation = $instrumentation
                    quality = $quality
                    material = $material
                    strategy = if ($material -eq 'OpaqueFast') { 'LegacyInlined' } else { 'GenericRetained' }
                    shippingAllowed = Test-RaygenExactString -Left $instrumentation -Right 'Shipping'
                }
            }
        }
    }
    $variants = @($manifest.variants)
    if ($variants.Count -ne $expected.Count)
    {
        throw 'Raygen variant manifest must contain exactly eight entries.'
    }
    $seenNames = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    $seenCombinations = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach ($variantDefinition in $variants)
    {
        $entryPropertyNames = @($variantDefinition.PSObject.Properties.Name | Sort-Object) -join ','
        if (-not (Test-RaygenExactString -Left $entryPropertyNames -Right 'instrumentation,material,name,quality,shippingAllowed,strategy'))
        {
            throw 'Raygen variant manifest entry has unsupported or missing fields.'
        }
        if (-not (Test-RaygenExactOneOf -Value $variantDefinition.instrumentation -ExpectedValues @('Shipping', 'Diagnostic')) -or
            -not (Test-RaygenExactOneOf -Value $variantDefinition.quality -ExpectedValues @('Mobile', 'High')) -or
            -not (Test-RaygenExactOneOf -Value $variantDefinition.material -ExpectedValues @('OpaqueFast', 'GenericDielectric')) -or
            -not (Test-RaygenExactOneOf -Value $variantDefinition.strategy -ExpectedValues @('GenericRetained', 'LegacyInlined')) -or
            $variantDefinition.shippingAllowed -isnot [bool])
        {
            throw "Raygen variant manifest entry is invalid: $($variantDefinition.name)"
        }
        if (-not $seenNames.Add([string]$variantDefinition.name))
        {
            throw "Raygen variant manifest has duplicate name: $($variantDefinition.name)"
        }
        $combination = "$($variantDefinition.instrumentation)|$($variantDefinition.quality)|$($variantDefinition.material)"
        if (-not $seenCombinations.Add($combination))
        {
            throw "Raygen variant manifest has duplicate combination: $combination"
        }
        $matchingExpected = @($expected | Where-Object {
            (Test-RaygenExactString -Left $_.name -Right $variantDefinition.name) -and
            (Test-RaygenExactString -Left $_.instrumentation -Right $variantDefinition.instrumentation) -and
            (Test-RaygenExactString -Left $_.quality -Right $variantDefinition.quality) -and
            (Test-RaygenExactString -Left $_.material -Right $variantDefinition.material) -and
            (Test-RaygenExactString -Left $_.strategy -Right $variantDefinition.strategy) -and
            $_.shippingAllowed -eq $variantDefinition.shippingAllowed
        })
        if ($matchingExpected.Count -ne 1)
        {
            throw "Raygen variant manifest entry is not part of the approved matrix: $($variantDefinition.name)"
        }
    }
    $canonicalLines = @('schema=1') + @($variants | Sort-Object name | ForEach-Object {
        '{0}|{1}|{2}|{3}|{4}|{5}' -f $_.name, $_.instrumentation, $_.quality,
            $_.material, $_.strategy, $_.shippingAllowed.ToString().ToLowerInvariant()
    })
    $canonicalManifest = [string]::Join("`n", $canonicalLines) + "`n"
    return [pscustomobject]@{
        Path = $fullPath
        Variants = @($variants | Sort-Object name)
        Sha256 = Get-RaygenSha256 -Text $canonicalManifest
    }
}

function Get-RaygenVariantOutputRoot
{
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or -not [IO.Path]::IsPathRooted($Path))
    {
        throw 'Variant compilation requires an absolute, caller-supplied output directory.'
    }
    $fullPath = [IO.Path]::GetFullPath($Path)
    $repoPrefix = $repoRoot.TrimEnd('\') + '\'
    $ignoredArtifactRoots = @(
        (Join-Path $repoRoot 'reports'),
        (Join-Path $repoRoot 'build'),
        (Join-Path $repoRoot 'build-current'),
        (Join-Path $repoRoot '.superpowers\sdd')
    )
    $insideRepository = $fullPath.Equals($repoRoot, [StringComparison]::OrdinalIgnoreCase) -or
        $fullPath.StartsWith($repoPrefix, [StringComparison]::OrdinalIgnoreCase)
    $insideIgnoredArtifactRoot = @($ignoredArtifactRoots | Where-Object {
        $ignoredPrefix = $_.TrimEnd('\') + '\'
        $fullPath.StartsWith($ignoredPrefix, [StringComparison]::OrdinalIgnoreCase)
    }).Count -gt 0
    # Temporary variants may be retained under existing ignored evidence roots,
    # but never in a tracked source/output location. Reviewed generated includes
    # stay a later task with a separate write path.
    if ($insideRepository -and -not $insideIgnoredArtifactRoot)
    {
        throw 'Variant compilation output directory must be outside the repository or under an approved ignored evidence root.'
    }
    if (Test-Path -LiteralPath $fullPath)
    {
        throw "Variant compilation output directory must not already exist: $fullPath"
    }
    return $fullPath
}

function Write-RaygenVariantSource
{
    param(
        [string]$ResolvedSourcePath,
        [pscustomobject]$VariantDefinition,
        [string]$ConfigPath
    )

    $values = @{
        Shipping = 0; Diagnostic = 1; Mobile = 0; High = 1
        OpaqueFast = 0; GenericDielectric = 1
    }
    $sourceLines = [IO.File]::ReadAllLines($ResolvedSourcePath)
    $insertionIndex = 0
    while ($insertionIndex -lt $sourceLines.Count -and
        ($sourceLines[$insertionIndex] -match '^\s*$' -or
         $sourceLines[$insertionIndex] -match '^\s*#(?:version|extension)\b'))
    {
        $insertionIndex++
    }
    $prefix = @($sourceLines[0..($insertionIndex - 1)])
    $suffix = @($sourceLines[$insertionIndex..($sourceLines.Count - 1)])
    $definitions = @(
        '// Temporary raygen variant configuration injected by compile-raygen.ps1.',
        ('#define HORDE_RT_VARIANT_INSTRUMENTATION {0}' -f $values[$VariantDefinition.instrumentation]),
        ('#define HORDE_RT_VARIANT_QUALITY {0}' -f $values[$VariantDefinition.quality]),
        ('#define HORDE_RT_VARIANT_MATERIAL {0}' -f $values[$VariantDefinition.material]),
        [IO.File]::ReadAllText($ConfigPath).TrimEnd("`r", "`n")
    )
    [IO.File]::WriteAllText($ResolvedSourcePath,
        ([string]::Join("`n", @($prefix + $definitions + $suffix)) + "`n"),
        [Text.UTF8Encoding]::new($false))
}

function Invoke-RaygenVariantCompilation
{
    param(
        [pscustomobject]$VariantDefinition,
        [string]$OutputRoot,
        [pscustomobject]$Manifest
    )

    $variantOutput = Join-Path $OutputRoot $VariantDefinition.name
    New-Item -ItemType Directory -Path $variantOutput | Out-Null
    $spirv = Join-Path $variantOutput 'minimal.rgen.spv'
    $disassembly = Join-Path $variantOutput 'minimal.rgen.spvasm'
    $resolvedSourcePath = Join-Path $variantOutput 'minimal.rgen.resolved'
    $preprocessedSourcePath = Join-Path $variantOutput 'minimal.rgen.preprocessed'
    $activePaths = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $dependencies = [Collections.Generic.List[string]]::new()
    [IO.File]::WriteAllText($resolvedSourcePath,
        (Resolve-RaygenIncludes -Path $source -ActivePaths $activePaths -Dependencies $dependencies),
        [Text.UTF8Encoding]::new($false))

    $configPath = Join-Path $repoRoot 'shaders\raytracing\include\rt_variant_config.glsl'
    if (-not (Test-Path -LiteralPath $configPath -PathType Leaf))
    {
        throw "Raygen variant config was not found: $configPath"
    }
    Write-RaygenVariantSource -ResolvedSourcePath $resolvedSourcePath -VariantDefinition $VariantDefinition -ConfigPath $configPath
    $legacyStrategy = Test-RaygenExactString -Left $VariantDefinition.strategy -Right 'LegacyInlined'
    if ($legacyStrategy)
    {
        $resolvedLegacySource = [IO.File]::ReadAllText($resolvedSourcePath)
        $activityExpression = '#define HORDE_GENERIC_TRANSMISSION_VARIANT 1'
        $replacementCount = ([regex]::Matches($resolvedLegacySource, [regex]::Escape($activityExpression))).Count
        if ($replacementCount -ne 1)
        {
            throw "Legacy raygen expected one generic transmission variant definition, found $replacementCount."
        }
        [IO.File]::WriteAllText($resolvedSourcePath,
            $resolvedLegacySource.Replace($activityExpression, '#define HORDE_GENERIC_TRANSMISSION_VARIANT 0'),
            [Text.UTF8Encoding]::new($false))
    }

    # Keep the compiler's exact macro-expanded source beside the temporary
    # matrix evidence. This is intentionally an ignored artifact: the
    # disassembly can legitimately retire whole specialized routes, while the
    # preprocessed source proves each owning route received its Mobile/High
    # literal budget before dead-code elimination.
    & $validator -E -S rgen $resolvedSourcePath | Set-Content -LiteralPath $preprocessedSourcePath -Encoding utf8
    if ($LASTEXITCODE -ne 0) { throw "Raygen variant preprocessing failed with exit code $LASTEXITCODE." }

    $dependencyHashes = @($dependencies | ForEach-Object {
        [ordered]@{ path = Get-RaygenRelativePath -Path $_; sha256 = Get-RaygenDependencyHash -Path $_ }
    })
    $dependencyHashes += [ordered]@{ path = 'shaders/raytracing/include/rt_variant_config.glsl'; sha256 = Get-RaygenDependencyHash -Path $configPath }
    $dependencyManifest = [string]::Join(';', @($dependencyHashes | ForEach-Object { "$($_.path):$($_.sha256)" }))
    $dependencyIdentity = ('key={0}|instrumentation={1}|quality={2}|material={3}|strategy={4}|target=vulkan1.2|manifest={5}|dependencies={6}' -f
        $VariantDefinition.name, $VariantDefinition.instrumentation, $VariantDefinition.quality,
        $VariantDefinition.material, $VariantDefinition.strategy, $Manifest.Sha256, $dependencyManifest)
    $dependencyHash = Get-RaygenSha256 -Text $dependencyIdentity

    $validatorArguments = @('-V', '--target-env', 'vulkan1.2')
    if ($legacyStrategy) { $validatorArguments += '-Os' }
    $validatorArguments += @('-S', 'rgen', '-o', $spirv, $resolvedSourcePath)
    & $validator @validatorArguments
    if ($LASTEXITCODE -ne 0) { throw "Raygen variant compilation failed with exit code $LASTEXITCODE." }
    $optimizedSpirv = "$spirv.optimized"
    if ($legacyStrategy)
    {
        & $optimizer -O $spirv -o $optimizedSpirv
    }
    else
    {
        & $optimizer --eliminate-dead-functions --eliminate-dead-code-aggressive `
            --simplify-instructions --eliminate-dead-branches --cfg-cleanup $spirv -o $optimizedSpirv
    }
    if ($LASTEXITCODE -ne 0) { throw "Raygen variant optimization failed with exit code $LASTEXITCODE." }
    [IO.File]::Copy($optimizedSpirv, $spirv, $true)
    Remove-Item -LiteralPath $optimizedSpirv -Force
    $spirvValidator = Join-Path $VulkanSdk 'Bin\spirv-val.exe'
    if (-not (Test-Path -LiteralPath $spirvValidator)) { throw "spirv-val was not found at $spirvValidator" }
    & $spirvValidator --target-env vulkan1.2 $spirv
    if ($LASTEXITCODE -ne 0) { throw "Raygen variant SPIR-V validation failed with exit code $LASTEXITCODE." }
    & $disassembler $spirv -o $disassembly
    if ($LASTEXITCODE -ne 0) { throw "Raygen variant disassembly failed with exit code $LASTEXITCODE." }

    $bytes = [IO.File]::ReadAllBytes($spirv)
    if (($bytes.Length % 4) -ne 0) { throw 'Compiled variant SPIR-V is not 32-bit word aligned.' }
    $assemblyLines = @(Get-Content -LiteralPath $disassembly)
    $instructionCount = @($assemblyLines | Where-Object { $_ -match '^\s*(?:%\S+\s*=\s*)?Op\w+' }).Count
    $branchOperationCount = @($assemblyLines | Where-Object { $_ -match '\bOp(?:Branch|BranchConditional|Switch)\b' }).Count
    $loopCount = @($assemblyLines | Where-Object { $_ -match '\bOpLoopMerge\b' }).Count
    $selectionMergeCount = @($assemblyLines | Where-Object { $_ -match '\bOpSelectionMerge\b' }).Count
    $functionCount = @($assemblyLines | Where-Object { $_ -match '\bOpFunction\b' }).Count
    $functionCallCount = @($assemblyLines | Where-Object { $_ -match '\bOpFunctionCall\b' }).Count
    $rayQueryInitializationCount = @($assemblyLines | Where-Object { $_ -match '\bOpRayQueryInitializeKHR\b' }).Count
    $atomicInstructionCount = @($assemblyLines | Where-Object { $_ -match '\bOpAtomic\w+\b' }).Count
    $hasDiagnosticsBinding = @($assemblyLines | Where-Object { $_ -match '\bOpDecorate\s+%\S+\s+Binding\s+22\b' }).Count -gt 0
    $driverSafeFullyInlined = $functionCount -eq 1 -and $functionCallCount -eq 0 -and $rayQueryInitializationCount -le 29
    $boundedGenericFunctionsRetained = $functionCount -gt 1 -and $functionCallCount -gt 0 -and $rayQueryInitializationCount -le 3
    if ($legacyStrategy -and -not $driverSafeFullyInlined)
    {
        throw "Variant $($VariantDefinition.name) lost the fully inlined legacy strategy shape."
    }
    if (-not $legacyStrategy -and -not $boundedGenericFunctionsRetained)
    {
        throw "Variant $($VariantDefinition.name) lost the retained generic strategy shape."
    }
    $stats = [ordered]@{
        schema = 1; key = $VariantDefinition.name; instrumentation = $VariantDefinition.instrumentation
        quality = $VariantDefinition.quality; material = $VariantDefinition.material; strategy = $VariantDefinition.strategy
        targetEnv = 'vulkan1.2'; manifestSha256 = $Manifest.Sha256; dependencySha256 = $dependencyHash
        dependencies = $dependencyHashes; compiledSpirvSha256 = Get-RaygenFileSha256 -Path $spirv
        bytes = $bytes.Length; words = $bytes.Length / 4; instructions = $instructionCount
        branchOperations = $branchOperationCount; loops = $loopCount; selectionMerges = $selectionMergeCount
        functions = $functionCount; functionCalls = $functionCallCount; rayQueryInitializations = $rayQueryInitializationCount
        atomicInstructions = $atomicInstructionCount; hasDiagnosticsBinding = $hasDiagnosticsBinding
        driverSafeFullyInlined = $driverSafeFullyInlined; boundedGenericFunctionsRetained = $boundedGenericFunctionsRetained
    }
    [IO.File]::WriteAllText((Join-Path $variantOutput 'raygen-stats.json'),
        ($stats | ConvertTo-Json -Depth 4), [Text.UTF8Encoding]::new($false))
    Write-Output ("Raygen variant {0}: {1} bytes; functions={2}; calls={3}; ray-query sites={4}; dependency SHA-256={5}" -f
        $VariantDefinition.name, $bytes.Length, $functionCount, $functionCallCount, $rayQueryInitializationCount, $dependencyHash)
}

$variantMode = $Matrix -or -not [string]::IsNullOrWhiteSpace($Variant)
if ($variantMode)
{
    if ($Check -or $Legacy -or -not [string]::IsNullOrWhiteSpace($EmbeddedIncludePath) -or ($Matrix -and -not [string]::IsNullOrWhiteSpace($Variant)))
    {
        throw 'Variant and matrix compilation modes cannot be combined with legacy/check generation modes.'
    }
    if ($Matrix -and -not [string]::IsNullOrWhiteSpace($Strategy))
    {
        throw 'Matrix compilation uses the manifest strategy for every key; do not pass -Strategy.'
    }
    $matrixManifest = Get-RaygenVariantManifest -Path $ManifestPath
    $matrixOutput = Get-RaygenVariantOutputRoot -Path $OutputDirectory
    $selectedVariants = if ($Matrix) { $matrixManifest.Variants } else {
        $match = @($matrixManifest.Variants | Where-Object { Test-RaygenExactString -Left $_.name -Right $Variant })
        if ($match.Count -ne 1) { throw "Unknown raygen variant key: $Variant" }
        if (-not [string]::IsNullOrWhiteSpace($Strategy) -and
            -not (Test-RaygenExactOneOf -Value $Strategy -ExpectedValues @('GenericRetained', 'LegacyInlined')))
        {
            throw "Unknown compiler strategy: $Strategy"
        }
        if (-not [string]::IsNullOrWhiteSpace($Strategy) -and
            -not (Test-RaygenExactString -Left $Strategy -Right $match[0].strategy))
        {
            throw "Requested strategy $Strategy contradicts manifest strategy $($match[0].strategy) for $Variant."
        }
        $match
    }
    New-Item -ItemType Directory -Path $matrixOutput | Out-Null
    foreach ($variantDefinition in $selectedVariants)
    {
        Invoke-RaygenVariantCompilation -VariantDefinition $variantDefinition -OutputRoot $matrixOutput -Manifest $matrixManifest
    }
    return
}
if (-not [string]::IsNullOrWhiteSpace($ManifestPath) -or -not [string]::IsNullOrWhiteSpace($Strategy))
{
    throw '-ManifestPath and -Strategy are only supported by variant or matrix compilation modes.'
}

$hasIncludeOverride = -not [string]::IsNullOrWhiteSpace($EmbeddedIncludePath)
if ($hasIncludeOverride -and -not $Check)
{
    throw '-EmbeddedIncludePath is a read-only test override and is only supported with -Check.'
}
if ($hasIncludeOverride)
{
    $include = [IO.Path]::GetFullPath($EmbeddedIncludePath)
    if (-not (Test-Path -LiteralPath $include -PathType Leaf))
    {
        throw "Embedded include override was not found: $include"
    }
}
$temporaryOutput = $false
if ($Check)
{
    if ([string]::IsNullOrWhiteSpace($OutputDirectory))
    {
        $OutputDirectory = Join-Path ([IO.Path]::GetTempPath()) ("horde-raygen-check-{0}" -f [guid]::NewGuid().ToString('N'))
        $temporaryOutput = $true
    }
    $outputFull = [IO.Path]::GetFullPath($OutputDirectory)
    New-Item -ItemType Directory -Force -Path $outputFull | Out-Null
    $spirv = Join-Path $outputFull "$outputStem.spv"
    $disassembly = Join-Path $outputFull "$outputStem.spvasm"
}
else
{
    if (-not [string]::IsNullOrWhiteSpace($OutputDirectory))
    {
        throw '-OutputDirectory is only supported with -Check.'
    }
    $spirv = Join-Path $repoRoot ("shaders\raytracing\$outputStem.spv")
    $disassembly = Join-Path ([IO.Path]::GetTempPath()) ("horde-raygen-{0}.spvasm" -f [guid]::NewGuid().ToString('N'))
}
$resolvedSourcePath = Join-Path (Split-Path -Parent $spirv) "$outputStem.resolved"
$activePaths = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$dependencies = [System.Collections.Generic.List[string]]::new()
[IO.File]::WriteAllText(
    $resolvedSourcePath,
    (Resolve-RaygenIncludes -Path $source -ActivePaths $activePaths -Dependencies $dependencies),
    [Text.UTF8Encoding]::new($false))
if ($Legacy)
{
    $resolvedLegacySource = [IO.File]::ReadAllText($resolvedSourcePath)
    $activityExpression = '#define HORDE_GENERIC_TRANSMISSION_VARIANT 1'
    $replacementCount = ([regex]::Matches(
        $resolvedLegacySource, [regex]::Escape($activityExpression))).Count
    if ($replacementCount -ne 1)
    {
        throw "Legacy raygen expected one generic transmission variant definition, found $replacementCount."
    }
    $resolvedLegacySource = $resolvedLegacySource.Replace(
        $activityExpression, '#define HORDE_GENERIC_TRANSMISSION_VARIANT 0')
    [IO.File]::WriteAllText(
        $resolvedSourcePath, $resolvedLegacySource, [Text.UTF8Encoding]::new($false))
}
$dependencyHashes = foreach ($dependency in $dependencies)
{
    [ordered]@{
        path = Get-RaygenRelativePath -Path $dependency
        sha256 = Get-RaygenDependencyHash -Path $dependency
    }
}
$dependencyManifest = $dependencyHashes | ConvertTo-Json -Compress
$dependencyIdentity = "$variantName|$dependencyManifest"
$dependencyHash = Get-RaygenSha256 -Text $dependencyIdentity

$validatorArguments = @('-V', '--target-env', 'vulkan1.2')
if ($Legacy)
{
    $validatorArguments += '-Os'
}
$validatorArguments += @('-S', 'rgen', '-o', $spirv, $resolvedSourcePath)
& $validator @validatorArguments
if ($LASTEXITCODE -ne 0)
{
    throw "Raygen compilation failed with exit code $LASTEXITCODE"
}

# The generic dielectric path deliberately retains GLSL helper boundaries.
# Exact SM-S948B validation showed glslang's fully inlined generic module
# duplicated bounded ray-query control flow and cost substantially more than
# this function-retaining form. The legacy no-glass shader keeps its previously
# validated fully inlined shape.
$optimizedSpirv = "$spirv.optimized"
if (-not $Legacy)
{
    & $optimizer --eliminate-dead-functions `
        --eliminate-dead-code-aggressive `
        --simplify-instructions `
        --eliminate-dead-branches `
        --cfg-cleanup $spirv -o $optimizedSpirv
}
else
{
    & $optimizer -O $spirv -o $optimizedSpirv
}
if ($LASTEXITCODE -ne 0)
{
    throw "Raygen SPIR-V optimization failed with exit code $LASTEXITCODE"
}
[IO.File]::Copy($optimizedSpirv, $spirv, $true)
Remove-Item -LiteralPath $optimizedSpirv -Force

$bytes = [System.IO.File]::ReadAllBytes($spirv)
if (($bytes.Length % 4) -ne 0)
{
    throw 'Compiled SPIR-V is not 32-bit word aligned.'
}

$wordValues = for ($offset = 0; $offset -lt $bytes.Length; $offset += 4)
{
    [BitConverter]::ToUInt32($bytes, $offset)
}
$words = foreach ($word in $wordValues)
{
    '0x{0:x8}u' -f $word
}

& $disassembler $spirv -o $disassembly
if ($LASTEXITCODE -ne 0)
{
    throw "SPIR-V disassembly failed with exit code $LASTEXITCODE"
}
$assemblyLines = @(Get-Content -LiteralPath $disassembly)
$instructionCount = @($assemblyLines | Where-Object { $_ -match '^\s*(?:%\S+\s*=\s*)?Op\w+' }).Count
$branchOperationCount = @($assemblyLines | Where-Object { $_ -match '\bOp(?:Branch|BranchConditional|Switch)\b' }).Count
$loopCount = @($assemblyLines | Where-Object { $_ -match '\bOpLoopMerge\b' }).Count
$selectionMergeCount = @($assemblyLines | Where-Object { $_ -match '\bOpSelectionMerge\b' }).Count
$functionCount = @($assemblyLines | Where-Object { $_ -match '\bOpFunction\b' }).Count
$functionCallCount = @($assemblyLines | Where-Object { $_ -match '\bOpFunctionCall\b' }).Count
$rayQueryInitializationCount = @($assemblyLines | Where-Object {
    $_ -match '\bOpRayQueryInitializeKHR\b'
}).Count
$driverSafeFullyInlined = $functionCount -eq 1 -and
    $functionCallCount -eq 0 -and $rayQueryInitializationCount -le 29
$boundedGenericFunctionsRetained = $functionCount -gt 1 -and
    $functionCallCount -gt 0 -and $rayQueryInitializationCount -le 3
if ($Legacy -and -not $driverSafeFullyInlined)
{
    throw ("Raygen optimizer cloned bounded traversal: functions={0}, calls={1}, ray-query sites={2}." -f `
        $functionCount, $functionCallCount, $rayQueryInitializationCount)
}
if (-not $Legacy -and -not $boundedGenericFunctionsRetained)
{
    throw ("Generic raygen lost its measured bounded function strategy: functions={0}, calls={1}, ray-query sites={2}." -f `
        $functionCount, $functionCallCount, $rayQueryInitializationCount)
}
$spirvHash = Get-RaygenFileSha256 -Path $spirv
$sourceHash = $dependencyHash
$includeHash = if (Test-Path -LiteralPath $include) {
    # Generated text is logically line-oriented. Preserve this identity across
    # CRLF/LF checkouts while compiled .spv payloads above stay raw-byte hashes.
    Get-RaygenDependencyHash -Path $include
} else { '' }

if ($Check)
{
    $includeText = Get-Content -LiteralPath $include -Raw
    $embeddedDependencyHash = [regex]::Match(
        $includeText,
        '^// Raygen dependency SHA-256: ([0-9a-f]{64})\s*$',
        [Text.RegularExpressions.RegexOptions]::Multiline).Groups[1].Value
    if ([string]::IsNullOrWhiteSpace($embeddedDependencyHash))
    {
        throw "Embedded raygen include has no dependency hash: $include"
    }
    $includeMatches = [regex]::Matches($includeText, '0x([0-9a-fA-F]{8})u')
    if ($includeMatches.Count -eq 0)
    {
        throw "Embedded raygen include contains no SPIR-V words: $include"
    }
    $embeddedWords = foreach ($match in $includeMatches)
    {
        [Convert]::ToUInt32($match.Groups[1].Value, 16)
    }
    $matchesEmbedded = $embeddedDependencyHash -eq $dependencyHash -and $embeddedWords.Count -eq $wordValues.Count
    if ($matchesEmbedded)
    {
        for ($index = 0; $index -lt $wordValues.Count; $index++)
        {
            if ($embeddedWords[$index] -ne $wordValues[$index])
            {
                $matchesEmbedded = $false
                break
            }
        }
    }

    $stats = [ordered]@{
        source = $source
        variant = $variantName
        sourceSha256 = $sourceHash
        dependencies = $dependencyHashes
        dependencySha256 = $dependencyHash
        compiledSpirv = $spirv
        compiledSpirvSha256 = $spirvHash
        embeddedInclude = $include
        embeddedIncludeSha256 = $includeHash
        bytes = $bytes.Length
        words = $wordValues.Count
        instructions = $instructionCount
        branchOperations = $branchOperationCount
        loops = $loopCount
        selectionMerges = $selectionMergeCount
        functions = $functionCount
        functionCalls = $functionCallCount
        rayQueryInitializations = $rayQueryInitializationCount
        driverSafeFullyInlined = $driverSafeFullyInlined
        boundedGenericFunctionsRetained = $boundedGenericFunctionsRetained
        matchesEmbeddedWords = $matchesEmbedded
    }
    $statsPath = Join-Path $outputFull 'raygen-stats.json'
    [IO.File]::WriteAllText(
        $statsPath,
        ($stats | ConvertTo-Json),
        [Text.UTF8Encoding]::new($false))
    Write-Output ("Raygen check artifacts: {0}" -f $outputFull)
    Write-Output ("Source dependency SHA-256: {0}; embedded include SHA-256: {1}" -f $sourceHash, $includeHash)
    Write-Output ("SPIR-V bytes: {0}; words: {1}; instructions: {2}; branch operations: {3}; loops: {4}; selection merges: {5}; SHA-256: {6}" -f $bytes.Length, $wordValues.Count, $instructionCount, $branchOperationCount, $loopCount, $selectionMergeCount, $spirvHash)
    Write-Output ("Raygen optimizer shape: functions={0}; calls={1}; ray-query sites={2}" -f `
        $functionCount, $functionCallCount, $rayQueryInitializationCount)
    if (-not $matchesEmbedded)
    {
        throw 'Embedded raygen SPIR-V is stale. Run tools\compile-raygen.ps1 and rebuild.'
    }
    Write-Output 'Embedded raygen SPIR-V words match the compiled shader.'
    if (-not $Legacy)
    {
        $legacyOutput = Join-Path $outputFull 'legacy'
        & $PSCommandPath -VulkanSdk $VulkanSdk -Check -OutputDirectory $legacyOutput -Legacy
        if ($LASTEXITCODE -ne 0)
        {
            throw "Legacy raygen check failed with exit code $LASTEXITCODE."
        }
    }
    if ($temporaryOutput)
    {
        Remove-Item -LiteralPath $outputFull -Recurse -Force
    }
    return
}

$lines = for ($index = 0; $index -lt $words.Count; $index += 8)
{
    $last = [Math]::Min($index + 7, $words.Count - 1)
    '    ' + (($words[$index..$last] -join ', ') + ',')
}

[System.IO.File]::WriteAllText(
    $include,
    "// Raygen dependency SHA-256: $dependencyHash`n" + ($lines -join "`n") + "`n",
    [System.Text.UTF8Encoding]::new($false))

if (Test-Path -LiteralPath $disassembly)
{
    Remove-Item -LiteralPath $disassembly -Force
}
if (Test-Path -LiteralPath $resolvedSourcePath)
{
    Remove-Item -LiteralPath $resolvedSourcePath -Force
}
Write-Output "Generated $include"
Write-Output ("Source dependency SHA-256: {0}; embedded include SHA-256 before generation: {1}" -f $sourceHash, $includeHash)
Write-Output ("SPIR-V bytes: {0}; words: {1}; instructions: {2}; branch operations: {3}; loops: {4}; selection merges: {5}; SHA-256: {6}" -f $bytes.Length, $wordValues.Count, $instructionCount, $branchOperationCount, $loopCount, $selectionMergeCount, $spirvHash)
Write-Output ("Raygen optimizer shape: functions={0}; calls={1}; ray-query sites={2}" -f `
    $functionCount, $functionCallCount, $rayQueryInitializationCount)
if (-not $Legacy)
{
    & $PSCommandPath -VulkanSdk $VulkanSdk -Legacy
    if ($LASTEXITCODE -ne 0)
    {
        throw "Legacy raygen generation failed with exit code $LASTEXITCODE."
    }
}
