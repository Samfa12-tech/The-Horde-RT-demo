[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$fixtureRoot = $PSScriptRoot

function Add-Float([System.IO.BinaryWriter]$writer, [float]$value) {
    $writer.Write($value)
}

function New-FixtureBinary {
    $stream = [System.IO.MemoryStream]::new()
    $writer = [System.IO.BinaryWriter]::new($stream)
    try {
        foreach ($value in @(0,0,0, 1,0,0, 1,1,0, 0,1,0)) { Add-Float $writer ([float]$value) }
        foreach ($value in @(0,0,1, 0,0,1, 0,0,1, 0,0,1)) { Add-Float $writer ([float]$value) }
        foreach ($value in @(1,0,0,1, 1,0,0,1, 1,0,0,1, 1,0,0,1)) { Add-Float $writer ([float]$value) }
        foreach ($value in @(0,0, 1,0, 1,1, 0,1)) { Add-Float $writer ([float]$value) }
        foreach ($value in @([uint16]0,[uint16]1,[uint16]2,[uint16]0,[uint16]2,[uint16]3)) { $writer.Write($value) }
        foreach ($value in @([uint32]0,[uint32]2,[uint32]1,[uint32]0,[uint32]3,[uint32]2)) { $writer.Write($value) }
        $writer.Write([byte[]]@(137,80,78,71,13,10,26,10))
        return $stream.ToArray()
    }
    finally {
        $writer.Dispose()
        $stream.Dispose()
    }
}

function Write-Glb([string]$path, [string]$json, [byte[]]$binary) {
    $jsonBytes = [System.Collections.Generic.List[byte]]::new([Text.Encoding]::UTF8.GetBytes($json))
    while (($jsonBytes.Count % 4) -ne 0) { $jsonBytes.Add(0x20) }
    $binaryBytes = [System.Collections.Generic.List[byte]]::new($binary)
    while (($binaryBytes.Count % 4) -ne 0) { $binaryBytes.Add(0) }
    $length = 12 + 8 + $jsonBytes.Count + 8 + $binaryBytes.Count
    $stream = [System.IO.File]::Open($path, [System.IO.FileMode]::Create)
    $writer = [System.IO.BinaryWriter]::new($stream)
    try {
        $writer.Write([uint32]0x46546c67)
        $writer.Write([uint32]2)
        $writer.Write([uint32]$length)
        $writer.Write([uint32]$jsonBytes.Count)
        $writer.Write([uint32]0x4e4f534a)
        $writer.Write($jsonBytes.ToArray())
        $writer.Write([uint32]$binaryBytes.Count)
        $writer.Write([uint32]0x004e4942)
        $writer.Write($binaryBytes.ToArray())
    }
    finally {
        $writer.Dispose()
        $stream.Dispose()
    }
}

$json = @'
{"asset":{"version":"2.0"},"extensionsUsed":["KHR_materials_transmission","KHR_materials_ior","KHR_materials_volume","KHR_materials_emissive_strength"],"extensionsRequired":["KHR_materials_transmission","KHR_materials_ior","KHR_materials_volume","KHR_materials_emissive_strength"],"buffers":[{"byteLength":236}],"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":48},{"buffer":0,"byteOffset":48,"byteLength":48},{"buffer":0,"byteOffset":96,"byteLength":64},{"buffer":0,"byteOffset":160,"byteLength":32},{"buffer":0,"byteOffset":192,"byteLength":12},{"buffer":0,"byteOffset":204,"byteLength":24},{"buffer":0,"byteOffset":228,"byteLength":8}],"accessors":[{"bufferView":0,"componentType":5126,"count":4,"type":"VEC3","min":[0,0,0],"max":[1,1,0]},{"bufferView":1,"componentType":5126,"count":4,"type":"VEC3"},{"bufferView":2,"componentType":5126,"count":4,"type":"VEC4"},{"bufferView":3,"componentType":5126,"count":4,"type":"VEC2"},{"bufferView":4,"componentType":5123,"count":6,"type":"SCALAR"},{"bufferView":5,"componentType":5125,"count":6,"type":"SCALAR"}],"images":[{"name":"FixtureImage","bufferView":6,"mimeType":"image/png"}],"textures":[{"source":0}],"materials":[{"name":"FixtureMaterial","pbrMetallicRoughness":{"baseColorFactor":[0.2,0.3,0.4,0.8],"metallicFactor":0.7,"roughnessFactor":0.25,"baseColorTexture":{"index":0},"metallicRoughnessTexture":{"index":0}},"normalTexture":{"index":0},"occlusionTexture":{"index":0,"strength":0.6},"emissiveTexture":{"index":0},"emissiveFactor":[0.1,0.2,0.3],"extensions":{"KHR_materials_transmission":{"transmissionFactor":0.4,"transmissionTexture":{"index":0}},"KHR_materials_ior":{"ior":1.7},"KHR_materials_volume":{"thicknessFactor":0.5,"thicknessTexture":{"index":0},"attenuationDistance":3.0,"attenuationColor":[0.8,0.7,0.6]},"KHR_materials_emissive_strength":{"emissiveStrength":1.5}}}],"meshes":[{"name":"Mesh16","primitives":[{"attributes":{"POSITION":0,"NORMAL":1,"TANGENT":2,"TEXCOORD_0":3},"indices":4,"material":0,"mode":4}]},{"name":"Mesh32","primitives":[{"attributes":{"POSITION":0,"NORMAL":1,"TANGENT":2,"TEXCOORD_0":3},"indices":5,"material":0,"mode":4}]}],"nodes":[{"name":"Root","mesh":0,"translation":[1,2,3],"children":[1]},{"name":"grip","rotation":[0,0,0,1]},{"name":"MatrixNode","mesh":1,"matrix":[1,0,0,0,0,1,0,0,0,0,1,0,4,5,6,1]}],"scenes":[{"nodes":[0,2]}],"scene":0}
'@

$binary = New-FixtureBinary
$validPath = Join-Path $fixtureRoot "valid-multi.glb"
Write-Glb $validPath $json $binary

$badMagic = [System.IO.File]::ReadAllBytes($validPath)
$badMagic[0] = 0
[System.IO.File]::WriteAllBytes((Join-Path $fixtureRoot "bad-magic.glb"), $badMagic)

$badVersion = [System.IO.File]::ReadAllBytes($validPath)
$badVersion[4] = 1
[System.IO.File]::WriteAllBytes((Join-Path $fixtureRoot "bad-version.glb"), $badVersion)

$badChunk = [System.IO.File]::ReadAllBytes($validPath)
[System.IO.File]::WriteAllBytes((Join-Path $fixtureRoot "bad-chunk.glb"), $badChunk[0..($badChunk.Length - 5)])

Get-ChildItem -LiteralPath $fixtureRoot -Filter *.glb | Sort-Object Name | ForEach-Object {
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant()
    "{0} {1} bytes {2}" -f $_.Name, $_.Length, $hash
}
