# Complete software notices are separate from the asset provenance summary.
function Copy-HordeThirdPartyNotices {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)][string]$PackageRoot
    )
    $source = Join-Path $RepositoryRoot 'third_party/cgltf/LICENSE'
    $destinationRoot = Join-Path $PackageRoot 'THIRD_PARTY_NOTICES'
    New-Item -ItemType Directory -Path $destinationRoot -Force | Out-Null
    $destination = Join-Path $destinationRoot 'cgltf-LICENSE.txt'
    Copy-Item -LiteralPath $source -Destination $destination
    if ((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -cne
        (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash) {
        throw 'Staged cgltf notice bytes disagree with the complete vendored licence.'
    }
}

function Assert-HordeThirdPartyNoticesPackage {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)][string]$ArchivePath,
        [Parameter(Mandatory = $true)][ValidateSet('Windows', 'Android')][string]$Platform
    )
    $source = Join-Path $RepositoryRoot 'third_party/cgltf/LICENSE'
    $expectedHash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
    $expectedLength = (Get-Item -LiteralPath $source).Length
    $entryName = 'THIRD_PARTY_NOTICES/cgltf-LICENSE.txt'
    if ($Platform -eq 'Android') { $entryName = 'assets/' + $entryName }
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [IO.Compression.ZipFile]::OpenRead([IO.Path]::GetFullPath($ArchivePath))
    try {
        $entries = @($archive.Entries | Where-Object { $_.FullName -ceq $entryName })
        if ($entries.Count -ne 1) {
            throw "$Platform package must contain exactly one complete cgltf notice at $entryName; found $($entries.Count)."
        }
        $stream = $entries[0].Open()
        $sha = [Security.Cryptography.SHA256]::Create()
        try { $actualHash = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '') }
        finally { $sha.Dispose(); $stream.Dispose() }
        if ($entries[0].Length -ne $expectedLength -or $actualHash -cne $expectedHash) {
            throw "$Platform package cgltf notice does not match the complete vendored licence bytes."
        }
    } finally { $archive.Dispose() }
}
