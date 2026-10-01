#requires -Version 7.5
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\capture-android-memory-samples.ps1')

$passed = 0
function Assert([bool]$Condition, [string]$Message)
{
    if (-not $Condition) { throw "Android memory sample test failed: $Message" }
    ++$script:passed
}

$sampleMeminfo = @'
Applications Memory Usage (in Kilobytes):
                   Pss  Private  Private  SwapPss      Rss
  Native Heap   12000    11900        0        0    14000
   EGL mtrack    7000     7000        0        0     7000
    GL mtrack   11000    11000        0        0    11000
           TOTAL PSS:   45000            TOTAL RSS:   62000
 App Summary
                       Pss(KB)                        Rss(KB)
            Graphics:   18000                          18000
'@
$metrics = ConvertFrom-AndroidMeminfo $sampleMeminfo
Assert ($metrics.totalPssKb -eq 45000 -and $metrics.totalRssKb -eq 62000) 'PSS/RSS totals parsed in KB'
Assert ($metrics.nativeHeapPssKb -eq 12000 -and $metrics.nativeHeapRssKb -eq 14000) 'native heap stays distinct'
Assert ($metrics.graphicsPssKb -eq 18000 -and $metrics.eglMtrackPssKb -eq 7000 -and $metrics.glMtrackPssKb -eq 11000) 'Android graphics accounting categories stay separate'
$missingMetrics = ConvertFrom-AndroidMeminfo "Applications Memory Usage`nno recognized rows"
Assert ($null -eq $missingMetrics) 'missing Android metrics are unavailable, not zero'
$partialMetrics = ConvertFrom-AndroidMeminfo 'Native Heap 253325 253320 0 7 255744'
Assert ($null -eq $partialMetrics.totalPssKb -and $partialMetrics.nativeHeapPssKb -eq 253325) 'absent individual meminfo fields remain null'

# Retained historical Debug snapshot fixture only; this does not represent a new phone capture.
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$historicalDebugMeminfo = Join-Path $repoRoot 'docs/evidence/2026-10-01-fast-resize/opening/final-replay-run-20261001-022603/runtime-resources-before.txt'
$historicalMetrics = ConvertFrom-AndroidMeminfo (Get-Content -LiteralPath $historicalDebugMeminfo -Raw)
Assert ($historicalMetrics.nativeHeapPssKb -eq 253325 -and $historicalMetrics.nativeHeapRssKb -eq 255744) 'retained Debug meminfo parses without relabeling it as a new result'

$status = ConvertFrom-AndroidProcStatus @'
Name:   horde
Pid:    1234
VmRSS:  64000 kB
VmHWM:  90000 kB
RssAnon: 40000 kB
RssFile: 22000 kB
Threads: 31
'@
Assert ($status.VmRSS -eq 64000 -and $status.VmHWM -eq 90000 -and $status.Threads -eq 31) '/proc status RAM and thread fields parse'
Assert ($null -eq $status.RssShmem -and $null -eq $status.VmSwap) 'missing /proc fields remain null'
Assert ($null -eq (ConvertFrom-AndroidProcStatus 'permission denied')) 'unrecognized /proc output is an unavailable source'

$pressure = ConvertFrom-AndroidMemoryPressure "some avg10=0.10 avg60=0.20 avg300=0.30 total=400`nfull avg10=0.00 avg60=0.00 avg300=0.00 total=0"
Assert ($pressure.some.avg10 -eq 0.10 -and $pressure.some.total -eq 400 -and $pressure.full.total -eq 0) 'PSI pressure fields parse with explicit zero preserved'
$partialPressure = ConvertFrom-AndroidMemoryPressure 'some avg10=0.00 avg60=0.00 avg300=0.00 total=0'
Assert ($null -eq $partialPressure.full) 'absent full-pressure line remains null'
Assert ($null -eq (ConvertFrom-AndroidPressure 'pressure unavailable')) 'unrecognized PSI output is an unavailable source'

