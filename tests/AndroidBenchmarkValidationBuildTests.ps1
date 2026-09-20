[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$androidRoot = Join-Path $repoRoot 'android'
$gradle = Join-Path $androidRoot 'gradlew.bat'

function Assert-True([bool]$condition, [string]$message) {
    if (-not $condition) { throw $message }
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
            if (Test-Path -LiteralPath $candidate -PathType Leaf) {
                [PSCustomObject]@{ Version = $version; Path = $candidate }
            }
        }
    }
    $selected = $matches | Sort-Object Version -Descending | Select-Object -First 1
    if ($null -eq $selected) { throw "Could not find $RelativeToolPath beneath $Root" }
    $selected.Path
}

function Invoke-GradleText {
    param([Parameter(Mandatory = $true)][string[]]$Arguments)
    Push-Location $androidRoot
    try {
        $ErrorActionPreference = 'Continue'
        $output = (& .\gradlew.bat @Arguments 2>&1 | Out-String)
        [PSCustomObject]@{ Output = $output; ExitCode = $LASTEXITCODE }
    }
    finally {
        Pop-Location
    }
}

Assert-True (Test-Path -LiteralPath $gradle -PathType Leaf) 'Android Gradle wrapper is missing.'

$previousValidationUnsigned = $env:HORDE_VALIDATION_UNSIGNED
$env:HORDE_VALIDATION_UNSIGNED = '1'
try {
    $tasksRun = Invoke-GradleText @('--no-daemon', ':app:tasks', '--all', '--console=plain')
    Assert-True ($tasksRun.ExitCode -eq 0) 'Gradle task inventory failed without benchmark opt-in.'
    Assert-True ($tasksRun.Output -notmatch '(?m)^assembleBenchmark\b') `
        'Benchmark variant must not be registered without hordeBenchmarkValidation=true.'

    $rejectedPolicy = Invoke-GradleText @('--no-daemon', '-PhordeBenchmarkValidation=true',
        '-PhordeRtInstrumentationOverride=Diagnostic', '-PhordeRtDielectricQualityOverride=Mobile',
        ':app:tasks', '--console=plain')
    Assert-True ($rejectedPolicy.ExitCode -ne 0 -and $rejectedPolicy.Output -match 'requires Shipping/Mobile') `
        'Benchmark policy accepted a non-Shipping instrumentation override.'
    $rejectedQuality = Invoke-GradleText @('--no-daemon', '-PhordeBenchmarkValidation=true',
        '-PhordeRtInstrumentationOverride=Shipping', '-PhordeRtDielectricQualityOverride=High',
        ':app:tasks', '--console=plain')
    Assert-True ($rejectedQuality.ExitCode -ne 0 -and $rejectedQuality.Output -match 'requires Shipping/Mobile') `
        'Benchmark policy accepted a non-Mobile dielectric override.'

    $buildRun = Invoke-GradleText @('--no-daemon', '-PhordeBenchmarkValidation=true',
        ':app:assembleBenchmark', '--console=plain')
    Assert-True ($buildRun.ExitCode -eq 0) "Benchmark APK build failed:`n$($buildRun.Output)"

    $apkDirectory = Join-Path $androidRoot 'app\build\outputs\apk\benchmark'
    $apks = @(Get-ChildItem -LiteralPath $apkDirectory -Filter '*.apk' -File)
    Assert-True ($apks.Count -eq 1) "Expected one benchmark APK in $apkDirectory; found $($apks.Count)."
    $apk = $apks[0].FullName

    $sdkRoot = if (-not [string]::IsNullOrWhiteSpace($env:ANDROID_HOME)) {
        $env:ANDROID_HOME
    } elseif (-not [string]::IsNullOrWhiteSpace($env:ANDROID_SDK_ROOT)) {
        $env:ANDROID_SDK_ROOT
    } else {
        throw 'ANDROID_HOME or ANDROID_SDK_ROOT is required for APK policy inspection.'
    }
    $buildTools = Join-Path $sdkRoot 'build-tools'
    $aapt2 = Find-LatestVersionedTool $buildTools 'aapt2.exe'
    $apksigner = Find-LatestVersionedTool $buildTools 'apksigner.bat'
    $badging = (& $aapt2 dump badging $apk 2>&1 | Out-String)
    Assert-True ($LASTEXITCODE -eq 0) 'aapt2 could not inspect the benchmark APK.'
    Assert-True ($badging -match "package: name='com\.samfa12\.hordelanternrt\.benchmark'") `
        'Benchmark APK package suffix is not isolated.'
    Assert-True ($badging -match "versionName='[^']*-benchmark'") `
        'Benchmark APK version name suffix is missing.'
    Assert-True ($badging -notmatch 'application-debuggable') `
        'Benchmark APK must be non-debuggable.'

    $certs = (& $apksigner verify --print-certs $apk 2>&1 | Out-String)
    Assert-True ($LASTEXITCODE -eq 0 -and $certs -match 'CN=Android Debug') `
        'Benchmark APK must use the development debug certificate, not production signing.'

    $cxxRoot = Join-Path $androidRoot 'app\build\intermediates\cxx'
    $buildModels = @(Get-ChildItem -LiteralPath $cxxRoot -Recurse -Filter 'build_model.json' -File |
        ForEach-Object {
            $model = Get-Content -LiteralPath $_.FullName -Raw | ConvertFrom-Json
            if ($model.variant.variantName -eq 'benchmark') {
                [PSCustomObject]@{ Path = $_.FullName; Model = $model }
            }
        })
    Assert-True ($buildModels.Count -eq 4) `
        "Expected one exact benchmark native model per ABI; found $($buildModels.Count)."
    Assert-True ((@($buildModels | ForEach-Object { $_.Model.info.name } | Sort-Object) -join ',') -ceq `
        'arm64-v8a,armeabi-v7a,x86,x86_64') `
        'Benchmark native metadata does not cover exactly the four configured ABIs.'
    foreach ($entry in $buildModels) {
        $model = $entry.Model
        Assert-True (-not [bool]$model.variant.isDebuggableEnabled) `
            "Benchmark native model is debuggable: $($entry.Path)"
        Assert-True ($model.variant.optimizationTag -eq 'RelWithDebInfo') `
            "Benchmark native model is not Release-derived: $($entry.Path)"
        Assert-True ($model.variant.buildSystemArgumentList -contains '-DHORDE_RT_DEBUG_CHECKPOINTS=OFF') `
            "Benchmark native model lacks checkpoints OFF: $($entry.Path)"
        Assert-True ($model.variant.buildSystemArgumentList -contains '-DHORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION=Shipping') `
            "Benchmark native model lacks Shipping instrumentation: $($entry.Path)"
        Assert-True ($model.variant.buildSystemArgumentList -contains '-DHORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY=Mobile') `
            "Benchmark native model lacks Mobile quality: $($entry.Path)"
        Assert-True ($model.soRepublishFolder -match '\\build\\intermediates\\cmake\\benchmark\\') `
            "Benchmark native model is not bound to benchmark packaging: $($entry.Path)"
        $cachePath = Join-Path $model.cxxBuildFolder 'CMakeCache.txt'
        Assert-True (Test-Path -LiteralPath $cachePath -PathType Leaf) `
            "Benchmark CMake cache is missing: $cachePath"
        $cache = Get-Content -LiteralPath $cachePath -Raw
        Assert-True ($cache.Contains('CMAKE_BUILD_TYPE:STRING=RelWithDebInfo')) `
            "Benchmark CMake cache is not RelWithDebInfo: $cachePath"
        Assert-True ($cache.Contains('HORDE_RT_DEBUG_CHECKPOINTS:BOOL=OFF')) `
            "Benchmark CMake cache lacks checkpoints OFF: $cachePath"
        Assert-True ($cache.Contains('HORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION:UNINITIALIZED=Shipping')) `
            "Benchmark CMake cache lacks Shipping instrumentation: $cachePath"
        Assert-True ($cache.Contains('HORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY:UNINITIALIZED=Mobile')) `
            "Benchmark CMake cache lacks Mobile quality: $cachePath"
    }

    $strippedLibraries = @(Get-ChildItem -LiteralPath (Join-Path $androidRoot 'app\build\intermediates\stripped_native_libs\benchmark') `
        -Recurse -Filter 'libhorde_rt_probe_android.so' -File -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match '\\arm64-v8a\\' })
    Assert-True ($strippedLibraries.Count -eq 1) `
        "Expected one stripped benchmark ARM64 library; found $($strippedLibraries.Count)."
    $containmentScript = Join-Path $repoRoot 'tools\InspectAndroidRtPipelineBundlePackage.ps1'
    $scannerScript = Join-Path $repoRoot 'tools\InspectRtPipelineBundleContainment.ps1'
    $scannerHost = (Get-Command pwsh.exe -ErrorAction SilentlyContinue).Source
    if ([string]::IsNullOrWhiteSpace($scannerHost)) { $scannerHost = (Get-Command powershell.exe).Source }
    $containmentOutput = (& $scannerHost -NoProfile -ExecutionPolicy Bypass -File $containmentScript `
        -Scanner $scannerScript -StrippedLibraryPath $strippedLibraries[0].FullName -ApkPath $apk `
        -Instrumentation Shipping -Quality Mobile 2>&1 | Out-String)
    Assert-True ($LASTEXITCODE -eq 0) "Packaged ARM64 containment scan failed:`n$containmentOutput"
    $reportsRoot = Join-Path $repoRoot 'reports'
    New-Item -ItemType Directory -Path $reportsRoot -Force | Out-Null
    $scannerReportPath = Join-Path $reportsRoot (
        'android-benchmark-containment-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.json')
    [IO.File]::WriteAllText($scannerReportPath, $containmentOutput.Trim() + [Environment]::NewLine)
    $containment = $containmentOutput | ConvertFrom-Json
    Assert-True ($containment.arm64Sha256 -and
                 $containment.stripped.targetSha256 -eq $containment.packaged.targetSha256) `
        'Packaged ARM64 containment scan did not prove byte-identical Shipping/Mobile output.'

    Write-Output "Android benchmark validation build passed: $apk"
    Write-Output "Containment JSON: $scannerReportPath"
}
finally {
    if ($null -eq $previousValidationUnsigned) {
        Remove-Item Env:HORDE_VALIDATION_UNSIGNED -ErrorAction SilentlyContinue
    } else {
        $env:HORDE_VALIDATION_UNSIGNED = $previousValidationUnsigned
    }
}
