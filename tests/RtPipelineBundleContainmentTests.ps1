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

$catalog = Get-Content -LiteralPath (Join-Path $repoRoot 'tools\raygen-variant-catalog.json') -Raw |
    ConvertFrom-Json
$selected = @($catalog.variants | Where-Object {
    $_.instrumentation -ceq $Instrumentation -and $_.quality -ceq $Quality
})
Assert-True ($selected.Count -eq 2) 'Control fixture could not resolve the selected pair.'
$selectedHashes = @($selected | ForEach-Object spirvSha256)
$forbidden = @($catalog.variants | Where-Object {
    $_.spirvSha256 -cnotin $selectedHashes
} | Select-Object -First 1)
Assert-True ($forbidden.Count -eq 1) 'Control fixture requires a distinguishable forbidden stream.'
$targetBytes = [IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $TargetPath))
$temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) ('horde-rt-containment-controls-' + [guid]::NewGuid().ToString('N'))
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

    $selectedPayloads = [Collections.Generic.List[byte[]]]::new()
    foreach ($row in $selected) {
        $selectedPayloads.Add(
            (Get-IncludeBytes (Join-Path $repoRoot ([string]$row.artifactPath))))
    }
    $selectedOffsets = @()
    foreach ($payload in $selectedPayloads) {
        $matches = @(Find-AlignedByteSequenceOffsets $targetBytes $payload)
        Assert-True ($matches.Count -eq 1) 'Control fixture requires one exact selected module in the valid target.'
        $selectedOffsets += $matches[0]
    }

    $zeroPath = Join-Path $temporaryRoot 'zero-modules.bin'
    $zeroModules = [byte[]]$targetBytes.Clone()
    for ($index = 0; $index -lt $selectedPayloads.Count; ++$index) {
        [Array]::Clear($zeroModules, [int]$selectedOffsets[$index], $selectedPayloads[$index].Length)
    }
    [IO.File]::WriteAllBytes($zeroPath, $zeroModules)
    Invoke-ScannerExpectFailure $zeroPath 'observed 0'

    $missingPath = Join-Path $temporaryRoot 'unreadable.bin'
    Invoke-ScannerExpectFailure $missingPath 'Cannot find path'

    $malformedPath = Join-Path $temporaryRoot 'malformed-raygen.bin'
    $malformed = [byte[]]$targetBytes.Clone()
    [Array]::Clear(
        $malformed,
        [int]$selectedOffsets[0] + $selectedPayloads[0].Length - 4,
        4)
    [IO.File]::WriteAllBytes($malformedPath, $malformed)
    Invoke-ScannerExpectFailure $malformedPath 'malformed, unknown, or ambiguous'

    $duplicatePath = Join-Path $temporaryRoot 'duplicate-selected.bin'
    $selectedBytes = Get-IncludeBytes (Join-Path $repoRoot ([string]$selected[0].artifactPath))
    [IO.File]::WriteAllBytes($duplicatePath, (Add-AlignedBytes $targetBytes $selectedBytes))
    Invoke-ScannerExpectFailure $duplicatePath 'observed 3'

    $contaminatedPath = Join-Path $temporaryRoot 'forbidden-stream.bin'
    $forbiddenBytes = Get-IncludeBytes (Join-Path $repoRoot ([string]$forbidden[0].artifactPath))
    [IO.File]::WriteAllBytes($contaminatedPath,
        (Add-AlignedBytes $targetBytes $forbiddenBytes))
    Invoke-ScannerExpectFailure $contaminatedPath 'observed 3'

    $metadataPath = Join-Path $temporaryRoot 'forbidden-metadata.bin'
    $metadataBytes = [Text.Encoding]::ASCII.GetBytes([string]$forbidden[0].key)
    [IO.File]::WriteAllBytes($metadataPath,
        (Add-AlignedBytes $targetBytes $metadataBytes))
    Invoke-ScannerExpectFailure $metadataPath 'Non-selected semantic key leaked'

    $compatibilityPath = Join-Path $temporaryRoot 'compatibility-stream.bin'
    $compatibilityBytes = Get-IncludeBytes (
        Join-Path $repoRoot 'src\vulkan\raytracing\MinimalLegacyRayGenShader.inc')
    [IO.File]::WriteAllBytes($compatibilityPath,
        (Add-AlignedBytes $targetBytes $compatibilityBytes))
    Invoke-ScannerExpectFailure $compatibilityPath 'observed 3'
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}

Write-Output 'Final-target containment scanner adversarial controls passed.'
