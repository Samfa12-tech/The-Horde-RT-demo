[CmdletBinding()]
param([string]$CmakeExecutable)

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
    Copy-Item -LiteralPath (Join-Path $repoRoot 'src\vulkan\raytracing\RtPipelineVariantCatalog.generated.h') -Destination $adapterFixture -Force
    $adapterBytes = [IO.File]::ReadAllBytes($adapterFixture)
    [IO.File]::WriteAllBytes($adapterFixture, [byte[]](0xef,0xbb,0xbf) + $adapterBytes)
    $rejectedAdapterBom = $false
    try { & (Join-Path $repoRoot 'tools\GenerateRtPipelineVariantCatalog.ps1') -Check -CatalogPath $catalogFixture -OutputPath $adapterFixture } catch { $rejectedAdapterBom = $true }
    Assert-True $rejectedAdapterBom 'A BOM-prefixed generated adapter must fail closed.'
    Copy-Item -LiteralPath (Join-Path $repoRoot 'tools\raygen-variant-catalog.json') -Destination $catalogFixture -Force
    $catalogBytes = [IO.File]::ReadAllBytes($catalogFixture)
    [IO.File]::WriteAllBytes($catalogFixture, [byte[]](0xef,0xbb,0xbf) + $catalogBytes)
    $rejectedCatalogBom = $false
    try { & (Join-Path $repoRoot 'tools\GenerateRtPipelineVariantCatalog.ps1') -Check -CatalogPath $catalogFixture -OutputPath (Join-Path $repoRoot 'src\vulkan\raytracing\RtPipelineVariantCatalog.generated.h') } catch { $rejectedCatalogBom = $true }
    Assert-True $rejectedCatalogBom 'A BOM-prefixed frozen catalog must fail closed.'
    Copy-Item -LiteralPath (Join-Path $repoRoot 'tools\raygen-variant-catalog.json') -Destination $catalogFixture -Force
    $catalog = Get-Content -LiteralPath $catalogFixture -Raw | ConvertFrom-Json
    $firstVariant = $catalog.variants[0]
    $catalog.variants[0] = $catalog.variants[1]
    $catalog.variants[1] = $firstVariant
    [IO.File]::WriteAllText($catalogFixture, ($catalog | ConvertTo-Json -Depth 100), [Text.UTF8Encoding]::new($false))
    $rejectedReorderedCatalog = $false
    try { & (Join-Path $repoRoot 'tools\GenerateRtPipelineVariantCatalog.ps1') -Check -CatalogPath $catalogFixture -OutputPath (Join-Path $repoRoot 'src\vulkan\raytracing\RtPipelineVariantCatalog.generated.h') } catch { $rejectedReorderedCatalog = $true }
    Assert-True $rejectedReorderedCatalog 'A reordered frozen catalog must fail closed.'
    Copy-Item -LiteralPath (Join-Path $repoRoot 'tools\raygen-variant-catalog.json') -Destination $catalogFixture -Force
    $catalog = Get-Content -LiteralPath $catalogFixture -Raw | ConvertFrom-Json
    $catalog | Add-Member -NotePropertyName unexpectedAuthority -NotePropertyValue 'reject'
    [IO.File]::WriteAllText($catalogFixture, ($catalog | ConvertTo-Json -Depth 100), [Text.UTF8Encoding]::new($false))
    $rejectedExtraCatalogField = $false
    try { & (Join-Path $repoRoot 'tools\GenerateRtPipelineVariantCatalog.ps1') -Check -CatalogPath $catalogFixture -OutputPath (Join-Path $repoRoot 'src\vulkan\raytracing\RtPipelineVariantCatalog.generated.h') } catch { $rejectedExtraCatalogField = $true }
    Assert-True $rejectedExtraCatalogField 'An extra frozen catalog field must fail closed.'
    Copy-Item -LiteralPath (Join-Path $repoRoot 'tools\raygen-variant-catalog.json') -Destination $catalogFixture -Force
    $catalogText = Get-Content -LiteralPath $catalogFixture -Raw
    [IO.File]::WriteAllText($catalogFixture, $catalogText.Replace('diagnostic_high_generic_dielectric', 'shipping_mobile_opaque_fast'), [Text.UTF8Encoding]::new($false))
    $rejectedCatalog = $false
    try { & (Join-Path $repoRoot 'tools\GenerateRtPipelineVariantCatalog.ps1') -Check -CatalogPath $catalogFixture -OutputPath (Join-Path $repoRoot 'src\vulkan\raytracing\RtPipelineVariantCatalog.generated.h') } catch { $rejectedCatalog = $true }
    Assert-True $rejectedCatalog 'A duplicate/missing frozen catalog key must fail closed.'
    Copy-Item -LiteralPath (Join-Path $repoRoot 'tools\raygen-variant-catalog.json') -Destination $catalogFixture -Force
    $catalogText = Get-Content -LiteralPath $catalogFixture -Raw
    [IO.File]::WriteAllText($catalogFixture, $catalogText.Replace('7ebdb794794a854b6cb5c44c75dd9a9decd42f4999c44a9cfa78f13b12a13f21', ('0' * 64)), [Text.UTF8Encoding]::new($false))
    $rejectedInclude = $false
    try { & (Join-Path $repoRoot 'tools\GenerateRtPipelineVariantCatalog.ps1') -Check -CatalogPath $catalogFixture -OutputPath (Join-Path $repoRoot 'src\vulkan\raytracing\RtPipelineVariantCatalog.generated.h') } catch { $rejectedInclude = $true }
    Assert-True $rejectedInclude 'A stale frozen include hash must fail before provider compilation.'
}
finally { if (Test-Path -LiteralPath $temporaryRoot) { Remove-Item -LiteralPath $temporaryRoot -Recurse -Force } }

if (-not [string]::IsNullOrWhiteSpace($CmakeExecutable)) {
    $freshConfigureRoot = Join-Path ([IO.Path]::GetTempPath()) ('horde-rt-fresh-configure-' + [guid]::NewGuid().ToString('N'))
    try {
        & $CmakeExecutable -S $repoRoot -B $freshConfigureRoot -DHORDE_RT_BUILD_VULKAN_TARGETS=OFF
        Assert-True ($LASTEXITCODE -eq 0) 'Fresh policy fixture configure failed.'
        $ctestFile = Get-Content -LiteralPath (Join-Path $freshConfigureRoot 'CTestTestfile.cmake') -Raw
        $runner = (Get-Command pwsh,powershell -ErrorAction Stop | Select-Object -First 1 -ExpandProperty Source).Replace('\','/')
        foreach ($name in @('shipping_mobile','shipping_high','diagnostic_mobile','diagnostic_high')) {
            Assert-True ($ctestFile -match ('horde_rt_provider_' + $name + '_fixture_evidence.*"' + [regex]::Escape($runner))) "Fresh fixture command does not begin with resolved PowerShell: $name"
        }
    }
    finally { if (Test-Path -LiteralPath $freshConfigureRoot) { Remove-Item -LiteralPath $freshConfigureRoot -Recurse -Force } }
}

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
