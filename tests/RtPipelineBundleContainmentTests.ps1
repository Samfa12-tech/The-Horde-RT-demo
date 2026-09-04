[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Scanner,
    [Parameter(Mandatory = $true)][string]$TargetPath,
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
function Invoke-ScannerExpectFailure([string]$path, [string]$expectedText) {
    $output = (& $PowerShellExecutable -NoProfile -File $Scanner -TargetPath $path `
        -Instrumentation $Instrumentation -Quality $Quality -SkipExternalValidation 2>&1 |
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
    $zeroPath = Join-Path $temporaryRoot 'zero-modules.bin'
    [IO.File]::WriteAllBytes($zeroPath, (New-Object byte[] 64))
    Invoke-ScannerExpectFailure $zeroPath 'observed 0'

    $missingPath = Join-Path $temporaryRoot 'unreadable.bin'
    Invoke-ScannerExpectFailure $missingPath 'Cannot find path'

    $malformedPath = Join-Path $temporaryRoot 'malformed-raygen.bin'
    $malformedWords = [uint32[]]@(0x07230203,0x00010500,0,2,0,
                                  ((3 -shl 16) -bor 15),5313,1,0)
    $malformed = New-Object byte[] ($malformedWords.Count * 4)
    for ($index = 0; $index -lt $malformedWords.Count; ++$index) {
        [Array]::Copy([BitConverter]::GetBytes($malformedWords[$index]), 0,
                      $malformed, $index * 4, 4)
    }
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
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}

Write-Output 'Final-target containment scanner adversarial controls passed.'
