[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$evidenceRoot = [IO.Path]::GetFullPath($PSScriptRoot)
$artifactRoot = 'C:\Dev\tmp\horde-final-windows-music-package-20261003'
$zipPath = Join-Path $artifactRoot 'Horde-Lantern-RT-Alpha-1.6.1-Windows-x64-SHIPPING-HIGH-CURRENT-PLAYER-ASSETS-UNPUBLISHABLE.zip'
$outputs = @(
    (Join-Path $evidenceRoot 'pipeline-containment.json'),
    (Join-Path $evidenceRoot 'windows-release-build.log'),
    (Join-Path $evidenceRoot 'final-zip-entry-roster.json'),
    (Join-Path $evidenceRoot 'ARCHIVE-RECEIPT.json')
)
foreach ($output in $outputs) {
    if (Test-Path -LiteralPath $output) { throw "Refusing to overwrite existing evidence: $output" }
}
foreach ($input in @(
    (Join-Path $artifactRoot 'pipeline-containment.json'),
    (Join-Path $artifactRoot 'windows-release-build.log'),
    $zipPath
)) {
    if (-not (Test-Path -LiteralPath $input -PathType Leaf)) { throw "Required existing artifact is missing: $input" }
}

Copy-Item -LiteralPath (Join-Path $artifactRoot 'pipeline-containment.json') -Destination (Join-Path $evidenceRoot 'pipeline-containment.json')
Copy-Item -LiteralPath (Join-Path $artifactRoot 'windows-release-build.log') -Destination (Join-Path $evidenceRoot 'windows-release-build.log')

Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    $entries = @(
        foreach ($entry in ($archive.Entries | Sort-Object -Property FullName -CaseSensitive)) {
            $stream = $entry.Open()
            try {
                $sha = [Security.Cryptography.SHA256]::Create()
                try { $digest = [Convert]::ToHexString($sha.ComputeHash($stream)).ToLowerInvariant() }
                finally { $sha.Dispose() }
            } finally { $stream.Dispose() }
            [pscustomobject]@{
                path = $entry.FullName
                bytes = $entry.Length
                sha256 = $digest
            }
        }
    )
} finally { $archive.Dispose() }
if ($entries.Count -ne 78) { throw "Expected 78 final ZIP entries; found $($entries.Count)." }

$zipRosterPath = Join-Path $evidenceRoot 'final-zip-entry-roster.json'
$utf8NoBom = [Text.UTF8Encoding]::new($false)
$roster = [ordered]@{
    archive = [IO.Path]::GetFileName($zipPath)
    archiveSha256 = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
    archiveBytes = (Get-Item -LiteralPath $zipPath).Length
    entryCount = $entries.Count
    entries = $entries
}
[IO.File]::WriteAllText($zipRosterPath, ($roster | ConvertTo-Json -Depth 5) + "`n", $utf8NoBom)

$boundPaths = @(
    'README.md',
    'SHA256SUMS.txt',
    'archive-final-package-evidence.ps1',
    'pipeline-containment.json',
    'windows-release-build.log',
    'final-zip-entry-roster.json'
)
$boundFiles = @(
    foreach ($relativePath in $boundPaths) {
        $path = Join-Path $evidenceRoot $relativePath
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Cannot bind missing archive evidence: $relativePath" }
        [pscustomobject]@{
            path = $relativePath
            bytes = (Get-Item -LiteralPath $path).Length
            sha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    }
)
$receipt = [ordered]@{
    purpose = 'Bind archived existing Windows package-admission inputs and documentation; not a build or game run.'
    sourceArtifactRoot = $artifactRoot
    finalZip = [ordered]@{
        path = [IO.Path]::GetFileName($zipPath)
        bytes = (Get-Item -LiteralPath $zipPath).Length
        sha256 = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    boundFiles = $boundFiles
}
$receiptPath = Join-Path $evidenceRoot 'ARCHIVE-RECEIPT.json'
[IO.File]::WriteAllText($receiptPath, ($receipt | ConvertTo-Json -Depth 5) + "`n", $utf8NoBom)
Get-FileHash -LiteralPath $receiptPath -Algorithm SHA256 | Select-Object Path, Hash
