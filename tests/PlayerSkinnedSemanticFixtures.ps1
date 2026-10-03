[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$TestExecutable)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$source = Join-Path $repoRoot 'assets/models/player/runtime/gothic-traveller-lod0.runtime.glb'
$bytes = [IO.File]::ReadAllBytes($source)
$jsonLength = [BitConverter]::ToUInt32($bytes, 12)
$jsonText = [Text.Encoding]::UTF8.GetString($bytes, 20, $jsonLength)
$binaryChunk = $bytes[(20 + $jsonLength)..($bytes.Length - 1)]
$temporaryBase = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$temporaryRoot = Join-Path $temporaryBase ('horde-player-semantic-fixtures-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
try {
    foreach ($case in @('unchanged', 'reordered', 'missing', 'duplicate', 'unknown')) {
        $document = $jsonText | ConvertFrom-Json
        if ($document.meshes.Count -ne 1 -or $document.meshes[0].primitives.Count -ne 5) {
            throw 'Admitted WorldBody fixture must have one five-primitive mesh'
        }
        switch ($case) {
            'reordered' { $document.meshes[0].primitives = @($document.meshes[0].primitives[4..0]) }
            # Removing only the fifth region would form a valid legacy profile.
            # Remove a required core semantic instead, retaining the remainder.
            'missing' { $document.meshes[0].primitives = @($document.meshes[0].primitives | Where-Object {
                $document.materials[$_.material].name -cne 'HeadPrimaryMasked'
            }) }
            'duplicate' { $document.meshes[0].primitives[4].material = $document.meshes[0].primitives[0].material }
            'unknown' { $document.materials[$document.meshes[0].primitives[4].material].name = 'UnknownPlayerPart' }
        }
        $json = $document | ConvertTo-Json -Depth 80 -Compress
        $jsonBytes = [Text.Encoding]::UTF8.GetBytes($json)
        while ($jsonBytes.Length % 4) { $jsonBytes += [byte]32 }
        $output = Join-Path $temporaryRoot ($case + '.glb')
        $stream = [IO.File]::Create($output)
        $writer = [IO.BinaryWriter]::new($stream)
        try {
            $writer.Write([uint32]0x46546c67)
            $writer.Write([uint32]2)
            $writer.Write([uint32](20 + $jsonBytes.Length + $binaryChunk.Length))
            $writer.Write([uint32]$jsonBytes.Length)
            $writer.Write([uint32]0x4e4f534a)
            $writer.Write([byte[]]$jsonBytes)
            $writer.Write([byte[]]$binaryChunk)
        } finally { $writer.Dispose(); $stream.Dispose() }
        $expectation = if ($case -in @('unchanged', 'reordered')) { 'accept' } else { 'reject-semantic' }
        & $TestExecutable --validate-player-asset $output $expectation
        if ($LASTEXITCODE -ne 0) { throw "$case failed its $expectation check" }
        if ($expectation -eq 'accept') {
            & $TestExecutable --validate-player-admission $output (Join-Path $repoRoot 'assets/models/player/runtime/asset.manifest.json')
            if ($LASTEXITCODE -ne 0) { throw "$case failed static/skinned stream admission" }
        }
    }
    Write-Output 'Actual skinned player: unchanged/reordered accepted; missing/duplicate/unknown rejected.'
} finally {
    $resolved = [IO.Path]::GetFullPath($temporaryRoot)
    if ([IO.Directory]::GetParent($resolved).FullName -eq $temporaryBase.TrimEnd('\', '/') -and
        [IO.Path]::GetFileName($resolved).StartsWith('horde-player-semantic-fixtures-')) {
        Remove-Item -LiteralPath $resolved -Recurse -Force
    } else { throw 'Refused unsafe fixture-directory cleanup' }
}
