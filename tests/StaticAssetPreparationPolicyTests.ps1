[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][ValidateSet("LodBudget", "UnsafeSkip", "CollidingLod")][string]$Mode
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) ("horde-static-preparation-policy-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
try {
    $sourceManifest = Join-Path $repoRoot "assets\models\weapons\meshy\runtime-development\asset.manifest.json"
    if ($Mode -eq "UnsafeSkip") {
        $outputPath = Join-Path $temporaryRoot "unsafe-skip"
        $rejected = $false
        try {
            & (Join-Path $repoRoot "tools\prepare-static-rt-asset.ps1") `
                -SourceGlb "assets\models\weapons\meshy\gothic_arming_sword_rh_lod1.glb" `
                -Manifest ([IO.Path]::GetRelativePath($repoRoot, $sourceManifest)) `
                -OutputDirectory ([IO.Path]::GetRelativePath($repoRoot, $outputPath)) `
                -ReviewedValidatorIssueCodes MESH_PRIMITIVE_GENERATED_TANGENT_SPACE `
                -SkipTextureCompilation
        }
        catch {
            $expected = "A parameter cannot be found that matches parameter name 'SkipTextureCompilation'."
            if ($_.Exception.Message -ne $expected) { throw "Expected '$expected', got '$($_.Exception.Message)'" }
            $rejected = $true
        }
        if (-not $rejected) { throw "Preparation accepted the unsafe SkipTextureCompilation bypass." }
        if (Test-Path -LiteralPath $outputPath) { throw "Unsafe skip rejection created an output directory." }
        Write-Output "Static asset preparation unsafe-skip policy passed"
        return
    }
    if ($Mode -eq "CollidingLod") {
        $sourcePath = Join-Path $temporaryRoot "gothic_arming_sword_rh_lod10.glb"
        Copy-Item -LiteralPath (Join-Path $repoRoot "assets\models\weapons\meshy\gothic_arming_sword_rh_lod1.glb") `
            -Destination $sourcePath
        $manifest = Get-Content -LiteralPath $sourceManifest -Raw | ConvertFrom-Json
        $manifest.lods = @(
            [ordered]@{ name = "lod1"; maxTriangles = 12357 },
            [ordered]@{ name = "lod10"; maxTriangles = 13000 }
        )
        $manifestPath = Join-Path $temporaryRoot "colliding-lods.manifest.json"
        $manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $manifestPath -Encoding utf8
        $outputPath = Join-Path $temporaryRoot "prepared-lod10"
        & (Join-Path $repoRoot "tools\prepare-static-rt-asset.ps1") `
            -SourceGlb ([IO.Path]::GetRelativePath($repoRoot, $sourcePath)) `
            -Manifest ([IO.Path]::GetRelativePath($repoRoot, $manifestPath)) `
            -OutputDirectory ([IO.Path]::GetRelativePath($repoRoot, $outputPath)) `
            -BaseColorImage "assets\models\weapons\meshy\gothic_arming_sword_rh_v01_textures\base_color.png" `
            -NormalImage "assets\models\weapons\meshy\gothic_arming_sword_rh_v01_textures\normal.png" `
            -RoughnessImage "assets\models\weapons\meshy\gothic_arming_sword_rh_v01_textures\roughness.png" `
            -MetallicImage "assets\models\weapons\meshy\gothic_arming_sword_rh_v01_textures\metallic.png" `
            -EmissiveImage "assets\models\weapons\meshy\gothic_arming_sword_rh_v01_textures\emission.png" `
            -ReviewedValidatorIssueCodes MESH_PRIMITIVE_GENERATED_TANGENT_SPACE
        $budget = Get-Content -LiteralPath (Join-Path $outputPath "runtime-budget.json") -Raw | ConvertFrom-Json
        if ([int64]$budget.runtime.triangles -ne 12358) {
            throw "Colliding LOD preparation did not publish the hand-checked 12,358-triangle lod10 proof."
        }
        Write-Output "Static asset preparation exact LOD suffix contract passed: lod10"
        return
    }
    $manifest = Get-Content -LiteralPath $sourceManifest -Raw | ConvertFrom-Json
    $manifest.lods[0].maxTriangles = 12357
    $manifestPath = Join-Path $temporaryRoot "lod-budget.manifest.json"
    $manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $manifestPath -Encoding utf8
    $outputPath = Join-Path $temporaryRoot "prepared"
    $rejected = $false
    try {
        & (Join-Path $repoRoot "tools\prepare-static-rt-asset.ps1") `
            -SourceGlb "assets\models\weapons\meshy\gothic_arming_sword_rh_lod1.glb" `
            -Manifest ([IO.Path]::GetRelativePath($repoRoot, $manifestPath)) `
            -OutputDirectory ([IO.Path]::GetRelativePath($repoRoot, $outputPath)) `
            -BaseColorImage "assets\models\weapons\meshy\gothic_arming_sword_rh_v01_textures\base_color.png" `
            -NormalImage "assets\models\weapons\meshy\gothic_arming_sword_rh_v01_textures\normal.png" `
            -RoughnessImage "assets\models\weapons\meshy\gothic_arming_sword_rh_v01_textures\roughness.png" `
            -MetallicImage "assets\models\weapons\meshy\gothic_arming_sword_rh_v01_textures\metallic.png" `
            -EmissiveImage "assets\models\weapons\meshy\gothic_arming_sword_rh_v01_textures\emission.png" `
            -ReviewedValidatorIssueCodes MESH_PRIMITIVE_GENERATED_TANGENT_SPACE
    }
    catch {
        $expected = "Runtime GLB exceeds selected LOD 'lod1' maxTriangles capacity."
        if ($_.Exception.Message -ne $expected) {
            throw "Expected '$expected', got '$($_.Exception.Message)'"
        }
        $rejected = $true
    }
    if (-not $rejected) { throw "Preparation accepted a runtime GLB above selected LOD 'lod1' maxTriangles." }
    if (Test-Path -LiteralPath (Join-Path $outputPath "runtime-budget.json")) {
        throw "Preparation published a success budget after LOD rejection."
    }
    Write-Output "Static asset preparation LOD policy passed"
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}
