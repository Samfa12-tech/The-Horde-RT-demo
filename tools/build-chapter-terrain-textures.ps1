[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$output=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $output){throw 'Choose a new versioned output directory; existing sources are never overwritten.'}
$families=@('damp_sand','forrest_ground_01')
function SourcePath([string]$family,[string]$file){
    if($family -eq 'forrest_ground_01'){return ('assets/textures/polyhaven/source/forrest_ground_01-1k-v1/'+$file.Replace('.jpg','.png'))}
    return "assets/textures/polyhaven/mobile_1k/$family/$file"
}
$sources=@();$products=@()
foreach($family in $families){
    foreach($file in @('diff.jpg','normal.jpg','arm.jpg')){
        $relative=SourcePath $family $file
        $source=Join-Path $root $relative
        if(!(Test-Path -LiteralPath $source)){throw "Missing admitted source: $relative"}
        $sources+=@{path=$relative;bytes=(Get-Item -LiteralPath $source).Length;sha256=(Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant()}
    }
}
New-Item -ItemType Directory -Path $output | Out-Null
foreach($map in @('base-color','normal','orm')){
    $file=switch($map){'base-color'{'diff.jpg'} 'normal'{'normal.jpg'} 'orm'{'arm.jpg'}}
    $inputs=@($families|ForEach-Object {Join-Path $root (SourcePath $_ $file)})
    foreach($platform in @('windows','android')){
        $format=if($platform -eq 'windows'){
            if($map -eq 'base-color'){'R8G8B8A8_SRGB'}else{'R8G8B8A8_UNORM'}
        }else{switch($map){'base-color'{'ASTC_6x6_SRGB_BLOCK'} 'normal'{'ASTC_4x4_UNORM_BLOCK'} 'orm'{'ASTC_6x6_UNORM_BLOCK'}}}
        $transfer=if($map -eq 'base-color'){'srgb'}else{'linear'}
        $name="$map.$platform.ktx2";$destination=Join-Path $output $name
        & (Join-Path $PSScriptRoot 'compile-static-texture-array.ps1') -InputPaths $inputs -OutputPath $destination `
            -Format $format -Transfer $transfer -Width 1024 -Height 1024 -AssignPrimaries bt709
        if($LASTEXITCODE){throw "Terrain encoding failed: $name"}
        $products+=@{file=$name;format=$format;transfer=$transfer;bytes=(Get-Item -LiteralPath $destination).Length;sha256=(Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash.ToLowerInvariant()}
    }
}
$receipt=[ordered]@{schema=1;asset='chapter-terrain-two-layer-addition';distribution='source-candidate';
    resolution=1024;mipLevels=11;layers=2;layerOrder=$families;primaries='bt709';
    sourceNotice='Poly Haven CC0 damp_sand and forrest_ground_01; original sources retained';
    recipe='tools/build-chapter-terrain-textures.ps1';sources=$sources;products=$products;
    scope='Exact two-layer addition; no existing prop array is re-encoded. Native admission is separate from physical device acceptance.'}
$receipt|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $output 'receipt.json') -Encoding utf8
