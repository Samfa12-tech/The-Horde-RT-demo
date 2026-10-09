# Preserve the original-software grant WITH its scope; this is not a licence for
# assets, Pocket Audio Core, other dependencies or a mixed package as a whole.
# Complete software notices are separate from the ASSET_LICENSES.md summary.
function Get-HordeSoftwarePackageNotices {
    @(
        @{ Source = 'LICENSE'; Destination = 'LICENSE' },
        @{ Source = 'LICENSE_SCOPE.md'; Destination = 'LICENSE_SCOPE.md' },
        @{ Source = 'third_party/cgltf/LICENSE'; Destination = 'THIRD_PARTY_NOTICES/cgltf-LICENSE.txt' }
    )
}

function Copy-HordeThirdPartyNotices {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)][string]$PackageRoot
    )
    foreach ($notice in Get-HordeSoftwarePackageNotices) {
        $source = Join-Path $RepositoryRoot $notice.Source
        $destination = Join-Path $PackageRoot $notice.Destination
        New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
        Copy-Item -LiteralPath $source -Destination $destination
        if ((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -cne
            (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash) {
            throw "Staged software notice bytes disagree with $($notice.Source)."
        }
    }
}

function Assert-HordeThirdPartyNoticesPackage {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)][string]$ArchivePath,
        [Parameter(Mandatory = $true)][ValidateSet('Windows', 'Android')][string]$Platform
    )
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [IO.Compression.ZipFile]::OpenRead([IO.Path]::GetFullPath($ArchivePath))
    try {
        foreach ($notice in Get-HordeSoftwarePackageNotices) {
            $source = Join-Path $RepositoryRoot $notice.Source
            $expectedHash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
            $expectedLength = (Get-Item -LiteralPath $source).Length
            $entryName = $notice.Destination
            if ($Platform -eq 'Android') { $entryName = 'assets/' + $entryName }
            $entries = @($archive.Entries | Where-Object { $_.FullName -ceq $entryName })
            if ($entries.Count -ne 1) {
                throw "$Platform package must contain exactly one complete software notice at $entryName; found $($entries.Count)."
            }
            $stream = $entries[0].Open()
            $sha = [Security.Cryptography.SHA256]::Create()
            try { $actualHash = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '') }
            finally { $sha.Dispose(); $stream.Dispose() }
            if ($entries[0].Length -ne $expectedLength -or $actualHash -cne $expectedHash) {
                throw "$Platform package $entryName does not match the complete $($notice.Source) bytes."
            }
        }
    } finally { $archive.Dispose() }
}
