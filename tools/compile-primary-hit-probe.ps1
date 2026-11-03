param(
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [string]$VulkanSdk = $env:VULKAN_SDK
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$output = [IO.Path]::GetFullPath($OutputDirectory)
if ([string]::IsNullOrWhiteSpace($OutputDirectory) -or -not [IO.Path]::IsPathRooted($OutputDirectory)) { throw 'OutputDirectory must be an absolute path.' }
$repoPrefix = $repoRoot.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
if ($output.Equals($repoRoot, [StringComparison]::OrdinalIgnoreCase) -or $output.StartsWith($repoPrefix, [StringComparison]::OrdinalIgnoreCase)) { throw 'OutputDirectory must remain external to the repository.' }
if (Test-Path -LiteralPath $output) { throw "Output directory already exists; preserving it: $output" }

$stagedCompiler = Join-Path $PSScriptRoot 'compile-staged-primary.ps1'
if (-not (Test-Path -LiteralPath $stagedCompiler -PathType Leaf)) { throw "Required staged-primary compiler is missing: $stagedCompiler" }
# Dot-source only its guarded helper API; its normal entry point is suppressed by InvocationName '.'.
. $stagedCompiler -Instrumentation Diagnostic -Quality Mobile -OutputDirectory $output

if ([string]::IsNullOrWhiteSpace($VulkanSdk)) { $VulkanSdk = 'C:\VulkanSDK\1.4.350.0' }
$bin = Join-Path $VulkanSdk 'Bin'
$glslang = Join-Path $bin 'glslangValidator.exe'
$optimizer = Join-Path $bin 'spirv-opt.exe'
$validator = Join-Path $bin 'spirv-val.exe'
$disassembler = Join-Path $bin 'spirv-dis.exe'
foreach ($tool in @($glslang, $optimizer, $validator, $disassembler)) { if (-not (Test-Path -LiteralPath $tool -PathType Leaf)) { throw "Required Vulkan shader tool is missing: $tool" } }

function Get-ProbeSha256 {
    param([Parameter(Mandatory = $true)][byte[]]$Bytes)
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($sha.ComputeHash($Bytes))).Replace('-', '').ToLowerInvariant() }
    finally { $sha.Dispose() }
}

function Write-ProbeText {
    param([Parameter(Mandatory = $true)][string]$Path, [Parameter(Mandatory = $true)][string]$Text)
    [IO.File]::WriteAllText($Path, $Text, [Text.UTF8Encoding]::new($false))
}

function Add-ProbeBufferAndHelpers {
    param([Parameter(Mandatory = $true)][string]$Source, [Parameter(Mandatory = $true)][string]$ProbeSource)
    $fieldAnchor = '    RtDielectricDiagnostics value;'
    if ([regex]::Matches($Source, [regex]::Escape($fieldAnchor)).Count -ne 1) { throw 'Expected exactly one unchanged binding-22 RtDielectricDiagnostics value anchor.' }
    if ($Source -match 'probeWords|primaryHitProbeStore|primaryHitProbeOutput') { throw 'Probe declarations already exist in resolved source.' }
    $Source = $Source.Replace($fieldAnchor, $fieldAnchor + "`n    uint reserved[3];`n    uint probeWords[14400];")
    $mainAnchor = 'void main()'
    if ([regex]::Matches($Source, [regex]::Escape($mainAnchor)).Count -ne 1) { throw 'Expected one raygen main insertion point.' }
    return $Source.Replace($mainAnchor, $ProbeSource.TrimEnd("`r", "`n") + "`n`n" + $mainAnchor)
}

function Add-NormalProbeCalls {
    param([Parameter(Mandatory = $true)][string]$Source)
    $shadeAnchor = '    vec3 color = shadePrimary(primary, rayDirection);'
    if ([regex]::Matches($Source, [regex]::Escape($shadeAnchor)).Count -ne 1) { throw 'Normal raygen must contain exactly one original primary-shading anchor.' }
    $Source = $Source.Replace($shadeAnchor, '    primaryHitProbeStore(0u, primary, origin, rayDirection, primaryMask);' + "`n" + $shadeAnchor)
    $storeAnchor = '    imageStore(outputImage, ivec2(HORDE_RT_PIXEL_ID.xy), vec4(clamp(color, 0.0, 1.0), 1.0));'
    if ([regex]::Matches($Source, [regex]::Escape($storeAnchor)).Count -ne 1) { throw 'Normal raygen must contain exactly one final imageStore anchor.' }
    return $Source.Replace($storeAnchor, '    primaryHitProbeOutput(0u, vec4(clamp(color, 0.0, 1.0), 1.0));' + "`n" + $storeAnchor)
}

