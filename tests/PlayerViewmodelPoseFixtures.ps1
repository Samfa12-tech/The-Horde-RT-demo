[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$TestExecutable)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$source = Join-Path $repoRoot 'assets/models/player/viewmodel/runtime/gothic-traveller-viewmodel.runtime.glb'
$manifest = Join-Path $repoRoot 'assets/models/player/viewmodel/runtime/asset.manifest.json'
$world = Join-Path $repoRoot 'assets/models/player/runtime/gothic-traveller-lod0.runtime.glb'
foreach ($path in @($source, $manifest, $world, $TestExecutable)) {
    if (-not (Test-Path -LiteralPath $path)) { throw "Required admission input is missing: $path" }
}

$sourceBytes = [IO.File]::ReadAllBytes($source)
if ($sourceBytes.Length -lt 28 -or [BitConverter]::ToUInt32($sourceBytes, 0) -ne 0x46546c67) {
    throw 'Viewmodel candidate is not a GLB.'
}
$jsonLength = [BitConverter]::ToUInt32($sourceBytes, 12)
$jsonText = [Text.Encoding]::UTF8.GetString($sourceBytes, 20, $jsonLength)
$binaryStart = 20 + [int]$jsonLength
$binaryChunk = New-Object byte[] ($sourceBytes.Length - $binaryStart)
[Array]::Copy($sourceBytes, $binaryStart, $binaryChunk, 0, $binaryChunk.Length)

function Get-AccessorOffset([object]$Document, [int]$AccessorIndex) {
    $accessor = $Document.accessors[$AccessorIndex]
    $view = $Document.bufferViews[[int]$accessor.bufferView]
    return 8 + [int]$view.byteOffset + [int]$accessor.byteOffset
}

function Set-Float([byte[]]$Binary, [int]$Offset, [single]$Value) {
    $encoded = [BitConverter]::GetBytes($Value)
    [Array]::Copy($encoded, 0, $Binary, $Offset, 4)
}

function Add-Children([object]$Node, [int[]]$Children) {
    $Node | Add-Member -MemberType NoteProperty -Name children -Value $Children -Force
}

function Write-Variant([string]$Path, [scriptblock]$Mutator) {
    $document = $jsonText | ConvertFrom-Json
    $binary = New-Object byte[] $binaryChunk.Length
    [Array]::Copy($binaryChunk, 0, $binary, 0, $binary.Length)
    & $Mutator $document $binary
    $encodedJson = [Text.Encoding]::UTF8.GetBytes(($document | ConvertTo-Json -Depth 100 -Compress))
    $jsonList = [Collections.Generic.List[byte]]::new()
    $jsonList.AddRange($encodedJson)
    while (($jsonList.Count % 4) -ne 0) { $jsonList.Add([byte]32) }
    $stream = [IO.File]::Create($Path)
    $writer = [IO.BinaryWriter]::new($stream)
    try {
        $writer.Write([uint32]0x46546c67)
        $writer.Write([uint32]2)
        $writer.Write([uint32](20 + $jsonList.Count + $binary.Length))
        $writer.Write([uint32]$jsonList.Count)
        $writer.Write([uint32]0x4e4f534a)
        $writer.Write([byte[]]$jsonList.ToArray())
        $writer.Write($binary)
    } finally {
        $writer.Dispose()
        $stream.Dispose()
    }
}

function Invoke-Admission([string]$Name, [string]$Path, [bool]$ExpectedPass, [string]$DiagnosticPattern) {
    $outputLines = & $TestExecutable --validate-viewmodel-admission $Path $manifest $world 2>&1
    $exitCode = $LASTEXITCODE
    $output = ($outputLines | Out-String).Trim()
    if ($ExpectedPass) {
        if ($exitCode -ne 0) { throw "$Name unexpectedly failed: $output" }
        Write-Output ("{0}: accepted" -f $Name)
        return
    }
    if ($exitCode -eq 0) { throw "$Name unexpectedly passed admission" }
    if ([string]::IsNullOrWhiteSpace($output) -or $output -notmatch $DiagnosticPattern) {
        throw "$Name failed without a meaningful diagnostic matching '$DiagnosticPattern': $output"
    }
    Write-Output ("{0}: rejected ({1})" -f $Name, $output)
}

