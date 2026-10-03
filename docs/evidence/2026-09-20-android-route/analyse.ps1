[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$summaryPath = Join-Path $root 'analysis-summary.json'
$expectedFrames = 1838
$expectedZones = [ordered]@{
    'opening' = 160
    'skeleton-room' = 98
    'shadow-corridor' = 599
    'skylight-chamber' = 189
    'yellow-torch-bay' = 157
    'blue-torch-bay' = 157
    'red-torch-bay' = 157
    'green-torch-bay' = 157
    'transmission-threshold' = 63
    'finale' = 101
}
$thermalStartBaselineA1 = [DateTimeOffset]::Parse('2026-09-20T06:24:14.4205173Z')
$failures = [System.Collections.Generic.List[string]]::new()

function Add-Failure([string]$message) {
    [void]$failures.Add($message)
}

function Require-File([string]$path, [string]$label) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing ${label}: $path"
    }
}

function Read-JsonFile([string]$path, [string]$label) {
    Require-File $path $label
    if (-not (Get-Command ConvertFrom-Json).Parameters.ContainsKey('DateKind')) {
        throw 'PowerShell ConvertFrom-Json must support -DateKind String to preserve evidence timestamps.'
    }
    return Get-Content -LiteralPath $path -Raw | ConvertFrom-Json -DateKind String
}

function Get-JsonValue($object, [string]$name) {
    $property = $object.PSObject.Properties[$name]
    if ($null -eq $property) { return $null }
    return $property.Value
}

function Require-Equal($actual, $expected, [string]$label) {
    if ($actual -ne $expected) {
        throw "$label expected '$expected', found '$actual'"
    }
}

function Require-TextValue([string]$text, [string]$pattern, [string]$label) {
    $match = [regex]::Match($text, $pattern, [Text.RegularExpressions.RegexOptions]::Multiline)
    if (-not $match.Success) { throw "Missing $label in legacy benchmark text" }
    return $match.Groups['value'].Value
}

function Read-LegacyRouteReport([string]$path, [string]$runName) {
    Require-File $path "$runName legacy benchmark text"
    $text = (Get-Content -LiteralPath $path -Raw) -replace "`r`n", "`n"
    Require-Equal ($text.Contains('Integrity: COMPLETE')) $true "$runName integrity"
    Require-Equal ($text.Contains('Status: complete')) $true "$runName status"
    Require-Equal ($text.Contains('RT presented every measured frame: yes')) $true "$runName RT presentation"
    $record = [ordered]@{
        run = $runName
        timestampUtc = Require-TextValue $text '^Timestamp \(UTC\): (?<value>.+)$' "$runName timestamp"
        rtMode = Require-TextValue $text '^RT mode: (?<value>.+)$' "$runName RT mode"
        gpu = Require-TextValue $text '^GPU: (?<value>.+)$' "$runName GPU"
        vulkanApi = Require-TextValue $text '^Vulkan API: (?<value>.+)$' "$runName Vulkan API"
        presentMode = Require-TextValue $text '^Swapchain present mode: (?<value>.+)$' "$runName present mode"
        materialEncoding = Require-TextValue $text '^Material route: (?<value>.+)$' "$runName material route"
        renderScalePercent = [int](Require-TextValue $text '^Render scale: (?<value>\d+)%$' "$runName scale")
        internalWidth = [int](Require-TextValue $text '^Internal RT extent: (?<value>\d+)x\d+$' "$runName internal width")
        internalHeight = [int]([regex]::Match($text, '^Internal RT extent: \d+x(?<value>\d+)$', [Text.RegularExpressions.RegexOptions]::Multiline).Groups['value'].Value)
        presentationWidth = [int](Require-TextValue $text '^Presentation extent: (?<value>\d+)x\d+$' "$runName presentation width")
        presentationHeight = [int]([regex]::Match($text, '^Presentation extent: \d+x(?<value>\d+)$', [Text.RegularExpressions.RegexOptions]::Multiline).Groups['value'].Value)
        lapsCompleted = [int](Require-TextValue $text '^Laps completed: (?<value>\d+)/2$' "$runName laps")
        waypointsReached = [int](Require-TextValue $text '^Waypoints reached: (?<value>\d+)/26$' "$runName waypoints")
        measuredFrames = [int](Require-TextValue $text '^Measured frames: (?<value>\d+)$' "$runName measured frames")
        medianMs = [double](Require-TextValue $text '^Median: (?<value>[0-9.]+) ms' "$runName median")
        p95Ms = [double](Require-TextValue $text '^P95: (?<value>[0-9.]+) ms$' "$runName p95")
        zoneFrames = [ordered]@{}
    }
    Require-Equal $record.lapsCompleted 2 "$runName laps"
    Require-Equal $record.waypointsReached 26 "$runName waypoints"
    Require-Equal $record.measuredFrames $expectedFrames "$runName measured frames"
    $zoneMatches = [regex]::Matches($text, '^(?<name>[a-z-]+),(?<frames>\d+),', [Text.RegularExpressions.RegexOptions]::Multiline)
    foreach ($zoneMatch in $zoneMatches) { $record.zoneFrames[$zoneMatch.Groups['name'].Value] = [int]$zoneMatch.Groups['frames'].Value }
    Require-Equal $record.zoneFrames.Count $expectedZones.Count "$runName zone count"
    foreach ($zone in $expectedZones.Keys) { Require-Equal $record.zoneFrames[$zone] $expectedZones[$zone] "$runName zone $zone" }
    return [pscustomobject]$record
}

