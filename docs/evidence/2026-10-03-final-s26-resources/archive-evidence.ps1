param(
    [string] $RawRoot = 'C:\Dev\tmp\horde-final-s26-resources-20261003',
    [string] $ArchiveRoot = (Join-Path $PSScriptRoot 'raw')
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression

$trialIds = @(
    'final26-a-route-20261003',
    'final26-b-high2-20261003',
    'final26-b-route-20261003',
    'final26-a-high-20261003',
    'final26-a-route2-20261003',
    'final26-b-live-20261003'
)
$memoryIds = @(
    'final26-b-high2-memory',
    'final26-b-route-memory',
    'final26-a-high-memory',
    'final26-a-route2-memory'
)

if (-not (Test-Path -LiteralPath $RawRoot -PathType Container)) {
    throw "Raw evidence root not found: $RawRoot"
}
if (Test-Path -LiteralPath $ArchiveRoot) {
    throw "Archive destination already exists; refusing to overwrite: $ArchiveRoot"
}
New-Item -ItemType Directory -Path $ArchiveRoot | Out-Null

$receipts = [System.Collections.Generic.List[object]]::new()
function Add-Receipt([string] $Source, [string] $Destination, [bool] $Compress) {
    if (-not (Test-Path -LiteralPath $Source -PathType Leaf)) {
        throw "Allowlisted source file missing: $Source"
    }
    $destinationDirectory = Split-Path -Parent $Destination
    New-Item -ItemType Directory -Path $destinationDirectory -Force | Out-Null
    if ($Compress) {
        $inputBytes = [System.IO.File]::ReadAllBytes($Source)
        $outputStream = [System.IO.File]::Open($Destination, [System.IO.FileMode]::CreateNew, [System.IO.FileAccess]::Write)
        try {
            $gzip = [System.IO.Compression.GZipStream]::new($outputStream, [System.IO.Compression.CompressionLevel]::Optimal, $true)
            try { $gzip.Write($inputBytes, 0, $inputBytes.Length) } finally { $gzip.Dispose() }
        } finally { $outputStream.Dispose() }
        $checkStream = [System.IO.File]::OpenRead($Destination)
        try {
            $decompressor = [System.IO.Compression.GZipStream]::new($checkStream, [System.IO.Compression.CompressionMode]::Decompress)
            try {
                $restored = [System.IO.MemoryStream]::new()
                try {
                    $decompressor.CopyTo($restored)
                    $restoredBytes = $restored.ToArray()
                    if (-not [System.Linq.Enumerable]::SequenceEqual[byte]($inputBytes, $restoredBytes)) {
                        throw "Compressed archive failed byte-for-byte round trip: $Destination"
                    }
                } finally { $restored.Dispose() }
            } finally { $decompressor.Dispose() }
        } finally { $checkStream.Dispose() }
    } else {
        [System.IO.File]::Copy($Source, $Destination, $false)
    }
    $sourceHash = (Get-FileHash -LiteralPath $Source -Algorithm SHA256).Hash.ToLowerInvariant()
    $archiveHash = (Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash.ToLowerInvariant()
    $receipts.Add([pscustomobject]@{
        sourcePath = [System.IO.Path]::GetRelativePath($RawRoot, $Source).Replace('\', '/')
        sourceBytes = (Get-Item -LiteralPath $Source).Length
        sourceSha256 = $sourceHash
        archivePath = [System.IO.Path]::GetRelativePath($PSScriptRoot, $Destination).Replace('\', '/')
        archiveBytes = (Get-Item -LiteralPath $Destination).Length
        archiveSha256 = $archiveHash
        gzip = $Compress
    })
}

foreach ($runId in $trialIds) {
    $runRoot = Join-Path $RawRoot $runId
    $reportRoot = Join-Path $runRoot $runId
    $resultPath = Join-Path $reportRoot 'result.json'
    $result = Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json
    if ($result.status -ne 'complete' -or $result.runId -ne $runId) {
        throw "Trial result is not complete or has wrong run identity: $runId"
    }
    foreach ($name in @('trial.json', 'context-samples.jsonl')) {
        $source = Join-Path $runRoot $name
        Add-Receipt $source (Join-Path $ArchiveRoot "$runId/$name") $false
    }
    foreach ($name in @('result.json', 'benchmark.json')) {
        $source = Join-Path $reportRoot $name
        if ($name -eq 'benchmark.json') {
            $benchmark = Get-Content -LiteralPath $source -Raw | ConvertFrom-Json
            if ($benchmark.status -ne 'complete' -or $benchmark.runId -ne $runId) {
                throw "Benchmark report is not complete or has wrong run identity: $runId"
            }
            $evidence = $benchmark.completedFrameEvidence
            if (-not $benchmark.presentedEveryFrame -or
                $evidence.status -ne 'complete' -or
                $evidence.counts.expected -ne $evidence.counts.completed -or
                $evidence.counts.completed -ne $evidence.counts.cpuAccepted -or
                $evidence.counts.rejected -ne 0 -or
                $evidence.counts.cancelled -ne 0 -or
                $evidence.counts.outstanding -ne 0 -or
                $evidence.gpuStatusCounts.denominator -ne $evidence.gpuStatusCounts.valid -or
                $evidence.gpuStatusCounts.valid -ne $evidence.counts.completed) {
                throw "Frame admission/presentation/GPU validity did not reconcile: $runId"
            }
            Add-Receipt $source (Join-Path $ArchiveRoot "$runId/benchmark.json.gz") $true
        } else {
            Add-Receipt $source (Join-Path $ArchiveRoot "$runId/result.json") $false
        }
    }
}

foreach ($sidecarId in $memoryIds) {
    $sidecarRoot = Join-Path $RawRoot $sidecarId
    $receipt = Get-Content -LiteralPath (Join-Path $sidecarRoot 'receipt.json') -Raw | ConvertFrom-Json
    if ($receipt.status -ne 'complete' -or $receipt.capturedSamples -ne 3 -or $receipt.missingSampleIndices.Count -ne 0) {
        throw "RAM sidecar is incomplete: $sidecarId"
    }
    foreach ($name in @('receipt.json', 'samples.jsonl')) {
        Add-Receipt (Join-Path $sidecarRoot $name) (Join-Path $ArchiveRoot "$sidecarId/$name") $false
    }
    foreach ($index in 1..3) {
        $sample = '{0:D3}' -f $index
        foreach ($kind in @('meminfo', 'procStatus', 'pressure', 'globalMeminfo')) {
            $name = "sample-$sample-$kind.txt"
            $source = Join-Path (Join-Path $sidecarRoot 'raw') $name
            Add-Receipt $source (Join-Path $ArchiveRoot "$sidecarId/raw/$name") $false
        }
    }
}

$summaryPath = Join-Path $PSScriptRoot 'README.md'
if (-not (Test-Path -LiteralPath $summaryPath -PathType Leaf)) {
    throw "Summary not found for receipt binding: $summaryPath"
}
$summaryHash = (Get-FileHash -LiteralPath $summaryPath -Algorithm SHA256).Hash.ToLowerInvariant()
$scriptHash = (Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256).Hash.ToLowerInvariant()
$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add('# SHA-256 archive receipt')
$lines.Add('')
$lines.Add('Each SOURCE line binds an exact allowlisted input to its archived output. benchmark.json inputs are gzip-compressed and byte-for-byte round-trip checked.')
$lines.Add('')
$lines.Add("SUMMARY`t$summaryHash`t$((Get-Item -LiteralPath $summaryPath).Length)`tREADME.md")
$lines.Add("ARCHIVER`t$scriptHash`t$((Get-Item -LiteralPath $PSCommandPath).Length)`tarchive-evidence.ps1")
foreach ($receipt in $receipts) {
    $lines.Add("SOURCE`t$($receipt.sourceSha256)`t$($receipt.sourceBytes)`t$($receipt.sourcePath)")
    $lines.Add("ARCHIVE`t$($receipt.archiveSha256)`t$($receipt.archiveBytes)`t$($receipt.archivePath)")
}
[System.IO.File]::WriteAllLines((Join-Path $PSScriptRoot 'SHA256SUMS.txt'), $lines, [System.Text.UTF8Encoding]::new($false))

Write-Output "Archived $($receipts.Count) allowlisted files; bound summary and script hashes in SHA256SUMS.txt"
