[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$InputDirectory,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [string]$KtxPath=""
)
$ErrorActionPreference='Stop'
$inputRoot=[IO.Path]::GetFullPath($InputDirectory)
$outputRoot=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $outputRoot) { throw 'Use a fresh Kit texture output directory; existing candidates are preserved.' }
New-Item -ItemType Directory -Path $outputRoot | Out-Null
foreach($kind in @('base-color','normal','orm','emissive')) {
    $inputPath=Join-Path $inputRoot "$kind.png"
    if(-not (Test-Path -LiteralPath $inputPath -PathType Leaf)) { throw "Missing Kit input: $inputPath" }
    $srgb=$kind -in @('base-color','emissive')
    $transfer=if($srgb){'srgb'}else{'linear'}
    foreach($platform in @('windows','android')) {
        $format=if($platform -eq 'windows') {
            if($srgb){'R8G8B8A8_SRGB'}else{'R8G8B8A8_UNORM'}
        } elseif($kind -eq 'normal') { 'ASTC_4x4_UNORM_BLOCK' }
          elseif($srgb) { 'ASTC_6x6_SRGB_BLOCK' } else { 'ASTC_6x6_UNORM_BLOCK' }
        & (Join-Path $PSScriptRoot 'compile-static-texture-array.ps1') -InputPaths @($inputPath) `
            -OutputPath (Join-Path $outputRoot "$kind.$platform.ktx2") -Format $format `
            -Transfer $transfer -Width 1024 -Height 1024 -AssignPrimaries bt709 -KtxPath $KtxPath
        if($LASTEXITCODE -ne 0) { throw 'Kit texture compilation failed' }
    }
}
$receipt=@(Get-ChildItem -LiteralPath $outputRoot -File | Sort-Object Name | ForEach-Object {
    [ordered]@{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
})
$receipt | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $outputRoot 'texture-receipt.json') -Encoding utf8
