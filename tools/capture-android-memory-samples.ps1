#requires -Version 7.5
[CmdletBinding()]
param(
    [string]$TrialPath,
    [string]$OutputPath,
    [int]$SampleCount,
    [int]$IntervalSeconds,
    [string[]]$BoundaryLabels = @(),
    [string]$AdbPath
)

$ErrorActionPreference = 'Stop'

function ConvertFrom-AndroidMeminfo([string]$Text)
{
    $total = [regex]::Match($Text, '(?m)^\s*TOTAL PSS:\s*(\d+)\s+TOTAL RSS:\s*(\d+)')
    $native = [regex]::Match($Text, '(?m)^\s*Native Heap\s+(\d+)\s+\d+\s+\d+\s+\d+\s+(\d+)\b')
    $graphics = [regex]::Match($Text, '(?m)^\s*Graphics:\s*(\d+)\s+(\d+)\s*$')
    $egl = [regex]::Match($Text, '(?m)^\s*EGL mtrack\s+(\d+)\b')
    $gl = [regex]::Match($Text, '(?m)^\s*GL mtrack\s+(\d+)\b')
    $recognized = $total.Success -or $native.Success -or $graphics.Success -or $egl.Success -or $gl.Success
    if (-not $recognized) { return $null }
    [pscustomobject][ordered]@{
        totalPssKb = if ($total.Success) { [long]$total.Groups[1].Value } else { $null }
        totalRssKb = if ($total.Success) { [long]$total.Groups[2].Value } else { $null }
        nativeHeapPssKb = if ($native.Success) { [long]$native.Groups[1].Value } else { $null }
        nativeHeapRssKb = if ($native.Success) { [long]$native.Groups[2].Value } else { $null }
        graphicsPssKb = if ($graphics.Success) { [long]$graphics.Groups[1].Value } else { $null }
        graphicsRssKb = if ($graphics.Success) { [long]$graphics.Groups[2].Value } else { $null }
        eglMtrackPssKb = if ($egl.Success) { [long]$egl.Groups[1].Value } else { $null }
        glMtrackPssKb = if ($gl.Success) { [long]$gl.Groups[1].Value } else { $null }
    }
}

function ConvertFrom-AndroidProcStatus([string]$Text)
{
    if ([string]::IsNullOrWhiteSpace($Text)) { return $null }
    $wanted = @('VmRSS','VmHWM','RssAnon','RssFile','RssShmem','VmSwap','Threads')
    $values = [ordered]@{}
    $recognized = $false
    foreach ($name in $wanted)
    {
        $match = [regex]::Match($Text, "(?m)^$([regex]::Escape($name)):\s*(\d+)(?:\s+kB)?\s*$")
        $values[$name] = if ($match.Success) { $recognized = $true; [long]$match.Groups[1].Value } else { $null }
    }
    if (-not $recognized) { return $null }
    [pscustomobject]$values
}

function ConvertFrom-AndroidPressure([string]$Text)
{
    if ([string]::IsNullOrWhiteSpace($Text)) { return $null }
    $result = [ordered]@{}
    $recognized = $false
    foreach ($kind in @('some','full'))
    {
        $line = [regex]::Match($Text, "(?m)^$kind\s+(.+)$")
        if (-not $line.Success) { $result[$kind] = $null; continue }
        $recognized = $true
        $record = [ordered]@{}
        foreach ($field in @('avg10','avg60','avg300','total'))
        {
            $value = [regex]::Match($line.Groups[1].Value, "(?:^|\s)$field=([0-9]+(?:\.[0-9]+)?)")
            if (-not $value.Success) { $record[$field] = $null }
            elseif ($field -eq 'total') { $record[$field] = [long]$value.Groups[1].Value }
            else { $record[$field] = [double]::Parse($value.Groups[1].Value, [Globalization.CultureInfo]::InvariantCulture) }
        }
        $result[$kind] = [pscustomobject]$record
    }
    if (-not $recognized) { return $null }
    [pscustomobject]$result
}

