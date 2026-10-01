param()

$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$generator = Join-Path $repoRoot 'tools/compile-staged-primary.ps1'
if (-not (Test-Path -LiteralPath $generator -PathType Leaf)) { throw "Generator missing: $generator" }
$testRoot = Join-Path ([IO.Path]::GetTempPath()) ("horde-staged-primary-tests-{0}" -f [guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $testRoot)
try
{
    . $generator -Instrumentation Diagnostic -Quality Mobile -OutputDirectory (Join-Path $testRoot 'unused-output')
    $script:StagedPrimaryRepoRoot = $repoRoot
    function Assert-StagedTest([bool]$Condition, [string]$Message)
    {
        if (-not $Condition) { throw "FAIL: $Message" }
        Write-Output "PASS: $Message"
    }
    function Assert-StagedThrows([scriptblock]$Action, [string]$Message)
    {
        $threw = $false
        try { $null = & $Action } catch { $threw = $true }
        Assert-StagedTest $threw $Message
    }

    $sourcePath = Join-Path $repoRoot 'shaders/raytracing/minimal.rgen'
    $active = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $deps = [Collections.Generic.List[string]]::new()
    $resolved = Resolve-StagedPrimaryIncludes -Path $sourcePath -ActivePaths $active -Dependencies $deps
    Assert-StagedTest -Condition ($deps.Count -gt 1 -and $resolved.Contains('HitInfo primary = traceScene(')) -Message 'normal include graph expands to primary raygen source'

    $recordPath = Join-Path $repoRoot 'shaders/raytracing/experimental/staged_primary_record.glsl'
    $record = [IO.File]::ReadAllText($recordPath)
    $opaqueSource = $resolved.Replace('#define HORDE_GENERIC_TRANSMISSION_VARIANT 1', '#define HORDE_GENERIC_TRANSMISSION_VARIANT 0')
    Assert-StagedTest -Condition ($opaqueSource -ne $resolved) -Message 'OpaqueFast compatibility selector is present and replaceable'
    $configured = Add-StagedPrimaryVariantConfiguration -Source $opaqueSource -InstrumentationName Diagnostic -QualityName Mobile -MaterialValue 0 -PassValue 1
    Assert-StagedTest -Condition ($configured.Contains('#define HORDE_RT_VARIANT_INSTRUMENTATION 1') -and $configured.Contains('#define HORDE_RT_VARIANT_QUALITY 0')) -Message 'injected instrumentation and quality values match normal compiler policy'

    $primary = New-StagedPrimaryPassSource -ResolvedSource $configured -RecordSource $record -Pass primary
    $shadeConfigured = Add-StagedPrimaryVariantConfiguration -Source $resolved -InstrumentationName Diagnostic -QualityName Mobile -MaterialValue 1 -PassValue 2
    $shade = New-StagedPrimaryPassSource -ResolvedSource $shadeConfigured -RecordSource $record -Pass shade
    $primaryCheckMain = Find-StagedPrimaryMainBody $primary
    $primaryCheckBody = $primary.Substring($primaryCheckMain.Start, $primaryCheckMain.End - $primaryCheckMain.Start + 1)
    $shadeCheckMain = Find-StagedPrimaryMainBody $shade
    $shadeCheckBody = $shade.Substring($shadeCheckMain.Start, $shadeCheckMain.End - $shadeCheckMain.Start + 1)
    Assert-StagedTest -Condition ($primaryCheckBody.Contains('traceScene(origin, rayDirection') -and $primaryCheckBody.Contains('storeStagedPrimary(primary);') -and -not $primaryCheckBody.Contains('integrateFireEmitters(')) -Message 'primary stage retains camera/trace and stops after record store'
    Assert-StagedTest -Condition ($shadeCheckBody.Contains('HitInfo primary = loadStagedPrimary();') -and $shadeCheckBody.Contains('shadePrimary(primary, rayDirection)') -and -not $shadeCheckBody.Contains('traceScene(origin, rayDirection')) -Message 'shade stage loads record while preserving original shade and presentation'

    $baseAnchor = '    vec3 color = shadePrimary(primary, rayDirection);'
    $duplicate = $configured.Replace($baseAnchor, $baseAnchor + "`n" + $baseAnchor)
    Assert-StagedThrows { New-StagedPrimaryPassSource -ResolvedSource $duplicate -RecordSource $record -Pass primary } 'repeated primary-stage anchor is rejected'
    $missing = $configured.Replace($baseAnchor, '    vec3 unavailable = vec3(0.0);')
    Assert-StagedThrows { New-StagedPrimaryPassSource -ResolvedSource $missing -RecordSource $record -Pass primary } 'missing primary-stage anchor is rejected'
    $shadeTraceAnchor = '    HitInfo primary = traceScene(origin, rayDirection, 10000.0, primaryMask,'
    $duplicateShadeEnd = $shadeConfigured.Replace($baseAnchor, $baseAnchor + "`n" + $baseAnchor)
    Assert-StagedThrows { New-StagedPrimaryPassSource -ResolvedSource $duplicateShadeEnd -RecordSource $record -Pass shade } 'repeated shade-stage end anchor is rejected'
    $missingShadeTrace = $shadeConfigured.Replace($shadeTraceAnchor, '    HitInfo primary = traceScene_REMOVED(')
    Assert-StagedThrows { New-StagedPrimaryPassSource -ResolvedSource $missingShadeTrace -RecordSource $record -Pass shade } 'missing shade-stage trace anchor is rejected'

    $main = Find-StagedPrimaryMainBody $configured
    $originalBody = $configured.Substring($main.Start, $main.End - $main.Start + 1)
    $originalSuffix = $originalBody.Substring($originalBody.IndexOf($baseAnchor, [StringComparison]::Ordinal) + $baseAnchor.Length)
    $primaryMain = Find-StagedPrimaryMainBody $primary
    $primaryBody = $primary.Substring($primaryMain.Start, $primaryMain.End - $primaryMain.Start + 1)
    Assert-StagedTest -Condition ($primaryBody.StartsWith($originalBody.Substring(0, $originalBody.IndexOf($baseAnchor, [StringComparison]::Ordinal)), [StringComparison]::Ordinal)) -Message 'primary transform preserves the exact camera and trace prefix'
    Assert-StagedTest -Condition ($primaryBody.EndsWith('storeStagedPrimary(primary);' + "`n`}", [StringComparison]::Ordinal) -and $originalSuffix.Contains('imageStore')) -Message 'primary transform only replaces shading tail and keeps one valid function close'

    $shadeMain = Find-StagedPrimaryMainBody $shade
    $shadeBody = $shade.Substring($shadeMain.Start, $shadeMain.End - $shadeMain.Start + 1)
    $load = '    HitInfo primary = loadStagedPrimary();' + "`n"
    $colorIndex = $shadeBody.IndexOf($baseAnchor, [StringComparison]::Ordinal)
    $originalShadeMain = Find-StagedPrimaryMainBody $shadeConfigured
    $originalShadeBody = $shadeConfigured.Substring($originalShadeMain.Start, $originalShadeMain.End - $originalShadeMain.Start + 1)
    $originalColorIndex = $originalShadeBody.IndexOf($baseAnchor, [StringComparison]::Ordinal)
    $sameShadeSuffix = $originalColorIndex -gt 0 -and [string]::Equals($shadeBody.Substring($colorIndex), $originalShadeBody.Substring($originalColorIndex), [StringComparison]::Ordinal)
    Assert-StagedTest -Condition ($colorIndex -gt 0 -and $shadeBody.Substring(0, $colorIndex).Contains($load) -and $sameShadeSuffix) -Message 'shade transform preserves the exact original shading/output suffix'

    $script:StagedPrimaryOpaquePairSha = 'a' * 64
    $script:StagedPrimaryGenericPairSha = 'b' * 64
    $fakeModules = @(
        [pscustomobject]@{ ArrayName = 'kPrimaryOpaqueWords'; Words = @(1u); Sha256 = '1' * 64 },
        [pscustomobject]@{ ArrayName = 'kShadeOpaqueWords'; Words = @(2u); Sha256 = '2' * 64 },
        [pscustomobject]@{ ArrayName = 'kPrimaryGenericWords'; Words = @(3u); Sha256 = '3' * 64 },
        [pscustomobject]@{ ArrayName = 'kShadeGenericWords'; Words = @(4u); Sha256 = '4' * 64 }
    )
    $header = New-StagedPrimaryHeader -Modules $fakeModules -InstrumentationName Diagnostic -QualityName Mobile
    Assert-StagedTest -Condition ($header.Contains('kPrimaryOpaqueWords') -and $header.Contains('kShadeOpaqueWords') -and $header.Contains('kPrimaryGenericWords') -and $header.Contains('kShadeGenericWords') -and $header.Contains('std::span<const std::uint32_t>') -and $header.Contains('kStagedPrimaryPage2Binding = 2')) -Message 'generated C++ header provides the four word arrays, spans, policy, and binding constants'
    Assert-StagedThrows { Invoke-StagedPrimaryCompilation -InstrumentationName Diagnostic -QualityName Mobile -Destination $repoRoot -SdkRoot 'not-used' } 'generator refuses an output directory equal to the repository root'

    $fixture = Join-Path $testRoot 'include-fixture'
    [void](New-Item -ItemType Directory -Path $fixture)
    $a = Join-Path $fixture 'a.glsl'; $b = Join-Path $fixture 'b.glsl'
    [IO.File]::WriteAllText($a, '#include "b.glsl"' + "`n")
    [IO.File]::WriteAllText($b, '#include "a.glsl"' + "`n")
    $script:StagedPrimaryRepoRoot = $testRoot
    Assert-StagedThrows {
        $stack = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        $list = [Collections.Generic.List[string]]::new()
        Resolve-StagedPrimaryIncludes -Path $a -ActivePaths $stack -Dependencies $list
    } 'include cycle is rejected'
    $script:StagedPrimaryRepoRoot = $repoRoot
    Assert-StagedThrows {
        $stack = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        $list = [Collections.Generic.List[string]]::new()
        Resolve-StagedPrimaryIncludes -Path ([IO.Path]::GetTempPath()) -ActivePaths $stack -Dependencies $list
    } 'out-of-repository dependency is rejected'

    Write-Output 'Staged-primary transformation tests passed.'
}
finally
{
    if (Test-Path -LiteralPath $testRoot)
    {
        $fullTestRoot = [IO.Path]::GetFullPath($testRoot)
        $tempBase = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/')
        $rootParent = [IO.Path]::GetDirectoryName($fullTestRoot).TrimEnd('\', '/')
        $name = [IO.Path]::GetFileName($fullTestRoot)
        if (-not $rootParent.Equals($tempBase, [StringComparison]::OrdinalIgnoreCase) -or $name -notmatch '^horde-staged-primary-tests-[0-9a-f]{32}$')
        { throw "Refusing to remove unexpected test fixture path: $fullTestRoot" }
        if (([IO.File]::GetAttributes($fullTestRoot) -band [IO.FileAttributes]::ReparsePoint) -ne 0)
        { throw "Refusing to remove reparse-point test fixture: $fullTestRoot" }
        Remove-Item -LiteralPath $fullTestRoot -Recurse -Force
    }
}
