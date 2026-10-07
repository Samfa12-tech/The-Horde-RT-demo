[CmdletBinding()]
param(
    [string]$PythonPath = "python",
    [string]$OutputDirectory = "",
    [string]$KtxPath = "",
    [switch]$SkipSourceGeneration
)

$ErrorActionPreference = "Stop"
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$shared = Join-Path $repoRoot "assets/textures/props/source/static-array-1k"
$props = Join-Path $repoRoot "assets/textures/props/source"
$collapse = Join-Path $props "collapsed-entry"
foreach ($family in @("boulder01", "medieval-wall02")) {
    foreach ($kind in @("base-color", "normal", "orm")) {
        $sourcePath = Join-Path $collapse "$family-$kind.png"
        if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
            throw "Approved collapsed-entry source texture is missing: $sourcePath"
        }
        $pngHeader = [IO.File]::ReadAllBytes($sourcePath)
        if ($pngHeader.Length -lt 33 -or $pngHeader[0] -ne 137 -or
            [Text.Encoding]::ASCII.GetString($pngHeader, 1, 3) -ne "PNG" -or
            [Text.Encoding]::ASCII.GetString($pngHeader, 12, 4) -ne "IHDR") {
            throw "Collapsed-entry source must be an actual PNG: $sourcePath"
        }
        $sourceWidth = ([uint32]$pngHeader[16] -shl 24) -bor ([uint32]$pngHeader[17] -shl 16) -bor
                       ([uint32]$pngHeader[18] -shl 8) -bor [uint32]$pngHeader[19]
        $sourceHeight = ([uint32]$pngHeader[20] -shl 24) -bor ([uint32]$pngHeader[21] -shl 16) -bor
                        ([uint32]$pngHeader[22] -shl 8) -bor [uint32]$pngHeader[23]
        if ($sourceWidth -ne 1024 -or $sourceHeight -ne 1024) {
            throw "Collapsed-entry source must remain exact 1K; implicit resizing is not admitted: $sourcePath"
        }
    }
}
if (-not $SkipSourceGeneration) {
    & $PythonPath (Join-Path $PSScriptRoot "generate-production-prop-textures.py")
    if ($LASTEXITCODE -ne 0) { throw "Production prop source texture generation failed." }
}
$runtime = if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    Join-Path $repoRoot "assets/textures/props/runtime"
} else { [IO.Path]::GetFullPath($OutputDirectory) }
$base = @(
    "$shared/sword-base-color.png", "$shared/torch-base-color.png",
    "$shared/player-base-color.png", "$shared/player-gauntlet-base-color.png",
    "$props/chest-wood-base-color.png",
    "$props/chest-iron-base-color.png", "$props/chest-wood-base-color.png",
    "$props/chest-iron-base-color.png", "$props/lantern-iron-base-color.png",
    "$props/lantern-iron-base-color.png", "$collapse/boulder01-base-color.png",
    "$collapse/medieval-wall02-base-color.png", "$props/rag-torch-v01/base-color.png")
$normal = @(
    "$shared/sword-normal.png", "$shared/torch-normal.png", "$shared/player-normal.png",
    "$shared/player-gauntlet-normal.png",
    "$props/chest-wood-normal.png", "$props/chest-iron-normal.png",
    "$props/chest-wood-normal.png", "$props/chest-iron-normal.png",
    "$props/lantern-iron-normal.png", "$props/lantern-iron-normal.png",
    "$collapse/boulder01-normal.png", "$collapse/medieval-wall02-normal.png",
    "$props/rag-torch-v01/normal.png")
$orm = @(
    "$shared/sword-orm.png", "$shared/torch-orm.png", "$shared/player-orm.png",
    "$shared/player-gauntlet-orm.png",
    "$props/chest-wood-orm.png", "$props/chest-iron-orm.png",
    "$props/chest-wood-orm.png", "$props/chest-iron-orm.png",
    "$props/lantern-iron-orm.png", "$props/lantern-iron-orm.png",
    "$collapse/boulder01-orm.png", "$collapse/medieval-wall02-orm.png",
    "$props/rag-torch-v01/orm.png")
$compiler = Join-Path $PSScriptRoot "compile-static-texture-array.ps1"
$compilerOptions = @{ AssignPrimaries = "bt709" }
if (-not [string]::IsNullOrWhiteSpace($KtxPath)) { $compilerOptions.KtxPath = $KtxPath }

