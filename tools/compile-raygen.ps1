param(
    [string]$VulkanSdk = $env:VULKAN_SDK,
    [switch]$Check,
    [string]$OutputDirectory,
    [string]$EmbeddedIncludePath,
    [switch]$Legacy,
    [string]$Variant,
    [switch]$Matrix,
    [string]$Strategy,
    [string]$ManifestPath,
    [switch]$Freeze,
    [switch]$CheckCatalog,
    [string]$ArtifactDirectory,
    [string]$CatalogPath,
    [string]$BudgetPath
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

function Get-RaygenCanonicalText
{
    param([string]$Path)

    $lines = [IO.File]::ReadAllLines($Path)
    if ($lines.Count -eq 0) { return '' }
    return [string]::Join("`n", $lines) + "`n"
}

function Write-RaygenCanonicalText
{
    param([string]$Path, [string]$Text)

    [IO.File]::WriteAllText($Path, $Text.Replace("`r`n", "`n").Replace("`r", "`n"), [Text.UTF8Encoding]::new($false))
}

function Get-RaygenToolVersion
{
    param([string]$Tool)

    # SPIRV-Tools writes its version to stderr on Windows. Run it through cmd
    # so PowerShell 5.1 receives a normal combined stdout stream rather than a
    # NativeCommandError under this script's fail-fast preference.
    $commandLine = '"' + $Tool + '" --version 2>&1'
    $versionLines = @(& $env:ComSpec /d /c $commandLine | ForEach-Object { ([string]$_).Trim() } | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($LASTEXITCODE -ne 0 -or $versionLines.Count -eq 0)
    {
        throw "Unable to determine tool version: $Tool"
    }
    return (($versionLines -join ' ') -replace '\s+', ' ').Trim()
}

function Get-RaygenSpirvWords
{
    param([string]$Path)

    $bytes = [IO.File]::ReadAllBytes($Path)
    if (($bytes.Length % 4) -ne 0) { throw "SPIR-V is not word aligned: $Path" }
    $words = for ($offset = 0; $offset -lt $bytes.Length; $offset += 4) { [BitConverter]::ToUInt32($bytes, $offset) }
    return [pscustomobject]@{ Bytes = $bytes; Words = @($words) }
}

function Get-RaygenIncludeText
{
    param([string]$Key, [string]$DependencySha256, [uint32[]]$Words)

    $lines = for ($index = 0; $index -lt $Words.Count; $index += 8)
    {
        $last = [Math]::Min($index + 7, $Words.Count - 1)
        '    ' + ((@($Words[$index..$last] | ForEach-Object { '0x{0:x8}u' -f $_ }) -join ', ') + ',')
    }
    return "// Raygen variant key: $Key`n// Raygen dependency SHA-256: $DependencySha256`n" + ($lines -join "`n") + "`n"
}

function Read-RaygenInclude
{
    param([string]$Path, [string]$ExpectedKey, [string]$ExpectedDependencySha256)

    $text = Get-RaygenCanonicalText -Path $Path
    $key = [regex]::Match($text, '^// Raygen variant key: ([a-z_]+)$', [Text.RegularExpressions.RegexOptions]::Multiline).Groups[1].Value
    $dependency = [regex]::Match($text, '^// Raygen dependency SHA-256: ([0-9a-f]{64})$', [Text.RegularExpressions.RegexOptions]::Multiline).Groups[1].Value
    if (-not (Test-RaygenExactString -Left $key -Right $ExpectedKey) -or
        -not (Test-RaygenExactString -Left $dependency -Right $ExpectedDependencySha256))
    {
        throw "Raygen variant include header mismatch: $Path"
    }
    $matches = [regex]::Matches($text, '0x([0-9a-fA-F]{8})u')
    if ($matches.Count -eq 0) { throw "Raygen variant include contains no words: $Path" }
    $words = foreach ($match in $matches) { [Convert]::ToUInt32($match.Groups[1].Value, 16) }
    $bytes = New-Object byte[] ($words.Count * 4)
    for ($index = 0; $index -lt $words.Count; ++$index)
    {
        [Array]::Copy([BitConverter]::GetBytes([uint32]$words[$index]), 0, $bytes, $index * 4, 4)
    }
    return [pscustomobject]@{ Text = $text; Words = @($words); Bytes = $bytes }
}

function Test-RaygenAllowedPath
{
    param([string]$Path, [string]$ExpectedPath, [string]$Label)

    if ([string]::IsNullOrWhiteSpace($Path) -or -not [IO.Path]::IsPathRooted($Path))
    {
        throw "$Label requires an absolute path."
    }
    $fullPath = [IO.Path]::GetFullPath($Path)
    $expectedFullPath = [IO.Path]::GetFullPath($ExpectedPath)
    if (-not $fullPath.Equals($expectedFullPath, [StringComparison]::OrdinalIgnoreCase))
    {
        throw "$Label must be exactly $expectedFullPath."
    }
    return $fullPath
}

function Assert-RaygenFrozenBudgets
{
    param([pscustomobject]$Budgets, [object[]]$Variants)

    if ($Budgets.schema -ne 1 -or -not (Test-RaygenExactString -Left $Budgets.status -Right 'frozen'))
    {
        throw 'Raygen variant budgets must use frozen schema 1.'
    }
    $metrics = @('bytes', 'words', 'instructions', 'branchOperations', 'loops', 'selectionMerges', 'functions', 'functionCalls', 'rayQueryInitializations', 'atomicInstructions')
    if (-not (Test-RaygenExactString -Left (@($Budgets.PSObject.Properties.Name | Sort-Object) -join ',') -Right 'budgets,metrics,schema,status') -or
        -not (Test-RaygenExactString -Left (@($Budgets.metrics) -join ',') -Right ($metrics -join ',')))
    {
        throw 'Raygen variant budgets have an unsupported schema or metric set.'
    }
    $rows = @($Budgets.budgets)
    if ($rows.Count -ne $Variants.Count) { throw 'Raygen variant budgets must contain exactly eight rows.' }
    for ($index = 0; $index -lt $Variants.Count; ++$index)
    {
        $row = $rows[$index]
        $variant = $Variants[$index]
        if (-not (Test-RaygenExactString -Left (@($row.PSObject.Properties.Name | Sort-Object) -join ',') -Right 'exact,key,max') -or
            -not (Test-RaygenExactString -Left $row.key -Right $variant.key) -or
            -not (Test-RaygenExactString -Left (@($row.max.PSObject.Properties.Name | Sort-Object) -join ',') -Right (($metrics | Sort-Object) -join ',')) -or
            -not (Test-RaygenExactString -Left (@($row.exact.PSObject.Properties.Name | Sort-Object) -join ',') -Right 'atomicInstructions,boundedGenericFunctionsRetained,driverSafeFullyInlined,hasDiagnosticsBinding,instrumentation,material,quality,shippingAllowed,strategy'))
        {
            throw "Raygen variant budget row is malformed: $($variant.key)"
        }
        foreach ($metric in $metrics)
        {
            if ($row.max.$metric -isnot [long] -and $row.max.$metric -isnot [int]) { throw "Raygen variant budget metric is invalid: $($variant.key)/$metric" }
            if ([long]$variant.$metric -gt [long]$row.max.$metric) { throw "Raygen variant exceeds frozen budget: $($variant.key)/$metric" }
        }
        foreach ($property in @('instrumentation', 'quality', 'material', 'strategy', 'shippingAllowed', 'atomicInstructions', 'hasDiagnosticsBinding', 'driverSafeFullyInlined', 'boundedGenericFunctionsRetained'))
        {
            if ($row.exact.$property -ne $variant.$property) { throw "Raygen variant exact budget invariant changed: $($variant.key)/$property" }
        }
        if ($variant.instrumentation -eq 'Shipping' -and ($variant.atomicInstructions -ne 0 -or $variant.hasDiagnosticsBinding))
        { throw "Shipping variant violates diagnostic budget invariants: $($variant.key)" }
        $expectedDiagnosticAtomics = if ($variant.material -eq 'GenericDielectric') { 32 } else { 5 }
        if ($variant.instrumentation -eq 'Diagnostic' -and (-not $variant.hasDiagnosticsBinding -or $variant.atomicInstructions -ne $expectedDiagnosticAtomics))
        { throw "Diagnostic variant violates reviewed counter shape: $($variant.key)" }
    }
}

function New-RaygenVariantCatalog
{
    param([pscustomobject]$Manifest, [string]$OutputRoot)

    $variantRows = @()
    foreach ($definition in $Manifest.Variants)
    {
        $variantRoot = Join-Path $OutputRoot $definition.name
        $stats = Get-Content -LiteralPath (Join-Path $variantRoot 'raygen-stats.json') -Raw | ConvertFrom-Json
        $spirvPath = Join-Path $variantRoot 'minimal.rgen.spv'
        $spirv = Get-RaygenSpirvWords -Path $spirvPath
        $artifactPath = "src/vulkan/raytracing/variants/$($definition.name).inc"
        $includeText = Get-RaygenIncludeText -Key $definition.name -DependencySha256 $stats.dependencySha256 -Words $spirv.Words
        $stageInclude = Join-Path $variantRoot "$($definition.name).inc"
        Write-RaygenCanonicalText -Path $stageInclude -Text $includeText
        $optimizerArguments = [Collections.ArrayList]::new()
        if ($definition.strategy -eq 'LegacyInlined') { [void]$optimizerArguments.Add('-O') }
        else
        {
            foreach ($pass in @('--eliminate-dead-functions', '--eliminate-dead-code-aggressive', '--simplify-instructions', '--eliminate-dead-branches', '--cfg-cleanup'))
            { [void]$optimizerArguments.Add($pass) }
        }
        $variantRows += [ordered]@{
            key = $definition.name; instrumentation = $definition.instrumentation; quality = $definition.quality
            material = $definition.material; strategy = $definition.strategy; shippingAllowed = $definition.shippingAllowed
            artifactPath = $artifactPath; dependencySha256 = $stats.dependencySha256; dependencies = @($stats.dependencies)
            includeSha256 = Get-RaygenDependencyHash -Path $stageInclude; spirvSha256 = Get-RaygenFileSha256 -Path $spirvPath
            bytes = $stats.bytes; words = $stats.words; instructions = $stats.instructions; branchOperations = $stats.branchOperations
            loops = $stats.loops; selectionMerges = $stats.selectionMerges; functions = $stats.functions
            functionCalls = $stats.functionCalls; rayQueryInitializations = $stats.rayQueryInitializations
            atomicInstructions = $stats.atomicInstructions; hasDiagnosticsBinding = $stats.hasDiagnosticsBinding
            driverSafeFullyInlined = $stats.driverSafeFullyInlined; boundedGenericFunctionsRetained = $stats.boundedGenericFunctionsRetained
            compiler = [ordered]@{
                validatorArguments = if ($definition.strategy -eq 'LegacyInlined') { @('-V', '--target-env', 'vulkan1.2', '-Os', '-S', 'rgen') } else { @('-V', '--target-env', 'vulkan1.2', '-S', 'rgen') }
                optimizerArguments = $optimizerArguments
                validatorArgumentsAfterOutput = @('-S', 'rgen')
            }
        }
    }
    return [ordered]@{
        schema = 1; status = 'frozen'; target = [ordered]@{ environment = 'vulkan1.2'; stage = 'rgen' }
        generator = [ordered]@{ path = 'tools/compile-raygen.ps1'; interface = 'freeze-v1'; sha256 = Get-RaygenDependencyHash -Path $PSCommandPath }
        toolchain = [ordered]@{
            glslangValidator = [ordered]@{ version = Get-RaygenToolVersion -Tool $validator; arguments = @('-V', '--target-env', 'vulkan1.2', '-S', 'rgen') }
            spirvOpt = [ordered]@{ version = Get-RaygenToolVersion -Tool $optimizer }
            spirvVal = [ordered]@{ version = Get-RaygenToolVersion -Tool (Join-Path $VulkanSdk 'Bin\spirv-val.exe'); arguments = @('--target-env', 'vulkan1.2') }
            spirvDis = [ordered]@{ version = Get-RaygenToolVersion -Tool $disassembler }
        }
        authorities = [ordered]@{
            manifest = [ordered]@{ path = 'tools/raygen-variants.json'; sha256 = $Manifest.Sha256 }
            source = [ordered]@{ path = 'shaders/raytracing/minimal.rgen'; sha256 = Get-RaygenDependencyHash -Path $source }
            variantConfig = [ordered]@{ path = 'shaders/raytracing/include/rt_variant_config.glsl'; sha256 = Get-RaygenDependencyHash -Path (Join-Path $repoRoot 'shaders\raytracing\include\rt_variant_config.glsl') }
        }
        variants = @($variantRows)
    }
}

function Write-RaygenCatalogJson
{
    param([string]$Path, [object]$Catalog)
    $json = ($Catalog | ConvertTo-Json -Depth 16).Replace("`r`n", "`n") + "`n"
    Write-RaygenCanonicalText -Path $Path -Text $json
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

function Get-RaygenBracedFunctionBody
{
    param([string]$Source, [string]$Name)

    $match = [regex]::Match($Source, ('(?ms)^\s*(?:[A-Za-z_]\w*\s+)+{0}\s*\([^{{;]*\)\s*\{{' -f [regex]::Escape($Name)))
    if (-not $match.Success) { throw "Missing preprocessed function definition: $Name" }
    $depth = 0
    for ($index = $match.Index + $match.Length - 1; $index -lt $Source.Length; ++$index)
    {
        if ($Source[$index] -eq '{') { ++$depth }
        elseif ($Source[$index] -eq '}')
        {
            --$depth
            if ($depth -eq 0) { return $Source.Substring($match.Index, $index - $match.Index + 1) }
        }
    }
    throw "Unterminated preprocessed function definition: $Name"
}

function Assert-RaygenMatrixRoute
{
    param([string]$Source, [string]$Name, [string]$InterfaceConstant, [string]$Guard, [switch]$StaticCeiling, [string]$VolumeConstant = '')

    $body = Get-RaygenBracedFunctionBody -Source $Source -Name $Name
    if ($body -match 'controls\.waterQuality' -or $body -notmatch ("\bconst\s+int\s+interfaceBudget\s*=\s*{0}\s*;" -f [regex]::Escape($InterfaceConstant)) -or
        $body -notmatch ("\b{0}\s*>=\s*interfaceBudget\b" -f [regex]::Escape($Guard)))
    { throw "Matrix route budget contract failed: $Name" }
    if ($StaticCeiling -and $body -notmatch ("\bfor\s*\([^;]*;\s*\w+\s*<=\s*{0}\s*;" -f [regex]::Escape($InterfaceConstant)))
    { throw "Matrix route lost its static interface ceiling: $Name" }
    if (-not [string]::IsNullOrWhiteSpace($VolumeConstant) -and
        ($body -notmatch ("\bconst\s+int\s+volumeBudget\s*=\s*{0}\s*;" -f [regex]::Escape($VolumeConstant)) -or
         $body -notmatch ("\[\s*{0}\s*\]" -f [regex]::Escape($VolumeConstant)) -or
         $body -notmatch ("\bfor\s*\([^;]*;\s*\w+\s*<\s*{0}\s*;" -f [regex]::Escape($VolumeConstant))))
    { throw "Matrix route volume budget contract failed: $Name" }
}

function Assert-RaygenSpecializedRoutes
{
    param([string]$Path, [pscustomobject]$VariantDefinition)

    if ($VariantDefinition.material -ne 'GenericDielectric') { return }
    $sourceText = Get-RaygenCanonicalText -Path $Path
    $interface = if ($VariantDefinition.quality -eq 'Mobile') { 4 } else { 8 }
    $volume = if ($VariantDefinition.quality -eq 'Mobile') { 2 } else { 4 }
    foreach ($constant in @('kRtVariantDielectricInterfaceBudget', 'kRtVariantShadowInterfaceBudget'))
    {
        if (@([regex]::Matches($sourceText, ("\bconst\s+int\s+{0}\s*=\s*{1}\s*;" -f $constant, $interface))).Count -ne 1)
        { throw "Matrix preprocessed source lost literal interface budget $constant=$interface." }
    }
    foreach ($constant in @('kRtVariantDielectricVolumeBudget', 'kRtVariantShadowVolumeBudget'))
    {
        if (@([regex]::Matches($sourceText, ("\bconst\s+int\s+{0}\s*=\s*{1}\s*;" -f $constant, $volume))).Count -ne 1)
        { throw "Matrix preprocessed source lost literal volume budget $constant=$volume." }
    }
    Assert-RaygenMatrixRoute -Source $sourceText -Name 'shadeBoundedDielectric' -InterfaceConstant 'kRtVariantDielectricInterfaceBudget' -Guard 'interfaceIndex' -StaticCeiling -VolumeConstant 'kRtVariantDielectricVolumeBudget'
    Assert-RaygenMatrixRoute -Source $sourceText -Name 'shadeProductionBoundedDielectric' -InterfaceConstant 'kRtVariantDielectricInterfaceBudget' -Guard 'interfaceIndex' -StaticCeiling
    Assert-RaygenMatrixRoute -Source $sourceText -Name 'shadowTransmittanceMask' -InterfaceConstant 'kRtVariantShadowInterfaceBudget' -Guard 'interfaceCount' -StaticCeiling -VolumeConstant 'kRtVariantShadowVolumeBudget'
    Assert-RaygenMatrixRoute -Source $sourceText -Name 'compactShadowTransmittanceMask' -InterfaceConstant 'kRtVariantShadowInterfaceBudget' -Guard 'interfaceCount' -StaticCeiling
    Assert-RaygenMatrixRoute -Source $sourceText -Name 'boundedShadowTransmittanceMask' -InterfaceConstant 'kRtVariantShadowInterfaceBudget' -Guard 'interfaceCount'
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
    Assert-RaygenSpecializedRoutes -Path $preprocessedSourcePath -VariantDefinition $VariantDefinition

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

function Invoke-RaygenFrozenCatalogMode
{
    param([switch]$Publish)

    if ($Check -or $Legacy -or $Matrix -or -not [string]::IsNullOrWhiteSpace($Variant) -or
        -not [string]::IsNullOrWhiteSpace($Strategy) -or -not [string]::IsNullOrWhiteSpace($OutputDirectory) -or
        -not [string]::IsNullOrWhiteSpace($EmbeddedIncludePath))
    {
        throw 'Freeze and catalog-check modes are mutually exclusive with compatibility and temporary matrix modes.'
    }
    $expectedArtifacts = Join-Path $repoRoot 'src\vulkan\raytracing\variants'
    $expectedCatalog = Join-Path $repoRoot 'tools\raygen-variant-catalog.json'
    $expectedBudgets = Join-Path $repoRoot 'tools\raygen-variant-budgets.json'
    $artifactRoot = Test-RaygenAllowedPath -Path $ArtifactDirectory -ExpectedPath $expectedArtifacts -Label 'ArtifactDirectory'
    $catalogFile = Test-RaygenAllowedPath -Path $CatalogPath -ExpectedPath $expectedCatalog -Label 'CatalogPath'
    $budgetFile = Test-RaygenAllowedPath -Path $BudgetPath -ExpectedPath $expectedBudgets -Label 'BudgetPath'
    if (-not (Test-Path -LiteralPath $budgetFile -PathType Leaf)) { throw "Frozen raygen budgets were not found: $budgetFile" }
    $budgets = Get-Content -LiteralPath $budgetFile -Raw | ConvertFrom-Json
    $manifest = Get-RaygenVariantManifest -Path $ManifestPath
    $temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) ('horde-raygen-freeze-' + [guid]::NewGuid().ToString('N'))
    $statusBefore = (& git -C $repoRoot status --porcelain) -join "`n"
    try
    {
        New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
        foreach ($definition in $manifest.Variants)
        {
            Invoke-RaygenVariantCompilation -VariantDefinition $definition -OutputRoot $temporaryRoot -Manifest $manifest
        }
        $expectedCatalogObject = New-RaygenVariantCatalog -Manifest $manifest -OutputRoot $temporaryRoot
        Assert-RaygenFrozenBudgets -Budgets $budgets -Variants @($expectedCatalogObject.variants)
        $expectedCatalogPath = Join-Path $temporaryRoot 'raygen-variant-catalog.json'
        Write-RaygenCatalogJson -Path $expectedCatalogPath -Catalog $expectedCatalogObject

        if ($Publish)
        {
            # No tracked output is touched until all eight compiles, SPIR-V
            # validations, catalog construction, and frozen-budget checks pass.
            if (-not (Test-Path -LiteralPath $artifactRoot)) { New-Item -ItemType Directory -Path $artifactRoot | Out-Null }
            $expectedNames = @($manifest.Variants | ForEach-Object { "$($_.name).inc" })
            $unexpected = @(Get-ChildItem -LiteralPath $artifactRoot -File -ErrorAction Stop | Where-Object { $_.Name -notin $expectedNames })
            if ($unexpected.Count -ne 0) { throw 'Artifact directory contains an unexpected file; refusing to publish a partial catalog.' }
            foreach ($definition in $manifest.Variants)
            {
                Copy-Item -LiteralPath (Join-Path (Join-Path $temporaryRoot $definition.name) "$($definition.name).inc") `
                    -Destination (Join-Path $artifactRoot "$($definition.name).inc") -Force
            }
            Copy-Item -LiteralPath $expectedCatalogPath -Destination $catalogFile -Force
            Write-Output "Frozen eight raygen variant artifacts and catalog."
            return
        }

        if (-not (Test-Path -LiteralPath $artifactRoot -PathType Container) -or -not (Test-Path -LiteralPath $catalogFile -PathType Leaf))
        {
            throw 'Frozen raygen variant artifacts or catalog are missing.'
        }
        $catalogBytes = [IO.File]::ReadAllBytes($catalogFile)
        if ($catalogBytes.Length -ge 3 -and $catalogBytes[0] -eq 0xef -and $catalogBytes[1] -eq 0xbb -and $catalogBytes[2] -eq 0xbf)
        { throw 'Frozen raygen variant catalog must not contain a UTF-8 BOM.' }
        $catalogText = Get-RaygenCanonicalText -Path $catalogFile
        if ($catalogText -match "`r") { throw 'Frozen raygen variant catalog must use LF line endings.' }
        $actualCatalog = $catalogText | ConvertFrom-Json
        if (-not (Test-RaygenExactString -Left (@($actualCatalog.PSObject.Properties.Name | Sort-Object) -join ',') -Right 'authorities,generator,schema,status,target,toolchain,variants') -or
            $actualCatalog.schema -ne 1 -or -not (Test-RaygenExactString -Left $actualCatalog.status -Right 'frozen'))
        { throw 'Frozen raygen variant catalog has an invalid schema.' }
        $expectedCatalogText = Get-RaygenCanonicalText -Path $expectedCatalogPath
        if (-not (Test-RaygenExactString -Left $catalogText -Right $expectedCatalogText))
        { throw 'Frozen raygen variant catalog is stale or malformed.' }
        foreach ($expectedVariant in $expectedCatalogObject.variants)
        {
            $includePath = Join-Path $repoRoot $expectedVariant.artifactPath.Replace('/', '\')
            if (-not $includePath.StartsWith($artifactRoot.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase))
            { throw "Catalog artifact path escapes the variant directory: $($expectedVariant.artifactPath)" }
            $include = Read-RaygenInclude -Path $includePath -ExpectedKey $expectedVariant.key -ExpectedDependencySha256 $expectedVariant.dependencySha256
            $compiled = Get-RaygenSpirvWords -Path (Join-Path (Join-Path $temporaryRoot $expectedVariant.key) 'minimal.rgen.spv')
            if ($include.Words.Count -ne $compiled.Words.Count -or (Get-RaygenDependencyHash -Path $includePath) -ne $expectedVariant.includeSha256)
            { throw "Frozen raygen variant include is stale: $($expectedVariant.key)" }
            for ($index = 0; $index -lt $compiled.Words.Count; ++$index)
            {
                if ($include.Words[$index] -ne $compiled.Words[$index]) { throw "Frozen raygen variant include words are stale: $($expectedVariant.key)" }
            }
            $rawHash = Get-RaygenFileSha256 -Path (Join-Path (Join-Path $temporaryRoot $expectedVariant.key) 'minimal.rgen.spv')
            $includeRawHash = ([BitConverter]::ToString(([Security.Cryptography.SHA256]::Create().ComputeHash($include.Bytes)))).Replace('-', '').ToLowerInvariant()
            if ($rawHash -ne $includeRawHash -or $rawHash -ne $expectedVariant.spirvSha256)
            { throw "Frozen raygen variant raw SPIR-V hash mismatch: $($expectedVariant.key)" }
        }
        if ((& git -C $repoRoot status --porcelain) -join "`n" -ne $statusBefore)
        { throw 'Read-only raygen catalog check modified the worktree.' }
        Write-Output 'Frozen raygen variant catalog matches a fresh eight-key compilation.'
    }
    finally
    {
        if (Test-Path -LiteralPath $temporaryRoot) { Remove-Item -LiteralPath $temporaryRoot -Recurse -Force }
    }
}

if ($Freeze -or $CheckCatalog)
{
    if ($Freeze -and $CheckCatalog) { throw '-Freeze and -CheckCatalog are mutually exclusive.' }
    Invoke-RaygenFrozenCatalogMode -Publish:$Freeze
    return
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
