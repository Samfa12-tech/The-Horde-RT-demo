param(
    [Parameter(Mandatory=$true)][string]$RunId,
    [Parameter(Mandatory=$true)][string]$BuildLabel,
    [Parameter(Mandatory=$true)][string]$SourceCommit,
    [Parameter(Mandatory=$true)][string]$InstalledApkSha256,
    [ValidateSet('showcase-route-v1','lantern-held-high-v1','lantern-held-low-v1','lantern-grazing-v1','lantern-reveal-sequence-v1')][string]$Workload,
    [switch]$ResumeExisting
)
$ErrorActionPreference='Stop'
$adb='C:/Users/sam_s/AppData/Local/Android/Sdk/platform-tools/adb.exe'
$serial='R5GL219SZGK'
$package='com.samfa12.hordelanternrt.benchmark'
$destination=Join-Path $PSScriptRoot "phone/$RunId"
$remote="/sdcard/Android/data/$package/files/benchmarks/$RunId"
if ($RunId -notmatch '^[a-zA-Z0-9_-]+$') { throw 'Invalid unique run id' }
if ((& $adb -s $serial shell getprop ro.product.model | Out-String).Trim() -ne 'SM-S948B') { throw 'Wrong physical device' }
if (Test-Path -LiteralPath $destination) { throw 'Do not overwrite existing local trial' }
& $adb -s $serial shell test -d $remote
if ($LASTEXITCODE -eq 0 -and -not $ResumeExisting) { throw 'Do not reuse existing remote run id' }
New-Item -ItemType Directory -Path $destination | Out-Null
$start=[DateTime]::UtcNow
$model=[ordered]@{runId=$RunId;buildLabel=$BuildLabel;sourceCommit=$SourceCommit;installedApkSha256=$InstalledApkSha256;workload=$Workload;deviceModel='SM-S948B';serial=$serial;package=$package;requestedScalePercent=75;requestedBackend='RayTracingPipeline';instrumentation='Shipping';quality='Mobile';startUtc=$start.ToString('o');recordingEnabled=$false;contextIntervalSeconds=5;observationDeadlineMinutes=30;resumeExisting=[bool]$ResumeExisting;status='starting'}
$model | ConvertTo-Json -Depth 5 > (Join-Path $destination 'trial.json')
if (-not $ResumeExisting) {
    # No force-stop, app-data reset, Debug intents, quality reduction or recording.
    # am-start observation timeouts are not a reason to relaunch the benchmark.
    & $adb -s $serial shell am start -W --user 0 -n "$package/com.samfa12.hordelanternrt.MainActivity" -a com.samfa12.hordelanternrt.action.BENCHMARK --es horde.benchmark.run_id $RunId --es horde.benchmark.workload $Workload *> (Join-Path $destination 'launch.log')
}
# This is a compiler-treatment cost investigation, not production acceptance.
# Keep the existing external observation deadline and exact workload/denominators.
$deadline=[DateTime]::UtcNow.AddMinutes(30)
$samples=[Collections.Generic.List[object]]::new()
$pidAtStart=''
do {
    $pidNow=(& $adb -s $serial shell pidof $package | Out-String).Trim()
    $battery=(& $adb -s $serial shell dumpsys battery | Out-String)
    $thermal=(& $adb -s $serial shell dumpsys thermalservice | Out-String)
    $power=(& $adb -s $serial shell cat /sys/class/kgsl/kgsl-3d0/thermal_pwrlevel 2>$null | Out-String).Trim()
    $tempMatch=[regex]::Match($battery,'(?m)^\s*temperature:\s*(\d+)')
    $thermalMatch=[regex]::Match($thermal,'(?m)^Thermal Status:\s*(\d+)')
    $powerValue=$null
    if ($power -match '^\d+$') { $powerValue=[int]$power }
    $sample=[ordered]@{utc=[DateTime]::UtcNow.ToString('o');processId=$pidNow;batteryC=if($tempMatch.Success){[int]$tempMatch.Groups[1].Value/10.0}else{$null};thermalStatus=if($thermalMatch.Success){[int]$thermalMatch.Groups[1].Value}else{$null};gpuThermalPowerLevel=$powerValue}
    $samples.Add($sample)
    [IO.File]::AppendAllText((Join-Path $destination 'context-samples.jsonl'),($sample | ConvertTo-Json -Compress)+"`n")
    if ($pidNow -and -not $pidAtStart) { $pidAtStart=$pidNow }
    & $adb -s $serial shell test -f "$remote/result.json"
    if ($LASTEXITCODE -eq 0) { break }
    if ([DateTime]::UtcNow -gt $deadline) { throw "Observation timeout for $RunId; inspect this same run before any restart" }
    if ($pidAtStart -and $pidNow -ne $pidAtStart) { throw "Benchmark process changed before result: $pidAtStart -> $pidNow" }
    Start-Sleep -Seconds 5
} while ($true)
& $adb -s $serial pull $remote $destination *> (Join-Path $destination 'pull.log')
if ($LASTEXITCODE -ne 0) { throw 'Cannot retain existing completed report' }
$marker=Get-Content (Join-Path $destination "$RunId/result.json") -Raw | ConvertFrom-Json
$report=Get-Content (Join-Path $destination "$RunId/benchmark.json") -Raw | ConvertFrom-Json
if($marker.status -ne 'complete' -or $marker.runId -ne $RunId -or $report.status -ne 'complete' -or $report.runId -ne $RunId -or $report.workload -ne $Workload){throw 'Invalid completion identity/workload'}
if([int]$report.renderScalePercent -ne 75 -or $report.executionBackend -ne 'RayTracingPipeline' -or $report.materialEncoding -ne 'ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2) + strict ASTC 6x6 lich'){throw 'Wrong scale/backend/encoding: retain report but do not accept matched trial'}
$expected=if($Workload -eq 'showcase-route-v1'){1838}else{600}
if([int]$report.measuredFrames -ne $expected -or -not $report.presentedEveryFrame){throw 'Incomplete presented-frame denominator'}
$model.status='complete'
$model.processId=$pidAtStart
$model.endUtc=[DateTime]::UtcNow.ToString('o')
$model.samples=$samples.ToArray()
$model.renderScalePercent=$report.renderScalePercent
$model.internalExtent=$report.internalExtent
$model.presentationExtent=$report.presentationExtent
$model.presentMode=$report.presentMode
$model.overall=$report.overall
$model | ConvertTo-Json -Depth 9 > (Join-Path $destination 'trial.json')
Write-Output "$RunId complete; $expected presented frames; median $($report.overall.medianMs) ms; p95 $($report.overall.p95Ms) ms. Not causal A/B acceptance until integrity/thermal matching."
