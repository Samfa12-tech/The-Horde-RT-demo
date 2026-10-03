[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Scanner,
    [Parameter(Mandatory = $true)][string]$TargetPath,
    [Parameter(Mandatory = $true)][ValidateSet('Windows','Android')][string]$TargetPlatform,
    [Parameter(Mandatory = $true)][ValidateSet('Shipping','Diagnostic')][string]$Instrumentation,
    [Parameter(Mandatory = $true)][ValidateSet('Mobile','High')][string]$Quality,
    [Parameter(Mandatory = $true)][string]$PowerShellExecutable
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
function Assert-True([bool]$condition, [string]$message) {
    if (-not $condition) { throw $message }
}
function Get-IncludeBytes([string]$path) {
    $words = @([regex]::Matches((Get-Content -LiteralPath $path -Raw),
        '0x([0-9a-fA-F]{8})u') | ForEach-Object {
            [Convert]::ToUInt32($_.Groups[1].Value, 16)
        })
    $bytes = New-Object byte[] ($words.Count * 4)
    for ($index = 0; $index -lt $words.Count; ++$index) {
        [Array]::Copy([BitConverter]::GetBytes([uint32]$words[$index]), 0,
                      $bytes, $index * 4, 4)
    }
    return $bytes
}
function Get-U32([byte[]]$bytes, [int]$offset) {
    return [BitConverter]::ToUInt32($bytes, $offset)
}
function Set-SpirvExecutionModel(
    [byte[]]$bytes,
    [int]$offset,
    [int]$wordCount,
    [uint32]$executionModel)
{
    $cursor = 5
    while ($cursor -lt $wordCount) {
        $instruction = Get-U32 $bytes ($offset + $cursor * 4)
        $count = [int]($instruction -shr 16)
        $opcode = [int]($instruction -band 0xffff)
        Assert-True ($count -gt 0 -and $cursor + $count -le $wordCount) `
            'Wrong-stage control could not parse the selected SPIR-V module.'
        if ($opcode -eq 15 -and $count -ge 3) {
            [Array]::Copy(
                [BitConverter]::GetBytes($executionModel), 0,
                $bytes, $offset + ($cursor + 1) * 4, 4)
            return
        }
        $cursor += $count
    }
    throw 'Wrong-stage control could not find OpEntryPoint.'
}
function Add-AlignedBytes([byte[]]$prefix, [byte[]]$suffix) {
    $padding = (4 - ($prefix.Length % 4)) % 4
    $combined = New-Object byte[] ($prefix.Length + $padding + $suffix.Length)
    [Array]::Copy($prefix, $combined, $prefix.Length)
    [Array]::Copy($suffix, 0, $combined, $prefix.Length + $padding, $suffix.Length)
    return $combined
}
function Find-AlignedByteSequenceOffsets([byte[]]$haystack, [byte[]]$needle) {
    $offsets = [Collections.Generic.List[int]]::new()
    for ($offset = 0; $offset -le $haystack.Length - $needle.Length; $offset += 4) {
        if ($haystack[$offset] -ne $needle[0] -or
            $haystack[$offset + 1] -ne $needle[1] -or
            $haystack[$offset + 2] -ne $needle[2] -or
            $haystack[$offset + 3] -ne $needle[3]) { continue }
        $equal = $true
        for ($index = 4; $index -lt $needle.Length; ++$index) {
            if ($haystack[$offset + $index] -ne $needle[$index]) {
                $equal = $false
                break
            }
        }
        if ($equal) { $offsets.Add($offset) }
    }
    return $offsets.ToArray()
}
function Invoke-ScannerExpectFailure([string]$path, [string]$expectedText) {
    $output = (& $PowerShellExecutable -NoProfile -File $Scanner -TargetPath $path `
        -TargetPlatform $TargetPlatform -Instrumentation $Instrumentation `
        -Quality $Quality -SkipExternalValidation 2>&1 |
        Out-String)
    $exitCode = $LASTEXITCODE
    Assert-True ($exitCode -ne 0) "Containment scanner unexpectedly accepted fixture: $path"
    Assert-True ($output.Contains($expectedText)) "Containment scanner failed without proving control '$expectedText': $output"
}
function Invoke-ScannerExpectSuccess([string]$path) {
    $output = (& $PowerShellExecutable -NoProfile -File $Scanner -TargetPath $path `
        -TargetPlatform $TargetPlatform -Instrumentation $Instrumentation `
        -Quality $Quality -SkipExternalValidation 2>&1 |
        Out-String)
    $exitCode = $LASTEXITCODE
    Assert-True ($exitCode -eq 0) "Containment scanner rejected valid fixture: $output"
    return $output | ConvertFrom-Json
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
        'Containment control temporary path escaped its intended parent.'
    Assert-True ([IO.Path]::GetFileName($resolvedPath).StartsWith(
        $requiredLeafPrefix, [StringComparison]::Ordinal)) `
        'Containment control temporary path lost its guarded leaf prefix.'
    return $resolvedPath
}

$raygenCatalog = Get-Content -LiteralPath (
    Join-Path $repoRoot 'tools\raygen-variant-catalog.json') -Raw |
    ConvertFrom-Json
$computeCatalog = Get-Content -LiteralPath (
    Join-Path $repoRoot 'tools\rayquery-variant-catalog.json') -Raw |
    ConvertFrom-Json
$selectedRaygen = @($raygenCatalog.variants | Where-Object {
    $_.instrumentation -ceq $Instrumentation -and $_.quality -ceq $Quality
})
$selectedCompute = @($computeCatalog.variants | Where-Object {
    $_.instrumentation -ceq $Instrumentation -and $_.quality -ceq $Quality
})
Assert-True ($selectedRaygen.Count -eq 2 -and $selectedCompute.Count -eq 2) `
    'Control fixture could not resolve both selected backend pairs.'
$selected = @(
    foreach ($row in $selectedRaygen) {
        [pscustomobject]@{
            Row = $row
            Backend = 'RayTracingPipeline'
            ExecutionModel = 'RayGenerationKHR'
        }
    }
    foreach ($row in $selectedCompute) {
        [pscustomobject]@{
            Row = $row
            Backend = 'RayQueryCompute'
            ExecutionModel = 'GLCompute'
        }
    }
)
$oppositeInstrumentation = if ($Instrumentation -ceq 'Shipping') {
    'Diagnostic'
} else {
    'Shipping'
}
$selectedRaygenHashes = @($selectedRaygen | ForEach-Object spirvSha256)
$selectedComputeHashes = @($selectedCompute | ForEach-Object spirvSha256)
$forbiddenRaygen = @($raygenCatalog.variants | Where-Object {
    $_.instrumentation -ceq $oppositeInstrumentation -and
    $_.spirvSha256 -cnotin $selectedRaygenHashes
} | Select-Object -First 1)
$forbiddenCompute = @($computeCatalog.variants | Where-Object {
    $_.instrumentation -ceq $oppositeInstrumentation -and
    $_.spirvSha256 -cnotin $selectedComputeHashes
} | Select-Object -First 1)
Assert-True ($forbiddenRaygen.Count -eq 1 -and $forbiddenCompute.Count -eq 1) `
    'Control fixture requires distinguishable forbidden streams for both backends.'
$targetBytes = [IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $TargetPath))
$validSummary = Invoke-ScannerExpectSuccess $TargetPath
Assert-True (@($validSummary.modules).Count -eq 4 -and
             @($validSummary.semanticKeys).Count -eq 4) `
    'Valid dual-backend target must expose exactly four modules and semantic keys.'
Assert-True (@($validSummary.modules | Where-Object {
    $_.backend -ceq 'RayTracingPipeline' -and
    $_.executionModel -ceq 'RayGenerationKHR'
}).Count -eq 2) 'Valid target must expose two RayGenerationKHR policy modules.'
Assert-True (@($validSummary.modules | Where-Object {
    $_.backend -ceq 'RayQueryCompute' -and
    $_.executionModel -ceq 'GLCompute'
}).Count -eq 2) 'Valid target must expose two GLCompute hardware-query policy modules.'
foreach ($selectedKey in @($selected | ForEach-Object { [string]$_.Row.key })) {
    Assert-True ($selectedKey -cin @($validSummary.semanticKeys)) `
        "Valid target summary is missing semantic key: $selectedKey"
}