function Validate-IdentityJoin($row, [int]$index) {
    $submitted = Get-JsonValue $row 'submittedIdentity'
    $completed = Get-JsonValue $row 'completionIdentity'
    if ($null -eq $submitted -or $null -eq $completed) { throw "Row $index missing submitted/completion identity" }
    foreach ($name in @('sceneEpoch','measurementGeneration','recordAttemptSerial','recordSerial','simulationTick','frameSlot','submissionSerial')) {
        $submittedProperty = $submitted.PSObject.Properties[$name]
        $completedProperty = $completed.PSObject.Properties[$name]
        if ($null -eq $submittedProperty -or $null -eq $completedProperty -or
            $null -eq $submittedProperty.Value -or $null -eq $completedProperty.Value) {
            throw "Row $index identity $name is missing on one side"
        }
        Require-Equal $submittedProperty.Value $completedProperty.Value "Row $index identity $name"
    }
}

function Validate-Evidence($report, [string]$runName) {
    $evidence = $report.completedFrameEvidence
    if ($null -eq $evidence) { throw "$runName completedFrameEvidence is missing" }
    Require-Equal $evidence.status 'complete' "$runName evidence status"
    Require-Equal $evidence.invalidRun $false "$runName invalidRun"
    foreach ($name in @('expected','completed','cpuAccepted')) { Require-Equal (Get-JsonValue $evidence.counts $name) $expectedFrames "$runName evidence $name" }
    foreach ($name in @('rejected','cancelled','cpuRejected','outstanding')) { Require-Equal (Get-JsonValue $evidence.counts $name) 0 "$runName evidence $name" }
    Require-Equal $evidence.rows.Count $expectedFrames "$runName evidence rows"
    Require-Equal (Get-JsonValue $evidence.gpuStatusCounts 'valid') $expectedFrames "$runName GPU valid"
    foreach ($name in @('not-ready','compiled-out','unknown','disabled','unsupported','pending','error','missing')) {
        Require-Equal (Get-JsonValue $evidence.gpuStatusCounts $name) 0 "$runName GPU $name"
    }
    foreach ($failure in $evidence.failureReasonCounts.PSObject.Properties) {
        Require-Equal $failure.Value 0 "$runName failure reason $($failure.Name)"
    }
    $lastCompletion = 0L
    for ($index = 0; $index -lt $expectedFrames; ++$index) {
        $row = $evidence.rows[$index]
        Require-Equal $row.index $index "$runName row index"
        Require-Equal $row.cpuSampleIndex $index "$runName CPU sample index"
        Require-Equal $row.lap 2 "$runName row lap"
        Require-Equal $row.disposition 'completed' "$runName row disposition"
        Require-Equal $row.failure 'none' "$runName row failure"
        Require-Equal $row.presentationOutcome 'presented' "$runName row presentation"
        Require-Equal $row.cpuStageStatus 'valid' "$runName row CPU status"
        Require-Equal $row.diagnosticStatus 'compiled-out' "$runName row diagnostic status"
        Require-Equal $row.gpuStatus 'valid' "$runName row GPU status"
        Require-Equal $row.cpuAccepted $true "$runName row CPU accepted"
        Validate-IdentityJoin $row $index
        $completionSerial = [int64]$row.completionIdentity.completionSerial
        if ($completionSerial -le $lastCompletion) { throw "$runName completion serial is not monotonic at row $index" }
        $lastCompletion = $completionSerial
    }
    return $lastCompletion
}

