param(
    [string]$RepoRoot = (Split-Path $PSScriptRoot -Parent),
    [string]$CmakeExecutable = 'cmake'
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path -LiteralPath $RepoRoot).Path
$cmake = (Get-Command $CmakeExecutable -ErrorAction Stop).Source
$module = (Join-Path $repo 'cmake/HordeRtPocketAudioCore.cmake').Replace('\','/')
$fixtureRoot = Join-Path ([IO.Path]::GetTempPath()) ('horde-core-pin-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $fixtureRoot | Out-Null
$utf8 = [Text.UTF8Encoding]::new($false)
$checks = 0
$complete = $false

function Assert-Pin([string]$Root, [string]$Name, [string]$Failure = '') {
    $script = Join-Path $fixtureRoot ($Name + '.cmake')
    $log = Join-Path $fixtureRoot ($Name + '.log')
    $cmakeRoot = $Root.Replace('\','/')
    [IO.File]::WriteAllText($script, "cmake_minimum_required(VERSION 3.22)`ninclude(`"$module`")`nhorde_rt_validate_pocket_audio_core(`"$cmakeRoot`")`n", $utf8)
    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $cmake -P $script *> $log
        $code = $LASTEXITCODE
    } finally { $ErrorActionPreference = $previousPreference }
    $output = Get-Content -LiteralPath $log -Raw
    if ($Failure -eq '') {
        if ($code -ne 0) { throw "$Name unexpectedly failed: $output" }
    } elseif ($code -eq 0 -or -not $output.Contains($Failure)) {
        throw "$Name did not reject its intended failure '$Failure': $output"
    }
    $script:checks++
}

function New-CoreFixture([string]$Name) {
    $root = Join-Path $fixtureRoot $Name
    $target = Join-Path $root 'third_party/pocket-audio-core'
    $source = Join-Path $repo 'third_party/pocket-audio-core'
    $manifest = Get-Content -LiteralPath (Join-Path $source 'manifest.json') -Raw | ConvertFrom-Json
    foreach ($relative in @($manifest.files.path) + @('manifest.json','README.horde.md')) {
        $destination = Join-Path $target $relative
        New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
        Copy-Item -LiteralPath (Join-Path $source $relative) -Destination $destination
    }
    return $root
}

try {
    Assert-Pin $repo 'current-source'
    Assert-Pin (New-CoreFixture 'exact-copy') 'exact-copy'

    $changed = New-CoreFixture 'changed-source'
    [IO.File]::AppendAllText((Join-Path $changed 'third_party/pocket-audio-core/native/src/PcmLoopStream.cpp'), "// fixture mutation`n", $utf8)
    Assert-Pin $changed 'changed-source' 'Pocket Audio Core file pin mismatch'

    $missing = New-CoreFixture 'missing-header'
    Remove-Item -LiteralPath (Join-Path $missing 'third_party/pocket-audio-core/native/include/pocket_audio/PcmWave.h')
    Assert-Pin $missing 'missing-header' 'Pocket Audio Core file is missing'

    $extra = New-CoreFixture 'unexpected-app'
    [IO.File]::WriteAllText((Join-Path $extra 'third_party/pocket-audio-core/native/editor.js'), '// fixture only', $utf8)
    Assert-Pin $extra 'unexpected-app' 'Pocket Audio Core native-only inventory mismatch'

    $repinned = New-CoreFixture 'changed-manifest'
    $manifestPath = Join-Path $repinned 'third_party/pocket-audio-core/manifest.json'
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $manifest.upstreamCommit = 'f' * 40
    [IO.File]::WriteAllText($manifestPath, ($manifest | ConvertTo-Json -Depth 6), $utf8)
    Assert-Pin $repinned 'changed-manifest' 'Pocket Audio Core manifest pin mismatch'

    $noManifest = New-CoreFixture 'missing-manifest'
    Remove-Item -LiteralPath (Join-Path $noManifest 'third_party/pocket-audio-core/manifest.json')
    Assert-Pin $noManifest 'missing-manifest' 'Pocket Audio Core manifest is missing'

    $complete = $true
    Write-Output "Pocket Audio Core pin admission: $checks/7 checks PASS"
} finally {
    if ($complete) {
        $resolvedFixture = [IO.Path]::GetFullPath($fixtureRoot)
        $resolvedTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar
        if (-not $resolvedFixture.StartsWith($resolvedTemp, [StringComparison]::OrdinalIgnoreCase) -or
            -not [IO.Path]::GetFileName($resolvedFixture).StartsWith('horde-core-pin-')) {
            throw 'Refusing cleanup outside the exact generated test-fixture root'
        }
        Remove-Item -LiteralPath $resolvedFixture -Recurse -Force
    } else {
        Write-Output "Failed test fixtures retained: $fixtureRoot"
    }
}
