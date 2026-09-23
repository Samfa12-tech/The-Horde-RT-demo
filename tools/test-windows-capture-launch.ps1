#requires -Version 7.0
param(
    [Parameter(Mandatory=$true)][string]$DebugExecutable,
    [Parameter(Mandatory=$true)][string]$ReleaseExecutable
)
$ErrorActionPreference = 'Stop'
$unusedOutput = Join-Path ([IO.Path]::GetTempPath()) ('horde-rejected-capture-' + [guid]::NewGuid().ToString('N'))
$cases = @(
    @{ name='portrait requires capture'; exe=$DebugExecutable;
       args=@('--capture-portrait'); message='--capture-portrait requires --capture-showcase.' },
    @{ name='duplicate portrait rejected'; exe=$DebugExecutable;
       args=@('--capture-portrait','--capture-portrait'); message='--capture-portrait may only be specified once.' },
    @{ name='portrait cannot select benchmark'; exe=$DebugExecutable;
       args=@('--benchmark-showcase',$unusedOutput,'--capture-portrait'); message='--capture-portrait requires --capture-showcase.' },
    @{ name='Release rejects portrait capture'; exe=$ReleaseExecutable;
       args=@('--capture-showcase',$unusedOutput,'--capture-portrait'); message='--capture-showcase is a Debug-only automation mode; Release builds reject it.' }
)
foreach ($case in $cases) {
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = (Resolve-Path -LiteralPath $case.exe).Path
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.WindowStyle = [Diagnostics.ProcessWindowStyle]::Hidden
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    foreach ($argument in $case.args) { $start.ArgumentList.Add($argument) }
    $process = [Diagnostics.Process]::Start($start)
    try {
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit(20000)) {
            $process.Kill()
            throw "Capture launch guard timed out: $($case.name)"
        }
        $output = $stdout.GetAwaiter().GetResult() + $stderr.GetAwaiter().GetResult()
        if ($process.ExitCode -ne 2 -or -not $output.Contains($case.message) -or
            $output.Contains('Probe initialisation:')) {
            throw "Capture launch guard failed: $($case.name); exit $($process.ExitCode); $output"
        }
        if (Test-Path -LiteralPath $unusedOutput) {
            throw "Rejected capture unexpectedly created output: $unusedOutput"
        }
        Write-Output "PASS $($case.name)"
    } finally { $process.Dispose() }
}
Write-Output 'All 4 capture launch guards passed before Vulkan initialization.'
