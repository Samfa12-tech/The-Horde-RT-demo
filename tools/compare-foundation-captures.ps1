param(
    [Parameter(Mandatory = $true)][string]$BaselineDirectory,
    [Parameter(Mandatory = $true)][string]$CandidateDirectory,
    [string]$OutputPath = (Join-Path $CandidateDirectory "capture-comparison.json")
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
$expected = @("opening", "skeleton", "worst-bend", "lantern-drop", "skylight", "yellow", "blue", "red", "green", "mirror", "lich", "finale-roof")

function Read-Manifest([string]$directory) {
    $path = Join-Path ([IO.Path]::GetFullPath($directory)) "capture-manifest.json"
    if (-not (Test-Path -LiteralPath $path)) { throw "Capture manifest not found: $path" }
    return Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
}

$baseline = Read-Manifest $BaselineDirectory
$candidate = Read-Manifest $CandidateDirectory
if (-not $baseline.complete -or -not $candidate.complete) { throw "Both capture manifests must be complete." }
if ($baseline.captures.Count -ne 12 -or $candidate.captures.Count -ne 12) { throw "Both capture sets must contain 12 records." }

$comparisons = [Collections.Generic.List[object]]::new()
$failed = $false
for ($index = 0; $index -lt $expected.Count; ++$index) {
    $before = $baseline.captures[$index]
    $after = $candidate.captures[$index]
    if ($before.checkpoint -ne $expected[$index] -or $after.checkpoint -ne $expected[$index]) {
        throw "Checkpoint order mismatch at index $index."
    }
    $beforePath = Join-Path ([IO.Path]::GetFullPath($BaselineDirectory)) $before.file
    $afterPath = Join-Path ([IO.Path]::GetFullPath($CandidateDirectory)) $after.file
    $beforeBitmap = [Drawing.Bitmap]::new($beforePath)
    $afterBitmap = [Drawing.Bitmap]::new($afterPath)
    try {
        if ($beforeBitmap.Width -ne $afterBitmap.Width -or $beforeBitmap.Height -ne $afterBitmap.Height) {
            throw "Capture dimensions differ for $($expected[$index])."
        }
        $differentOverOne = 0L
        $maximumDifference = 0
        $pixels = [long]$beforeBitmap.Width * [long]$beforeBitmap.Height
        for ($y = 0; $y -lt $beforeBitmap.Height; ++$y) {
            for ($x = 0; $x -lt $beforeBitmap.Width; ++$x) {
                $left = $beforeBitmap.GetPixel($x, $y)
                $right = $afterBitmap.GetPixel($x, $y)
                $difference = [Math]::Max([Math]::Abs([int]$left.R - [int]$right.R),
                    [Math]::Max([Math]::Abs([int]$left.G - [int]$right.G), [Math]::Abs([int]$left.B - [int]$right.B)))
                if ($difference -gt 1) { ++$differentOverOne }
                if ($difference -gt $maximumDifference) { $maximumDifference = $difference }
            }
        }
        $fraction = $(if ($pixels -gt 0) { $differentOverOne / $pixels } else { 1.0 })
        $passed = $maximumDifference -le 3 -and $fraction -le 0.001
        if (-not $passed) { $failed = $true }
        $comparisons.Add([PSCustomObject]@{
            checkpoint = $expected[$index]
            pixels = $pixels
            pixelsDifferentByMoreThanOne = $differentOverOne
            differentFraction = [Math]::Round($fraction, 8)
            maximumChannelDifference = $maximumDifference
            passed = $passed
        })
    } finally {
        $beforeBitmap.Dispose()
        $afterBitmap.Dispose()
    }
}

$baselineMedian = [double]$baseline.timing.overallMedianMs
$candidateMedian = [double]$candidate.timing.overallMedianMs
$timingPassed = $candidateMedian -le $baselineMedian * 1.02
if (-not $timingPassed) { $failed = $true }
$result = [ordered]@{
    schema = 1
    passed = -not $failed
    pixelTolerance = [ordered]@{ maximumChannelDifference = 3; maximumFractionOverOne = 0.001 }
    timingTolerancePercent = 2.0
    baselineOverallMedianMs = $baselineMedian
    candidateOverallMedianMs = $candidateMedian
    timingDeltaPercent = [Math]::Round((($candidateMedian / $baselineMedian) - 1.0) * 100.0, 3)
    timingPassed = $timingPassed
    captures = @($comparisons)
}
$outputFull = [IO.Path]::GetFullPath($OutputPath)
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $outputFull) | Out-Null
$result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $outputFull -Encoding utf8
if ($failed) { throw "Foundation capture/timing comparison failed. See $outputFull" }
Write-Host "Foundation capture/timing comparison passed: $outputFull"
