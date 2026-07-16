param(
    [string]$GlbPath = "",
    [string]$KtxPath = "",
    [switch]$KeepPreview
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($GlbPath)) {
    $GlbPath = Join-Path $repoRoot "assets\models\enemies\meshy\lich_placeholder_merged_animations_v01.glb"
}
$outputRoot = Join-Path $repoRoot "assets\textures\meshy\lich_placeholder_v01"
$tempRoot = Join-Path $env:TEMP "horde-lich-textures-v01"
$expectedSourceHash = "049979A83ACA55358F54AF8D3AF1F7D518607BEF634474A4EF015BFDFF947A42"

if ([string]::IsNullOrWhiteSpace($KtxPath)) {
    $ktxCommand = Get-Command ktx.exe -ErrorAction SilentlyContinue
    if ($null -eq $ktxCommand) { $ktxCommand = Get-Command ktx -ErrorAction SilentlyContinue }
    if ($null -ne $ktxCommand) {
        $KtxPath = $ktxCommand.Source
    } else {
        $sharedFallback = "C:\Users\sam_s\Documents\Codex\shared-tools\KTX-Software-4.4.2-src\build-local\Release\ktx.exe"
        if (Test-Path -LiteralPath $sharedFallback) { $KtxPath = $sharedFallback }
    }
}
if (-not (Test-Path -LiteralPath $KtxPath)) {
    throw "Khronos ktx.exe was not found on PATH. Pass -KtxPath explicitly."
}
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $GlbPath).Hash -ne $expectedSourceHash) {
    throw "The lich GLB does not match the audited v01 source. Audit a new revision before deriving textures."
}

