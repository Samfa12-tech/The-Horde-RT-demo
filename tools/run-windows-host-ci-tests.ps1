param([Parameter(Mandatory=$true)][string]$BuildDirectory,
      [string]$Configuration='Release', [string]$ExcludePattern='')
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$ctest=(Get-Command ctest -ErrorAction Stop).Source
$common=@('--test-dir',$BuildDirectory,'--build-config',$Configuration,'--output-on-failure','--no-tests=error')
$policy='horde_rt_horde_162_asset_policy_tests'
$listing=@('--test-dir',$BuildDirectory,'--build-config',$Configuration,'--show-only=json-v1')
if($ExcludePattern){$listing+=@('-E',$ExcludePattern)}
$json=& $ctest @listing
if($LASTEXITCODE -ne 0){throw 'Could not inspect the complete Windows CTest selection.'}
$names=@(($json | ConvertFrom-Json).tests | ForEach-Object name)
if(@($names | Where-Object {$_ -ceq $policy}).Count -ne 1){throw 'Asset-policy fixture must occur exactly once in the complete selection.'}
$remaining=$names | Where-Object {$_ -cne $policy}
if(@($remaining).Count -eq 0){throw 'Remaining Windows CTest selection is empty.'}
# Only the asset-policy process overlaps. It reads immutable repository inputs
# and owns a GUID-named temporary directory; the other tests remain sequential.
$start=[Diagnostics.ProcessStartInfo]::new()
$start.FileName=$ctest;$start.UseShellExecute=$false;$start.CreateNoWindow=$true
$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
foreach($arg in ($common+@('-R',"^$policy`$"))){$start.ArgumentList.Add($arg)}
$process=[Diagnostics.Process]::Start($start)
$stdout=$process.StandardOutput.ReadToEndAsync();$stderr=$process.StandardError.ReadToEndAsync()
try {
    # CTest's regex engine supports ordinary groups, not PCRE (?:...) groups.
    $restPattern=if($ExcludePattern){"($ExcludePattern)|^$policy`$"}else{"^$policy`$"}
    & $ctest @common -E $restPattern
    $restExit=$LASTEXITCODE
    $process.WaitForExit()
    Write-Output $stdout.Result
    if($stderr.Result){Write-Output $stderr.Result}
    Write-Output "Windows selection: $($names.Count) tests; asset-policy once plus $(@($remaining).Count) sequential tests."
    if($restExit -ne 0 -or $process.ExitCode -ne 0){throw "Windows CTest failure: remaining=$restExit; asset-policy=$($process.ExitCode)."}
} finally {
    # Never leave a fixture running after an interrupted host gate.
    if(-not $process.HasExited){$process.Kill($true);$process.WaitForExit()}
    $process.Dispose()
}
