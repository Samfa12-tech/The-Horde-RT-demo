param(
    [string]$VulkanSdk = $env:VULKAN_SDK,
    [switch]$Check,
    [string]$OutputDirectory,
    [string]$EmbeddedIncludePath,
    [switch]$Legacy
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
    return [Convert]::ToHexString(
        [Security.Cryptography.SHA256]::HashData(
            [Text.Encoding]::UTF8.GetBytes($canonicalText))).ToLowerInvariant()
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
        path = [IO.Path]::GetRelativePath($repoRoot, $dependency).Replace('\', '/')
        sha256 = Get-RaygenDependencyHash -Path $dependency
    }
}
$dependencyManifest = $dependencyHashes | ConvertTo-Json -Compress
$dependencyIdentity = "$variantName|$dependencyManifest"
$dependencyHash = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($dependencyIdentity))).ToLowerInvariant()

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
$spirvHash = (Get-FileHash -LiteralPath $spirv -Algorithm SHA256).Hash.ToLowerInvariant()
$sourceHash = $dependencyHash
$includeHash = if (Test-Path -LiteralPath $include) {
    (Get-FileHash -LiteralPath $include -Algorithm SHA256).Hash.ToLowerInvariant()
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
