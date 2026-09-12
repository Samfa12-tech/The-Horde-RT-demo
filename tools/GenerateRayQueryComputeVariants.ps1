[CmdletBinding()]
param(
    [switch]$Check,
    [switch]$Write,
    [string]$VulkanSdk = $env:VULKAN_SDK,
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
if ($Check -and $Write)
{
    throw 'Use either -Check or -Write, not both.'
}
$checkMode = $Check -or -not $Write
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($VulkanSdk))
{
    $VulkanSdk = 'C:\VulkanSDK\1.4.350.0'
}
if ([string]::IsNullOrWhiteSpace($OutputDirectory))
{
    $OutputDirectory = Join-Path ([IO.Path]::GetTempPath()) `
        ('horde-rt-rayquery-compute-' + [guid]::NewGuid().ToString('N'))
}
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
$repoPrefix = $repoRoot.TrimEnd('\') + '\'
if ($outputRoot.Equals($repoRoot, [StringComparison]::OrdinalIgnoreCase) -or
    $outputRoot.StartsWith($repoPrefix, [StringComparison]::OrdinalIgnoreCase))
{
    throw 'Compute variant staging output must be outside the repository.'
}

$sourcePath = Join-Path $repoRoot 'shaders\raytracing\minimal.comp'
$manifestPath = Join-Path $repoRoot 'tools\raygen-variants.json'
$catalogPath = Join-Path $repoRoot 'tools\rayquery-variant-catalog.json'
$artifactDirectory = Join-Path $repoRoot 'src\vulkan\raytracing\variants'
$headerPath = Join-Path $repoRoot 'src\vulkan\raytracing\RtRayQueryVariantCatalog.generated.h'
$compiler = Join-Path $VulkanSdk 'Bin\glslangValidator.exe'
$optimizer = Join-Path $VulkanSdk 'Bin\spirv-opt.exe'
$validator = Join-Path $VulkanSdk 'Bin\spirv-val.exe'
$disassembler = Join-Path $VulkanSdk 'Bin\spirv-dis.exe'

function Assert-ComputeTrue
{
    param([bool]$Value, [string]$Message)
    if (-not $Value) { throw $Message }
}

function Test-ComputeExactString
{
    param([object]$Left, [object]$Right)
    return $Left -is [string] -and $Right -is [string] -and
        [string]::Equals($Left, $Right, [StringComparison]::Ordinal)
}

function Assert-ComputePropertyNames
{
    param([object]$Value, [string[]]$Expected, [string]$Message)
    $actual = @($Value.PSObject.Properties.Name | Sort-Object)
    $sortedExpected = @($Expected | Sort-Object)
    Assert-ComputeTrue (($actual -join "`n") -ceq ($sortedExpected -join "`n")) $Message
}

function Get-ComputeCanonicalText
{
    param([string]$Path)
    $lines = [IO.File]::ReadAllLines($Path)
    if ($lines.Count -eq 0) { return '' }
    return [string]::Join("`n", $lines) + "`n"
}

function Write-ComputeCanonicalText
{
    param([string]$Path, [string]$Text)
    $canonical = $Text.Replace("`r`n", "`n").Replace("`r", "`n")
    [IO.File]::WriteAllText($Path, $canonical, [Text.UTF8Encoding]::new($false))
}

function Get-ComputeSha256Bytes
{
    param([byte[]]$Bytes)
    $sha256 = [Security.Cryptography.SHA256]::Create()
    try
    {
        return ([BitConverter]::ToString($sha256.ComputeHash($Bytes))).Replace('-', '').ToLowerInvariant()
    }
    finally
    {
        $sha256.Dispose()
    }
}

function Get-ComputeTextSha256
{
    param([string]$Text)
    return Get-ComputeSha256Bytes -Bytes ([Text.Encoding]::UTF8.GetBytes($Text))
}

function Get-ComputeCanonicalFileSha256
{
    param([string]$Path)
    return Get-ComputeTextSha256 -Text (Get-ComputeCanonicalText -Path $Path)
}

function Get-ComputeFileSha256
{
    param([string]$Path)
    return Get-ComputeSha256Bytes -Bytes ([IO.File]::ReadAllBytes($Path))
}

