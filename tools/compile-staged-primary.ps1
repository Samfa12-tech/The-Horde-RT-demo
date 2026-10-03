param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Shipping', 'Diagnostic')]
    [string]$Instrumentation,
    [Parameter(Mandatory = $true)]
    [ValidateSet('Mobile', 'High')]
    [string]$Quality,
    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory,
    [string]$VulkanSdk = $env:VULKAN_SDK,
    [switch]$Rebuild
)

$ErrorActionPreference = 'Stop'
$script:StagedPrimaryRepoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$script:StagedPrimaryPasses = @(
    [pscustomobject]@{ Name = 'primary'; Value = 1 },
    [pscustomobject]@{ Name = 'shade'; Value = 2 }
)

function Get-StagedPrimarySha256
{
    param([Parameter(Mandatory = $true)][byte[]]$Bytes)
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($sha.ComputeHash($Bytes))).Replace('-', '').ToLowerInvariant() }
    finally { $sha.Dispose() }
}

function Get-StagedPrimaryToolInfo
{
    param([Parameter(Mandatory = $true)][string]$Path)
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = $Path
    $start.Arguments = '--version'
    $start.UseShellExecute = $false
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $start.CreateNoWindow = $true
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $start
    if (-not $process.Start()) { throw "Could not start version query: $Path" }
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    $version = (($stdoutTask.Result + ' ' + $stderrTask.Result) -replace '\s+', ' ').Trim()
    if ($process.ExitCode -ne 0 -or [string]::IsNullOrWhiteSpace($version)) { throw "Could not determine tool version for $Path (exit $($process.ExitCode))." }
    return [ordered]@{ path = $Path; version = $version; sha256 = Get-StagedPrimarySha256 ([IO.File]::ReadAllBytes($Path)) }
}

function Get-StagedPrimaryRepoRelativePath
{
    param([Parameter(Mandatory = $true)][string]$Path)
    $fullPath = [IO.Path]::GetFullPath($Path)
    $prefix = $script:StagedPrimaryRepoRoot.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    if (-not $fullPath.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase))
    {
        throw "Shader include escapes repository root: $fullPath"
    }
    return $fullPath.Substring($prefix.Length)
}

function Resolve-StagedPrimaryIncludes
{
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][Collections.Generic.HashSet[string]]$ActivePaths,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][Collections.Generic.List[string]]$Dependencies
    )

    $fullPath = [IO.Path]::GetFullPath($Path)
    [void](Get-StagedPrimaryRepoRelativePath $fullPath)
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) { throw "Shader include was not found: $fullPath" }
    if (-not $ActivePaths.Add($fullPath)) { throw "Shader include cycle detected: $fullPath" }
    $Dependencies.Add($fullPath)

    $builder = [Text.StringBuilder]::new()
    foreach ($line in [IO.File]::ReadAllLines($fullPath))
    {
        if ($line -match '^\s*#include\s+"([^"]+)"\s*$')
        {
            $child = [IO.Path]::GetFullPath((Join-Path (Split-Path -Parent $fullPath) $Matches[1]))
            [void](Get-StagedPrimaryRepoRelativePath $child)
            [void]$builder.Append((Resolve-StagedPrimaryIncludes -Path $child -ActivePaths $ActivePaths -Dependencies $Dependencies))
        }
        else { [void]$builder.AppendLine($line) }
    }
    [void]$ActivePaths.Remove($fullPath)
    return $builder.ToString()
}

