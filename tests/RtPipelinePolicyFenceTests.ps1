[CmdletBinding()]
param([string]$CmakeExecutable)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
function Assert-True([bool]$condition, [string]$message) { if (-not $condition) { throw $message } }
function Get-CtestFirstCommandToken([string]$ctestContents, [string]$testName) {
    $record = [regex]::Match($ctestContents, ('(?s)add_test\(\[=\[' + [regex]::Escape($testName) + '\]=\]\s+"([^"]+)"'))
    Assert-True $record.Success "Generated CTest record is missing: $testName"
    return $record.Groups[1].Value
}
function Assert-CtestFirstCommandToken([string]$ctestContents, [string]$testName, [string]$expectedRunner) {
    $actualRunner = Get-CtestFirstCommandToken $ctestContents $testName
    Assert-True ($actualRunner -ceq $expectedRunner) "Generated CTest record does not begin with resolved PowerShell: $testName ($actualRunner)"
}
function Assert-CtestTimeout([string]$ctestContents, [string]$testName, [int]$expectedSeconds) {
    $record = [regex]::Match($ctestContents, ('(?s)set_tests_properties\(\[=\[' + [regex]::Escape($testName) + '\]=\]\s+PROPERTIES\s+(.*?)\)'))
    Assert-True $record.Success "Generated CTest timeout metadata is missing: $testName"
    $timeout = [regex]::Match($record.Groups[1].Value, 'TIMEOUT\s+"?([0-9]+)"?')
    Assert-True ($timeout.Success -and [int]$timeout.Groups[1].Value -eq $expectedSeconds) "Generated CTest timeout is wrong: $testName"
}

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
            Assert-CtestFirstCommandToken $ctestFile ("horde_rt_provider_${name}_fixture_evidence") $runner
        }
        $syntheticWrongFirst = 'add_test([=[horde_rt_provider_synthetic_fixture_evidence]=] "C:/wrong.exe" "C:/Program Files/PowerShell/7/pwsh.exe")'
        $rejectedWrongFirst = $false
        try { Assert-CtestFirstCommandToken $syntheticWrongFirst 'horde_rt_provider_synthetic_fixture_evidence' 'C:/Program Files/PowerShell/7/pwsh.exe' } catch { $rejectedWrongFirst = $true }
        Assert-True $rejectedWrongFirst 'Fresh-command parser must reject a correct runner that appears after a wrong first executable.'
        foreach ($name in @('horde_rt_pipeline_policy_fence_tests', 'horde_rt_pipeline_policy_execution_tests')) {
            Assert-CtestTimeout $ctestFile $name 300
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
    'src\vulkan\raytracing\RtPipelineBundleContracts.h', 'src\vulkan\raytracing\RtPipelineBundleContracts.cpp',
    'src\vulkan\raytracing\RtPipelineBundle.h', 'src\vulkan\raytracing\RtPipelineBundle.cpp',
    'src\vulkan\raytracing\RtPipelineVariantCatalog.generated.h', 'tests\RtPipelineVariantsTests.cpp',
    'tests\RtPipelineVariantProviderTests.cpp', 'tests\RtPipelineVariantFixture.cpp')
foreach ($relativePath in $isolatedSources) {
    $contents = Get-Content -LiteralPath (Join-Path $repoRoot $relativePath) -Raw
    Assert-True (-not $contents.Contains('WaterQuality')) "Provider boundary must not couple to WaterQuality: $relativePath"
}
$scene = Get-Content -LiteralPath (Join-Path $repoRoot 'src\vulkan\raytracing\PresentableTinyRtScene.cpp') -Raw
$sceneHeader = Get-Content -LiteralPath (Join-Path $repoRoot 'src\vulkan\raytracing\PresentableTinyRtScene.h') -Raw
$windowsHost = Get-Content -LiteralPath (Join-Path $repoRoot 'src\platform\windows\DiagnosticWindow.cpp') -Raw
$androidHost = Get-Content -LiteralPath (Join-Path $repoRoot 'android\app\src\main\cpp\android_probe_bridge.cpp') -Raw
$foundationRunner = Get-Content -LiteralPath (Join-Path $repoRoot 'tools\run-foundation-validation.ps1') -Raw
$androidRunner = Get-Content -LiteralPath (Join-Path $repoRoot 'tools\run-android-showcase-validation.ps1') -Raw
$androidComparison = Get-Content -LiteralPath (Join-Path $repoRoot 'tools\compare-android-gpu-timing-ab.ps1') -Raw
foreach ($retired in @('MinimalRayGenShader.inc', 'MinimalLegacyRayGenShader.inc',
                        'kMinimalRayGenShader', 'kMinimalLegacyRayGenShader',
                        'legacyPipeline_', 'legacyShaderBindingTable_')) {
    Assert-True (-not $scene.Contains($retired)) "Runtime source still retains compatibility ownership: $retired"
}
Assert-True $sceneHeader.Contains('RtPipelineBundle pipelineBundle_') 'Scene must own exactly one selected RT pipeline bundle.'
Assert-True $scene.Contains('BuildRtPipelineBundleResources(pipelineBundle_') 'Live scene must use the production-tested bundle builder.'