function Get-ComputeRelativePath
{
    param([string]$Path)
    $fullPath = [IO.Path]::GetFullPath($Path)
    if (-not $fullPath.StartsWith($repoPrefix, [StringComparison]::OrdinalIgnoreCase))
    {
        throw "Compute shader dependency is outside the repository: $fullPath"
    }
    return $fullPath.Substring($repoPrefix.Length).Replace('\', '/')
}

function Add-ComputeShaderDependencies
{
    param(
        [string]$Path,
        [Collections.Generic.HashSet[string]]$ActivePaths,
        [Collections.Generic.HashSet[string]]$SeenPaths,
        [Collections.Generic.List[string]]$Dependencies
    )
    $fullPath = [IO.Path]::GetFullPath($Path)
    Assert-ComputeTrue (Test-Path -LiteralPath $fullPath -PathType Leaf) `
        "Compute shader dependency was not found: $fullPath"
    if ($ActivePaths.Contains($fullPath))
    {
        throw "Compute shader include cycle detected at $fullPath"
    }
    if (-not $SeenPaths.Add($fullPath)) { return }
    Assert-ComputeTrue ($fullPath.StartsWith($repoPrefix, [StringComparison]::OrdinalIgnoreCase)) `
        "Compute shader dependency is outside the repository: $fullPath"
    [void]$Dependencies.Add($fullPath)
    [void]$ActivePaths.Add($fullPath)
    $directory = Split-Path -Parent $fullPath
    foreach ($line in [IO.File]::ReadAllLines($fullPath))
    {
        if ($line -match '^\s*#include\s+"([^"]+)"\s*$')
        {
            Add-ComputeShaderDependencies -Path (Join-Path $directory $Matches[1]) `
                -ActivePaths $ActivePaths -SeenPaths $SeenPaths -Dependencies $Dependencies
        }
    }
    [void]$ActivePaths.Remove($fullPath)
}

function Get-ComputeToolVersion
{
    param([string]$Tool)
    $commandLine = '"' + $Tool + '" --version 2>&1'
    $lines = @(& $env:ComSpec /d /c $commandLine | ForEach-Object {
        ([string]$_).Trim()
    } | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($LASTEXITCODE -ne 0 -or $lines.Count -eq 0)
    {
        throw "Unable to determine shader tool version: $Tool"
    }
    return (($lines -join ' ') -replace '\s+', ' ').Trim()
}

function Read-ComputeManifest
{
    param([string]$Path)
    Assert-ComputeTrue (Test-Path -LiteralPath $Path -PathType Leaf) `
        "Compute policy manifest was not found: $Path"
    $manifest = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    Assert-ComputePropertyNames $manifest @('schema', 'variants') `
        'Compute policy manifest root schema is invalid.'
    Assert-ComputeTrue (($manifest.schema -is [int] -or $manifest.schema -is [long]) -and
        $manifest.schema -eq 1) 'Compute policy manifest must use integer schema 1.'

    $expected = @()
    foreach ($instrumentation in @('Shipping', 'Diagnostic'))
    {
        foreach ($quality in @('Mobile', 'High'))
        {
            foreach ($material in @('OpaqueFast', 'GenericDielectric'))
            {
                $materialName = if ($material -ceq 'OpaqueFast') { 'opaque_fast' } else { 'generic_dielectric' }
                $policyKey = '{0}_{1}_{2}' -f $instrumentation.ToLowerInvariant(), `
                    $quality.ToLowerInvariant(), $materialName
                $expected += [pscustomobject]@{
                    policyKey = $policyKey
                    key = "rayquery_compute_$policyKey"
                    instrumentation = $instrumentation
                    quality = $quality
                    material = $material
                    strategy = if ($material -ceq 'OpaqueFast') { 'LegacyInlined' } else { 'GenericRetained' }
                    shippingAllowed = $instrumentation -ceq 'Shipping'
                    instrumentationValue = if ($instrumentation -ceq 'Shipping') { 0 } else { 1 }
                    qualityValue = if ($quality -ceq 'Mobile') { 0 } else { 1 }
                    materialValue = if ($material -ceq 'OpaqueFast') { 0 } else { 1 }
                }
            }
        }
    }

    $rows = @($manifest.variants)
    Assert-ComputeTrue ($rows.Count -eq 8) `
        'Compute policy manifest must contain exactly eight policy rows.'
    foreach ($expectedRow in $expected)
    {
        $matches = @($rows | Where-Object {
            Test-ComputeExactString $_.name $expectedRow.policyKey
        })
        Assert-ComputeTrue ($matches.Count -eq 1) `
            "Compute policy manifest is missing or duplicates $($expectedRow.policyKey)."
        $row = $matches[0]
        Assert-ComputePropertyNames $row @('name', 'instrumentation', 'quality', 'material', 'strategy', 'shippingAllowed') `
            "Compute policy row schema is invalid: $($expectedRow.policyKey)"
        foreach ($property in @('instrumentation', 'quality', 'material', 'strategy'))
        {
            Assert-ComputeTrue (Test-ComputeExactString $row.$property $expectedRow.$property) `
                "Compute policy row has invalid ${property}: $($expectedRow.policyKey)"
        }
        Assert-ComputeTrue ($row.shippingAllowed -is [bool] -and
            $row.shippingAllowed -eq $expectedRow.shippingAllowed) `
            "Compute policy row has invalid shippingAllowed: $($expectedRow.policyKey)"
    }
    return [pscustomobject]@{
        Sha256 = Get-ComputeCanonicalFileSha256 -Path $Path
        Variants = @($expected | Sort-Object key)
    }
}

function Get-ComputeSpirvWords
{
    param([string]$Path)
    $bytes = [IO.File]::ReadAllBytes($Path)
    Assert-ComputeTrue (($bytes.Length % 4) -eq 0 -and $bytes.Length -gt 0) `
        "Compute SPIR-V is empty or not word aligned: $Path"
    $words = for ($offset = 0; $offset -lt $bytes.Length; $offset += 4)
    {
        [BitConverter]::ToUInt32($bytes, $offset)
    }
    return [pscustomobject]@{ Bytes = $bytes; Words = @($words) }
}