function Add-StagedPrimaryVariantConfiguration
{
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$InstrumentationName,
        [Parameter(Mandatory = $true)][string]$QualityName,
        [Parameter(Mandatory = $true)][int]$MaterialValue,
        [Parameter(Mandatory = $true)][int]$PassValue
    )

    $values = @{ Shipping = 0; Diagnostic = 1; Mobile = 0; High = 1 }
    $lines = $Source -split '\r?\n'
    $insert = 0
    while ($insert -lt $lines.Count -and ($lines[$insert] -match '^\s*$' -or $lines[$insert] -match '^\s*#(?:version|extension)\b')) { $insert++ }
    if ($insert -eq 0 -or $insert -ge $lines.Count) { throw 'Resolved raygen has no legal configuration insertion point.' }
    $prefix = @($lines[0..($insert - 1)])
    $suffix = @($lines[$insert..($lines.Count - 1)])
    $configPath = Join-Path $script:StagedPrimaryRepoRoot 'shaders/raytracing/include/rt_variant_config.glsl'
    if (-not (Test-Path -LiteralPath $configPath -PathType Leaf)) { throw "Variant configuration is missing: $configPath" }
    $definitions = @(
        '// Temporary staged-primary variant configuration; investigation only.',
        ('#define HORDE_RT_VARIANT_INSTRUMENTATION {0}' -f $values[$InstrumentationName]),
        ('#define HORDE_RT_VARIANT_QUALITY {0}' -f $values[$QualityName]),
        ('#define HORDE_RT_VARIANT_MATERIAL {0}' -f $MaterialValue),
        ('#define HORDE_RT_STAGED_PRIMARY_PASS {0}' -f $PassValue),
        [IO.File]::ReadAllText($configPath).TrimEnd("`r", "`n")
    )
    return [string]::Join("`n", @($prefix + $definitions + $suffix)) + "`n"
}

function Find-StagedPrimaryMainBody
{
    param([Parameter(Mandatory = $true)][string]$Source)
    $matches = [regex]::Matches($Source, '(?m)^void\s+main\s*\(\s*\)\s*\{')
    if ($matches.Count -ne 1) { throw "Expected one void main body, found $($matches.Count)." }
    $match = $matches[0]
    $depth = 0
    for ($i = $match.Index + $match.Length - 1; $i -lt $Source.Length; $i++)
    {
        if ($Source[$i] -eq '{') { $depth++ }
        elseif ($Source[$i] -eq '}')
        {
            $depth--
            if ($depth -eq 0) { return [pscustomobject]@{ Start = $match.Index; Open = $match.Index + $match.Length - 1; End = $i } }
        }
    }
    throw 'The raygen main body has unbalanced braces.'
}

function Replace-StagedPrimaryRange
{
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$StartAnchor,
        [Parameter(Mandatory = $true)][string]$EndAnchor,
        [Parameter(Mandatory = $true)][string]$Replacement,
        [Parameter(Mandatory = $true)][string]$Label
    )
    $startMatches = [regex]::Matches($Source, [regex]::Escape($StartAnchor))
    $endMatches = [regex]::Matches($Source, [regex]::Escape($EndAnchor))
    if ($startMatches.Count -ne 1) { throw "$Label expected one start anchor, found $($startMatches.Count)." }
    if ($endMatches.Count -ne 1) { throw "$Label expected one end anchor, found $($endMatches.Count)." }
    $start = $startMatches[0].Index
    $end = $endMatches[0].Index
    if ($end -le $start) { throw "$Label anchors are out of order." }
    return $Source.Substring(0, $start) + $Replacement + $Source.Substring($end + $EndAnchor.Length)
}