function Add-StagedProbeCalls {
    param([Parameter(Mandatory = $true)][string]$Source, [Parameter(Mandatory = $true)][ValidateSet('primary', 'shade')][string]$Pass)
    if ($Pass -eq 'primary') {
        $anchor = '    storeStagedPrimary(primary);'
        if ([regex]::Matches($Source, [regex]::Escape($anchor)).Count -ne 1) { throw 'Staged primary pass is missing its exact record store.' }
        return $Source.Replace($anchor, '    primaryHitProbeStore(0u, primary, origin, rayDirection, primaryMask);' + "`n" + $anchor)
    }
    $load = '    HitInfo primary = loadStagedPrimary();'
    if ([regex]::Matches($Source, [regex]::Escape($load)).Count -ne 1) { throw 'Staged shade pass is missing its exact record load.' }
    $Source = $Source.Replace($load, $load + "`n" + '    primaryHitProbeStore(1u, primary, origin, rayDirection, primaryMask);')
    $storeAnchor = '    imageStore(outputImage, ivec2(HORDE_RT_PIXEL_ID.xy), vec4(clamp(color, 0.0, 1.0), 1.0));'
    if ([regex]::Matches($Source, [regex]::Escape($storeAnchor)).Count -ne 1) { throw 'Staged shade pass must contain exactly one final imageStore anchor.' }
    return $Source.Replace($storeAnchor, '    primaryHitProbeOutput(1u, vec4(clamp(color, 0.0, 1.0), 1.0));' + "`n" + $storeAnchor)
}

function Get-ProbeAtomicInstructionCount {
    param([Parameter(Mandatory = $true)][string]$AssemblyPath)
    $assembly = [IO.File]::ReadAllText($AssemblyPath)
    return [regex]::Matches($assembly, '(?m)^\s*%\S+\s*=\s*OpAtomic\w+\b').Count
}

function Get-ProbeDescriptorBindings {
    param([Parameter(Mandatory = $true)][string]$AssemblyPath)
    $assembly = [IO.File]::ReadAllText($AssemblyPath)
    $sets = @{}
    $bindings = @{}
    foreach ($match in [regex]::Matches($assembly, '(?m)^\s*OpDecorate\s+(%\S+)\s+DescriptorSet\s+(\d+)\s*$')) { $sets[$match.Groups[1].Value] = [int]$match.Groups[2].Value }
    foreach ($match in [regex]::Matches($assembly, '(?m)^\s*OpDecorate\s+(%\S+)\s+Binding\s+(\d+)\s*$')) { $bindings[$match.Groups[1].Value] = [int]$match.Groups[2].Value }
    $found = @()
    foreach ($id in $sets.Keys) { if ($bindings.ContainsKey($id)) { $found += [pscustomobject]@{ Set = $sets[$id]; Binding = $bindings[$id] } } }
    $duplicates = @($found | Group-Object { '{0}:{1}' -f $_.Set, $_.Binding } | Where-Object Count -ne 1)
    if ($duplicates.Count -ne 0) { throw "Duplicate SPIR-V descriptor binding in $AssemblyPath." }
    if (@($found | Where-Object Set -gt 1).Count -ne 0) { throw "Unexpected descriptor set above 1 in $AssemblyPath." }
    if (@($found | Where-Object { $_.Set -eq 0 -and $_.Binding -eq 22 }).Count -ne 1) { throw "Binding 22 probe/diagnostics storage missing in $AssemblyPath." }
    return $found
}

