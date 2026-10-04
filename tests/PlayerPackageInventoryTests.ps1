[CmdletBinding()]
param([string]$AndroidApk = '')
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$required = @(
    'assets/models/player/viewmodel/runtime/asset.manifest.json',
    'assets/models/player/viewmodel/runtime/gothic-traveller-viewmodel.runtime.glb')
function Test-PortableViewmodelInventory([string]$WorkflowText) {
    # Stop at the next sibling job, not at a particular historical job name.
    # Additional CI lanes must not enter the shared-gameplay fetch inventory.
    $lane = [regex]::Match($WorkflowText,
        '(?ms)^  shared-gameplay:[ \t]*\r?\n(.*?)(?=^  [A-Za-z0-9_-]+:[ \t]*\r?$|\z)')
    return $lane.Success -and [regex]::Matches($lane.Groups[1].Value,
        [regex]::Escape('assets/models/player/viewmodel/runtime/*.glb')).Count -eq 2
}
function Test-PropsSourcePolicyInventory([string]$WorkflowText, [string]$JobName) {
    # Admission checks both canonical native profiles, even for an Android APK.
    # Require each bounded pattern in fetch AND checkout within its owning lane.
    $lane = [regex]::Match($WorkflowText,
        '(?ms)^  ' + [regex]::Escape($JobName) + ':[ \t]*\r?\n(.*?)(?=^  [A-Za-z0-9_-]+:[ \t]*\r?$|\z)')
    if (-not $lane.Success) { return $false }
    $fetch = [regex]::Matches($lane.Groups[1].Value,
        '(?s)\bgit[^\r\n]*lfs fetch\s+--include="([^"]+)"')
    $checkout = [regex]::Matches($lane.Groups[1].Value,
        '(?s)&& git lfs checkout\s+(.*?)(?=\r?\n\s*\r?\n|\z)')
    if ($fetch.Count -ne 1 -or $checkout.Count -ne 1) { return $false }
    $fetched = @($fetch[0].Groups[1].Value.Split(','))
    $checkedOut = @([regex]::Matches($checkout[0].Groups[1].Value, '"([^"]+)"') |
        ForEach-Object { $_.Groups[1].Value })
    foreach ($platform in @('android', 'windows')) {
        $requiredPattern = "assets/textures/props/runtime/*.$platform.ktx2"
        if (@($fetched | Where-Object { $_ -ceq $requiredPattern }).Count -ne 1 -or
            @($checkedOut | Where-Object { $_ -ceq $requiredPattern }).Count -ne 1) { return $false }
    }
    # An all-repository/all-props substitute must not masquerade as bounded hydration.
    foreach ($pattern in @('*', '**', 'assets/**', 'assets/textures/props/runtime/*.ktx2')) {
        if ($fetched -ccontains $pattern -or $checkedOut -ccontains $pattern) { return $false }
    }
    return $true
}
# Regress an inserted sibling lane and retain the missing-checkout negative gate.
$fixture = "  shared-gameplay:`n    fetch: assets/models/player/viewmodel/runtime/*.glb`n    checkout: assets/models/player/viewmodel/runtime/*.glb`n  inserted-lane:`n    fetch: assets/models/player/viewmodel/runtime/*.glb`n    checkout: assets/models/player/viewmodel/runtime/*.glb`n  player-vulkan-host:`n"
$withoutCheckout = $fixture.Replace(
    'checkout: assets/models/player/viewmodel/runtime/*.glb', 'checkout: none')