$temporaryBase = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$temporaryRoot = Join-Path $temporaryBase ('horde-viewmodel-pose-fixtures-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
try {
    $unchanged = Join-Path $temporaryRoot 'unchanged.glb'
    Copy-Item -LiteralPath $source -Destination $unchanged
    Invoke-Admission 'unchanged candidate' $unchanged $true ''

    $inverseBind = Join-Path $temporaryRoot 'wrong-inverse-bind.glb'
    Write-Variant $inverseBind {
        param($document, $binary)
        $accessor = [int]$document.skins[0].inverseBindMatrices
        Set-Float $binary ((Get-AccessorOffset $document $accessor) + (12 * 4)) ([single]0.125)
    }
    Invoke-Admission 'finite inverse-bind change' $inverseBind $false '(?i)inverse.?bind|rig|pose'

    $renamedJoint = Join-Path $temporaryRoot 'renamed-nonrequired-joint.glb'
    Write-Variant $renamedJoint {
        param($document, $binary)
        $jointIndex = -1
        for ($i = 0; $i -lt $document.nodes.Count; ++$i) {
            if ($document.nodes[$i].name -in @('Hips', 'Pelvis')) { $jointIndex = $i; break }
        }
        if ($jointIndex -lt 0) { throw 'Candidate has no Hips or Pelvis joint to rename.' }
        $document.nodes[$jointIndex].name = 'ViewmodelOnlyJoint'
    }
    Invoke-Admission 'renamed nonrequired joint' $renamedJoint $false '(?i)joint|socket|rig|required|pose'

    $parentCycle = Join-Path $temporaryRoot 'node-parent-cycle.glb'
    Write-Variant $parentCycle {
        param($document, $binary)
        $rootIndex = [int]$document.scenes[0].nodes[0]
        # A scene root has no existing parent. Adding itself as a child creates
        # a genuine cycle without first triggering the multiple-parent gate.
        $children = @()
        if ($null -ne $document.nodes[$rootIndex].children) {
            $children = @($document.nodes[$rootIndex].children)
        }
        Add-Children $document.nodes[$rootIndex] @($children + $rootIndex)
    }
    Invoke-Admission 'node-parent cycle' $parentCycle $false 'Skinned GLB hierarchy contains a cycle'

    $invalidChannel = Join-Path $temporaryRoot 'invalid-animation-channel-node.glb'
    Write-Variant $invalidChannel {
        param($document, $binary)
        $document.animations[0].channels[0].target.node = 9999
    }
    Invoke-Admission 'invalid animation channel node' $invalidChannel $false 'Skinned animation channel has an invalid target node'

    $nonfinitePosition = Join-Path $temporaryRoot 'nonfinite-position.glb'
    Write-Variant $nonfinitePosition {
        param($document, $binary)
        $accessor = [int]$document.meshes[0].primitives[0].attributes.POSITION
        Set-Float $binary (Get-AccessorOffset $document $accessor) ([single]::NaN)
    }
    Invoke-Admission 'nonfinite position' $nonfinitePosition $false 'Skinned GLB vertex attribute is not finite'

    $nonfiniteWeight = Join-Path $temporaryRoot 'nonfinite-weight.glb'
    Write-Variant $nonfiniteWeight {
        param($document, $binary)
        $accessor = [int]$document.meshes[0].primitives[0].attributes.WEIGHTS_0
        Set-Float $binary (Get-AccessorOffset $document $accessor) ([single]::NaN)
    }
    Invoke-Admission 'nonfinite weight' $nonfiniteWeight $false 'Skinned GLB vertex attribute is not finite'
} finally {
    $resolved = [IO.Path]::GetFullPath($temporaryRoot)
    if ([IO.Directory]::GetParent($resolved).FullName -eq $temporaryBase.TrimEnd('\', '/') -and
        [IO.Path]::GetFileName($resolved).StartsWith('horde-viewmodel-pose-fixtures-')) {
        Remove-Item -LiteralPath $resolved -Recurse -Force
    } else {
        throw 'Refused unsafe fixture-directory cleanup'
    }
}
