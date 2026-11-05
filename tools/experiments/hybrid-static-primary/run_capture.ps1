param([Parameter(Mandatory=$true)][string]$Output,[Parameter(Mandatory=$true)][string]$Executable,[Parameter(Mandatory=$true)][string]$RepoRoot,[string]$Checkpoint='chapter-niche-rect',[switch]$Hybrid,[switch]$Query,[switch]$Validation,[switch]$Portrait)
$ErrorActionPreference='Stop'
$hordeRoot=[IO.Path]::GetFullPath($RepoRoot)
$captureOutput=[IO.Path]::GetFullPath($Output)
if(Test-Path -LiteralPath $captureOutput){throw 'Choose a fresh capture path; previous evidence is preserved'}
New-Item -ItemType Directory -Path $captureOutput -Force | Out-Null
$captureStart=[Diagnostics.ProcessStartInfo]::new()
$captureStart.FileName=[IO.Path]::GetFullPath($Executable)
$captureStart.WorkingDirectory=$hordeRoot
$captureStart.UseShellExecute=$false;$captureStart.CreateNoWindow=$true;$captureStart.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
$captureStart.RedirectStandardOutput=$true;$captureStart.RedirectStandardError=$true
foreach($argument in @('--development-rescue-journey','--capture-showcase',$captureOutput,'--development-checkpoint',$Checkpoint,'--capture-hidden')){$captureStart.ArgumentList.Add($argument)}
if($Hybrid){$captureStart.ArgumentList.Add('--hybrid-static-primary')}
if($Query){$captureStart.ArgumentList.Add('--require-rayquery-compute')}
if($Portrait){$captureStart.ArgumentList.Add('--capture-portrait')}
if($Validation){$captureStart.Environment['VK_INSTANCE_LAYERS']='VK_LAYER_KHRONOS_validation';$captureStart.Environment['VK_LOADER_DEBUG']='layer,warn,error'}
$captureProcess=[Diagnostics.Process]::Start($captureStart)
$captureOut=$captureProcess.StandardOutput.ReadToEndAsync();$captureErr=$captureProcess.StandardError.ReadToEndAsync()
if(!$captureProcess.WaitForExit(180000)){$captureProcess.Kill();throw 'Capture exceeded 180 seconds; own process stopped'}
[IO.File]::WriteAllText((Join-Path $captureOutput 'stdout.log'),$captureOut.Result)
[IO.File]::WriteAllText((Join-Path $captureOutput 'stderr.log'),$captureErr.Result)
[ordered]@{checkpoint=$Checkpoint;hybridRequested=[bool]$Hybrid;queryRequested=[bool]$Query;validationLayerRequested=[bool]$Validation;exitCode=$captureProcess.ExitCode;executableSha256=(Get-FileHash -LiteralPath $captureStart.FileName).Hash.ToLowerInvariant()} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $captureOutput 'launch.json')
Get-ChildItem -LiteralPath $captureOutput | Select-Object Name,Length
if($captureProcess.ExitCode){Get-Content -LiteralPath (Join-Path $captureOutput 'stderr.log');exit $captureProcess.ExitCode}