if (-not (Test-PortableViewmodelInventory $fixture) -or
    -not (Test-PortableViewmodelInventory ($fixture.Replace("`n", "`r`n"))) -or
    (Test-PortableViewmodelInventory $withoutCheckout) -or
    (Test-PortableViewmodelInventory '  absent-job:')) {
    throw 'Portable workflow job-boundary regression failed'
}
$workflow = Get-Content (Join-Path $repo '.github/workflows/shared-simulation-host.yml') -Raw
if (-not (Test-PortableViewmodelInventory $workflow)) {
    throw 'Portable manifest checks require viewmodel GLBs in both LFS fetch and checkout lists'
}
$propsPatterns = @('assets/textures/props/runtime/*.android.ktx2', 'assets/textures/props/runtime/*.windows.ktx2')
$propsFetch = '          --include="' + ($propsPatterns -join ',') + '"'
$propsCheckout = '          "' + ($propsPatterns -join "`"`n          `"") + '"'
foreach ($job in @('android-debug', 'shared-gameplay')) {
    $propsFixture = "  ${job}:`n    steps:`n      - name: Bounded payloads`n        run: >-`n          git -c lfs.fetchexclude= lfs fetch`n$propsFetch`n          && git lfs checkout`n$propsCheckout`n`n  inserted-lane:`n    ignored: true`n"
    if (-not (Test-PropsSourcePolicyInventory $propsFixture $job) -or
        -not (Test-PropsSourcePolicyInventory ($propsFixture.Replace("`n", "`r`n")) $job) -or
        -not (Test-PropsSourcePolicyInventory $workflow $job)) {
        throw "Both canonical props profiles require bounded fetch and checkout in $job"
    }
    foreach ($pattern in $propsPatterns) {
        foreach ($operation in @('fetch', 'checkout')) {
            $original = if ($operation -ceq 'fetch') { $propsFetch } else { $propsCheckout }
            $missing = $propsFixture.Replace($original, $original.Replace($pattern, 'omitted'))
            # A complete sibling cannot rescue a missing pattern in this lane.
            $sibling = $propsFixture.Replace("  ${job}:", '  unrelated-complete-lane:')
            if ((Test-PropsSourcePolicyInventory $missing $job) -or
                (Test-PropsSourcePolicyInventory ($missing + $sibling) $job)) {
                throw "Missing $operation $pattern must fail within $job"
            }
        }
    }
    $duplicateFetch = $propsFixture.Replace($propsFetch, $propsFetch.Replace($propsPatterns[0], $propsPatterns[0] + ',' + $propsPatterns[0]))
    $duplicateCheckout = $propsFixture.Replace($propsCheckout, $propsCheckout + "`n          `"" + $propsPatterns[1] + '"')
    $unbounded = $propsFixture.Replace($propsFetch, $propsFetch.Replace($propsPatterns[0], $propsPatterns[0] + ',assets/**'))
    if ((Test-PropsSourcePolicyInventory $duplicateFetch $job) -or
        (Test-PropsSourcePolicyInventory $duplicateCheckout $job) -or
        (Test-PropsSourcePolicyInventory $unbounded $job) -or
        (Test-PropsSourcePolicyInventory '  absent-job:' $job)) {
        throw "Duplicate, unbounded or absent props hydration must fail in $job"
    }
}
Write-Output 'Both canonical props profiles have bounded per-lane LFS fetch/checkout; missing/duplicate/sibling/unbounded guards passed.'
foreach ($relative in @('tools/package-alpha.ps1', 'tools/run-foundation-validation.ps1')) {
    $errors = $null
    $ast = [Management.Automation.Language.Parser]::ParseFile(
        (Join-Path $repo $relative), [ref]$null, [ref]$errors)
    if ($errors.Count) { throw "$relative does not parse: $errors" }
    $strings = @($ast.FindAll({ param($node)
        $node -is [Management.Automation.Language.StringConstantExpressionAst]
    }, $true) | ForEach-Object { $_.Value.Replace('\', '/') })
    foreach ($name in $required) {
        # One source-copy entry plus Windows ZIP and Android APK checks.
        if (@($strings | Where-Object { $_ -ceq $name }).Count -ne 3) {
            throw "$relative must stage and check both platform packages for $name"
        }
    }
}
$gradle = Get-Content (Join-Path $repo 'android/app/build.gradle') -Raw
$activity = Get-Content (Join-Path $repo 'android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java') -Raw
# APK inclusion is insufficient: the native loader reads the private files root.
# Keep the production pair in the unconditional held-item/player staging chain.
$staging = [regex]::Match($activity, '(?s)final boolean heldItemsStaged\s*=([^;]+);')
if (-not $staging.Success -or $staging.Groups[1].Value -match 'BuildConfig|\?|\|\|') {
    throw 'Production player staging must not be gated by a candidate/build switch'
}
foreach ($name in $required) {
    $entry = $name.Substring('assets/'.Length)
    if (-not $gradle.Contains("include '$entry'")) { throw "Default Android staging lacks $entry" }
    if (-not $staging.Groups[1].Value.Contains("stageAsset(`"$entry`", `"$entry`")")) {
        throw "Normal Android startup does not stage required runtime asset: $entry"
    }
}
if ($AndroidApk) {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [IO.Compression.ZipFile]::OpenRead([IO.Path]::GetFullPath($AndroidApk))
    try {
        foreach ($name in $required) {
            $entry = $archive.GetEntry($name)
            if ($null -eq $entry) { throw "APK lacks $name" }
            $stream = $entry.Open()
            $sha = [Security.Cryptography.SHA256]::Create()
            try { $actual = -join ($sha.ComputeHash($stream) | ForEach-Object { $_.ToString('x2') }) }
            finally { $sha.Dispose(); $stream.Dispose() }
            $expected = (Get-FileHash (Join-Path $repo $name) -Algorithm SHA256).Hash.ToLowerInvariant()
            if ($actual -cne $expected) { throw "APK has stale viewmodel bytes: $name" }
        }
        if (@($archive.Entries | Where-Object { $_.FullName -match 'processing\.json$' }).Count) {
            throw 'Processing receipts must not enter the runtime APK'
        }
    } finally { $archive.Dispose() }
}
Write-Output 'Viewmodel pair is staged and required in both platform package inventories.'
if ($AndroidApk) { Write-Output 'Actual APK viewmodel GLB/manifest bytes match the current source.' }
