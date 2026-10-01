param(
    [Parameter(Mandatory=$true)][string]$RunId,
    [Parameter(Mandatory=$true)][string]$SourceCommit,
    [Parameter(Mandatory=$true)][string]$ApkPath,
    [Parameter(Mandatory=$true)][string]$ApkSha256,
    [Parameter(Mandatory=$true)][string]$DeviceSerial,
    [Parameter(Mandatory=$true)][string]$DeviceModel,
    [Parameter(Mandatory=$true)][ValidateSet('RayTracingPipeline','RayQueryCompute')][string]$Backend,
    [Parameter(Mandatory=$true)][ValidateSet('normal-control','primary-opacity-candidate')][string]$Member,
    [Parameter(Mandatory=$true)][ValidateSet('showcase-route-v1','lantern-held-high-v1','lantern-reveal-sequence-v1')][string]$Workload,
    [Parameter(Mandatory=$true)][string]$OutputRoot,
    [switch]$Install
)
$ErrorActionPreference='Stop'
# Reuses September30's exact Shipping trial method. This wrapper changes only
# identity/device parameters and records memory/pacing sources, not the workload.
$adb='C:/Users/sam_s/AppData/Local/Android/Sdk/platform-tools/adb.exe'
$package='com.samfa12.hordelanternrt.benchmark'
if($RunId -notmatch '^[a-zA-Z0-9_-]+$' -or $SourceCommit -notmatch '^[a-f0-9]{40}$' -or
   $ApkSha256 -notmatch '^[a-f0-9]{64}$'){throw 'Invalid immutable trial identity'}
if((Get-FileHash -LiteralPath $ApkPath).Hash.ToLowerInvariant() -cne $ApkSha256){throw 'APK changed'}
if((& $adb -s $DeviceSerial shell getprop ro.product.model | Out-String).Trim() -cne $DeviceModel){throw 'Wrong device'}
$destination=Join-Path $OutputRoot $RunId
$remote="/sdcard/Android/data/$package/files/benchmarks/$RunId"
if(Test-Path -LiteralPath $destination){throw 'Do not repeat/overwrite retained trial'}
& $adb -s $DeviceSerial shell test -d $remote
if($LASTEXITCODE -eq 0){throw 'Remote trial already exists; retain it, do not restart'}
New-Item -ItemType Directory -Path $destination | Out-Null
if($Install){
    & $adb -s $DeviceSerial install -r -t $ApkPath *> (Join-Path $destination 'install.log')
    if($LASTEXITCODE -ne 0){throw 'Install failed'}
}
$remoteApk=((& $adb -s $DeviceSerial shell pm path $package | Out-String).Trim() -replace '^package:','')
if($remoteApk -notmatch '^/data/app/[^\r\n]+/base\.apk$'){throw 'Unexpected installed APK path'}
$pullback=Join-Path $destination 'installed-base.apk'
& $adb -s $DeviceSerial pull $remoteApk $pullback *> (Join-Path $destination 'pullback.log')
if($LASTEXITCODE -ne 0 -or (Get-FileHash -LiteralPath $pullback).Hash.ToLowerInvariant() -cne $ApkSha256){throw 'Installed artifact mismatch'}
$model=[ordered]@{runId=$RunId;sourceCommit=$SourceCommit;installedApkSha256=$ApkSha256;
    member=$Member;workload=$Workload;deviceModel=$DeviceModel;serial=$DeviceSerial;package=$package;
    requestedScalePercent=75;requestedBackend=$Backend;instrumentation='Shipping';quality='Mobile';
    startUtc=[DateTime]::UtcNow.ToString('o');recordingEnabled=$false;appDataCleared=$false;
    status='starting';gpuMemoryCounters='not-collected; not a pass';memoryBoundary='before/after trial; not frame aligned'}
