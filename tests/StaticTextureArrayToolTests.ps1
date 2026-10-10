[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) ("horde-static-texture-array-" + [guid]::NewGuid().ToString("N"))
$outputPath = Join-Path $temporaryRoot "two-layer.ktx2"
New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
try {
    & (Join-Path $repoRoot "tools\compile-static-texture-array.ps1") `
        -InputPaths @(
            (Join-Path $repoRoot "assets\textures\props\source\static-array-1k\sword-base-color.png"),
            (Join-Path $repoRoot "assets\textures\props\source\collapsed-entry\boulder01-base-color.png")) `
        -OutputPath $outputPath `
        -Format R8G8B8A8_SRGB `
        -Transfer srgb `
        -AssignPrimaries bt709 `
        -Width 512 `
        -Height 512
    if ($LASTEXITCODE -ne 0) { throw "Static texture array compiler returned exit $LASTEXITCODE." }
    $bytes = [IO.File]::ReadAllBytes($outputPath)
    if ($bytes.Length -lt 80) { throw "Compiled KTX2 is shorter than its header." }
    $layers = [BitConverter]::ToUInt32($bytes, 32)
    $width = [BitConverter]::ToUInt32($bytes, 20)
    $height = [BitConverter]::ToUInt32($bytes, 24)
    if ($layers -ne 2 -or $width -ne 512 -or $height -ne 512) {
        throw "Expected a 512x512 two-layer KTX2 array; got ${width}x${height} with $layers layer(s)."
    }
    $levels = [BitConverter]::ToUInt32($bytes, 40)
    $descriptorOffset = [BitConverter]::ToUInt32($bytes, 48)
    if ($levels -ne 10 -or $descriptorOffset + 16 -gt $bytes.Length -or
        $bytes[$descriptorOffset + 13] -ne 1 -or $bytes[$descriptorOffset + 14] -ne 2) {
        throw "Expected full mip chain and assigned BT709/sRGB metadata for mixed-metadata input PNGs."
    }
    $sourceImage = Join-Path $repoRoot "assets\textures\props\source\static-array-1k\sword-base-color.png"
    $oneOverCapacity = @($sourceImage) * 19
    try {
        & (Join-Path $repoRoot "tools\compile-static-texture-array.ps1") `
            -InputPaths $oneOverCapacity `
            -OutputPath (Join-Path $temporaryRoot "nineteen-layer-overflow.ktx2") `
            -Format R8G8B8A8_SRGB -Transfer srgb -Width 512 -Height 512
        throw "Expected the 19-layer input to exceed the approved 18-layer capacity."
    }
    catch {
        if ($_.Exception.Message -notmatch "cannot exceed the approved 18-layer capacity") { throw }
    }
    Write-Output "Static texture array tool contract passed: two real layers, full mips, mixed metadata, and 19-layer overflow rejection"
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        $resolvedTemporaryRoot = [IO.Path]::GetFullPath($temporaryRoot)
        $expectedTemporaryPrefix = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
        if (-not $resolvedTemporaryRoot.StartsWith($expectedTemporaryPrefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing cleanup outside the fixture temporary directory."
        }
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}
