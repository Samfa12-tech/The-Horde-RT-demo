[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$TargetPath,
    [Parameter(Mandatory = $true)][ValidateSet('Windows','Android')][string]$TargetPlatform,
    [Parameter(Mandatory = $true)][ValidateSet('Shipping','Diagnostic')][string]$Instrumentation,
    [Parameter(Mandatory = $true)][ValidateSet('Mobile','High')][string]$Quality,
    [string]$CatalogPath,
    [switch]$SkipExternalValidation
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($CatalogPath)) {
    $CatalogPath = Join-Path $repoRoot 'tools\raygen-variant-catalog.json'
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
    $words = @([regex]::Matches((Get-Content -LiteralPath $path -Raw),
        '0x([0-9a-fA-F]{8})u') | ForEach-Object {
            [Convert]::ToUInt32($_.Groups[1].Value, 16)
        })
    Assert-True ($words.Count -gt 5) "SPIR-V include contains no complete module: $path"
    $bytes = New-Object byte[] ($words.Count * 4)
    for ($index = 0; $index -lt $words.Count; ++$index) {
        [Array]::Copy([BitConverter]::GetBytes([uint32]$words[$index]), 0,
                      $bytes, $index * 4, 4)
    }
    return $bytes
}
function Test-RaygenPrefix([byte[]]$bytes, [int]$offset) {
    if ($offset + 20 -gt $bytes.Length -or (Get-U32 $bytes $offset) -ne 0x07230203) {
        return $false
    }
    $remainingWords = [int](($bytes.Length - $offset) / 4)
    $limit = [Math]::Min($remainingWords, 4096)
    $cursor = 5
    while ($cursor -lt $limit) {
        $instruction = Get-U32 $bytes ($offset + $cursor * 4)
        $wordCount = [int]($instruction -shr 16)
        $opcode = [int]($instruction -band 0xffff)
        if ($wordCount -le 0 -or $cursor + $wordCount -gt $limit) { return $false }
        if ($opcode -eq 15 -and $wordCount -ge 3 -and
            (Get-U32 $bytes ($offset + ($cursor + 1) * 4)) -eq 5313) {
            return $true
        }
        if ($opcode -eq 54) { return $false }
        $cursor += $wordCount
    }
    return $false
}
function Get-ExactModuleShape([byte[]]$targetBytes, [int]$offset, [int]$wordCount) {
    if ($wordCount -le 5 -or $offset + $wordCount * 4 -gt $targetBytes.Length) {
        return $null
    }
    $cursor = 5
    $lastOpcode = -1
    $raygen = $false
    $binding22 = $false
    $atomics = 0
    while ($cursor -lt $wordCount) {
        $instruction = Get-U32 $targetBytes ($offset + $cursor * 4)
        $count = [int]($instruction -shr 16)
        $opcode = [int]($instruction -band 0xffff)
        if ($count -le 0 -or $cursor + $count -gt $wordCount) { return $null }
        if ($opcode -eq 15 -and $count -ge 3 -and
            (Get-U32 $targetBytes ($offset + ($cursor + 1) * 4)) -eq 5313) {
            $raygen = $true
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
        $lastOpcode = $opcode
        $cursor += $count
    }
    if (-not $raygen -or $cursor -ne $wordCount -or $lastOpcode -ne 56) {
        return $null
    }
    $moduleBytes = New-Object byte[] ($wordCount * 4)
    [Array]::Copy($targetBytes, $offset, $moduleBytes, 0, $moduleBytes.Length)
    return [pscustomobject]@{
        Bytes = $moduleBytes
        Sha256 = Get-Sha256Hex $moduleBytes
        Words = $wordCount
        HasBinding22 = $binding22
        AtomicInstructions = $atomics
    }
}
function Test-ContainsAscii([string]$ascii, [string]$value) {
    return $ascii.IndexOf($value, [StringComparison]::Ordinal) -ge 0
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
    Assert-True ((Get-U16 $bytes 18) -eq 0x00b7) `
        'Final Android target machine is not AArch64.'
}

$resolvedTarget = (Resolve-Path -LiteralPath $TargetPath -ErrorAction Stop).Path
Assert-True (Test-Path -LiteralPath $resolvedTarget -PathType Leaf) "Final target is not a file: $resolvedTarget"
$catalog = Get-Content -LiteralPath $CatalogPath -Raw | ConvertFrom-Json
$rows = @($catalog.variants)
Assert-True ($catalog.schema -eq 1 -and $catalog.status -ceq 'frozen' -and $rows.Count -eq 8) 'Frozen raygen catalog is malformed.'
$selected = @($rows | Where-Object {
    $_.instrumentation -ceq $Instrumentation -and $_.quality -ceq $Quality
})
Assert-True ($selected.Count -eq 2) 'Selected final-target policy does not resolve exactly two catalog rows.'
foreach ($material in @('OpaqueFast','GenericDielectric')) {
    Assert-True (@($selected | Where-Object material -CEQ $material).Count -eq 1) "Selected policy is missing material strategy: $material"
}

$rowModules = @{}
$knownModuleHashes = [Collections.Generic.List[string]]::new()
$knownWordCounts = [Collections.Generic.List[int]]::new()
foreach ($row in $rows) {
    $includePath = Join-Path $repoRoot ([string]$row.artifactPath)
    $raw = Get-IncludeBytes $includePath
    $rawHash = Get-Sha256Hex $raw
    Assert-True ($raw.Length -eq [int64]$row.bytes -and
                 $raw.Length / 4 -eq [int64]$row.words -and
                 $rawHash -ceq [string]$row.spirvSha256) "Frozen catalog module is stale: $($row.key)"
    $rowModules[[string]$row.key] = $raw
    $knownModuleHashes.Add($rawHash)
    $knownWordCounts.Add([int]$row.words)
}
$compatibilityModules = @()
foreach ($compatibilityInclude in @('src\vulkan\raytracing\MinimalRayGenShader.inc',
                                     'src\vulkan\raytracing\MinimalLegacyRayGenShader.inc')) {
    $compatibilityBytes = Get-IncludeBytes (Join-Path $repoRoot $compatibilityInclude)
    $compatibilityModules += [pscustomobject]@{
        Include = $compatibilityInclude
        Sha256 = Get-Sha256Hex $compatibilityBytes
        Words = [int]($compatibilityBytes.Length / 4)
    }
    $knownModuleHashes.Add($compatibilityModules[-1].Sha256)
    $knownWordCounts.Add($compatibilityModules[-1].Words)
}

$targetBytes = [IO.File]::ReadAllBytes($resolvedTarget)
Assert-FinalTargetIdentity $targetBytes $TargetPlatform
Assert-True ($targetBytes.Length -ge 20) 'Final target is too small to contain SPIR-V.'
$observed = @()
$wordCounts = @($knownWordCounts | Sort-Object -Unique)
for ($offset = 0; $offset -le $targetBytes.Length - 20; $offset += 4) {
    if ($targetBytes[$offset] -ne 0x03 -or
        $targetBytes[$offset + 1] -ne 0x02 -or
        $targetBytes[$offset + 2] -ne 0x23 -or
        $targetBytes[$offset + 3] -ne 0x07 -or
        -not (Test-RaygenPrefix $targetBytes $offset)) { continue }
    $matches = @()
    foreach ($words in $wordCounts) {
        $shape = Get-ExactModuleShape $targetBytes $offset $words
        if ($null -ne $shape -and
            $shape.Sha256 -cin $knownModuleHashes) {
            $matches += $shape
        }
    }
    $matches = @($matches | Sort-Object Sha256 -Unique)
    Assert-True ($matches.Count -eq 1) "Raygen module at aligned offset $offset is malformed, unknown, or ambiguous."
    $observed += [pscustomobject]@{ Offset = $offset; Shape = $matches[0] }
}
Assert-True ($observed.Count -eq 2) "Final target must contain exactly two reconstructed raygen modules; observed $($observed.Count)."

$selectedHashes = @($selected | ForEach-Object { [string]$_.spirvSha256 } | Sort-Object -Unique)
foreach ($entry in $observed) {
    Assert-True ($entry.Shape.Sha256 -cin $selectedHashes) "Final target contains a non-selected raygen module at offset $($entry.Offset)."
}
foreach ($row in $selected) {
    $count = @($observed | Where-Object { $_.Shape.Sha256 -ceq [string]$row.spirvSha256 }).Count
    Assert-True ($count -eq 1) "Selected raygen module is missing or duplicated: $($row.key)"
    $shape = @($observed | Where-Object { $_.Shape.Sha256 -ceq [string]$row.spirvSha256 })[0].Shape
    Assert-True ($shape.Words -eq [int]$row.words -and
                 $shape.AtomicInstructions -eq [int]$row.atomicInstructions -and
                 $shape.HasBinding22 -eq [bool]$row.hasDiagnosticsBinding) "Selected raygen reflection disagrees with the catalog: $($row.key)"
}

$ascii = [Text.Encoding]::ASCII.GetString($targetBytes)
$selectedKeys = @($selected | ForEach-Object { [string]$_.key })
foreach ($key in $selectedKeys) {
    Assert-True (Test-ContainsAscii $ascii $key) "Selected semantic key is absent from the final target: $key"
}
foreach ($row in $rows | Where-Object { [string]$_.key -cnotin $selectedKeys }) {
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
    if ($compatibilityModule.Sha256 -cnotin $selectedHashes) {
        Assert-True (@($observed | Where-Object {
            $_.Shape.Sha256 -ceq $compatibilityModule.Sha256
        }).Count -eq 0) "Compatibility raygen module leaked into final target: $($compatibilityModule.Include)"
    }
}

$validated = 'not-found'
$disassembled = 'not-found'
if (-not $SkipExternalValidation) {
    $validator = Resolve-SpirvTool 'spirv-val'
    $disassembler = Resolve-SpirvTool 'spirv-dis'
    $temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) ('horde-rt-final-modules-' + [guid]::NewGuid().ToString('N'))
    try {
        New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
        foreach ($entry in $observed) {
            $modulePath = Join-Path $temporaryRoot ("$($entry.Shape.Sha256).spv")
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
                $disassembled = 'passed'
            }
        }
    }
    finally {
        if (Test-Path -LiteralPath $temporaryRoot) {
            Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
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
            sha256 = $_.Shape.Sha256
            words = $_.Shape.Words
            binding22 = $_.Shape.HasBinding22
            atomicInstructions = $_.Shape.AtomicInstructions
        }
    })
    spirvVal = $validated
    spirvDis = $disassembled
}
$summary | ConvertTo-Json -Depth 5 -Compress