function Read-CandidateReport([string]$runId) {
    $directory = Join-Path $root $runId
    $marker = Read-JsonFile (Join-Path $directory 'result.json') "$runId result marker"
    $report = Read-JsonFile (Join-Path $directory 'benchmark.json') "$runId benchmark report"
    Require-Equal $marker.status 'complete' "$runId result status"
    Require-Equal $marker.runId $runId "$runId result identity"
    Require-Equal $report.runId $runId "$runId report identity"
    Require-Equal $report.result 'complete' "$runId result"
    Require-Equal $report.schema 2 "$runId schema"
    Require-Equal $report.routeTraversalComplete $true "$runId route traversal"
    Require-Equal $report.workloadComplete $true "$runId workload completion"
    Require-Equal $report.workload 'showcase-route-v1' "$runId workload"
    Require-Equal $report.simulationPolicy 'fixed-step-60hz' "$runId simulation policy"
    Require-Equal $report.presentedEveryFrame $true "$runId presented-every-frame"
    Require-Equal $report.executionBackend 'RayTracingPipeline' "$runId execution backend"
    Require-Equal $report.legacyFrameTimingScope 'android-render-entry-through-present' "$runId legacy timing scope"
    Require-Equal $report.lapsCompleted 2 "$runId laps"
    Require-Equal $report.waypointsReached 26 "$runId waypoints"
    Require-Equal $report.measuredFrames $expectedFrames "$runId measured frames"
    Require-Equal $report.renderScalePercent 76 "$runId render scale"
    Require-Equal $report.internalExtent.width 1094 "$runId internal width"
    Require-Equal $report.internalExtent.height 2265 "$runId internal height"
    Require-Equal $report.presentationExtent.width 1440 "$runId presentation width"
    Require-Equal $report.presentationExtent.height 2980 "$runId presentation height"
    Require-Equal $report.rtMode 'RayTracingPipeline' "$runId RT mode"
    Require-Equal $report.presentMode 'MAILBOX' "$runId present mode"
    if ($report.materialEncoding -notmatch 'ASTC') { throw "$runId material route is not ASTC" }
    Require-Equal $report.zones.Count $expectedZones.Count "$runId zone count"
    foreach ($zone in $expectedZones.Keys) {
        $found = @($report.zones | Where-Object { $_.name -ceq $zone })
        Require-Equal $found.Count 1 "$runId zone $zone presence"
        Require-Equal $found[0].frames $expectedZones[$zone] "$runId zone $zone frames"
    }
    $lastCompletion = Validate-Evidence $report $runId
    return [pscustomobject]@{
        run = $runId
        timestampUtc = [DateTimeOffset]::Parse($report.timestampUtc)
        gpu = [string]$report.gpu
        vulkanApi = [string]$report.vulkanApi
        rtMode = [string]$report.rtMode
        presentMode = [string]$report.presentMode
        materialEncoding = [string]$report.materialEncoding
        medianMs = [double]$report.overall.medianMs
        p95Ms = [double]$report.overall.p95Ms
        lastCompletionSerial = $lastCompletion
    }
}

