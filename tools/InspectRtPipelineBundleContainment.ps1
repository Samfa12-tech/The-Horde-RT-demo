[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$TargetPath,
    [Parameter(Mandatory = $true)][ValidateSet('Windows','Android')][string]$TargetPlatform,
    [Parameter(Mandatory = $true)][ValidateSet('Shipping','Diagnostic')][string]$Instrumentation,
    [Parameter(Mandatory = $true)][ValidateSet('Mobile','High')][string]$Quality,
    [string]$CatalogPath,
    [string]$RayQueryCatalogPath,
    [switch]$SkipExternalValidation
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($CatalogPath)) {
    $CatalogPath = Join-Path $repoRoot 'tools\raygen-variant-catalog.json'
}
if ([string]::IsNullOrWhiteSpace($RayQueryCatalogPath)) {
    $RayQueryCatalogPath = Join-Path $repoRoot 'tools\rayquery-variant-catalog.json'
}
function Assert-True([bool]$condition, [string]$message) {
    if (-not $condition) { throw $message }
}
function Get-U32([byte[]]$bytes, [int]$offset) {
    return [BitConverter]::ToUInt32($bytes, $offset)
}
function Get-U16([byte[]]$bytes, [int]$offset) {
    return [BitConverter]::ToUInt16($bytes, $offset)
}
function Get-Sha256Hex([byte[]]$bytes) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return ([Convert]::ToHexString($sha.ComputeHash($bytes))).ToLowerInvariant() }
    finally { $sha.Dispose() }
}
function Get-IncludeBytes([string]$path) {
    $bytes = [HordeRtContainment.AlignedSpirvScanner]::ParseInclude(
        (Get-Content -LiteralPath $path -Raw))
    Assert-True ($bytes.Length -gt 20) "SPIR-V include contains no complete module: $path"
    return $bytes
}
function Get-ModuleBytesAtOffset(
    [byte[]]$targetBytes,
    [int]$offset,
    [int]$wordCount)
{
    if ($wordCount -le 5 -or $offset + $wordCount * 4 -gt $targetBytes.Length) {
        return $null
    }
    $moduleBytes = New-Object byte[] ($wordCount * 4)
    [Array]::Copy($targetBytes, $offset, $moduleBytes, 0, $moduleBytes.Length)
    return $moduleBytes
}
function Get-RelevantExecutionModel([byte[]]$bytes, [int]$offset) {
    if ($offset + 20 -gt $bytes.Length -or (Get-U32 $bytes $offset) -ne 0x07230203) {
        return $null
    }
    $remainingWords = [int](($bytes.Length - $offset) / 4)
    $limit = [Math]::Min($remainingWords, 4096)
    $cursor = 5
    while ($cursor -lt $limit) {
        $instruction = Get-U32 $bytes ($offset + $cursor * 4)
        $wordCount = [int]($instruction -shr 16)
        $opcode = [int]($instruction -band 0xffff)
        if ($wordCount -le 0 -or $cursor + $wordCount -gt $limit) { return $null }
        if ($opcode -eq 15 -and $wordCount -ge 3) {
            $candidate = [int](Get-U32 $bytes ($offset + ($cursor + 1) * 4))
            if ($candidate -eq 5 -or $candidate -eq 5313) {
                return $candidate
            }
        }
        if ($opcode -eq 54) { break }
        $cursor += $wordCount
    }
    return $null
}
function Get-ExecutionModelName([int]$executionModel) {
    if ($executionModel -eq 5313) { return 'RayGenerationKHR' }
    if ($executionModel -eq 5) { return 'GLCompute' }
    return $null
}
function Get-ExecutionBackendName([int]$executionModel) {
    if ($executionModel -eq 5313) { return 'RayTracingPipeline' }
    if ($executionModel -eq 5) { return 'RayQueryCompute' }
    return $null
}
function Get-ExactModuleShape(
    [byte[]]$targetBytes,
    [int]$offset,
    [int]$wordCount,
    [int]$expectedExecutionModel)
{
    if ($wordCount -le 5 -or $offset + $wordCount * 4 -gt $targetBytes.Length) {
        return $null
    }
    $cursor = 5
    $lastOpcode = -1
    $executionModel = $null
    $rayQueryCapability = $false
    $rayQueryInitializations = 0
    $binding22 = $false
    $atomics = 0
    while ($cursor -lt $wordCount) {
        $instruction = Get-U32 $targetBytes ($offset + $cursor * 4)
        $count = [int]($instruction -shr 16)
        $opcode = [int]($instruction -band 0xffff)
        if ($count -le 0 -or $cursor + $count -gt $wordCount) { return $null }
        if ($opcode -eq 15 -and $count -ge 3) {
            $candidate = [int](Get-U32 $targetBytes ($offset + ($cursor + 1) * 4))
            if ($candidate -eq 5 -or $candidate -eq 5313) {
                if ($null -ne $executionModel -and $executionModel -ne $candidate) {
                    return $null
                }
                $executionModel = $candidate
            }
        }
        if ($opcode -eq 17 -and $count -eq 2 -and
            (Get-U32 $targetBytes ($offset + ($cursor + 1) * 4)) -eq 4472) {
            $rayQueryCapability = $true
        }
        if ($opcode -eq 71 -and $count -eq 4 -and
            (Get-U32 $targetBytes ($offset + ($cursor + 2) * 4)) -eq 33 -and
            (Get-U32 $targetBytes ($offset + ($cursor + 3) * 4)) -eq 22) {
            $binding22 = $true
        }
        if (($opcode -ge 227 -and $opcode -le 242) -or
            $opcode -eq 318 -or $opcode -eq 319 -or
            $opcode -eq 5614 -or $opcode -eq 5615 -or $opcode -eq 6035) {
            ++$atomics
        }
        if ($opcode -eq 4473) {
            ++$rayQueryInitializations
        }
        $lastOpcode = $opcode
        $cursor += $count
    }
    if ($executionModel -ne $expectedExecutionModel -or
        $cursor -ne $wordCount -or $lastOpcode -ne 56) {
        return $null
    }
    $moduleBytes = Get-ModuleBytesAtOffset $targetBytes $offset $wordCount
    return [pscustomobject]@{
        Bytes = $moduleBytes
        Sha256 = Get-Sha256Hex $moduleBytes
        Words = $wordCount
        Backend = Get-ExecutionBackendName $executionModel
        ExecutionModel = Get-ExecutionModelName $executionModel
        ExecutionModelValue = $executionModel
        HasRayQueryCapability = $rayQueryCapability
        RayQueryInitializations = $rayQueryInitializations
        HasBinding22 = $binding22
        AtomicInstructions = $atomics
    }
}
function Test-ContainsAscii([string]$ascii, [string]$value) {
    return $ascii.IndexOf($value, [StringComparison]::Ordinal) -ge 0
}
function Resolve-SafeTemporaryChild(
    [string]$path,
    [string]$parent,
    [string]$requiredLeafPrefix)
{
    $resolvedParent = [IO.Path]::GetFullPath($parent)
    $separator = [IO.Path]::DirectorySeparatorChar.ToString()
    if (-not $resolvedParent.EndsWith($separator, [StringComparison]::Ordinal)) {
        $resolvedParent += $separator
    }
    $resolvedPath = [IO.Path]::GetFullPath($path)
    Assert-True ($resolvedPath.StartsWith(
        $resolvedParent, [StringComparison]::OrdinalIgnoreCase)) `
        'Containment scanner temporary path escaped its intended parent.'
    Assert-True ([IO.Path]::GetFileName($resolvedPath).StartsWith(
        $requiredLeafPrefix, [StringComparison]::Ordinal)) `
        'Containment scanner temporary path lost its guarded leaf prefix.'
    return $resolvedPath
}
function Resolve-SpirvTool([string]$name) {
    if (-not [string]::IsNullOrWhiteSpace($env:VULKAN_SDK)) {
        $candidate = Join-Path $env:VULKAN_SDK "Bin\$name.exe"
        if (Test-Path -LiteralPath $candidate -PathType Leaf) { return $candidate }
    }
    $installedSdkRoot = 'C:\VulkanSDK'
    if (Test-Path -LiteralPath $installedSdkRoot -PathType Container) {
        $candidate = Get-ChildItem -LiteralPath $installedSdkRoot -Directory |
            ForEach-Object {
                $version = $null
                if ([Version]::TryParse($_.Name, [ref]$version)) {
                    $tool = Join-Path $_.FullName "Bin\$name.exe"
                    if (Test-Path -LiteralPath $tool -PathType Leaf) {
                        [pscustomobject]@{ Version = $version; Path = $tool }
                    }
                }
            } | Sort-Object Version -Descending | Select-Object -First 1
        if ($null -ne $candidate) { return $candidate.Path }
    }
    $command = Get-Command "$name.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
    return if ($null -eq $command) { $null } else { $command.Source }
}
function Assert-FinalTargetIdentity([byte[]]$bytes, [string]$platform) {
    if ($platform -ceq 'Windows') {
        Assert-True ($bytes.Length -ge 64 -and $bytes[0] -eq 0x4d -and
                     $bytes[1] -eq 0x5a) 'Final Windows target is not a PE image.'
        $peOffset = [int64](Get-U32 $bytes 0x3c)
        Assert-True ($peOffset -ge 0 -and $peOffset + 26 -le $bytes.Length -and
                     $bytes[[int]$peOffset] -eq 0x50 -and
                     $bytes[[int]$peOffset + 1] -eq 0x45 -and
                     $bytes[[int]$peOffset + 2] -eq 0x00 -and
                     $bytes[[int]$peOffset + 3] -eq 0x00) `
            'Final Windows target is not a PE image.'
        Assert-True ((Get-U16 $bytes ([int]$peOffset + 4)) -eq 0x8664) `
            'Final Windows target machine is not AMD64.'
        Assert-True ((Get-U16 $bytes ([int]$peOffset + 24)) -eq 0x020b) `
            'Final Windows target is not PE32+.'
        return
    }
    Assert-True ($bytes.Length -ge 64 -and $bytes[0] -eq 0x7f -and
                 $bytes[1] -eq 0x45 -and $bytes[2] -eq 0x4c -and
                 $bytes[3] -eq 0x46) 'Final Android target is not an ELF image.'
    Assert-True ($bytes[4] -eq 2 -and $bytes[5] -eq 1) `
        'Final Android target is not ELF64 little-endian.'
    Assert-True ((Get-U16 $bytes 16) -eq 0x0003) `
        'Final Android target is not an ELF shared object.'
    Assert-True ((Get-U16 $bytes 18) -eq 0x00b7) `
        'Final Android target machine is not AArch64.'
}

if ($null -eq ('HordeRtContainment.AlignedSpirvScanner' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text.RegularExpressions;

namespace HordeRtContainment
{
    public static class AlignedSpirvScanner
    {
        private static readonly Regex IncludeWord = new Regex(
            @"0x([0-9a-fA-F]{8})u", RegexOptions.Compiled | RegexOptions.CultureInvariant);

        public static byte[] ParseInclude(string text)
        {
            if (text == null) throw new ArgumentNullException(nameof(text));
            var matches = IncludeWord.Matches(text);
            var bytes = new byte[matches.Count * 4];
            for (var index = 0; index < matches.Count; ++index)
            {
                var value = uint.Parse(
                    matches[index].Groups[1].Value,
                    NumberStyles.HexNumber,
                    CultureInfo.InvariantCulture);
                var offset = index * 4;
                bytes[offset] = (byte)value;
                bytes[offset + 1] = (byte)(value >> 8);
                bytes[offset + 2] = (byte)(value >> 16);
                bytes[offset + 3] = (byte)(value >> 24);
            }
            return bytes;
        }

        public static int[] Find(byte[] bytes)
        {
            if (bytes == null) throw new ArgumentNullException(nameof(bytes));
            var offsets = new List<int>();
            for (var offset = 0; offset <= bytes.Length - 20; offset += 4)
            {
                if (bytes[offset] == 0x03 && bytes[offset + 1] == 0x02 &&
                    bytes[offset + 2] == 0x23 && bytes[offset + 3] == 0x07)
                {
                    offsets.Add(offset);
                }
            }
            return offsets.ToArray();
        }
    }
}
'@
}

$resolvedTarget = (Resolve-Path -LiteralPath $TargetPath -ErrorAction Stop).Path
Assert-True (Test-Path -LiteralPath $resolvedTarget -PathType Leaf) "Final target is not a file: $resolvedTarget"
$raygenCatalog = Get-Content -LiteralPath $CatalogPath -Raw | ConvertFrom-Json
$raygenRows = @($raygenCatalog.variants)
Assert-True ($raygenCatalog.schema -eq 1 -and
             $raygenCatalog.status -ceq 'frozen' -and
             $raygenRows.Count -eq 8) 'Frozen raygen catalog is malformed.'
$computeCatalog = Get-Content -LiteralPath $RayQueryCatalogPath -Raw | ConvertFrom-Json
$computeRows = @($computeCatalog.variants)
Assert-True ($computeCatalog.schema -eq 1 -and
             $computeCatalog.status -ceq 'frozen' -and
             $computeRows.Count -eq 8 -and
             $computeCatalog.target.executionBackend -ceq 'RayQueryCompute' -and
             $computeCatalog.target.stage -ceq 'comp' -and
             $computeCatalog.target.executionModel -ceq 'GLCompute' -and
             $computeCatalog.target.hardwareTraversal -ceq 'RayQueryKHR') `
    'Frozen RayQuery compute catalog is malformed.'
foreach ($row in $computeRows) {
    $raygenPolicy = @($raygenRows | Where-Object {
        [string]$_.key -ceq [string]$row.policyKey
    })
    Assert-True ($row.executionBackend -ceq 'RayQueryCompute' -and
                 $row.stage -ceq 'comp' -and
                 $row.executionModel -ceq 'GLCompute' -and
                 $raygenPolicy.Count -eq 1 -and
                 $raygenPolicy[0].instrumentation -ceq $row.instrumentation -and
                 $raygenPolicy[0].quality -ceq $row.quality -and
                 $raygenPolicy[0].material -ceq $row.material -and
                 $raygenPolicy[0].strategy -ceq $row.strategy) `
        "RayQuery compute row lost policy parity: $($row.key)"
}

$selectedRaygen = @($raygenRows | Where-Object {
    $_.instrumentation -ceq $Instrumentation -and $_.quality -ceq $Quality
})
$selectedCompute = @($computeRows | Where-Object {
    $_.instrumentation -ceq $Instrumentation -and $_.quality -ceq $Quality
})
Assert-True ($selectedRaygen.Count -eq 2 -and $selectedCompute.Count -eq 2) `
    'Selected final-target policy does not resolve two rows for each backend.'
foreach ($material in @('OpaqueFast','GenericDielectric')) {
    Assert-True (@($selectedRaygen | Where-Object material -CEQ $material).Count -eq 1) `
        "Selected raygen policy is missing material strategy: $material"
    Assert-True (@($selectedCompute | Where-Object material -CEQ $material).Count -eq 1) `
        "Selected compute policy is missing material strategy: $material"
}

$catalogModules = @(
    foreach ($row in $raygenRows) {
        [pscustomobject]@{
            Row = $row
            Backend = 'RayTracingPipeline'
            ExecutionModel = 'RayGenerationKHR'
            ExecutionModelValue = 5313
        }
    }
    foreach ($row in $computeRows) {
        [pscustomobject]@{
            Row = $row
            Backend = 'RayQueryCompute'
            ExecutionModel = 'GLCompute'
            ExecutionModelValue = 5
        }
    }
)
foreach ($module in $catalogModules) {
    $row = $module.Row
    $includePath = Join-Path $repoRoot ([string]$row.artifactPath)
    $raw = Get-IncludeBytes $includePath
    $rawHash = Get-Sha256Hex $raw
    Assert-True ($raw.Length -eq [int64]$row.bytes -and
                 $raw.Length / 4 -eq [int64]$row.words -and
                 $rawHash -ceq [string]$row.spirvSha256) "Frozen catalog module is stale: $($row.key)"
}
$compatibilityModules = @()
foreach ($compatibilityInclude in @('src\vulkan\raytracing\MinimalRayGenShader.inc',
                                     'src\vulkan\raytracing\MinimalLegacyRayGenShader.inc')) {
    $compatibilityBytes = Get-IncludeBytes (Join-Path $repoRoot $compatibilityInclude)
    $compatibilityModules += [pscustomobject]@{
        Include = $compatibilityInclude
        Sha256 = Get-Sha256Hex $compatibilityBytes
        Words = [int]($compatibilityBytes.Length / 4)
        ExecutionModelValue = 5313
    }
}
$knownModules = @($catalogModules | ForEach-Object {
    [pscustomobject]@{
        Key = [string]$_.Row.key
        Sha256 = [string]$_.Row.spirvSha256
        Words = [int]$_.Row.words
        ExecutionModelValue = [int]$_.ExecutionModelValue
    }
}) + @($compatibilityModules | ForEach-Object {
    [pscustomobject]@{
        Key = [string]$_.Include
        Sha256 = [string]$_.Sha256
        Words = [int]$_.Words
        ExecutionModelValue = [int]$_.ExecutionModelValue
    }
})

$targetBytes = [IO.File]::ReadAllBytes($resolvedTarget)
Assert-FinalTargetIdentity $targetBytes $TargetPlatform
Assert-True ($targetBytes.Length -ge 20) 'Final target is too small to contain SPIR-V.'
$observed = @()
foreach ($offset in [HordeRtContainment.AlignedSpirvScanner]::Find($targetBytes)) {
    $executionModel = Get-RelevantExecutionModel $targetBytes $offset
    if ($null -eq $executionModel) { continue }
    $knownForModel = @($knownModules | Where-Object {
        $_.ExecutionModelValue -eq $executionModel
    })
    $wordCounts = @($knownForModel | ForEach-Object { $_.Words } |
        Sort-Object -Unique)
    $matches = @()
    foreach ($words in $wordCounts) {
        $moduleBytes = Get-ModuleBytesAtOffset $targetBytes $offset $words
        if ($null -eq $moduleBytes) { continue }
        $moduleHash = Get-Sha256Hex $moduleBytes
        if (@($knownForModel | Where-Object {
            $_.Words -eq $words -and $_.Sha256 -ceq $moduleHash
        }).Count -ge 1) {
            $matches += [pscustomobject]@{
                Words = $words
                Sha256 = $moduleHash
            }
        }
    }
    $matches = @($matches | Sort-Object Sha256, Words -Unique)
    Assert-True ($matches.Count -eq 1) `
        "Relevant SPIR-V module at aligned offset $offset is malformed, unknown, or ambiguous."
    $shape = Get-ExactModuleShape `
        $targetBytes $offset $matches[0].Words $executionModel
    Assert-True ($null -ne $shape -and
                 $shape.Sha256 -ceq $matches[0].Sha256) `
        "Relevant SPIR-V module at aligned offset $offset has an invalid exact shape."
    $observed += [pscustomobject]@{ Offset = $offset; Shape = $shape }
}
Assert-True ($observed.Count -eq 4) `
    "Final target must contain exactly four reconstructed raygen/compute modules; observed $($observed.Count)."

$selectedModules = @(
    $catalogModules | Where-Object {
        $_.Row.instrumentation -ceq $Instrumentation -and
        $_.Row.quality -ceq $Quality
    }
)
foreach ($entry in $observed) {
    $selectedMatch = @($selectedModules | Where-Object {
        [string]$_.Row.spirvSha256 -ceq $entry.Shape.Sha256 -and
        [int]$_.ExecutionModelValue -eq $entry.Shape.ExecutionModelValue
    })
    Assert-True ($selectedMatch.Count -ge 1) `
        "Final target contains a non-selected raygen/compute module at offset $($entry.Offset)."
}
foreach ($module in $selectedModules) {
    $row = $module.Row
    $matches = @($observed | Where-Object {
        $_.Shape.Sha256 -ceq [string]$row.spirvSha256 -and
        $_.Shape.ExecutionModelValue -eq [int]$module.ExecutionModelValue
    })
    Assert-True ($matches.Count -eq 1) `
        "Selected $($module.Backend) module is missing or duplicated: $($row.key)"
    $shape = $matches[0].Shape
    Assert-True ($shape.Words -eq [int]$row.words -and
                 $shape.Backend -ceq $module.Backend -and
                 $shape.ExecutionModel -ceq $module.ExecutionModel -and
                 $shape.HasRayQueryCapability -and
                 $shape.RayQueryInitializations -eq [int]$row.rayQueryInitializations -and
                 $shape.AtomicInstructions -eq [int]$row.atomicInstructions -and
                 $shape.HasBinding22 -eq [bool]$row.hasDiagnosticsBinding) `
        "Selected $($module.Backend) reflection disagrees with the catalog: $($row.key)"
}

