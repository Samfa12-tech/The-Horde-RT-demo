param(
    [ValidateSet("Host", "Full")]
    [string]$Mode = "Host",
    [ValidateRange(30, 300)]
    [int]$TimeoutSeconds = 180,
    [string]$ExpectedDeviceModel = "SM-S948B",
    [string]$BaselineWindowsCaptureDirectory,
    [string]$BaselineAndroidTimingCsv,
    [string]$OutputRoot = (Join-Path $PSScriptRoot "..\reports\foundation-runs")
)

$ErrorActionPreference = "Stop"
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$outputRootFull = [IO.Path]::GetFullPath($OutputRoot)
$allowedOutputRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot "reports"))
$allowedOutputPrefix = $allowedOutputRoot.TrimEnd([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
if ($outputRootFull -ne $allowedOutputRoot -and
    -not $outputRootFull.StartsWith($allowedOutputPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "OutputRoot must stay inside the ignored reports directory: $allowedOutputRoot"
}

$runId = Get-Date -Format "yyyyMMdd-HHmmss"
$runDirectory = Join-Path $outputRootFull "run-$runId"
$logDirectory = Join-Path $runDirectory "logs"
$artifactDirectory = Join-Path $runDirectory "artifacts"
$captureDirectory = Join-Path $runDirectory "captures"
$windowsCaptureDirectory = Join-Path $captureDirectory "windows"
$androidDeviceRoot = Join-Path $runDirectory "android-device"
$shaderDirectory = Join-Path $runDirectory "shader"
$windowsStage = Join-Path $artifactDirectory "windows-stage"
$windowsZip = Join-Path $artifactDirectory "Horde-Lantern-RT-validation-$runId-Windows-x64-UNPUBLISHABLE.zip"
$androidValidationApk = Join-Path $artifactDirectory "Horde-Lantern-RT-validation-$runId-Android-UNSIGNED-DO-NOT-PUBLISH.apk"
$summaryPath = Join-Path $runDirectory "summary.json"
$validationPath = Join-Path $runDirectory "validation.md"
$hashPath = Join-Path $runDirectory "SHA256SUMS.txt"
$stageResults = [Collections.Generic.List[object]]::new()
$failureMessage = ""
$gitCommit = "unknown"
$gitStatusBefore = ""
$gitStatusAfter = ""
$shaderRetention = "not-evaluated"
$sourcePackageVersion = ""
$sourceVersionCode = 0

foreach ($directory in @($runDirectory, $logDirectory, $artifactDirectory, $captureDirectory,
                          $windowsCaptureDirectory, $shaderDirectory)) {
    New-Item -ItemType Directory -Force -Path $directory | Out-Null
}

function Find-LatestVersionedTool {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$RelativeToolPath
    )
    $matches = foreach ($directory in Get-ChildItem -LiteralPath $Root -Directory -ErrorAction SilentlyContinue) {
        $version = $null
        if ([Version]::TryParse($directory.Name, [ref]$version)) {
            $candidate = Join-Path $directory.FullName $RelativeToolPath
            if (Test-Path -LiteralPath $candidate) {
                [PSCustomObject]@{ Version = $version; Path = $candidate }
            }
        }
    }
    $selected = $matches | Sort-Object Version -Descending | Select-Object -First 1
    if (-not $selected) { throw "Could not find $RelativeToolPath beneath $Root" }
    return $selected.Path
}

function Invoke-CheckedNative {
    param([Parameter(Mandatory = $true)][string]$FilePath, [string[]]$Arguments = @())
    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$FilePath failed with exit code $LASTEXITCODE"
    }
}

function Invoke-Stage {
    param([Parameter(Mandatory = $true)][string]$Name,
          [Parameter(Mandatory = $true)][scriptblock]$Action)
    $safeName = $Name -replace '[^0-9A-Za-z.-]', '-'
    $logPath = Join-Path $logDirectory "$safeName.log"
    $started = [DateTime]::UtcNow
    Write-Host "== $Name =="
    "[$($started.ToString('o'))] Starting stage: $Name" | Set-Content -LiteralPath $logPath -Encoding utf8
    try {
        & $Action *>&1 | Tee-Object -FilePath $logPath -Append
        $stageResults.Add([PSCustomObject]@{
            name = $Name
            status = "pass"
            durationSeconds = [Math]::Round(([DateTime]::UtcNow - $started).TotalSeconds, 3)
            log = [IO.Path]::GetRelativePath($runDirectory, $logPath)
        })
    } catch {
        $_ | Out-String | Add-Content -LiteralPath $logPath -Encoding utf8
        $stageResults.Add([PSCustomObject]@{
            name = $Name
            status = "fail"
            durationSeconds = [Math]::Round(([DateTime]::UtcNow - $started).TotalSeconds, 3)
            log = [IO.Path]::GetRelativePath($runDirectory, $logPath)
            error = $_.Exception.Message
        })
        throw
    }
}

function Assert-ExpectedFailure {
    param([Parameter(Mandatory = $true)][scriptblock]$Action,
          [Parameter(Mandatory = $true)][string]$MessagePattern)
    $failedAsExpected = $false
    try {
        & $Action
        if ($LASTEXITCODE -ne 0) { throw "Native command exited with code $LASTEXITCODE" }
    } catch {
        if ($_.Exception.Message -notmatch $MessagePattern) {
            throw "Negative gate failed for the wrong reason: $($_.Exception.Message)"
        }
        $failedAsExpected = $true
        Write-Output "Expected rejection: $($_.Exception.Message)"
    }
    if (-not $failedAsExpected) { throw "Expected a failure matching '$MessagePattern', but the command succeeded." }
}

function Test-PngDirectory {
    param([Parameter(Mandatory = $true)][string]$Directory,
          [Parameter(Mandatory = $true)][int]$ExpectedCount)
    $pngs = @(Get-ChildItem -LiteralPath $Directory -Recurse -Filter "*.png" -File)
    if ($pngs.Count -ne $ExpectedCount) {
        throw "Expected $ExpectedCount PNG captures beneath $Directory; found $($pngs.Count)."
    }
    foreach ($png in $pngs) {
        $stream = [IO.File]::OpenRead($png.FullName)
        try {
            $signature = [byte[]]::new(8)
            if ($stream.Read($signature, 0, 8) -ne 8 -or
                ([BitConverter]::ToString($signature) -ne "89-50-4E-47-0D-0A-1A-0A")) {
                throw "Invalid PNG signature: $($png.FullName)"
            }
        } finally {
            $stream.Dispose()
        }
    }
}

function Get-PngMetadata {
    param([Parameter(Mandatory = $true)][string]$Path)
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 24 -or [BitConverter]::ToString($bytes[0..7]) -ne "89-50-4E-47-0D-0A-1A-0A") {
        throw "Invalid PNG: $Path"
    }
    $width = ([int]$bytes[16] -shl 24) -bor ([int]$bytes[17] -shl 16) -bor ([int]$bytes[18] -shl 8) -bor [int]$bytes[19]
    $height = ([int]$bytes[20] -shl 24) -bor ([int]$bytes[21] -shl 16) -bor ([int]$bytes[22] -shl 8) -bor [int]$bytes[23]
    if ($width -le 0 -or $height -le 0) { throw "PNG has invalid dimensions: $Path" }
    return [PSCustomObject]@{
        width = $width
        height = $height
        sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}

function Test-CaptureManifest {
    param([Parameter(Mandatory = $true)][string]$ManifestPath,
          [Parameter(Mandatory = $true)][ValidateSet("Windows", "Android")][string]$Platform,
          [string]$ExpectedModel = "")
    if (-not (Test-Path -LiteralPath $ManifestPath)) { throw "$Platform capture manifest is missing: $ManifestPath" }
    $manifest = Get-Content -LiteralPath $ManifestPath -Raw | ConvertFrom-Json
    $records = @($(if ($Platform -eq "Windows") { $manifest.captures } else { $manifest.checkpoints }))
    $expected = @("opening", "skeleton", "worst-bend", "lantern-drop", "skylight", "yellow", "blue", "red", "green", "mirror", "lich", "finale-roof", "two-enemy-combat")
    if ($records.Count -ne $expected.Count) { throw "$Platform manifest contains $($records.Count) captures; expected $($expected.Count)." }
    if ($Platform -eq "Windows") {
        if (-not $manifest.complete -or -not $manifest.sceneOnly -or $manifest.overlaysIncluded) {
            throw "Windows capture manifest is not a complete scene-only capture set."
        }
        if ([double]$manifest.fixedAnimationTimeSeconds -ne 0.0) { throw "Windows capture animation time is not fixed at zero." }
        if ([string]::IsNullOrWhiteSpace($manifest.buildId) -or [string]::IsNullOrWhiteSpace($manifest.raygenSha256) -or
            [string]::IsNullOrWhiteSpace($manifest.device.gpuName)) { throw "Windows capture identity metadata is incomplete." }
        if ([int]$manifest.presentation.dispatchWidth -le 0 -or [int]$manifest.presentation.dispatchHeight -le 0 -or
            [int]$manifest.presentation.swapchainWidth -le 0 -or [int]$manifest.presentation.swapchainHeight -le 0) {
            throw "Windows capture extent metadata is invalid."
        }
    } else {
        if ([string]::IsNullOrWhiteSpace($manifest.device.model)) { throw "Android capture device metadata is incomplete." }
        if ($ExpectedModel -and $manifest.device.model -ne $ExpectedModel) {
            throw "Full validation requires $ExpectedModel; connected capture device was $($manifest.device.model)."
        }
        if (-not $manifest.lifecycle.homeResumePassed -or -not $manifest.lifecycle.honestPresentationAfterResume) {
            throw "Android Home/resume presentation evidence did not pass."
        }
    }
    $manifestDirectory = Split-Path -Parent $ManifestPath
    for ($index = 0; $index -lt $expected.Count; ++$index) {
        $record = $records[$index]
        if ($record.checkpoint -ne $expected[$index]) {
            throw "$Platform capture order mismatch at ${index}: expected $($expected[$index]), found $($record.checkpoint)."
        }
        $file = $(if ($Platform -eq "Windows") { $record.file } else { $record.png.file })
        $expectedWidth = [int]$(if ($Platform -eq "Windows") { $record.width } else { $record.png.width })
        $expectedHeight = [int]$(if ($Platform -eq "Windows") { $record.height } else { $record.png.height })
        $expectedHash = [string]$(if ($Platform -eq "Windows") { $record.pngSha256 } else { $record.png.sha256 })
        $pngPath = Join-Path $manifestDirectory $file
        $png = Get-PngMetadata -Path $pngPath
        if ($png.width -ne $expectedWidth -or $png.height -ne $expectedHeight -or $png.sha256 -ne $expectedHash) {
            throw "$Platform capture metadata/hash mismatch: $file"
        }
        if ($Platform -eq "Windows") {
            if (-not $record.honestlyPresentedRtFrame -or
                -not ($record.PSObject.Properties.Name -contains "outputRedBlueSwapAppliedAndNormalised")) {
                throw "Windows capture lacks honest-presentation or colour-route metadata: $file"
            }
        } else {
            if (-not $record.presented -or -not $record.sceneOnly -or [double]$record.animationTime -ne 0.0 -or
                [string]::IsNullOrWhiteSpace($record.gpu) -or [string]::IsNullOrWhiteSpace($record.buildIdentity) -or
                [string]::IsNullOrWhiteSpace($record.shaderIdentity) -or
                -not ($record.PSObject.Properties.Name -contains "outputRedBlueSwap")) {
                throw "Android capture lacks required state/identity/colour metadata: $file"
            }
        }
    }
    return $records.Count
}

function Write-ValidationPackage {
    param([Parameter(Mandatory = $true)][string]$WindowsExe,
          [Parameter(Mandatory = $true)][string]$AndroidApk)
    New-Item -ItemType Directory -Force -Path $windowsStage | Out-Null
    @(
        "UNPUBLISHABLE VALIDATION BUILD",
        "Generated by tools/run-foundation-validation.ps1.",
        "This is not a publishable release candidate and must never be uploaded or distributed."
    ) | Set-Content -LiteralPath (Join-Path $windowsStage "UNPUBLISHABLE_VALIDATION_BUILD.txt") -Encoding ascii
    Copy-Item -LiteralPath $WindowsExe -Destination (Join-Path $windowsStage "HordeLanternRT.exe")
    Copy-Item -LiteralPath (Join-Path $repoRoot "release\windows\README.txt") -Destination (Join-Path $windowsStage "README.txt")
    Copy-Item -LiteralPath (Join-Path $repoRoot "ASSET_LICENSES.md") -Destination (Join-Path $windowsStage "ASSET_LICENSES.md")
    $releaseNoteMatches = @(Get-ChildItem -LiteralPath (Join-Path $repoRoot "docs") -Filter "*RELEASE_NOTES*.md" -File |
        Where-Object {
            $text = Get-Content -LiteralPath $_.FullName -Raw
            $text.Contains("Package version: ``$($script:sourcePackageVersion)``") -and
                $text.Contains("Android version code: ``$($script:sourceVersionCode)``")
        })
    if ($releaseNoteMatches.Count -ne 1) {
        throw "Expected exactly one release-note file for $($script:sourcePackageVersion) / Android versionCode $($script:sourceVersionCode); found $($releaseNoteMatches.Count)."
    }
    Copy-Item -LiteralPath $releaseNoteMatches[0].FullName -Destination (Join-Path $windowsStage "ALPHA_RELEASE_NOTES.md")
    $copies = @(
        @{ Source = "assets\models\enemies\meshy\skeleton_biped_merged_animations_v01.glb"; Destination = "assets\models\enemies\meshy" },
        @{ Source = "assets\models\enemies\meshy\lich_placeholder_merged_animations_v01.glb"; Destination = "assets\models\enemies\meshy" },
        @{ Source = "assets\textures\meshy\lich_placeholder_v01\base-color-2048-rgba8.ktx2"; Destination = "assets\textures\meshy\lich_placeholder_v01" },
        @{ Source = "assets\textures\meshy\lich_placeholder_v01\emissive-2048-rgba8.ktx2"; Destination = "assets\textures\meshy\lich_placeholder_v01" },
        @{ Source = "assets\textures\polyhaven\mobile_1k\diff-array-512.rgba"; Destination = "assets\textures\polyhaven\mobile_1k" },
        @{ Source = "assets\textures\polyhaven\mobile_1k\normal-array-512.rgba"; Destination = "assets\textures\polyhaven\mobile_1k" },
        @{ Source = "assets\textures\polyhaven\mobile_1k\arm-array-512.rgba"; Destination = "assets\textures\polyhaven\mobile_1k" },
        @{ Source = "assets\models\weapons\runtime\asset.manifest.json"; Destination = "assets\models\weapons\runtime" },
        @{ Source = "assets\models\weapons\runtime\gothic-arming-sword-rh-lod0.runtime.glb"; Destination = "assets\models\weapons\runtime" },
        @{ Source = "assets\models\props\runtime\asset.manifest.json"; Destination = "assets\models\props\runtime" },
        @{ Source = "assets\models\props\runtime\gothic-hand-torch-lod0.runtime.glb"; Destination = "assets\models\props\runtime" },
        @{ Source = "assets\models\props\runtime\dielectric-fixture\asset.manifest.json"; Destination = "assets\models\props\runtime\dielectric-fixture" },
        @{ Source = "assets\models\props\runtime\dielectric-fixture\closed-glass-lod0.runtime.glb"; Destination = "assets\models\props\runtime\dielectric-fixture" },
        @{ Source = "assets\models\props\runtime\gothic-chest-base\asset.manifest.json"; Destination = "assets\models\props\runtime\gothic-chest-base" },
        @{ Source = "assets\models\props\runtime\gothic-chest-base\gothic-chest-base-lod0.runtime.glb"; Destination = "assets\models\props\runtime\gothic-chest-base" },
        @{ Source = "assets\models\props\runtime\gothic-chest-lid\asset.manifest.json"; Destination = "assets\models\props\runtime\gothic-chest-lid" },
        @{ Source = "assets\models\props\runtime\gothic-chest-lid\gothic-chest-lid-lod0.runtime.glb"; Destination = "assets\models\props\runtime\gothic-chest-lid" },
        @{ Source = "assets\models\props\runtime\reward-lantern-ring\asset.manifest.json"; Destination = "assets\models\props\runtime\reward-lantern-ring" },
        @{ Source = "assets\models\props\runtime\reward-lantern-ring\reward-lantern-ring-lod0.runtime.glb"; Destination = "assets\models\props\runtime\reward-lantern-ring" },
        @{ Source = "assets\models\props\runtime\reward-lantern-body\asset.manifest.json"; Destination = "assets\models\props\runtime\reward-lantern-body" },
        @{ Source = "assets\models\props\runtime\reward-lantern-body\reward-lantern-body-lod0.runtime.glb"; Destination = "assets\models\props\runtime\reward-lantern-body" },
        @{ Source = "assets\models\player\runtime\asset.manifest.json"; Destination = "assets\models\player\runtime" },
        @{ Source = "assets\models\player\runtime\clip-manifest.json"; Destination = "assets\models\player\runtime" },
        @{ Source = "assets\models\player\runtime\gothic-traveller-lod0.runtime.glb"; Destination = "assets\models\player\runtime" },
        @{ Source = "assets\textures\props\runtime\asset.manifest.json"; Destination = "assets\textures\props\runtime" },
        @{ Source = "assets\textures\props\runtime\base-color.windows.ktx2"; Destination = "assets\textures\props\runtime" },
        @{ Source = "assets\textures\props\runtime\normal.windows.ktx2"; Destination = "assets\textures\props\runtime" },
        @{ Source = "assets\textures\props\runtime\orm.windows.ktx2"; Destination = "assets\textures\props\runtime" },
        @{ Source = "assets\textures\props\runtime\emissive.windows.ktx2"; Destination = "assets\textures\props\runtime" }
    )
    foreach ($copy in $copies) {
        $destination = Join-Path $windowsStage $copy.Destination
        New-Item -ItemType Directory -Force -Path $destination | Out-Null
        Copy-Item -LiteralPath (Join-Path $repoRoot $copy.Source) -Destination $destination
    }
    $audioDestination = Join-Path $windowsStage "assets\audio\filmcow"
    New-Item -ItemType Directory -Force -Path $audioDestination | Out-Null
    Copy-Item -Path (Join-Path $repoRoot "assets\audio\filmcow\*.wav") -Destination $audioDestination
    $waterAudioDestination = Join-Path $windowsStage "assets\audio\pixabay"
    New-Item -ItemType Directory -Force -Path $waterAudioDestination | Out-Null
    Copy-Item -LiteralPath (Join-Path $repoRoot "assets\audio\pixabay\waterfall_loop.wav") -Destination $waterAudioDestination
    Compress-Archive -Path (Join-Path $windowsStage "*") -DestinationPath $windowsZip -CompressionLevel Optimal
    Copy-Item -LiteralPath $AndroidApk -Destination $androidValidationApk
    @(
        "UNPUBLISHABLE VALIDATION BUILD",
        "Unsigned Android validation APK generated by tools/run-foundation-validation.ps1."
    ) | Set-Content -LiteralPath "$androidValidationApk.txt" -Encoding ascii
}

function Assert-ZipContainsEntries {
    param([Parameter(Mandatory = $true)][string]$ArchivePath,
          [Parameter(Mandatory = $true)][string[]]$RequiredEntries,
          [Parameter(Mandatory = $true)][string]$PackageLabel)
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [IO.Compression.ZipFile]::OpenRead($ArchivePath)
    try {
        $entries = @($archive.Entries | ForEach-Object FullName)
        foreach ($required in $RequiredEntries) {
            if ($entries -notcontains $required) { throw "$PackageLabel is missing $required" }
        }
        return $entries
    } finally { $archive.Dispose() }
}

function Test-ValidationPackages {
    $entries = @(Assert-ZipContainsEntries -ArchivePath $windowsZip -PackageLabel "Windows validation zip" -RequiredEntries @(
        "HordeLanternRT.exe", "README.txt", "ASSET_LICENSES.md", "UNPUBLISHABLE_VALIDATION_BUILD.txt",
        "assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb",
        "assets/models/enemies/meshy/lich_placeholder_merged_animations_v01.glb",
        "assets/textures/meshy/lich_placeholder_v01/base-color-2048-rgba8.ktx2",
        "assets/textures/polyhaven/mobile_1k/diff-array-512.rgba",
        "assets/models/weapons/runtime/asset.manifest.json",
        "assets/models/weapons/runtime/gothic-arming-sword-rh-lod0.runtime.glb",
        "assets/models/props/runtime/asset.manifest.json",
        "assets/models/props/runtime/gothic-hand-torch-lod0.runtime.glb",
        "assets/models/props/runtime/dielectric-fixture/asset.manifest.json",
        "assets/models/props/runtime/dielectric-fixture/closed-glass-lod0.runtime.glb",
        "assets/models/props/runtime/gothic-chest-base/asset.manifest.json",
        "assets/models/props/runtime/gothic-chest-base/gothic-chest-base-lod0.runtime.glb",
        "assets/models/props/runtime/gothic-chest-lid/asset.manifest.json",
        "assets/models/props/runtime/gothic-chest-lid/gothic-chest-lid-lod0.runtime.glb",
        "assets/models/props/runtime/reward-lantern-ring/asset.manifest.json",
        "assets/models/props/runtime/reward-lantern-ring/reward-lantern-ring-lod0.runtime.glb",
        "assets/models/props/runtime/reward-lantern-body/asset.manifest.json",
        "assets/models/props/runtime/reward-lantern-body/reward-lantern-body-lod0.runtime.glb",
        "assets/models/player/runtime/asset.manifest.json",
        "assets/models/player/runtime/clip-manifest.json",
        "assets/models/player/runtime/gothic-traveller-lod0.runtime.glb",
        "assets/textures/props/runtime/asset.manifest.json",
        "assets/textures/props/runtime/base-color.windows.ktx2",
        "assets/audio/filmcow/sword_swing_1.wav",
        "assets/audio/pixabay/waterfall_loop.wav"))
    foreach ($forbidden in @("/source/", "/high/", "runtime-development", "gothic_arming_sword", "models/props/meshy/production-", ".processing.json", ".android.ktx2")) {
        if (@($entries | Where-Object { $_ -like "*$forbidden*" }).Count -ne 0) {
            throw "Windows validation zip contains forbidden development static asset content: $forbidden"
        }
    }

    $entries = @(Assert-ZipContainsEntries -ArchivePath $androidValidationApk -PackageLabel "Android validation APK" -RequiredEntries @(
        "assets/textures/polyhaven/mobile_1k/diff-array-512-astc6x6.ktx2",
        "assets/textures/polyhaven/mobile_1k/normal-array-512-astc4x4.ktx2",
        "assets/textures/polyhaven/mobile_1k/arm-array-512-astc6x6.ktx2",
        "assets/textures/meshy/lich_placeholder_v01/base-color-2048-astc6x6.ktx2",
        "assets/models/weapons/runtime/asset.manifest.json",
        "assets/models/weapons/runtime/gothic-arming-sword-rh-lod0.runtime.glb",
        "assets/models/props/runtime/asset.manifest.json",
        "assets/models/props/runtime/gothic-hand-torch-lod0.runtime.glb",
        "assets/models/props/runtime/dielectric-fixture/asset.manifest.json",
        "assets/models/props/runtime/dielectric-fixture/closed-glass-lod0.runtime.glb",
        "assets/models/props/runtime/gothic-chest-base/asset.manifest.json",
        "assets/models/props/runtime/gothic-chest-base/gothic-chest-base-lod0.runtime.glb",
        "assets/models/props/runtime/gothic-chest-lid/asset.manifest.json",
        "assets/models/props/runtime/gothic-chest-lid/gothic-chest-lid-lod0.runtime.glb",
        "assets/models/props/runtime/reward-lantern-ring/asset.manifest.json",
        "assets/models/props/runtime/reward-lantern-ring/reward-lantern-ring-lod0.runtime.glb",
        "assets/models/props/runtime/reward-lantern-body/asset.manifest.json",
        "assets/models/props/runtime/reward-lantern-body/reward-lantern-body-lod0.runtime.glb",
        "assets/models/player/runtime/asset.manifest.json",
        "assets/models/player/runtime/clip-manifest.json",
        "assets/models/player/runtime/gothic-traveller-lod0.runtime.glb",
        "assets/textures/props/runtime/asset.manifest.json",
        "assets/textures/props/runtime/base-color.android.ktx2",
        "assets/textures/props/runtime/normal.android.ktx2",
        "assets/textures/props/runtime/orm.android.ktx2",
        "assets/textures/props/runtime/emissive.android.ktx2",
        "assets/ASSET_LICENSES.md",
        "assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb",
        "assets/models/enemies/meshy/lich_placeholder_merged_animations_v01.glb",
        "assets/audio/pixabay/waterfall_loop.wav"))
    foreach ($forbidden in @("/source/", "/high/", "runtime-development", "gothic_arming_sword", "models/props/meshy/production-", ".processing.json", ".windows.ktx2")) {
        if (@($entries | Where-Object { $_ -like "*$forbidden*" }).Count -ne 0) {
            throw "Android validation APK contains forbidden development static asset content: $forbidden"
        }
    }
    $nativeEntries = @($entries | Where-Object { $_ -match '^lib/.+\.so$' })
    if ($nativeEntries.Count -lt 1) { throw "Android validation APK contains no native runtime library." }
    if ($nativeEntries -match 'libc\+\+_shared\.so$') { throw "Android validation APK contains libc++_shared.so." }

    $androidSdkRoot = if (-not [string]::IsNullOrWhiteSpace($env:ANDROID_HOME)) {
        $env:ANDROID_HOME
    } elseif (-not [string]::IsNullOrWhiteSpace($env:ANDROID_SDK_ROOT)) {
        $env:ANDROID_SDK_ROOT
    } else {
        Join-Path $env:LOCALAPPDATA "Android\Sdk"
    }
    if (-not (Test-Path -LiteralPath $androidSdkRoot)) { throw "Android SDK was not found: $androidSdkRoot" }
    $buildToolsRoot = Join-Path $androidSdkRoot "build-tools"
    $aapt2 = Find-LatestVersionedTool -Root $buildToolsRoot -RelativeToolPath "aapt2.exe"
    $zipalign = Find-LatestVersionedTool -Root $buildToolsRoot -RelativeToolPath "zipalign.exe"
    $resources = (& $aapt2 dump resources $androidValidationApk 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0) { throw "aapt2 could not inspect Android validation resources." }
    foreach ($marker in @(
        "string/credits_body", "Hotstrike Studio", "FilmCow", "Meshy", "DRAGON-STUDIO", "Pixabay",
        "Production Gothic arming sword created with Meshy; runtime processing by Samfa12/Codex",
        "Production medieval hand torch created with Meshy; runtime processing by Samfa12/Codex"
        "Historical-Gothic traveller/fighter created with Meshy; runtime processing and animation integration by Samfa12/Codex"
        "Production Gothic reward chest created with Meshy; runtime processing by Samfa12/Codex"
        "Production Gothic reward lantern created with Meshy; runtime processing by Samfa12/Codex"
    )) {
        if ($resources -notmatch [regex]::Escape($marker)) { throw "Android validation APK lacks credit marker: $marker" }
    }
    & (Join-Path $PSScriptRoot "test-held-item-package-contract.ps1") `
        -AndroidApkPath $androidValidationApk -WindowsZipPath $windowsZip
    if ($LASTEXITCODE -ne 0) { throw "Held-item validation package contract failed." }
    $manifest = (& $aapt2 dump xmltree --file AndroidManifest.xml $androidValidationApk 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0) { throw "aapt2 could not inspect Android validation manifest." }
    $escapedSourceVersion = [regex]::Escape($script:sourcePackageVersion)
    foreach ($pattern in @(
        'package="com\.samfa12\.hordelanternrt"',
        "versionName[^\r\n]*=`"$escapedSourceVersion`"",
        "versionCode[^\r\n]*=$($script:sourceVersionCode)(?:\s|$)",
        'screenOrientation[^\r\n]*=7(?:\s|$)')) {
        if ($manifest -notmatch $pattern) { throw "Android validation manifest failed required pattern: $pattern" }
    }
    & $zipalign -c -P 16 -v 4 $androidValidationApk | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "Android validation APK failed 16 KiB zip alignment." }

    $readelf = Find-LatestVersionedTool -Root (Join-Path $androidSdkRoot "ndk") `
        -RelativeToolPath "toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-readelf.exe"
    $strippedNativeRoot = Join-Path $repoRoot "android\app\build\intermediates\stripped_native_libs\release\stripReleaseDebugSymbols\out\lib"
    $releaseLibraries = @(Get-ChildItem -LiteralPath $strippedNativeRoot -Recurse -Filter "*.so")
    if ($releaseLibraries.Count -lt 1) { throw "No stripped Android release libraries were found for ELF alignment checks." }
    foreach ($library in $releaseLibraries) {
        $headers = @(& $readelf -l $library.FullName 2>&1 | Select-String '^\s*LOAD\s')
        if ($LASTEXITCODE -ne 0 -or $headers.Count -lt 1) { throw "Could not inspect ELF headers: $($library.FullName)" }
        foreach ($header in $headers) {
            if ($header.Line -notmatch '\s0x4000\s*$') { throw "Android native library has a LOAD segment below 16 KiB alignment: $($library.FullName)" }
        }
    }
}

try {
    Push-Location $repoRoot
    try {
        $gitCommit = (& git rev-parse HEAD).Trim()
        if ($LASTEXITCODE -ne 0) { throw "Could not read git commit." }
        $gitStatusBefore = (& git status --porcelain=v1 | Out-String).TrimEnd()
        $gitStatusBefore | Set-Content -LiteralPath (Join-Path $runDirectory "git-status-before.txt") -Encoding utf8

        Invoke-Stage "shader-staleness" {
            & (Join-Path $PSScriptRoot "compile-raygen.ps1") -Check -OutputDirectory $shaderDirectory
            if ($LASTEXITCODE -ne 0) { throw "Raygen staleness check failed." }
        }

        Invoke-Stage "negative-safety-gates" {
            $staleInclude = Join-Path $shaderDirectory "deliberately-stale.inc"
            "    0x07230203u, 0x00000000u," | Set-Content -LiteralPath $staleInclude -Encoding ascii
            Assert-ExpectedFailure {
                & (Join-Path $PSScriptRoot "compile-raygen.ps1") -Check `
                    -OutputDirectory (Join-Path $shaderDirectory "negative-check") `
                    -EmbeddedIncludePath $staleInclude
            } 'stale'
            $trackedShaderInclude = Join-Path $repoRoot "shaders\raytracing\include\rt_atmosphere.glsl"
            $trackedShaderIncludeOriginal = [IO.File]::ReadAllText($trackedShaderInclude)
            try {
                [IO.File]::WriteAllText(
                    $trackedShaderInclude,
                    $trackedShaderIncludeOriginal + "`n// foundation include-staleness negative fixture`n",
                    [Text.UTF8Encoding]::new($false))
                Assert-ExpectedFailure {
                    & (Join-Path $PSScriptRoot "compile-raygen.ps1") -Check `
                        -OutputDirectory (Join-Path $shaderDirectory "negative-tracked-include")
                } 'stale'
            }
            finally {
                [IO.File]::WriteAllText(
                    $trackedShaderInclude,
                    $trackedShaderIncludeOriginal,
                    [Text.UTF8Encoding]::new($false))
            }
            Assert-ExpectedFailure {
                & (Join-Path $PSScriptRoot "package-alpha.ps1") -Version "0.1.1-alpha.1" -VersionCode 4
            } 'immutable'
            Assert-ExpectedFailure {
                & (Join-Path $PSScriptRoot "package-alpha.ps1") -Version "0.1.2+rebuild" -VersionCode 4
            } 'immutable'
            Assert-ExpectedFailure {
                & (Join-Path $PSScriptRoot "package-alpha.ps1") -Version "0.1.2.preview" -VersionCode 4
            } 'immutable'
            Assert-ExpectedFailure {
                & (Join-Path $PSScriptRoot "package-alpha.ps1") -Version "0.1.3-alpha.1" -VersionCode 5
            } 'immutable'
            Assert-ExpectedFailure {
                & (Join-Path $PSScriptRoot "package-alpha.ps1") -Version "0.1.4-alpha.1" -VersionCode 6
            } 'immutable'
            Assert-ExpectedFailure {
                & (Join-Path $PSScriptRoot "package-alpha.ps1") -Version "0.1.5-alpha.1" -VersionCode 7
            } 'immutable'
            Assert-ExpectedFailure {
                & (Join-Path $PSScriptRoot "package-alpha.ps1") -Version "1.5.2" -VersionCode 8
            } 'immutable'
            Assert-ExpectedFailure {
                & (Join-Path $PSScriptRoot "package-alpha.ps1") -Version "1.5.3" -VersionCode 7
            } 'greater than'
            Assert-ExpectedFailure {
                & (Join-Path $PSScriptRoot "push-alpha-to-itch.ps1") -Version "1.5.2" `
                    -ButlerPath (Join-Path $runDirectory "missing-butler.exe")
            } 'immutable'

            $negativePackageRoot = Join-Path $runDirectory "negative-packages"
            $missingAssetRoot = Join-Path $negativePackageRoot "missing-asset"
            $badLayoutRoot = Join-Path $negativePackageRoot "bad-layout\nested-root"
            New-Item -ItemType Directory -Force -Path $missingAssetRoot, $badLayoutRoot | Out-Null
            "fixture" | Set-Content -LiteralPath (Join-Path $missingAssetRoot "HordeLanternRT.exe") -Encoding ascii
            "fixture" | Set-Content -LiteralPath (Join-Path $badLayoutRoot "HordeLanternRT.exe") -Encoding ascii
            $missingAssetZip = Join-Path $negativePackageRoot "missing-asset.zip"
            $badLayoutZip = Join-Path $negativePackageRoot "bad-layout.zip"
            Add-Type -AssemblyName System.IO.Compression.FileSystem
            [IO.Compression.ZipFile]::CreateFromDirectory($missingAssetRoot, $missingAssetZip)
            [IO.Compression.ZipFile]::CreateFromDirectory((Split-Path -Parent $badLayoutRoot), $badLayoutZip)
            Assert-ExpectedFailure {
                Assert-ZipContainsEntries -ArchivePath $missingAssetZip -PackageLabel "negative package" `
                    -RequiredEntries @("HordeLanternRT.exe", "ASSET_LICENSES.md") | Out-Null
            } 'missing ASSET_LICENSES.md'
            Assert-ExpectedFailure {
                Assert-ZipContainsEntries -ArchivePath $badLayoutZip -PackageLabel "negative package" `
                    -RequiredEntries @("HordeLanternRT.exe") | Out-Null
            } 'missing HordeLanternRT.exe'
        }

        $cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        if (-not (Test-Path -LiteralPath $cmake)) { $cmake = (Get-Command cmake -ErrorAction Stop).Source }
        $ctest = Join-Path (Split-Path -Parent $cmake) "ctest.exe"
        $windowsBuild = Join-Path $repoRoot "build\foundation-validation\$runId"

        Invoke-Stage "windows-fresh-build-and-tests" {
            Invoke-CheckedNative $cmake @("-S", $repoRoot, "-B", $windowsBuild, "-G", "Visual Studio 17 2022", "-A", "x64")
            foreach ($configuration in @("Debug", "Release")) {
                Invoke-CheckedNative $cmake @("--build", $windowsBuild, "--config", $configuration)
                Invoke-CheckedNative $ctest @("--test-dir", $windowsBuild, "-C", $configuration, "--output-on-failure")
            }
        }

        Invoke-Stage "windows-fixed-captures" {
            $windowsDebugExe = Join-Path $windowsBuild "Debug\HordeLanternRT.exe"
            $captureProcess = Start-Process -FilePath $windowsDebugExe `
                -ArgumentList @("--capture-showcase", "`"$windowsCaptureDirectory`"") -Wait -PassThru
            if ($captureProcess.ExitCode -ne 0) { throw "Windows Debug capture failed with exit code $($captureProcess.ExitCode)." }
            $captureCount = Test-CaptureManifest `
                -ManifestPath (Join-Path $windowsCaptureDirectory "capture-manifest.json") -Platform Windows
            if ($captureCount -ne 13) { throw "Windows deterministic capture count was $captureCount." }
            $releaseRejectionDirectory = Join-Path $captureDirectory "release-must-not-create"
            $releaseProcess = Start-Process -FilePath (Join-Path $windowsBuild "Release\HordeLanternRT.exe") `
                -ArgumentList @("--capture-showcase", "`"$releaseRejectionDirectory`"") -Wait -PassThru
            if ($releaseProcess.ExitCode -ne 2) { throw "Windows Release capture automation was not rejected with exit code 2." }
            if (Test-Path -LiteralPath $releaseRejectionDirectory) { throw "Windows Release capture rejection created output." }
        }

        Invoke-Stage "android-fresh-build-and-lint" {
            Push-Location (Join-Path $repoRoot "android")
            $validationUnsignedExisted = Test-Path Env:\HORDE_VALIDATION_UNSIGNED
            $validationUnsignedPrevious = $(if ($validationUnsignedExisted) { $env:HORDE_VALIDATION_UNSIGNED } else { $null })
            try {
                $env:HORDE_VALIDATION_UNSIGNED = "1"
                .\gradlew.bat clean assembleDebug assembleRelease lintRelease --console=plain
                if ($LASTEXITCODE -ne 0) { throw "Android build or lint failed." }
            } finally {
                if ($validationUnsignedExisted) { $env:HORDE_VALIDATION_UNSIGNED = $validationUnsignedPrevious }
                else { Remove-Item Env:\HORDE_VALIDATION_UNSIGNED -ErrorAction SilentlyContinue }
                Pop-Location
            }
            $windowsPowerShell = (Get-Command powershell -ErrorAction Stop).Source
            foreach ($assetMathConfiguration in @("Debug", "Release")) {
                Invoke-CheckedNative $windowsPowerShell @(
                    "-NoProfile",
                    "-File", (Join-Path $repoRoot "tests\AssetMathCompilerContractTests.ps1"),
                    "-Configuration", $assetMathConfiguration,
                    "-SkipAndroidBuild",
                    "-OutputDirectory", (Join-Path $runDirectory "asset-math-compiler-contract\$($assetMathConfiguration.ToLowerInvariant())")
                )
            }
        }

        $windowsReleaseExe = Join-Path $windowsBuild "Release\HordeLanternRT.exe"
        $androidUnsignedApk = Join-Path $repoRoot "android\app\build\outputs\apk\release\app-release-unsigned.apk"
        Invoke-Stage "validation-package-and-licence-gate" {
            $cmakeVersionSource = Get-Content -LiteralPath (Join-Path $repoRoot "cmake\HordeRtSources.cmake") -Raw
            $gradleVersionSource = Get-Content -LiteralPath (Join-Path $repoRoot "android\app\build.gradle") -Raw
            $cmakeVersionMatch = [regex]::Match($cmakeVersionSource, 'HORDE_RT_PACKAGE_VERSION\s+"([^"]+)"')
            $gradleNameMatch = [regex]::Match($gradleVersionSource, "versionName\s+'([^']+)'")
            $gradleCodeMatch = [regex]::Match($gradleVersionSource, 'versionCode\s+(\d+)(?:\s|$)')
            if (-not $cmakeVersionMatch.Success -or -not $gradleNameMatch.Success -or -not $gradleCodeMatch.Success) {
                throw "Could not parse the current CMake/Android release identity."
            }
            if ($cmakeVersionMatch.Groups[1].Value -ne $gradleNameMatch.Groups[1].Value) {
                throw "CMake and Android package versions disagree."
            }
            $script:sourcePackageVersion = $cmakeVersionMatch.Groups[1].Value
            $script:sourceVersionCode = [int]$gradleCodeMatch.Groups[1].Value
            foreach ($required in @(
                $windowsReleaseExe, $androidUnsignedApk, (Join-Path $repoRoot "ASSET_LICENSES.md"),
                (Join-Path $repoRoot "assets\models\enemies\meshy\lich_placeholder_source_licence.png"))) {
                if (-not (Test-Path -LiteralPath $required)) { throw "Required validation/package input is missing: $required" }
            }
            Write-ValidationPackage -WindowsExe $windowsReleaseExe -AndroidApk $androidUnsignedApk
            Test-ValidationPackages
            $windowsVersion = (Get-Item -LiteralPath $windowsReleaseExe).VersionInfo
            if ($windowsVersion.FileVersion -ne $script:sourcePackageVersion -or
                $windowsVersion.ProductVersion -ne $script:sourcePackageVersion) {
                throw "Windows validation binary version surfaces do not agree on $($script:sourcePackageVersion)."
            }
            $windowsBinaryText = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($windowsReleaseExe))
            foreach ($marker in @(
                "credits and licences", "Hotstrike Studio", "FilmCow", "Meshy", "DRAGON-STUDIO", "Pixabay",
                "Production Gothic arming sword created with Meshy; runtime processing by Samfa12/Codex",
                "Production medieval hand torch created with Meshy; runtime processing by Samfa12/Codex"
                "Historical-Gothic traveller/fighter created with Meshy; runtime processing and animation integration by Samfa12/Codex"
                "Production Gothic reward chest created with Meshy; runtime processing by Samfa12/Codex"
                "Production Gothic reward lantern created with Meshy; runtime processing by Samfa12/Codex"
            )) {
                if ($windowsBinaryText -notmatch [regex]::Escape($marker)) { throw "Windows executable lacks credit marker: $marker" }
            }
        }

        if ($Mode -eq "Full") {
            Invoke-Stage "android-device-validation-and-captures" {
                & (Join-Path $PSScriptRoot "run-android-showcase-validation.ps1") `
                    -Mode Both -Scale 75 -Include100 -Capture -SkipBuild `
                    -TimeoutSeconds $TimeoutSeconds -OutputRoot $androidDeviceRoot
                if ($LASTEXITCODE -ne 0) { throw "Android device validation failed." }
                $manifests = @(Get-ChildItem -LiteralPath $androidDeviceRoot -Recurse -Filter "capture-manifest.json" -File)
                if ($manifests.Count -ne 1) { throw "Expected one Android capture manifest; found $($manifests.Count)." }
                $captureCount = Test-CaptureManifest -ManifestPath $manifests[0].FullName `
                    -Platform Android -ExpectedModel $ExpectedDeviceModel
                if ($captureCount -ne 13) { throw "Android deterministic capture count was $captureCount." }
            }

            if ([string]::IsNullOrWhiteSpace($BaselineWindowsCaptureDirectory) -xor
                [string]::IsNullOrWhiteSpace($BaselineAndroidTimingCsv)) {
                throw "Supply both baseline capture and Android timing paths, or neither."
            }
            if (-not [string]::IsNullOrWhiteSpace($BaselineWindowsCaptureDirectory)) {
                Invoke-Stage "shader-retention-ab" {
                    $comparisonPath = Join-Path $runDirectory "windows-capture-comparison.json"
                    & (Join-Path $PSScriptRoot "compare-foundation-captures.ps1") `
                        -BaselineDirectory $BaselineWindowsCaptureDirectory `
                        -CandidateDirectory $windowsCaptureDirectory `
                        -OutputPath $comparisonPath
                    if ($LASTEXITCODE -ne 0) { throw "Windows shader A/B comparison failed." }

                    $androidRuns = @(Get-ChildItem -LiteralPath $androidDeviceRoot -Directory | Sort-Object LastWriteTime -Descending)
                    if ($androidRuns.Count -lt 1) { throw "Android Full run directory was not found." }
                    $candidateTimingPath = Join-Path $androidRuns[0].FullName "timing.csv"
                    $baselineRows = @(Import-Csv -LiteralPath ([IO.Path]::GetFullPath($BaselineAndroidTimingCsv)) |
                        Where-Object { [int]$_.scale -eq 75 })
                    $candidateRows = @(Import-Csv -LiteralPath $candidateTimingPath |
                        Where-Object { [int]$_.scale -eq 75 })
                    $historicalRequired = @("opening", "worst-bend", "skylight", "green", "lich")
                    $expandedRequired = @("opening", "two-enemy-combat", "worst-bend", "skylight", "green", "lich")
                    if ($baselineRows.Count -notin @(5, 6) -or $candidateRows.Count -ne 6) {
                        throw "Android A/B requires a historical five- or expanded six-checkpoint baseline and exactly six candidate rows at 75%."
                    }
                    $required = $(if ($baselineRows.Count -eq 5) { $historicalRequired } else { $expandedRequired })
                    foreach ($row in @($baselineRows) + @($candidateRows)) {
                        if ([int]$row.scale -ne 75 -or [string]$row.timing_method -ne "cpu-present-loop" -or
                            [string]$row.presented -ne "True" -or [int]$row.thermal_status -lt 0 -or
                            [int]$row.thermal_status -gt 3) {
                            throw "Android A/B timing rows must retain 75% CPU-present-loop timing, honest RT presentation, and thermal status 0-3."
                        }
                    }
                    $phoneComparisons = foreach ($checkpoint in $required) {
                        $before = $baselineRows | Where-Object checkpoint -eq $checkpoint | Select-Object -First 1
                        $after = $candidateRows | Where-Object checkpoint -eq $checkpoint | Select-Object -First 1
                        if (-not $before -or -not $after) { throw "Android A/B is missing checkpoint $checkpoint." }
                        if ([Math]::Abs([int]$before.thermal_status - [int]$after.thermal_status) -gt 1) {
                            throw "Android thermal status is not comparable for $checkpoint."
                        }
                        $baselineMs = [double]$before.median_of_window_avgs_ms
                        $candidateMs = [double]$after.median_of_window_avgs_ms
                        [PSCustomObject]@{
                            checkpoint = $checkpoint
                            baselineMs = $baselineMs
                            candidateMs = $candidateMs
                            deltaPercent = [Math]::Round((($candidateMs / $baselineMs) - 1.0) * 100.0, 3)
                            baselineThermal = [int]$before.thermal_status
                            candidateThermal = [int]$after.thermal_status
                            passed = $candidateMs -le $baselineMs * 1.02
                        }
                    }
                    if (@($phoneComparisons | Where-Object { -not $_.passed }).Count -gt 0) {
                        throw "One or more Android 75% checkpoints exceeded the 2% A/B tolerance."
                    }
                    $shaderStats = Get-Content -LiteralPath (Join-Path $shaderDirectory "raygen-stats.json") -Raw | ConvertFrom-Json
                    $structurePassed = [int]$shaderStats.bytes -lt 71908 -and
                        [int]$shaderStats.instructions -lt 4025 -and
                        [int]$shaderStats.branchOperations -lt 568 -and
                        [int]$shaderStats.selectionMerges -lt 216 -and
                        [int]$shaderStats.loops -le 6
                    if (-not $structurePassed) { throw "Optimized shader did not improve the recorded SPIR-V structure baseline." }
                    $retention = [ordered]@{
                        schema = 1
                        decision = "retain"
                        structurePassed = $structurePassed
                        baselineSpirv = [ordered]@{ bytes = 71908; instructions = 4025; branchOperations = 568; loops = 6; selectionMerges = 216 }
                        candidateSpirv = $shaderStats
                        windowsComparison = "windows-capture-comparison.json"
                        androidBaselineTiming = [IO.Path]::GetFullPath($BaselineAndroidTimingCsv)
                        androidCandidateTiming = $candidateTimingPath
                        unpairedAndroidCandidate = $(if ($baselineRows.Count -eq 5) {
                            $twoEnemy = $candidateRows | Where-Object checkpoint -eq "two-enemy-combat" | Select-Object -First 1
                            [ordered]@{
                                checkpoint = "two-enemy-combat"
                                medianMs = [double]$twoEnemy.median_of_window_avgs_ms
                                thermalStatus = [int]$twoEnemy.thermal_status
                                honestlyPresented = [string]$twoEnemy.presented -eq "True"
                            }
                        } else { $null })
                        phoneComparisons = @($phoneComparisons)
                    }
                    $retention | ConvertTo-Json -Depth 8 | Set-Content `
                        -LiteralPath (Join-Path $runDirectory "shader-retention.json") -Encoding utf8
                    $script:shaderRetention = "retain"
                }
            }
        }

        Invoke-Stage "evidence-hashes" {
            $sourceAndArtifactTargets = @(
                (Join-Path $repoRoot "shaders\raytracing\minimal.rgen"),
                (Join-Path $repoRoot "src\vulkan\raytracing\MinimalRayGenShader.inc"),
                (Join-Path $repoRoot "ASSET_LICENSES.md"),
                $windowsReleaseExe,
                $androidUnsignedApk,
                $windowsZip,
                $androidValidationApk,
                (Join-Path $repoRoot "assets\models\enemies\meshy\skeleton_biped_merged_animations_v01.glb"),
                (Join-Path $repoRoot "assets\models\enemies\meshy\lich_placeholder_merged_animations_v01.glb"),
                (Join-Path $repoRoot "assets\textures\polyhaven\mobile_1k\diff-array-512-astc6x6.ktx2"),
                (Join-Path $repoRoot "assets\textures\polyhaven\mobile_1k\normal-array-512-astc4x4.ktx2"),
                (Join-Path $repoRoot "assets\textures\polyhaven\mobile_1k\arm-array-512-astc6x6.ktx2"),
                (Join-Path $repoRoot "assets\textures\meshy\lich_placeholder_v01\base-color-2048-astc6x6.ktx2"),
                (Join-Path $repoRoot "assets\textures\meshy\lich_placeholder_v01\emissive-2048-astc6x6.ktx2")
            )
            $inventory = foreach ($target in $sourceAndArtifactTargets) {
                if (-not (Test-Path -LiteralPath $target)) { throw "Hash inventory target is missing: $target" }
                [ordered]@{
                    path = $target
                    size = (Get-Item -LiteralPath $target).Length
                    sha256 = (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToLowerInvariant()
                }
            }
            $inventory | ConvertTo-Json -Depth 4 | Set-Content `
                -LiteralPath (Join-Path $runDirectory "source-artifact-hashes.json") -Encoding utf8
            $hashTargets = @(Get-ChildItem -LiteralPath $runDirectory -Recurse -File | Where-Object {
                $_.FullName -ne $summaryPath -and $_.FullName -ne $validationPath -and $_.FullName -ne $hashPath
            })
            $hashLines = foreach ($target in $hashTargets) {
                $hash = (Get-FileHash -LiteralPath $target.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
                "$hash  $([IO.Path]::GetRelativePath($runDirectory, $target.FullName).Replace('\', '/'))"
            }
            $hashLines | Sort-Object | Set-Content -LiteralPath $hashPath -Encoding ascii
        }
    } finally {
        Pop-Location
    }
} catch {
    $failureMessage = $_.Exception.Message
} finally {
    Push-Location $repoRoot
    try { $gitStatusAfter = (& git status --porcelain=v1 | Out-String).TrimEnd() } finally { Pop-Location }
    $gitStatusAfter | Set-Content -LiteralPath (Join-Path $runDirectory "git-status-after.txt") -Encoding utf8
    $pngCount = @(Get-ChildItem -LiteralPath $captureDirectory -Recurse -Filter "*.png" -File -ErrorAction SilentlyContinue).Count +
                @(Get-ChildItem -LiteralPath $androidDeviceRoot -Recurse -Filter "*.png" -File -ErrorAction SilentlyContinue).Count
    $summary = [ordered]@{
        schema = 1
        runId = $runId
        mode = $Mode
        result = $(if ([string]::IsNullOrWhiteSpace($failureMessage)) { "pass" } else { "fail" })
        failure = $failureMessage
        gitCommit = $gitCommit
        gitStatusBefore = $gitStatusBefore
        gitStatusAfter = $gitStatusAfter
        shaderRegisterPressureMeasured = $false
        shaderRegisterPressureNote = "NVIDIA Nsight is not installed; SPIR-V structure and device timing are the available evidence."
        shaderRetention = $shaderRetention
        validationArtifactsPublishable = $false
        captureCount = $pngCount
        stages = @($stageResults)
    }
    $summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $summaryPath -Encoding utf8
    @(
        "# Foundation validation run $runId",
        "",
        "- Mode: $Mode",
        "- Result: $($summary.result.ToUpperInvariant())",
        "- Git commit: ``$gitCommit``",
        "- Captures: $pngCount",
        "- Validation artifacts: UNPUBLISHABLE",
        "- Register pressure: not numerically measured; NVIDIA Nsight is not installed.",
        $(if ($failureMessage) { "- Failure: $failureMessage" } else { "- Failure: none" }),
        "",
        "See ``summary.json``, ``SHA256SUMS.txt``, ``logs/``, ``shader/``, ``captures/``, and ``artifacts/``."
    ) | Set-Content -LiteralPath $validationPath -Encoding utf8
}

if ($failureMessage) {
    Write-Error "Foundation validation failed: $failureMessage`nEvidence: $runDirectory"
    exit 1
}
Write-Host "Foundation validation passed: $runDirectory"
