param(
    [ValidateSet("Benchmark", "Replay", "Both")]
    [string]$Mode = "Both",
    [ValidateRange(50, 100)]
    [int]$Scale = 75,
    [string[]]$Checkpoints = @("opening", "two-enemy-combat", "worst-bend", "skylight", "green", "lich"),
    [ValidateSet("Enabled", "Disabled")]
    [string]$GpuTiming = "Enabled",
    [switch]$Include100,
    [switch]$Capture,
    [switch]$SkipBuild,
    [switch]$SkipInstall,
    [ValidateRange(30, 300)]
    [int]$TimeoutSeconds = 120,
    [string]$OutputRoot = (Join-Path $PSScriptRoot "..\reports\android-showcase-runs")
)

$ErrorActionPreference = "Stop"
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$androidRoot = Join-Path $repoRoot "android"
$apk = Join-Path $androidRoot "app\build\outputs\apk\debug\app-debug.apk"
$packageName = "com.samfa12.hordelanternrt.debug"
$activityName = "$packageName/com.samfa12.hordelanternrt.MainActivity"
$adb = Join-Path $env:LOCALAPPDATA "Android\Sdk\platform-tools\adb.exe"
$runId = Get-Date -Format "yyyyMMdd-HHmmss"
$outputDirectory = [IO.Path]::GetFullPath((Join-Path $OutputRoot "run-$runId"))
$gpuTimingEnabled = $GpuTiming -eq "Enabled"
$gpuTimingLabel = $GpuTiming.ToLowerInvariant()
$gpuTimingArgument = $(if ($gpuTimingEnabled) { "true" } else { "false" })
$hardBudgetMs = 20.0
$lowHeadroomWarningMs = 18.5
$sourceCommit = (& git -C $repoRoot rev-parse HEAD 2>&1 | Out-String).Trim()
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($sourceCommit)) { throw "Could not resolve the source Git commit." }
$sourceDirty = -not [string]::IsNullOrWhiteSpace((& git -C $repoRoot status --porcelain 2>&1 | Out-String).Trim())
$raygenSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $repoRoot "src\vulkan\raytracing\MinimalRayGenShader.inc")).Hash.ToLowerInvariant()
$checkpointZones = @{
    "opening" = "opening"
    "skeleton" = "skeleton-room"
    "worst-bend" = "shadow-corridor"
    "lantern-drop" = "shadow-corridor"
    "skylight" = "skylight-chamber"
    "yellow" = "yellow-torch-bay"
    "blue" = "blue-torch-bay"
    "red" = "red-torch-bay"
    "green" = "green-torch-bay"
    "mirror" = "finale"
    "lich" = "finale"
    "finale-roof" = "finale"
    "two-enemy-combat" = "skeleton-room"
}
$baselineCheckpoints = @("opening", "two-enemy-combat", "worst-bend", "skylight", "green", "lich")
$captureCheckpoints = @("opening", "skeleton", "worst-bend", "lantern-drop", "skylight", "yellow", "blue", "red", "green", "mirror", "lich", "finale-roof", "two-enemy-combat")
$timingRows = [System.Collections.Generic.List[object]]::new()
$captureRecords = [System.Collections.Generic.List[object]]::new()
$failures = [System.Collections.Generic.List[string]]::new()
$initialWakefulness = ""
$lifecycleEvidence = [ordered]@{
    requested = [bool]$Capture
    homeResumePassed = $false
    honestPresentationAfterResume = $false
    log = $null
}

function Invoke-AdbText {
    param([string[]]$Arguments, [switch]$AllowFailure)
    $output = (& $adb @Arguments 2>&1 | Out-String).TrimEnd()
    $exitCode = $LASTEXITCODE
    if (-not $AllowFailure -and $exitCode -ne 0) {
        throw "adb $($Arguments -join ' ') failed with exit code $exitCode`n$output"
    }
    return $output
}

function Get-ScopedLogcat {
    return Invoke-AdbText @("logcat", "-d", "-v", "threadtime", "-s", "HordeRtProbeBridge", "HordeLanternAudio", "AndroidRuntime")
}