$slowAdb = Join-Path ([IO.Path]::GetTempPath()) ('horde-memory-slow-adb-' + [guid]::NewGuid().ToString('N') + '.ps1')
[IO.File]::WriteAllText($slowAdb,"Write-Output 'partial diagnostic'; Start-Sleep -Seconds 3",[Text.UTF8Encoding]::new($false))
try
{
    $timeoutResult = Invoke-AndroidAdb $slowAdb @() 300
    Assert ($timeoutResult.TimedOut -and $timeoutResult.ExitCode -eq -1) 'an ADB subprocess exceeding its deadline reports timeout instead of hanging'
    Assert ($timeoutResult.ElapsedMs -lt 2500 -and $timeoutResult.ErrorText -match 'Timed out after 300 ms') 'timeout is bounded and retains an explicit diagnostic'
}
finally { Remove-Item -LiteralPath $slowAdb -Force -ErrorAction SilentlyContinue }

$ticksFields = @('S') + @(1..18 | ForEach-Object { '0' }) + @('9876')
$ticksText = '1234 (com.samfa12.hordelanternrt: renderer) ' + ($ticksFields -join ' ')
Assert ((Get-ProcStartTicks $ticksText '1234') -eq 9876) '/proc stat parser handles spaces in comm and extracts start ticks'
$pidMismatchRejected = $false
try { $null = Get-ProcStartTicks $ticksText '9999' } catch { $pidMismatchRejected = $true }
Assert $pidMismatchRejected 'mismatched PID in proc stat is rejected'