function ConvertFrom-AndroidGlobalMeminfo([string]$Text)
{
    if ([string]::IsNullOrWhiteSpace($Text)) { return $null }
    $wanted = @('MemTotal','MemAvailable','MemFree','SwapTotal','SwapFree','Dirty','Writeback')
    $values = [ordered]@{}
    $recognized = $false
    foreach ($name in $wanted)
    {
        $match = [regex]::Match($Text, "(?m)^$([regex]::Escape($name)):\s*(\d+)(?:\s+kB)?\s*$")
        $values[$name] = if ($match.Success) { $recognized = $true; [long]$match.Groups[1].Value } else { $null }
    }
    if (-not $recognized) { return $null }
    [pscustomobject]$values
}

function Get-ProcStartTicks([string]$Text, [string]$ExpectedPid)
{
    $match = [regex]::Match($Text, '^\s*(\d+)\s+\(.*\)\s+(.*)$')
    if (-not $match.Success -or $match.Groups[1].Value -cne $ExpectedPid) { throw 'Invalid /proc/<pid>/stat identity.' }
    $fields = @($match.Groups[2].Value.Trim() -split '\s+') # starts at field 3 (state)
    if ($fields.Count -le 19 -or $fields[19] -notmatch '^\d+$') { throw 'Missing process start-time field in /proc stat.' }
    [long]$fields[19]
}

function ConvertFrom-AndroidMemoryPressure([string]$Text) { ConvertFrom-AndroidPressure $Text }

function Test-AndroidTrialIdentity($Trial)
{
    if (-not $Trial -or $Trial.runId -notmatch '^[A-Za-z0-9_-]+$' -or
        $Trial.sourceCommit -notmatch '^(?i:[0-9a-f]{7,64})$' -or
        $Trial.installedApkSha256 -notmatch '^(?i:[0-9a-f]{64})$' -or
        [string]::IsNullOrWhiteSpace([string]$Trial.workload) -or
        [string]::IsNullOrWhiteSpace([string]$Trial.serial) -or
        [string]::IsNullOrWhiteSpace([string]$Trial.deviceModel) -or
        $Trial.instrumentation -cne 'Shipping' -or $Trial.quality -cne 'Mobile' -or
        $Trial.package -notmatch '^[A-Za-z][A-Za-z0-9_]*(\.[A-Za-z][A-Za-z0-9_]*)+$')
    { throw 'Trial identity is incomplete or is not a Shipping/Mobile run.' }
    if ($Trial.requestedScalePercent -ne 75 -or $Trial.requestedBackend -cne 'RayTracingPipeline')
    { throw 'Trial identity is not the established 75% Shipping ray-tracing workload.' }
}

function Get-JsonText($Value)
{
    $Value | ConvertTo-Json -Depth 10
}

function Get-JsonLine($Value)
{
    $Value | ConvertTo-Json -Depth 10 -Compress
}

function Write-Utf8NoBom([string]$Path, [string]$Text)
{
    [IO.File]::WriteAllText($Path, $Text, [Text.UTF8Encoding]::new($false))
}

