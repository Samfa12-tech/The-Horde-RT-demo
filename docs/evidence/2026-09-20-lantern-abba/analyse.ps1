[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$summaryPath = Join-Path $root 'analysis-summary.json'
$expectedFrames = 600
$expectedMaterialEncoding = 'ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2) + strict ASTC 6x6 lich'
$expectedZones = [ordered]@{
    'opening' = 0; 'skeleton-room' = 0; 'shadow-corridor' = 0;
    'skylight-chamber' = 0; 'yellow-torch-bay' = 600; 'blue-torch-bay' = 0;
    'red-torch-bay' = 0; 'green-torch-bay' = 0; 'transmission-threshold' = 0;
    'finale' = 0
}
$thermalStartBaselineA1 = [DateTimeOffset]::Parse('2026-09-20T07:07:14.9290809Z')
$failures = [System.Collections.Generic.List[string]]::new()

function Get-JsonValue($object, [string]$name) {
    $property = $object.PSObject.Properties[$name]
    if ($null -eq $property) { return $null }
    $property.Value
}
function Require-File([string]$path, [string]$label) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing ${label}: $path" }
}
function Read-JsonFile([string]$path, [string]$label) {
    Require-File $path $label
    if (-not (Get-Command ConvertFrom-Json).Parameters.ContainsKey('DateKind')) {
        throw 'PowerShell ConvertFrom-Json must support -DateKind String.'
    }
    Get-Content -LiteralPath $path -Raw | ConvertFrom-Json -DateKind String
}
function Require-Equal($actual, $expected, [string]$label) {
    if ($actual -ne $expected) { throw "$label expected '$expected', found '$actual'" }
}
function Text-Value([string]$text, [string]$pattern, [string]$label) {
    $match = [regex]::Match($text, $pattern, [Text.RegularExpressions.RegexOptions]::Multiline)
    if (-not $match.Success) { throw "Missing $label in legacy text" }
    $match.Groups['value'].Value
}
function Validate-Identity($row, [int]$index) {
    $submitted = $row.submittedIdentity
    $completed = $row.completionIdentity
    if ($null -eq $submitted -or $null -eq $completed) { throw "Row $index missing identity" }
    foreach ($name in @('sceneEpoch','measurementGeneration','recordAttemptSerial','recordSerial','simulationTick','frameSlot','submissionSerial')) {
        $left = $submitted.PSObject.Properties[$name]
        $right = $completed.PSObject.Properties[$name]
        if ($null -eq $left -or $null -eq $right -or $null -eq $left.Value -or $null -eq $right.Value) {
            throw "Row $index identity $name missing on one side"
        }
        Require-Equal $left.Value $right.Value "Row $index identity $name"
    }
}
function Read-Baseline([string]$path, [string]$run) {
    Require-File $path "$run legacy text"
    $text = (Get-Content -LiteralPath $path -Raw) -replace "`r`n", "`n"
    Require-Equal $text.Contains('Integrity: COMPLETE') $true "$run integrity"
    Require-Equal $text.Contains('Status: complete') $true "$run status"
    Require-Equal $text.Contains('RT presented every measured frame: yes') $true "$run presentation"
    Require-Equal (Text-Value $text '^Build: (?<value>.+)$' "$run build") '1.6.0 source-baseline (57c81b6 + lantern harness)' "$run build"
    Require-Equal (Text-Value $text '^Shader: (?<value>.+)$' "$run shader") 'd0c8cccea28d' "$run shader"
    Require-Equal (Text-Value $text '^Workload: (?<value>.+)$' "$run workload identity") 'lantern-held-high-v1' "$run workload identity"
    Require-Equal (Text-Value $text '^Simulation policy: (?<value>.+)$' "$run simulation policy") 'frozen-authored-snapshot' "$run simulation policy"
    Require-Equal (Text-Value $text '^Preset: (?<value>.+)$' "$run workload") 'lantern-held-high-v1' "$run workload"
    Require-Equal (Text-Value $text '^Simulation: (?<value>.+)$' "$run policy") 'frozen authored gameplay snapshot' "$run policy"
    Require-Equal (Text-Value $text '^Laps completed: (?<value>\d+)/2$' "$run laps") 2 "$run laps"
    Require-Equal (Text-Value $text '^Cases completed: (?<value>\d+)/2$' "$run cases") 2 "$run cases"
    if ($text -match '(?m)^Waypoints reached:') { throw "$run must not report route waypoints" }
    Require-Equal (Text-Value $text '^Measured frames: (?<value>\d+)$' "$run frames") $expectedFrames "$run frames"
    $record = [pscustomobject]@{
        run = $run
        timestampUtc = Text-Value $text '^Timestamp \(UTC\): (?<value>.+)$' "$run timestamp"
        gpu = Text-Value $text '^GPU: (?<value>.+)$' "$run GPU"
        vulkanApi = Text-Value $text '^Vulkan API: (?<value>.+)$' "$run Vulkan API"
        rtMode = Text-Value $text '^RT mode: (?<value>.+)$' "$run RT mode"
        presentMode = Text-Value $text '^Swapchain present mode: (?<value>.+)$' "$run present mode"
        materialEncoding = Text-Value $text '^Material route: (?<value>.+)$' "$run material"
        renderScalePercent = [int](Text-Value $text '^Render scale: (?<value>\d+)%$' "$run scale")
        internalExtent = Text-Value $text '^Internal RT extent: (?<value>.+)$' "$run internal extent"
        presentationExtent = Text-Value $text '^Presentation extent: (?<value>.+)$' "$run presentation extent"
        medianMs = [double](Text-Value $text '^Median: (?<value>[0-9.]+) ms' "$run median")
        p95Ms = [double](Text-Value $text '^P95: (?<value>[0-9.]+) ms$' "$run p95")
        zoneFrames = [ordered]@{}
    }
    Require-Equal $record.renderScalePercent 100 "$run scale"
    Require-Equal $record.internalExtent '1440x2980' "$run internal extent"
    Require-Equal $record.presentationExtent '1440x2980' "$run presentation extent"
    Require-Equal $record.materialEncoding $expectedMaterialEncoding "$run material"
    $zoneMatches = [regex]::Matches($text, '^(?<name>[a-z-]+),(?<frames>\d+),', [Text.RegularExpressions.RegexOptions]::Multiline)
    Require-Equal $zoneMatches.Count $expectedZones.Count "$run zone row count"
    foreach ($zoneMatch in $zoneMatches) {
        $zoneName = $zoneMatch.Groups['name'].Value
        if ($record.zoneFrames.Contains($zoneName)) { throw "$run duplicate zone $zoneName" }
        $record.zoneFrames[$zoneName] = [int]$zoneMatch.Groups['frames'].Value
    }
    Require-Equal $record.zoneFrames.Count $expectedZones.Count "$run zone count"
    foreach ($zone in $expectedZones.Keys) { Require-Equal $record.zoneFrames[$zone] $expectedZones[$zone] "$run zone $zone" }
    return $record
}
function Validate-Evidence($report, [string]$run) {
    $evidence = $report.completedFrameEvidence
    if ($null -eq $evidence) { throw "$run evidence missing" }
    Require-Equal $evidence.status 'complete' "$run evidence status"
    Require-Equal $evidence.invalidRun $false "$run invalidRun"
    foreach ($name in @('expected','completed','cpuAccepted')) { Require-Equal (Get-JsonValue $evidence.counts $name) $expectedFrames "$run count $name" }
    foreach ($name in @('rejected','cancelled','cpuRejected','outstanding')) { Require-Equal (Get-JsonValue $evidence.counts $name) 0 "$run count $name" }
    Require-Equal $evidence.rows.Count $expectedFrames "$run rows"
    Require-Equal (Get-JsonValue $evidence.gpuStatusCounts 'valid') $expectedFrames "$run GPU valid"
    foreach ($name in @('not-ready','compiled-out','unknown','disabled','unsupported','pending','error','missing')) {
        Require-Equal (Get-JsonValue $evidence.gpuStatusCounts $name) 0 "$run GPU $name"
    }
    foreach ($failure in $evidence.failureReasonCounts.PSObject.Properties) { Require-Equal $failure.Value 0 "$run failure $($failure.Name)" }
    $last = 0L
    for ($index = 0; $index -lt $expectedFrames; ++$index) {
        $row = $evidence.rows[$index]
        Require-Equal $row.index $index "$run row index"
        Require-Equal $row.cpuSampleIndex $index "$run CPU sample"
        Require-Equal $row.lap 2 "$run row lap"
        Require-Equal $row.disposition 'completed' "$run disposition"
        Require-Equal $row.failure 'none' "$run failure marker"
        Require-Equal $row.presentationOutcome 'presented' "$run presentation"
        Require-Equal $row.cpuStageStatus 'valid' "$run CPU status"
        Require-Equal $row.diagnosticStatus 'compiled-out' "$run diagnostic status"
        Require-Equal $row.gpuStatus 'valid' "$run GPU status"
        Require-Equal $row.cpuAccepted $true "$run CPU accepted"
        Validate-Identity $row $index
        $serial = [int64]$row.completionIdentity.completionSerial
        if ($serial -le $last) { throw "$run completion serial is not monotonic at row $index" }
        $last = $serial
    }
    $last
}
function Read-Candidate([string]$run) {
    $dir = Join-Path $root $run
    $marker = Read-JsonFile (Join-Path $dir 'result.json') "$run result"
    $report = Read-JsonFile (Join-Path $dir 'benchmark.json') "$run report"
    Require-Equal $marker.status 'complete' "$run result status"
    Require-Equal $marker.runId $run "$run marker runId"
    Require-Equal $report.runId $run "$run report runId"
    Require-Equal $report.result 'complete' "$run result"
    Require-Equal $report.schema 2 "$run schema"
    Require-Equal $report.routeTraversalComplete $false "$run route traversal"
    Require-Equal $report.workloadComplete $true "$run workload completion"
    Require-Equal $report.workload 'lantern-held-high-v1' "$run workload"
    Require-Equal $report.simulationPolicy 'frozen-authored-snapshot' "$run policy"
    Require-Equal $report.measuredFrames $expectedFrames "$run frames"
    Require-Equal $report.lapsCompleted 2 "$run laps"
    Require-Equal $report.lapsRequested 2 "$run requested laps"
    Require-Equal $report.completedCaseWindows 2 "$run case windows"
    Require-Equal $report.waypointsReached 0 "$run waypoints"
    Require-Equal $report.renderScalePercent 100 "$run scale"
    Require-Equal $report.internalExtent.width 1440 "$run internal width"
    Require-Equal $report.internalExtent.height 2980 "$run internal height"
    Require-Equal $report.presentationExtent.width 1440 "$run presentation width"
    Require-Equal $report.presentationExtent.height 2980 "$run presentation height"
    Require-Equal $report.rtMode 'RayTracingPipeline' "$run RT mode"
    Require-Equal $report.executionBackend 'RayTracingPipeline' "$run execution backend"
    Require-Equal $report.presentMode 'MAILBOX' "$run present mode"
    Require-Equal $report.legacyFrameTimingScope 'android-render-entry-through-present' "$run timing scope"
    Require-Equal $report.presentedEveryFrame $true "$run presentation proof"
    Require-Equal $report.materialEncoding $expectedMaterialEncoding "$run material"
    Require-Equal $report.zones.Count $expectedZones.Count "$run zones"
    foreach ($zone in $expectedZones.Keys) {
        $found = @($report.zones | Where-Object { $_.name -ceq $zone })
        Require-Equal $found.Count 1 "$run zone $zone presence"
        Require-Equal $found[0].frames $expectedZones[$zone] "$run zone $zone frames"
    }
    [pscustomobject]@{
        run = $run
        timestampUtc = [DateTimeOffset]::Parse([string]$report.timestampUtc)
        gpu = [string]$report.gpu
        vulkanApi = [string]$report.vulkanApi
        rtMode = [string]$report.rtMode
        presentMode = [string]$report.presentMode
        materialEncoding = [string]$report.materialEncoding
        medianMs = [double]$report.overall.medianMs
        p95Ms = [double]$report.overall.p95Ms
        lastCompletionSerial = Validate-Evidence $report $run
    }
}
function Read-Window($windows, [string]$name) {
    $key = ($name.ToLowerInvariant() -replace '[^a-z0-9]', '')
    foreach ($property in $windows.PSObject.Properties) {
        if ((([string]$property.Name).ToLowerInvariant() -replace '[^a-z0-9]', '') -eq $key) { return $property.Value }
    }
    throw "Missing run window for $name"
}
function Thermal-Rows {
    Require-File (Join-Path $root 'thermal-context.jsonl') 'thermal context'
    if (-not (Get-Command ConvertFrom-Json).Parameters.ContainsKey('DateKind')) { throw 'ConvertFrom-Json -DateKind String is required' }
    @(Get-Content -LiteralPath (Join-Path $root 'thermal-context.jsonl') | Where-Object { $_.Trim() } | ForEach-Object {
        $row = $_ | ConvertFrom-Json -DateKind String
        $thermal = [regex]::Match([string]$row.thermal, 'Thermal Status:\s*(\d+)')
        $battery = [regex]::Match([string]$row.battery, 'temperature:\s*(\d+)')
        $gpuValue = 0; $gpuParsed = [int]::TryParse(([string]$row.gpuThermalPowerLevel).Trim(), [ref]$gpuValue)
        [pscustomobject]@{
            utc = [DateTimeOffset]::Parse([string]$row.utc)
            thermalStatus = if ($thermal.Success) { [int]$thermal.Groups[1].Value } else { $null }
            batteryTempC = if ($battery.Success) { [double]$battery.Groups[1].Value / 10.0 } else { $null }
            gpu = if ($gpuParsed) { $gpuValue } else { $null }
        }
    })
}
function Thermal-Summary($rows, [string]$run, [DateTimeOffset]$start, [DateTimeOffset]$end) {
    $selected = @($rows | Where-Object { $_.utc -ge $start -and $_.utc -le $end })
    if (!$selected.Count) { throw "No thermal samples overlap $run window" }
    $thermal = @($selected | Where-Object { $null -ne $_.thermalStatus } | % thermalStatus)
    $battery = @($selected | Where-Object { $null -ne $_.batteryTempC } | % batteryTempC)
    $gpu = @($selected | Where-Object { $null -ne $_.gpu } | % gpu)
    [ordered]@{
        run = $run; windowStartUtc = $start.ToString('o'); windowEndUtc = $end.ToString('o'); sampleCount = $selected.Count
        thermalStatusMin = if ($thermal.Count) { ($thermal | Measure-Object -Minimum).Minimum } else { $null }
        thermalStatusMax = if ($thermal.Count) { ($thermal | Measure-Object -Maximum).Maximum } else { $null }
        batteryTempCMin = if ($battery.Count) { [math]::Round(($battery | Measure-Object -Minimum).Minimum, 1) } else { $null }
        batteryTempCMax = if ($battery.Count) { [math]::Round(($battery | Measure-Object -Maximum).Maximum, 1) } else { $null }
        gpuThermalPowerLevelMin = if ($gpu.Count) { ($gpu | Measure-Object -Minimum).Minimum } else { $null }
        gpuThermalPowerLevelMax = if ($gpu.Count) { ($gpu | Measure-Object -Maximum).Maximum } else { $null }
        gpuThermalPowerLevelMissingCount = $selected.Count - $gpu.Count
    }
}