$ascii = [Text.Encoding]::ASCII.GetString($targetBytes)
$selectedKeys = @($selectedModules | ForEach-Object { [string]$_.Row.key })
foreach ($key in $selectedKeys) {
    Assert-True (Test-ContainsAscii $ascii $key) "Selected semantic key is absent from the final target: $key"
}
foreach ($row in @($raygenRows) + @($computeRows) | Where-Object {
    [string]$_.key -cnotin $selectedKeys
}) {
    Assert-True (-not (Test-ContainsAscii $ascii ([string]$row.key))) "Non-selected semantic key leaked into the final target: $($row.key)"
}
if ($Instrumentation -ceq 'Shipping') {
    foreach ($entry in $observed) {
        Assert-True (-not $entry.Shape.HasBinding22 -and $entry.Shape.AtomicInstructions -eq 0) 'Shipping final target retains diagnostic binding 22 or atomics.'
    }
} else {
    foreach ($entry in $observed) {
        Assert-True ($entry.Shape.HasBinding22 -and $entry.Shape.AtomicInstructions -gt 0) 'Diagnostic final target lost binding 22 or diagnostic atomics.'
    }
}

foreach ($compatibilityModule in $compatibilityModules) {
    $selectedAlias = @($selectedModules | Where-Object {
        $_.ExecutionModelValue -eq $compatibilityModule.ExecutionModelValue -and
        [string]$_.Row.spirvSha256 -ceq $compatibilityModule.Sha256
    })
    if ($selectedAlias.Count -eq 0) {
        Assert-True (@($observed | Where-Object {
            $_.Shape.ExecutionModelValue -eq $compatibilityModule.ExecutionModelValue -and
            $_.Shape.Sha256 -ceq $compatibilityModule.Sha256
        }).Count -eq 0) "Compatibility raygen module leaked into final target: $($compatibilityModule.Include)"
    }
}