function Invoke-AndroidAdb([string]$Path, [string[]]$Arguments, [int]$TimeoutMilliseconds = 20000)
{
    $watch = [Diagnostics.Stopwatch]::StartNew()
    $processInfo = [Diagnostics.ProcessStartInfo]::new()
    $processInfo.UseShellExecute = $false
    $processInfo.CreateNoWindow = $true
    $processInfo.RedirectStandardOutput = $true
    $processInfo.RedirectStandardError = $true
    $actualArguments = @($Arguments)
    if ([IO.Path]::GetExtension($Path) -ieq '.ps1')
    {
        $pwsh = Get-Command pwsh.exe -ErrorAction SilentlyContinue
        if (-not $pwsh) { return [pscustomobject]@{ ExitCode=-1; Text=''; ErrorText='pwsh.exe is required to run the explicit PowerShell fixture.'; TimedOut=$false; ElapsedMs=0 } }
        $processInfo.FileName = $pwsh.Source
        $actualArguments = @('-NoProfile','-File',$Path) + $actualArguments
    }
    else { $processInfo.FileName = $Path }
    foreach ($argument in $actualArguments) { $processInfo.ArgumentList.Add([string]$argument) }

    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $processInfo
    try
    {
        if (-not $process.Start()) { throw 'Process.Start returned false.' }
        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit($TimeoutMilliseconds))
        {
            if (-not $process.HasExited) { try { $process.Kill() } catch { } }
            $null = $process.WaitForExit(2000)
            $stdout = if ($stdoutTask.IsCompleted) { $stdoutTask.GetAwaiter().GetResult() } else { '' }
            $stderr = if ($stderrTask.IsCompleted) { $stderrTask.GetAwaiter().GetResult() } else { '' }
            $watch.Stop()
            return [pscustomobject]@{
                ExitCode=-1; Text=$stdout; ErrorText=(($stderr + "`nTimed out after $TimeoutMilliseconds ms; owned process was terminated.").Trim())
                TimedOut=$true; ElapsedMs=[math]::Round($watch.Elapsed.TotalMilliseconds,3)
            }
        }
        $process.WaitForExit()
        $stdout = $stdoutTask.GetAwaiter().GetResult()
        $stderr = $stderrTask.GetAwaiter().GetResult()
        $watch.Stop()
        [pscustomobject]@{ ExitCode=$process.ExitCode; Text=$stdout.TrimEnd("`r","`n"); ErrorText=$stderr.TrimEnd("`r","`n"); TimedOut=$false; ElapsedMs=[math]::Round($watch.Elapsed.TotalMilliseconds,3) }
    }
    catch
    {
        $watch.Stop()
        [pscustomobject]@{ ExitCode=-1; Text=''; ErrorText=$_.Exception.Message; TimedOut=$false; ElapsedMs=[math]::Round($watch.Elapsed.TotalMilliseconds,3) }
    }
    finally { $process.Dispose() }
}

function Invoke-RecordedAdb([string]$Path, [string[]]$Arguments, [string]$DiagnosticsPath)
{
    $result = Invoke-AndroidAdb $Path $Arguments
    $record = [ordered]@{
        utc=[DateTime]::UtcNow.ToString('o'); executable=$Path; arguments=@($Arguments)
        exitCode=$result.ExitCode; timedOut=$result.TimedOut; elapsedMs=$result.ElapsedMs
        stdout=$result.Text; stderr=$result.ErrorText
    }
    [IO.File]::AppendAllText($DiagnosticsPath,(Get-JsonLine ([pscustomobject]$record))+"`n",[Text.UTF8Encoding]::new($false))
    $result
}

function Get-NearestTrialThermalSample([string]$TrialDirectory, [DateTime]$Utc)
{
    $path = Join-Path $TrialDirectory 'context-samples.jsonl'
    if (-not (Test-Path -LiteralPath $path)) { return $null }
    $best = $null
    $bestDelta = [double]::PositiveInfinity
    foreach ($line in [IO.File]::ReadLines($path))
    {
        try { $sample = $line | ConvertFrom-Json -DateKind String } catch { continue }
        try { $time = [DateTimeOffset]::Parse([string]$sample.utc).UtcDateTime } catch { continue }
        $delta = [math]::Abs(($time - $Utc).TotalSeconds)
        if ($delta -lt $bestDelta)
        {
            $bestDelta = $delta
            $best = [pscustomobject]@{
                utc = $time.ToString('o'); deltaSeconds = $delta
                batteryC = $sample.batteryC; thermalStatus = $sample.thermalStatus
                gpuThermalPowerLevel = $sample.gpuThermalPowerLevel
                alignment = 'nearest separate host context sample; not frame-aligned'
            }
        }
    }
    $best
}