& $compiler -InputPaths $base -OutputPath "$runtime/base-color.windows.ktx2" `
    -Format R8G8B8A8_SRGB -Transfer srgb -Width 1024 -Height 1024 @compilerOptions
& $compiler -InputPaths $base -OutputPath "$runtime/base-color.android.ktx2" `
    -Format ASTC_6x6_SRGB_BLOCK -Transfer srgb -Width 1024 -Height 1024 @compilerOptions
& $compiler -InputPaths $normal -OutputPath "$runtime/normal.windows.ktx2" `
    -Format R8G8B8A8_UNORM -Transfer linear -Width 1024 -Height 1024 @compilerOptions
& $compiler -InputPaths $normal -OutputPath "$runtime/normal.android.ktx2" `
    -Format ASTC_4x4_UNORM_BLOCK -Transfer linear -Width 1024 -Height 1024 @compilerOptions
& $compiler -InputPaths $orm -OutputPath "$runtime/orm.windows.ktx2" `
    -Format R8G8B8A8_UNORM -Transfer linear -Width 1024 -Height 1024 @compilerOptions
& $compiler -InputPaths $orm -OutputPath "$runtime/orm.android.ktx2" `
    -Format ASTC_6x6_UNORM_BLOCK -Transfer linear -Width 1024 -Height 1024 @compilerOptions
& $compiler -InputPaths @("$shared/torch-emissive.png") `
    -OutputPath "$runtime/emissive.windows.ktx2" `
    -Format R8G8B8A8_SRGB -Transfer srgb -Width 1024 -Height 1024 @compilerOptions
& $compiler -InputPaths @("$shared/torch-emissive.png") `
    -OutputPath "$runtime/emissive.android.ktx2" `
    -Format ASTC_6x6_SRGB_BLOCK -Transfer srgb -Width 1024 -Height 1024 @compilerOptions

function Get-Sha([string]$name) {
    return (Get-FileHash -LiteralPath (Join-Path $runtime $name) -Algorithm SHA256).Hash.ToLowerInvariant()
}
function Get-SourceRecord([string]$path) {
    return [ordered]@{
        path = [IO.Path]::GetFullPath($path).Substring($repoRoot.Length + 1).Replace('\', '/')
        bytes = (Get-Item -LiteralPath $path).Length
        sha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}
$manifest = [ordered]@{
    schema = 1
    asset = "production-static-prop-pbr-arrays"
    resolution = 1024
    mipmapped = $true
    layerOrder = @(
        "gothic-arming-sword-rh-lod0", "gothic-hand-torch-lod0",
        "gothic-traveller-lod0", "gothic-traveller-lod0.GauntletPrimaryVisible",
        "gothic-chest-base.ChestWood",
        "gothic-chest-base.BlackIron", "gothic-chest-lid.ChestWood",
        "gothic-chest-lid.BlackIron", "reward-lantern-ring.BlackIron",
        "reward-lantern-body.BlackIron", "collapsed-entry/Boulder01Rock",
        "collapsed-entry/MedievalWall02", "rag-torch-player/RagTorch_Atlas")
    layerCounts = [ordered]@{ baseColor = 13; normal = 13; orm = 13; emissive = 1 }
    sourceLayers = @(
        for ($layer = 0; $layer -lt $base.Count; ++$layer) {
            [ordered]@{ layer = $layer; baseColor = Get-SourceRecord $base[$layer];
                normal = Get-SourceRecord $normal[$layer]; orm = Get-SourceRecord $orm[$layer] }
        }
    )
    normalConvention = "linear glTF/OpenGL +Y; existing generated-mipmap filter unchanged; runtime renormalizes mapped normals"
    colorPrimaries = "BT709/sRGB assigned metadata only, no pixel conversion"
    ormConvention = "R ambient occlusion, G roughness, B metallic; source ARM is already this channel order"
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
    licenceStatus = "Existing Meshy outputs conservatively CC BY 4.0; collapsed-entry Boulder01 and MedievalWall02 CC0; RagTorch_Atlas is original assistant-authored geometry/maps with no blanket licence assigned; provenance in ASSET_LICENSES.md"
}
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $runtime "asset.manifest.json") -Encoding utf8
Write-Output "Built deterministic 1K production static-prop texture arrays and manifest."