$model | ConvertTo-Json -Depth 5 > (Join-Path $destination 'trial.json')
& $adb -s $DeviceSerial shell am start -W --user 0 -n "$package/com.samfa12.hordelanternrt.MainActivity" -a com.samfa12.hordelanternrt.action.BENCHMARK --es horde.benchmark.run_id $RunId --es horde.benchmark.workload $Workload *> (Join-Path $destination 'launch.log')
& $adb -s $DeviceSerial shell dumpsys meminfo $package *> (Join-Path $destination 'memory-before.txt')
& $adb -s $DeviceSerial shell cat /proc/pressure/memory *> (Join-Path $destination 'memory-pressure-before.txt')
& $adb -s $DeviceSerial shell dumpsys SurfaceFlinger --list *> (Join-Path $destination 'surface-layers-before.txt')
$deadline=[DateTime]::UtcNow.AddMinutes(15)
$pidAtStart=''
$activeMemoryAt=[DateTime]::UtcNow.AddSeconds(30)
$activeMemorySampled=$false
do {
    $pidNow=(& $adb -s $DeviceSerial shell pidof $package | Out-String).Trim()
    $battery=(& $adb -s $DeviceSerial shell dumpsys battery | Out-String)
    $thermal=(& $adb -s $DeviceSerial shell dumpsys thermalservice | Out-String)
    $power=(& $adb -s $DeviceSerial shell cat /sys/class/kgsl/kgsl-3d0/thermal_pwrlevel 2>$null | Out-String).Trim()
    $temperature=[regex]::Match($battery,'(?m)^\s*temperature:\s*(\d+)')
    $status=[regex]::Match($thermal,'(?m)^Thermal Status:\s*(\d+)')
    $sample=[ordered]@{utc=[DateTime]::UtcNow.ToString('o');processId=$pidNow;
        batteryC=if($temperature.Success){[int]$temperature.Groups[1].Value/10.0}else{$null};
        thermalStatus=if($status.Success){[int]$status.Groups[1].Value}else{$null};
        gpuThermalPowerLevel=if($power -match '^\d+$'){[int]$power}else{$null}}
    [IO.File]::AppendAllText((Join-Path $destination 'context-samples.jsonl'),($sample | ConvertTo-Json -Compress)+"`n")
    if($pidNow -and -not $pidAtStart){$pidAtStart=$pidNow}
    & $adb -s $DeviceSerial shell test -f "$remote/result.json"
    if($LASTEXITCODE -eq 0){break}
    if(-not $activeMemorySampled -and [DateTime]::UtcNow -ge $activeMemoryAt){
        # One sparse, matched active snapshot. Retain its time separately: it is
        # not a GPU bandwidth counter or a per-frame RAM-pressure attribution.
        $memoryStart=[DateTime]::UtcNow.ToString('o')
        & $adb -s $DeviceSerial shell dumpsys meminfo $package *> (Join-Path $destination 'memory-active.txt')
        & $adb -s $DeviceSerial shell cat /proc/pressure/memory *> (Join-Path $destination 'memory-pressure-active.txt')
        & $adb -s $DeviceSerial shell cat /proc/meminfo *> (Join-Path $destination 'system-memory-active.txt')
        [ordered]@{startUtc=$memoryStart;endUtc=[DateTime]::UtcNow.ToString('o');processId=$pidNow;scope='one active snapshot; not frame aligned'} | ConvertTo-Json > (Join-Path $destination 'memory-active-context.json')
        $activeMemorySampled=$true
    }
    if([DateTime]::UtcNow -gt $deadline){throw 'Timed out: inspect/resume this exact trial, do not restart'}
    if($pidAtStart -and $pidNow -cne $pidAtStart){throw 'Process changed; preserve incomplete evidence'}
    Start-Sleep -Seconds 5
} while($true)
& $adb -s $DeviceSerial pull $remote $destination *> (Join-Path $destination 'pull.log')
if($LASTEXITCODE -ne 0){throw 'Report pull failed; retain existing remote result'}
& $adb -s $DeviceSerial shell dumpsys meminfo $package *> (Join-Path $destination 'memory-after.txt')
& $adb -s $DeviceSerial shell cat /proc/pressure/memory *> (Join-Path $destination 'memory-pressure-after.txt')
& $adb -s $DeviceSerial shell dumpsys SurfaceFlinger --list *> (Join-Path $destination 'surface-layers-after.txt')
$marker=Get-Content (Join-Path $destination "$RunId/result.json") -Raw | ConvertFrom-Json
$report=Get-Content (Join-Path $destination "$RunId/benchmark.json") -Raw | ConvertFrom-Json
if($marker.status -cne 'complete' -or $marker.runId -cne $RunId -or $report.status -cne 'complete' -or
   $report.runId -cne $RunId -or $report.workload -cne $Workload){throw 'Incomplete/mismatched trial'}
if($report.renderScalePercent -ne 75 -or $report.executionBackend -cne $Backend -or
   $report.materialEncoding -cne 'ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2) + strict ASTC 6x6 lich'){throw 'Scale/backend/encoding mismatch'}
$expected=if($Workload -ceq 'showcase-route-v1'){1838}else{600}
if($report.measuredFrames -ne $expected -or -not $report.presentedEveryFrame){throw 'Presented denominator incomplete'}
$model.status='complete';$model.processId=$pidAtStart;$model.endUtc=[DateTime]::UtcNow.ToString('o')
$model.renderScalePercent=$report.renderScalePercent;$model.internalExtent=$report.internalExtent
$model.presentationExtent=$report.presentationExtent;$model.presentMode=$report.presentMode;$model.overall=$report.overall
$model | ConvertTo-Json -Depth 9 > (Join-Path $destination 'trial.json')
Write-Output "$RunId retained: $expected presented frames, native-cycle median $($report.overall.medianMs) ms. Row/thermal admission remains separate."
