param(
    [string]$Version = "0.1.0-alpha.1",
    [ValidateSet("Release", "Debug")]
    [string]$WindowsConfiguration = "Release",
    [string]$OutputRoot = (Join-Path $PSScriptRoot "..\releases\candidates")
)

$ErrorActionPreference = "Stop"
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$outputFull = [IO.Path]::GetFullPath($OutputRoot)
$allowedRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot "releases\candidates"))
if (-not $outputFull.StartsWith($allowedRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw "OutputRoot must stay inside $allowedRoot"
}

$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if (-not (Test-Path -LiteralPath $cmake)) {
    $cmakeCommand = Get-Command cmake -ErrorAction Stop
    $cmake = $cmakeCommand.Source
}

$safeVersion = $Version -replace '[^0-9A-Za-z.-]', '-'
$baseName = "Horde-Lantern-RT-Alpha-$safeVersion"
$releaseSigningValues = @(
    $env:HORDE_RELEASE_STORE_FILE,
    $env:HORDE_RELEASE_STORE_PASSWORD,
    $env:HORDE_RELEASE_KEY_ALIAS,
    $env:HORDE_RELEASE_KEY_PASSWORD
)
$releaseSigningConfigured = @($releaseSigningValues | Where-Object { [string]::IsNullOrWhiteSpace($_) }).Count -eq 0
$stageRoot = Join-Path $outputFull "stage"
$windowsStage = Join-Path $stageRoot "$baseName-Windows-x64"

foreach ($path in @($stageRoot, $outputFull)) {
    if (Test-Path -LiteralPath $path) {
        $resolved = [IO.Path]::GetFullPath($path)
        if (-not $resolved.StartsWith($allowedRoot, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to clean path outside candidate output: $resolved"
        }
    }
}
if (Test-Path -LiteralPath $stageRoot) {
    Remove-Item -LiteralPath $stageRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $windowsStage | Out-Null

& $cmake -S $repoRoot -B (Join-Path $repoRoot "build") -G "Visual Studio 17 2022" -A x64
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }
& $cmake --build (Join-Path $repoRoot "build") --config $WindowsConfiguration --target horde_rt_diagnostic_window horde_rt_combat_smoke
if ($LASTEXITCODE -ne 0) { throw "Windows build failed." }
& (Join-Path $repoRoot "build\$WindowsConfiguration\horde_rt_combat_smoke.exe")
if ($LASTEXITCODE -ne 0) { throw "Combat smoke test failed." }

$windowsExe = Join-Path $repoRoot "build\$WindowsConfiguration\HordeLanternRT.exe"
Copy-Item -LiteralPath $windowsExe -Destination (Join-Path $windowsStage "HordeLanternRT.exe")
Copy-Item -LiteralPath (Join-Path $repoRoot "release\windows\README.txt") -Destination (Join-Path $windowsStage "README.txt")
Copy-Item -LiteralPath (Join-Path $repoRoot "ASSET_LICENSES.md") -Destination (Join-Path $windowsStage "ASSET_LICENSES.md")
Copy-Item -LiteralPath (Join-Path $repoRoot "docs\ALPHA_RELEASE_NOTES_2026-07-15.md") -Destination (Join-Path $windowsStage "ALPHA_RELEASE_NOTES.md")

$assetCopies = @(
    @{ Source = "assets\models\enemies\meshy\skeleton_biped_merged_animations_v01.glb"; Destination = "assets\models\enemies\meshy" },
    @{ Source = "assets\textures\polyhaven\mobile_1k\diff-array-512.rgba"; Destination = "assets\textures\polyhaven\mobile_1k" },
    @{ Source = "assets\textures\polyhaven\mobile_1k\normal-array-512.rgba"; Destination = "assets\textures\polyhaven\mobile_1k" },
    @{ Source = "assets\textures\polyhaven\mobile_1k\arm-array-512.rgba"; Destination = "assets\textures\polyhaven\mobile_1k" }
)
foreach ($copy in $assetCopies) {
    $destination = Join-Path $windowsStage $copy.Destination
    New-Item -ItemType Directory -Force -Path $destination | Out-Null
    Copy-Item -LiteralPath (Join-Path $repoRoot $copy.Source) -Destination $destination
}
$audioDestination = Join-Path $windowsStage "assets\audio\filmcow"
New-Item -ItemType Directory -Force -Path $audioDestination | Out-Null
Copy-Item -Path (Join-Path $repoRoot "assets\audio\filmcow\*.wav") -Destination $audioDestination

$windowsZip = Join-Path $outputFull "$baseName-Windows-x64.zip"
if (Test-Path -LiteralPath $windowsZip) { Remove-Item -LiteralPath $windowsZip -Force }
Compress-Archive -Path (Join-Path $windowsStage "*") -DestinationPath $windowsZip -CompressionLevel Optimal

Push-Location (Join-Path $repoRoot "android")
try {
    .\gradlew.bat assembleDebug assembleRelease lintRelease --console=plain
    if ($LASTEXITCODE -ne 0) { throw "Android build or lint failed." }
} finally {
    Pop-Location
}

$debugApk = Join-Path $repoRoot "android\app\build\outputs\apk\debug\app-debug.apk"
$debugCandidate = Join-Path $outputFull "$baseName-Android-preview-debug-signed.apk"
Copy-Item -LiteralPath $debugApk -Destination $debugCandidate -Force

$signedRelease = Join-Path $repoRoot "android\app\build\outputs\apk\release\app-release.apk"
$unsignedRelease = Join-Path $repoRoot "android\app\build\outputs\apk\release\app-release-unsigned.apk"
if ($releaseSigningConfigured -and (Test-Path -LiteralPath $signedRelease)) {
    $androidCandidate = Join-Path $outputFull "$baseName-Android.apk"
    Copy-Item -LiteralPath $signedRelease -Destination $androidCandidate -Force
    $androidStatus = "SIGNED"
} elseif (-not $releaseSigningConfigured -and (Test-Path -LiteralPath $unsignedRelease)) {
    $androidCandidate = Join-Path $outputFull "$baseName-Android-UNSIGNED-DO-NOT-PUBLISH.apk"
    Copy-Item -LiteralPath $unsignedRelease -Destination $androidCandidate -Force
    $androidStatus = "UNSIGNED - DO NOT PUBLISH"
} elseif ($releaseSigningConfigured) {
    throw "Release signing was configured, but no signed release APK was produced."
} else {
    throw "No unsigned Android release APK was produced."
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::OpenRead($windowsZip)
try {
    $entryNames = @($archive.Entries | ForEach-Object FullName)
    foreach ($required in @(
        "HordeLanternRT.exe",
        "README.txt",
        "ASSET_LICENSES.md",
        "assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb",
        "assets/textures/polyhaven/mobile_1k/diff-array-512.rgba",
        "assets/audio/filmcow/sword_swing_1.wav"
    )) {
        if ($entryNames -notcontains $required) { throw "Windows zip is missing required entry: $required" }
    }
} finally {
    $archive.Dispose()
}

$knownCandidates = @(
    $windowsZip,
    $debugCandidate,
    (Join-Path $outputFull "$baseName-Android.apk"),
    (Join-Path $outputFull "$baseName-Android-UNSIGNED-DO-NOT-PUBLISH.apk")
)
$hashTargets = @($knownCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -Unique)
$hashLines = foreach ($target in $hashTargets) {
    $hash = Get-FileHash -Algorithm SHA256 -LiteralPath $target
    "$($hash.Hash.ToLowerInvariant())  $([IO.Path]::GetFileName($target))"
}
$hashFile = Join-Path $outputFull "SHA256SUMS.txt"
$hashLines | Set-Content -LiteralPath $hashFile -Encoding ascii

if (Test-Path -LiteralPath $stageRoot) {
    Remove-Item -LiteralPath $stageRoot -Recurse -Force
}

Write-Host "Windows candidate: $windowsZip"
Write-Host "Android preview:   $debugCandidate"
Write-Host "Android release:   $androidCandidate ($androidStatus)"
Write-Host "Hashes:            $hashFile"