function Compile-ProbeModule {
    param(
        [Parameter(Mandatory = $true)][string]$SourceText,
        [Parameter(Mandatory = $true)][string]$Stem,
        [Parameter(Mandatory = $true)][int]$MaterialValue,
        [Parameter(Mandatory = $true)][string]$Destination,
        [switch]$ExpectStaged,
        [switch]$ExpectProbeBuffer
    )
    $sourcePath = Join-Path $Destination "$Stem.rgen"
    Write-ProbeText -Path $sourcePath -Text $SourceText
    $spvPath = "$sourcePath.spv"
    $assemblyPath = "$sourcePath.spvasm"
    $compileArgs = @('-V', '--target-env', 'vulkan1.2')
    if ($MaterialValue -eq 0) { $compileArgs += '-Os' }
    $compileArgs += @('-S', 'rgen', '-o', $spvPath, $sourcePath)
    $null = & $glslang @compileArgs
    if ($LASTEXITCODE -ne 0) { throw "glslang failed for $Stem with exit $LASTEXITCODE." }
    $optimized = "$spvPath.optimized"
    if ($MaterialValue -eq 0) { $null = & $optimizer -O $spvPath -o $optimized }
    else { $null = & $optimizer --eliminate-dead-functions --eliminate-dead-code-aggressive --simplify-instructions --eliminate-dead-branches --cfg-cleanup $spvPath -o $optimized }
    if ($LASTEXITCODE -ne 0) { throw "spirv-opt failed for $Stem with exit $LASTEXITCODE." }
    [IO.File]::Copy($optimized, $spvPath, $true)
    Remove-Item -LiteralPath $optimized -Force
    $null = & $validator --target-env vulkan1.2 $spvPath
    if ($LASTEXITCODE -ne 0) { throw "spirv-val failed for $Stem." }
    $null = & $disassembler $spvPath -o $assemblyPath
    if ($LASTEXITCODE -ne 0) { throw "spirv-dis failed for $Stem." }
    $bindings = @(Get-ProbeDescriptorBindings $assemblyPath)
    if ($ExpectProbeBuffer) {
        foreach ($layoutDecoration in @(
            'OpMemberDecorate %RtDielectricDiagnosticsBuffer 0 Offset 0',
            'OpMemberDecorate %RtDielectricDiagnosticsBuffer 1 Offset 164',
            'OpMemberDecorate %RtDielectricDiagnosticsBuffer 2 Offset 176',
            'OpDecorate %rtDielectricDiagnostics Binding 22',
            'OpMemberName %RtDielectricDiagnosticsBuffer 2 "probeWords"'
        )) {
            if ([regex]::Matches([IO.File]::ReadAllText($assemblyPath), [regex]::Escape($layoutDecoration)).Count -ne 1) { throw "Probe storage-buffer ABI mismatch in ${Stem}: $layoutDecoration" }
        }
    }
    if ($ExpectStaged) {
        $stagedBindings = @($bindings | Where-Object Set -eq 1 | ForEach-Object Binding | Sort-Object -Unique)
        if (($stagedBindings -join ',') -ne '0,1,2') { throw "Staged module $Stem must retain exactly set-1 bindings 0,1,2." }
    } elseif (@($bindings | Where-Object Set -eq 1).Count -ne 0) { throw "Normal module $Stem unexpectedly declares a set-1 staged record." }
    $bytes = [IO.File]::ReadAllBytes($spvPath)
    if (($bytes.Length % 4) -ne 0) { throw "SPIR-V is not word aligned: $spvPath" }
    $words = [uint32[]]::new([int]($bytes.Length / 4))
    for ($wordIndex = 0; $wordIndex -lt $words.Length; $wordIndex++) { $words[$wordIndex] = [BitConverter]::ToUInt32($bytes, $wordIndex * 4) }
    return [pscustomobject]@{
        Stem = $Stem; Source = (Split-Path -Leaf $sourcePath); SourceSha256 = Get-ProbeSha256 ([IO.File]::ReadAllBytes($sourcePath))
        Spv = (Split-Path -Leaf $spvPath); Assembly = (Split-Path -Leaf $assemblyPath); Bytes = $bytes; Words = $words
        Sha256 = Get-ProbeSha256 $bytes; AtomicInstructions = Get-ProbeAtomicInstructionCount $assemblyPath
        DescriptorBindingsSet1 = @($bindings | Where-Object Set -eq 1 | ForEach-Object { [int]$_.Binding } | Sort-Object -Unique)
    }
}

function New-ProbeWordsInclude {
    param([Parameter(Mandatory = $true)][uint32[]]$Words, [Parameter(Mandatory = $true)][string]$Path)
    $builder = [Text.StringBuilder]::new()
    for ($i = 0; $i -lt $Words.Count; $i++) {
        if (($i % 8) -eq 0) { [void]$builder.Append("`n    ") }
        [void]$builder.Append(('0x{0:x8}u' -f $Words[$i]))
        if ($i -lt $Words.Count - 1) { [void]$builder.Append(', ') }
    }
    [void]$builder.AppendLine()
    Write-ProbeText -Path $Path -Text $builder.ToString()
}