$temporaryParent = [IO.Path]::GetTempPath()
$temporaryRoot = Resolve-SafeTemporaryChild `
    (Join-Path $temporaryParent (
        'horde-rt-containment-controls-' + [guid]::NewGuid().ToString('N'))) `
    $temporaryParent 'horde-rt-containment-controls-'
try {
    New-Item -ItemType Directory -Path $temporaryRoot | Out-Null

    $wrongContainerPath = Join-Path $temporaryRoot 'wrong-container.bin'
    $wrongContainer = [byte[]]$targetBytes.Clone()
    $wrongContainer[0] = $wrongContainer[0] -bxor 0xff
    [IO.File]::WriteAllBytes($wrongContainerPath, $wrongContainer)
    Invoke-ScannerExpectFailure $wrongContainerPath $(if ($TargetPlatform -ceq 'Windows') {
        'Final Windows target is not a PE image.'
    } else {
        'Final Android target is not an ELF image.'
    })

    $wrongMachinePath = Join-Path $temporaryRoot 'wrong-machine.bin'
    $wrongMachine = [byte[]]$targetBytes.Clone()
    if ($TargetPlatform -ceq 'Windows') {
        $peOffset = [BitConverter]::ToUInt32($wrongMachine, 0x3c)
        $wrongMachine[[int]$peOffset + 4] = 0x4c
        $wrongMachine[[int]$peOffset + 5] = 0x01
    } else {
        $wrongMachine[18] = 0x3e
        $wrongMachine[19] = 0x00
    }
    [IO.File]::WriteAllBytes($wrongMachinePath, $wrongMachine)
    Invoke-ScannerExpectFailure $wrongMachinePath $(if ($TargetPlatform -ceq 'Windows') {
        'Final Windows target machine is not AMD64.'
    } else {
        'Final Android target machine is not AArch64.'
    })

    if ($TargetPlatform -ceq 'Android') {
        $wrongElfTypePath = Join-Path $temporaryRoot 'wrong-elf-type.bin'
        $wrongElfType = [byte[]]$targetBytes.Clone()
        $wrongElfType[16] = 0x02
        $wrongElfType[17] = 0x00
        [IO.File]::WriteAllBytes($wrongElfTypePath, $wrongElfType)
        Invoke-ScannerExpectFailure $wrongElfTypePath `
            'Final Android target is not an ELF shared object.'
    }

    $selectedModules = @()
    foreach ($descriptor in $selected) {
        $payload = Get-IncludeBytes (
            Join-Path $repoRoot ([string]$descriptor.Row.artifactPath))
        $matches = @(Find-AlignedByteSequenceOffsets $targetBytes $payload)
        Assert-True ($matches.Count -eq 1) 'Control fixture requires one exact selected module in the valid target.'
        $selectedModules += [pscustomobject]@{
            Row = $descriptor.Row
            Backend = $descriptor.Backend
            ExecutionModel = $descriptor.ExecutionModel
            Bytes = $payload
            Offset = [int]$matches[0]
        }
    }

    $zeroPath = Join-Path $temporaryRoot 'zero-modules.bin'
    $zeroModules = [byte[]]$targetBytes.Clone()
    foreach ($module in $selectedModules) {
        [Array]::Clear($zeroModules, $module.Offset, $module.Bytes.Length)
    }
    [IO.File]::WriteAllBytes($zeroPath, $zeroModules)
    Invoke-ScannerExpectFailure $zeroPath 'observed 0'

    $missingPath = Join-Path $temporaryRoot 'unreadable.bin'
    Invoke-ScannerExpectFailure $missingPath 'Cannot find path'

    $malformedPath = Join-Path $temporaryRoot 'malformed-raygen.bin'
    $malformed = [byte[]]$targetBytes.Clone()
    [Array]::Clear(
        $malformed,
        $selectedModules[0].Offset + $selectedModules[0].Bytes.Length - 4,
        4)
    [IO.File]::WriteAllBytes($malformedPath, $malformed)
    Invoke-ScannerExpectFailure $malformedPath 'malformed, unknown, or ambiguous'

    $duplicatePath = Join-Path $temporaryRoot 'duplicate-selected.bin'
    [IO.File]::WriteAllBytes(
        $duplicatePath,
        (Add-AlignedBytes $targetBytes $selectedModules[0].Bytes))
    Invoke-ScannerExpectFailure $duplicatePath 'observed 5'

    $raygenContaminationPath = Join-Path $temporaryRoot 'forbidden-raygen.bin'
    $forbiddenRaygenBytes = Get-IncludeBytes (
        Join-Path $repoRoot ([string]$forbiddenRaygen[0].artifactPath))
    [IO.File]::WriteAllBytes(
        $raygenContaminationPath,
        (Add-AlignedBytes $targetBytes $forbiddenRaygenBytes))
    Invoke-ScannerExpectFailure $raygenContaminationPath 'observed 5'

    $computeContaminationPath = Join-Path $temporaryRoot 'forbidden-compute.bin'
    $forbiddenComputeBytes = Get-IncludeBytes (
        Join-Path $repoRoot ([string]$forbiddenCompute[0].artifactPath))
    [IO.File]::WriteAllBytes(
        $computeContaminationPath,
        (Add-AlignedBytes $targetBytes $forbiddenComputeBytes))
    Invoke-ScannerExpectFailure $computeContaminationPath 'observed 5'

    $metadataPath = Join-Path $temporaryRoot 'forbidden-metadata.bin'
    $metadataBytes = [Text.Encoding]::ASCII.GetBytes(
        [string]$forbiddenCompute[0].key)
    [IO.File]::WriteAllBytes($metadataPath,
        (Add-AlignedBytes $targetBytes $metadataBytes))
    Invoke-ScannerExpectFailure $metadataPath 'Non-selected semantic key leaked'

    $compatibilityPath = Join-Path $temporaryRoot 'compatibility-stream.bin'
    $compatibilityBytes = Get-IncludeBytes (
        Join-Path $repoRoot 'src\vulkan\raytracing\MinimalLegacyRayGenShader.inc')
    [IO.File]::WriteAllBytes($compatibilityPath,
        (Add-AlignedBytes $targetBytes $compatibilityBytes))
    Invoke-ScannerExpectFailure $compatibilityPath 'observed 5'

    $selectedComputeModule = @($selectedModules | Where-Object {
        $_.Backend -ceq 'RayQueryCompute'
    })[0]
    $wrongStagePath = Join-Path $temporaryRoot 'wrong-compute-stage.bin'
    $wrongStage = [byte[]]$targetBytes.Clone()
    Set-SpirvExecutionModel $wrongStage $selectedComputeModule.Offset `
        ([int]$selectedComputeModule.Row.words) 5317u
    [IO.File]::WriteAllBytes($wrongStagePath, $wrongStage)
    Invoke-ScannerExpectFailure $wrongStagePath 'observed 3'

    $unknownComputePath = Join-Path $temporaryRoot 'unknown-compute.bin'
    $unknownCompute = [byte[]]$targetBytes.Clone()
    $unknownCompute[$selectedComputeModule.Offset + 8] =
        $unknownCompute[$selectedComputeModule.Offset + 8] -bxor 1
    [IO.File]::WriteAllBytes($unknownComputePath, $unknownCompute)
    Invoke-ScannerExpectFailure $unknownComputePath `
        'malformed, unknown, or ambiguous'
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        $safeTemporaryRoot = Resolve-SafeTemporaryChild `
            $temporaryRoot $temporaryParent 'horde-rt-containment-controls-'
        Remove-Item -LiteralPath $safeTemporaryRoot -Recurse -Force
    }
}

Write-Output 'Final-target containment scanner adversarial controls passed.'
