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
            (Join-Path $repoRoot "assets\models\weapons\meshy\gothic_arming_sword_rh_v01_textures\base_color.png"),
            (Join-Path $repoRoot "assets\models\weapons\meshy\gothic_arming_sword_rh_v01_textures\emission.png")) `
        -OutputPath $outputPath `
        -Format R8G8B8A8_SRGB `
        -Transfer srgb `
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
    Write-Output "Static texture array tool contract passed: two real layers"
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}