function Add-PrimaryHitProbeConstants {
    param([Parameter(Mandatory = $true)][string]$Header)
    $close = $Header.LastIndexOf('}')
    if ($close -lt 0) { throw 'Could not find generated staged header namespace close.' }
    $constants = @(
        'inline constexpr bool kPrimaryHitProbeEnabled = true;',
        'inline constexpr std::uint32_t kPrimaryHitProbeBufferBytes = 57776;',
        'inline constexpr std::uint32_t kPrimaryHitProbeRecordWords = 96;',
        'inline constexpr std::uint32_t kPrimaryHitProbeRecordCount = 75;',
        'inline constexpr std::uint32_t kPrimaryHitProbePhases = 2;'
    ) -join "`n"
    return $Header.Substring(0, $close) + $constants + [Environment]::NewLine + $Header.Substring($close)
}

$probePath = Join-Path $repoRoot 'shaders/raytracing/experimental/primary_hit_probe.glsl'
$sourcePath = Join-Path $repoRoot 'shaders/raytracing/minimal.rgen'
$recordPath = Join-Path $repoRoot 'shaders/raytracing/experimental/staged_primary_record.glsl'
$configPath = Join-Path $repoRoot 'shaders/raytracing/include/rt_variant_config.glsl'
foreach ($path in @($probePath, $sourcePath, $recordPath, $configPath)) { if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required probe input is missing: $path" } }

$probeSource = [IO.File]::ReadAllText($probePath)
$recordSource = [IO.File]::ReadAllText($recordPath)
$probeHash = Get-ProbeSha256 ([IO.File]::ReadAllBytes($probePath))
$recordHash = Get-ProbeSha256 ([IO.File]::ReadAllBytes($recordPath))
$probeLayoutChecks = @(
    'const uint kPrimaryHitProbeRecordWords = 96u;',
    'const uint kPrimaryHitProbeRecordCount = 75u;',
    'const uint kPrimaryHitProbeExtentX = 1080u;',
    'const uint kPrimaryHitProbeExtentY = 2235u;',
    'uint base = (phase * kPrimaryHitProbeRecordCount + slot) * kPrimaryHitProbeRecordWords;',
    'rtDielectricDiagnostics.probeWords[base + 4u] = kPrimaryHitProbeExtentX;',
    'rtDielectricDiagnostics.probeWords[base + 5u] = kPrimaryHitProbeExtentY;',
    'rtDielectricDiagnostics.probeWords[base + 80u] = floatBitsToUint(color.r);',
    'rtDielectricDiagnostics.probeWords[base + 83u] = floatBitsToUint(color.a);'
)
foreach ($layoutCheck in $probeLayoutChecks) {
    $expectedCount = if ($layoutCheck.StartsWith('uint base =', [StringComparison]::Ordinal)) { 2 } else { 1 }
    if ([regex]::Matches($probeSource, [regex]::Escape($layoutCheck)).Count -ne $expectedCount) { throw "Probe record schema/extent/offset contract changed: $layoutCheck" }
}
$active = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$dependencies = [Collections.Generic.List[string]]::new()
$resolved = Resolve-StagedPrimaryIncludes -Path $sourcePath -ActivePaths $active -Dependencies $dependencies
$shaderAnchor = 'layout(std430, set = 0, binding = 22) restrict buffer RtDielectricDiagnosticsBuffer'
if ([regex]::Matches($resolved, [regex]::Escape($shaderAnchor)).Count -ne 1) { throw 'Resolved source must contain exactly one canonical binding-22 diagnostics block.' }
if ([regex]::Matches($resolved, [regex]::Escape('RtDielectricDiagnostics value;')).Count -ne 1) { throw 'Resolved source diagnostics prefix changed; probe refused.' }
    $pushStart = $resolved.IndexOf('layout(push_constant) uniform SceneControls', [StringComparison]::Ordinal)
    $pushEnd = $resolved.IndexOf('} controls;', $pushStart, [StringComparison]::Ordinal)
    if ($pushStart -lt 0 -or $pushEnd -le $pushStart) { throw 'SceneControls push-constant block was not found.' }
    $pushText = $resolved.Substring($pushStart, $pushEnd - $pushStart)
    $pushFields = @([regex]::Matches($pushText, '(?m)^\s*float\s+(\w+);\s*$') | ForEach-Object { $_.Groups[1].Value })
    $expectedPushFields = @('yaw','pitch','torchLight','time','cameraX','cameraZ','walkAmount','outputRedBlueSwap','outputExposure','damageFlash','enemyKind','staffLightStrength','staffX','staffY','staffZ','finaleSkylightOpen','finaleDawnReveal','heldPropDepth','waterQuality','waterfallWidthScale','fogDensityScale','torchHueDegrees','torchIntensityScale','skylightHueDegrees','skylightIntensityScale','passageHueDegrees','passageIntensityScale','staffHueDegrees','staffIntensityScale','workloadPreset','genericTransmissionActive','guidanceLightStrength')
    if (($pushFields -join ',') -cne ($expectedPushFields -join ',')) { throw 'SceneControls is no longer the expected exact 32-float declaration order.' }
    $probePushMatch = [regex]::Match($probeSource, '(?s)float\s+pushValues\s*\[\s*32\s*\]\s*=\s*float\s*\[\s*32\s*\]\s*\((.*?)\);')
    if (-not $probePushMatch.Success) { throw 'Probe push-constant raw-word initializer was not found.' }
    $probePushFields = @([regex]::Matches($probePushMatch.Groups[1].Value, 'controls\.(\w+)') | ForEach-Object { $_.Groups[1].Value })
    if (($probePushFields -join ',') -cne ($expectedPushFields -join ',')) { throw 'Probe push-value raw-word mapping no longer matches all 32 source fields in declaration order.' }
    $diagStruct = [regex]::Match($resolved, '(?s)struct\s+RtDielectricDiagnostics\s*\{(.*?)\};')
    if (-not $diagStruct.Success) { throw 'RtDielectricDiagnostics type declaration not found.' }
    $diagUintFields = [regex]::Matches($diagStruct.Groups[1].Value, '(?m)^\s*uint\s+\w+\s*;').Count
    if ($diagUintFields -ne 41) { throw "RtDielectricDiagnostics prefix changed: expected 41 uint words/164 bytes, found $diagUintFields." }
    if (57776 -ne (176 + 75 * 2 * 96 * 4)) { throw 'Probe allocation byte arithmetic is inconsistent.' }
    for ($lane = 0; $lane -lt 8; $lane++) {
        $canonical = [regex]::Match($recordSource, "(?m)^\s*r\.lanes\[$lane\]\s*=\s*(.+);\s*$")
        $probeLane = [regex]::Match($probeSource, "(?m)^\s*lanes\[$lane\]\s*=\s*(.+);\s*$")
        if (-not $canonical.Success -or -not $probeLane.Success) { throw "Could not resolve source/record pack lane $lane." }
        $canonicalExpr = $canonical.Groups[1].Value -replace '\s+', ''
        $probeExpr = $probeLane.Groups[1].Value -replace '\s+', ''
        if ($canonicalExpr -cne $probeExpr) { throw "Probe pack lane $lane differs from staged_primary_record.glsl." }
    }
    foreach ($guardLine in @('uint instanceAndGuard = uint(h.instance);','spawn = h.dielectricSpawnPosition;','bias = h.dielectricSpawnMinimumNormalBias;','if (h.dielectricSpawnGuarded) instanceAndGuard |= 0x80000000u;')) {
        foreach ($sourceToCheck in @($recordSource,$probeSource)) {
            if ([regex]::Matches($sourceToCheck, [regex]::Escape($guardLine)).Count -ne 1) { throw "Probe instance/guard or spawn packing changed: $guardLine" }
        }
    }
$instrumentedResolved = Add-ProbeBufferAndHelpers -Source $resolved -ProbeSource $probeSource
$instrumentedResolved = Add-NormalProbeCalls $instrumentedResolved
$materials = @([pscustomobject]@{ Name = 'opaque'; Label = 'OpaqueFast'; Value = 0 }, [pscustomobject]@{ Name = 'generic'; Label = 'GenericDielectric'; Value = 1 })
$normalProbeModules = @()
$normalBaselineAtoms = @{}
$normalProbeAtoms = @{}

[void](New-Item -ItemType Directory -Path $output)
$baselineDirectory = Join-Path $output '_normal-baseline'
$stagedDirectory = $output
[void](New-Item -ItemType Directory -Path $baselineDirectory)

$toolchain = [ordered]@{
    glslangValidator = Get-StagedPrimaryToolInfo $glslang
    spirvOpt = Get-StagedPrimaryToolInfo $optimizer
    spirvVal = Get-StagedPrimaryToolInfo $validator
    spirvDis = Get-StagedPrimaryToolInfo $disassembler
}

foreach ($material in $materials) {
    $sourceForMaterial = $resolved
    $probeForMaterial = $instrumentedResolved
    if ($material.Value -eq 0) {
        $legacyDefine = '#define HORDE_GENERIC_TRANSMISSION_VARIANT 1'
        if ([regex]::Matches($sourceForMaterial, [regex]::Escape($legacyDefine)).Count -ne 1) { throw 'Opaque compatibility route expected exactly one generic-variant define.' }
        $sourceForMaterial = $sourceForMaterial.Replace($legacyDefine, '#define HORDE_GENERIC_TRANSMISSION_VARIANT 0')
        $probeForMaterial = $probeForMaterial.Replace($legacyDefine, '#define HORDE_GENERIC_TRANSMISSION_VARIANT 0')
    }
    $baseVariant = Add-StagedPrimaryVariantConfiguration -Source $sourceForMaterial -InstrumentationName Diagnostic -QualityName Mobile -MaterialValue $material.Value -PassValue 0
    $probeVariant = Add-StagedPrimaryVariantConfiguration -Source $probeForMaterial -InstrumentationName Diagnostic -QualityName Mobile -MaterialValue $material.Value -PassValue 0
    $baseline = Compile-ProbeModule -SourceText $baseVariant -Stem "baseline-$($material.Name)" -MaterialValue $material.Value -Destination $baselineDirectory
    $normal = Compile-ProbeModule -SourceText $probeVariant -Stem "normal-$($material.Name)-probe" -MaterialValue $material.Value -Destination $output -ExpectProbeBuffer
    if ($baseline.AtomicInstructions -ne $normal.AtomicInstructions) { throw "Probe changed Diagnostic atomics for $($material.Label): baseline $($baseline.AtomicInstructions), probe $($normal.AtomicInstructions)." }
    $normalBaselineAtoms[$material.Name] = $baseline.AtomicInstructions
    $normalProbeAtoms[$material.Name] = $normal.AtomicInstructions
    $includeName = "primary-hit-probe-normal-$($material.Name).words.inc"
    $includePath = Join-Path $output $includeName
    New-ProbeWordsInclude -Words ([uint32[]]$normal.Words) -Path $includePath
    $normal | Add-Member -NotePropertyName Material -NotePropertyValue $material.Label
    $normal | Add-Member -NotePropertyName Include -NotePropertyValue $includeName
    $normal | Add-Member -NotePropertyName IncludeSha256 -NotePropertyValue (Get-ProbeSha256 ([IO.File]::ReadAllBytes($includePath)))
    $normalProbeModules += $normal
}

$probeHeader = [Text.StringBuilder]::new()
[void]$probeHeader.AppendLine('#pragma once')
[void]$probeHeader.AppendLine('#include <array>')
[void]$probeHeader.AppendLine('#include <cstdint>')
[void]$probeHeader.AppendLine('#include <string_view>')
[void]$probeHeader.AppendLine('namespace horde::vulkan::raytracing::experimental {')
[void]$probeHeader.AppendLine('inline constexpr std::uint32_t kProbeInstrument = 1u;')
[void]$probeHeader.AppendLine('inline constexpr std::uint32_t kProbeQuality = 0u;')
[void]$probeHeader.AppendLine('inline constexpr std::uint32_t kProbeBufferBytes = 57776u;')
[void]$probeHeader.AppendLine('inline constexpr std::uint32_t kProbePrefixBytes = 176u;')
[void]$probeHeader.AppendLine('inline constexpr std::uint32_t kProbeRecordWords = 96u;')
[void]$probeHeader.AppendLine('inline constexpr std::uint32_t kProbeRecordCount = 75u;')
[void]$probeHeader.AppendLine('inline constexpr std::uint32_t kProbePhases = 2u;')
foreach ($material in $materials) {
    $module = @($normalProbeModules | Where-Object { $_.Stem -eq "normal-$($material.Name)-probe" })[0]
    $label = if ($material.Name -eq 'opaque') { 'Opaque' } else { 'Generic' }
    $array = "kProbeNormal${label}Words"
    [void]$probeHeader.AppendLine(('inline constexpr std::array<std::uint32_t, {0}> {1}{{' -f $module.Words.Count, $array))
    [void]$probeHeader.AppendLine(('#include "{0}"' -f $module.Include))
    [void]$probeHeader.AppendLine('};')
    [void]$probeHeader.AppendLine(('inline constexpr std::string_view kProbeNormal{0}Sha256 = "{1}";' -f $label, $module.Sha256))
    [void]$probeHeader.AppendLine(('inline constexpr std::string_view kProbeNormal{0}IncludeSha256 = "{1}";' -f $label, $module.IncludeSha256))
    [void]$probeHeader.AppendLine(('inline constexpr std::uint32_t kProbeNormal{0}AtomicInstructions = {1}u;' -f $label, $module.AtomicInstructions))
    [void]$probeHeader.AppendLine(('static_assert({0}.size() * sizeof(std::uint32_t) == {1}u);' -f $array, $module.Bytes.Length))
}
[void]$probeHeader.AppendLine('static_assert(kProbeInstrument == 1u && kProbeQuality == 0u);')
[void]$probeHeader.AppendLine('static_assert(kProbeBufferBytes == kProbePrefixBytes + 75u * 2u * 96u * sizeof(std::uint32_t));')
[void]$probeHeader.AppendLine('}')
Write-ProbeText -Path (Join-Path $output 'PrimaryHitProbeShaders.generated.h') -Text $probeHeader.ToString()

$stageMaterials = @([pscustomobject]@{ Name = 'opaque'; Label = 'OpaqueFast'; Value = 0 }, [pscustomobject]@{ Name = 'generic'; Label = 'GenericDielectric'; Value = 1 })
$stagePasses = @([pscustomobject]@{ Name = 'primary'; Value = 1 }, [pscustomobject]@{ Name = 'shade'; Value = 2 })
$stagedModules = @()
foreach ($material in $stageMaterials) {
    $materialSource = $resolved
    if ($material.Value -eq 0) {
        $legacyDefine = '#define HORDE_GENERIC_TRANSMISSION_VARIANT 1'
        if ([regex]::Matches($materialSource, [regex]::Escape($legacyDefine)).Count -ne 1) { throw 'Staged OpaqueFast route expected exactly one generic-variant define.' }
        $materialSource = $materialSource.Replace($legacyDefine, '#define HORDE_GENERIC_TRANSMISSION_VARIANT 0')
    }
    foreach ($pass in $stagePasses) {
        $variant = Add-StagedPrimaryVariantConfiguration -Source $materialSource -InstrumentationName Diagnostic -QualityName Mobile -MaterialValue $material.Value -PassValue $pass.Value
        $variant = Add-ProbeBufferAndHelpers -Source $variant -ProbeSource $probeSource
        $source = New-StagedPrimaryPassSource -ResolvedSource $variant -RecordSource $recordSource -Pass $pass.Name
        $source = Add-StagedProbeCalls -Source $source -Pass $pass.Name
        $arrayName = if ($material.Name -eq 'opaque') { if ($pass.Name -eq 'primary') { 'kPrimaryOpaqueWords' } else { 'kShadeOpaqueWords' } } else { if ($pass.Name -eq 'primary') { 'kPrimaryGenericWords' } else { 'kShadeGenericWords' } }
        $module = Compile-ProbeModule -SourceText $source -Stem "$($material.Name)-$($pass.Name)" -MaterialValue $material.Value -Destination $stagedDirectory -ExpectStaged -ExpectProbeBuffer
        $module | Add-Member -NotePropertyName ArrayName -NotePropertyValue $arrayName
        $module | Add-Member -NotePropertyName Material -NotePropertyValue $material.Label
        $module | Add-Member -NotePropertyName Pass -NotePropertyValue $pass.Name
        $stagedModules += $module
    }
}

$opaquePrimary = @($stagedModules | Where-Object ArrayName -eq 'kPrimaryOpaqueWords')[0].Bytes
$opaqueShade = @($stagedModules | Where-Object ArrayName -eq 'kShadeOpaqueWords')[0].Bytes
$genericPrimary = @($stagedModules | Where-Object ArrayName -eq 'kPrimaryGenericWords')[0].Bytes
$genericShade = @($stagedModules | Where-Object ArrayName -eq 'kShadeGenericWords')[0].Bytes
$script:StagedPrimaryOpaquePairSha = Get-ProbeSha256 ([byte[]]($opaquePrimary + $opaqueShade))
$script:StagedPrimaryGenericPairSha = Get-ProbeSha256 ([byte[]]($genericPrimary + $genericShade))
$stageHeader = New-StagedPrimaryHeader -Modules $stagedModules -InstrumentationName Diagnostic -QualityName Mobile
$stageHeader = Add-PrimaryHitProbeConstants -Header $stageHeader
Write-ProbeText -Path (Join-Path $stagedDirectory 'StagedPrimaryShaders.generated.h') -Text $stageHeader

$depsRows = foreach ($dependency in $dependencies) {
    $rel = Get-StagedPrimaryRepoRelativePath $dependency
    [ordered]@{ path = $rel.Replace('\', '/'); sha256 = Get-ProbeSha256 ([IO.File]::ReadAllBytes($dependency)) }
}
$normalRows = foreach ($module in $normalProbeModules) {
    $baseKey = if ($module.Material -eq 'OpaqueFast') { 'opaque' } else { 'generic' }
    [ordered]@{ material = $module.Material; source = $module.Source; sourceSha256 = $module.SourceSha256; spirv = $module.Spv; spirvSha256 = $module.Sha256; bytes = $module.Bytes.Length; words = $module.Words.Count; include = $module.Include; includeSha256 = $module.IncludeSha256; atomicInstructions = $module.AtomicInstructions; baselineAtomicInstructions = $normalBaselineAtoms[$baseKey] }
}
$stagedRows = foreach ($module in $stagedModules) {
    [ordered]@{ material = $module.Material; pass = $module.Pass; source = $module.Source; sourceSha256 = $module.SourceSha256; spirv = $module.Spv; spirvSha256 = $module.Sha256; bytes = $module.Bytes.Length; words = $module.Words.Count; descriptorBindingsSet1 = $module.DescriptorBindingsSet1 }
}
$manifest = [ordered]@{
    schema = 1
    classification = 'investigation-only-primary-hit-probe'
    instrumentation = 'Diagnostic'
    quality = 'Mobile'
    stage = 'rgen'
    targetEnvironment = 'vulkan1.2'
    generator = [ordered]@{ path = 'tools/compile-primary-hit-probe.ps1'; sha256 = Get-ProbeSha256 ([IO.File]::ReadAllBytes($PSCommandPath)) }
    stagedPrimaryGenerator = [ordered]@{ path = 'tools/compile-staged-primary.ps1'; sha256 = Get-ProbeSha256 ([IO.File]::ReadAllBytes($stagedCompiler)) }
    toolchain = $toolchain
    source = [ordered]@{ path = 'shaders/raytracing/minimal.rgen'; dependencies = @($depsRows) }
    probeSource = [ordered]@{ path = 'shaders/raytracing/experimental/primary_hit_probe.glsl'; sha256 = $probeHash }
    stagedRecordIncludeSha256 = $recordHash
    stagedVariantConfigSha256 = Get-ProbeSha256 ([IO.File]::ReadAllBytes($configPath))
    passContract = [ordered]@{ primary = 'camera + primary trace + primary diagnostics + probe phase0 + storeStagedPrimary'; shade = 'camera + loadStagedPrimary + probe phase1 + original shading/presentation'; descriptorSet = 1; bindings = @(0, 1, 2); pages = 3; bytesPerRecord = 128 }
    pairs = @(
        [ordered]@{ key = 'staged_primary_v1_diagnostic_mobile_opaque_fast'; strategy = 'opaque-fast'; sha256 = $script:StagedPrimaryOpaquePairSha },
        [ordered]@{ key = 'staged_primary_v1_diagnostic_mobile_generic_dielectric'; strategy = 'generic-dielectric'; sha256 = $script:StagedPrimaryGenericPairSha }
    )
    modules = @($stagedRows)
    primaryHitProbe = [ordered]@{
        schema = 1; enabled = $true; bufferBytes = 57776; prefixBytes = 176; recordWords = 96; recordCount = 75; phases = 2; extent = @(1080, 2235)
        patches = @(@{ center = @(875,1857); width = 5; height = 5 }, @{ center = @(216,1955); width = 5; height = 5 }, @{ center = @(194,1085); width = 5; height = 5 })
        normalModules = @($normalRows)
        baselineAtomicInstructions = [ordered]@{ opaque = $normalBaselineAtoms.opaque; generic = $normalBaselineAtoms.generic }
    }
}
Write-ProbeText -Path (Join-Path $output 'staged-primary-manifest.json') -Text (($manifest | ConvertTo-Json -Depth 16) + "`n")

Write-Output "Primary-hit probe artifacts: $output"
Write-Output 'Policy: Diagnostic/Mobile; two normal probe raygens plus four staged probe raygens compiled, optimized, validated, and disassembled.'
Write-Output ("Probe record buffer: 57776 bytes; normal atomics unchanged: opaque={0}, generic={1}." -f $normalProbeAtoms.opaque, $normalProbeAtoms.generic)
