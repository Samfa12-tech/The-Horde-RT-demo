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