function Wait-ForLogPattern {
    param([string]$Pattern, [string]$Description)
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        $log = Get-ScopedLogcat
        if ($log -match $Pattern) { return $log }
        Start-Sleep -Milliseconds 750
    } while ([DateTime]::UtcNow -lt $deadline)
    throw "Timed out waiting for $Description."
}

function Get-ThermalStatus {
    $raw = Invoke-AdbText @("shell", "cmd", "thermalservice", "get-current-thermal-status") -AllowFailure
    $match = [regex]::Match($raw, '(\d+)\s*$')
    if ($match.Success -and $raw -notmatch 'Unknown command') {
        return [int]$match.Groups[1].Value
    }
    $raw = Invoke-AdbText @("shell", "dumpsys", "thermalservice") -AllowFailure
    $match = [regex]::Match($raw, '(?m)^Thermal Status:\s*(\d+)\s*$')
    return $(if ($match.Success) { [int]$match.Groups[1].Value } else { -1 })
}

function Get-BatteryTemperatureC {
    $raw = Invoke-AdbText @("shell", "dumpsys", "battery") -AllowFailure
    $match = [regex]::Match($raw, '(?m)^\s*temperature:\s*(\d+)')
    return $(if ($match.Success) { [math]::Round(([double]$match.Groups[1].Value) / 10.0, 1) } else { -1.0 })
}

function Save-PrivateFile {
    param([string]$RemotePath, [string]$Destination)
    $content = Invoke-AdbText @("shell", "run-as", $packageName, "cat", $RemotePath)
    $content | Set-Content -LiteralPath $Destination -Encoding utf8
}

function Get-InstalledApkSha256 {
    $packagePaths = Invoke-AdbText @("shell", "pm", "path", $packageName)
    $baseMatch = [regex]::Match($packagePaths, '(?m)^package:(.+/base\.apk)\r?$')
    if (-not $baseMatch.Success) { throw "Could not resolve the installed base APK for $packageName." }
    $remotePath = $baseMatch.Groups[1].Value.Trim()
    $temporaryApk = Join-Path $outputDirectory "installed-base.apk"
    try {
        Invoke-AdbText @("pull", $remotePath, $temporaryApk) | Out-Null
        return (Get-FileHash -Algorithm SHA256 -LiteralPath $temporaryApk).Hash.ToLowerInvariant()
    } finally {
        if (Test-Path -LiteralPath $temporaryApk) { Remove-Item -LiteralPath $temporaryApk -Force }
    }
}

function Get-ShowcaseState {
    param([string]$Destination)
    Save-PrivateFile -RemotePath "files/reports/showcase_debug_state.json" -Destination $Destination
    try {
        return Get-Content -LiteralPath $Destination -Raw | ConvertFrom-Json
    } catch {
        throw "Native showcase state is not valid JSON: $Destination`n$($_.Exception.Message)"
    }
}

function Save-Screenshot {
    param([string]$Name)
    $remote = "/data/local/tmp/horde-$runId-$Name.png"
    $destination = Join-Path $outputDirectory "$Name.png"
    Invoke-AdbText @("shell", "screencap", "-p", $remote) | Out-Null
    Invoke-AdbText @("pull", $remote, $destination) | Out-Null
    Invoke-AdbText @("shell", "rm", $remote) -AllowFailure | Out-Null
    $bytes = [IO.File]::ReadAllBytes($destination)
    if ($bytes.Length -lt 24 -or $bytes[0] -ne 0x89 -or $bytes[1] -ne 0x50 -or
        $bytes[2] -ne 0x4e -or $bytes[3] -ne 0x47 -or $bytes[4] -ne 0x0d -or
        $bytes[5] -ne 0x0a -or $bytes[6] -ne 0x1a -or $bytes[7] -ne 0x0a) {
        throw "ADB screencap did not produce a valid PNG: $destination"
    }
    $width = ([int]$bytes[16] -shl 24) -bor ([int]$bytes[17] -shl 16) -bor ([int]$bytes[18] -shl 8) -bor [int]$bytes[19]
    $height = ([int]$bytes[20] -shl 24) -bor ([int]$bytes[21] -shl 16) -bor ([int]$bytes[22] -shl 8) -bor [int]$bytes[23]
    if ($width -le 0 -or $height -le 0) { throw "ADB screencap PNG has invalid dimensions: ${width}x${height}." }
    return [PSCustomObject]@{
        file = [IO.Path]::GetFileName($destination)
        width = $width
        height = $height
        sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $destination).Hash.ToLowerInvariant()
    }
}

