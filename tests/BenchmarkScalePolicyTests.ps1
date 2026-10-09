[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$CmakeExecutable,
    [string]$GradleExecutable = '')
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$tempBase = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$temporaryRoot = Join-Path $tempBase ('horde-benchmark-scale-' + [guid]::NewGuid().ToString('N'))
function Invoke-Case([string]$program, [string[]]$arguments, [string]$directory) {
    Push-Location $directory
    try {
        $saved = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
        try { $output = & $program @arguments 2>&1; $code = $LASTEXITCODE }
        finally { $ErrorActionPreference = $saved }
        return ,@($code, ($output -join "`n"))
    } finally { Pop-Location }
}
function Assert-Case([string]$name, $result, [bool]$succeeds, [string]$witness) {
    if (($result[0] -eq 0) -ne $succeeds -or !$result[1].Contains($witness)) {
        throw "Scale admission case $name failed its expected result/witness: $($result[1])"
    }
}
try {
    New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
    $module = (Join-Path $repoRoot 'android/app/src/main/cpp/benchmark_scale_policy.cmake').Replace('\','/')
    $script = Join-Path $temporaryRoot 'admission.cmake'
    [IO.File]::WriteAllText($script, "include([=[$module]=])`nmessage(`"scale-min=`${HORDE_RT_MIN_RENDER_SCALE_PERCENT}`")`n", [Text.UTF8Encoding]::new($false))
    $baseline = @('-DHORDE_RT_MIN_RENDER_SCALE_PERCENT=33', '-DHORDE_RT_ANDROID_BENCHMARK_VALIDATION=ON',
        '-DCMAKE_BUILD_TYPE=RelWithDebInfo', '-DHORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION=Shipping',
        '-DHORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY=Mobile', '-DHORDE_RT_DEBUG_CHECKPOINTS=OFF',
        '-DHORDE_RT_DEBUG_VIEWMODEL_CANDIDATE=OFF', '-DHORDE_RT_STAGED_PRIMARY_SHADER_DIR=',
        '-DHORDE_RT_STAGED_PRIMARY_DEFAULT=OFF', '-DHORDE_RT_STAGED_PRIMARY_TIMING=OFF')
    Assert-Case 'ordinary-default' (Invoke-Case $CmakeExecutable @('-P',$script) $temporaryRoot) $true 'scale-min=33'
    Assert-Case 'ordinary-compatibility50' (Invoke-Case $CmakeExecutable @('-DHORDE_RT_MIN_RENDER_SCALE_PERCENT=50','-P',$script) $temporaryRoot) $true 'scale-min=50'
    Assert-Case 'ordinary-manual33-debug' (Invoke-Case $CmakeExecutable @('-DHORDE_RT_MIN_RENDER_SCALE_PERCENT=33',
        '-DHORDE_RT_ANDROID_BENCHMARK_VALIDATION=OFF','-DCMAKE_BUILD_TYPE=Debug','-P',$script) $temporaryRoot) $true 'scale-min=33'
    Assert-Case 'explicit-benchmark' (Invoke-Case $CmakeExecutable ($baseline + @('-P',$script)) $temporaryRoot) $true 'scale-min=33'
    foreach ($negative in @(
        '-DCMAKE_BUILD_TYPE=Debug', '-DCMAKE_BUILD_TYPE=',
        '-DHORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION=Diagnostic', '-DHORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY=High',
        '-DHORDE_RT_INSTRUMENTATION_OVERRIDE=Diagnostic', '-DHORDE_RT_DIELECTRIC_QUALITY_OVERRIDE=High',
        '-DHORDE_RT_DEBUG_CHECKPOINTS=ON', '-DHORDE_RT_DEBUG_VIEWMODEL_CANDIDATE=ON',
        '-DHORDE_RT_STAGED_PRIMARY_SHADER_DIR=unadmitted', '-DHORDE_RT_STAGED_PRIMARY_DEFAULT=ON',
        '-DHORDE_RT_STAGED_PRIMARY_TIMING=ON')) {
        Assert-Case $negative (Invoke-Case $CmakeExecutable ($baseline + @($negative,'-P',$script)) $temporaryRoot) $false 'Sub-50 scales require'
    }
    $timing = $baseline + @('-DHORDE_RT_MIN_RENDER_SCALE_PERCENT=50', '-DHORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION=ON')
    Assert-Case 'timing-benchmark50' (Invoke-Case $CmakeExecutable ($timing + @('-P',$script)) $temporaryRoot) $true 'scale-min=50'
    foreach ($negative in @('-DHORDE_RT_ANDROID_BENCHMARK_VALIDATION=OFF', '-DCMAKE_BUILD_TYPE=Debug',
        '-DHORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION=Diagnostic', '-DHORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY=High',
        '-DHORDE_RT_STAGED_PRIMARY_DEFAULT=ON', '-DHORDE_RT_DEBUG_VIEWMODEL_CANDIDATE=ON')) {
        Assert-Case "timing-$negative" (Invoke-Case $CmakeExecutable ($timing + @($negative,'-P',$script)) $temporaryRoot) $false 'Presentation timing requires isolated'
    }
    $motion = $baseline + @('-DHORDE_RT_MIN_RENDER_SCALE_PERCENT=33',
        '-DHORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION=ON','-DHORDE_RT_ANDROID_MOTION_VALIDATION=ON')
    Assert-Case 'motion-isolated' (Invoke-Case $CmakeExecutable ($motion + @('-P',$script)) $temporaryRoot) $true 'scale-min=33'
    foreach ($negative in @('-DHORDE_RT_ANDROID_PRESENT_TIMING_VALIDATION=OFF',
        '-DHORDE_RT_ANDROID_BENCHMARK_VALIDATION=OFF','-DHORDE_RT_MIN_RENDER_SCALE_PERCENT=50',
        '-DCMAKE_BUILD_TYPE=Debug','-DHORDE_RT_DEBUG_CHECKPOINTS=ON')) {
        Assert-Case "motion-$negative" (Invoke-Case $CmakeExecutable ($motion + @($negative,'-P',$script)) $temporaryRoot) $false 'Motion validation requires the isolated'
    }
    foreach ($invalid in @('32','34','39','40','41','49','033','33.0','33;50')) {
        Assert-Case "invalid-$invalid" (Invoke-Case $CmakeExecutable ($baseline + @("-DHORDE_RT_MIN_RENDER_SCALE_PERCENT=$invalid",'-P',$script)) $temporaryRoot) $false 'Render scale minimum must be exactly'
    }
    if ($GradleExecutable) {
        # No assembly/configuration of native targets; exercise the actual Gradle
        # application DSL guard, preserving developer signing and Debug data.
        $android = Join-Path $repoRoot 'android'
        $neutral = @('-PhordeBenchmarkValidation=false', '-PhordeBenchmarkMotionValidation=false', '-PhordeStagedPrimaryDebugValidation=false',
            '-PhordeStagedPrimaryTiming=false', '-PhordeRtInstrumentationOverride=',
            '-PhordeRtDielectricQualityOverride=', '-PhordeViewmodelCandidateDir=')
        function Gradle-Case([string[]]$properties) {
            return Invoke-Case $GradleExecutable (@('--no-daemon','--console=plain','-q',
                ':app:printHordeRtPolicyForTest') + $neutral + $properties) $android
        }
        Assert-Case 'gradle-benchmark33' (Gradle-Case @('-PhordeBenchmarkValidation=true','-PhordeBenchmarkMinRenderScale=33')) $true 'debugMinRenderScale=33|releaseMinRenderScale=33|benchmarkMinRenderScale=33'
        Assert-Case 'gradle-timing-benchmark' (Gradle-Case @('-PhordeBenchmarkValidation=true','-PhordePresentTimingValidation=true')) $true 'ordinaryPresentTiming=OFF|benchmarkPresentTiming=ON'
        Assert-Case 'gradle-timing-outside' (Gradle-Case @('-PhordePresentTimingValidation=true')) $false 'requires the isolated Shipping/Mobile'
        Assert-Case 'gradle-timing-invalid' (Gradle-Case @('-PhordePresentTimingValidation=yes')) $false 'must be true or false'
        Assert-Case 'gradle-motion' (Gradle-Case @('-PhordeBenchmarkValidation=true','-PhordePresentTimingValidation=true',
            '-PhordeBenchmarkMinRenderScale=33','-PhordeBenchmarkMotionValidation=true')) $true 'benchmarkMotionValidation=ON'
        Assert-Case 'gradle-motion-requires-timing' (Gradle-Case @('-PhordeBenchmarkValidation=true',
            '-PhordeBenchmarkMinRenderScale=33','-PhordeBenchmarkMotionValidation=true')) $false 'requires isolated benchmark validation and VK_GOOGLE_display_timing'
        Assert-Case 'gradle-motion-requires-33' (Gradle-Case @('-PhordeBenchmarkValidation=true',
            '-PhordePresentTimingValidation=true','-PhordeBenchmarkMotionValidation=true')) $false 'requires hordeBenchmarkMinRenderScale=33'
        Assert-Case 'gradle-outside-benchmark' (Gradle-Case @('-PhordeBenchmarkMinRenderScale=33')) $false 'requires explicit isolated'
        foreach ($invalid in @('34','40','50','033','33.0')) {
            Assert-Case "gradle-invalid-$invalid" (Gradle-Case @('-PhordeBenchmarkValidation=true',"-PhordeBenchmarkMinRenderScale=$invalid")) $false 'requires explicit isolated'
        }
        Assert-Case 'gradle-staged-timing' (Gradle-Case @('-PhordeBenchmarkValidation=true','-PhordeBenchmarkMinRenderScale=33','-PhordeStagedPrimaryTiming=true')) $false 'requires the explicit staged benchmark'
        Assert-Case 'gradle-viewmodel' (Gradle-Case @('-PhordeBenchmarkValidation=true','-PhordeBenchmarkMinRenderScale=33','-PhordeViewmodelCandidateDir=unadmitted')) $false 'require the unchanged Shipping/Mobile'
        Assert-Case 'gradle-diagnostic' (Gradle-Case @('-PhordeBenchmarkValidation=true','-PhordeBenchmarkMinRenderScale=33','-PhordeRtInstrumentationOverride=Diagnostic','-PhordeRtDielectricQualityOverride=Mobile')) $false 'requires Shipping/Mobile native policy'
    }
} finally {
    # Keep recursive cleanup confined to the single generated test directory.
    $resolved = [IO.Path]::GetFullPath($temporaryRoot)
    $prefix = $tempBase.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
    if (!$resolved.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFileName($resolved) -notlike 'horde-benchmark-scale-*') {
        throw 'Scale fixture cleanup escaped its generated temporary directory.'
    }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
}
Write-Output 'Benchmark scale CMake admission contracts passed; optional Gradle cases ran only when supplied.'