function Invoke-AndroidMemoryCapture
{
    if (-not $TrialPath -or -not $OutputPath -or $SampleCount -lt 1 -or $SampleCount -gt 100 -or
        $IntervalSeconds -lt 5 -or $IntervalSeconds -gt 3600)
    { throw 'Supply TrialPath, OutputPath, SampleCount 1-100, and IntervalSeconds 5-3600.' }
    if ($BoundaryLabels.Count -ne $SampleCount -or
        @($BoundaryLabels | Where-Object { [string]::IsNullOrWhiteSpace($_) }).Count -gt 0)
    { throw 'Supply one non-empty operator-declared boundary label per scheduled sample.' }

    $resolvedTrial = (Resolve-Path -LiteralPath $TrialPath).Path
    $trial = Get-Content -LiteralPath $resolvedTrial -Raw | ConvertFrom-Json -DateKind String
    Test-AndroidTrialIdentity $trial
    $trialDirectory = Split-Path -Parent $resolvedTrial
    if (-not $AdbPath)
    {
        $adbCommand = Get-Command adb.exe -ErrorAction SilentlyContinue
        if ($adbCommand) { $AdbPath = $adbCommand.Source }
        elseif ($env:LOCALAPPDATA) { $AdbPath = Join-Path $env:LOCALAPPDATA 'Android/Sdk/platform-tools/adb.exe' }
    }
    if (-not $AdbPath -or -not (Test-Path -LiteralPath $AdbPath)) { throw 'ADB executable not found; pass -AdbPath explicitly.' }

    $resolvedOutput = [IO.Path]::GetFullPath($OutputPath)
    if (Test-Path -LiteralPath $resolvedOutput) { throw 'Refusing to overwrite existing memory-capture output.' }
    $null = New-Item -ItemType Directory -Path $resolvedOutput
    $rawDirectory = Join-Path $resolvedOutput 'raw'
    $null = New-Item -ItemType Directory -Path $rawDirectory

    $identity = [ordered]@{
        runId = $trial.runId; sourceCommit = $trial.sourceCommit
        installedApkSha256 = $trial.installedApkSha256; workload = $trial.workload
        deviceModel = $trial.deviceModel; serial = $trial.serial; package = $trial.package
        instrumentation = $trial.instrumentation; quality = $trial.quality
        scalePercent = $trial.requestedScalePercent; backend = $trial.requestedBackend
    }
    $receipt = [ordered]@{
        schemaVersion = 1; identity = $identity; boundaryLabels = @($BoundaryLabels)
        boundaryLabelsAre = 'operator-declared schedule labels; not independently observed or frame-aligned'
        requestedSampleCount = $SampleCount; intervalSeconds = $IntervalSeconds
        expectedSampleIndices = @(1..$SampleCount)
        startedUtc = [DateTime]::UtcNow.ToString('o'); completedUtc = $null
        status = 'starting'; capturedSamples = 0; capturedSampleIndices = @()
        missingSampleIndices = @(1..$SampleCount); failedAtSample = $null; sampleGaps = @()
        thermalSource = 'same trial context-samples.jsonl, nearest timestamp only; not frame-aligned'
        gpuCounters = [ordered]@{ status = 'not-collected'; gap = 'Supported vendor/driver counters were not inventoried or captured by this sidecar.' }
        measurementCaveat = 'dumpsys/proc reads add observer work; per-sample collection duration is recorded. This file does not infer FPS or DRAM traffic.'
        recordTrafficEstimate = [ordered]@{
            status = 'analytical-only-not-collected'
            note = '128-byte/pixel records distributed across 3 pages at example 1080x2235: logical 309 MB and estimated 618 MB read/write per frame. Analytical estimate only; not an observed allocation or measured DRAM traffic.'
        }
    }
    $receiptPath = Join-Path $resolvedOutput 'receipt.json'
    $samplesPath = Join-Path $resolvedOutput 'samples.jsonl'
    $adbDiagnosticsPath = Join-Path $rawDirectory 'adb-diagnostics.jsonl'
    Write-Utf8NoBom $receiptPath (Get-JsonText ([pscustomobject]$receipt))
    $failed = $false
    $activeSampleIndex = $null
    try
    {
        $modelResult = Invoke-RecordedAdb $AdbPath @('-s',$trial.serial,'shell','getprop','ro.product.model') $adbDiagnosticsPath
        if ($modelResult.ExitCode -ne 0 -or $modelResult.Text.Trim() -cne $trial.deviceModel) { throw 'Connected device model does not match trial identity.' }
        $pidResult = Invoke-RecordedAdb $AdbPath @('-s',$trial.serial,'shell','pidof',$trial.package) $adbDiagnosticsPath
        if ($pidResult.ExitCode -ne 0 -or $pidResult.Text.Trim() -notmatch '^\d+$') { throw 'Expected exactly one live process for the trial package.' }
        $expectedPid = $pidResult.Text.Trim()
        $statResult = Invoke-RecordedAdb $AdbPath @('-s',$trial.serial,'shell','cat',"/proc/$expectedPid/stat") $adbDiagnosticsPath
        if ($statResult.ExitCode -ne 0) { throw 'Cannot read initial process identity.' }
        $expectedStartTicks = Get-ProcStartTicks $statResult.Text $expectedPid
        $cmdlineResult = Invoke-RecordedAdb $AdbPath @('-s',$trial.serial,'shell','cat',"/proc/$expectedPid/cmdline") $adbDiagnosticsPath
        $cmdline = $cmdlineResult.Text -replace "`0", ' '
        if ($cmdlineResult.ExitCode -ne 0 -or $cmdline.IndexOf([string]$trial.package,[StringComparison]::Ordinal) -lt 0)
        { throw 'Live process command line does not match trial package.' }
        $receipt.processId = $expectedPid
        $receipt.processStartTicks = $expectedStartTicks
        $receipt.status = 'capturing'
        Write-Utf8NoBom $receiptPath (Get-JsonText ([pscustomobject]$receipt))

        $nextSampleAt = [Diagnostics.Stopwatch]::GetTimestamp()
        for ($index = 0; $index -lt $SampleCount; ++$index)
        {
            $activeSampleIndex = $index + 1
            if ($index -gt 0)
            {
                while ([Diagnostics.Stopwatch]::GetTimestamp() -lt $nextSampleAt) { Start-Sleep -Milliseconds 100 }
            }
            $beginUtc = [DateTime]::UtcNow
            $watch = [Diagnostics.Stopwatch]::StartNew()
            $currentTrial = Get-Content -LiteralPath $resolvedTrial -Raw | ConvertFrom-Json -DateKind String
            Test-AndroidTrialIdentity $currentTrial
            foreach ($field in @('runId','sourceCommit','installedApkSha256','workload','deviceModel','serial','package','instrumentation','quality'))
            {
                if ([string]$currentTrial.$field -cne [string]$trial.$field) { throw "Trial identity changed during capture ($field)." }
            }
            $pidNow = Invoke-RecordedAdb $AdbPath @('-s',$trial.serial,'shell','pidof',$trial.package) $adbDiagnosticsPath
            if ($pidNow.ExitCode -ne 0 -or $pidNow.Text.Trim() -cne $expectedPid) { throw 'Process ID changed or became unavailable; capture identity is invalid.' }
            $statNow = Invoke-RecordedAdb $AdbPath @('-s',$trial.serial,'shell','cat',"/proc/$expectedPid/stat") $adbDiagnosticsPath
            if ($statNow.ExitCode -ne 0 -or (Get-ProcStartTicks $statNow.Text $expectedPid) -ne $expectedStartTicks)
            { throw 'Process start-time identity changed; possible PID reuse.' }

            $captured = [ordered]@{ meminfo=$null; procStatus=$null; pressure=$null; globalMeminfo=$null }
            $sourceDiagnostics = [ordered]@{}
            $rawPaths = [ordered]@{}
            $requests = [ordered]@{
                meminfo = @('shell','dumpsys','meminfo',$expectedPid)
                procStatus = @('shell','cat',"/proc/$expectedPid/status")
                pressure = @('shell','cat','/proc/pressure/memory')
                globalMeminfo = @('shell','cat','/proc/meminfo')
            }
            foreach ($source in $requests.Keys)
            {
                $result = Invoke-RecordedAdb $AdbPath (@('-s',$trial.serial) + $requests[$source]) $adbDiagnosticsPath
                $rawPath = Join-Path $rawDirectory ("sample-{0:D3}-{1}.txt" -f ($index + 1),$source)
                $rawText = $result.Text
                if (-not [string]::IsNullOrWhiteSpace($result.ErrorText)) { $rawText += "`n--- stderr / process diagnostic ---`n" + $result.ErrorText }
                Write-Utf8NoBom $rawPath $rawText
                $rawPaths[$source] = [IO.Path]::GetFileName($rawPath)
                $sourceDiagnostics[$source] = [pscustomobject]@{ exitCode=$result.ExitCode; timedOut=$result.TimedOut; elapsedMs=$result.ElapsedMs; error=$result.ErrorText }
                if ($result.ExitCode -eq 0 -and -not $result.TimedOut)
                {
                    switch ($source)
                    {
                        'meminfo' { $captured.meminfo = ConvertFrom-AndroidMeminfo $result.Text }
                        'procStatus' { $captured.procStatus = ConvertFrom-AndroidProcStatus $result.Text }
                        'pressure' { $captured.pressure = ConvertFrom-AndroidPressure $result.Text }
                        'globalMeminfo' { $captured.globalMeminfo = ConvertFrom-AndroidGlobalMeminfo $result.Text }
                    }
                }
            }
            $pidAfterCapture = Invoke-RecordedAdb $AdbPath @('-s',$trial.serial,'shell','pidof',$trial.package) $adbDiagnosticsPath
            if ($pidAfterCapture.ExitCode -ne 0 -or $pidAfterCapture.Text.Trim() -cne $expectedPid)
            { throw 'Process ID changed or became unavailable during source capture; sample rejected.' }
            $statAfterCapture = Invoke-RecordedAdb $AdbPath @('-s',$trial.serial,'shell','cat',"/proc/$expectedPid/stat") $adbDiagnosticsPath
            if ($statAfterCapture.ExitCode -ne 0 -or (Get-ProcStartTicks $statAfterCapture.Text $expectedPid) -ne $expectedStartTicks)
            { throw 'Process start-time identity changed during source capture; sample rejected for PID reuse.' }
            $watch.Stop()
            $endUtc = [DateTime]::UtcNow
            $thermalSample = Get-NearestTrialThermalSample $trialDirectory $beginUtc
            $gaps = @($captured.Keys | Where-Object { $null -eq $captured[$_] })
            $sample = [ordered]@{
                index = $index + 1; scheduledBoundary = $BoundaryLabels[$index]
                boundarySource = 'operator-declared, not independently observed'
                startedUtc = $beginUtc.ToString('o'); completedUtc = $endUtc.ToString('o')
                collectionDurationMs = [math]::Round($watch.Elapsed.TotalMilliseconds,3)
                processId = $expectedPid; processStartTicks = $expectedStartTicks
                meminfo = $captured.meminfo; procStatus = $captured.procStatus
                systemMemoryPressure = $captured.pressure; systemMeminfo = $captured.globalMeminfo
                nearestTrialThermalContext = $thermalSample; unavailableSources = $gaps; sourceDiagnostics=$sourceDiagnostics
                gpuCounters = [ordered]@{ status = 'not-collected' }
                rawFiles = $rawPaths
            }
            [IO.File]::AppendAllText($samplesPath,(Get-JsonLine ([pscustomobject]$sample))+"`n",[Text.UTF8Encoding]::new($false))
            $receipt.capturedSamples = $index + 1
            $receipt.capturedSampleIndices += ($index + 1)
            $receipt.missingSampleIndices = @($receipt.expectedSampleIndices | Where-Object { $_ -notin $receipt.capturedSampleIndices })
            if ($gaps.Count -gt 0) { $receipt.sampleGaps += [pscustomobject]@{ sample = $index + 1; sources = $gaps } }
            $receipt.status = 'capturing'
            Write-Utf8NoBom $receiptPath (Get-JsonText ([pscustomobject]$receipt))
            $nextSampleAt = [Diagnostics.Stopwatch]::GetTimestamp() + [long]($IntervalSeconds * [Diagnostics.Stopwatch]::Frequency)
        }
        $receipt.status = 'complete'
    }
    catch
    {
        $failed = $true
        $receipt.status = 'failed'
        $receipt.failedAtSample = $activeSampleIndex
        $receipt.failure = $_.Exception.Message
    }
    finally
    {
        $receipt.completedUtc = [DateTime]::UtcNow.ToString('o')
        Write-Utf8NoBom $receiptPath (Get-JsonText ([pscustomobject]$receipt))
    }
    if ($failed) { throw "Memory capture failed; partial raw evidence retained at '$resolvedOutput'. $($receipt.failure)" }
    Write-Output "$($receipt.capturedSamples) memory samples captured for run $($trial.runId); GPU counters remain not collected."
}

if ($MyInvocation.InvocationName -ne '.')
{
    Invoke-AndroidMemoryCapture
}