function Get-ComputeIncludeText
{
    param([object]$Variant, [string]$DependencySha256, [uint32[]]$Words)
    $lines = for ($index = 0; $index -lt $Words.Count; $index += 8)
    {
        $last = [Math]::Min($index + 7, $Words.Count - 1)
        '    ' + ((@($Words[$index..$last] | ForEach-Object {
            '0x{0:x8}u' -f $_
        }) -join ', ') + ',')
    }
    return "// RayQuery compute variant key: $($Variant.key)`n" +
        "// RayQuery compute dependency SHA-256: $DependencySha256`n" +
        ($lines -join "`n") + "`n"
}

function Invoke-ComputeVariantCompilation
{
    param(
        [object]$Variant,
        [string]$RunRoot,
        [object[]]$Dependencies,
        [string]$ManifestSha256
    )
    $variantRoot = Join-Path $RunRoot $Variant.key
    [void](New-Item -ItemType Directory -Path $variantRoot -Force)
    $rawSpirv = Join-Path $variantRoot 'minimal.comp.raw.spv'
    $spirv = Join-Path $variantRoot 'minimal.comp.spv'
    $assemblyPath = Join-Path $variantRoot 'minimal.comp.spvasm'

    $compileMetadata = @('-V', '--target-env', 'vulkan1.2', '-S', 'comp')
    if ($Variant.material -ceq 'OpaqueFast') { $compileMetadata += '-Os' }
    $compileMetadata += @(
        "-DHORDE_RT_VARIANT_INSTRUMENTATION=$($Variant.instrumentationValue)",
        "-DHORDE_RT_VARIANT_QUALITY=$($Variant.qualityValue)",
        "-DHORDE_RT_VARIANT_MATERIAL=$($Variant.materialValue)",
        '-o', '<output>', '<source>')
    $compileArguments = @($compileMetadata)
    $compileArguments[$compileArguments.Count - 2] = $rawSpirv
    $compileArguments[$compileArguments.Count - 1] = $sourcePath
    $compileOutput = @(& $compiler @compileArguments 2>&1)
    $compileExitCode = $LASTEXITCODE
    if ($compileExitCode -ne 0)
    {
        throw "Compute compilation failed for $($Variant.key) with exit code ${compileExitCode}: $($compileOutput -join ' ')"
    }

    $optimizerMetadata = if ($Variant.material -ceq 'OpaqueFast')
    {
        @('-O', '<input>', '-o', '<output>')
    }
    else
    {
        @('--eliminate-dead-functions', '--eliminate-dead-code-aggressive',
          '--simplify-instructions', '--eliminate-dead-branches', '--cfg-cleanup',
          '<input>', '-o', '<output>')
    }
    $optimizerArguments = @($optimizerMetadata)
    $optimizerArguments[$optimizerArguments.Count - 3] = $rawSpirv
    $optimizerArguments[$optimizerArguments.Count - 1] = $spirv
    $optimizerOutput = @(& $optimizer @optimizerArguments 2>&1)
    $optimizerExitCode = $LASTEXITCODE
    if ($optimizerExitCode -ne 0)
    {
        throw "Compute optimization failed for $($Variant.key) with exit code ${optimizerExitCode}: $($optimizerOutput -join ' ')"
    }

    $validatorOutput = @(& $validator --target-env vulkan1.2 $spirv 2>&1)
    $validatorExitCode = $LASTEXITCODE
    if ($validatorExitCode -ne 0)
    {
        throw "Compute SPIR-V validation failed for $($Variant.key) with exit code ${validatorExitCode}: $($validatorOutput -join ' ')"
    }
    $disassemblerOutput = @(& $disassembler $spirv -o $assemblyPath 2>&1)
    $disassemblerExitCode = $LASTEXITCODE
    if ($disassemblerExitCode -ne 0)
    {
        throw "Compute SPIR-V disassembly failed for $($Variant.key) with exit code ${disassemblerExitCode}: $($disassemblerOutput -join ' ')"
    }

    $assembly = Get-Content -LiteralPath $assemblyPath -Raw
    foreach ($required in @('OpEntryPoint GLCompute', 'OpCapability RayQueryKHR',
        'OpRayQueryInitializeKHR', 'OpRayQueryProceedKHR', 'OpImageWrite'))
    {
        Assert-ComputeTrue ($assembly.Contains($required)) `
            "Compute artifact $($Variant.key) is missing $required."
    }
    Assert-ComputeTrue ($assembly -match '\bOpExecutionMode\s+%\S+\s+LocalSize\s+8\s+8\s+1\b') `
        "Compute artifact $($Variant.key) lost its 8x8x1 local size."
    Assert-ComputeTrue ($assembly -notmatch '\bOpCapability\s+RayTracingKHR\b|\bOpTraceRayKHR\b|\bOpEntryPoint\s+RayGenerationKHR\b') `
        "Ray-tracing-pipeline execution leaked into compute artifact $($Variant.key)."

    $assemblyLines = @(Get-Content -LiteralPath $assemblyPath)
    $instructionCount = @($assemblyLines | Where-Object {
        $_ -match '^\s*(?:%\S+\s*=\s*)?Op\w+'
    }).Count
    $branchOperationCount = @($assemblyLines | Where-Object {
        $_ -match '\bOp(?:Branch|BranchConditional|Switch)\b'
    }).Count
    $loopCount = @($assemblyLines | Where-Object { $_ -match '\bOpLoopMerge\b' }).Count
    $selectionMergeCount = @($assemblyLines | Where-Object {
        $_ -match '\bOpSelectionMerge\b'
    }).Count
    $functionCount = @($assemblyLines | Where-Object { $_ -match '\bOpFunction\b' }).Count
    $functionCallCount = @($assemblyLines | Where-Object {
        $_ -match '\bOpFunctionCall\b'
    }).Count
    $rayQueryInitializationCount = @($assemblyLines | Where-Object {
        $_ -match '\bOpRayQueryInitializeKHR\b'
    }).Count
    $atomicInstructionCount = @($assemblyLines | Where-Object {
        $_ -match '\bOpAtomic\w+\b'
    }).Count
    $hasDiagnosticsBinding = @($assemblyLines | Where-Object {
        $_ -match '\bOpDecorate\s+%\S+\s+Binding\s+22\b'
    }).Count -gt 0
    if ($Variant.instrumentation -ceq 'Shipping')
    {
        Assert-ComputeTrue ($atomicInstructionCount -eq 0 -and -not $hasDiagnosticsBinding) `
            "Shipping diagnostic overhead leaked into $($Variant.key)."
    }
    else
    {
        Assert-ComputeTrue ($atomicInstructionCount -gt 0 -and $hasDiagnosticsBinding) `
            "Diagnostic counters were removed from $($Variant.key)."
    }

    $dependencyIdentity = [Text.StringBuilder]::new()
    foreach ($line in @(
        'schema=1',
        "key=$($Variant.key)",
        "policyKey=$($Variant.policyKey)",
        "instrumentation=$($Variant.instrumentation)",
        "quality=$($Variant.quality)",
        "material=$($Variant.material)",
        "strategy=$($Variant.strategy)",
        'target=vulkan1.2',
        'stage=comp',
        'executionModel=GLCompute',
        'executionBackend=RayQueryCompute',
        'localSize=8,8,1',
        "manifest=$ManifestSha256",
        ('compile=' + ($compileMetadata -join '|')),
        ('optimize=' + ($optimizerMetadata -join '|'))))
    {
        [void]$dependencyIdentity.AppendLine($line)
    }
    foreach ($dependency in $Dependencies)
    {
        [void]$dependencyIdentity.AppendLine("$($dependency.path)=$($dependency.sha256)")
    }
    $dependencySha256 = Get-ComputeTextSha256 -Text `
        ($dependencyIdentity.ToString().Replace("`r`n", "`n"))
    $spirvWords = Get-ComputeSpirvWords -Path $spirv
    $includePath = Join-Path $variantRoot "$($Variant.key).inc"
    Write-ComputeCanonicalText -Path $includePath -Text `
        (Get-ComputeIncludeText -Variant $Variant -DependencySha256 $dependencySha256 `
            -Words $spirvWords.Words)

    $row = [ordered]@{
        key = $Variant.key
        policyKey = $Variant.policyKey
        instrumentation = $Variant.instrumentation
        quality = $Variant.quality
        material = $Variant.material
        strategy = $Variant.strategy
        shippingAllowed = $Variant.shippingAllowed
        executionBackend = 'RayQueryCompute'
        stage = 'comp'
        executionModel = 'GLCompute'
        artifactPath = "src/vulkan/raytracing/variants/$($Variant.key).inc"
        dependencySha256 = $dependencySha256
        dependencies = @($Dependencies)
        includeSha256 = Get-ComputeCanonicalFileSha256 -Path $includePath
        spirvSha256 = Get-ComputeFileSha256 -Path $spirv
        bytes = $spirvWords.Bytes.Length
        words = $spirvWords.Words.Count
        instructions = $instructionCount
        branchOperations = $branchOperationCount
        loops = $loopCount
        selectionMerges = $selectionMergeCount
        functions = $functionCount
        functionCalls = $functionCallCount
        rayQueryInitializations = $rayQueryInitializationCount
        atomicInstructions = $atomicInstructionCount
        hasDiagnosticsBinding = $hasDiagnosticsBinding
        compiler = [ordered]@{
            glslangCompileArguments = @($compileMetadata)
            spirvOptArguments = @($optimizerMetadata)
            spirvValArguments = @('--target-env', 'vulkan1.2', '<input>')
            spirvDisArguments = @('<input>', '-o', '<output>')
        }
    }
    Write-Host ("RayQuery compute variant {0}: bytes={1}; functions={2}; calls={3}; ray-query sites={4}; atomics={5}; SPIR-V SHA-256={6}" -f `
        $row.key, $row.bytes, $row.functions, $row.functionCalls,
        $row.rayQueryInitializations, $row.atomicInstructions, $row.spirvSha256)
    return [pscustomobject]@{ Row = [pscustomobject]$row; IncludePath = $includePath }
}

function New-ComputeCatalog
{
    param(
        [object[]]$Compiled,
        [object]$Manifest,
        [object[]]$Dependencies
    )
    return [ordered]@{
        schema = 1
        status = 'frozen'
        target = [ordered]@{
            environment = 'vulkan1.2'
            stage = 'comp'
            executionModel = 'GLCompute'
            executionBackend = 'RayQueryCompute'
            localSize = @(8, 8, 1)
            hardwareTraversal = 'RayQueryKHR'
        }
        generator = [ordered]@{
            path = 'tools/GenerateRayQueryComputeVariants.ps1'
            interface = 'freeze-v1'
            sha256 = Get-ComputeCanonicalFileSha256 -Path $PSCommandPath
        }
        toolchain = [ordered]@{
            glslangValidator = [ordered]@{
                version = Get-ComputeToolVersion -Tool $compiler
                preprocessArguments = @('-E', '-S', 'comp', '<source>')
            }
            spirvOpt = [ordered]@{ version = Get-ComputeToolVersion -Tool $optimizer }
            spirvVal = [ordered]@{ version = Get-ComputeToolVersion -Tool $validator }
            spirvDis = [ordered]@{ version = Get-ComputeToolVersion -Tool $disassembler }
        }
        authorities = [ordered]@{
            policyManifest = [ordered]@{
                path = 'tools/raygen-variants.json'
                sha256 = $Manifest.Sha256
            }
            source = [ordered]@{
                path = 'shaders/raytracing/minimal.comp'
                sha256 = Get-ComputeCanonicalFileSha256 -Path $sourcePath
            }
            dependencies = @($Dependencies)
        }
        variants = @($Compiled | ForEach-Object { $_.Row })
    }
}

function Render-ComputeCatalogRow
{
    param([object]$Row)
    $hasDiagnosticsBinding = if ($Row.hasDiagnosticsBinding) { 'true' } else { 'false' }
    return "    RtPipelineCatalogRecord{{RtInstrumentation::$($Row.instrumentation), DielectricQuality::$($Row.quality), RtMaterialStrategy::$($Row.material), RtExecutionBackend::RayQueryCompute}, `"$($Row.key)`", `"$($Row.artifactPath)`", `"$($Row.spirvSha256)`", `"$($Row.includeSha256)`", $($Row.words), $($Row.atomicInstructions), $hasDiagnosticsBinding}"
}