function Send-AutomationIntent {
    param([string]$Checkpoint, [int]$RequestedScale, [switch]$Replay, [switch]$CaptureOnly)
    $arguments = @("shell", "am", "start", "--activity-single-top", "-n", $activityName,
                   "--ei", "horde.debug.scale", "$RequestedScale",
                   "--ez", "horde.debug.autostart", "true",
                   "--ez", "horde.debug.overlay", "false",
                   "--ez", "horde.debug.gpu_timing", $gpuTimingArgument)
    if ($Replay) {
        $arguments += @("--ez", "horde.debug.replay", "true")
    } else {
        $arguments += @("--es", "horde.debug.checkpoint", $Checkpoint)
        if ($CaptureOnly) { $arguments += @("--ez", "horde.debug.capture", "true") }
    }
    Invoke-AdbText $arguments | Out-Null
}

function Invoke-CaptureCheckpoint {
    param([string]$Checkpoint, [int]$RequestedScale, [int]$Index)
    if (-not $checkpointZones.ContainsKey($Checkpoint)) { throw "Unknown capture checkpoint '$Checkpoint'." }
    Write-Host "Capturing deterministic scene-only checkpoint $Checkpoint at $RequestedScale%..."
    Send-AutomationIntent -Checkpoint $Checkpoint -RequestedScale $RequestedScale -CaptureOnly
    $escapedName = [regex]::Escape($Checkpoint)
    $log = Wait-ForLogPattern -Pattern "HORDE_CAPTURE_READY generation=\d+ checkpoint=$escapedName scale=$RequestedScale stable_frames=12 presented=1" -Description "$Checkpoint capture-ready marker"
    $readyMatches = [regex]::Matches($log, "HORDE_CAPTURE_READY generation=(\d+) checkpoint=$escapedName scale=$RequestedScale stable_frames=12 presented=1")
    if ($readyMatches.Count -lt 1) { throw "No capture-ready generation marker for $Checkpoint." }
    $generation = [int]$readyMatches[$readyMatches.Count - 1].Groups[1].Value
    $statePath = Join-Path $outputDirectory ("capture-{0:d2}-{1}-state.json" -f $Index, $Checkpoint)
    $state = Get-ShowcaseState -Destination $statePath
    if ($state.status -ne "capture-ready") { $failures.Add("$Checkpoint capture state status was '$($state.status)'.") }
    if ($state.checkpoint -ne $Checkpoint) { $failures.Add("$Checkpoint capture state identified '$($state.checkpoint)'.") }
    if ($state.zone -ne $checkpointZones[$Checkpoint]) { $failures.Add("$Checkpoint capture state reported zone '$($state.zone)'.") }
    if (-not $state.presented) { $failures.Add("$Checkpoint capture did not retain honest RT presentation.") }
    if ([int]$state.captureStableFrames -lt 12) { $failures.Add("$Checkpoint capture had only $($state.captureStableFrames) stable presented frames.") }
    if ([double]$state.animationTime -ne 0.0) { $failures.Add("$Checkpoint capture animation time was not fixed at zero.") }
    $image = Save-Screenshot ("capture-{0:d2}-{1}-{2}" -f $Index, $Checkpoint, $RequestedScale)
    $captureRecords.Add([PSCustomObject]@{
        index = $Index
        checkpoint = $Checkpoint
        expectedZone = $checkpointZones[$Checkpoint]
        zone = $state.zone
        generation = $generation
        camera = $state.player
        renderScale = $state.renderScale
        internalExtent = $state.internalExtent
        swapchainExtent = $state.swapchainExtent
        gpu = $state.gpu
        buildIdentity = $state.buildIdentity
        shaderIdentity = $state.shaderIdentity
        outputRedBlueSwap = [bool]$state.outputRedBlueSwap
        animationTime = $state.animationTime
        stablePresentedFrames = $state.captureStableFrames
        presented = [bool]$state.presented
        sceneOnly = $true
        overlaysHidden = @("menu", "touch-actions", "HUD", "diagnostics", "developer-overlay")
        png = $image
        nativeStateFile = [IO.Path]::GetFileName($statePath)
    })
}

