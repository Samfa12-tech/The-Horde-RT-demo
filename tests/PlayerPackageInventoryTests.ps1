[CmdletBinding()]
param([string]$AndroidApk = '')
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$required = @(
    'assets/models/player/viewmodel/runtime/asset.manifest.json',
    'assets/models/player/viewmodel/runtime/gothic-traveller-viewmodel.runtime.glb')
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
foreach ($name in $required) {
    $entry = $name.Substring('assets/'.Length)
    if (-not $gradle.Contains("include '$entry'")) { throw "Default Android staging lacks $entry" }
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
