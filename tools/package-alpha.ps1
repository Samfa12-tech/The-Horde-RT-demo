param(
    [Parameter(Mandatory = $true)]
    [string]$Version,
    [Parameter(Mandatory = $true)]
    [int]$VersionCode,
    [ValidateSet("Release", "Debug")]
    [string]$WindowsConfiguration = "Release",
    [string]$OutputRoot = (Join-Path $PSScriptRoot "..\releases\candidates")
)

$ErrorActionPreference = "Stop"

if ($Version -match '^0\.1\.(?:1|2)(?:$|[-+.])') {
    throw "The published 0.1.1 and 0.1.2 release lines are immutable. Choose a new Version."
}
if ($VersionCode -le 3) {
    throw "VersionCode must be greater than the immutable published value 3."
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
    if (-not $selected) {
        throw "Could not find $RelativeToolPath beneath $Root"
    }
    return $selected.Path
}

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$outputFull = [IO.Path]::GetFullPath($OutputRoot)
$allowedRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot "releases\candidates"))
$allowedPrefix = $allowedRoot.TrimEnd([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
if ($outputFull -ne $allowedRoot -and -not $outputFull.StartsWith($allowedPrefix, [StringComparison]::OrdinalIgnoreCase)) {
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
$candidateFiles = @(
    (Join-Path $outputFull "$baseName-Windows-x64.zip"),
    (Join-Path $outputFull "$baseName-Android-preview-debug-signed.apk"),
    (Join-Path $outputFull "$baseName-Android.apk"),
    (Join-Path $outputFull "$baseName-Android-UNSIGNED-DO-NOT-PUBLISH.apk"),
    (Join-Path $outputFull "SHA256SUMS.txt")
)

foreach ($path in @($stageRoot, $outputFull)) {
    if (Test-Path -LiteralPath $path) {
        $resolved = [IO.Path]::GetFullPath($path)
        if ($resolved -ne $allowedRoot -and -not $resolved.StartsWith($allowedPrefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to clean path outside candidate output: $resolved"
        }
    }
}
if (Test-Path -LiteralPath $stageRoot) {
    Remove-Item -LiteralPath $stageRoot -Recurse -Force
}
foreach ($candidateFile in $candidateFiles) {
    if (Test-Path -LiteralPath $candidateFile) {
        Remove-Item -LiteralPath $candidateFile -Force
    }
}
New-Item -ItemType Directory -Force -Path $windowsStage | Out-Null

& $cmake -S $repoRoot -B (Join-Path $repoRoot "build") -G "Visual Studio 17 2022" -A x64
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }
& $cmake --build (Join-Path $repoRoot "build") --config $WindowsConfiguration
if ($LASTEXITCODE -ne 0) { throw "Windows build failed." }
$ctest = Join-Path (Split-Path -Parent $cmake) "ctest.exe"
& $ctest --test-dir (Join-Path $repoRoot "build") -C $WindowsConfiguration --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Windows CTest suite failed." }

$windowsExe = Join-Path $repoRoot "build\$WindowsConfiguration\HordeLanternRT.exe"
$windowsSdkBin = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin"
$mt = Find-LatestVersionedTool -Root $windowsSdkBin -RelativeToolPath "x64\mt.exe"
$manifestAudit = Join-Path $outputFull "HordeLanternRT.embedded.manifest"
try {
    & $mt -nologo "-inputresource:$windowsExe;#1" "-out:$manifestAudit"
    if ($LASTEXITCODE -ne 0) { throw "Failed to extract the Windows application manifest." }
    $manifestText = Get-Content -LiteralPath $manifestAudit -Raw
    if ($manifestText -notmatch 'PerMonitorV2') {
        throw "Windows executable does not declare Per-Monitor V2 DPI awareness."
    }
} finally {
    if (Test-Path -LiteralPath $manifestAudit) {
        Remove-Item -LiteralPath $manifestAudit -Force
    }
}
$windowsBinaryText = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($windowsExe))
foreach ($creditMarker in @("credits and licences", "Hotstrike Studio", "FilmCow", "Meshy")) {
    if ($windowsBinaryText -notmatch [regex]::Escape($creditMarker)) {
        throw "Windows executable is missing in-app credit marker: $creditMarker"
    }
}
$windowsVersion = (Get-Item -LiteralPath $windowsExe).VersionInfo
if ($windowsVersion.FileVersion -ne $Version -or $windowsVersion.ProductVersion -ne $Version) {
    throw "Windows executable version mismatch: file=$($windowsVersion.FileVersion), product=$($windowsVersion.ProductVersion), expected=$Version"
}
$releaseNoteMatches = @(Get-ChildItem -LiteralPath (Join-Path $repoRoot "docs") -Filter "*RELEASE_NOTES*.md" -File |
    Where-Object {
        $text = Get-Content -LiteralPath $_.FullName -Raw
        $text.Contains("Package version: ``$Version``") -and
            $text.Contains("Android version code: ``$VersionCode``")
    })
if ($releaseNoteMatches.Count -ne 1) {
    throw "Expected exactly one release-note file for $Version / Android versionCode $VersionCode; found $($releaseNoteMatches.Count)."
}
$releaseNotes = $releaseNoteMatches[0].FullName
Copy-Item -LiteralPath $windowsExe -Destination (Join-Path $windowsStage "HordeLanternRT.exe")
Copy-Item -LiteralPath (Join-Path $repoRoot "release\windows\README.txt") -Destination (Join-Path $windowsStage "README.txt")
Copy-Item -LiteralPath (Join-Path $repoRoot "ASSET_LICENSES.md") -Destination (Join-Path $windowsStage "ASSET_LICENSES.md")
Copy-Item -LiteralPath $releaseNotes -Destination (Join-Path $windowsStage "ALPHA_RELEASE_NOTES.md")

$lichLicenceEvidence = @(
    (Join-Path $repoRoot "assets\models\enemies\meshy\lich_placeholder_source_licence.md"),
    (Join-Path $repoRoot "assets\models\enemies\meshy\lich_placeholder_source_licence.png")
) | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if ($null -eq $lichLicenceEvidence) {
    throw "Lich public-package gate is unmet. Retain the original CC0 source URL or licence screenshot before packaging."
}

$assetCopies = @(
    @{ Source = "assets\models\enemies\meshy\skeleton_biped_merged_animations_v01.glb"; Destination = "assets\models\enemies\meshy" },
    @{ Source = "assets\models\enemies\meshy\lich_placeholder_merged_animations_v01.glb"; Destination = "assets\models\enemies\meshy" },
    @{ Source = "assets\textures\meshy\lich_placeholder_v01\base-color-2048-rgba8.ktx2"; Destination = "assets\textures\meshy\lich_placeholder_v01" },
    @{ Source = "assets\textures\meshy\lich_placeholder_v01\emissive-2048-rgba8.ktx2"; Destination = "assets\textures\meshy\lich_placeholder_v01" },
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

$androidSdkRoot = if (-not [string]::IsNullOrWhiteSpace($env:ANDROID_HOME)) {
    $env:ANDROID_HOME
} elseif (-not [string]::IsNullOrWhiteSpace($env:ANDROID_SDK_ROOT)) {
    $env:ANDROID_SDK_ROOT
} else {
    throw "ANDROID_HOME or ANDROID_SDK_ROOT is required to inspect the Android candidate."
}
$androidBuildTools = Join-Path $androidSdkRoot "build-tools"
$aapt2 = Find-LatestVersionedTool -Root $androidBuildTools -RelativeToolPath "aapt2.exe"
$zipalign = Find-LatestVersionedTool -Root $androidBuildTools -RelativeToolPath "zipalign.exe"
$androidResources = (& $aapt2 dump resources $androidCandidate 2>&1 | Out-String)
if ($LASTEXITCODE -ne 0) { throw "Failed to inspect Android resources in $androidCandidate" }
foreach ($creditMarker in @("string/credits_body", "Hotstrike Studio", "FilmCow", "Meshy")) {
    if ($androidResources -notmatch [regex]::Escape($creditMarker)) {
        throw "Android candidate is missing in-app credit marker: $creditMarker"
    }
}
$androidManifest = (& $aapt2 dump xmltree --file AndroidManifest.xml $androidCandidate 2>&1 | Out-String)
if ($LASTEXITCODE -ne 0) { throw "Failed to inspect the Android manifest in $androidCandidate" }
$escapedVersion = [regex]::Escape($Version)
if ($androidManifest -notmatch "versionName[^\r\n]*=`"$escapedVersion`"") {
    throw "Android candidate does not contain versionName $Version."
}
if ($androidManifest -notmatch "versionCode[^\r\n]*=$VersionCode(?:\s|$)") {
    throw "Android candidate does not contain versionCode $VersionCode."
}
if ($androidManifest -notmatch 'screenOrientation[^\r\n]*=7(?:\s|$)') {
    throw "Android candidate is not locked to sensorPortrait (screenOrientation=7)."
}

& $zipalign -c -P 16 -v 4 $androidCandidate | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw "Android candidate failed 16 KiB APK alignment verification."
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
$apkArchive = [IO.Compression.ZipFile]::OpenRead($androidCandidate)
try {
    $nativeEntries = @($apkArchive.Entries | Where-Object { $_.FullName -match '^lib/.+\.so$' } | ForEach-Object FullName)
    if ($nativeEntries.Count -lt 1) {
        throw "Android candidate does not contain a native runtime library."
    }
    if ($nativeEntries -match 'libc\+\+_shared\.so$') {
        throw "Android candidate still packages the r26 shared C++ runtime, which is not 16 KiB-page compatible."
    }
} finally {
    $apkArchive.Dispose()
}

$ndkRoot = Join-Path $androidSdkRoot "ndk"
$readelf = Find-LatestVersionedTool -Root $ndkRoot -RelativeToolPath "toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-readelf.exe"
$strippedNativeRoot = Join-Path $repoRoot "android\app\build\intermediates\stripped_native_libs\release\stripReleaseDebugSymbols\out\lib"
$releaseNativeLibraries = @(Get-ChildItem -LiteralPath $strippedNativeRoot -Recurse -Filter "*.so")
if ($releaseNativeLibraries.Count -lt 1) {
    throw "Could not find stripped Android release libraries for 16 KiB ELF verification."
}
foreach ($nativeLibrary in $releaseNativeLibraries) {
    $programHeaders = @(& $readelf -l $nativeLibrary.FullName 2>&1 | Select-String '^\s*LOAD\s')
    if ($LASTEXITCODE -ne 0 -or $programHeaders.Count -lt 1) {
        throw "Failed to inspect ELF program headers: $($nativeLibrary.FullName)"
    }
    foreach ($header in $programHeaders) {
        if ($header.Line -notmatch '\s0x4000\s*$') {
            throw "Android release library has a LOAD segment below 16 KiB alignment: $($nativeLibrary.FullName)"
        }
    }
}

$archive = [IO.Compression.ZipFile]::OpenRead($windowsZip)
try {
    $entryNames = @($archive.Entries | ForEach-Object FullName)
    foreach ($required in @(
        "HordeLanternRT.exe",
        "README.txt",
        "ASSET_LICENSES.md",
        "assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb",
        "assets/models/enemies/meshy/lich_placeholder_merged_animations_v01.glb",
        "assets/textures/meshy/lich_placeholder_v01/base-color-2048-rgba8.ktx2",
        "assets/textures/meshy/lich_placeholder_v01/emissive-2048-rgba8.ktx2",
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