function New-StagedPrimaryPassSource
{
    param(
        [Parameter(Mandatory = $true)][string]$ResolvedSource,
        [Parameter(Mandatory = $true)][string]$RecordSource,
        [Parameter(Mandatory = $true)][ValidateSet('primary', 'shade')][string]$Pass
    )
    $recordOccurrences = ([regex]::Matches($ResolvedSource, [regex]::Escape('void storeStagedPrimary(HitInfo h)'))).Count
    if ($recordOccurrences -ne 0) { throw 'Staged record declarations were already present in resolved raygen.' }
    $main = Find-StagedPrimaryMainBody $ResolvedSource
    $withRecord = $ResolvedSource.Insert($main.Start, $RecordSource.TrimEnd("`r", "`n") + "`n`n")
    $main = Find-StagedPrimaryMainBody $withRecord
    $body = $withRecord.Substring($main.Start, $main.End - $main.Start + 1)
    $colorAnchor = '    vec3 color = shadePrimary(primary, rayDirection);'

    if ($Pass -eq 'primary')
    {
        $count = ([regex]::Matches($body, [regex]::Escape($colorAnchor))).Count
        if ($count -ne 1) { throw "Primary pass expected one shade anchor, found $count." }
        $start = $body.IndexOf($colorAnchor, [StringComparison]::Ordinal)
        $body = $body.Substring(0, $start) + "    storeStagedPrimary(primary);`n}"
    }
    else
    {
        $traceAnchor = '    HitInfo primary = traceScene(origin, rayDirection, 10000.0, primaryMask,'
        $count = ([regex]::Matches($body, [regex]::Escape($traceAnchor))).Count
        if ($count -ne 1) { throw "Shade pass expected one primary trace anchor, found $count." }
        $colorCount = ([regex]::Matches($body, [regex]::Escape($colorAnchor))).Count
        if ($colorCount -ne 1) { throw "Shade pass expected one shade anchor, found $colorCount." }
        $start = $body.IndexOf($traceAnchor, [StringComparison]::Ordinal)
        $color = $body.IndexOf($colorAnchor, [StringComparison]::Ordinal)
        if ($color -le $start) { throw 'Shade pass primary trace/diagnostic block is missing or reordered.' }
        $traceText = $body.Substring($start, $color - $start)
        if ($traceText -notmatch '(?s)HitInfo primary = traceScene\(.*?\);\s*if\s*\(primary\.hit\).*?\}')
        { throw 'Shade pass refused to replace an incomplete primary trace/diagnostic block.' }
        # The end anchor is unique and the prefix/suffix are retained byte-for-byte.
        $body = $body.Substring(0, $start) + "    HitInfo primary = loadStagedPrimary();`n" + $body.Substring($color)
    }
    $updated = $withRecord.Substring(0, $main.Start) + $body + $withRecord.Substring($main.End + 1)
    $updatedMain = Find-StagedPrimaryMainBody $updated
    $updatedBody = $updated.Substring($updatedMain.Start, $updatedMain.End - $updatedMain.Start + 1)
    if ($updatedBody -notmatch 'vec3 origin\s*=') { throw "$Pass pass unexpectedly removed camera/ray setup." }
    if ($Pass -eq 'primary' -and $updatedBody -match 'integrateFireEmitters\s*\(') { throw 'Primary pass unexpectedly retained downstream shading work.' }
    if ($Pass -eq 'shade' -and $updatedBody -notmatch 'shadePrimary\(primary, rayDirection\)') { throw 'Shade pass unexpectedly removed shading/output work.' }
    return $updated
}

function Get-StagedPrimaryDescriptorBindings
{
    param([Parameter(Mandatory = $true)][string]$AssemblyPath)
    $assembly = [IO.File]::ReadAllText($AssemblyPath)
    $sets = @{}
    $bindings = @{}
    foreach ($match in [regex]::Matches($assembly, '(?m)^\s*OpDecorate\s+(%\S+)\s+DescriptorSet\s+(\d+)\s*$')) { $sets[$match.Groups[1].Value] = [int]$match.Groups[2].Value }
    foreach ($match in [regex]::Matches($assembly, '(?m)^\s*OpDecorate\s+(%\S+)\s+Binding\s+(\d+)\s*$')) { $bindings[$match.Groups[1].Value] = [int]$match.Groups[2].Value }
    $found = @()
    foreach ($id in $sets.Keys)
    {
        if ($sets[$id] -eq 1 -and $bindings.ContainsKey($id)) { $found += [pscustomobject]@{ Id = $id; Set = $sets[$id]; Binding = $bindings[$id] } }
    }
    $actual = @($found | ForEach-Object { [int]$_.Binding } | Sort-Object -Unique)
    if (($actual -join ',') -ne '0,1,2') { throw "Compiled staged shader descriptor bindings were '$($actual -join ',')', expected set 1 bindings 0,1,2." }
    return $found
}