function Invoke-HomeResumeLifecycleCheck {
    Write-Host "Checking Android Home/resume surface recreation..."
    $beforeLog = Get-ScopedLogcat
    $presentationPattern = "RT frame reached Android swapchain presentation"
    $presentationCountBefore = [regex]::Matches($beforeLog, $presentationPattern).Count
    Invoke-AdbText @("shell", "input", "keyevent", "3") | Out-Null
    Start-Sleep -Milliseconds 1200
    Invoke-AdbText @("shell", "am", "start", "--activity-single-top", "-n", $activityName) | Out-Null
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        $resumeLog = Get-ScopedLogcat
        if ([regex]::Matches($resumeLog, $presentationPattern).Count -gt $presentationCountBefore) { break }
        Start-Sleep -Milliseconds 750
    } while ([DateTime]::UtcNow -lt $deadline)
    if ([regex]::Matches($resumeLog, $presentationPattern).Count -le $presentationCountBefore) {
        throw "Timed out waiting for honest RT presentation after Home/resume."
    }
    $lifecycleEvidence.homeResumePassed = $true
    $lifecycleEvidence.honestPresentationAfterResume = $true
    $lifecycleEvidence.log = "lifecycle-home-resume-logcat.txt"
    $resumeLog | Set-Content -LiteralPath (Join-Path $outputDirectory $lifecycleEvidence.log) -Encoding utf8
}

function Start-AutomationSession {
    param([int]$RequestedScale)
    Invoke-AdbText @("shell", "am", "start", "-n", $activityName,
                     "--ei", "horde.debug.scale", "$RequestedScale",
                     "--ez", "horde.debug.autostart", "true",
                     "--ez", "horde.debug.gpu_timing", $gpuTimingArgument) | Out-Null
}

