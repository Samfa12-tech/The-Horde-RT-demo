param(
    [switch]$ValidateOnly,
    [string]$EvidenceRoot = $PSScriptRoot
)
$ErrorActionPreference = 'Stop'
$scriptRoot = [IO.Path]::GetFullPath($PSScriptRoot)
if ($scriptRoot -cne 'C:\Dev\tmp\horde-opaque-retained-profile-20260930') { throw 'Use the bounded opaque-retained compiler-treatment comparator only.' }
$taskRoot = [IO.Path]::GetFullPath($EvidenceRoot)
if ($taskRoot -cne $scriptRoot) {
    $fixtureParent = [IO.Path]::GetFullPath((Join-Path $scriptRoot 'comparison-fixtures'))
    $fixturePrefix = $fixtureParent.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
    if (-not $ValidateOnly -or -not $taskRoot.StartsWith($fixturePrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Alternate evidence roots are allowed only for ValidateOnly fixtures under comparison-fixtures.'
    }
}
$compilerReceiptPath = Join-Path $scriptRoot 'opaque-retained-profile.json'
if ((Get-FileHash -LiteralPath $compilerReceiptPath -Algorithm SHA256).Hash.ToLowerInvariant() -cne '23f17fc83520a515bf2bdf4642527910f292e4115ea66ef8eba80167aac91760') { throw 'Immutable compiler-treatment receipt changed.' }
$compilerReceipt = Get-Content -LiteralPath $compilerReceiptPath -Raw | ConvertFrom-Json
$moduleReceiptPath = Join-Path $scriptRoot 'independent-scan-v3/independent-apk-module-receipt.json'
if ((Get-FileHash -LiteralPath $moduleReceiptPath -Algorithm SHA256).Hash.ToLowerInvariant() -cne '6b2d154668ee1d56a09a9ac8d6c557eaf3a0fbc4353a3c7a7965f5f96d013f7b') { throw 'Immutable candidate module-scan receipt changed.' }
$moduleReceipt = Get-Content -LiteralPath $moduleReceiptPath -Raw | ConvertFrom-Json
$controlContainmentPath = 'C:\Dev\tmp\horde-generic-route-profile-20260930\profile\provenance\containment-control-benchmark-shipping-mobile-eaf-scanner.json'
if ((Get-FileHash -LiteralPath $controlContainmentPath -Algorithm SHA256).Hash.ToLowerInvariant() -cne '5bde1466796ef6020962cd9486235dd86a35d1e7802ce792b0f661f0f3f05f87') { throw 'Immutable control containment receipt changed.' }
$controlContainment = Get-Content -LiteralPath $controlContainmentPath -Raw | ConvertFrom-Json
foreach($pair in @(@('shipping_mobile_opaque_fast','66e39df9f53b058fb62cbfa913d424b161c93be4aff59a1685cf8b7e54bb9c4b'),@('shipping_mobile_generic_dielectric','ce2302811cb2cb8bbff706fd54cd7f48705e4a5e8b2cda744a7d999574f84532'))){
    $matches=@($controlContainment.packaged.modules | Where-Object { $_.backend -ceq 'RayTracingPipeline' -and $_.sha256 -ceq $pair[1] })
    if($matches.Count -ne 1){throw "Pinned control pipeline module mismatch: $($pair[0])"}
}
$candidateArtifact=@($moduleReceipt.androidArtifacts | Where-Object kind -CEQ 'benchmark')
if($candidateArtifact.Count -ne 1 -or $candidateArtifact[0].sha256 -cne '4827a3c2e26d3328ed53608e15c77afe12a34077e96dcd0564c8d65532e21ff4'){throw 'Candidate immutable Shipping APK receipt mismatch.'}
foreach($pair in @(@('shipping_mobile_opaque_fast','14509c272fa4fa9f92a8d180abf170b2485447798b7940ec98f55150c05249d3'),@('shipping_mobile_generic_dielectric','ce2302811cb2cb8bbff706fd54cd7f48705e4a5e8b2cda744a7d999574f84532'))){
    $matches=@($candidateArtifact[0].modules | Where-Object { $_.backend -ceq 'RayTracingPipeline' -and $_.catalogKey -ceq $pair[0] -and $_.sha256 -ceq $pair[1] })
    if($matches.Count -ne 1){throw "Pinned candidate pipeline module mismatch: $($pair[0])"}
}
$analyzerPath = Join-Path $scriptRoot 'analyse-trial.ps1'
if ((Get-FileHash -LiteralPath $analyzerPath -Algorithm SHA256).Hash.ToLowerInvariant() -cne '7a9ca8dc897d5168843bc9e2c2435ce1568c13e7bebd70427972ad5aa33f63f0') { throw 'Strict trial analyzer changed after test review.' }
function Get-RequiredJsonPropertyValue($Object, [string]$Name) {
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) { throw "Missing JSON property '$Name'." }
    return $property.Value
}
function Get-JsonPropertyCount($Object) {
    $properties = @($Object.PSObject.Properties)
    return $properties.Count
}
$ids = @('opaque-compiler-control-a1-20261001','opaque-compiler-profile-b1-20261001','opaque-compiler-profile-b2-20261001','opaque-compiler-control-a2-20261001')
$missing = @($ids | Where-Object { -not (Test-Path -LiteralPath (Join-Path $taskRoot "phone/$_/analysis.json")) })
if ($missing.Count -gt 0) { throw "ABBA admission requires all four exact runs; missing analysis: $($missing -join ', ')" }
$rows = foreach ($id in $ids) {
    $path = Join-Path $taskRoot "phone/$id/analysis.json"
    $value = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
    $candidate = $id -match '^opaque-compiler-profile-'
    $expectedBuild = if ($candidate) { 'opaque-retained-profile' } else { 'control' }
    $expectedStrategy = 'opaque-fast'
    $expectedApk = if ($candidate) { '4827a3c2e26d3328ed53608e15c77afe12a34077e96dcd0564c8d65532e21ff4' } else { 'ab3e2261fd081f87e667a6e4967e2476077fa96554702330e6aa49baa8133eae' }
    $routeCounts = $value.owningActiveStrategy.counts
    $openingCounts = $value.owningActiveStrategy.openingCounts
    $routeStrategyCount = Get-RequiredJsonPropertyValue $routeCounts $expectedStrategy
    $openingStrategyCount = Get-RequiredJsonPropertyValue $openingCounts $expectedStrategy
    if ($value.runId -cne $id -or $value.integrity -cne 'PASS' -or $value.apkSha256 -cne $expectedApk -or
        $value.buildLabel -cne $expectedBuild -or $value.sourceCommit -cne $compilerReceipt.source.head -or
        $value.deviceModel -cne 'SM-S948B' -or $value.workload -cne 'showcase-route-v1' -or
        $value.frames -ne 1838 -or $value.diagnosticRowsCompiledOut -ne 1838 -or $value.presentMode -cne 'MAILBOX' -or
        $value.internalExtent.width -ne 1080 -or $value.internalExtent.height -ne 2235 -or
        $value.compilerTreatment.classification -cne 'compiler-treatment' -or
        $value.compilerTreatment.samePreprocessedSourceSha256 -cne 'eaa0b8a9f7b1a87fce83e34ab8568c524b1a8ed4160009bf35fc1a89e728713f' -or
        $value.compilerTreatment.runtimeStrategy -cne 'opaque-fast' -or
        $value.artifactReceipts.compilerTreatment -cne 'opaque-retained-profile.json' -or
        $value.artifactReceipts.candidateActualModuleScan -cne 'independent-scan-v3/independent-apk-module-receipt.json' -or
        $value.artifactReceipts.controlStandardContainment -cne $controlContainmentPath -or
        $value.owningActiveStrategy.exported -ne $true -or
        $routeStrategyCount -ne 1838 -or (Get-JsonPropertyCount $routeCounts) -ne 1 -or
        $openingStrategyCount -ne 160 -or (Get-JsonPropertyCount $openingCounts) -ne 1) { throw "Comparison admission failed: $id" }
    if ($candidate -and ($value.physicalEvidenceClass -cne 'investigation-only-compiler-treatment' -or
        $value.gainClaim -cne 'none' -or $value.physicalAcceptance -cne 'not-evaluated' -or
        $value.optimizationAcceptance -cne 'none' -or
        $value.compilerTreatment.optimizationAdmission -cne 'none' -or
        $value.compilerTreatment.gainClaim -cne 'none' -or
        $value.compilerTreatment.physicalAcceptance -cne 'not-evaluated' -or
        $value.compilerTreatment.separateOpeningPixelGate.pixelSubsetPassed -ne $false -or
        $value.compilerTreatment.separateOpeningPixelGate.maximumChannelDifference -ne 14 -or
        $value.compilerTreatment.separateOpeningPixelGate.pixelsDifferentByMoreThanOne -ne 125)) { throw "Compiler-treatment evidence classification failed: $id" }
    if (-not $candidate -and $value.physicalEvidenceClass -cne 'exact-control-reference') { throw "Control evidence classification failed: $id" }
    foreach ($moduleKey in @('shipping_mobile_opaque_fast','shipping_mobile_generic_dielectric')) {
        $expectedModuleSha = if ($moduleKey -ceq 'shipping_mobile_opaque_fast') { if ($candidate) { '14509c272fa4fa9f92a8d180abf170b2485447798b7940ec98f55150c05249d3' } else { '66e39df9f53b058fb62cbfa913d424b161c93be4aff59a1685cf8b7e54bb9c4b' } } else { 'ce2302811cb2cb8bbff706fd54cd7f48705e4a5e8b2cda744a7d999574f84532' }
        $actualModuleSha = $value.actualPipelineModuleSha256.PSObject.Properties[$moduleKey].Value
        if ($actualModuleSha -cne $expectedModuleSha) { throw "Module/APK receipt join failed: $id $moduleKey" }
    }
    $opening = @($value.zoneGpuCommandBufferTiming | Where-Object zone -CEQ 'opening')
    if ($opening.Count -ne 1 -or $opening[0].frames -ne 160) { throw "Opening ownership count failed: $id" }
    [ordered]@{
        runId = $id; apkSha256 = $value.apkSha256
        analysisSha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
        routeMedianMs = $value.overall.medianMs; routeP95Ms = $value.overall.p95Ms
        medianDerivedFps = $value.medianDerivedFps; routeGpuMedianMs = $value.gpuRtDurationMs.medianMilliseconds
        gpuRtDurationMs = $value.gpuRtDurationMs; cpuStages = $value.cpuStages
        openingGpuMedianMs = $opening[0].medianMs; openingGpuP95Ms = $opening[0].p95NearestRankMs
        openingGpuWithin33_333Ms = $opening[0].within33_333Ms; context = $value.context
        activeStrategy = $value.owningActiveStrategy; actualPipelineModuleSha256 = $value.actualPipelineModuleSha256
        compilerTreatment = $value.compilerTreatment; physicalEvidenceClass=$value.physicalEvidenceClass
    }
}
$control = @($rows[0],$rows[3]); $candidate = @($rows[1],$rows[2])
$metrics = foreach ($field in @('routeMedianMs','routeP95Ms','routeGpuMedianMs','openingGpuMedianMs','openingGpuP95Ms')) {
    $a = ($control[0][$field] + $control[1][$field]) / 2.0
    $b = ($candidate[0][$field] + $candidate[1][$field]) / 2.0
    [ordered]@{ metric=$field; controlMeanOfRunStatistics=$a; candidateMeanOfRunStatistics=$b; descriptiveChangePercent=100.0*($b/$a-1.0) }
}
$result = [ordered]@{
    schema=1; evidenceClass='investigation-only-compiler-treatment'; gainClaim='none'
    optimizationAcceptance='none'; physicalAcceptance='not-evaluated'; causalConclusion='none'
    sourceCommit=$compilerReceipt.source.head; samePreprocessedSourceSha256='eaa0b8a9f7b1a87fce83e34ab8568c524b1a8ed4160009bf35fc1a89e728713f'
    phaseAcceptance='not-complete'; target30Fps='not-demonstrated'; externallyCooled=$false
    fullRouteRows=7352; openingRows=640; runs=$rows; contrasts=$metrics
    pixelGate=[ordered]@{scope='separate Diagnostic/Mobile opening capture comparison';passed=$false;maximumChannelDifference=14;pixelsDifferentByMoreThanOne=125;physicalAcceptance='not-established'}
    limitations=@(
        'This compiler-treatment group compiles the exact same OpaqueFast preprocessed source while changing glslang flags and SPIR-V pass treatment together; individual compiler-pass causality is not isolated.',
        'The separate Diagnostic/Mobile opening pixel gate failed: maximum channel difference 14 and 125 pixels differ by more than one. This is not physical/visual acceptance for the Shipping route experiment.',
        'Battery temperatures and GPU thermal power levels vary; context is not aligned to individual measured frames. This is not a causal effect estimate or steady-state thermal certification.',
        'Mean of per-run statistics is descriptive, not a pooled-frame median, causal estimate, or display-pacing measurement.',
        'GPU command duration includes AS work, RT dispatch and copy; CPU skinning remains separate.',
        'Standard package containment remains RED on compute/raygen strategy parity; independent module extraction does not replace that guard.',
        'No optimization admission, gain, physical acceptance, 30 FPS, sustained, or display-pacing claim is made.'
    )
}
if ($ValidateOnly) { $result | ConvertTo-Json -Depth 8; return }
$output = Join-Path $taskRoot 'comparison.json'
if (Test-Path -LiteralPath $output) { throw 'Refuse to overwrite the frozen comparison receipt.' }
$result | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $output -Encoding utf8NoBOM
Write-Output 'Four-run compiler-treatment comparison admitted; investigation only, no optimisation/gain or 30 FPS acceptance.'