function Render-ComputeCatalogPair
{
    param([object[]]$Rows, [string]$Instrumentation, [string]$Quality, [int]$I, [int]$Q)
    $pair = @($Rows | Where-Object {
        $_.instrumentation -ceq $Instrumentation -and $_.quality -ceq $Quality
    } | Sort-Object material)
    Assert-ComputeTrue ($pair.Count -eq 2 -and
        $pair[0].material -ceq 'GenericDielectric' -and
        $pair[1].material -ceq 'OpaqueFast') `
        "Frozen RayQuery compute pair is incomplete: $Instrumentation/$Quality"
    return @(
        "#if HORDE_RT_SELECTED_INSTRUMENTATION == $I && HORDE_RT_SELECTED_DIELECTRIC_QUALITY == $Q",
        'inline constexpr std::array<RtPipelineCatalogRecord, 2> kSelectedRtRayQueryCatalog{',
        ((Render-ComputeCatalogRow -Row $pair[1]) + ','),
        ((Render-ComputeCatalogRow -Row $pair[0]) + '};'))
}

function Get-ComputeGeneratedHeader
{
    param([object[]]$Rows)
    $lines = @(
        '// Generated by tools/GenerateRayQueryComputeVariants.ps1. Do not edit.',
        '#pragma once',
        '',
        '#include "vulkan/raytracing/RtPipelineVariantCatalog.generated.h"',
        '',
        '#if !defined(HORDE_RT_SELECTED_INSTRUMENTATION) || !defined(HORDE_RT_SELECTED_DIELECTRIC_QUALITY)',
        '#error "The RT RayQuery compute bundle policy must define both selected dimensions."',
        '#endif',
        '',
        'namespace horde::vulkan::raytracing::detail {',
        '')
    $pairs = @(
        @('Shipping', 'Mobile', 0, 0),
        @('Shipping', 'High', 0, 1),
        @('Diagnostic', 'Mobile', 1, 0),
        @('Diagnostic', 'High', 1, 1))
    for ($index = 0; $index -lt $pairs.Count; ++$index)
    {
        $pairLines = @(Render-ComputeCatalogPair -Rows $Rows `
            -Instrumentation $pairs[$index][0] -Quality $pairs[$index][1] `
            -I $pairs[$index][2] -Q $pairs[$index][3])
        if ($index -gt 0) { $pairLines[0] = $pairLines[0].Replace('#if', '#elif') }
        $lines += $pairLines
    }
    $lines += @(
        '#else',
        '#error "Unsupported exact RT RayQuery compute bundle policy."',
        '#endif',
        '',
        '} // namespace horde::vulkan::raytracing::detail',
        '')
    return $lines -join "`n"
}

