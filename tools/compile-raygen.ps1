param(
    [string]$VulkanSdk = $env:VULKAN_SDK
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

$source = Join-Path $repoRoot 'shaders\raytracing\minimal.rgen'
$spirv = Join-Path $repoRoot 'shaders\raytracing\minimal.rgen.spv'
$include = Join-Path $repoRoot 'src\vulkan\raytracing\MinimalRayGenShader.inc'

& $validator -V --target-env vulkan1.2 -Os -S rgen -o $spirv $source
if ($LASTEXITCODE -ne 0)
{
    throw "Raygen compilation failed with exit code $LASTEXITCODE"
}

$bytes = [System.IO.File]::ReadAllBytes($spirv)
if (($bytes.Length % 4) -ne 0)
{
    throw 'Compiled SPIR-V is not 32-bit word aligned.'
}

$words = for ($offset = 0; $offset -lt $bytes.Length; $offset += 4)
{
    '0x{0:x8}u' -f [BitConverter]::ToUInt32($bytes, $offset)
}
$lines = for ($index = 0; $index -lt $words.Count; $index += 8)
{
    $last = [Math]::Min($index + 7, $words.Count - 1)
    '    ' + (($words[$index..$last] -join ', ') + ',')
}

[System.IO.File]::WriteAllText(
    $include,
    ($lines -join "`n") + "`n",
    [System.Text.UTF8Encoding]::new($false))

$hash = (Get-FileHash -LiteralPath $spirv -Algorithm SHA256).Hash
Write-Output "Generated $include"
Write-Output "SPIR-V bytes: $($bytes.Length); words: $($words.Count); SHA-256: $hash"
