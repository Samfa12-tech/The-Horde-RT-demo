[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
function Assert-True([bool]$condition, [string]$message) { if (-not $condition) { throw $message } }

& (Join-Path $repoRoot 'tools\GenerateRtPipelineVariantCatalog.ps1') -Check
$temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) ('horde-rt-provider-fence-' + [guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
    $catalogFixture = Join-Path $temporaryRoot 'catalog.json'
    $adapterFixture = Join-Path $temporaryRoot 'adapter.h'
    Copy-Item -LiteralPath (Join-Path $repoRoot 'tools\raygen-variant-catalog.json') -Destination $catalogFixture
    Copy-Item -LiteralPath (Join-Path $repoRoot 'src\vulkan\raytracing\RtPipelineVariantCatalog.generated.h') -Destination $adapterFixture
    $adapterText = Get-Content -LiteralPath $adapterFixture -Raw
    [IO.File]::WriteAllText($adapterFixture, $adapterText.Replace('shipping_mobile_opaque_fast', 'shipping_mobile_opaque_stale'), [Text.UTF8Encoding]::new($false))
    $rejectedHandEdit = $false
    try { & (Join-Path $repoRoot 'tools\GenerateRtPipelineVariantCatalog.ps1') -Check -CatalogPath $catalogFixture -OutputPath $adapterFixture } catch { $rejectedHandEdit = $true }
    Assert-True $rejectedHandEdit 'A hand-edited generated adapter must fail closed.'
    $catalogText = Get-Content -LiteralPath $catalogFixture -Raw
    [IO.File]::WriteAllText($catalogFixture, $catalogText.Replace('diagnostic_high_generic_dielectric', 'shipping_mobile_opaque_fast'), [Text.UTF8Encoding]::new($false))
    $rejectedCatalog = $false
    try { & (Join-Path $repoRoot 'tools\GenerateRtPipelineVariantCatalog.ps1') -Check -CatalogPath $catalogFixture -OutputPath (Join-Path $repoRoot 'src\vulkan\raytracing\RtPipelineVariantCatalog.generated.h') } catch { $rejectedCatalog = $true }
    Assert-True $rejectedCatalog 'A duplicate/missing frozen catalog key must fail closed.'
}
finally { if (Test-Path -LiteralPath $temporaryRoot) { Remove-Item -LiteralPath $temporaryRoot -Recurse -Force } }

$policy = Get-Content -LiteralPath (Join-Path $repoRoot 'cmake\HordeRtRaygenPolicy.cmake') -Raw
foreach ($required in @('HORDE_RT_INSTRUMENTATION_OVERRIDE', 'HORDE_RT_DIELECTRIC_QUALITY_OVERRIDE',
    'must be supplied together', 'exactly Shipping or Diagnostic', 'exactly Mobile or High',
    'HORDE_RT_POLICY_UNSUPPORTED_CONFIGURATION')) {
    Assert-True $policy.Contains($required) "Missing fail-closed policy contract: $required"
}
$gradle = Get-Content -LiteralPath (Join-Path $repoRoot 'android\app\build.gradle') -Raw
foreach ($required in @('hordeRtInstrumentationOverride', 'hordeRtDielectricQualityOverride',
    'HORDE_RT_ANDROID_DEFAULT_INSTRUMENTATION', 'HORDE_RT_ANDROID_DEFAULT_DIELECTRIC_QUALITY')) {
    Assert-True $gradle.Contains($required) "Android policy argument/precedence contract is missing: $required"
}

$isolatedSources = @('src\vulkan\raytracing\RtPipelineVariants.h', 'src\vulkan\raytracing\RtPipelineVariants.cpp',
    'src\vulkan\raytracing\RtPipelineVariantProvider.h', 'src\vulkan\raytracing\RtPipelineVariantProvider.cpp',
    'src\vulkan\raytracing\RtPipelineVariantCatalog.generated.h', 'tests\RtPipelineVariantsTests.cpp',
    'tests\RtPipelineVariantProviderTests.cpp', 'tests\RtPipelineVariantFixture.cpp')
foreach ($relativePath in $isolatedSources) {
    $contents = Get-Content -LiteralPath (Join-Path $repoRoot $relativePath) -Raw
    Assert-True (-not $contents.Contains('WaterQuality')) "Provider boundary must not couple to WaterQuality: $relativePath"
}
$forbidden = [ordered]@{
    'src/vulkan/raytracing/PresentableTinyRtScene.cpp' = '8d35e7c29ae210a9c7272ed1592778208653647e'
    'src/vulkan/raytracing/PresentableTinyRtScene.h' = 'e52e4ca74c00c629625e4da44bec843633240364'
    'src/platform/windows/DiagnosticWindow.cpp' = 'eacddf9cf069980893eb06f1d478b935835ed2e4'
    'android/app/src/main/cpp/android_probe_bridge.cpp' = '325973cc8ef62373fe6a592befd9a910270bc143'
}
function Assert-OwnershipFence([hashtable]$expected) {
    foreach ($entry in $expected.GetEnumerator()) {
        $actual = ((& git -C $repoRoot hash-object $entry.Key) -join '').Trim()
        Assert-True ($actual -eq $entry.Value) "Task 3e-a ownership fence changed: $($entry.Key)"
    }
}
Assert-OwnershipFence $forbidden
$ineffectiveControl = [ordered]@{} + $forbidden
$ineffectiveControl['src/vulkan/raytracing/PresentableTinyRtScene.cpp'] = '0' * 40
$rejectedControl = $false
try { Assert-OwnershipFence $ineffectiveControl } catch { $rejectedControl = $true }
Assert-True $rejectedControl 'Ownership fence control must fail when an expected source identity is wrong.'
Write-Output 'RT provider policy and ownership fence contracts passed.'