function Test-ComputeByteEqual
{
    param([byte[]]$Left, [byte[]]$Right)
    if ($Left.Length -ne $Right.Length) { return $false }
    for ($index = 0; $index -lt $Left.Length; ++$index)
    {
        if ($Left[$index] -ne $Right[$index]) { return $false }
    }
    return $true
}

function Assert-ComputeFileMatches
{
    param([string]$Actual, [string]$Expected, [string]$Label)
    Assert-ComputeTrue (Test-Path -LiteralPath $Actual -PathType Leaf) "$Label is missing: $Actual"
    Assert-ComputeTrue (Test-ComputeByteEqual -Left ([IO.File]::ReadAllBytes($Actual)) `
        -Right ([IO.File]::ReadAllBytes($Expected))) "$Label is stale or malformed: $Actual"
}

function Publish-ComputeFiles
{
    param([object[]]$Files)
    $records = @()
    foreach ($file in $Files)
    {
        $exists = Test-Path -LiteralPath $file.Destination -PathType Leaf
        $records += [pscustomobject]@{
            Destination = $file.Destination
            Exists = $exists
            Bytes = if ($exists) { [IO.File]::ReadAllBytes($file.Destination) } else { $null }
        }
    }
    try
    {
        foreach ($file in $Files)
        {
            $parent = Split-Path -Parent $file.Destination
            if (-not (Test-Path -LiteralPath $parent -PathType Container))
            {
                [void](New-Item -ItemType Directory -Path $parent -Force)
            }
            [IO.File]::WriteAllBytes($file.Destination, [IO.File]::ReadAllBytes($file.Source))
        }
    }
    catch
    {
        foreach ($record in $records)
        {
            if ($record.Exists)
            {
                [IO.File]::WriteAllBytes($record.Destination, $record.Bytes)
            }
            elseif (Test-Path -LiteralPath $record.Destination -PathType Leaf)
            {
                Remove-Item -LiteralPath $record.Destination -Force
            }
        }
        throw
    }
}

foreach ($path in @($sourcePath, $manifestPath, $compiler, $optimizer, $validator, $disassembler))
{
    Assert-ComputeTrue (Test-Path -LiteralPath $path -PathType Leaf) `
        "Missing RayQuery compute generation dependency: $path"
}
$manifest = Read-ComputeManifest -Path $manifestPath
$expectedArtifactNames = @($manifest.Variants | ForEach-Object { "$($_.key).inc" } | Sort-Object)
$actualComputeArtifacts = @(
    Get-ChildItem -LiteralPath $artifactDirectory -File -Filter 'rayquery_compute_*.inc' |
        ForEach-Object Name | Sort-Object)
$unexpectedArtifacts = @($actualComputeArtifacts | Where-Object {
    $expectedArtifactNames -cnotcontains $_
})
Assert-ComputeTrue ($unexpectedArtifacts.Count -eq 0) `
    ("Unexpected frozen RayQuery compute artifacts exist: " + ($unexpectedArtifacts -join ', '))
if ($checkMode)
{
    Assert-ComputeTrue (Test-Path -LiteralPath $catalogPath -PathType Leaf) `
        "Frozen RayQuery compute catalog is missing: $catalogPath"
    Assert-ComputeTrue (Test-Path -LiteralPath $headerPath -PathType Leaf) `
        "Generated RayQuery compute catalog adapter is missing: $headerPath"
    foreach ($name in $expectedArtifactNames)
    {
        Assert-ComputeTrue (Test-Path -LiteralPath (Join-Path $artifactDirectory $name) -PathType Leaf) `
            "Frozen RayQuery compute artifact is missing: $name"
    }
    Assert-ComputeTrue (($actualComputeArtifacts -join "`n") -ceq ($expectedArtifactNames -join "`n")) `
        'Frozen RayQuery compute artifact names are missing, duplicated, or reordered.'
}

$activePaths = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$seenPaths = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$dependencyPaths = [Collections.Generic.List[string]]::new()
Add-ComputeShaderDependencies -Path $sourcePath -ActivePaths $activePaths `
    -SeenPaths $seenPaths -Dependencies $dependencyPaths
