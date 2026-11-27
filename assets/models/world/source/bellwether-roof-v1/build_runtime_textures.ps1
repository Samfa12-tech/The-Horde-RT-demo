$ErrorActionPreference = "Stop"
$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\..\..\..\..")).Path
$source = Join-Path $PSScriptRoot "maps"
$output = Join-Path $root "assets\textures\props\runtime\bellwether-roof-v1"
$compiler = Join-Path $root "tools\compile-static-texture-array.ps1"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$maps = @(
    @{ Name = "base-color"; Transfer = "srgb"; Windows = "R8G8B8A8_SRGB"; Android = "ASTC_6x6_SRGB_BLOCK" },
    @{ Name = "normal"; Transfer = "linear"; Windows = "R8G8B8A8_UNORM"; Android = "ASTC_4x4_UNORM_BLOCK" },
    @{ Name = "orm"; Transfer = "linear"; Windows = "R8G8B8A8_UNORM"; Android = "ASTC_6x6_UNORM_BLOCK" }
)
foreach ($map in $maps) {
    foreach ($platform in @("windows", "android")) {
        $format = if ($platform -eq "windows") { $map.Windows } else { $map.Android }
        $input = Join-Path $source "$($map.Name).png"
        $target = Join-Path $output "$($map.Name).$platform.ktx2"
        & $compiler -InputPaths $input -OutputPath $target -Format $format `
            -Transfer $map.Transfer -Width 1024 -Height 1024 -AssignPrimaries bt709
        if ($LASTEXITCODE -ne 0) { throw "Texture compilation failed: $($map.Name).$platform" }
    }
}
Write-Output "Built one-layer 1024-square KTX2 roof family for Windows and Android."