$tempRoot = Join-Path ([IO.Path]::GetTempPath()) ('horde-memory-fixtures-' + [guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Path $tempRoot
try
{
    $trialPath = Join-Path $tempRoot 'trial.json'
    $trial = [ordered]@{
        runId='fixture-shipping-01';sourceCommit=('a' * 40);installedApkSha256=('b' * 64)
        workload='showcase-route-v1';deviceModel='SM-S948B';serial='FIXTURE_SERIAL'
        package='com.samfa12.hordelanternrt.benchmark';requestedScalePercent=75
        requestedBackend='RayTracingPipeline';instrumentation='Shipping';quality='Mobile'
        startUtc='2026-10-01T01:00:00Z';status='starting'
    }
    [IO.File]::WriteAllText($trialPath,($trial | ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText((Join-Path $tempRoot 'context-samples.jsonl'),('{"utc":"'+[DateTime]::UtcNow.ToString('o')+'","batteryC":31.2,"thermalStatus":1,"gpuThermalPowerLevel":2}'+"`n"),[Text.UTF8Encoding]::new($false))
    $fakeAdb = Join-Path $tempRoot 'fake-adb.ps1'
    $fakeBody = @'
$joined = $args -join ' '
$state = $env:HORDE_MEMORY_FAKE_STAT_COUNT
if (-not $state) { $state = Join-Path $env:TEMP 'horde-memory-fake-stat-count.txt' }
if ($joined -match 'shell getprop ro\.product\.model$') { 'SM-S948B'; return }
if ($joined -match 'shell pidof ') { '1234'; return }
if ($joined -match 'shell cat /proc/1234/stat$') {
    $count = 0
    if (Test-Path -LiteralPath $state) { $count = [int](Get-Content -LiteralPath $state -Raw) }
    ++$count
    [IO.File]::WriteAllText($state,[string]$count)
    $ticks = if ($env:HORDE_MEMORY_FAKE_PID_REUSE -eq '1' -and $count -ge 3) { 9877 } else { 9876 }
    $tail = @('S') + @(1..18 | ForEach-Object { '0' }) + @([string]$ticks)
    ('1234 (com.samfa12.hordelanternrt: benchmark) ' + ($tail -join ' '))
    return
}
if ($joined -match 'shell cat /proc/1234/cmdline$') { 'com.samfa12.hordelanternrt.benchmark'; return }
if ($joined -match 'shell dumpsys meminfo 1234$') {
    if ($env:HORDE_MEMORY_FAKE_MISSING_MEMINFO -eq '1') { exit 9 }
    '  Native Heap 12000 11900 0 0 14000'
    '  EGL mtrack 7000 7000 0 0 7000'
    '  GL mtrack 11000 11000 0 0 11000'
    '  TOTAL PSS: 45000 TOTAL RSS: 62000'
    '  Graphics: 18000 18000'
    return
}
if ($joined -match 'shell cat /proc/1234/status$') { "VmRSS: 64000 kB`nVmHWM: 90000 kB`nThreads: 31"; return }
if ($joined -match 'shell cat /proc/pressure/memory$') { 'some avg10=0.10 avg60=0.20 avg300=0.30 total=400'; return }
if ($joined -match 'shell cat /proc/meminfo$') { "MemTotal: 8000000 kB`nMemAvailable: 2000000 kB`nSwapFree: 1000 kB"; return }
exit 9
'@
    [IO.File]::WriteAllText($fakeAdb,$fakeBody,[Text.UTF8Encoding]::new($false))

    $okOutput = Join-Path $tempRoot 'capture-ok'
    $env:HORDE_MEMORY_FAKE_STAT_COUNT = Join-Path $tempRoot 'stat-ok.txt'
    Remove-Item -LiteralPath $env:HORDE_MEMORY_FAKE_STAT_COUNT -ErrorAction SilentlyContinue
    & (Join-Path $PSScriptRoot '..\capture-android-memory-samples.ps1') -TrialPath $trialPath -OutputPath $okOutput -SampleCount 2 -IntervalSeconds 5 -BoundaryLabels @('opening-start','lantern-heavy-start') -AdbPath $fakeAdb | Out-Null
    $receipt = Get-Content (Join-Path $okOutput 'receipt.json') -Raw | ConvertFrom-Json
    $sampleLines = [IO.File]::ReadAllLines((Join-Path $okOutput 'samples.jsonl'))
    $samples = @($sampleLines | ForEach-Object { $_ | ConvertFrom-Json })
    Assert ($receipt.status -eq 'complete' -and $receipt.capturedSamples -eq 2 -and $receipt.identity.installedApkSha256 -eq ('b' * 64)) 'capture receipt preserves exact run/artifact identity'
    Assert ($receipt.capturedSampleIndices.Count -eq 2 -and $receipt.missingSampleIndices.Count -eq 0) 'completed schedule has no missing sample indices'
    Assert ($sampleLines.Count -eq 2 -and $samples.Count -eq 2 -and $samples[0].index -eq 1 -and $samples[1].index -eq 2) 'samples.jsonl contains exactly two independently parseable single-line records'
    Assert ($samples[0].scheduledBoundary -eq 'opening-start' -and $samples[1].scheduledBoundary -eq 'lantern-heavy-start') 'sparse sample labels stay explicitly scheduled'
    $sample = $samples[0]
    Assert ($sample.processId -eq '1234' -and $sample.processStartTicks -eq 9876 -and $sample.scheduledBoundary -eq 'opening-start') 'sample binds PID lifetime and caller-declared boundary'
    Assert ($sample.meminfo.totalPssKb -eq 45000 -and $sample.systemMemoryPressure.some.total -eq 400) 'sample contains RAM and pressure, not GPU traffic metrics'
    Assert ($sample.gpuCounters.status -eq 'not-collected' -and $receipt.recordTrafficEstimate.status -eq 'analytical-only-not-collected' -and $receipt.recordTrafficEstimate.note -match '128-byte/pixel.*3 pages.*1080x2235') 'GPU counters and labeled analytical estimate remain explicitly unmeasured'
    Assert ($sample.collectionDurationMs -ge 0 -and $null -ne $sample.nearestTrialThermalContext.thermalStatus) 'sampler overhead is timestamped and thermal context is separately joined'
    Assert (Test-Path -LiteralPath (Join-Path $okOutput 'raw/sample-001-meminfo.txt')) 'raw meminfo is retained'
    $adbLogLines = [IO.File]::ReadAllLines((Join-Path $okOutput 'raw/adb-diagnostics.jsonl'))
    $adbRecords = @($adbLogLines | ForEach-Object { $_ | ConvertFrom-Json })
    Assert ($adbRecords.Count -ge 10 -and $adbRecords[0].arguments -contains 'getprop' -and $adbRecords[0].timedOut -eq $false) 'all bounded identity and source subprocesses leave compact raw diagnostics'
    Assert ($sample.sourceDiagnostics.meminfo.exitCode -eq 0 -and $sample.sourceDiagnostics.meminfo.timedOut -eq $false) 'successful per-source status is explicit in the sample'

    $overwriteRejected = $false
    try { & (Join-Path $PSScriptRoot '..\capture-android-memory-samples.ps1') -TrialPath $trialPath -OutputPath $okOutput -SampleCount 1 -IntervalSeconds 5 -BoundaryLabels @('opening-start') -AdbPath $fakeAdb | Out-Null }
    catch { $overwriteRejected = $_.Exception.Message -match 'overwrite' }
    Assert $overwriteRejected 'existing output cannot be overwritten'

    $badScheduleRejected = $false
    try { & (Join-Path $PSScriptRoot '..\capture-android-memory-samples.ps1') -TrialPath $trialPath -OutputPath (Join-Path $tempRoot 'bad-schedule') -SampleCount 2 -IntervalSeconds 4 -BoundaryLabels @('opening','lantern-heavy') -AdbPath $fakeAdb | Out-Null }
    catch { $badScheduleRejected = $_.Exception.Message -match 'IntervalSeconds' }
    Assert $badScheduleRejected 'sub-five-second cadence is rejected'

    $gapOutput = Join-Path $tempRoot 'capture-gap'
    $env:HORDE_MEMORY_FAKE_STAT_COUNT = Join-Path $tempRoot 'stat-gap.txt'
    $env:HORDE_MEMORY_FAKE_MISSING_MEMINFO = '1'
    & (Join-Path $PSScriptRoot '..\capture-android-memory-samples.ps1') -TrialPath $trialPath -OutputPath $gapOutput -SampleCount 1 -IntervalSeconds 5 -BoundaryLabels @('lantern-heavy') -AdbPath $fakeAdb | Out-Null
    $gapSample = Get-Content (Join-Path $gapOutput 'samples.jsonl') -Raw | ConvertFrom-Json
    Assert ($null -eq $gapSample.meminfo -and $gapSample.unavailableSources -contains 'meminfo') 'unreadable metrics are null and logged as a sample gap'
    Assert ($gapSample.sourceDiagnostics.meminfo.exitCode -eq 9 -and -not $gapSample.sourceDiagnostics.meminfo.timedOut) 'nonzero source command is recorded as a gap, not a zero-valued metric'
    Assert (Test-Path -LiteralPath (Join-Path $gapOutput 'raw/sample-001-meminfo.txt')) 'failed metric raw response is retained'
    Remove-Item Env:HORDE_MEMORY_FAKE_MISSING_MEMINFO -ErrorAction SilentlyContinue

    $reuseOutput = Join-Path $tempRoot 'capture-pid-reuse'
    $env:HORDE_MEMORY_FAKE_STAT_COUNT = Join-Path $tempRoot 'stat-reuse.txt'
    $env:HORDE_MEMORY_FAKE_PID_REUSE = '1'
    $reuseRejected = $false
    try { & (Join-Path $PSScriptRoot '..\capture-android-memory-samples.ps1') -TrialPath $trialPath -OutputPath $reuseOutput -SampleCount 1 -IntervalSeconds 5 -BoundaryLabels @('opening-end') -AdbPath $fakeAdb | Out-Null }
    catch { $reuseRejected = $_.Exception.Message -match 'PID reuse' }
    $reuseReceipt = Get-Content (Join-Path $reuseOutput 'receipt.json') -Raw | ConvertFrom-Json
    Assert ($reuseRejected -and $reuseReceipt.status -eq 'failed' -and $reuseReceipt.capturedSamples -eq 0 -and $reuseReceipt.failedAtSample -eq 1 -and $reuseReceipt.missingSampleIndices -contains 1) 'PID reuse invalidates capture and preserves exact missing sample index'
    Assert (Test-Path -LiteralPath (Join-Path $reuseOutput 'raw/sample-001-meminfo.txt')) 'raw source files remain retained when post-capture identity check rejects the sample'
}
finally
{
    Remove-Item Env:HORDE_MEMORY_FAKE_STAT_COUNT -ErrorAction SilentlyContinue
    Remove-Item Env:HORDE_MEMORY_FAKE_MISSING_MEMINFO -ErrorAction SilentlyContinue
    Remove-Item Env:HORDE_MEMORY_FAKE_PID_REUSE -ErrorAction SilentlyContinue
    $resolvedTemp = [IO.Path]::GetFullPath($tempRoot)
    $resolvedBase = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    if ($resolvedTemp.StartsWith($resolvedBase,[StringComparison]::OrdinalIgnoreCase))
    { Remove-Item -LiteralPath $resolvedTemp -Recurse -Force }
}

Write-Output "Android memory sampler host fixtures passed: $passed assertions; no device commands executed."
