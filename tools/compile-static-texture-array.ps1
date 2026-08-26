[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string[]]$InputPaths,
    [Parameter(Mandatory = $true)][string]$OutputPath,
    [Parameter(Mandatory = $true)][ValidateSet(
        "R8G8B8A8_SRGB", "R8G8B8A8_UNORM",
        "ASTC_4x4_SRGB_BLOCK", "ASTC_4x4_UNORM_BLOCK",
        "ASTC_6x6_SRGB_BLOCK", "ASTC_6x6_UNORM_BLOCK")][string]$Format,
    [Parameter(Mandatory = $true)][ValidateSet("srgb", "linear")][string]$Transfer,
    [int]$Width = 512,
    [int]$Height = 512,
    [string]$KtxPath = ""
)

$ErrorActionPreference = "Stop"
if ($InputPaths.Count -lt 1) { throw "Static texture arrays require at least one input layer." }
if ($InputPaths.Count -gt 16) { throw "Static texture arrays cannot exceed 16 layers." }
if ($Width -lt 1 -or $Height -lt 1) { throw "Static texture array dimensions must be greater than zero." }
$resolvedInputs = @($InputPaths | ForEach-Object { [IO.Path]::GetFullPath($_) })
foreach ($inputPath in $resolvedInputs) {
    if (-not (Test-Path -LiteralPath $inputPath)) { throw "Static texture layer source is missing: $inputPath" }
}
if ([string]::IsNullOrWhiteSpace($KtxPath)) {
    $command = Get-Command ktx -ErrorAction SilentlyContinue
    if ($null -ne $command) { $KtxPath = $command.Source }
}
if ([string]::IsNullOrWhiteSpace($KtxPath)) {
    $KtxPath = "C:\Users\sam_s\Documents\Codex\shared-tools\KTX-Software-4.4.2-src\build-local\Release\ktx.exe"
}
if (-not (Test-Path -LiteralPath $KtxPath)) { throw "KTX-Software 4.4.2 is required for runtime texture compilation." }
$resolvedOutput = [IO.Path]::GetFullPath($OutputPath)
New-Item -ItemType Directory -Path (Split-Path -Parent $resolvedOutput) -Force | Out-Null
& $KtxPath create --testrun --format $Format --layers $resolvedInputs.Count `
    --width $Width --height $Height --generate-mipmap --assign-tf $Transfer `
    @resolvedInputs $resolvedOutput
if ($LASTEXITCODE -ne 0) { throw "KTX2 array compilation failed for $resolvedOutput." }
& $KtxPath validate $resolvedOutput
if ($LASTEXITCODE -ne 0) { throw "KTX2 validation failed for $resolvedOutput." }
Write-Output "Compiled $($resolvedInputs.Count)-layer static texture array: $resolvedOutput"