function Read-RunWindow($windows, [string]$runName) {
    $direct = Get-JsonValue $windows $runName
    if ($null -ne $direct) { return $direct }
    $normalizedRunName = ($runName.ToLowerInvariant() -replace '[^a-z0-9]', '')
    foreach ($property in $windows.PSObject.Properties) {
        if (([string]$property.Name).ToLowerInvariant() -replace '[^a-z0-9]', '' -eq $normalizedRunName) {
            return $property.Value
        }
    }
    foreach ($collectionName in @('runs','windows','observations')) {
        $collection = Get-JsonValue $windows $collectionName
        if ($null -eq $collection) { continue }
        foreach ($entry in @($collection)) {
            if ((Get-JsonValue $entry 'run') -eq $runName -or (Get-JsonValue $entry 'runId') -eq $runName -or
                (Get-JsonValue $entry 'name') -eq $runName -or
                (([string](Get-JsonValue $entry 'run')).ToLowerInvariant() -replace '[^a-z0-9]', '') -eq $normalizedRunName -or
                (([string](Get-JsonValue $entry 'runId')).ToLowerInvariant() -replace '[^a-z0-9]', '') -eq $normalizedRunName -or
                (([string](Get-JsonValue $entry 'name')).ToLowerInvariant() -replace '[^a-z0-9]', '') -eq $normalizedRunName) { return $entry }
        }
    }
    throw "Missing run window for $runName"
}

function Read-ThermalRows {
    Require-File (Join-Path $root 'thermal-context.jsonl') 'thermal context'
    return @(Get-Content -LiteralPath (Join-Path $root 'thermal-context.jsonl') | Where-Object { $_.Trim() } |
        ForEach-Object {
            if (-not (Get-Command ConvertFrom-Json).Parameters.ContainsKey('DateKind')) {
                throw 'PowerShell ConvertFrom-Json must support -DateKind String to preserve thermal timestamps.'
            }
            $row = $_ | ConvertFrom-Json -DateKind String
            $thermalMatch = [regex]::Match([string]$row.thermal, 'Thermal Status:\s*(\d+)')
            $temperatureMatch = [regex]::Match([string]$row.battery, 'temperature:\s*(\d+)')
            $gpuValue = 0
            $gpuParsed = [int]::TryParse(([string]$row.gpuThermalPowerLevel).Trim(), [ref]$gpuValue)
            [pscustomobject]@{
                utc = [DateTimeOffset]::Parse([string]$row.utc)
                thermalStatus = if ($thermalMatch.Success) { [int]$thermalMatch.Groups[1].Value } else { $null }
                batteryTempC = if ($temperatureMatch.Success) { [double]$temperatureMatch.Groups[1].Value / 10.0 } else { $null }
                gpuThermalPowerLevel = if ($gpuParsed) { $gpuValue } else { $null }
            }
        })
}

function Summarize-Thermal($rows, [string]$runName, [DateTimeOffset]$start, [DateTimeOffset]$end) {
    $selected = @($rows | Where-Object { $_.utc -ge $start -and $_.utc -le $end })
    if ($selected.Count -eq 0) { throw "No thermal context samples overlap $runName window $start to $end" }
    $thermal = @($selected | Where-Object { $null -ne $_.thermalStatus } | ForEach-Object thermalStatus)
    $battery = @($selected | Where-Object { $null -ne $_.batteryTempC } | ForEach-Object batteryTempC)
    $gpu = @($selected | Where-Object { $null -ne $_.gpuThermalPowerLevel } | ForEach-Object gpuThermalPowerLevel)
    return [ordered]@{
        run = $runName
        windowStartUtc = $start.ToString('o')
        windowEndUtc = $end.ToString('o')
        sampleCount = $selected.Count
        thermalStatusMissingCount = $selected.Count - $thermal.Count
        batteryTempMissingCount = $selected.Count - $battery.Count
        gpuThermalPowerLevelMissingCount = $selected.Count - $gpu.Count
        thermalStatusMin = if ($thermal.Count) { ($thermal | Measure-Object -Minimum).Minimum } else { $null }
        thermalStatusMax = if ($thermal.Count) { ($thermal | Measure-Object -Maximum).Maximum } else { $null }
        batteryTempCMin = if ($battery.Count) { [math]::Round(($battery | Measure-Object -Minimum).Minimum, 1) } else { $null }
        batteryTempCMax = if ($battery.Count) { [math]::Round(($battery | Measure-Object -Maximum).Maximum, 1) } else { $null }
        gpuThermalPowerLevelMin = if ($gpu.Count) { ($gpu | Measure-Object -Minimum).Minimum } else { $null }
        gpuThermalPowerLevelMax = if ($gpu.Count) { ($gpu | Measure-Object -Maximum).Maximum } else { $null }
    }
}

