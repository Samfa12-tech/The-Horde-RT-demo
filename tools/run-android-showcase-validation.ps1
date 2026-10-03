param(
    [ValidateSet("Benchmark", "Replay", "Both")]
    [string]$Mode = "Both",
    [ValidateRange(50, 100)]
    [int]$Scale = 75,
    [string[]]$Checkpoints = @("opening", "two-enemy-combat", "worst-bend", "skylight", "green", "lich"),
    [ValidateSet("Enabled", "Disabled")]
    [string]$GpuTiming = "Enabled",
    [switch]$RequireRayQueryCompute,
    [switch]$Include100,
    [switch]$Capture,
    [string[]]$CaptureSelection = @(),
    [switch]$RtLabWorkloadComparison,
    [switch]$SkipBuild,
    [switch]$SkipInstall,
    [string]$ViewmodelCandidateDirectory = "",
    [switch]$AnatomicalPlayerMount,
    [switch]$StagedPrimaryInvestigation,
    [string]$StagedPrimaryManifest = "",
    [string]$ApkPath = "",
    [string]$ArtifactSourceRoot = "",
    [ValidateNotNullOrEmpty()]
    [string]$DeviceSerial = "R5GL219SZGK",
    [ValidateNotNullOrEmpty()]
    [string]$ExpectedDeviceModel = "SM-S948B",
    [ValidateRange(30, 300)]
    [int]$TimeoutSeconds = 120,
    [string]$OutputRoot = (Join-Path $PSScriptRoot "..\reports\android-showcase-runs")
)

$ErrorActionPreference = "Stop"
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$androidRoot = Join-Path $repoRoot "android"
if ($ApkPath -and -not $SkipBuild) { throw 'An explicit immutable ApkPath requires SkipBuild.' }
if ($ArtifactSourceRoot -and (-not $SkipBuild -or -not $ApkPath)) {
    throw 'ArtifactSourceRoot requires SkipBuild and an explicit immutable ApkPath.'
}
$artifactRepositoryRoot = if ($ArtifactSourceRoot) { [IO.Path]::GetFullPath($ArtifactSourceRoot) } else { $repoRoot }
$apk = ""
. (Join-Path $PSScriptRoot 'AndroidStagedPrimaryAdmission.ps1')
$validationTarget = Resolve-AndroidShowcaseValidationTarget -StagedPrimary ([bool]$StagedPrimaryInvestigation) `
    -SkipBuild ([bool]$SkipBuild) -ApkPath $ApkPath -ManifestPath $StagedPrimaryManifest `
    -ViewmodelDirectory $ViewmodelCandidateDirectory -AnatomicalMount ([bool]$AnatomicalPlayerMount)
$packageName = $validationTarget.package
$activityName = "$packageName/com.samfa12.hordelanternrt.MainActivity"
if ($RequireRayQueryCompute -and $StagedPrimaryInvestigation) {
    throw 'Required Compute cannot use the separate Pipeline-only staged-primary investigation.'
}
$adb = Join-Path $env:LOCALAPPDATA "Android\Sdk\platform-tools\adb.exe"
$runId = (Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '-' + [Guid]::NewGuid().ToString('N').Substring(0,8)
$outputDirectory = [IO.Path]::GetFullPath((Join-Path $OutputRoot "run-$runId"))
$repositoryPrefix = $repoRoot.TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar
$privateReportsPrefix = [IO.Path]::GetFullPath((Join-Path $repoRoot 'reports')).TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar
if ($outputDirectory.StartsWith($repositoryPrefix, [StringComparison]::OrdinalIgnoreCase) -and
    -not $outputDirectory.StartsWith($privateReportsPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Raw device logs/screenshots must stay in ignored reports or outside the source repository.'
}
$runLogStart = ''; $operationLogStart = ''; $lastLogProcessId = ''
$observedLogProcessIds = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
$gpuTimingEnabled = $GpuTiming -eq "Enabled"
$gpuTimingLabel = $GpuTiming.ToLowerInvariant()
$gpuTimingArgument = $(if ($gpuTimingEnabled) { "true" } else { "false" })
$reference60FpsMs = 1000.0 / 60.0
$reference50FpsMs = 20.0
$reference30FpsMs = 1000.0 / 30.0
$sourceCommit = (& git -C $artifactRepositoryRoot rev-parse HEAD 2>&1 | Out-String).Trim()
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($sourceCommit)) { throw "Could not resolve the source Git commit." }
$sourceDirty = -not [string]::IsNullOrWhiteSpace((& git -C $artifactRepositoryRoot status --porcelain 2>&1 | Out-String).Trim())
$runnerSourceCommit = (& git -C $repoRoot rev-parse HEAD 2>&1 | Out-String).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Could not resolve the validation runner source commit.' }
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
    "player-body-grips" = "opening"
    "player-fallback-grips" = "opening"
    "player-body-owner-feedback" = "opening"
    "player-body-downward-cut" = "opening"
    "player-body-upward-slice" = "opening"
    "glass-transport" = "skylight-chamber"
    "glass-fire-transport" = "skylight-chamber"
    "glass-tinted-transport" = "skylight-chamber"
    "glass-millimetre-closed" = "skylight-chamber"
    "glass-edge-fresnel" = "skylight-chamber"
    "pbr-sword-closeup" = "opening"
    "pbr-torch-fire" = "opening"
    "player-body-forward" = "opening"
    "player-fallback-forward" = "opening"
    "player-viewmodel-grips" = "opening"
    "player-viewmodel-forward" = "opening"
    "player-viewmodel-downward-cut" = "opening"
    "player-viewmodel-upward-slice" = "opening"
    "player-viewmodel-look-up" = "opening"
    "player-viewmodel-look-down" = "opening"
    "player-viewmodel-lantern-high" = "yellow-torch-bay"
    "player-viewmodel-lantern-low" = "yellow-torch-bay"
    "player-viewmodel-lantern-low-parry" = "yellow-torch-bay"
    "player-viewmodel-lantern-low-look-down" = "yellow-torch-bay"
    "player-viewmodel-lantern-high-look-up" = "yellow-torch-bay"
    "lantern-chest-unlock" = "finale"
    "lantern-glass-production" = "finale"
    "lantern-held-high" = "yellow-torch-bay"
    "lantern-held-low" = "yellow-torch-bay"
    "lantern-glass-transmission" = "yellow-torch-bay"
    "lantern-motion-extreme" = "yellow-torch-bay"
    "lantern-sweep-high-forward" = "yellow-torch-bay"
    "lantern-sweep-high-backward" = "yellow-torch-bay"
    "lantern-sweep-high-left" = "yellow-torch-bay"
    "lantern-sweep-high-right" = "yellow-torch-bay"
    "lantern-sweep-high-diagonal" = "yellow-torch-bay"
    "lantern-sweep-high-opposite" = "yellow-torch-bay"
    "lantern-sweep-low-forward" = "yellow-torch-bay"
    "lantern-sweep-low-backward" = "yellow-torch-bay"
    "lantern-sweep-low-left" = "yellow-torch-bay"
    "lantern-sweep-low-right" = "yellow-torch-bay"
    "lantern-sweep-high-alt-camera" = "yellow-torch-bay"
    "lantern-sweep-low-alt-camera" = "yellow-torch-bay"
    "lantern-wall-high" = "shadow-corridor"
    "lantern-wall-low" = "shadow-corridor"
    "lantern-held-look-up" = "yellow-torch-bay"
    "lantern-chest-held-high" = "finale"
}
$baselineCheckpoints = @("opening", "two-enemy-combat", "worst-bend", "skylight", "green", "lich")
$viewmodelCheckpoints = @(
    "player-viewmodel-grips", "player-viewmodel-forward",
    "player-viewmodel-downward-cut", "player-viewmodel-upward-slice",
    "player-viewmodel-look-up", "player-viewmodel-look-down",
    "player-viewmodel-lantern-high", "player-viewmodel-lantern-low",
    "player-viewmodel-lantern-low-parry", "player-viewmodel-lantern-low-look-down",
    "player-viewmodel-lantern-high-look-up")
