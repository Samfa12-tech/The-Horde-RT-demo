$ErrorActionPreference='Stop'
$adb='C:/Users/sam_s/AppData/Local/Android/Sdk/platform-tools/adb.exe'
$serial='R5GL219SZGK'
$package='com.samfa12.hordelanternrt.debug'
$runId='glass-geometric-live-20260930-01'
$destination=$PSScriptRoot
$remote="/sdcard/Android/data/$package/files/benchmarks/$runId"
$videoRemote='/sdcard/horde-glass-geometric-live-20260930-01.mp4'
if (Test-Path -LiteralPath (Join-Path $destination $runId)) { throw 'Existing local evidence' }
& $adb -s $serial shell test -d $remote
if ($LASTEXITCODE -eq 0) { throw 'Existing remote evidence' }
& $adb -s $serial shell test -f $videoRemote
if ($LASTEXITCODE -eq 0) { throw 'Existing recording' }
$context=[Collections.Generic.List[object]]::new()
$startUtc=[DateTime]::UtcNow.ToString('o')
$record=Start-Process -FilePath $adb -WindowStyle Hidden -PassThru -ArgumentList @(
    '-s',$serial,'shell','screenrecord','--size','720x1560','--bit-rate','4000000',
    '--time-limit','180',$videoRemote) -RedirectStandardError (Join-Path $destination 'screenrecord-stderr.txt')
& $adb -s $serial shell am start -S -W --user 0 -n "$package/com.samfa12.hordelanternrt.MainActivity" `
    -a com.samfa12.hordelanternrt.action.BENCHMARK --es horde.benchmark.run_id $runId `
    --es horde.benchmark.workload lantern-reveal-sequence-v1 *> (Join-Path $destination 'launch.log')
if ($LASTEXITCODE -ne 0) { throw 'Benchmark launch failed; preserve recording' }
$processId=(& $adb -s $serial shell pidof $package | Out-String).Trim()
if (-not $processId) { throw 'No benchmark process' }
$deadline=[DateTime]::UtcNow.AddMinutes(10)
do {
    $thermal=(& $adb -s $serial shell dumpsys thermalservice | Select-String '^Thermal Status:' | Out-String).Trim()
    $battery=(& $adb -s $serial shell dumpsys battery | Select-String '^\s+temperature:' | Select-Object -First 1 | Out-String).Trim()
    $power=(& $adb -s $serial shell cat /sys/class/kgsl/kgsl-3d0/thermal_pwrlevel | Out-String).Trim()
    $context.Add([ordered]@{utc=[DateTime]::UtcNow.ToString('o');thermal=$thermal;battery=$battery;gpuThermalPowerLevel=$power})
    & $adb -s $serial shell test -f "$remote/result.json"
    if ($LASTEXITCODE -eq 0) { break }
    if ([DateTime]::UtcNow -gt $deadline) { throw 'Benchmark export timeout' }
    $currentProcess=(& $adb -s $serial shell pidof $package | Out-String).Trim()
    if ($currentProcess -ne $processId) { throw 'Benchmark process died/changed' }
    Start-Sleep -Seconds 5
} while ($true)
& $adb -s $serial pull $remote $destination
if ($LASTEXITCODE -ne 0) { throw 'Report pull failed' }
[ordered]@{runId=$runId;workload='lantern-reveal-sequence-v1';processId=$processId;startUtc=$startUtc;endUtc=[DateTime]::UtcNow.ToString('o');samples=$context.ToArray();recordingEnabled=$true} |
    ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $destination 'context.json') -Encoding utf8NoBOM
$marker=Get-Content -LiteralPath (Join-Path $destination "$runId/result.json") -Raw | ConvertFrom-Json
if ($marker.status -ne 'complete' -or $marker.runId -ne $runId) { throw 'Invalid completion marker' }
while (-not $record.WaitForExit(10000)) {
    if ([DateTime]::UtcNow -gt $deadline) { throw 'Recording completion timeout' }
}
if ($record.ExitCode -ne 0) { throw 'Recording failed' }
& $adb -s $serial pull $videoRemote (Join-Path $destination 'live-reveal.mp4')
if ($LASTEXITCODE -ne 0) { throw 'Recording pull failed' }
& $adb -s $serial shell input keyevent 3
Write-Output 'Reports and continuous recording retained; phone returned Home.'
