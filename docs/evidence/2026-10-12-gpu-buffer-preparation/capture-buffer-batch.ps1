param([Parameter(Mandatory=$true)][string]$Output,
      [string]$Checkpoint,[switch]$Preview,[switch]$Query)
$ErrorActionPreference='Stop'
$root=(Get-Location).Path
$destination=Join-Path $root $Output
if(Test-Path -LiteralPath $destination){throw 'Choose a fresh capture directory'}
if($Preview -eq (-not [string]::IsNullOrEmpty($Checkpoint))){throw 'Choose preview or one checkpoint'}
$start=[Diagnostics.ProcessStartInfo]::new()
$start.FileName=Join-Path $root 'build/presets/windows-x64-debug/Debug/HordeLanternRT.exe'
$start.WorkingDirectory=$root;$start.UseShellExecute=$false;$start.CreateNoWindow=$true
$start.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
$arguments=if($Preview){@('--capture-graphics-preview',$destination)}else{
    @('--development-rescue-journey','--capture-showcase',$destination,'--development-checkpoint',$Checkpoint)}
foreach($argument in $arguments){$start.ArgumentList.Add($argument)}
if($Query){$start.ArgumentList.Add('--require-rayquery-compute')}
$process=[Diagnostics.Process]::Start($start)
$out=$process.StandardOutput.ReadToEndAsync();$err=$process.StandardError.ReadToEndAsync()
$timedOut=-not $process.WaitForExit(180000)
if($timedOut){$process.Kill();$process.WaitForExit()}
[IO.File]::WriteAllText($destination+'.stdout.log',$out.Result)
[IO.File]::WriteAllText($destination+'.stderr.log',$err.Result)
if($timedOut){throw 'Scoped capture exceeded its unchanged 180-second deadline'}
Write-Output ("$Output capture exit: "+$process.ExitCode)
$global:LASTEXITCODE=$process.ExitCode
if($process.ExitCode -ne 0){exit $process.ExitCode}
