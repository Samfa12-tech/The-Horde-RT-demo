param(
    [ValidateRange(50, 100)]
    [int]$Scale = 75,
    [ValidateRange(30, 300)]
    [int]$TimeoutSeconds = 150,
    [switch]$SkipInstall,
    [string]$OutputRoot = (Join-Path $PSScriptRoot "..\reports\android-vitality-runs")
)

$ErrorActionPreference = "Stop"
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$apk = Join-Path $repoRoot "android\app\build\outputs\apk\debug\app-debug.apk"
$packageName = "com.samfa12.hordelanternrt.debug"
$activityName = "$packageName/com.samfa12.hordelanternrt.MainActivity"
$retryAction = "com.samfa12.hordelanternrt.DEBUG_RETRY_ENCOUNTER"
$adb = Join-Path $env:LOCALAPPDATA "Android\Sdk\platform-tools\adb.exe"
$runId = Get-Date -Format "yyyyMMdd-HHmmss"
$outputDirectory = [IO.Path]::GetFullPath((Join-Path $OutputRoot "run-$runId"))
$results = [System.Collections.Generic.List[object]]::new()
$failures = [System.Collections.Generic.List[string]]::new()

function Invoke-AdbText {
    param([string[]]$Arguments, [switch]$AllowFailure)
    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = (& $adb @Arguments 2>&1 | Out-String).TrimEnd()
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
    if (-not $AllowFailure -and $exitCode -ne 0) {
        throw "adb $($Arguments -join ' ') failed with exit code $exitCode`n$output"
    }
    return $output
}

function Get-ScopedLogcat {
    return Invoke-AdbText @(
        "logcat", "-d", "-v", "threadtime", "-s",
        "HordeRtProbeBridge", "HordeLanternAudio", "AndroidRuntime"
    )
}
function Get-Sha256 {
    param([string]$Path)
    $stream = [IO.File]::OpenRead($Path)
    try {
        $sha256 = [Security.Cryptography.SHA256]::Create()
        try {
            return (($sha256.ComputeHash($stream) | ForEach-Object {
                $_.ToString("x2")
            }) -join "")
        } finally {
            $sha256.Dispose()
        }
    } finally {
        $stream.Dispose()
    }
}

function Wait-ForLogPattern {
    param([string]$Pattern, [string]$Description)
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        $log = Get-ScopedLogcat
        if ($log -match $Pattern) {
            return $log
        }
        Start-Sleep -Milliseconds 750
    } while ([DateTime]::UtcNow -lt $deadline)
    throw "Timed out waiting for $Description."
}

function Save-UiHierarchy {
    param([string]$Destination)
    $remote = "/sdcard/horde-vitality-$runId.xml"
    Invoke-AdbText @("shell", "uiautomator", "dump", "--compressed", $remote) | Out-Null
    Invoke-AdbText @("pull", $remote, $Destination) | Out-Null
    Invoke-AdbText @("shell", "rm", $remote) -AllowFailure | Out-Null
    return [IO.File]::ReadAllText($Destination)
}

function Wait-ForUi {
    param(
        [scriptblock]$Predicate,
        [string]$Description,
        [string]$Destination
    )
    $pollPath = Join-Path $outputDirectory "poll-ui.xml"
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        try {
            $xml = Save-UiHierarchy -Destination $pollPath
        } catch {
            # UIAutomator can briefly lose the root while the surface or modal changes.
            Write-Warning "UI hierarchy sample failed: $($_.Exception.Message)"
            Start-Sleep -Milliseconds 1000
            continue
        }
        if (& $Predicate $xml) {
            Copy-Item -LiteralPath $pollPath -Destination $Destination -Force
            return $xml
        }
        Start-Sleep -Milliseconds 1000
    } while ([DateTime]::UtcNow -lt $deadline)
    throw "Timed out waiting for $Description."
}