$summary = [ordered]@{
    schema = 1
    status = 'fail'
    generatedUtc = [DateTimeOffset]::UtcNow.ToString('o')
    failures = @()
    baseline = @()
    candidates = @()
    performance = $null
    thermal = [ordered]@{ assessment = 'descriptive-only; temporal overlap is not a matched thermal-state claim'; runs = @() }
    limitations = @('This report is observational and does not make a causal performance claim.')
}

try {
    $requiredInputs = @(
        @{ path = (Join-Path $root 'baseline-a1.txt'); label = 'baseline-a1 legacy text' }
        @{ path = (Join-Path $root 'baseline-a2.txt'); label = 'baseline-a2 legacy text' }
        @{ path = (Join-Path $root 'route-b1-20260920/result.json'); label = 'route-b1 result marker' }
        @{ path = (Join-Path $root 'route-b1-20260920/benchmark.json'); label = 'route-b1 benchmark report' }
        @{ path = (Join-Path $root 'route-b2-20260920/result.json'); label = 'route-b2 result marker' }
        @{ path = (Join-Path $root 'route-b2-20260920/benchmark.json'); label = 'route-b2 benchmark report' }
        @{ path = (Join-Path $root 'route-b1-20260920-context.json'); label = 'route-b1 context' }
        @{ path = (Join-Path $root 'route-b2-20260920-context.json'); label = 'route-b2 context' }
        @{ path = (Join-Path $root 'run-windows.json'); label = 'run windows' }
        @{ path = (Join-Path $root 'thermal-context.jsonl'); label = 'thermal context' }
    )
    foreach ($input in $requiredInputs) {
        if (-not (Test-Path -LiteralPath $input.path -PathType Leaf)) {
            [void]$failures.Add("Missing $($input.label): $($input.path)")
        }
    }
    if ($failures.Count -gt 0) { throw ($failures -join '; ') }
    $baselineA1 = Read-LegacyRouteReport (Join-Path $root 'baseline-a1.txt') 'baseline-a1'
    $baselineA2 = Read-LegacyRouteReport (Join-Path $root 'baseline-a2.txt') 'baseline-a2'
    $candidateB1 = Read-CandidateReport 'route-b1-20260920'
    $candidateB2 = Read-CandidateReport 'route-b2-20260920'
    $summary.baseline = @($baselineA1, $baselineA2)
    $summary.candidates = @($candidateB1, $candidateB2)

    foreach ($baseline in @($baselineA1, $baselineA2)) {
        Require-Equal $baseline.rtMode 'RayTracingPipeline' "$($baseline.run) RT mode"
        Require-Equal $baseline.presentMode 'MAILBOX' "$($baseline.run) present mode"
        if ($baseline.materialEncoding -notmatch 'ASTC') { throw "$($baseline.run) material route is not ASTC" }
        Require-Equal $baseline.renderScalePercent 76 "$($baseline.run) render scale"
        Require-Equal $baseline.internalWidth 1094 "$($baseline.run) internal width"
        Require-Equal $baseline.internalHeight 2265 "$($baseline.run) internal height"
        Require-Equal $baseline.presentationWidth 1440 "$($baseline.run) presentation width"
        Require-Equal $baseline.presentationHeight 2980 "$($baseline.run) presentation height"
    }
    $metadataRuns = @($baselineA1, $baselineA2, $candidateB1, $candidateB2)
    foreach ($field in @('gpu','vulkanApi','rtMode','presentMode','materialEncoding')) {
        $distinct = @($metadataRuns | ForEach-Object { [string](Get-JsonValue $_ $field) } | Select-Object -Unique)
        Require-Equal $distinct.Count 1 "common metadata $field"
    }
    $baselineMeanMedian = (@($baselineA1.medianMs, $baselineA2.medianMs) | Measure-Object -Average).Average
    $candidateMeanMedian = (@($candidateB1.medianMs, $candidateB2.medianMs) | Measure-Object -Average).Average
    $summary.performance = [ordered]@{
        runs = @(
            [ordered]@{ run = 'baseline-a1'; medianMs = [math]::Round($baselineA1.medianMs, 3); p95Ms = [math]::Round($baselineA1.p95Ms, 3) }
            [ordered]@{ run = 'baseline-a2'; medianMs = [math]::Round($baselineA2.medianMs, 3); p95Ms = [math]::Round($baselineA2.p95Ms, 3) }
            [ordered]@{ run = 'route-b1-20260920'; medianMs = [math]::Round($candidateB1.medianMs, 3); p95Ms = [math]::Round($candidateB1.p95Ms, 3) }
            [ordered]@{ run = 'route-b2-20260920'; medianMs = [math]::Round($candidateB2.medianMs, 3); p95Ms = [math]::Round($candidateB2.p95Ms, 3) }
        )
        meanOfRunMediansBaselineMs = [math]::Round($baselineMeanMedian, 3)
        meanOfRunMediansCandidateMs = [math]::Round($candidateMeanMedian, 3)
        meanOfRunMediansChangePercent = [math]::Round((($candidateMeanMedian - $baselineMeanMedian) / $baselineMeanMedian) * 100.0, 3)
        interpretation = 'Descriptive mean-of-run-medians change only; not pooled and not causal.'
    }

    $runWindowsPath = Join-Path $root 'run-windows.json'
    $thermalRows = Read-ThermalRows
    $baselineA1End = [DateTimeOffset]::Parse($baselineA1.timestampUtc)
    $windows = Read-JsonFile $runWindowsPath 'run windows'
    $baselineA2Window = Read-RunWindow $windows 'baseline-a2'
    $baselineA2Start = [DateTimeOffset]::Parse([string](Get-JsonValue $baselineA2Window 'startedUtc'))
    $baselineA2End = [DateTimeOffset]::Parse($baselineA2.timestampUtc)
    $summary.thermal.runs += Summarize-Thermal $thermalRows 'baseline-a1' $thermalStartBaselineA1 $baselineA1End
    $summary.thermal.runs += Summarize-Thermal $thermalRows 'baseline-a2' $baselineA2Start $baselineA2End
    foreach ($candidate in @($candidateB1, $candidateB2)) {
        $contextPath = Join-Path $root "$($candidate.run)-context.json"
        $context = Read-JsonFile $contextPath "$($candidate.run) context"
        $start = [DateTimeOffset]::Parse([string](Get-JsonValue $context 'startedUtc'))
        $endValue = Get-JsonValue $context 'observedCompleteUtc'
        $end = if ($null -ne $endValue) { [DateTimeOffset]::Parse([string]$endValue) } else { $candidate.timestampUtc }
        $summary.thermal.runs += Summarize-Thermal $thermalRows $candidate.run $start $end
    }
    $summary.status = 'complete'
}
catch {
    $errorMessage = $_.Exception.Message
    if ($failures.Count -eq 0 -or $errorMessage -ne ($failures -join '; ')) {
        Add-Failure $errorMessage
    }
    $summary.failures = @($failures)
}
finally {
    if ($summary.status -eq 'complete') { $summary.failures = @() }
    [IO.File]::WriteAllText($summaryPath, ($summary | ConvertTo-Json -Depth 12) + [Environment]::NewLine)
}

if ($summary.status -ne 'complete') {
    throw "ABBA analysis failed. See $summaryPath for explicit missing/invalid evidence."
}
Write-Output "ABBA analysis complete: $summaryPath"
