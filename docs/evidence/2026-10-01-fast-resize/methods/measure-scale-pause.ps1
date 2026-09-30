param([Parameter(Mandatory)][string]$RunId,
      [Parameter(Mandatory)][string]$ExpectedApkSha256,
      [switch]$RequireFastPath)
$ErrorActionPreference='Stop'
$taskRoot = $PSScriptRoot
$adb = 'C:/Users/sam_s/AppData/Local/Android/Sdk/platform-tools/adb.exe'
$serial = 'R5GL219SZGK'
$package = 'com.samfa12.hordelanternrt.debug'
$out = Join-Path $taskRoot $RunId
if(Test-Path -LiteralPath $out){throw 'Unique output directory required.'}
New-Item -ItemType Directory -Path $out | Out-Null
if((& $adb -s $serial shell getprop ro.product.model).Trim() -cne 'SM-S948B'){throw 'Wrong phone.'}
$packagePath = (& $adb -s $serial shell pm path $package | Select-Object -First 1) -replace '^package:',''
& $adb -s $serial pull $packagePath.Trim() (Join-Path $out 'installed.apk') *> (Join-Path $out 'pull.txt')
if($LASTEXITCODE -ne 0){throw 'Installed package pull failed.'}
$actualHash = (Get-FileHash -LiteralPath (Join-Path $out 'installed.apk') -Algorithm SHA256).Hash.ToLowerInvariant()
if($actualHash -cne $ExpectedApkSha256){throw 'Installed APK identity mismatch.'}
$beforePid = (& $adb -s $serial shell pidof $package).Trim()
$beforeCaptures = 0
if($beforePid){$beforeCaptures = ([regex]::Matches(((& $adb -s $serial logcat -d -v epoch --pid=$beforePid -s 'HordeRtProbeBridge:I' '*:S') -join "`n"),'HORDE_CAPTURE_READY.*checkpoint=opening.*scale=75')).Count}
& $adb -s $serial shell am start -n "$package/com.samfa12.hordelanternrt.MainActivity" --es horde.debug.checkpoint opening --ei horde.debug.scale 75 --ez horde.debug.capture true --ez horde.debug.autostart true *> (Join-Path $out 'launch.txt')
$appPid = (& $adb -s $serial shell pidof $package).Trim()
if(-not $appPid){throw 'App has no process.'}
if($appPid -cne $beforePid){$beforeCaptures = 0}
$deadline = [DateTime]::UtcNow.AddSeconds(60)
do {
  Start-Sleep -Milliseconds 250
  $initialLog = (& $adb -s $serial logcat -d -v epoch --pid=$appPid -s 'HordeRtProbeBridge:I' 'HordeLanternAudio:I' '*:S') -join "`n"
} until(([regex]::Matches($initialLog,'HORDE_CAPTURE_READY.*checkpoint=opening.*scale=75')).Count -gt $beforeCaptures -or [DateTime]::UtcNow -gt $deadline)
if(([regex]::Matches($initialLog,'HORDE_CAPTURE_READY.*checkpoint=opening.*scale=75')).Count -le $beforeCaptures){throw 'Opening readiness timeout.'}
$rows = @()
foreach($scale in @(100,75,50,75)){
  $sequence = $rows.Count+1
  $requestUtc = [DateTime]::UtcNow
  & $adb -s $serial shell dumpsys battery | Set-Content -LiteralPath (Join-Path $out "battery-$sequence.txt")
  & $adb -s $serial shell dumpsys thermalservice | Set-Content -LiteralPath (Join-Path $out "thermal-$sequence.txt")
  $before = (& $adb -s $serial logcat -d -v epoch --pid=$appPid -s 'HordeRtProbeBridge:I' 'HordeLanternAudio:I' '*:S') -join "`n"
  $presentCount = ([regex]::Matches($before,'RT frame reached Android swapchain presentation\.')).Count
  & $adb -s $serial shell am start -n "$package/com.samfa12.hordelanternrt.MainActivity" --ei horde.debug.scale $scale --ez horde.debug.autostart true *> (Join-Path $out "request-$sequence.txt")
  $deadline = [DateTime]::UtcNow.AddSeconds(60)
  do {
    Start-Sleep -Milliseconds 100
    $log = (& $adb -s $serial logcat -d -v epoch --pid=$appPid -s 'HordeRtProbeBridge:I' 'HordeLanternAudio:I' '*:S') -join "`n"
  } until(([regex]::Matches($log,'RT frame reached Android swapchain presentation\.')).Count -gt $presentCount -or [DateTime]::UtcNow -gt $deadline)
  $log | Set-Content -LiteralPath (Join-Path $out "log-$sequence.txt")
  if(([regex]::Matches($log,'RT frame reached Android swapchain presentation\.')).Count -le $presentCount){throw "No fresh presentation for scale$scale."}
  if((& $adb -s $serial shell pidof $package).Trim() -cne $appPid){throw 'Process changed during in-place resize.'}
  $lines=$log -split "`n"
  $accepted=@($lines | Where-Object {$_ -match "Accepted debug automation intent:.*scale=$scale(?: |$)"})[-1]
  $present=@($lines | Where-Object {$_ -match 'RT frame reached Android swapchain presentation\.'})[-1]
  if(-not $accepted -or -not $present){throw 'Missing request/presentation timestamps.'}
  $startSeconds=[double]::Parse(($accepted.Trim() -split '\s+')[0],[cultureinfo]::InvariantCulture)
  $endSeconds=[double]::Parse(($present.Trim() -split '\s+')[0],[cultureinfo]::InvariantCulture)
  $fast=@($lines | Where-Object {$_ -match "HORDE_RT_SCALE_RESIZE.*scale=$scale(?: |$)"}) | Select-Object -Last 1
  if($RequireFastPath -and -not $fast){throw 'Missing fast-path resource-preservation receipt.'}
  $rows += [pscustomobject]@{sequence=$sequence;scale=$scale;requestUtc=$requestUtc.ToString('o');pid=$appPid;requestLog=$accepted;presentationLog=$present;requestToPresentedMs=($endSeconds-$startSeconds)*1000;fastPathLog=$fast}
  Write-Output "scale$scale request-to-present=$([math]::Round($rows[-1].requestToPresentedMs,2))ms"
}
[pscustomobject]@{schema=1;device='SM-S948B';apkSha256=$actualHash;method='Same-process Debug intent acceptance to successful native swapchain presentation log timestamps, not touchscreen/compositor latency or Shipping FPS';cooled=$false;rows=$rows} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $out 'summary.json')
& $adb -s $serial shell input keyevent KEYCODE_HOME