function Invoke-CheckpointBenchmark {
    param([string]$Checkpoint, [int]$RequestedScale, [bool]$EnforceBudget)
    if (-not $checkpointZones.ContainsKey($Checkpoint)) {
        throw "Unknown checkpoint '$Checkpoint'."
    }
    Write-Host "Benchmarking $Checkpoint at $RequestedScale% with GPU timing $gpuTimingLabel..."
    Send-AutomationIntent -Checkpoint $Checkpoint -RequestedScale $RequestedScale
    $escapedName = [regex]::Escape($Checkpoint)
    $log = Wait-ForLogPattern -Pattern "HORDE_BENCH complete generation=\d+ checkpoint=$escapedName scale=$RequestedScale windows=3" -Description "$Checkpoint benchmark completion"
    $beginMatches = [regex]::Matches($log, "HORDE_BENCH begin generation=(\d+) checkpoint=$escapedName scale=$RequestedScale")
    if ($beginMatches.Count -lt 1) { throw "No benchmark generation marker for $Checkpoint." }
    $generation = [int]$beginMatches[$beginMatches.Count - 1].Groups[1].Value
    $samplePattern = "HORDE_BENCH sample generation=$generation checkpoint=$escapedName scale=$RequestedScale window=(\d+) frames=120 total_ms=([0-9.]+) fence_ms=([0-9.]+) record_ms=([0-9.]+) present_ms=([0-9.]+) zone=([^ ]+) presented=(\d)"
    $samples = [regex]::Matches($log, $samplePattern)
    if ($samples.Count -ne 3) { throw "Expected exactly three 120-frame windows for $Checkpoint; found $($samples.Count)." }
    $totals = @($samples | ForEach-Object { [double]$_.Groups[2].Value })
    $sorted = @($totals | Sort-Object)
    $median = $sorted[1]
    $mean = ($totals | Measure-Object -Average).Average
    $actualZone = $samples[2].Groups[6].Value
    $presented = @($samples | ForEach-Object { $_.Groups[7].Value }) -notcontains "0"
    $thermal = Get-ThermalStatus
    $battery = Get-BatteryTemperatureC
    $budgetClassification = if (-not $EnforceBudget) {
        "REPORT ONLY"
    } elseif ($median -ge $hardBudgetMs) {
        "FAIL"
    } elseif ($median -ge $lowHeadroomWarningMs) {
        "PASS - LOW HEADROOM"
    } else {
        "PASS"
    }
    $row = [PSCustomObject]@{
        scale = $RequestedScale
        checkpoint = $Checkpoint
        expected_zone = $checkpointZones[$Checkpoint]
        actual_zone = $actualZone
        window_1_avg_ms = $totals[0]
        window_2_avg_ms = $totals[1]
        window_3_avg_ms = $totals[2]
        median_of_window_avgs_ms = [math]::Round($median, 3)
        mean_of_window_avgs_ms = [math]::Round($mean, 3)
        median_derived_fps = [math]::Round(1000.0 / $median, 3)
        thermal_status = $thermal
        battery_c = $battery
        presented = $presented
        timing_method = "cpu-present-loop"
        gpu_timing = $gpuTimingLabel
        budget_classification = $budgetClassification
        low_headroom_warning_ms = $lowHeadroomWarningMs
        hard_budget_ms = $hardBudgetMs
    }
    $timingRows.Add($row)
    if ($actualZone -ne $checkpointZones[$Checkpoint]) { $failures.Add("$Checkpoint reported zone $actualZone.") }
    if (-not $presented) { $failures.Add("$Checkpoint did not retain honest RT presentation.") }
    if ($EnforceBudget -and $median -ge $hardBudgetMs) { $failures.Add("$Checkpoint median $median ms reached or exceeded the strict below-20 ms 75% gate.") }
    elseif ($EnforceBudget -and $median -ge $lowHeadroomWarningMs) {
        Write-Warning "$Checkpoint median $median ms passed the hard gate with low headroom (warning threshold $lowHeadroomWarningMs ms)."
    }
    if ($EnforceBudget -and ($thermal -lt 0 -or $thermal -gt 3)) {
        $failures.Add("$Checkpoint thermal status $thermal was outside the required 0-3 range for the 75% gate.")
    }
    $state = Get-ShowcaseState -Destination (Join-Path $outputDirectory "$Checkpoint-$RequestedScale-state.json")
    if ($state.status -ne "complete") { $failures.Add("$Checkpoint native state status was '$($state.status)'.") }
    if ($state.checkpoint -ne $Checkpoint) { $failures.Add("$Checkpoint native state identified '$($state.checkpoint)'.") }
    if ($state.zone -ne $checkpointZones[$Checkpoint]) { $failures.Add("$Checkpoint native state reported zone '$($state.zone)'.") }
    if (-not $state.presented) { $failures.Add("$Checkpoint native state did not retain honest RT presentation.") }
    if ($state.gpuTimingMode -ne $gpuTimingLabel) { $failures.Add("$Checkpoint native state reported GPU timing '$($state.gpuTimingMode)' instead of '$gpuTimingLabel'.") }
    if ($Checkpoint -eq "two-enemy-combat") {
        if ([int]$state.activeEnemyEntities -ne 2) { $failures.Add("Two-enemy checkpoint reported $($state.activeEnemyEntities) active enemy entities instead of 2.") }
        if ([int]$state.skeletonPoseBuckets -lt 1 -or [int]$state.skeletonPoseBuckets -gt 2) {
            $failures.Add("Two-enemy checkpoint reported $($state.skeletonPoseBuckets) skeleton pose buckets outside the bounded 1-2 contract.")
        }
    }
    if ([int]$state.benchmarkWindowsCompleted -ne 3) { $failures.Add("$Checkpoint native state completed $($state.benchmarkWindowsCompleted) timing windows.") }
}

if (-not (Test-Path -LiteralPath $adb)) { throw "adb not found: $adb" }
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null

