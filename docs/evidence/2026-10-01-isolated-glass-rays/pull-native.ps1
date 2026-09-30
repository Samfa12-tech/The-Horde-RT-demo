param([Parameter(Mandatory=$true)][string]$Destination)
$ErrorActionPreference = 'Stop'
$adb = 'C:/Users/sam_s/AppData/Local/Android/Sdk/platform-tools/adb.exe'
$serial = 'R5GL219SZGK'
$package = 'com.samfa12.hordelanternrt.debug'
$remote = 'files/reports/isolated-transport.rgba'
if (Test-Path -LiteralPath $Destination) { throw 'Never overwrite an immutable native capture.' }
if ((& $adb -s $serial shell getprop ro.product.model | Out-String).Trim() -cne 'SM-S948B') { throw 'Wrong device.' }
$remoteHash = (& $adb -s $serial shell run-as $package sha256sum $remote | Out-String).Trim().Split(' ')[0]
if ($LASTEXITCODE -ne 0 -or $remoteHash -notmatch '^[a-f0-9]{64}$') { throw 'Native capture hash unavailable.' }
$info = [Diagnostics.ProcessStartInfo]::new()
$info.FileName = $adb
$info.UseShellExecute = $false
$info.CreateNoWindow = $true
$info.RedirectStandardOutput = $true
$info.RedirectStandardError = $true
foreach ($arg in @('-s', $serial, 'exec-out', 'run-as', $package, 'cat', $remote)) { $info.ArgumentList.Add($arg) }
$process = [Diagnostics.Process]::Start($info)
$output = [IO.File]::Open($Destination, [IO.FileMode]::CreateNew)
try { $process.StandardOutput.BaseStream.CopyTo($output) } finally { $output.Dispose() }
$errorText = $process.StandardError.ReadToEnd()
$process.WaitForExit()
if ($process.ExitCode -ne 0) { throw "Native capture transfer failed: $errorText" }
$hash = (Get-FileHash -LiteralPath $Destination).Hash.ToLowerInvariant()
if ($hash -cne $remoteHash) { throw 'Native byte transfer hash mismatch.' }
$bytes = [IO.File]::ReadAllBytes($Destination)
$width = [BitConverter]::ToUInt32($bytes, 0)
$height = [BitConverter]::ToUInt32($bytes, 4)
if ($width -ne 1080 -or $height -ne 2235 -or $bytes.Length -ne 8 + $width * $height * 4) { throw 'Wrong extent/byte count.' }
[ordered]@{deviceModel='SM-S948B';package=$package;remote=$remote;destination=$Destination;width=$width;height=$height;bytes=$bytes.Length;sha256=$hash;remoteSha256=$remoteHash;transfer='native bytes; no Android screenshot resampling';result='PASS'} | ConvertTo-Json
