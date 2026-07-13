param(
    [string]$KtxPath = "C:\Users\sam_s\Documents\Codex\shared-tools\KTX-Software-4.4.2-src\build-local\Release\ktx.exe"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$assetRoot = Join-Path $repoRoot "assets\textures\polyhaven\mobile_1k"
$tempRoot = Join-Path $env:TEMP "horde-lantern-material-ktx"

if (-not (Test-Path -LiteralPath $KtxPath)) {
    throw "Khronos ktx.exe was not found at '$KtxPath'. Pass -KtxPath explicitly."
}

New-Item -ItemType Directory -Force -Path $tempRoot | Out-Null

$jobs = @(
    @{ Name = "diff"; InputFormat = "R8G8B8A8_SRGB"; OutputFormat = "ASTC_6x6_SRGB_BLOCK"; ExpectedVkFormat = 166; Output = "diff-array-512-astc6x6.ktx2"; Transfer = "srgb"; Perceptual = $true },
    @{ Name = "normal"; InputFormat = "R8G8B8A8_UNORM"; OutputFormat = "ASTC_4x4_UNORM_BLOCK"; ExpectedVkFormat = 157; Output = "normal-array-512-astc4x4.ktx2"; Transfer = "linear"; Perceptual = $false },
    @{ Name = "arm"; InputFormat = "R8G8B8A8_UNORM"; OutputFormat = "ASTC_6x6_UNORM_BLOCK"; ExpectedVkFormat = 165; Output = "arm-array-512-astc6x6.ktx2"; Transfer = "linear"; Perceptual = $false }
)

foreach ($job in $jobs) {
    $source = Join-Path $assetRoot ($job.Name + "-array-512.rgba")
    $bytes = [IO.File]::ReadAllBytes($source)
    $layerSize = 512 * 512 * 4
    if ($bytes.Length -ne $layerSize * 5) {
        throw "Unexpected raw array size for '$source'."
    }

    $inputs = @()
    for ($layer = 0; $layer -lt 5; ++$layer) {
        $slice = New-Object byte[] $layerSize
        [Array]::Copy($bytes, $layer * $layerSize, $slice, 0, $layerSize)
        $layerPath = Join-Path $tempRoot ($job.Name + "-" + $layer + ".rgba")
        [IO.File]::WriteAllBytes($layerPath, $slice)
        $inputs += $layerPath
    }

    $intermediate = Join-Path $tempRoot ($job.Name + "-uncompressed.ktx2")
    & $KtxPath create --format $job.InputFormat --raw --width 512 --height 512 --layers 5 --levels 1 --assign-tf $job.Transfer --assign-texcoord-origin top-left @inputs $intermediate
    if ($LASTEXITCODE -ne 0) { throw "KTX creation failed for $($job.Name)." }

    $output = Join-Path $assetRoot $job.Output
    $encodeArgs = @("encode", "--format", $job.OutputFormat, "--astc-quality", "thorough")
    if ($job.Perceptual) { $encodeArgs += "--astc-perceptual" }
    $encodeArgs += @($intermediate, $output)
    & $KtxPath @encodeArgs
    if ($LASTEXITCODE -ne 0) { throw "ASTC encoding failed for $($job.Name)." }
    & $KtxPath validate $output
    if ($LASTEXITCODE -ne 0) { throw "KTX validation failed for '$output'." }

    $header = [IO.File]::ReadAllBytes($output)
    $actualVkFormat = [BitConverter]::ToUInt32($header, 12)
    if ($actualVkFormat -ne $job.ExpectedVkFormat) {
        throw "'$output' encoded as VkFormat $actualVkFormat, expected $($job.ExpectedVkFormat). Khronos KTX 4.4.2 requires its command_encode ASTC block-dimension fix for 6x6 output."
    }
}

Get-Item (Join-Path $assetRoot "*-astc*.ktx2") | Select-Object Name, Length