try {
    $devices = @((Invoke-AdbText @("devices")) -split "`r?`n" | Where-Object { $_ -match "\tdevice$" })
    if ($devices.Count -ne 1) { throw "Exactly one authorised Android device is required; found $($devices.Count)." }
    $serial = ($devices[0] -split "\t")[0]
    $initialWakefulness = Invoke-AdbText @("shell", "dumpsys", "power") -AllowFailure
    $deviceModel = Invoke-AdbText @("shell", "getprop", "ro.product.model")
    $androidVersion = Invoke-AdbText @("shell", "getprop", "ro.build.version.release")
    $apiLevel = Invoke-AdbText @("shell", "getprop", "ro.build.version.sdk")
    $osBuildFingerprint = Invoke-AdbText @("shell", "getprop", "ro.build.fingerprint")
    $displaySize = Invoke-AdbText @("shell", "wm", "size")
    $displayDensity = Invoke-AdbText @("shell", "wm", "density")
    $deviceBuild = Invoke-AdbText @("shell", "getprop")
    $packageBeforeInstall = Invoke-AdbText @("shell", "dumpsys", "package", $packageName) -AllowFailure
    $thermalBefore = Invoke-AdbText @("shell", "dumpsys", "thermalservice") -AllowFailure
    $batteryBefore = Invoke-AdbText @("shell", "dumpsys", "battery") -AllowFailure
    $deviceBuild | Set-Content -LiteralPath (Join-Path $outputDirectory "device-properties.txt") -Encoding utf8
    $packageBeforeInstall | Set-Content -LiteralPath (Join-Path $outputDirectory "package-before-install.txt") -Encoding utf8
    $thermalBefore | Set-Content -LiteralPath (Join-Path $outputDirectory "thermal-before.txt") -Encoding utf8
    $batteryBefore | Set-Content -LiteralPath (Join-Path $outputDirectory "battery-before.txt") -Encoding utf8

    if (-not $SkipBuild) {
        Push-Location $androidRoot
        try {
            .\gradlew.bat assembleDebug --console=plain 2>&1 | Tee-Object -FilePath (Join-Path $outputDirectory "gradle-build.txt")
            if ($LASTEXITCODE -ne 0) { throw "Android debug build failed." }
        } finally { Pop-Location }
    }
    if (-not (Test-Path -LiteralPath $apk)) { throw "Debug APK not found: $apk" }
    $apkHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $apk).Hash.ToLowerInvariant()
    if (-not $SkipInstall) { Invoke-AdbText @("install", "-r", $apk) | Set-Content -LiteralPath (Join-Path $outputDirectory "install.txt") }
    $installedApkHash = Get-InstalledApkSha256
    if ($installedApkHash -ne $apkHash) {
        throw "Installed base APK SHA-256 $installedApkHash does not match local debug APK $apkHash."
    }

    Invoke-AdbText @("logcat", "-c") | Out-Null
    Invoke-AdbText @("shell", "am", "force-stop", $packageName) | Out-Null
    Start-AutomationSession -RequestedScale $Scale
    $startupLog = Wait-ForLogPattern -Pattern "RT frame reached Android swapchain presentation" -Description "honest RT presentation"
    if ($startupLog -notmatch "HORDE_GPU_TIMING mode=$gpuTimingLabel rt_rendering=unchanged") {
        throw "Renderer did not report the requested GPU timing mode '$gpuTimingLabel'."
    }
    if ($startupLog -notmatch [regex]::Escape("PBR material encoding: ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2) + strict ASTC 6x6 lich")) {
        throw "Strict ASTC environment/lich selection was not reported."
    }

    if ($Mode -in @("Benchmark", "Both")) {
        foreach ($checkpoint in $Checkpoints) {
            Invoke-CheckpointBenchmark -Checkpoint $checkpoint -RequestedScale $Scale -EnforceBudget:($Scale -eq 75 -and $baselineCheckpoints -contains $checkpoint)
        }
        if ($Include100) {
            Invoke-CheckpointBenchmark -Checkpoint "opening" -RequestedScale 100 -EnforceBudget:$false
            Send-AutomationIntent -Checkpoint "opening" -RequestedScale $Scale
            Wait-ForLogPattern -Pattern "HORDE_BENCH begin generation=\d+ checkpoint=opening scale=$Scale" -Description "recommended scale restoration" | Out-Null
        }
    }

    if ($Mode -in @("Replay", "Both")) {
        Write-Host "Running deterministic collision-route replay..."
        Send-AutomationIntent -RequestedScale $Scale -Replay
        $replayLog = Wait-ForLogPattern -Pattern "HORDE_REPLAY complete generation=\d+ reached=13 expected=13 zone=finale" -Description "route replay completion"
        if ($replayLog -match "HORDE_REPLAY failed") { $failures.Add("Deterministic route replay reported failure.") }
        $replayState = Get-ShowcaseState -Destination (Join-Path $outputDirectory "route-replay-state.json")
        if ($replayState.status -ne "complete" -or -not $replayState.replayComplete -or $replayState.replayFailed) {
            $failures.Add("Native route replay state was not a clean completion.")
        }
        if ([int]$replayState.replayWaypointsReached -ne 13 -or $replayState.zone -ne "finale") {
            $failures.Add("Native route replay state did not finish all 13 waypoints in the finale.")
        }
        if (-not $replayState.presented) { $failures.Add("Route replay native state did not retain honest RT presentation.") }
    }

    if ($Capture) {
        for ($captureIndex = 0; $captureIndex -lt $captureCheckpoints.Count; ++$captureIndex) {
            Invoke-CaptureCheckpoint -Checkpoint $captureCheckpoints[$captureIndex] -RequestedScale $Scale -Index ($captureIndex + 1)
        }
        Invoke-HomeResumeLifecycleCheck
        $captureManifest = [ordered]@{
            schema = 1
            runId = $runId
            captureMode = "debug-only deterministic checkpoint intent plus ADB screencap"
            scale = $Scale
            device = [ordered]@{
                serial = $serial
                model = $deviceModel
                androidVersion = $androidVersion
                apiLevel = $apiLevel
                displaySize = $displaySize
                displayDensity = $displayDensity
            }
            package = $packageName
            apkSha256 = $apkHash
            installedApkSha256 = $installedApkHash
            sourceCommit = $sourceCommit
            sourceDirty = $sourceDirty
            raygenSha256 = $raygenSha256
            checkpointCount = $captureRecords.Count
            checkpoints = @($captureRecords)
            lifecycle = $lifecycleEvidence
        }
        $captureManifest | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $outputDirectory "capture-manifest.json") -Encoding utf8
    }

    $finalLog = Get-ScopedLogcat
    $finalLog | Set-Content -LiteralPath (Join-Path $outputDirectory "logcat.txt") -Encoding utf8
    $crashPattern = "FATAL EXCEPTION|Fatal signal|renderer initialisation failed|Diagnostic surface render loop ended unexpectedly|Failed to apply requested RT render scale"
    if ($finalLog -match $crashPattern) { $failures.Add("Current logcat contains a fatal/runtime renderer failure marker.") }

    Save-PrivateFile -RemotePath "files/reports/vulkan_capability_report.txt" -Destination (Join-Path $outputDirectory "vulkan_capability_report.txt")
    $capabilityPath = Join-Path $outputDirectory "vulkan_capability_report.json"
    Save-PrivateFile -RemotePath "files/reports/vulkan_capability_report.json" -Destination $capabilityPath
    $capability = Get-Content -LiteralPath $capabilityPath -Raw | ConvertFrom-Json
    Invoke-AdbText @("shell", "dumpsys", "package", $packageName) -AllowFailure | Set-Content -LiteralPath (Join-Path $outputDirectory "package-after-install.txt") -Encoding utf8
    Invoke-AdbText @("shell", "dumpsys", "thermalservice") -AllowFailure | Set-Content -LiteralPath (Join-Path $outputDirectory "thermal-after.txt") -Encoding utf8
    Invoke-AdbText @("shell", "dumpsys", "battery") -AllowFailure | Set-Content -LiteralPath (Join-Path $outputDirectory "battery-after.txt") -Encoding utf8
    $timingRows | Export-Csv -LiteralPath (Join-Path $outputDirectory "timing.csv") -NoTypeInformation
    $metadata = [ordered]@{
        schema = 6
        runId = $runId
        mode = $Mode
        scale = $Scale
        include100 = [bool]$Include100
        checkpoints = $Checkpoints
        gpuTiming = $gpuTimingLabel
        deviceSerial = $serial
        deviceModel = $deviceModel
        androidVersion = $androidVersion
        apiLevel = $apiLevel
        osBuildFingerprint = $osBuildFingerprint
        gpuName = $capability.gpuName
        gpuVendorId = $capability.vendorId
        gpuDeviceId = $capability.deviceId
        gpuDriverVersion = $capability.driverVersion
        vulkanApiVersion = $capability.vulkanApiVersion
        displaySize = $displaySize
        displayDensity = $displayDensity
        package = $packageName
        apkSha256 = $apkHash
        installedApkSha256 = $installedApkHash
        sourceCommit = $sourceCommit
        sourceDirty = $sourceDirty
        raygenSha256 = $raygenSha256
        captureRequested = [bool]$Capture
        captureCheckpointCount = $captureRecords.Count
        captureManifest = $(if ($Capture) { "capture-manifest.json" } else { $null })
        lifecycle = $lifecycleEvidence
        timingMethod = "CPU wall-clock from frame start through vkQueuePresentKHR; not a Vulkan GPU timestamp"
        performanceBudget = [ordered]@{
            lowHeadroomWarningMs = $lowHeadroomWarningMs
            hardFailureMs = $hardBudgetMs
            lowHeadroomCheckpoints = @($timingRows | Where-Object {
                $_.budget_classification -eq "PASS - LOW HEADROOM"
            } | ForEach-Object { $_.checkpoint })
        }
        failures = @($failures)
    }
    $metadata | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $outputDirectory "summary.json") -Encoding utf8
    $timingSummary = @($timingRows | ForEach-Object {
        "- $($_.scale)% $($_.checkpoint): $($_.median_of_window_avgs_ms) ms, $($_.budget_classification)"
    }) -join "`n"
    @(
        "# Android showcase automation run $runId"
        ""
        "- Mode: $Mode"
        "- Device: $deviceModel (Android $androidVersion / API $apiLevel)"
        "- Debug APK SHA-256: ``$apkHash``"
        "- Installed base APK SHA-256: ``$installedApkHash`` (exact match)"
        "- Source: ``$sourceCommit``$(if ($sourceDirty) { ' with a dirty worktree recorded' } else { ' from a clean worktree' })"
        "- Embedded raygen SHA-256: ``$raygenSha256``"
        "- Scale: $Scale%$(if ($Include100) { ' plus report-only 100% opening' } else { '' })"
        "- GPU timestamp instrumentation: $gpuTimingLabel (RT rendering unchanged)"
        "- Evidence type: automated deterministic checkpoint/replay evidence; visual quality and perceived spatial audio remain hands-on checks."
        "- Result: $(if ($failures.Count) { 'FAIL' } else { 'PASS' })"
        ""
        "## Timing classification"
        ""
        "Hard 75% gate: every required checkpoint must remain below $hardBudgetMs ms. Engineering warning: medians from $lowHeadroomWarningMs ms up to the hard gate pass with low headroom."
        ""
        $timingSummary
        ""
        "See ``timing.csv``, ``summary.json``, ``logcat.txt``, checkpoint state JSON, screenshots (when requested), and the private Vulkan capability report in this directory."
    ) | Set-Content -LiteralPath (Join-Path $outputDirectory "validation.md") -Encoding utf8

    if ($failures.Count) { throw "Validation failed: $($failures -join ' ')" }
    Write-Host "Android showcase validation passed: $outputDirectory"
} finally {
    try { Invoke-AdbText @("shell", "am", "force-stop", $packageName) -AllowFailure | Out-Null } catch {}
    try {
        if ($initialWakefulness -match "mWakefulness=Asleep") {
            $currentPower = Invoke-AdbText @("shell", "dumpsys", "power") -AllowFailure
            if ($currentPower -match "mWakefulness=Awake") {
                Invoke-AdbText @("shell", "input", "keyevent", "26") -AllowFailure | Out-Null
            }
        }
    } catch {}
}