Add-Type -AssemblyName System.Drawing
if (-not ("HordeLichTexturePrep" -as [type])) {
    Add-Type -ReferencedAssemblies @(
        [System.Drawing.Bitmap].Assembly.Location,
        [System.Drawing.Rectangle].Assembly.Location,
        (Join-Path $PSHOME "System.Private.Windows.GdiPlus.dll"),
        (Join-Path $PSHOME "System.Private.Windows.Core.dll")
    ) -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class HordeLichTexturePrep
{
    public static byte[] DecodeRgba(byte[] encoded, out int width, out int height)
    {
        using (var stream = new System.IO.MemoryStream(encoded, false))
        using (var decoded = new Bitmap(stream))
        using (var bitmap = new Bitmap(decoded.Width, decoded.Height, PixelFormat.Format32bppArgb))
        {
            width = decoded.Width;
            height = decoded.Height;
            using (Graphics graphics = Graphics.FromImage(bitmap)) graphics.DrawImageUnscaled(decoded, 0, 0);
            var rectangle = new Rectangle(0, 0, width, height);
            BitmapData locked = bitmap.LockBits(rectangle, ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
            try
            {
                int stride = Math.Abs(locked.Stride);
                byte[] bgra = new byte[stride * height];
                Marshal.Copy(locked.Scan0, bgra, 0, bgra.Length);
                byte[] rgba = new byte[width * height * 4];
                for (int y = 0; y < height; ++y)
                {
                    int sourceRow = locked.Stride >= 0 ? y * stride : (height - 1 - y) * stride;
                    int destinationRow = y * width * 4;
                    for (int x = 0; x < width; ++x)
                    {
                        int source = sourceRow + x * 4;
                        int destination = destinationRow + x * 4;
                        rgba[destination + 0] = bgra[source + 2];
                        rgba[destination + 1] = bgra[source + 1];
                        rgba[destination + 2] = bgra[source + 0];
                        rgba[destination + 3] = bgra[source + 3];
                    }
                }
                return rgba;
            }
            finally { bitmap.UnlockBits(locked); }
        }
    }

    public static byte[] DeriveVioletEmission(byte[] baseRgba)
    {
        byte[] result = new byte[baseRgba.Length];
        for (int offset = 0; offset < baseRgba.Length; offset += 4)
        {
            int red = baseRgba[offset + 0];
            int green = baseRgba[offset + 1];
            int blue = baseRgba[offset + 2];
            int alpha = baseRgba[offset + 3];
            int blueExcess = blue - green;
            int redExcess = red - green;
            bool violetGem = alpha > 16 && blue >= 145 && red >= 90 && blueExcess >= 40 && redExcess >= 22;
            int strength = violetGem ? Math.Min(255, Math.Max(0, (Math.Max(red, blue) - 120) * 2 + blueExcess)) : 0;
            result[offset + 0] = (byte)(red * strength / 255);
            result[offset + 1] = (byte)(green * strength / 255);
            result[offset + 2] = (byte)(blue * strength / 255);
            result[offset + 3] = 255;
        }
        return result;
    }

    public static void SaveRgbaPreview(byte[] rgba, int width, int height, string path)
    {
        using (var bitmap = new Bitmap(width, height, PixelFormat.Format32bppArgb))
        {
            var rectangle = new Rectangle(0, 0, width, height);
            BitmapData locked = bitmap.LockBits(rectangle, ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
            try
            {
                int stride = Math.Abs(locked.Stride);
                byte[] bgra = new byte[stride * height];
                for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x)
                {
                    int source = (y * width + x) * 4;
                    int destination = y * stride + x * 4;
                    bgra[destination + 0] = rgba[source + 2];
                    bgra[destination + 1] = rgba[source + 1];
                    bgra[destination + 2] = rgba[source + 0];
                    bgra[destination + 3] = rgba[source + 3];
                }
                Marshal.Copy(bgra, 0, locked.Scan0, bgra.Length);
            }
            finally { bitmap.UnlockBits(locked); }
            bitmap.Save(path, ImageFormat.Png);
        }
    }
}
"@
}

$fileBytes = [IO.File]::ReadAllBytes($GlbPath)
$reader = New-Object IO.BinaryReader (New-Object IO.MemoryStream (,$fileBytes))
if ($reader.ReadUInt32() -ne 0x46546c67 -or $reader.ReadUInt32() -ne 2) { throw "Expected a glTF 2.0 GLB." }
$null = $reader.ReadUInt32()
$jsonText = $null
$binary = $null
while ($reader.BaseStream.Position + 8 -le $reader.BaseStream.Length) {
    $length = $reader.ReadUInt32()
    $type = $reader.ReadUInt32()
    $chunk = $reader.ReadBytes($length)
    if ($type -eq 0x4e4f534a) { $jsonText = [Text.Encoding]::UTF8.GetString($chunk).TrimEnd([char]0, ' ', "`t", "`r", "`n") }
    if ($type -eq 0x004e4942) { $binary = $chunk }
}
$reader.Dispose()
if ($null -eq $jsonText -or $null -eq $binary) { throw "GLB is missing JSON or BIN data." }
$gltf = $jsonText | ConvertFrom-Json -Depth 100
$primitive = $gltf.meshes[0].primitives[0]
$material = $gltf.materials[$primitive.material]

function Get-EmbeddedTextureBytes([int]$textureIndex) {
    $imageIndex = [int]$gltf.textures[$textureIndex].source
    $image = $gltf.images[$imageIndex]
    if ($image.mimeType -ne "image/png") { throw "Only the audited embedded PNG layout is supported." }
    $view = $gltf.bufferViews[$image.bufferView]
    $offset = if ($null -eq $view.byteOffset) { 0 } else { [int]$view.byteOffset }
    $result = New-Object byte[] ([int]$view.byteLength)
    [Array]::Copy($binary, $offset, $result, 0, $result.Length)
    return $result
}

$baseEncoded = Get-EmbeddedTextureBytes ([int]$material.pbrMetallicRoughness.baseColorTexture.index)
$sourceEmissionEncoded = Get-EmbeddedTextureBytes ([int]$material.emissiveTexture.index)
$width = 0; $height = 0; $emissionWidth = 0; $emissionHeight = 0
$baseRgba = [HordeLichTexturePrep]::DecodeRgba($baseEncoded, [ref]$width, [ref]$height)
$sourceEmissionRgba = [HordeLichTexturePrep]::DecodeRgba($sourceEmissionEncoded, [ref]$emissionWidth, [ref]$emissionHeight)
if ($width -ne 2048 -or $height -ne 2048 -or $emissionWidth -ne $width -or $emissionHeight -ne $height) {
    throw "The audited v01 texture dimensions changed."
}
if (-not [Linq.Enumerable]::SequenceEqual([byte[]]$baseRgba, [byte[]]$sourceEmissionRgba)) {
    throw "The v01 embedded base and emissive images no longer decode identically; audit the real emissive source instead of thresholding it."
}
$emissiveRgba = [HordeLichTexturePrep]::DeriveVioletEmission($baseRgba)

New-Item -ItemType Directory -Force -Path $outputRoot,$tempRoot | Out-Null
$jobs = @(
    @{ Name = "base-color"; Rgba = $baseRgba; Perceptual = $true },
    @{ Name = "emissive"; Rgba = $emissiveRgba; Perceptual = $false }
)
foreach ($job in $jobs) {
    $rawPath = Join-Path $tempRoot ($job.Name + "-2048.rgba")
    [IO.File]::WriteAllBytes($rawPath, $job.Rgba)
    $uncompressed = Join-Path $tempRoot ($job.Name + "-uncompressed.ktx2")
    $windowsOutput = Join-Path $outputRoot ($job.Name + "-2048-rgba8.ktx2")
    $output = Join-Path $outputRoot ($job.Name + "-2048-astc6x6.ktx2")
    & $KtxPath create --format R8G8B8A8_SRGB --raw --width $width --height $height --levels 1 --assign-tf srgb --assign-texcoord-origin top-left $rawPath $uncompressed
    if ($LASTEXITCODE -ne 0) { throw "KTX creation failed for $($job.Name)." }
    Copy-Item -LiteralPath $uncompressed -Destination $windowsOutput -Force
    $encodeArgs = @("encode", "--format", "ASTC_6x6_SRGB_BLOCK", "--astc-quality", "thorough")
    if ($job.Perceptual) { $encodeArgs += "--astc-perceptual" }
    $encodeArgs += @($uncompressed, $output)
    & $KtxPath @encodeArgs
    if ($LASTEXITCODE -ne 0) { throw "ASTC encoding failed for $($job.Name)." }
    & $KtxPath validate $output
    if ($LASTEXITCODE -ne 0) { throw "KTX validation failed for '$output'." }
    $header = [IO.File]::ReadAllBytes($output)
    if ([BitConverter]::ToUInt32($header, 12) -ne 166) { throw "'$output' is not VK_FORMAT_ASTC_6x6_SRGB_BLOCK." }
}

if ($KeepPreview) {
    [HordeLichTexturePrep]::SaveRgbaPreview($emissiveRgba, $width, $height, (Join-Path $outputRoot "emissive-mask-preview.png"))
}

Get-Item (Join-Path $outputRoot "*.ktx2") |
    Select-Object Name,Length,@{Name="SHA256";Expression={(Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash}}