$summary = [ordered]@{ schema = 1; status = 'fail'; generatedUtc = [DateTimeOffset]::UtcNow.ToString('o'); failures = @(); baseline = @(); candidates = @(); performance = $null; thermal = [ordered]@{ assessment = 'descriptive-only; temporal overlap is not a matched thermal-state claim'; runs = @() }; limitations = @('Observational comparison only; no causal performance claim.') }
try {
    $required = @(
        @{p=(Join-Path $root 'baseline-a1.txt');n='baseline-a1 text'}; @{p=(Join-Path $root 'baseline-a2.txt');n='baseline-a2 text'}
        @{p=(Join-Path $root 'held-high-b1-20260920/result.json');n='held-high-b1 result'}; @{p=(Join-Path $root 'held-high-b1-20260920/benchmark.json');n='held-high-b1 report'}
        @{p=(Join-Path $root 'held-high-b2-20260920/result.json');n='held-high-b2 result'}; @{p=(Join-Path $root 'held-high-b2-20260920/benchmark.json');n='held-high-b2 report'}
        @{p=(Join-Path $root 'held-high-b1-20260920-context.json');n='held-high-b1 context'}; @{p=(Join-Path $root 'held-high-b2-20260920-context.json');n='held-high-b2 context'}
        @{p=(Join-Path $root 'run-windows.json');n='run windows'}; @{p=(Join-Path $root 'thermal-context.jsonl');n='thermal context'})
    foreach ($item in $required) { if (!(Test-Path -LiteralPath $item.p -PathType Leaf)) { [void]$failures.Add("Missing $($item.n): $($item.p)") } }
    if ($failures.Count) { throw ($failures -join '; ') }
    $a1=Read-Baseline (Join-Path $root 'baseline-a1.txt') 'baseline-a1'; $a2=Read-Baseline (Join-Path $root 'baseline-a2.txt') 'baseline-a2'
    $b1=Read-Candidate 'held-high-b1-20260920'; $b2=Read-Candidate 'held-high-b2-20260920'; $summary.baseline=@($a1,$a2); $summary.candidates=@($b1,$b2)
    foreach ($item in @($a1,$a2,$b1,$b2)) { Require-Equal $item.gpu 'Adreno (TM) 840' "$($item.run) GPU"; Require-Equal $item.vulkanApi '1.4.295' "$($item.run) Vulkan"; Require-Equal $item.rtMode 'RayTracingPipeline' "$($item.run) RT"; Require-Equal $item.presentMode 'MAILBOX' "$($item.run) present"; Require-Equal $item.materialEncoding $expectedMaterialEncoding "$($item.run) material" }
    $baseMedian=(@($a1.medianMs,$a2.medianMs)|Measure-Object -Average).Average; $candMedian=(@($b1.medianMs,$b2.medianMs)|Measure-Object -Average).Average
    $summary.performance=[ordered]@{runs=@([ordered]@{run='baseline-a1';medianMs=[math]::Round($a1.medianMs,3);p95Ms=[math]::Round($a1.p95Ms,3)},[ordered]@{run='baseline-a2';medianMs=[math]::Round($a2.medianMs,3);p95Ms=[math]::Round($a2.p95Ms,3)},[ordered]@{run=$b1.run;medianMs=[math]::Round($b1.medianMs,3);p95Ms=[math]::Round($b1.p95Ms,3)},[ordered]@{run=$b2.run;medianMs=[math]::Round($b2.medianMs,3);p95Ms=[math]::Round($b2.p95Ms,3)});meanOfRunMediansBaselineMs=[math]::Round($baseMedian,3);meanOfRunMediansCandidateMs=[math]::Round($candMedian,3);meanOfRunMediansChangePercent=[math]::Round((($candMedian-$baseMedian)/$baseMedian)*100,3);interpretation='Descriptive mean-of-run-medians change; not pooled or causal.'}
    $windows=Read-JsonFile (Join-Path $root 'run-windows.json') 'run windows'; $thermal=Thermal-Rows
    $summary.thermal.runs += Thermal-Summary $thermal 'baseline-a1' $thermalStartBaselineA1 ([DateTimeOffset]::Parse($a1.timestampUtc)); $a2w=Read-Window $windows 'baseline-a2'; $summary.thermal.runs += Thermal-Summary $thermal 'baseline-a2' ([DateTimeOffset]::Parse([string](Get-JsonValue $a2w 'startedUtc'))) ([DateTimeOffset]::Parse($a2.timestampUtc))
    foreach ($candidate in @($b1,$b2)) { $context=Read-JsonFile (Join-Path $root "$($candidate.run)-context.json") "$($candidate.run) context"; $start=[DateTimeOffset]::Parse([string](Get-JsonValue $context 'startedUtc')); $endValue=Get-JsonValue $context 'observedCompleteUtc'; $end=if($null -ne $endValue){[DateTimeOffset]::Parse([string]$endValue)}else{$candidate.timestampUtc}; $summary.thermal.runs += Thermal-Summary $thermal $candidate.run $start $end }
    $summary.status='complete'
} catch { $message=$_.Exception.Message; if($failures.Count -eq 0 -or $message -ne ($failures -join '; ')){[void]$failures.Add($message)}; $summary.failures=@($failures) }
finally { if($summary.status -eq 'complete'){$summary.failures=@()}; [IO.File]::WriteAllText($summaryPath,($summary|ConvertTo-Json -Depth 12)+[Environment]::NewLine) }
if($summary.status -ne 'complete'){throw "Lantern ABBA analysis failed. See $summaryPath."}; Write-Output "Lantern ABBA analysis complete: $summaryPath"