function New-StagedPrimaryHeader
{
    param([Parameter(Mandatory = $true)][object[]]$Modules, [string]$InstrumentationName, [string]$QualityName)
    $instrumentationValue = if ($InstrumentationName -eq 'Shipping') { 0 } else { 1 }
    $qualityValue = if ($QualityName -eq 'Mobile') { 0 } else { 1 }
    $builder = [Text.StringBuilder]::new()
    [void]$builder.AppendLine('#pragma once')
    [void]$builder.AppendLine('#include <array>')
    [void]$builder.AppendLine('#include <cstdint>')
    [void]$builder.AppendLine('#include <span>')
    [void]$builder.AppendLine('#include <string_view>')
    [void]$builder.AppendLine('namespace horde::vulkan::raytracing::experimental {')
    [void]$builder.AppendLine('inline constexpr std::uint32_t kStagedPrimaryDescriptorSet = 1;')
    [void]$builder.AppendLine('inline constexpr std::uint32_t kStagedPrimaryPage0Binding = 0;')
    [void]$builder.AppendLine('inline constexpr std::uint32_t kStagedPrimaryPage1Binding = 1;')
    [void]$builder.AppendLine('inline constexpr std::uint32_t kStagedPrimaryPage2Binding = 2;')
    [void]$builder.AppendLine('inline constexpr std::uint32_t kStagedPrimaryPageCount = 3;')
    [void]$builder.AppendLine('inline constexpr std::uint32_t kStagedPrimaryRecordBytes = 128;')
    [void]$builder.AppendLine('static_assert(kStagedPrimaryDescriptorSet == 1 && kStagedPrimaryPage0Binding == 0 && kStagedPrimaryPage1Binding == 1 && kStagedPrimaryPage2Binding == 2);')
    [void]$builder.AppendLine('static_assert(kStagedPrimaryPageCount == 3 && kStagedPrimaryRecordBytes == 128);')
    [void]$builder.AppendLine(('inline constexpr std::uint32_t kInstrumentation = {0};' -f $instrumentationValue))
    [void]$builder.AppendLine(('inline constexpr std::uint32_t kQuality = {0};' -f $qualityValue))

    foreach ($module in $Modules)
    {
        $arrayName = $module.ArrayName
        $words = $module.Words
        [void]$builder.Append(('inline constexpr std::array<std::uint32_t, {0}> {1}{{' -f $words.Count, $arrayName))
        for ($i = 0; $i -lt $words.Count; $i++)
        {
            if (($i % 8) -eq 0) { [void]$builder.Append("`n    ") }
            [void]$builder.Append(('0x{0:x8}u' -f $words[$i]))
            if ($i -lt ($words.Count - 1)) { [void]$builder.Append(', ') }
        }
        [void]$builder.AppendLine("`n};")
        $hashName = $arrayName -replace 'Words$', ''
        [void]$builder.AppendLine(('inline constexpr std::string_view {0}Sha256 = "{1}";' -f $hashName, $module.Sha256))
        $spanName = $arrayName -replace '^k', 'Get'
        [void]$builder.AppendLine(('inline std::span<const std::uint32_t> {0}() noexcept {{ return {1}; }}' -f $spanName, $arrayName))
    }
    foreach ($strategy in @('Opaque', 'Generic'))
    {
        $primary = if ($strategy -eq 'Opaque') { 'kPrimaryOpaque' } else { 'kPrimaryGeneric' }
        $shade = if ($strategy -eq 'Opaque') { 'kShadeOpaque' } else { 'kShadeGeneric' }
        $pairSha = if ($strategy -eq 'Opaque') { $script:StagedPrimaryOpaquePairSha } else { $script:StagedPrimaryGenericPairSha }
        $strategyName = if ($strategy -eq 'Opaque') { 'opaque_fast' } else { 'generic_dielectric' }
        $keyStrategy = $strategyName
        [void]$builder.AppendLine(('inline constexpr std::string_view k{0}PairSha256 = "{1}";' -f $strategy, $pairSha))
        [void]$builder.AppendLine(('inline constexpr std::string_view k{0}PairKey = "staged_primary_v1_{1}_{2}_{3}";' -f $strategy, $InstrumentationName.ToLowerInvariant(), $QualityName.ToLowerInvariant(), $keyStrategy))
    }
    [void]$builder.AppendLine('}')
    return $builder.ToString()
}

