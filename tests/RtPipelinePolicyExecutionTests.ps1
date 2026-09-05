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

    function Invoke-GradlePolicyCase([string]$name, [string[]]$properties, [bool]$expectFailure = $false) {
        # Explicit empty CLI properties defeat any developer-local gradle.properties
        # values while retaining the established dependency cache (no network lane).
        $neutralProperties = @()
        if (@($properties | Where-Object { $_ -like '-PhordeRtInstrumentationOverride=*' }).Count -eq 0) { $neutralProperties += '-PhordeRtInstrumentationOverride=' }
        if (@($properties | Where-Object { $_ -like '-PhordeRtDielectricQualityOverride=*' }).Count -eq 0) { $neutralProperties += '-PhordeRtDielectricQualityOverride=' }
        $arguments = @('--no-daemon', '--console=plain', '-q', ':app:printHordeRtPolicyForTest') + $neutralProperties + $properties
        $result = Invoke-Native $GradleExecutable $arguments (Join-Path $repoRoot 'android')
        if ($expectFailure) {
            Assert-True ($result[0] -ne 0) "Gradle policy case $name unexpectedly succeeded."
            return
        }
        Assert-True ($result[0] -eq 0) "Gradle policy case $name failed: $($result[1])"
        return $result[1]
    }

    $gradleDefault = Invoke-GradlePolicyCase 'defaults' @()
    Assert-True ($gradleDefault.Contains('debug=-DHORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION=Diagnostic|-DHORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY=Mobile')) 'Gradle Debug default policy is incorrect.'
    Assert-True ($gradleDefault.Contains('release=-DHORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION=Shipping|-DHORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY=Mobile')) 'Gradle Release default policy is incorrect.'
    $gradleOverride = Invoke-GradlePolicyCase 'override-precedence' @('-PhordeRtInstrumentationOverride=Diagnostic', '-PhordeRtDielectricQualityOverride=High')
    Assert-True ($gradleOverride.Contains('debug=-DHORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION=Diagnostic|-DHORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY=High')) 'Gradle override did not apply to Debug.'
    Assert-True ($gradleOverride.Contains('release=-DHORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION=Diagnostic|-DHORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY=High')) 'Gradle override did not take precedence over Release default.'
    Invoke-GradlePolicyCase 'partial-override' @('-PhordeRtInstrumentationOverride=Shipping') $true
    Invoke-GradlePolicyCase 'invalid-override-case' @('-PhordeRtInstrumentationOverride=shipping', '-PhordeRtDielectricQualityOverride=Mobile') $true
    Invoke-GradlePolicyCase 'invalid-quality' @('-PhordeRtInstrumentationOverride=Shipping', '-PhordeRtDielectricQualityOverride=mobile') $true

}
finally {
    if ($null -eq $savedInstrumentationEnvironment) { Remove-Item Env:ORG_GRADLE_PROJECT_hordeRtInstrumentationOverride -ErrorAction SilentlyContinue } else { $env:ORG_GRADLE_PROJECT_hordeRtInstrumentationOverride = $savedInstrumentationEnvironment }
    if ($null -eq $savedQualityEnvironment) { Remove-Item Env:ORG_GRADLE_PROJECT_hordeRtDielectricQualityOverride -ErrorAction SilentlyContinue } else { $env:ORG_GRADLE_PROJECT_hordeRtDielectricQualityOverride = $savedQualityEnvironment }
    if (Test-Path -LiteralPath $temporaryRoot) { Remove-Item -LiteralPath $temporaryRoot -Recurse -Force }
}
Write-Output 'Executable CMake and Gradle RT policy contracts passed.'