$validated = 'not-found'
$disassembled = 'not-found'
if (-not $SkipExternalValidation) {
    $validator = Resolve-SpirvTool 'spirv-val'
    $disassembler = Resolve-SpirvTool 'spirv-dis'
    $temporaryParent = [IO.Path]::GetTempPath()
    $temporaryRoot = Resolve-SafeTemporaryChild `
        (Join-Path $temporaryParent (
            'horde-rt-final-modules-' + [guid]::NewGuid().ToString('N'))) `
        $temporaryParent 'horde-rt-final-modules-'
    try {
        New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
        foreach ($entry in $observed) {
            $modulePath = Join-Path $temporaryRoot (
                "$($entry.Shape.ExecutionModel)-$($entry.Shape.Sha256).spv")
            [IO.File]::WriteAllBytes($modulePath, $entry.Shape.Bytes)
            if ($null -ne $validator) {
                & $validator --target-env vulkan1.2 $modulePath
                Assert-True ($LASTEXITCODE -eq 0) "spirv-val rejected extracted module: $($entry.Shape.Sha256)"
                $validated = 'passed'
            }
            if ($null -ne $disassembler) {
                $assemblyPath = "$modulePath.spvasm"
                & $disassembler $modulePath -o $assemblyPath
                Assert-True ($LASTEXITCODE -eq 0 -and (Test-Path -LiteralPath $assemblyPath)) "spirv-dis rejected extracted module: $($entry.Shape.Sha256)"
                $assembly = Get-Content -LiteralPath $assemblyPath -Raw
                Assert-True ($assembly.Contains(
                    "OpEntryPoint $($entry.Shape.ExecutionModel)")) `
                    "Extracted module has the wrong execution stage: $($entry.Shape.Sha256)"
                if ($entry.Shape.Backend -ceq 'RayQueryCompute') {
                    Assert-True ($assembly.Contains('OpCapability RayQueryKHR') -and
                                 $assembly.Contains('OpRayQueryInitializeKHR')) `
                        "Extracted compute module lost hardware ray-query traversal: $($entry.Shape.Sha256)"
                }
                $disassembled = 'passed'
            }
        }
    }
    finally {
        if (Test-Path -LiteralPath $temporaryRoot) {
            $safeTemporaryRoot = Resolve-SafeTemporaryChild `
                $temporaryRoot $temporaryParent 'horde-rt-final-modules-'
            Remove-Item -LiteralPath $safeTemporaryRoot -Recurse -Force
        }
    }
}

$summary = [pscustomobject]@{
    target = $resolvedTarget
    targetPlatform = $TargetPlatform
    instrumentation = $Instrumentation
    quality = $Quality
    targetSha256 = (Get-FileHash -LiteralPath $resolvedTarget -Algorithm SHA256).Hash.ToLowerInvariant()
    targetBytes = $targetBytes.Length
    semanticKeys = $selectedKeys
    modules = @($observed | ForEach-Object {
        [pscustomobject]@{
            offset = $_.Offset
            backend = $_.Shape.Backend
            executionModel = $_.Shape.ExecutionModel
            sha256 = $_.Shape.Sha256
            words = $_.Shape.Words
            rayQueryCapability = $_.Shape.HasRayQueryCapability
            rayQueryInitializations = $_.Shape.RayQueryInitializations
            binding22 = $_.Shape.HasBinding22
            atomicInstructions = $_.Shape.AtomicInstructions
        }
    })
    spirvVal = $validated
    spirvDis = $disassembled
}
$summary | ConvertTo-Json -Depth 5 -Compress
