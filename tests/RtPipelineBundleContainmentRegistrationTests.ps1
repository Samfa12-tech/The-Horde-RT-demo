[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$CmakeExecutable,
    [Parameter(Mandatory = $true)][string]$ConfiguredCtestFile,
    [Parameter(Mandatory = $true)][string]$Configuration,
    [Parameter(Mandatory = $true)][ValidateSet('Shipping','Diagnostic')][string]$Instrumentation,
    [Parameter(Mandatory = $true)][ValidateSet('Mobile','High')][string]$Quality,
    [Parameter(Mandatory = $true)][string]$Generator,
    [string]$GeneratorPlatform
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
function Assert-True([bool]$condition, [string]$message) {
    if (-not $condition) { throw $message }
}
function Assert-Registration(
    [string]$ctestFile,
    [string]$configuration,
    [string]$instrumentation,
    [string]$quality)
{
    $lines = @(Get-Content -LiteralPath $ctestFile | Where-Object {
        $_.Contains('add_test([=[horde_rt_final_windows_bundle_containment') -and
        $_.Contains("/$configuration/HordeLanternRT.exe")
    })
    Assert-True ($lines.Count -eq 2) "Expected scanner and control registrations for $configuration."
    foreach ($line in $lines) {
        Assert-True ($line.Contains('"-TargetPlatform" "Windows"')) `
            "$configuration containment registration lost the Windows target identity."
        Assert-True ($line.Contains("`"-Instrumentation`" `"$instrumentation`"")) `
            "$configuration containment registration expected $instrumentation."
        Assert-True ($line.Contains("`"-Quality`" `"$quality`"")) `
            "$configuration containment registration expected $quality."
    }
}

function Assert-MobileLifetimePolicy(
    [string]$instrumentation,
    [string]$temporaryRoot,
    [string]$ctestExecutable)
{
    $arguments = @(
        '-S', $repoRoot, '-B', $temporaryRoot, '-G', $Generator,
        "-DHORDE_RT_INSTRUMENTATION_OVERRIDE=$instrumentation",
        '-DHORDE_RT_DIELECTRIC_QUALITY_OVERRIDE=Mobile')
    if (-not [string]::IsNullOrWhiteSpace($GeneratorPlatform)) {
        $arguments += @('-A', $GeneratorPlatform)
    }
    & $CmakeExecutable @arguments
    Assert-True ($LASTEXITCODE -eq 0) "$instrumentation/Mobile tree failed to configure."

    $overrideCtest = Join-Path $temporaryRoot 'CTestTestfile.cmake'
    Assert-Registration $overrideCtest 'Debug' $instrumentation 'Mobile'
    Assert-Registration $overrideCtest 'Release' $instrumentation 'Mobile'

    & $CmakeExecutable --build $temporaryRoot --config Debug `
        --target horde_rt_pipeline_bundle_lifetime_tests --parallel
    Assert-True ($LASTEXITCODE -eq 0) `
        "$instrumentation/Mobile lifetime target failed to build."
    & $ctestExecutable --test-dir $temporaryRoot -C Debug --output-on-failure `
        -R '^horde_rt_pipeline_bundle_lifetime_tests$'
    Assert-True ($LASTEXITCODE -eq 0) `
        "$instrumentation/Mobile lifetime target failed to execute."
}

Assert-Registration $ConfiguredCtestFile $Configuration $Instrumentation $Quality

$cmakeItem = Get-Item -LiteralPath $CmakeExecutable
$ctestFileName = if ($cmakeItem.Extension -eq '.exe') { 'ctest.exe' } else { 'ctest' }
$ctestExecutable = Join-Path $cmakeItem.DirectoryName $ctestFileName
Assert-True (Test-Path -LiteralPath $ctestExecutable -PathType Leaf) `
    'CTest must be installed beside the configured CMake executable.'

$temporaryParent = Join-Path $repoRoot 'build'
New-Item -ItemType Directory -Path $temporaryParent -Force | Out-Null
foreach ($mobileInstrumentation in @('Shipping', 'Diagnostic')) {
    $temporaryRoot = Join-Path $temporaryParent (
        'rtm-' + $mobileInstrumentation.Substring(0, 1).ToLowerInvariant() + '-' +
        [guid]::NewGuid().ToString('N').Substring(0, 12))
    try {
        Assert-MobileLifetimePolicy $mobileInstrumentation $temporaryRoot $ctestExecutable
    }
    finally {
        if (Test-Path -LiteralPath $temporaryRoot) {
            Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
        }
    }
}

Write-Output 'Effective final-target containment registrations and Mobile lifetime policies passed.'