function Invoke-StagedPrimaryCompilation
{
    param([string]$InstrumentationName, [string]$QualityName, [string]$Destination, [string]$SdkRoot, [switch]$AllowRebuild)

    $output = [IO.Path]::GetFullPath($Destination)
    if ([string]::IsNullOrWhiteSpace($Destination) -or -not [IO.Path]::IsPathRooted($Destination)) { throw 'OutputDirectory must be an absolute path.' }
    $repoPrefix = $script:StagedPrimaryRepoRoot.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    if ($output.Equals($script:StagedPrimaryRepoRoot, [StringComparison]::OrdinalIgnoreCase) -or
        $output.StartsWith($repoPrefix, [StringComparison]::OrdinalIgnoreCase))
    { throw 'OutputDirectory must remain external to the repository.' }
    $knownFiles = @('staged-primary-manifest.json', 'StagedPrimaryShaders.generated.h')
    foreach ($strategy in @('opaque', 'generic')) { foreach ($pass in @('primary', 'shade')) { $knownFiles += "$strategy-$pass.rgen"; $knownFiles += "$strategy-$pass.rgen.spv"; $knownFiles += "$strategy-$pass.rgen.spvasm" } }
    if (Test-Path -LiteralPath $output)
    {
        if (-not $AllowRebuild) { throw "Output directory already exists; preserving it: $output" }
        if (-not (Test-Path -LiteralPath $output -PathType Container)) { throw "Output path exists and is not a directory: $output" }
        $existing = @(Get-ChildItem -LiteralPath $output -Force -Recurse)
        if (@($existing | Where-Object { $_.PSIsContainer -or $_.Name -notin $knownFiles }).Count -ne 0) { throw 'Rebuild refused: output contains unexpected files or directories.' }
        foreach ($item in $existing) { Remove-Item -LiteralPath $item.FullName -Force }
    }
    else { [void](New-Item -ItemType Directory -Path $output) }

    if ([string]::IsNullOrWhiteSpace($SdkRoot)) { $SdkRoot = 'C:\VulkanSDK\1.4.350.0' }
    $bin = Join-Path $SdkRoot 'Bin'
    $glslang = Join-Path $bin 'glslangValidator.exe'
    $optimizer = Join-Path $bin 'spirv-opt.exe'
    $validator = Join-Path $bin 'spirv-val.exe'
    $disassembler = Join-Path $bin 'spirv-dis.exe'
    foreach ($tool in @($glslang, $optimizer, $validator, $disassembler)) { if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) { throw "Required Vulkan shader tool is missing: $tool" } }

    $sourcePath = Join-Path $script:StagedPrimaryRepoRoot 'shaders/raytracing/minimal.rgen'
    $recordPath = Join-Path $script:StagedPrimaryRepoRoot 'shaders/raytracing/experimental/staged_primary_record.glsl'
    $configPath = Join-Path $script:StagedPrimaryRepoRoot 'shaders/raytracing/include/rt_variant_config.glsl'
    $active = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $deps = [Collections.Generic.List[string]]::new()
    $resolved = Resolve-StagedPrimaryIncludes -Path $sourcePath -ActivePaths $active -Dependencies $deps
    $record = [IO.File]::ReadAllText($recordPath)
    $materials = @([pscustomobject]@{ Name = 'opaque'; Label = 'OpaqueFast'; Value = 0 }, [pscustomobject]@{ Name = 'generic'; Label = 'GenericDielectric'; Value = 1 })
    $modules = @()
    $moduleRows = @()
    $configHash = Get-StagedPrimarySha256 ([IO.File]::ReadAllBytes($configPath))
    $recordHash = Get-StagedPrimarySha256 ([IO.File]::ReadAllBytes($recordPath))
    $toolchain = [ordered]@{
        glslangValidator = Get-StagedPrimaryToolInfo $glslang
        spirvOpt = Get-StagedPrimaryToolInfo $optimizer
        spirvVal = Get-StagedPrimaryToolInfo $validator
        spirvDis = Get-StagedPrimaryToolInfo $disassembler
    }

    foreach ($material in $materials)
    {
        $sourceForMaterial = $resolved
        if ($material.Value -eq 0)
        {
            $legacyDefine = '#define HORDE_GENERIC_TRANSMISSION_VARIANT 1'
            $count = ([regex]::Matches($sourceForMaterial, [regex]::Escape($legacyDefine))).Count
            if ($count -ne 1) { throw "OpaqueFast compatibility route expected exactly one generic define, found $count." }
            $sourceForMaterial = $sourceForMaterial.Replace($legacyDefine, '#define HORDE_GENERIC_TRANSMISSION_VARIANT 0')
        }
        foreach ($pass in $script:StagedPrimaryPasses)
        {
            $variant = Add-StagedPrimaryVariantConfiguration -Source $sourceForMaterial -InstrumentationName $InstrumentationName -QualityName $QualityName -MaterialValue $material.Value -PassValue $pass.Value
            $transformed = New-StagedPrimaryPassSource -ResolvedSource $variant -RecordSource $record -Pass $pass.Name
            $stem = "$($material.Name)-$($pass.Name).rgen"
            $sourceOut = Join-Path $output $stem
            [IO.File]::WriteAllText($sourceOut, $transformed, [Text.UTF8Encoding]::new($false))
            $spv = "$sourceOut.spv"
            $asm = "$sourceOut.spvasm"
            $compileArgs = @('-V', '--target-env', 'vulkan1.2')
            if ($material.Value -eq 0) { $compileArgs += '-Os' }
            $compileArgs += @('-S', 'rgen', '-o', $spv, $sourceOut)
            & $glslang @compileArgs
            if ($LASTEXITCODE -ne 0) { throw "glslang failed for $stem with exit $LASTEXITCODE." }
            $optimized = "$spv.optimized"
            if ($material.Value -eq 0) { & $optimizer -O $spv -o $optimized }
            else { & $optimizer --eliminate-dead-functions --eliminate-dead-code-aggressive --simplify-instructions --eliminate-dead-branches --cfg-cleanup $spv -o $optimized }
            if ($LASTEXITCODE -ne 0) { throw "spirv-opt failed for $stem with exit $LASTEXITCODE." }
            [IO.File]::Copy($optimized, $spv, $true)
            Remove-Item -LiteralPath $optimized -Force
            & $validator --target-env vulkan1.2 $spv
            if ($LASTEXITCODE -ne 0) { throw "spirv-val failed for $stem." }
            & $disassembler $spv -o $asm
            if ($LASTEXITCODE -ne 0) { throw "spirv-dis failed for $stem." }
            $bindings = @(Get-StagedPrimaryDescriptorBindings $asm)
            $bytes = [IO.File]::ReadAllBytes($spv)
            if (($bytes.Length % 4) -ne 0) { throw "SPIR-V is not word aligned: $spv" }
            $words = @(); for ($offset = 0; $offset -lt $bytes.Length; $offset += 4) { $words += [BitConverter]::ToUInt32($bytes, $offset) }
            $name = if ($material.Value -eq 0) { if ($pass.Name -eq 'primary') { 'kPrimaryOpaqueWords' } else { 'kShadeOpaqueWords' } } else { if ($pass.Name -eq 'primary') { 'kPrimaryGenericWords' } else { 'kShadeGenericWords' } }
            $module = [pscustomobject]@{ ArrayName = $name; Words = $words; Bytes = $bytes; Sha256 = (Get-StagedPrimarySha256 $bytes); File = (Split-Path -Leaf $spv); Source = (Split-Path -Leaf $sourceOut); Material = $material.Label; Pass = $pass.Name }
            $modules += $module
            $moduleRows += [ordered]@{ material = $material.Label; pass = $pass.Name; source = $module.Source; sourceSha256 = Get-StagedPrimarySha256 ([IO.File]::ReadAllBytes($sourceOut)); spirv = $module.File; spirvSha256 = $module.Sha256; bytes = $bytes.Length; words = $words.Count; descriptorBindingsSet1 = @($bindings | ForEach-Object { $_.Binding } | Sort-Object -Unique) }
        }
    }
    $opaquePrimary = @($modules | Where-Object ArrayName -eq 'kPrimaryOpaqueWords')[0].Bytes
    $opaqueShade = @($modules | Where-Object ArrayName -eq 'kShadeOpaqueWords')[0].Bytes
    $genericPrimary = @($modules | Where-Object ArrayName -eq 'kPrimaryGenericWords')[0].Bytes
    $genericShade = @($modules | Where-Object ArrayName -eq 'kShadeGenericWords')[0].Bytes
    $script:StagedPrimaryOpaquePairSha = Get-StagedPrimarySha256 ([byte[]]($opaquePrimary + $opaqueShade))
    $script:StagedPrimaryGenericPairSha = Get-StagedPrimarySha256 ([byte[]]($genericPrimary + $genericShade))
    $header = New-StagedPrimaryHeader -Modules $modules -InstrumentationName $InstrumentationName -QualityName $QualityName
    [IO.File]::WriteAllText((Join-Path $output 'StagedPrimaryShaders.generated.h'), $header, [Text.UTF8Encoding]::new($false))
    $depsRows = foreach ($dep in $deps) { [ordered]@{ path = (Get-StagedPrimaryRepoRelativePath $dep).Replace('\', '/'); sha256 = Get-StagedPrimarySha256 ([IO.File]::ReadAllBytes($dep)) } }
    $manifest = [ordered]@{
        schema = 1; classification = 'investigation-only-staged-primary'; instrumentation = $InstrumentationName; quality = $QualityName; stage = 'rgen'; targetEnvironment = 'vulkan1.2';
        generator = [ordered]@{ path = 'tools/compile-staged-primary.ps1'; sha256 = Get-StagedPrimarySha256 ([IO.File]::ReadAllBytes($PSCommandPath)) }; toolchain = $toolchain;
        compilerRecipe = [ordered]@{
            opaque = [ordered]@{ glslang = @('-V', '--target-env', 'vulkan1.2', '-Os', '-S', 'rgen'); spirvOpt = @('-O') };
            generic = [ordered]@{ glslang = @('-V', '--target-env', 'vulkan1.2', '-S', 'rgen'); spirvOpt = @('--eliminate-dead-functions', '--eliminate-dead-code-aggressive', '--simplify-instructions', '--eliminate-dead-branches', '--cfg-cleanup') }
        };
        source = [ordered]@{ path = 'shaders/raytracing/minimal.rgen'; dependencies = @($depsRows) }; recordIncludeSha256 = $recordHash; variantConfigSha256 = $configHash;
        passContract = [ordered]@{ primary = 'camera + primary trace + primary diagnostics + storeStagedPrimary'; shade = 'camera + loadStagedPrimary + original shading/presentation'; descriptorSet = 1; bindings = @(0, 1, 2); pages = 3; bytesPerRecord = 128 };
        pairs = @([ordered]@{ key = "staged_primary_v1_$($InstrumentationName.ToLowerInvariant())_$($QualityName.ToLowerInvariant())_opaque_fast"; strategy = 'opaque-fast'; sha256 = $script:StagedPrimaryOpaquePairSha }, [ordered]@{ key = "staged_primary_v1_$($InstrumentationName.ToLowerInvariant())_$($QualityName.ToLowerInvariant())_generic_dielectric"; strategy = 'generic-dielectric'; sha256 = $script:StagedPrimaryGenericPairSha });
        modules = @($moduleRows)
    }
    [IO.File]::WriteAllText((Join-Path $output 'staged-primary-manifest.json'), (($manifest | ConvertTo-Json -Depth 12) + "`n"), [Text.UTF8Encoding]::new($false))
    Write-Output "Staged-primary artifacts: $output"
    Write-Output ("Policy: {0}/{1}; four Vulkan 1.2 raygen modules compiled, optimized, validated and disassembled." -f $InstrumentationName, $QualityName)
    Write-Output ("Opaque pair SHA-256: {0}; Generic pair SHA-256: {1}" -f $script:StagedPrimaryOpaquePairSha, $script:StagedPrimaryGenericPairSha)
}

if ($MyInvocation.InvocationName -ne '.')
{
    Invoke-StagedPrimaryCompilation -InstrumentationName $Instrumentation -QualityName $Quality -Destination $OutputDirectory -SdkRoot $VulkanSdk -AllowRebuild:$Rebuild
}
