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

Assert-Registration $ConfiguredCtestFile $Configuration $Instrumentation $Quality

$temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) (
    'horde-rt-containment-registration-' + [guid]::NewGuid().ToString('N'))
try {
    $arguments = @(
        '-S', $repoRoot, '-B', $temporaryRoot, '-G', $Generator,
        '-DHORDE_RT_INSTRUMENTATION_OVERRIDE=Shipping',
        '-DHORDE_RT_DIELECTRIC_QUALITY_OVERRIDE=Mobile')
    if (-not [string]::IsNullOrWhiteSpace($GeneratorPlatform)) {
        $arguments += @('-A', $GeneratorPlatform)
    }
    & $CmakeExecutable @arguments
    Assert-True ($LASTEXITCODE -eq 0) 'Paired containment-override tree failed to configure.'
    $overrideCtest = Join-Path $temporaryRoot 'CTestTestfile.cmake'
    Assert-Registration $overrideCtest 'Debug' 'Shipping' 'Mobile'
    Assert-Registration $overrideCtest 'Release' 'Shipping' 'Mobile'
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}

Write-Output 'Effective final-target containment registrations passed.'
