[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$CmakeExecutable,
    [Parameter(Mandatory = $true)][string]$GradleExecutable)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
function Assert-True([bool]$condition, [string]$message) { if (-not $condition) { throw $message } }
function Write-Utf8NoBom([string]$path, [string]$contents) {
    [IO.File]::WriteAllText($path, $contents, [Text.UTF8Encoding]::new($false))
}
function Invoke-Native([string]$file, [string[]]$arguments, [string]$workingDirectory) {
    Push-Location $workingDirectory
    try {
        # Windows PowerShell turns a native stderr warning into a terminating
        # NativeCommandError under the script-wide Stop policy.  Keep captured
        # stderr diagnostic-only and make the native exit code authoritative.
        $previousErrorActionPreference = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        try { $output = & $file @arguments 2>&1; $exitCode = $LASTEXITCODE }
        finally { $ErrorActionPreference = $previousErrorActionPreference }
        return ,@($exitCode, ($output -join "`n"))
    }
    finally { Pop-Location }
}

$repoForCmake = $repoRoot.Replace('\', '/')
$temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) ('horde-rt-policy-execution-' + [guid]::NewGuid().ToString('N'))
$savedInstrumentationEnvironment = $env:ORG_GRADLE_PROJECT_hordeRtInstrumentationOverride
$savedQualityEnvironment = $env:ORG_GRADLE_PROJECT_hordeRtDielectricQualityOverride
Remove-Item Env:ORG_GRADLE_PROJECT_hordeRtInstrumentationOverride -ErrorAction SilentlyContinue
Remove-Item Env:ORG_GRADLE_PROJECT_hordeRtDielectricQualityOverride -ErrorAction SilentlyContinue
try {
    New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
    $fixtureSource = @'
#include "vulkan/raytracing/RtPipelineVariantCatalog.generated.h"
int rt_policy_fixture() { return 0; }
'@
    $fixtureCmake = @"
cmake_minimum_required(VERSION 3.22)
project(HordeRtPolicyFixture LANGUAGES CXX)
include("$repoForCmake/cmake/HordeRtRaygenPolicy.cmake")
add_library(rt_policy_fixture STATIC fixture.cpp)
target_compile_features(rt_policy_fixture PRIVATE cxx_std_20)
target_include_directories(rt_policy_fixture PRIVATE "$repoForCmake/src")
horde_rt_apply_raygen_policy(rt_policy_fixture "`$`{HORDE_RT_POLICY_PLATFORM`}")
add_library(rt_policy_treewide_fixture STATIC fixture.cpp)
target_compile_features(rt_policy_treewide_fixture PRIVATE cxx_std_20)
target_include_directories(rt_policy_treewide_fixture PRIVATE "$repoForCmake/src")
horde_rt_apply_raygen_policy(rt_policy_treewide_fixture "`$`{HORDE_RT_POLICY_PLATFORM`}")
file(GENERATE OUTPUT "`$`{CMAKE_BINARY_DIR}/definitions-`$<CONFIG>.txt" CONTENT "`$<JOIN:`$<TARGET_PROPERTY:rt_policy_fixture,COMPILE_DEFINITIONS>,\\n>")
file(GENERATE OUTPUT "`$`{CMAKE_BINARY_DIR}/treewide-definitions-`$<CONFIG>.txt" CONTENT "`$<JOIN:`$<TARGET_PROPERTY:rt_policy_treewide_fixture,COMPILE_DEFINITIONS>,\\n>")
"@
    Write-Utf8NoBom (Join-Path $temporaryRoot 'fixture.cpp') $fixtureSource
    Write-Utf8NoBom (Join-Path $temporaryRoot 'CMakeLists.txt') $fixtureCmake

    $generatorArguments = if ($env:OS -eq 'Windows_NT') { @('-G', 'Visual Studio 17 2022', '-A', 'x64') } else { @('-G', 'Ninja') }
    function Invoke-PolicyCase([string]$name, [string]$platform, [string]$configuration, [string[]]$cmakeArguments,
        [string[]]$expectedDefinitions, [bool]$expectConfigureFailure = $false, [bool]$expectBuildFailure = $false) {
        $buildDirectory = Join-Path $temporaryRoot $name
        $arguments = @('-S', $temporaryRoot, '-B', $buildDirectory) + $generatorArguments + @("-DHORDE_RT_POLICY_PLATFORM=$platform") + $cmakeArguments
        $result = Invoke-Native $CmakeExecutable $arguments $temporaryRoot
        if ($expectConfigureFailure) {
            Assert-True ($result[0] -ne 0) "Policy case $name unexpectedly configured."
            return
        }
        Assert-True ($result[0] -eq 0) "Policy case $name failed to configure: $($result[1])"
        $definitions = @((Get-Content -LiteralPath (Join-Path $buildDirectory "definitions-$configuration.txt") -Raw), (Get-Content -LiteralPath (Join-Path $buildDirectory "treewide-definitions-$configuration.txt") -Raw))
        foreach ($definition in $expectedDefinitions) {
            foreach ($targetDefinitions in $definitions) {
                Assert-True ($targetDefinitions.Contains($definition)) "Policy case $name missed tree-wide definition: $definition; got: $targetDefinitions"
            }
        }
        $build = Invoke-Native $CmakeExecutable @('--build', $buildDirectory, '--config', $configuration) $temporaryRoot
        if ($expectBuildFailure) {
            Assert-True ($build[0] -ne 0) "Policy case $name unexpectedly built."
        }
        else {
            Assert-True ($build[0] -eq 0) "Policy case $name failed to build: $($build[1])"
        }
    }
    function Invoke-WindowsOverrideTreewideCase {
        $name = 'windows-override-treewide'
        $buildDirectory = Join-Path $temporaryRoot $name
        $arguments = @('-S', $temporaryRoot, '-B', $buildDirectory) + $generatorArguments + @(
            '-DHORDE_RT_POLICY_PLATFORM=Windows',
            '-DHORDE_RT_INSTRUMENTATION_OVERRIDE=Diagnostic',
            '-DHORDE_RT_DIELECTRIC_QUALITY_OVERRIDE=Mobile')
        $result = Invoke-Native $CmakeExecutable $arguments $temporaryRoot
        Assert-True ($result[0] -eq 0) "Policy case $name failed to configure: $($result[1])"
        foreach ($configuration in @('Debug', 'Release')) {
            $definitions = @(
                (Get-Content -LiteralPath (Join-Path $buildDirectory "definitions-$configuration.txt") -Raw),
                (Get-Content -LiteralPath (Join-Path $buildDirectory "treewide-definitions-$configuration.txt") -Raw))
            foreach ($targetDefinitions in $definitions) {
                Assert-True ($targetDefinitions.Contains('HORDE_RT_SELECTED_INSTRUMENTATION=1')) "Policy case $name missed Debug/Release tree-wide Diagnostic override."
                Assert-True ($targetDefinitions.Contains('HORDE_RT_SELECTED_DIELECTRIC_QUALITY=0')) "Policy case $name missed Debug/Release tree-wide Mobile override."
            }
            $build = Invoke-Native $CmakeExecutable @('--build', $buildDirectory, '--config', $configuration) $temporaryRoot
            Assert-True ($build[0] -eq 0) "Policy case $name failed to build ${configuration}: $($build[1])"
        }
    }

    Invoke-PolicyCase 'windows-debug' 'Windows' 'Debug' @() @('HORDE_RT_SELECTED_INSTRUMENTATION=1', 'HORDE_RT_SELECTED_DIELECTRIC_QUALITY=1')
    Invoke-PolicyCase 'windows-release' 'Windows' 'Release' @() @('HORDE_RT_SELECTED_INSTRUMENTATION=0', 'HORDE_RT_SELECTED_DIELECTRIC_QUALITY=1')
    Invoke-PolicyCase 'windows-unsupported' 'Windows' 'RelWithDebInfo' @() @('HORDE_RT_POLICY_UNSUPPORTED_CONFIGURATION=1') $false $true
    Invoke-PolicyCase 'android-debug-default' 'Android' 'Debug' @('-DHORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION=Diagnostic', '-DHORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY=Mobile') @('HORDE_RT_SELECTED_INSTRUMENTATION=1', 'HORDE_RT_SELECTED_DIELECTRIC_QUALITY=0')
    Invoke-PolicyCase 'android-release-default' 'Android' 'Release' @('-DHORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION=Shipping', '-DHORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY=Mobile') @('HORDE_RT_SELECTED_INSTRUMENTATION=0', 'HORDE_RT_SELECTED_DIELECTRIC_QUALITY=0')
    Invoke-PolicyCase 'android-override-precedence' 'Android' 'Release' @('-DHORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION=Shipping', '-DHORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY=Mobile', '-DHORDE_RT_INSTRUMENTATION_OVERRIDE=Diagnostic', '-DHORDE_RT_DIELECTRIC_QUALITY_OVERRIDE=High') @('HORDE_RT_SELECTED_INSTRUMENTATION=1', 'HORDE_RT_SELECTED_DIELECTRIC_QUALITY=1')
    Invoke-WindowsOverrideTreewideCase
    Invoke-PolicyCase 'partial-override' 'Windows' 'Debug' @('-DHORDE_RT_INSTRUMENTATION_OVERRIDE=Shipping') @() $true
    Invoke-PolicyCase 'invalid-override-case' 'Windows' 'Debug' @('-DHORDE_RT_INSTRUMENTATION_OVERRIDE=shipping', '-DHORDE_RT_DIELECTRIC_QUALITY_OVERRIDE=Mobile') @() $true
    Invoke-PolicyCase 'invalid-quality' 'Windows' 'Debug' @('-DHORDE_RT_INSTRUMENTATION_OVERRIDE=Shipping', '-DHORDE_RT_DIELECTRIC_QUALITY_OVERRIDE=mobile') @() $true
    Invoke-PolicyCase 'missing-android-defaults' 'Android' 'Debug' @() @() $true
    Invoke-PolicyCase 'unknown-platform' 'Unknown' 'Debug' @() @() $true

    function Invoke-GradlePolicyCase([string]$name, [string[]]$properties, [bool]$expectFailure = $false,
        [string]$expectedDiagnostic = '') {
        # Explicit empty CLI properties defeat any developer-local gradle.properties
        # values while retaining the established dependency cache (no network lane).
        $neutralProperties = @()
        if (@($properties | Where-Object { $_ -like '-PhordeRtInstrumentationOverride=*' }).Count -eq 0) { $neutralProperties += '-PhordeRtInstrumentationOverride=' }
        if (@($properties | Where-Object { $_ -like '-PhordeRtDielectricQualityOverride=*' }).Count -eq 0) { $neutralProperties += '-PhordeRtDielectricQualityOverride=' }
        $arguments = @('--offline', '--no-daemon', '--console=plain', '-q', ':app:printHordeRtPolicyForTest') + $neutralProperties + $properties
        $result = Invoke-Native $GradleExecutable $arguments (Join-Path $repoRoot 'android')
        if ($expectFailure) {
            Assert-True ($result[0] -ne 0) "Gradle policy case $name unexpectedly succeeded."
            if ($expectedDiagnostic) {
                Assert-True ($result[1].Contains($expectedDiagnostic)) "Gradle policy case $name failed for the wrong reason: $($result[1])"
            }
            return
        }
        Assert-True ($result[0] -eq 0) "Gradle policy case $name failed: $($result[1])"
        return $result[1]
    }

    $gradleDefault = Invoke-GradlePolicyCase 'defaults' @()
    Assert-True ($gradleDefault.Contains('debug=-DHORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION=Diagnostic|-DHORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY=Mobile')) 'Gradle Debug default policy is incorrect.'
    Assert-True ($gradleDefault.Contains('release=-DHORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION=Shipping|-DHORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY=Mobile')) 'Gradle Release default policy is incorrect.'
    $observersOff = 'debugObservation=-DHORDE_RT_ANDROID_CHAPTER_ROUTE_TIMING=OFF|-DHORDE_RT_ANDROID_OUTDOOR_SCALE_PROBE=OFF|-DHORDE_RT_FRAME_STAGE_PROBE=OFF|-DHORDE_RT_DISPATCH_CLOCK_PROBE=OFF'
    Assert-True ($gradleDefault.Contains($observersOff)) 'Ordinary Debug must compile all route/stage/clock observers out by default.'
    $gradleOverride = Invoke-GradlePolicyCase 'override-precedence' @('-PhordeRtInstrumentationOverride=Diagnostic', '-PhordeRtDielectricQualityOverride=High')
    Assert-True ($gradleOverride.Contains('debug=-DHORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION=Diagnostic|-DHORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY=High')) 'Gradle override did not apply to Debug.'
    Assert-True ($gradleOverride.Contains('release=-DHORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION=Diagnostic|-DHORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY=High')) 'Gradle override did not take precedence over Release default.'
    Invoke-GradlePolicyCase 'partial-override' @('-PhordeRtInstrumentationOverride=Shipping') $true
    Invoke-GradlePolicyCase 'invalid-override-case' @('-PhordeRtInstrumentationOverride=shipping', '-PhordeRtDielectricQualityOverride=Mobile') $true
    Invoke-GradlePolicyCase 'invalid-quality' @('-PhordeRtInstrumentationOverride=Shipping', '-PhordeRtDielectricQualityOverride=mobile') $true

    # Exercise actual Gradle configuration, without compiling/substituting a shader
    # or using a device. The reporting task shares the Debug native argument list.
    $shippingMobile = @('-PhordeRtInstrumentationOverride=Shipping', '-PhordeRtDielectricQualityOverride=Mobile')
    $route = $shippingMobile + '-PhordeRtRouteTiming=true'
    $stages = $route + '-PhordeFrameStageProbe=true'
    $productionStages = Invoke-GradlePolicyCase 'production-bank-stages' $stages
    Assert-True ($productionStages.Contains('debugObservation=-DHORDE_RT_ANDROID_CHAPTER_ROUTE_TIMING=ON|-DHORDE_RT_ANDROID_OUTDOOR_SCALE_PROBE=OFF|-DHORDE_RT_FRAME_STAGE_PROBE=ON|-DHORDE_RT_DISPATCH_CLOCK_PROBE=OFF')) 'Production stage-only profile must enable timestamps without shader clocks.'
    $productionDebugPolicy = @($productionStages -split '\r?\n' | Where-Object { $_.StartsWith('debug=') })
    Assert-True ($productionDebugPolicy.Count -eq 1) 'Production stage-only profile must expose one Debug native policy.'
    $productionRoots = @($productionDebugPolicy[0].Substring(6).Split('|') | Where-Object { $_.StartsWith('-DHORDE_RT_REGION_PAGE_PROTOTYPE_ROOT=') })
    Assert-True ($productionRoots.Count -eq 1 -and $productionRoots[0] -ceq '-DHORDE_RT_REGION_PAGE_PROTOTYPE_ROOT=') 'Production stage-only profile must retain the empty private root.'
    $explicitOff = Invoke-GradlePolicyCase 'observers-explicit-off' @('-PhordeRtRouteTiming=false', '-PhordeFrameStageProbe=false', '-PhordeDispatchClockProbe=false')
    Assert-True ($explicitOff.Contains($observersOff)) 'Explicit false must leave route/stage/clock observers disabled.'

    Invoke-GradlePolicyCase 'stages-without-route' ($shippingMobile + '-PhordeFrameStageProbe=true') $true 'Frame stage profiling requires separate Debug chapter route timing'
    Invoke-GradlePolicyCase 'stages-malformed' ($route + '-PhordeFrameStageProbe=True') $true 'hordeFrameStageProbe must be true or false.'
    Invoke-GradlePolicyCase 'clocks-malformed' ($stages + '-PhordeDispatchClockProbe=1') $true 'hordeDispatchClockProbe must be true or false.'
    Invoke-GradlePolicyCase 'stages-diagnostic' @('-PhordeRtInstrumentationOverride=Diagnostic', '-PhordeRtDielectricQualityOverride=Mobile', '-PhordeRtRouteTiming=true', '-PhordeFrameStageProbe=true') $true 'Route timing requires Shipping/Mobile Debug'
    Invoke-GradlePolicyCase 'stages-high' @('-PhordeRtInstrumentationOverride=Shipping', '-PhordeRtDielectricQualityOverride=High', '-PhordeRtRouteTiming=true', '-PhordeFrameStageProbe=true') $true 'Route timing requires Shipping/Mobile Debug'
    Invoke-GradlePolicyCase 'conflicting-route-aliases' ($stages + '-PhordeRegionPageRouteTiming=false') $true 'Conflicting route timing properties.'
    Invoke-GradlePolicyCase 'stages-benchmark' ($stages + '-PhordeBenchmarkValidation=true') $true 'Frame stage profiling requires separate Debug chapter route timing'
    Invoke-GradlePolicyCase 'stages-staged-debug' @('-PhordeRtRouteTiming=true', '-PhordeFrameStageProbe=true', '-PhordeStagedPrimaryDebugValidation=true') $true 'Staged Debug validation requires explicit Diagnostic/Mobile artifacts'
    Invoke-GradlePolicyCase 'stages-viewmodel' ($stages + "-PhordeViewmodelCandidateDir=$temporaryRoot") $true 'Frame stage profiling requires separate Debug chapter route timing'
    Invoke-GradlePolicyCase 'clocks-without-stages' ($route + '-PhordeDispatchClockProbe=true') $true 'Dispatch clock observation requires the private frame stage/clock shader profile.'
    Invoke-GradlePolicyCase 'production-bank-clocks' ($stages + '-PhordeDispatchClockProbe=true') $true 'Dispatch clock observation requires the private frame stage/clock shader profile.'
    Invoke-GradlePolicyCase 'relative-private-root' ($stages + '-PhordeRegionPagePrototypeRoot=relative-root') $true 'hordeRegionPagePrototypeRoot requires an absolute private directory'
    Invoke-GradlePolicyCase 'missing-private-catalog' ($stages + "-PhordeRegionPagePrototypeRoot=$temporaryRoot") $true 'Region-page investigation needs a prepared private shader catalog.'

    # This fixture proves only configuration admission. No private artifact is
    # compiled, and CMake still validates the real catalogs/ABI before building.
    $privateFixture = Join-Path $temporaryRoot 'private-profile'
    $privateCatalogDirectory = Join-Path $privateFixture 'src/vulkan/raytracing'
    New-Item -ItemType Directory -Path $privateCatalogDirectory -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $repoRoot 'src/vulkan/raytracing/RtRayQueryVariantCatalog.generated.h') -Destination $privateCatalogDirectory
    $privateStages = $stages + "-PhordeRegionPagePrototypeRoot=$privateFixture"
    $privateResult = Invoke-GradlePolicyCase 'private-bank-stages' $privateStages
    Assert-True ($privateResult.Contains("-DHORDE_RT_REGION_PAGE_PROTOTYPE_ROOT=$($privateFixture.Replace('\', '/'))")) 'Existing explicit private stage profile must remain admitted.'
    Invoke-GradlePolicyCase 'private-clocks-missing-abi' ($privateStages + '-PhordeDispatchClockProbe=true') $true 'Dispatch clock observation needs the matching private shader ABI marker.'
    Write-Utf8NoBom (Join-Path $privateFixture 'dispatch-clock-probe.json') '{"schema":1}'
    $privateClockResult = Invoke-GradlePolicyCase 'private-clock-configuration' ($privateStages + '-PhordeDispatchClockProbe=true')
    Assert-True ($privateClockResult.Contains('-DHORDE_RT_FRAME_STAGE_PROBE=ON|-DHORDE_RT_DISPATCH_CLOCK_PROBE=ON')) 'Explicit private clock profile must retain both native observers.'

}
finally {
    if ($null -eq $savedInstrumentationEnvironment) { Remove-Item Env:ORG_GRADLE_PROJECT_hordeRtInstrumentationOverride -ErrorAction SilentlyContinue } else { $env:ORG_GRADLE_PROJECT_hordeRtInstrumentationOverride = $savedInstrumentationEnvironment }
    if ($null -eq $savedQualityEnvironment) { Remove-Item Env:ORG_GRADLE_PROJECT_hordeRtDielectricQualityOverride -ErrorAction SilentlyContinue } else { $env:ORG_GRADLE_PROJECT_hordeRtDielectricQualityOverride = $savedQualityEnvironment }
    if (Test-Path -LiteralPath $temporaryRoot) { Remove-Item -LiteralPath $temporaryRoot -Recurse -Force }
}
Write-Output 'Executable CMake and Gradle RT policy contracts passed.'
