param(
    [Parameter(Mandatory = $true)][string]$FirstRunDirectory,
    [Parameter(Mandatory = $true)][string]$SecondRunDirectory,
    [ValidateRange(0.1, 10.0)][double]$MaximumStartingTemperatureDifferenceC = 2.0,
    [ValidateRange(0.0, 100.0)][double]$InvestigationThresholdPercent = 15.0,
    [string]$OutputPath = (Join-Path $SecondRunDirectory "gpu-timing-ab-comparison.json")
)

$ErrorActionPreference = "Stop"

function Read-Run([string]$directory) {
    $full = [IO.Path]::GetFullPath($directory)
    $summaryPath = Join-Path $full "summary.json"
    $timingPath = Join-Path $full "timing.csv"
    $thermalPath = Join-Path $full "thermal-before.txt"
    $capabilityPath = Join-Path $full "vulkan_capability_report.json"
    if (-not (Test-Path -LiteralPath $summaryPath) -or
        -not (Test-Path -LiteralPath $timingPath) -or
        -not (Test-Path -LiteralPath $thermalPath) -or
        -not (Test-Path -LiteralPath $capabilityPath)) {
        throw "A/B run is missing summary.json, timing.csv, thermal-before.txt, or Vulkan capability evidence: $full"
    }
    $summary = Get-Content -LiteralPath $summaryPath -Raw | ConvertFrom-Json
    if ([int]$summary.schema -lt 4) { throw "A/B run lacks exact installed-APK/source provenance: $full" }
    $thermal = Get-Content -LiteralPath $thermalPath -Raw
    $currentSection = [regex]::Match(
        $thermal,
        'Current temperatures from HAL:(?<body>[\s\S]*?)Current cooling devices from HAL:').Groups['body'].Value
    if ([string]::IsNullOrWhiteSpace($currentSection)) { throw "Could not read current HAL temperatures: $thermalPath" }
    $temperatures = [ordered]@{}
    foreach ($sensor in @("AP", "SKIN", "BAT")) {
        $match = [regex]::Match($currentSection, "Temperature\{mValue=([0-9.]+),[^`r`n]*mName=$sensor,")
        if (-not $match.Success) { throw "Could not read starting $sensor temperature: $thermalPath" }
        $temperatures[$sensor] = [double]$match.Groups[1].Value
    }
    $statusMatch = [regex]::Match($thermal, '(?m)^Thermal Status:\s*(\d+)\s*$')
    if (-not $statusMatch.Success) { throw "Could not read starting thermal status: $thermalPath" }
    return [PSCustomObject]@{
        directory = $full
        summary = $summary
        timing = @(Import-Csv -LiteralPath $timingPath)
        capability = Get-Content -LiteralPath $capabilityPath -Raw | ConvertFrom-Json
        startingThermalStatus = [int]$statusMatch.Groups[1].Value
        startingTemperaturesC = $temperatures
    }
}

$first = Read-Run $FirstRunDirectory
$second = Read-Run $SecondRunDirectory
$enabled = $null
$disabled = $null
foreach ($run in @($first, $second)) {
    if ($run.summary.gpuTiming -eq "enabled") { $enabled = $run }
    elseif ($run.summary.gpuTiming -eq "disabled") { $disabled = $run }
    else { throw "Unexpected GPU timing mode '$($run.summary.gpuTiming)' in $($run.directory)." }
}
if ($null -eq $enabled -or $null -eq $disabled) { throw "A/B requires exactly one enabled run and one disabled run." }

$mismatches = [Collections.Generic.List[string]]::new()
$provenanceWarnings = [Collections.Generic.List[string]]::new()
foreach ($field in @("deviceModel", "androidVersion", "apiLevel", "scale", "package", "apkSha256", "installedApkSha256", "sourceCommit", "sourceDirty", "raygenSha256")) {
    if ([string]$enabled.summary.$field -ne [string]$disabled.summary.$field) {
        $mismatches.Add("Run provenance differs at $field.")
    }
}
foreach ($field in @("gpuName", "vendorId", "deviceId", "driverVersion", "vulkanApiVersion")) {
    if ([string]$enabled.capability.$field -ne [string]$disabled.capability.$field) {
        $mismatches.Add("Vulkan device provenance differs at $field.")
    }
}
if ([int]$enabled.summary.schema -ge 5 -and [int]$disabled.summary.schema -ge 5) {
    foreach ($field in @("deviceSerial", "osBuildFingerprint")) {
        if ([string]::IsNullOrWhiteSpace([string]$enabled.summary.$field) -or
            [string]$enabled.summary.$field -ne [string]$disabled.summary.$field) {
            $mismatches.Add("Physical-device provenance differs or is missing at $field.")
        }
    }
} else {
    $provenanceWarnings.Add("Legacy schema-4 evidence lacks device serial and OS build fingerprint; model and Vulkan device/driver identity were matched instead.")
}
foreach ($run in @($enabled, $disabled)) {
    if ($run.summary.apkSha256 -ne $run.summary.installedApkSha256) {
        $mismatches.Add("Local and installed APK hashes differ in $($run.directory).")
    }
    if (@($run.summary.failures).Count -ne 0) { $mismatches.Add("Run reports validation failures: $($run.directory).") }
    foreach ($row in $run.timing) {
        if ([int]$row.scale -ne [int]$run.summary.scale -or
            [string]$row.gpu_timing -ne [string]$run.summary.gpuTiming -or
            [string]$row.timing_method -ne "cpu-present-loop" -or
            [string]$row.presented -ne "True") {
            $mismatches.Add("Timing-row metadata is inconsistent with honest matched-run provenance in $($run.directory).")
            break
        }
    }
}

