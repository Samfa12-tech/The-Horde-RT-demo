param([string] $SourceRoot = 'C:\Dev\tmp\horde-final-s26-interactive-20261003')

# One-time allowlist archive of completed checks; this does not rerun the game.
$ErrorActionPreference = 'Stop'
$destination = Join-Path $PSScriptRoot 'raw'
if (Test-Path -LiteralPath $destination) { throw 'Refusing to overwrite frozen evidence.' }
New-Item -ItemType Directory -Path $destination | Out-Null
$names = @(
    'debug-shipping-containment.json', 'release-shipping-containment.json',
    'shipping-live-ui.xml', 'shipping-live.png', 'heavy-live-ui.xml',
    'heavy-live.png', 'motion-02.png', 'motion-04.png', 'motion-06.png',
    'settings-before-ui.xml', 'settings-scale-100-1270-ui.xml',
    'settings-scale-50-168-ui.xml', 'settings-scale-75-720-ui.xml',
    'post-resize-live-ui.xml', 'post-resize-live.png',
    'home-return-ui.xml', 'home-return-ready-ui.xml', 'scale-resize.log'
)
$lines = [System.Collections.Generic.List[string]]::new()
foreach ($name in $names) {
    $source = Join-Path $SourceRoot $name
    $target = Join-Path $destination $name
    [IO.File]::Copy($source, $target, $false)
    $hash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant()
    if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToLowerInvariant() -cne $hash) {
        throw "Copy identity failed: $name"
    }
    $lines.Add("$hash`t$((Get-Item -LiteralPath $target).Length)`traw/$name")
}
$nativeLog = Join-Path $SourceRoot 'lifecycle-resize-music.log'
$subset = @(Get-Content -LiteralPath $nativeLog | Where-Object {
    $_ -match 'HORDE_SURFACE_|HORDE_RT_SCALE_RESIZE|HordeLanternMusic|FATAL EXCEPTION|ANR'
})
if (@($subset | Where-Object { $_ -match 'HORDE_RT_SCALE_RESIZE' }).Count -ne 4 -or
    -not ($subset -match 'HORDE_SURFACE_PRESENTED generation=5')) {
    throw 'Expected completed resize/resume markers absent.'
}
$filtered = Join-Path $destination 'game-lifecycle-music.log'
[IO.File]::WriteAllLines($filtered, $subset, [Text.UTF8Encoding]::new($false))
$lines.Add("$((Get-FileHash -LiteralPath $filtered -Algorithm SHA256).Hash.ToLowerInvariant())`t$((Get-Item -LiteralPath $filtered).Length)`traw/game-lifecycle-music.log")
$lines.Add("$((Get-FileHash -LiteralPath $nativeLog -Algorithm SHA256).Hash.ToLowerInvariant())`t$((Get-Item -LiteralPath $nativeLog).Length)`tLOCAL_SOURCE/lifecycle-resize-music.log")

$localArtifacts = @{
    'HordeLanternRT-current-Shipping-Mobile-debug.apk' = '6eab75f8d829c8756a5c45f222f77104954b4010d24f7a9d479893d6e480f85a'
    'installed-shipping-debug.apk' = '6eab75f8d829c8756a5c45f222f77104954b4010d24f7a9d479893d6e480f85a'
    'HordeLanternRT-1.6.1-UNSIGNED-MOBILE-DO-NOT-PUBLISH.apk' = '79ceaa7a9ba4faa10410ee3e174fdc5ca58906983cc2b1342acc242e4fbe14e9'
    'debug-arm64.so' = 'cf65cc3cc0aaa8e38a9ab0b3e22624aaf83461445e81292c3b2c05e034e01f74'
    'release-arm64.so' = 'b836454af1d4a0d51b7958d816b14d31073a08d83ecb45505188ccdef344cd71'
}
foreach ($name in ($localArtifacts.Keys | Sort-Object)) {
    $path = Join-Path $SourceRoot $name
    $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($hash -cne $localArtifacts[$name]) { throw "Local artifact identity mismatch: $name" }
    $lines.Add("$hash`t$((Get-Item -LiteralPath $path).Length)`tLOCAL_ARTIFACT/$name")
}
foreach ($name in @('shipping-motion.mp4')) {
    $path = Join-Path $SourceRoot $name
    $lines.Add("$((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant())`t$((Get-Item -LiteralPath $path).Length)`tLOCAL_ARTIFACT/$name")
}
foreach ($name in @('README.md', 'archive-evidence.ps1')) {
    $path = Join-Path $PSScriptRoot $name
    $lines.Add("$((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant())`t$((Get-Item -LiteralPath $path).Length)`t$name")
}
[IO.File]::WriteAllLines((Join-Path $PSScriptRoot 'SHA256SUMS.txt'), $lines, [Text.UTF8Encoding]::new($false))
"Archived $($names.Count + 1) bounded evidence files; local artifact identities verified."
