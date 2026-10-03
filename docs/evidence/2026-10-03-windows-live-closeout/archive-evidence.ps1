param(
    [string] $GameReports = 'C:\Dev\tmp\horde-final-windows-music-package-20261003\package-current-player-assets\reports',
    [string] $UiEvidence = 'C:\Dev\tmp\horde-windows-live-closeout-20261003'
)

# Archive a completed unique run, never rerun or overwrite its evidence.
$ErrorActionPreference = 'Stop'
$destination = Join-Path $PSScriptRoot 'raw'
if (Test-Path -LiteralPath $destination) { throw 'Refusing to overwrite frozen evidence.' }
$reportName = 'HordeLanternRT-benchmark-20261003-004125'
$jsonSource = Join-Path $GameReports "$reportName.json"
$report = Get-Content -LiteralPath $jsonSource -Raw | ConvertFrom-Json
$counts = $report.completedFrameEvidence.counts
$gpuCounts = $report.completedFrameEvidence.gpuStatusCounts
if ($report.result -ne 'complete' -or -not $report.routeTraversalComplete -or
    -not $report.workloadComplete -or -not $report.presentedEveryFrame -or
    $report.executionBackend -ne 'RayQueryCompute' -or $report.measuredFrames -ne 1838 -or
    $counts.expected -ne 1838 -or $counts.completed -ne 1838 -or $counts.cpuAccepted -ne 1838 -or
    $counts.rejected -ne 0 -or $counts.cancelled -ne 0 -or $counts.outstanding -ne 0 -or
    $gpuCounts.valid -ne 1838 -or $report.completedFrameEvidence.rows.Count -ne 1838) {
    throw 'Expected completed exact Compute report is absent.'
}
foreach ($row in $report.completedFrameEvidence.rows) {
    if ($row.presentationOutcome -ne 'presented' -or $row.gpuStatus -ne 'valid' -or
        $row.diagnosticStatus -ne 'compiled-out' -or $null -ne $row.diagnosticCounters -or
        $row.submittedIdentity.submissionSerial -ne $row.completionIdentity.completionSerial) {
        throw 'A completed row fails its presentation/Shipping/identity contract.'
    }
}
New-Item -ItemType Directory -Path $destination | Out-Null
$lines = [Collections.Generic.List[string]]::new()
$copies = @(
    @{ Source = Join-Path $GameReports "$reportName.txt"; Name = "$reportName.txt" },
    @{ Source = Join-Path $GameReports 'vulkan_capability_report.json'; Name = 'vulkan_capability_report.json' },
    @{ Source = Join-Path $UiEvidence 'restart-live.jpg'; Name = 'restart-live.jpg' },
    @{ Source = Join-Path $UiEvidence 'ui-observations.json'; Name = 'ui-observations.json' },
    @{ Source = Join-Path $UiEvidence 'updater-ctest.log'; Name = 'updater-ctest.log' }
)
foreach ($entry in $copies) {
    $target = Join-Path $destination $entry.Name
    [IO.File]::Copy($entry.Source, $target, $false)
    $hash = (Get-FileHash -LiteralPath $entry.Source -Algorithm SHA256).Hash.ToLowerInvariant()
    if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToLowerInvariant() -cne $hash) {
        throw "Copy identity failed: $($entry.Name)"
    }
    $lines.Add("$hash`t$((Get-Item -LiteralPath $target).Length)`traw/$($entry.Name)")
}
$compressed = Join-Path $destination "$reportName.json.gz"
$inputStream = [IO.File]::OpenRead($jsonSource)
$outputStream = [IO.File]::Open($compressed, [IO.FileMode]::CreateNew)
$gzip = [IO.Compression.GZipStream]::new($outputStream, [IO.Compression.CompressionLevel]::Optimal)
try { $inputStream.CopyTo($gzip) } finally { $gzip.Dispose(); $outputStream.Dispose(); $inputStream.Dispose() }
$lines.Add("$((Get-FileHash -LiteralPath $jsonSource -Algorithm SHA256).Hash.ToLowerInvariant())`t$((Get-Item -LiteralPath $jsonSource).Length)`tUNCOMPRESSED/$reportName.json")
$lines.Add("$((Get-FileHash -LiteralPath $compressed -Algorithm SHA256).Hash.ToLowerInvariant())`t$((Get-Item -LiteralPath $compressed).Length)`traw/$reportName.json.gz")
foreach ($name in @('README.md', 'archive-evidence.ps1')) {
    $path = Join-Path $PSScriptRoot $name
    $lines.Add("$((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant())`t$((Get-Item -LiteralPath $path).Length)`t$name")
}
[IO.File]::WriteAllLines((Join-Path $PSScriptRoot 'SHA256SUMS.txt'), $lines, [Text.UTF8Encoding]::new($false))
'Archived finite Windows UI,1838-row Compute report and updater CTest; no game run.'