$enabledCheckpoints = @($enabled.timing | ForEach-Object checkpoint)
$disabledCheckpoints = @($disabled.timing | ForEach-Object checkpoint)
if (($enabledCheckpoints -join '|') -ne ($disabledCheckpoints -join '|')) {
    $mismatches.Add("Checkpoint order differs between the two runs.")
}
if ($enabled.timing.Count -eq 0) { $mismatches.Add("A/B runs contain no benchmark rows.") }

$temperatureDifferences = [ordered]@{}
foreach ($sensor in @("AP", "SKIN", "BAT")) {
    $difference = [math]::Abs(
        [double]$enabled.startingTemperaturesC[$sensor] - [double]$disabled.startingTemperaturesC[$sensor])
    $temperatureDifferences[$sensor] = [math]::Round($difference, 2)
    if ($difference -gt $MaximumStartingTemperatureDifferenceC) {
        $mismatches.Add("Starting $sensor temperature differs by $([math]::Round($difference, 2)) C.")
    }
}
$thermalStatusDifference = [math]::Abs($enabled.startingThermalStatus - $disabled.startingThermalStatus)
if ($thermalStatusDifference -gt 1) { $mismatches.Add("Starting thermal status differs by more than one level.") }

$comparisons = [Collections.Generic.List[object]]::new()
$investigationRequired = $false
if ($mismatches.Count -eq 0) {
    for ($index = 0; $index -lt $enabled.timing.Count; ++$index) {
        $enabledRow = $enabled.timing[$index]
        $disabledRow = $disabled.timing[$index]
        $enabledMs = [double]$enabledRow.median_of_window_avgs_ms
        $disabledMs = [double]$disabledRow.median_of_window_avgs_ms
        $regressionPercent = $(if ($disabledMs -gt 0.0) { (($enabledMs / $disabledMs) - 1.0) * 100.0 } else { [double]::PositiveInfinity })
        $rowInvestigation = $regressionPercent -gt $InvestigationThresholdPercent
        $investigationRequired = $investigationRequired -or $rowInvestigation
        $comparisons.Add([PSCustomObject]@{
            checkpoint = $enabledRow.checkpoint
            enabledMedianMs = $enabledMs
            disabledMedianMs = $disabledMs
            enabledVersusDisabledPercent = [math]::Round($regressionPercent, 3)
            enabledMedianDerivedFps = [math]::Round(1000.0 / $enabledMs, 3)
            disabledMedianDerivedFps = [math]::Round(1000.0 / $disabledMs, 3)
            investigationRequired = $rowInvestigation
        })
    }
}

$passed = $mismatches.Count -eq 0 -and -not $investigationRequired
$result = [ordered]@{
    schema = 2
    passed = $passed
    enabledRun = $enabled.directory
    disabledRun = $disabled.directory
    matchedApkSha256 = $enabled.summary.apkSha256
    matchedSourceCommit = $enabled.summary.sourceCommit
    matchedSourceDirty = [bool]$enabled.summary.sourceDirty
    matchedRaygenSha256 = $enabled.summary.raygenSha256
    maximumStartingTemperatureDifferenceC = $MaximumStartingTemperatureDifferenceC
    startingTemperatureDifferencesC = $temperatureDifferences
    startingThermalStatusDifference = $thermalStatusDifference
    investigationThresholdPercent = $InvestigationThresholdPercent
    mismatches = @($mismatches)
    provenanceWarnings = @($provenanceWarnings)
    investigationRequired = $investigationRequired
    checkpoints = @($comparisons)
}
$outputFull = [IO.Path]::GetFullPath($OutputPath)
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $outputFull) | Out-Null
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $outputFull -Encoding utf8
if (-not $passed) { throw "GPU timing A/B comparison did not pass. See $outputFull" }
Write-Host "GPU timing A/B comparison passed: $outputFull"