function Save-Screenshot {
    param([string]$Destination)
    $remote = "/data/local/tmp/horde-vitality-$runId.png"
    Invoke-AdbText @("shell", "screencap", "-p", $remote) | Out-Null
    Invoke-AdbText @("pull", $remote, $Destination) | Out-Null
    Invoke-AdbText @("shell", "rm", $remote) -AllowFailure | Out-Null
    $bytes = [IO.File]::ReadAllBytes($Destination)
    if ($bytes.Length -lt 24 -or
        $bytes[0] -ne 0x89 -or $bytes[1] -ne 0x50 -or
        $bytes[2] -ne 0x4e -or $bytes[3] -ne 0x47) {
        throw "ADB screencap did not produce a valid PNG: $Destination"
    }
    return Get-Sha256 -Path $Destination
}

if (-not (Test-Path -LiteralPath $adb -PathType Leaf)) {
    throw "adb was not found at $adb"
}
if (-not (Test-Path -LiteralPath $apk -PathType Leaf)) {
    throw "Debug APK was not found at $apk"
}
if ((Invoke-AdbText @("get-state")).Trim() -ne "device") {
    throw "No authorized Android device is ready."
}

New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
if (-not $SkipInstall) {
    Write-Host "Installing the exact Debug APK under test..."
    Invoke-AdbText @("install", "-r", $apk) | Out-Null
}

$deviceModel = (Invoke-AdbText @("shell", "getprop", "ro.product.model")).Trim()
$deviceCode = (Invoke-AdbText @("shell", "getprop", "ro.product.device")).Trim()
$androidVersion = (Invoke-AdbText @("shell", "getprop", "ro.build.version.release")).Trim()
$apiLevel = (Invoke-AdbText @("shell", "getprop", "ro.build.version.sdk")).Trim()
$apkHash = Get-Sha256 -Path $apk
$encounters = @(
    [PSCustomObject]@{
        name = "skeleton"
        launchCheckpoint = "skeleton"
        retryCheckpoint = "opening"
    },
    [PSCustomObject]@{
        name = "lich"
        launchCheckpoint = "mirror"
        retryCheckpoint = "mirror"
    }
)

foreach ($encounter in $encounters) {
    Write-Host "Validating real $($encounter.name) damage, death UI, and retry..."
    try {
        Invoke-AdbText @("shell", "am", "force-stop", $packageName) | Out-Null
        Invoke-AdbText @("logcat", "-c") | Out-Null
        Invoke-AdbText @(
            "shell", "am", "start", "-n", $activityName,
            "--ei", "horde.debug.scale", "$Scale",
            "--ez", "horde.debug.autostart", "true",
            "--ez", "horde.debug.overlay", "false",
            "--es", "horde.debug.checkpoint", $encounter.launchCheckpoint
        ) | Out-Null

        $escapedLaunch = [regex]::Escape($encounter.launchCheckpoint)
        Wait-ForLogPattern `
            -Pattern "HORDE_BENCH complete generation=\d+ checkpoint=$escapedLaunch scale=$Scale windows=3" `
            -Description "$($encounter.name) benchmark completion" | Out-Null

        $deadUiPath = Join-Path $outputDirectory "$($encounter.name)-dead-ui.xml"
        $deadXml = Wait-ForUi `
            -Predicate { param($xml) $xml -match "YOU FELL" } `
            -Description "$($encounter.name) death UI" `
            -Destination $deadUiPath
        $deathActionsPresent =
            $deadXml -match "RETRY ENCOUNTER" -and
            $deadXml -match "RESTART ROUTE" -and
            $deadXml -match "QUIT DEMO"
        if (-not $deathActionsPresent) {
            throw "$($encounter.name) death UI omitted one or more required actions."
        }

        $deadScreenshot = Join-Path $outputDirectory "$($encounter.name)-dead.png"
        $deadHash = Save-Screenshot -Destination $deadScreenshot
        Invoke-AdbText @(
            "shell", "am", "broadcast", "-a", $retryAction, "-p", $packageName
        ) | Out-Null

        $escapedRetry = [regex]::Escape($encounter.retryCheckpoint)
        $retryLog = Wait-ForLogPattern `
            -Pattern "HORDE_PLAYER retry checkpoint=$escapedRetry" `
            -Description "$($encounter.name) native retry"
        $javaAccepted = $retryLog -match "Accepted debug encounter-retry broadcast\."
        if (-not $javaAccepted) {
            throw "$($encounter.name) Java retry receiver acceptance marker is missing."
        }

        $retryUiPath = Join-Path $outputDirectory "$($encounter.name)-retry-ui.xml"
        $retryXml = Wait-ForUi `
            -Predicate {
                param($xml)
                $xml -notmatch "YOU FELL" -and
                ($xml -match "Vitality 3 of 3" -or $xml -match "VITALITY  3 / 3")
            } `
            -Description "$($encounter.name) 3/3 vitality restoration" `
            -Destination $retryUiPath
        $retryScreenshot = Join-Path $outputDirectory "$($encounter.name)-retry.png"
        $retryHash = Save-Screenshot -Destination $retryScreenshot
        $finalLog = Get-ScopedLogcat
        [IO.File]::WriteAllText(
            (Join-Path $outputDirectory "$($encounter.name)-logcat.txt"),
            $finalLog + [Environment]::NewLine
        )

        $results.Add([ordered]@{
            encounter = $encounter.name
            damageSource = "real enemy AI after deterministic benchmark sampling completed"
            deathUi = $true
            deathActionsPresent = $deathActionsPresent
            retryTrigger = "debuggable runtime broadcast invoking the production Java retry handler"
            javaBroadcastAccepted = $javaAccepted
            nativeRetryCheckpoint = $encounter.retryCheckpoint
            deathUiCleared = $retryXml -notmatch "YOU FELL"
            vitalityHudRestored =
                $retryXml -match "Vitality 3 of 3" -or
                $retryXml -match "VITALITY  3 / 3"
            deadScreenshotSha256 = $deadHash
            retryScreenshotSha256 = $retryHash
        })
    } catch {
        $failures.Add("$($encounter.name): $($_.Exception.Message)")
        try {
            [IO.File]::WriteAllText(
                (Join-Path $outputDirectory "$($encounter.name)-failure-logcat.txt"),
                (Get-ScopedLogcat) + [Environment]::NewLine
            )
        } catch {
            # Preserve the original failure if logcat is no longer reachable.
        }
    }
}