$captureCheckpoints = @("opening", "skeleton", "worst-bend", "lantern-drop", "skylight", "yellow", "blue", "red", "green", "mirror", "lich", "finale-roof", "two-enemy-combat")
if ($CaptureSelection.Count -gt 0) { $captureCheckpoints = @($CaptureSelection) }
if ($Capture) { Assert-AndroidShowcaseCaptureSelection -Selection $captureCheckpoints -KnownZones $checkpointZones }
$combatCaptureExpectations = @{
    "player-body-downward-cut" = @{
        action = "swing-active"; animationTime = 0.5833; actionTime = 0.4033; minimumConsumedAttackSequence = 1
        stagedAttackEdges = 1; stagedSwingEvents = 1
    }
    "player-body-upward-slice" = @{
        action = "upward-active"; animationTime = 0.6167; actionTime = 0.1667; minimumConsumedAttackSequence = 2
        stagedAttackEdges = 2; stagedSwingEvents = 2
    }
}
# Both routes stage the same late active poses through the shared 60 Hz
# simulation. They differ in render ownership, not combat timing.
$combatCaptureExpectations['player-viewmodel-downward-cut'] = $combatCaptureExpectations['player-body-downward-cut']
$combatCaptureExpectations['player-viewmodel-upward-slice'] = $combatCaptureExpectations['player-body-upward-slice']
$combatCaptureExpectations['player-viewmodel-lantern-low-parry'] = @{
    action = "parry-active"; animationTime = 0.15; actionTime = 0.11
    minimumConsumedParrySequence = 1; stagedParryEdges = 1
}
$rtLabComparisonCheckpoints = @('lantern-drop', 'skylight', 'finale-roof')
$rtLabExpectedWaterQuality = 1
$timingRows = [System.Collections.Generic.List[object]]::new()
$captureRecords = [System.Collections.Generic.List[object]]::new()
$failures = [System.Collections.Generic.List[string]]::new()
$warnings = [System.Collections.Generic.List[string]]::new()
$selectedRtPipelineBundle = $null
$selectedRtPipelineBundleSerialized = $null
$requestedExecutionBackend = if ($RequireRayQueryCompute) { 'RayQueryCompute' } else { 'RayTracingPipeline' }
$backendSelectionEvidence = [ordered]@{
    requested = $requestedExecutionBackend; effective = $null; status = 'Pending'
    requireRayQueryCompute = [bool]$RequireRayQueryCompute; currentUser = $null
    observations = [Collections.Generic.List[object]]::new()
}
$initialWakefulness = ""
$automationSessionStarted = $false
$lifecycleEvidence = [ordered]@{
    requested = [bool]$Capture
    homeResumePassed = $false
    honestPresentationAfterResume = $false
    log = $null
}

function Invoke-AdbText {
    param([string[]]$Arguments, [switch]$AllowFailure)
    $scopedArguments = @("-s", $DeviceSerial) + $Arguments
    $output = (& $adb @scopedArguments 2>&1 | Out-String).TrimEnd()
    $exitCode = $LASTEXITCODE
    if (-not $AllowFailure -and $exitCode -ne 0) {
        throw "adb -s $DeviceSerial $($Arguments -join ' ') failed with exit code $exitCode`n$output"
    }
    return $output
}

function Get-ExpectedShowcaseInstanceCapacity {
    param([string]$RepositoryRoot)
    $abi = Get-Content -LiteralPath (Join-Path $RepositoryRoot 'src/vulkan/raytracing/RtSceneAbi.def') -Raw | ConvertFrom-Json
    if ($abi.schema -ne 1 -or
        ($abi.capacities.instanceMetadata -isnot [int] -and $abi.capacities.instanceMetadata -isnot [long]) -or
        $abi.capacities.instanceMetadata -lt 1 -or $abi.capacities.instanceMetadata -gt 256) {
        throw 'The checkout has no valid generated showcase instance capacity.'
    }
    return [int]$abi.capacities.instanceMetadata
}

function New-ScopedLogcatArguments {
    param([string]$StartTimestamp, [string]$ProcessId)
    if ($StartTimestamp -cnotmatch '^\d+\.\d{9}$' -or $ProcessId -cnotmatch '^[1-9]\d*$') {
        throw 'Logcat requires a precise device-time lower bound and one actual app PID.'
    }
    return @('logcat', '-d', '-b', 'main', '-b', 'crash', '-v', 'threadtime', '-T', $StartTimestamp,
        "--pid=$ProcessId", '-s', 'HordeRtProbeBridge', 'HordeLanternAudio', 'AndroidRuntime', 'libc')
}

function Start-ScopedLogWindow {
    param([switch]$NewRun)
    # Device clock avoids host/device clock skew and a decimal point prevents
    # logcat interpreting -T as a line count. No global buffers are cleared.
    $stamp = (Invoke-AdbText @('shell', 'date', '+%s.%N')).Trim()
    if ($stamp -cnotmatch '^\d+\.\d{9}$') { throw 'Device date did not supply a precise logcat timestamp.' }
    $script:operationLogStart = $stamp
    if ($NewRun) {
        $script:runLogStart = $stamp; $script:lastLogProcessId = ''
        $script:observedLogProcessIds.Clear()
    }
}

function Get-ScopedLogcat {
    param([switch]$WholeRun)
    $liveProcessId = (Invoke-AdbText @('shell', 'pidof', $packageName) -AllowFailure).Trim()
    if ($liveProcessId -cmatch '^[1-9]\d*$') {
        $script:lastLogProcessId = $liveProcessId
        $null = $script:observedLogProcessIds.Add($liveProcessId)
    } elseif ($liveProcessId) { throw 'The validation app has an ambiguous PID; no other process logs will be collected.' }
    if (-not $script:lastLogProcessId) { return '' } # Startup may not have created its process yet.
    $stamp = if ($WholeRun) { $script:runLogStart } else { $script:operationLogStart }
    $processIds = if ($WholeRun) { @($script:observedLogProcessIds) } else { @($script:lastLogProcessId) }
    return (@(foreach ($processId in $processIds) {
        Invoke-AdbText (New-ScopedLogcatArguments -StartTimestamp $stamp -ProcessId $processId)
    }) -join "`n")
}

