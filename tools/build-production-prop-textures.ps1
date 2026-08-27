[CmdletBinding()]
param(
    [string]$PythonPath = "python"
)

$ErrorActionPreference = "Stop"
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
& $PythonPath (Join-Path $PSScriptRoot "generate-production-prop-textures.py")
if ($LASTEXITCODE -ne 0) { throw "Production prop source texture generation failed." }

$shared = Join-Path $repoRoot "assets/textures/props/source/static-array-1k"
$props = Join-Path $repoRoot "assets/textures/props/source"
$runtime = Join-Path $repoRoot "assets/textures/props/runtime"
$base = @(
    "$shared/sword-base-color.png", "$shared/torch-base-color.png",
    "$shared/player-base-color.png", "$props/chest-wood-base-color.png",
    "$props/chest-iron-base-color.png", "$props/chest-wood-base-color.png",
    "$props/chest-iron-base-color.png", "$props/lantern-iron-base-color.png",
    "$props/lantern-iron-base-color.png")
$normal = @(
    "$shared/sword-normal.png", "$shared/torch-normal.png", "$shared/player-normal.png",
    "$props/chest-wood-normal.png", "$props/chest-iron-normal.png",
    "$props/chest-wood-normal.png", "$props/chest-iron-normal.png",
    "$props/lantern-iron-normal.png", "$props/lantern-iron-normal.png")
$orm = @(
    "$shared/sword-orm.png", "$shared/torch-orm.png", "$shared/player-orm.png",
    "$props/chest-wood-orm.png", "$props/chest-iron-orm.png",
    "$props/chest-wood-orm.png", "$props/chest-iron-orm.png",
    "$props/lantern-iron-orm.png", "$props/lantern-iron-orm.png")
$compiler = Join-Path $PSScriptRoot "compile-static-texture-array.ps1"

& $compiler -InputPaths $base -OutputPath "$runtime/base-color.windows.ktx2" `
    -Format R8G8B8A8_SRGB -Transfer srgb -Width 1024 -Height 1024
& $compiler -InputPaths $base -OutputPath "$runtime/base-color.android.ktx2" `
    -Format ASTC_6x6_SRGB_BLOCK -Transfer srgb -Width 1024 -Height 1024
& $compiler -InputPaths $normal -OutputPath "$runtime/normal.windows.ktx2" `
    -Format R8G8B8A8_UNORM -Transfer linear -Width 1024 -Height 1024
& $compiler -InputPaths $normal -OutputPath "$runtime/normal.android.ktx2" `
    -Format ASTC_4x4_UNORM_BLOCK -Transfer linear -Width 1024 -Height 1024
& $compiler -InputPaths $orm -OutputPath "$runtime/orm.windows.ktx2" `
    -Format R8G8B8A8_UNORM -Transfer linear -Width 1024 -Height 1024
& $compiler -InputPaths $orm -OutputPath "$runtime/orm.android.ktx2" `
    -Format ASTC_6x6_UNORM_BLOCK -Transfer linear -Width 1024 -Height 1024
& $compiler -InputPaths @("$shared/torch-emissive.png") `
    -OutputPath "$runtime/emissive.windows.ktx2" `
    -Format R8G8B8A8_SRGB -Transfer srgb -Width 1024 -Height 1024
& $compiler -InputPaths @("$shared/torch-emissive.png") `
    -OutputPath "$runtime/emissive.android.ktx2" `
    -Format ASTC_6x6_SRGB_BLOCK -Transfer srgb -Width 1024 -Height 1024

function Get-Sha([string]$name) {
    return (Get-FileHash -LiteralPath (Join-Path $runtime $name) -Algorithm SHA256).Hash.ToLowerInvariant()
}
$manifest = [ordered]@{
    schema = 1
    asset = "production-static-prop-pbr-arrays"
    resolution = 1024
    mipmapped = $true
    layerOrder = @(
        "gothic-arming-sword-rh-lod0", "gothic-hand-torch-lod0",
        "gothic-traveller-lod0", "gothic-chest-base.ChestWood",
        "gothic-chest-base.BlackIron", "gothic-chest-lid.ChestWood",
        "gothic-chest-lid.BlackIron", "reward-lantern-ring.BlackIron",
        "reward-lantern-body.BlackIron")
    layerCounts = [ordered]@{ baseColor = 9; normal = 9; orm = 9; emissive = 1 }
    android = [ordered]@{
        baseColor = [ordered]@{ format = "ASTC_6x6_SRGB_BLOCK"; sha256 = Get-Sha "base-color.android.ktx2" }
        normal = [ordered]@{ format = "ASTC_4x4_UNORM_BLOCK"; sha256 = Get-Sha "normal.android.ktx2" }
        orm = [ordered]@{ format = "ASTC_6x6_UNORM_BLOCK"; sha256 = Get-Sha "orm.android.ktx2" }
        emissive = [ordered]@{ format = "ASTC_6x6_SRGB_BLOCK"; sha256 = Get-Sha "emissive.android.ktx2" }
    }
    windows = [ordered]@{
        baseColor = [ordered]@{ format = "R8G8B8A8_SRGB"; sha256 = Get-Sha "base-color.windows.ktx2" }
        normal = [ordered]@{ format = "R8G8B8A8_UNORM"; sha256 = Get-Sha "normal.windows.ktx2" }
        orm = [ordered]@{ format = "R8G8B8A8_UNORM"; sha256 = Get-Sha "orm.windows.ktx2" }
        emissive = [ordered]@{ format = "R8G8B8A8_SRGB"; sha256 = Get-Sha "emissive.windows.ktx2" }
    }
    licenceStatus = "Meshy outputs distributed conservatively under CC BY 4.0; attribution in ASSET_LICENSES.md"
}
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $runtime "asset.manifest.json") -Encoding utf8
Write-Output "Built deterministic 1K production static-prop texture arrays and manifest."