$initialiseStart = $scene.IndexOf('bool PresentableTinyRtScene::InitialiseWithOrchestration(')
$initialiseEnd = $scene.IndexOf('bool PresentableTinyRtScene::ContinueInitialiseAfterPreflight(', $initialiseStart)
Assert-True ($initialiseStart -ge 0 -and $initialiseEnd -gt $initialiseStart) 'Unable to isolate scene Initialise ownership path.'
$initialise = $scene.Substring($initialiseStart, $initialiseEnd - $initialiseStart)
$preflightAt = $initialise.IndexOf('api.resolvePreflight(')
Assert-True ($preflightAt -gt $initialise.IndexOf('RT dispatch extent is zero.')) 'Selected provider preflight must follow non-owning argument checks.'
$ownershipContinuationAt = $initialise.IndexOf('api.continueAfterPreflight(')
Assert-True ($ownershipContinuationAt -gt $preflightAt) 'Scene ownership continuation must follow selected-provider preflight.'
foreach ($later in @('vkGetPhysicalDeviceFormatProperties', 'characterSlot_.LoadAssets',
                      'LoadStaticHeldItemAssets', 'CreateStorageImage')) {
    Assert-True (-not $initialise.Contains($later)) "Scene orchestration contains ownership/work before its guarded continuation: $later"
}

$recordStart = $scene.IndexOf('bool PresentableTinyRtScene::RecordTraceAndCopy(')
$recordEnd = $scene.IndexOf('bool PresentableTinyRtScene::CaptureStorageImage(', $recordStart)
Assert-True ($recordStart -ge 0 -and $recordEnd -gt $recordStart) 'Unable to isolate record-time selection path.'
$record = $scene.Substring($recordStart, $recordEnd - $recordStart)
foreach ($required in @('pipelineBundle_.Strategy(', 'genericTransmissionActive_',
                         'activeStrategy.pipeline', 'activeStrategy.sbtRegions[0]',
                         'pipelineBundle_.pipelineLayout', 'pipelineBundle_.descriptorSet')) {
    Assert-True $record.Contains($required) "Record path is missing selected-record ownership: $required"
}
foreach ($forbiddenRecordDecision in @('ResolveCompiledRtPipelineBundlePreflight',
        'RtPipelineVariantProvider', 'RtPipelineBundleRequest', 'CreateBuffer(',
        'BuildRtPipelineBundleResources', 'Destroy(', 'WaterQuality', 'waterQuality =')) {
    Assert-True (-not $record.Contains($forbiddenRecordDecision)) "Record path contains a forbidden policy/lifetime decision: $forbiddenRecordDecision"
}
foreach ($hostSource in @($windowsHost, $androidHost)) {
    Assert-True (-not $hostSource.Contains('RtPipelineBundleRequest')) 'A platform host must not construct or pass a bundle request.'
    Assert-True (-not $hostSource.Contains('RtPipelineVariantProvider')) 'A platform host must not resolve provider policy.'
    Assert-True $hostSource.Contains('SelectedOpaqueFastKey()') 'A platform host must serialize the selected OpaqueFast key.'
    Assert-True $hostSource.Contains('SelectedGenericDielectricKey()') 'A platform host must serialize the selected GenericDielectric key.'
    Assert-True $hostSource.Contains('DiagnosticsAvailability()') 'A platform host must publish diagnostic availability beside legacy scalars.'
    Assert-True $hostSource.Contains('SelectedPipelineBundleIdentity()') 'A platform host must persist the full selected pair identity.'
    Assert-True $hostSource.Contains('SelectedPipelineBundleDisplayIdentity()') 'A platform host must display the short selected pair identity.'
    Assert-True (-not $hostSource.Contains('SelectedGenericDielectricSha256()).substr')) 'A platform host must not collapse pair identity to the generic-dielectric hash.'
}
foreach ($toolSource in @($foundationRunner, $androidRunner, $androidComparison)) {
    Assert-True $toolSource.Contains('selectedRtPipelineBundle') 'Live validation tooling must preserve selected RT pipeline-pair provenance.'
    Assert-True (-not $toolSource.Contains('raygenSha256')) 'Live validation tooling must not accept the compatibility raygen hash as runtime provenance.'
}

$rejectedControl = $false
try { Assert-True $record.Replace('pipelineBundle_.Strategy(', 'removed(').Contains('pipelineBundle_.Strategy(') 'Synthetic record without atomic strategy selection must fail.' } catch { $rejectedControl = $true }
Assert-True $rejectedControl 'Behavioral ownership-fence control did not prove it can reject a missing selection.'
Write-Output 'RT provider policy and ownership behavior contracts passed.'