$dependencies = @($dependencyPaths | ForEach-Object {
    [ordered]@{
        path = Get-ComputeRelativePath -Path $_
        sha256 = Get-ComputeCanonicalFileSha256 -Path $_
    }
} | Sort-Object path)

[void](New-Item -ItemType Directory -Path $outputRoot -Force)
$runRoot = Join-Path $outputRoot ('run-' + [guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $runRoot)
try
{
    $compiled = @()
    foreach ($variant in $manifest.Variants)
    {
        $compiled += Invoke-ComputeVariantCompilation -Variant $variant -RunRoot $runRoot `
            -Dependencies $dependencies -ManifestSha256 $manifest.Sha256
    }
    Assert-ComputeTrue ($compiled.Count -eq 8) `
        'RayQuery compute generation did not produce exactly eight variants.'
    foreach ($instrumentation in @('Shipping', 'Diagnostic'))
    {
        foreach ($quality in @('Mobile', 'High'))
        {
            $pair = @($compiled | ForEach-Object Row | Where-Object {
                $_.instrumentation -ceq $instrumentation -and $_.quality -ceq $quality
            })
            Assert-ComputeTrue ($pair.Count -eq 2) `
                "RayQuery compute material pair is incomplete: $instrumentation/$quality"
            $opaque = @($pair | Where-Object material -CEQ 'OpaqueFast')
            $generic = @($pair | Where-Object material -CEQ 'GenericDielectric')
            Assert-ComputeTrue ($opaque.Count -eq 1 -and $generic.Count -eq 1 -and
                $opaque[0].spirvSha256 -cne $generic[0].spirvSha256) `
                "Opaque/generic material policy did not specialize: $instrumentation/$quality"
        }
    }

    $catalog = New-ComputeCatalog -Compiled $compiled -Manifest $manifest `
        -Dependencies $dependencies
    $stagedCatalog = Join-Path $runRoot 'rayquery-variant-catalog.json'
    Write-ComputeCanonicalText -Path $stagedCatalog -Text `
        (($catalog | ConvertTo-Json -Depth 16) + "`n")
    $stagedHeader = Join-Path $runRoot 'RtRayQueryVariantCatalog.generated.h'
    Write-ComputeCanonicalText -Path $stagedHeader -Text `
        (Get-ComputeGeneratedHeader -Rows @($catalog.variants))

    if ($checkMode)
    {
        Assert-ComputeFileMatches -Actual $catalogPath -Expected $stagedCatalog `
            -Label 'Frozen RayQuery compute catalog'
        Assert-ComputeFileMatches -Actual $headerPath -Expected $stagedHeader `
            -Label 'Generated RayQuery compute catalog adapter'
        foreach ($record in $compiled)
        {
            Assert-ComputeFileMatches `
                -Actual (Join-Path $artifactDirectory "$($record.Row.key).inc") `
                -Expected $record.IncludePath `
                -Label "Frozen RayQuery compute artifact $($record.Row.key)"
        }
        Write-Output 'Frozen RayQuery compute artifacts, provenance catalog, and generated adapter match a fresh eight-mode compilation.'
    }
    else
    {
        $publicationFiles = @()
        foreach ($record in $compiled)
        {
            $publicationFiles += [pscustomobject]@{
                Source = $record.IncludePath
                Destination = Join-Path $artifactDirectory "$($record.Row.key).inc"
            }
        }
        $publicationFiles += [pscustomobject]@{ Source = $stagedCatalog; Destination = $catalogPath }
        $publicationFiles += [pscustomobject]@{ Source = $stagedHeader; Destination = $headerPath }
        Publish-ComputeFiles -Files $publicationFiles
        Write-Output 'Frozen eight RayQuery compute artifacts, provenance catalog, and generated adapter.'
    }
}
finally
{
    if (Test-Path -LiteralPath $runRoot -PathType Container)
    {
        $resolvedOutputRoot = (Resolve-Path -LiteralPath $outputRoot).Path
        $resolvedRunRoot = (Resolve-Path -LiteralPath $runRoot).Path
        $resolvedRunParent = Split-Path -Parent $resolvedRunRoot
        Assert-ComputeTrue (-not $resolvedRunRoot.Equals(
                $resolvedOutputRoot, [StringComparison]::OrdinalIgnoreCase) -and
            $resolvedRunParent.Equals(
                $resolvedOutputRoot, [StringComparison]::OrdinalIgnoreCase)) `
            "Refusing to recursively remove compute staging outside its exact output parent: $resolvedRunRoot"
        Remove-Item -LiteralPath $resolvedRunRoot -Recurse -Force
    }
}