$pollPath = Join-Path $outputDirectory "poll-ui.xml"
if (Test-Path -LiteralPath $pollPath) {
    Remove-Item -LiteralPath $pollPath -Force
}

$summary = [ordered]@{
    schema = 1
    runId = $runId
    deviceModel = $deviceModel
    deviceCode = $deviceCode
    androidVersion = $androidVersion
    apiLevel = $apiLevel
    package = $packageName
    apkSha256 = $apkHash
    evidenceType =
        "automated real-enemy combat plus Android UI hierarchy, screenshots, Java/JNI/native retry markers"
    results = @($results)
    limitations = @(
        "Retry uses a Debug-only runtime broadcast through the production Java handler.",
        "Human touch feel and perceived haptics remain hands-on checks."
    )
    failures = @($failures)
}
[IO.File]::WriteAllText(
    (Join-Path $outputDirectory "summary.json"),
    ($summary | ConvertTo-Json -Depth 8) + [Environment]::NewLine
)

$resultText = if ($failures.Count -eq 0 -and $results.Count -eq 2) { "PASS" } else { "FAIL" }
$validation = @"
# Android player vitality validation run $runId

- Device: $deviceModel (Android $androidVersion / API $apiLevel)
- Debug APK SHA-256: ``$apkHash``
- Result: $resultText for real skeleton and lich damage/death, Android death UI, native opening/mirror retry, death-UI clearance, and 3/3 vitality restoration.
- Retry activation evidence: Debug-only runtime broadcast through the production Java retry handler; this is not a human touch or perceived-haptics claim.

See ``summary.json``, the four PNGs, UI XML files, and scoped logcat files.
"@
[IO.File]::WriteAllText(
    (Join-Path $outputDirectory "validation.md"),
    $validation.TrimStart() + [Environment]::NewLine
)

Write-Host "VITALITY_RUN=$outputDirectory"
Write-Host "APK_SHA256=$apkHash"
Write-Host "RESULT=$resultText"
Write-Host "RESULT_COUNT=$($results.Count)"
Write-Host "FAILURE_COUNT=$($failures.Count)"
if ($failures.Count -ne 0 -or $results.Count -ne 2) {
    throw "Android vitality validation failed: $($failures -join '; ')"
}