function Wait-ForLogPattern {
    param([string]$Pattern, [string]$Description)
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        $log = Get-ScopedLogcat
        if ($RequireRayQueryCompute -and $log -match 'Required hardware RayQuery compute backend is unavailable') {
            $script:backendSelectionEvidence.status = 'Rejected'
            Get-ShowcaseCapability -Destination (Join-Path $outputDirectory 'backend-rejected-capability.json') | Out-Null
            throw 'Required hardware Compute is unavailable; no fallback evidence is accepted.'
        }
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

function Get-GpuThermalPowerLevel {
    $raw = Invoke-AdbText @("shell", "cat", "/sys/class/kgsl/kgsl-3d0/thermal_pwrlevel") -AllowFailure
    $parsed = 0
    if ([int]::TryParse($raw.Trim(), [ref]$parsed)) { return $parsed }
    return $null
}

function Get-PerformanceBand([double]$MedianMs) {
    if ($MedianMs -le $reference60FpsMs) { return "60 FPS REFERENCE OR BETTER" }
    if ($MedianMs -le $reference50FpsMs) { return "50-60 FPS REFERENCE BAND" }
    if ($MedianMs -le $reference30FpsMs) { return "30-50 FPS REFERENCE BAND" }
    return "BELOW 30 FPS REFERENCE"
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
        $state = Get-Content -LiteralPath $Destination -Raw | ConvertFrom-Json
    } catch {
        throw "Native showcase state is not valid JSON: $Destination`n$($_.Exception.Message)"
    }
    Register-ShowcaseBackendEvidence -Backend $state.executionBackend -Presented $state.presented `
        -Bundle $state.selectedRtPipelineBundle -Context $Destination
    Register-SelectedRtPipelineBundle -Bundle $state.selectedRtPipelineBundle -Context $Destination
    return $state
}

function Get-ShowcaseBackendIntentArguments {
    param([bool]$RequireCompute)
    if ($RequireCompute) { return @('--user', '0', '--ez', 'horde_require_rayquery_compute', 'true') }
    return @() # Preserve the ordinary Pipeline launch arguments.
}

function Assert-ShowcaseComputeUserZero {
    param([bool]$RequireCompute, [string]$CurrentUser)
    if ($RequireCompute -and $CurrentUser.Trim() -cne '0') {
        throw 'Required Compute validation is scoped to verified current Android user 0.'
    }
}

function Get-ShowcaseInstallArguments {
    param([bool]$RequireCompute, [string]$LocalApk)
    $arguments = @('install')
    if ($RequireCompute) { $arguments += @('--user','0') }
    return $arguments + @('-r','-t',$LocalApk)
}

function Assert-ShowcaseExecutionBackend {
    param([string]$ExpectedBackend, [string]$Backend, $Presented, $Bundle = $null)
    if ($ExpectedBackend -cnotin @('RayTracingPipeline','RayQueryCompute') -or
        $Backend -cne $ExpectedBackend -or $Presented -isnot [bool] -or -not $Presented) {
        throw "Requested backend '$ExpectedBackend' was not actually RT-presented (reported '$Backend', presented '$Presented')."
    }
    if ($null -ne $Bundle) {
        foreach ($strategy in @('opaqueFast','genericDielectric')) {
            $key = [string]$Bundle.$strategy.key
            if ([string]::IsNullOrWhiteSpace($key) -or
                ($key.StartsWith('rayquery_compute_', [StringComparison]::Ordinal) -ne ($Backend -ceq 'RayQueryCompute'))) {
                throw 'Selected shader pair does not agree with the actual execution backend.'
            }
        }
    }
}

function Register-ShowcaseBackendEvidence {
    param([string]$Backend, $Presented, $Bundle = $null, [string]$Context)
    $script:backendSelectionEvidence.effective = if ($Backend) { $Backend } else { $null }
    try {
        Assert-ShowcaseExecutionBackend -ExpectedBackend $requestedExecutionBackend -Backend $Backend -Presented $Presented -Bundle $Bundle
        $script:backendSelectionEvidence.status = 'Accepted'
    } catch {
        $script:backendSelectionEvidence.status = 'Rejected'
        throw
    } finally {
        $script:backendSelectionEvidence.observations.Add([pscustomobject]@{
            context = [IO.Path]::GetFileName($Context); executionBackend = $Backend
            presented = $Presented; status = $script:backendSelectionEvidence.status
        })
    }
}

function Get-ShowcaseCapability {
    param([string]$Destination)
    Save-PrivateFile -RemotePath 'files/reports/vulkan_capability_report.json' -Destination $Destination
    $capability = Get-Content -LiteralPath $Destination -Raw | ConvertFrom-Json
    Register-ShowcaseBackendEvidence -Backend $capability.executionBackend -Presented $capability.rtScene.presented -Context $Destination
    return $capability
}

function Register-SelectedRtPipelineBundle {
    param([Parameter(Mandatory = $true)]$Bundle,
          [Parameter(Mandatory = $true)][string]$Context)
    foreach ($strategy in @("opaqueFast", "genericDielectric")) {
        $entry = $Bundle.$strategy
        if ($null -eq $entry -or [string]::IsNullOrWhiteSpace([string]$entry.key) -or
            [string]$entry.sha256 -cnotmatch '^[0-9a-f]{64}$') {
            throw "$Context is missing exact selected $strategy key/hash identity."
        }
    }
    Assert-AndroidStagedPrimaryBundle -Bundle $Bundle -ExpectedBundle $validationTarget.expectedBundle
    $opaquePolicy = ([string]$Bundle.opaqueFast.key) -replace '_opaque_fast$', ''
    $genericPolicy = ([string]$Bundle.genericDielectric.key) -replace '_generic_dielectric$', ''
    if ($opaquePolicy -ceq [string]$Bundle.opaqueFast.key -or
        $genericPolicy -ceq [string]$Bundle.genericDielectric.key -or
        $opaquePolicy -cne $genericPolicy) {
        throw "$Context does not identify one coherent selected RT pipeline policy pair."
    }
    $serialized = $Bundle | ConvertTo-Json -Depth 4 -Compress
    if ($null -eq $script:selectedRtPipelineBundle) {
        $script:selectedRtPipelineBundle = $Bundle
        $script:selectedRtPipelineBundleSerialized = $serialized
    } elseif ($serialized -cne $script:selectedRtPipelineBundleSerialized) {
        throw "$Context selected RT pipeline bundle changed during one evidence run."
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
    param([string]$Checkpoint, [int]$RequestedScale, [switch]$Replay, [switch]$CaptureOnly,
          [int]$RtWorkload = -1)
    Start-ScopedLogWindow
    $arguments = @("shell", "am", "start", "--activity-single-top", "-n", $activityName,
                   "--ei", "horde.debug.scale", "$RequestedScale",
                   "--ez", "horde.debug.autostart", "true",
                   "--ez", "horde.debug.overlay", "false",
                   "--ez", "horde.debug.gpu_timing", $gpuTimingArgument)
    $arguments += @(Get-ShowcaseBackendIntentArguments -RequireCompute ([bool]$RequireRayQueryCompute))
    if ($RtWorkload -ge 0) {
        $arguments += @("--ez", "horde.debug.rt_lab", "true",
                        "--ei", "horde.debug.rt_workload", "$RtWorkload")
    }
    if ($Replay) {
        $arguments += @("--ez", "horde.debug.replay", "true")
    } else {
        $arguments += @("--es", "horde.debug.checkpoint", $Checkpoint)
        if ($CaptureOnly) { $arguments += @("--ez", "horde.debug.capture", "true") }
    }
    Invoke-AdbText $arguments | Out-Null
}

function Test-CheckpointPlayerOwnership {
    param([string]$Checkpoint, $State)
    # Authored checkpoint115 inspects the isolated production glass while its
    # locked chest state deliberately masks the player. It is not gameplay or
    # a held-lantern capture. Do not exempt any other checkpoint by flags alone.
    if ($Checkpoint -ceq 'lantern-glass-production') {
        return $State.rtLab.productionRewardPropsVisible -eq $true -and
               $State.rtLab.productionLanternGlassOnly -eq $true -and
               $State.dedicatedPlayerPrimaryOwnership -eq $false
    }
    return $State.dedicatedPlayerPrimaryOwnership -eq $true
}

function Invoke-CaptureCheckpoint {
    param([string]$Checkpoint, [int]$RequestedScale, [int]$Index)
    if (-not $checkpointZones.ContainsKey($Checkpoint)) { throw "Unknown capture checkpoint '$Checkpoint'." }
    Write-Host "Capturing private deterministic native-display checkpoint $Checkpoint at $RequestedScale%..."
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
    if ($combatCaptureExpectations.ContainsKey($Checkpoint)) {
        $expectedCombat = $combatCaptureExpectations[$Checkpoint]
        if ([math]::Abs([double]$state.animationTime - [double]$expectedCombat.animationTime) -gt 0.001) {
            $failures.Add("$Checkpoint capture animation time $($state.animationTime) did not retain its authoritative staged time $($expectedCombat.animationTime).")
        }
        if ($state.playerCombat.action -ne $expectedCombat.action -or
            [math]::Abs([double]$state.playerCombat.actionTime - [double]$expectedCombat.actionTime) -gt 0.001) {
            $failures.Add("$Checkpoint capture combat phase '$($state.playerCombat.action)' at $($state.playerCombat.actionTime) s did not match the staged active phase.")
        }
        if ($expectedCombat.ContainsKey('stagedParryEdges')) {
            if ([int64]$state.playerCombat.lastConsumedParrySequence -lt [int64]$expectedCombat.minimumConsumedParrySequence) {
                $failures.Add("$Checkpoint capture consumed parry sequence $($state.playerCombat.lastConsumedParrySequence), below the required monotonic minimum $($expectedCombat.minimumConsumedParrySequence).")
            }
            $stagePattern = "HORDE_PARRY_STAGE checkpoint=$escapedName staged=1 consumed_parry_edges=$($expectedCombat.stagedParryEdges) parry_success_events=0 player_damaged_events=0 player_killed_events=0 enemy_hit_events=0 action=$($expectedCombat.action) action_time=[0-9.]+ events_cleared=1"
            if ($log -notmatch $stagePattern) {
                $failures.Add("$Checkpoint did not log a real isolated parry edge without hit/damage feedback.")
            }
        } else {
            if ([int64]$state.playerCombat.lastConsumedAttackSequence -lt [int64]$expectedCombat.minimumConsumedAttackSequence) {
                $failures.Add("$Checkpoint capture consumed attack sequence $($state.playerCombat.lastConsumedAttackSequence), below the required monotonic minimum $($expectedCombat.minimumConsumedAttackSequence).")
            }
            $stagePattern = "HORDE_COMBO_STAGE checkpoint=$escapedName staged=1 consumed_attack_edges=$($expectedCombat.stagedAttackEdges) player_swing_events=$($expectedCombat.stagedSwingEvents) enemy_hit_events=0 action=$($expectedCombat.action) action_time=[0-9.]+ events_cleared=1"
            if ($log -notmatch $stagePattern) {
                $failures.Add("$Checkpoint did not log its relative attack-edge and exact-once swing-event staging contract.")
            }
        }
    } elseif ([double]$state.animationTime -ne 0.0) {
        $failures.Add("$Checkpoint capture animation time was not fixed at zero.")
    }
    if ($Checkpoint -in @("player-body-grips", "player-body-owner-feedback", "player-body-downward-cut", "player-body-upward-slice")) {
        if ($state.playerRenderRoute -ne "skinned") { $failures.Add("Skinned player capture reported route '$($state.playerRenderRoute)'.") }
        if ([int]$state.playerSkinCadenceHz -ne 60 -or [int64]$state.playerSkinUpdates -lt 1) {
            $failures.Add("Skinned player capture did not prove the selected 60 Hz CPU update route.")
        }
        if ([double]$state.playerMaxSocketErrorM -gt 0.015) { $failures.Add("Skinned player capture exceeded the 15 mm socket tolerance.") }
    }
    if ($Checkpoint -eq "player-fallback-grips" -and $state.playerRenderRoute -ne "procedural") {
        $failures.Add("Procedural player capture reported route '$($state.playerRenderRoute)'.")
    }
    $requiresModelledRoute = -not $Checkpoint.StartsWith('player-body-') -and
                            -not $Checkpoint.StartsWith('player-fallback-')
    if ($requiresModelledRoute -and (
        $state.playerRenderRoute -ne "modelled-viewmodel" -or [int]$state.playerSkinCadenceHz -ne 60 -or
        [int64]$state.playerSkinUpdates -lt 1 -or [double]$state.playerMaxSocketErrorM -gt 0.015)) {
        $failures.Add("$Checkpoint did not retain modelled-viewmodel, 60 Hz skinning and exact grip authority.")
    }
    if ($requiresModelledRoute -and ($state.playerMountProfile -cne 'AnatomicalBody' -or
        -not (Test-CheckpointPlayerOwnership $Checkpoint $state))) {
        $failures.Add("$Checkpoint lacks the requested anatomical gameplay profile or nonduplicating primary ownership.")
    }
    if ($Checkpoint.StartsWith("player-") -and [int]$state.tlasInstanceCount -ne $expectedShowcaseInstanceCapacity) {
        $failures.Add("$Checkpoint reported $($state.tlasInstanceCount) TLAS instances instead of this checkout's generated capacity $expectedShowcaseInstanceCapacity.")
    }
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
        selectedRtPipelineBundle = $state.selectedRtPipelineBundle
        requestedExecutionBackend = $requestedExecutionBackend
        effectiveExecutionBackend = $state.executionBackend
        outputRedBlueSwap = [bool]$state.outputRedBlueSwap
        animationTime = $state.animationTime
        playerCombat = $state.playerCombat
        stablePresentedFrames = $state.captureStableFrames
        presented = [bool]$state.presented
        playerRenderRoute = $state.playerRenderRoute
        playerSkinCadenceHz = $state.playerSkinCadenceHz
        playerSkinUpdates = $state.playerSkinUpdates
        playerSkinCpuAverageMs = $state.playerSkinCpuAverageMs
        playerMaxSocketErrorM = $state.playerMaxSocketErrorM
        tlasInstanceCount = $state.tlasInstanceCount
        sceneOnly = $false
        nativeDisplayCapture = $true
        privateEvidence = $true
        overlaysHidden = @("menu", "touch-actions", "HUD", "diagnostics", "developer-overlay")
        png = $image
        nativeStateFile = [IO.Path]::GetFileName($statePath)
    })
}

function Invoke-HomeResumeLifecycleCheck {
    Write-Host "Checking Android Home/resume surface recreation..."
    Start-ScopedLogWindow
    $beforeLog = Get-ScopedLogcat
    $presentationPattern = "RT frame reached Android swapchain presentation"
    $presentationCountBefore = [regex]::Matches($beforeLog, $presentationPattern).Count
    Invoke-AdbText @("shell", "input", "keyevent", "3") | Out-Null
    Start-Sleep -Milliseconds 1200
    Invoke-AdbText (@("shell", "am", "start", "--activity-single-top", "-n", $activityName) +
        @(Get-ShowcaseBackendIntentArguments -RequireCompute ([bool]$RequireRayQueryCompute))) | Out-Null
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        $resumeLog = Get-ScopedLogcat
        if ([regex]::Matches($resumeLog, $presentationPattern).Count -gt $presentationCountBefore) { break }
        Start-Sleep -Milliseconds 750
    } while ([DateTime]::UtcNow -lt $deadline)
    if ([regex]::Matches($resumeLog, $presentationPattern).Count -le $presentationCountBefore) {
        throw "Timed out waiting for honest RT presentation after Home/resume."
    }
    $resumeCapability = Get-ShowcaseCapability -Destination (Join-Path $outputDirectory 'lifecycle-home-resume-capability.json')
    $lifecycleEvidence.requestedExecutionBackend = $requestedExecutionBackend
    $lifecycleEvidence.effectiveExecutionBackend = $resumeCapability.executionBackend
    $lifecycleEvidence.homeResumePassed = $true
    $lifecycleEvidence.honestPresentationAfterResume = $true
    $lifecycleEvidence.log = "lifecycle-home-resume-logcat.txt"
    $resumeLog | Set-Content -LiteralPath (Join-Path $outputDirectory $lifecycleEvidence.log) -Encoding utf8
}

function Start-AutomationSession {
    param([int]$RequestedScale)
    Start-ScopedLogWindow
    Invoke-AdbText (@("shell", "am", "start", "-n", $activityName,
                     "--ei", "horde.debug.scale", "$RequestedScale",
                     "--ez", "horde.debug.autostart", "true",
                     "--ez", "horde.debug.gpu_timing", $gpuTimingArgument) +
        @(Get-ShowcaseBackendIntentArguments -RequireCompute ([bool]$RequireRayQueryCompute))) | Out-Null
}

function Invoke-CheckpointBenchmark {
    param([string]$Checkpoint, [int]$RequestedScale, [bool]$StandardRouteSample,
          [string]$RtLabProfile = "authored-standard", [int]$RtWorkload = -1)
    if (-not $checkpointZones.ContainsKey($Checkpoint)) {
        throw "Unknown checkpoint '$Checkpoint'."
    }
    Write-Host "Benchmarking $Checkpoint at $RequestedScale% with GPU timing $gpuTimingLabel ($RtLabProfile)..."
    Send-AutomationIntent -Checkpoint $Checkpoint -RequestedScale $RequestedScale -RtWorkload $RtWorkload
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
    $gpuThermalPowerLevel = Get-GpuThermalPowerLevel
    $performanceBand = Get-PerformanceBand -MedianMs $median
    $row = [PSCustomObject]@{
        scale = $RequestedScale
        rt_lab_profile = $RtLabProfile
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
        performance_band = $performanceBand
        standard_route_sample = $StandardRouteSample
        gpu_thermal_power_level = $gpuThermalPowerLevel
        reference_60_fps_ms = [math]::Round($reference60FpsMs, 3)
        reference_50_fps_ms = $reference50FpsMs
        reference_30_fps_ms = [math]::Round($reference30FpsMs, 3)
        # Kept so older report readers still find a classification column.
        budget_classification = $performanceBand
    }
    $timingRows.Add($row)
    if ($actualZone -ne $checkpointZones[$Checkpoint]) { $failures.Add("$Checkpoint reported zone $actualZone.") }
    if (-not $presented) { $failures.Add("$Checkpoint did not retain honest RT presentation.") }
    if ($StandardRouteSample -and ($thermal -lt 0 -or $thermal -gt 3)) {
        $message = "$Checkpoint recorded Android thermal status $thermal; retain the timing as sustained-device evidence, but do not compare it with a materially different thermal state."
        $warnings.Add($message)
        Write-Warning $message
    }
    if ($StandardRouteSample -and $null -ne $gpuThermalPowerLevel -and $gpuThermalPowerLevel -gt 0) {
        $message = "$Checkpoint recorded GPU thermal power level $gpuThermalPowerLevel; the device governor was limiting peak GPU performance."
        $warnings.Add($message)
        Write-Warning $message
    }
    $stateName = $(if ($RtLabProfile -eq "authored-standard") {
        "$Checkpoint-$RequestedScale-state.json"
    } else {
        "$RtLabProfile-$Checkpoint-$RequestedScale-state.json"
    })
    $state = Get-ShowcaseState -Destination (Join-Path $outputDirectory $stateName)
    $row | Add-Member -NotePropertyName requested_execution_backend -NotePropertyValue $requestedExecutionBackend
    $row | Add-Member -NotePropertyName effective_execution_backend -NotePropertyValue $state.executionBackend
    if ($state.status -ne "complete") { $failures.Add("$Checkpoint native state status was '$($state.status)'.") }
    if ($state.checkpoint -ne $Checkpoint) { $failures.Add("$Checkpoint native state identified '$($state.checkpoint)'.") }
    if ($state.zone -ne $checkpointZones[$Checkpoint]) { $failures.Add("$Checkpoint native state reported zone '$($state.zone)'.") }
    if (-not $state.presented) { $failures.Add("$Checkpoint native state did not retain honest RT presentation.") }
    if ($state.gpuTimingMode -ne $gpuTimingLabel) { $failures.Add("$Checkpoint native state reported GPU timing '$($state.gpuTimingMode)' instead of '$gpuTimingLabel'.") }
    if ($Checkpoint -in @("player-body-grips", "player-body-owner-feedback", "player-body-downward-cut", "player-body-upward-slice")) {
        if ($state.playerRenderRoute -ne "skinned" -or [int]$state.playerSkinCadenceHz -ne 60 -or
            [int64]$state.playerSkinUpdates -lt 1 -or [double]$state.playerMaxSocketErrorM -gt 0.015) {
            $failures.Add("Skinned player benchmark did not retain its 60 Hz route and socket contract.")
        }
    }
    if ($Checkpoint -eq "player-fallback-grips" -and $state.playerRenderRoute -ne "procedural") {
        $failures.Add("Procedural player benchmark reported route '$($state.playerRenderRoute)'.")
    }
    $requiresModelledRoute = -not $Checkpoint.StartsWith('player-body-') -and
                            -not $Checkpoint.StartsWith('player-fallback-')
    if ($requiresModelledRoute -and (
        $state.playerRenderRoute -ne "modelled-viewmodel" -or [int]$state.playerSkinCadenceHz -ne 60 -or
        [int64]$state.playerSkinUpdates -lt 1 -or [double]$state.playerMaxSocketErrorM -gt 0.015)) {
        $failures.Add("$Checkpoint benchmark did not retain modelled-viewmodel, 60 Hz skinning and exact grip authority.")
    }
    if ($requiresModelledRoute -and ($state.playerMountProfile -cne 'AnatomicalBody' -or
        -not (Test-CheckpointPlayerOwnership $Checkpoint $state))) {
        $failures.Add("$Checkpoint benchmark lacks the requested anatomical gameplay profile or nonduplicating primary ownership.")
    }
    if ($Checkpoint.StartsWith("player-") -and [int]$state.tlasInstanceCount -ne $expectedShowcaseInstanceCapacity) {
        $failures.Add("$Checkpoint benchmark reported $($state.tlasInstanceCount) TLAS instances instead of this checkout's generated capacity $expectedShowcaseInstanceCapacity.")
    }
    if ($RtWorkload -ge 0 -and [int]$state.rtLab.workloadPreset -ne $RtWorkload) {
        $failures.Add("$Checkpoint $RtLabProfile state reported workload $($state.rtLab.workloadPreset) instead of $RtWorkload.")
    }
    if ($RtWorkload -ge 0 -and
        [math]::Abs(([double]$state.renderScale * 100.0) - [double]$RequestedScale) -gt 0.01) {
        $failures.Add("$Checkpoint $RtLabProfile state reported render scale $($state.renderScale) instead of $RequestedScale%.")
    }
    if ($RtWorkload -ge 0 -and [int]$state.waterQuality -ne $rtLabExpectedWaterQuality) {
        $failures.Add("$Checkpoint $RtLabProfile state reported water quality $($state.waterQuality) instead of Mobile ($rtLabExpectedWaterQuality).")
    }
    if ($Checkpoint -eq "two-enemy-combat") {
        if ([int]$state.activeEnemyEntities -ne 2) { $failures.Add("Two-enemy checkpoint reported $($state.activeEnemyEntities) active enemy entities instead of 2.") }
        if ([int]$state.skeletonPoseBuckets -lt 1 -or [int]$state.skeletonPoseBuckets -gt 2) {
            $failures.Add("Two-enemy checkpoint reported $($state.skeletonPoseBuckets) skeleton pose buckets outside the bounded 1-2 contract.")
        }
    }
    if ([int]$state.benchmarkWindowsCompleted -ne 3) { $failures.Add("$Checkpoint native state completed $($state.benchmarkWindowsCompleted) timing windows.") }
}

$expectedShowcaseInstanceCapacity = Get-ExpectedShowcaseInstanceCapacity -RepositoryRoot $artifactRepositoryRoot
if (-not (Test-Path -LiteralPath $adb)) { throw "adb not found: $adb" }
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null

try {
    $deviceOutput = (& $adb devices 2>&1 | Out-String).TrimEnd()
    if ($LASTEXITCODE -ne 0) { throw "adb devices failed.`n$deviceOutput" }
    $devices = @($deviceOutput -split "`r?`n" | Where-Object { $_ -match "\tdevice$" })
    $target = @($devices | Where-Object { ($_ -split "\t")[0] -eq $DeviceSerial })
    if ($target.Count -ne 1) {
        throw "Approved Android device $DeviceSerial is not the one authorised target in adb devices."
    }
    $serial = $DeviceSerial
    $initialWakefulness = Invoke-AdbText @("shell", "dumpsys", "power") -AllowFailure
    $deviceModel = Invoke-AdbText @("shell", "getprop", "ro.product.model")
    if ($deviceModel.Trim() -ne $ExpectedDeviceModel) {
        throw "Approved serial $DeviceSerial reported model '$($deviceModel.Trim())'; expected $ExpectedDeviceModel."
    }
    $androidVersion = Invoke-AdbText @("shell", "getprop", "ro.build.version.release")
    if ($RequireRayQueryCompute) {
        $currentUser = (Invoke-AdbText @('shell','am','get-current-user')).Trim()
        Assert-ShowcaseComputeUserZero -RequireCompute $true -CurrentUser $currentUser
        $backendSelectionEvidence.currentUser = 0
    }
    $apiLevel = Invoke-AdbText @("shell", "getprop", "ro.build.version.sdk")
    $osBuildFingerprint = Invoke-AdbText @("shell", "getprop", "ro.build.fingerprint")
    $displaySize = Invoke-AdbText @("shell", "wm", "size")
    $displayDensity = Invoke-AdbText @("shell", "wm", "density")
    $deviceBuild = @("model=$deviceModel", "android=$androidVersion", "api=$apiLevel", "build=$osBuildFingerprint") -join "`n"
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
            $buildArguments = @('assembleDebug', '--console=plain')
            if ($ViewmodelCandidateDirectory) {
                $buildArguments += "-PhordeViewmodelCandidateDir=$([IO.Path]::GetFullPath($ViewmodelCandidateDirectory))"
            }
            if ($AnatomicalPlayerMount) { $buildArguments += '-PhordeAnatomicalPlayerMount=true' }
            & .\gradlew.bat @buildArguments 2>&1 | Tee-Object -FilePath (Join-Path $outputDirectory "gradle-build.txt")
            if ($LASTEXITCODE -ne 0) { throw "Android debug build failed." }
        } finally { Pop-Location }
    }
    $apk = if ($ApkPath) { [IO.Path]::GetFullPath($ApkPath) } else {
        & (Join-Path $PSScriptRoot 'resolve-android-apk.ps1') -AndroidRoot $androidRoot -Variant debug
    }
    if (-not (Test-Path -LiteralPath $apk)) { throw "Debug APK not found: $apk" }
    # Verify package identity BEFORE install: a stale APK must not overwrite the
    # ordinary Debug app when the separate viewmodel candidate was requested.
    $buildTools = Join-Path $env:LOCALAPPDATA 'Android\Sdk\build-tools'
    $aapt = Get-ChildItem -LiteralPath $buildTools -Directory | Sort-Object Name -Descending |
        ForEach-Object { Join-Path $_.FullName 'aapt.exe' } | Where-Object { Test-Path -LiteralPath $_ } |
        Select-Object -First 1
    if (-not $aapt) { throw 'Android aapt is required to verify the APK package before installation.' }
    $badging = (& $aapt dump badging $apk 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0 -or $badging -notmatch "package: name='([^']+)'" -or $Matches[1] -cne $packageName) {
        throw "Local APK package does not match the requested validation target $packageName."
    }
    $apkHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $apk).Hash.ToLowerInvariant()
    # This runner targets the verified Debug package only. ABI-targeted Gradle
    # builds can be testOnly; explicitly allow that developer artifact, while
    # retaining app data and the pre-install package/hash checks.
    if (-not $SkipInstall) {
        Invoke-AdbText (Get-ShowcaseInstallArguments -RequireCompute ([bool]$RequireRayQueryCompute) -LocalApk $apk) |
            Set-Content -LiteralPath (Join-Path $outputDirectory 'install.txt')
    }
    $installedApkHash = Get-InstalledApkSha256
    if ($installedApkHash -ne $apkHash) {
        throw "Installed base APK SHA-256 $installedApkHash does not match local debug APK $apkHash."
    }

    $automationSessionStarted = $true
    Invoke-AdbText @("shell", "am", "force-stop", $packageName) | Out-Null
    Start-ScopedLogWindow -NewRun
    Start-AutomationSession -RequestedScale $Scale
    $startupLog = Wait-ForLogPattern -Pattern "RT frame reached Android swapchain presentation" -Description "honest RT presentation"
    Get-ShowcaseCapability -Destination (Join-Path $outputDirectory 'startup-capability.json') | Out-Null
    if ($startupLog -notmatch "HORDE_GPU_TIMING mode=$gpuTimingLabel rt_rendering=unchanged") {
        throw "Renderer did not report the requested GPU timing mode '$gpuTimingLabel'."
    }
    $runtimePid = (Invoke-AdbText @("shell", "pidof", $packageName) -AllowFailure).Trim()
    Invoke-AdbText @("shell", "dumpsys", "meminfo", $packageName) -AllowFailure |
        Set-Content -LiteralPath (Join-Path $outputDirectory "runtime-resources-before.txt") -Encoding utf8
    if (-not [string]::IsNullOrWhiteSpace($runtimePid)) {
        Invoke-AdbText @("shell", "ps", "-T", "-p", $runtimePid) -AllowFailure |
            Set-Content -LiteralPath (Join-Path $outputDirectory "runtime-threads-before.txt") -Encoding utf8
    }
    if ($startupLog -notmatch [regex]::Escape("PBR material encoding: ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2) + strict ASTC 6x6 lich")) {
        throw "Strict ASTC environment/lich selection was not reported."
    }

    if ($Mode -in @("Benchmark", "Both")) {
        foreach ($checkpoint in $Checkpoints) {
            Invoke-CheckpointBenchmark -Checkpoint $checkpoint -RequestedScale $Scale -StandardRouteSample:($Scale -eq 75 -and $baselineCheckpoints -contains $checkpoint)
        }
        if ($Include100) {
            Invoke-CheckpointBenchmark -Checkpoint "opening" -RequestedScale 100 -StandardRouteSample:$false
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
            schema = 3
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
            runnerSourceCommit = $runnerSourceCommit
            artifactSourceRootExplicit = [bool]$ArtifactSourceRoot
            privateEvidence = $true
            expectedShowcaseInstanceCapacity = $expectedShowcaseInstanceCapacity
            selectedRtPipelineBundle = $script:selectedRtPipelineBundle
            backendSelection = $backendSelectionEvidence
            checkpointCount = $captureRecords.Count
            checkpoints = @($captureRecords)
            lifecycle = $lifecycleEvidence
        }
        if ($StagedPrimaryInvestigation) {
            $captureManifest.stagedPrimaryInvestigation = [ordered]@{
                investigationOnly = $true; manifestSha256 = $validationTarget.manifestSha256
                expectedBundle = $validationTarget.expectedBundle; publishable = $false
            }
        }
        $captureManifest | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $outputDirectory "capture-manifest.json") -Encoding utf8
    }

    if ($RtLabWorkloadComparison) {
        $comparisonOrder = @(
            @{ checkpoint = 'lantern-drop'; profiles = @(@{ name = 'authored'; workload = 1 }, @{ name = 'max'; workload = 2 }, @{ name = 'lean'; workload = 0 }) },
            @{ checkpoint = 'skylight'; profiles = @(@{ name = 'max'; workload = 2 }, @{ name = 'lean'; workload = 0 }, @{ name = 'authored'; workload = 1 }) },
            @{ checkpoint = 'finale-roof'; profiles = @(@{ name = 'lean'; workload = 0 }, @{ name = 'authored'; workload = 1 }, @{ name = 'max'; workload = 2 }) }
        )
        foreach ($pair in $comparisonOrder) {
            if ($rtLabComparisonCheckpoints -notcontains $pair.checkpoint) {
                throw "Unknown RT Lab comparison checkpoint '$($pair.checkpoint)'."
            }
            foreach ($profile in $pair.profiles) {
                Invoke-CheckpointBenchmark -Checkpoint $pair.checkpoint -RequestedScale $Scale `
                    -StandardRouteSample:$false -RtLabProfile $profile.name -RtWorkload $profile.workload
            }
        }
    }

    $finalLog = Get-ScopedLogcat -WholeRun
    $finalLog | Set-Content -LiteralPath (Join-Path $outputDirectory "logcat.txt") -Encoding utf8
    $crashPattern = "FATAL EXCEPTION|Fatal signal|renderer initialisation failed|Diagnostic surface render loop ended unexpectedly|Failed to apply requested RT render scale"
    if ($finalLog -match $crashPattern) { $failures.Add("Current logcat contains a fatal/runtime renderer failure marker.") }

    $runtimePid = (Invoke-AdbText @("shell", "pidof", $packageName) -AllowFailure).Trim()
    Invoke-AdbText @("shell", "dumpsys", "meminfo", $packageName) -AllowFailure |
        Set-Content -LiteralPath (Join-Path $outputDirectory "runtime-resources-after.txt") -Encoding utf8
    if (-not [string]::IsNullOrWhiteSpace($runtimePid)) {
        Invoke-AdbText @("shell", "ps", "-T", "-p", $runtimePid) -AllowFailure |
            Set-Content -LiteralPath (Join-Path $outputDirectory "runtime-threads-after.txt") -Encoding utf8
    }

    Save-PrivateFile -RemotePath "files/reports/vulkan_capability_report.txt" -Destination (Join-Path $outputDirectory "vulkan_capability_report.txt")
    $capabilityPath = Join-Path $outputDirectory "vulkan_capability_report.json"
    $capability = Get-ShowcaseCapability -Destination $capabilityPath
    $backendSelectionEvidence | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $outputDirectory 'backend-selection.json') -Encoding utf8
    Invoke-AdbText @("shell", "dumpsys", "package", $packageName) -AllowFailure | Set-Content -LiteralPath (Join-Path $outputDirectory "package-after-install.txt") -Encoding utf8
    Invoke-AdbText @("shell", "dumpsys", "thermalservice") -AllowFailure | Set-Content -LiteralPath (Join-Path $outputDirectory "thermal-after.txt") -Encoding utf8
    Invoke-AdbText @("shell", "dumpsys", "battery") -AllowFailure | Set-Content -LiteralPath (Join-Path $outputDirectory "battery-after.txt") -Encoding utf8
    $timingRows | Export-Csv -LiteralPath (Join-Path $outputDirectory "timing.csv") -NoTypeInformation
    $metadata = [ordered]@{
        schema = 9
        privateEvidence = $true
        evidenceScope = 'Actual validation app PID(s), selected tags, precise device-time run lower bound; raw display screenshots require private owner review.'
        logDeviceTimeStart = $runLogStart
        logProcessIds = @($observedLogProcessIds)
        expectedShowcaseInstanceCapacity = $expectedShowcaseInstanceCapacity
        runId = $runId
        mode = $Mode
        scale = $Scale
        include100 = [bool]$Include100
        rtLabWorkloadComparison = [bool]$RtLabWorkloadComparison
        rtLabComparisonCheckpoints = $(if ($RtLabWorkloadComparison) { $rtLabComparisonCheckpoints } else { @() })
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
        runnerSourceCommit = $runnerSourceCommit
        artifactSourceRootExplicit = [bool]$ArtifactSourceRoot
        selectedRtPipelineBundle = $script:selectedRtPipelineBundle
        backendSelection = $backendSelectionEvidence
        captureRequested = [bool]$Capture
        captureCheckpointCount = $captureRecords.Count
        captureManifest = $(if ($Capture) { "capture-manifest.json" } else { $null })
        lifecycle = $lifecycleEvidence
        timingMethod = "CPU wall-clock from frame start through vkQueuePresentKHR; not a Vulkan GPU timestamp"
        performanceReference = [ordered]@{
            policy = "descriptive FPS-equivalent bands; no single frame-time threshold is a product acceptance gate"
            sixtyFpsMs = [math]::Round($reference60FpsMs, 3)
            fiftyFpsMs = $reference50FpsMs
            thirtyFpsMs = [math]::Round($reference30FpsMs, 3)
            matchedRegressionInvestigationPercent = 15.0
        }
        warnings = @($warnings)
        failures = @($failures)
    }
    if ($StagedPrimaryInvestigation) {
        $metadata.stagedPrimaryInvestigation = [ordered]@{
            investigationOnly = $true; manifestSha256 = $validationTarget.manifestSha256
            expectedBundle = $validationTarget.expectedBundle; publishable = $false
        }
    }
    $metadata | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $outputDirectory "summary.json") -Encoding utf8
    $timingSummary = @($timingRows | ForEach-Object {
        "- $($_.scale)% $($_.checkpoint) [$($_.rt_lab_profile)]: $($_.median_of_window_avgs_ms) ms, $($_.budget_classification)"
    }) -join "`n"
    @(
        "# Android showcase automation run $runId"
        ""
        "- Mode: $Mode"
        "- Execution backend: requested $requestedExecutionBackend, effective $($backendSelectionEvidence.effective), selection $($backendSelectionEvidence.status)"
        "- Device: $deviceModel (Android $androidVersion / API $apiLevel)"
        "- Debug APK SHA-256: ``$apkHash``"
        "- Installed base APK SHA-256: ``$installedApkHash`` (exact match)"
        "- Source: ``$sourceCommit``$(if ($sourceDirty) { ' with a dirty worktree recorded' } else { ' from a clean worktree' })"
        "- Selected RT pipeline pair: ``$($script:selectedRtPipelineBundle.opaqueFast.key)@$($script:selectedRtPipelineBundle.opaqueFast.sha256) | $($script:selectedRtPipelineBundle.genericDielectric.key)@$($script:selectedRtPipelineBundle.genericDielectric.sha256)``"
        "- Scale: $Scale%$(if ($Include100) { ' plus report-only 100% opening' } else { '' })"
        "- GPU timestamp instrumentation: $gpuTimingLabel (RT rendering unchanged)"
        "- Evidence type: automated deterministic checkpoint/replay evidence; visual quality and perceived spatial audio remain hands-on checks."
        "- Result: $(if ($failures.Count) { 'FAIL' } else { 'PASS' })"
        ""
        "## Timing classification"
        ""
        "Descriptive references: 16.667 ms ~= 60 FPS, 20.000 ms = 50 FPS, and 33.333 ms ~= 30 FPS. Crossing a reference line is reported, not treated as an automatic product failure. Compare matched runs and investigate regressions above 15%."
        ""
        $timingSummary
        ""
        "See ``timing.csv``, ``summary.json``, ``logcat.txt``, checkpoint state JSON, screenshots (when requested), and the private Vulkan capability report in this directory."
    ) | Set-Content -LiteralPath (Join-Path $outputDirectory "validation.md") -Encoding utf8

    if ($failures.Count) { throw "Validation failed: $($failures -join ' ')" }
    Write-Host "Android showcase validation passed: $outputDirectory"
} catch {
    if ($backendSelectionEvidence.status -ceq 'Pending') { $backendSelectionEvidence.status = 'NotVerified' }
    $backendSelectionEvidence.failureReason = $_.Exception.Message
    $backendSelectionEvidence | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $outputDirectory 'backend-selection.json') -Encoding utf8
    throw
} finally {
    if ($automationSessionStarted) {
        try { Invoke-AdbText @("shell", "am", "force-stop", $packageName) -AllowFailure | Out-Null } catch {}
    }
    try {
        if ($automationSessionStarted -and $initialWakefulness -match "mWakefulness=Asleep") {
            $currentPower = Invoke-AdbText @("shell", "dumpsys", "power") -AllowFailure
            if ($currentPower -match "mWakefulness=Awake") {
                Invoke-AdbText @("shell", "input", "keyevent", "26") -AllowFailure | Out-Null
            }
        }
    } catch {}
}
