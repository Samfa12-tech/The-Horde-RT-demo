param([Parameter(Mandatory)][string]$RunId,[Parameter(Mandatory)][string]$ExpectedApkSha256,[switch]$RequireFastPath)
$ErrorActionPreference='Stop'
$adb='C:/Users/sam_s/AppData/Local/Android/Sdk/platform-tools/adb.exe';$serial='R5GL219SZGK';$package='com.samfa12.hordelanternrt.debug'
$out=Join-Path $PSScriptRoot $RunId
if(Test-Path -LiteralPath $out){throw 'Unique run required.'};New-Item -ItemType Directory -Path $out | Out-Null
if((& $adb -s $serial shell getprop ro.product.model).Trim() -cne 'SM-S948B'){throw 'Wrong phone.'}
$appPid=(& $adb -s $serial shell pidof $package).Trim();if(-not $appPid){throw 'Open the app Settings first.'}
$base=((& $adb -s $serial shell pm path $package | Select-Object -First 1) -replace '^package:','').Trim()
& $adb -s $serial pull $base (Join-Path $out 'installed.apk') *> (Join-Path $out 'pull.txt')
$hash=(Get-FileHash -LiteralPath (Join-Path $out 'installed.apk') -Algorithm SHA256).Hash.ToLowerInvariant()
if($hash -cne $ExpectedApkSha256){throw 'Wrong actual APK.'}
function Get-Log{((& $adb -s $serial logcat -d -v epoch --pid=$appPid -s 'HordeRtProbeBridge:I' 'HordeLanternAudio:I' '*:S') -join "`n")}
$rows=@()
foreach($scale in @(100,75,50,75)){
 $seq=$rows.Count+1
 $beforeReportText=(& $adb -s $serial shell run-as $package cat files/reports/vulkan_capability_report.json) -join "`n"
 $beforeReport=$beforeReportText | ConvertFrom-Json
 $beforeReportText | Set-Content -LiteralPath (Join-Path $out "capability-before-$seq.json")
 $beforeEpoch=$beforeReport.rtFrameEvidence.completedFrame.identity.sceneEpoch
 $expectedBackend=$beforeReport.rtFrameEvidence.completedFrame.pipeline.executionBackend
 if(-not $beforeEpoch -or -not $expectedBackend){throw 'Missing current owning-frame backend/epoch before measurement.'}
 & $adb -s $serial shell uiautomator dump /sdcard/horde-resize-ui.xml *> $null
 $xmlText=(& $adb -s $serial shell cat /sdcard/horde-resize-ui.xml) -join "`n"
 $xmlText | Set-Content -LiteralPath (Join-Path $out "ui-before-$seq.xml")
 $ui=[xml]$xmlText;$label=@($ui.SelectNodes('//node[starts-with(@text,"Render resolution")]'))
 if($label.Count -ne 1 -or $label[0].bounds -cne '[154,1493][1286,1635]'){throw 'Settings layout changed: inspect before touching.'}
 $seek=@($ui.SelectNodes('//node[@class="android.widget.SeekBar" and @bounds="[154,1635][1286,1712]"]'))
 if($seek.Count -ne 1){throw 'Render-scale control not in observed bounds.'}
 $x=switch($scale){100{1268}75{720}50{172}}
 $before=Get-Log;$presentCount=([regex]::Matches($before,'RT frame reached Android swapchain presentation\.')).Count
 $surfaceCount=([regex]::Matches($before,'Started Android diagnostic surface rendering loop\.')).Count
 & $adb -s $serial shell dumpsys battery | Set-Content -LiteralPath (Join-Path $out "battery-$seq.txt")
 & $adb -s $serial shell dumpsys thermalservice | Set-Content -LiteralPath (Join-Path $out "thermal-$seq.txt")
 $request=(& $adb -s $serial shell "date +%s.%N; input tap $x 1673") -join "`n"
 $request | Set-Content -LiteralPath (Join-Path $out "request-$seq.txt")
 $startSeconds=[double]::Parse($request.Trim(),[cultureinfo]::InvariantCulture)
 $deadline=[DateTime]::UtcNow.AddSeconds(60)
 do{Start-Sleep -Milliseconds 100;$log=Get-Log}until(([regex]::Matches($log,'RT frame reached Android swapchain presentation\.')).Count -gt $presentCount -or [DateTime]::UtcNow -gt $deadline)
 $log | Set-Content -LiteralPath (Join-Path $out "log-$seq.txt")
 if(([regex]::Matches($log,'RT frame reached Android swapchain presentation\.')).Count -le $presentCount){throw 'No fresh presentation.'}
 if(([regex]::Matches($log,'Started Android diagnostic surface rendering loop\.')).Count -ne $surfaceCount){throw 'Surface restart contaminates inline resize measurement.'}
 if((& $adb -s $serial shell pidof $package).Trim() -cne $appPid){throw 'Process changed.'}
 $present=@(($log -split "`n") | Where-Object {$_ -match 'RT frame reached Android swapchain presentation\.'})[-1]
 $endSeconds=[double]::Parse(($present.Trim() -split '\s+')[0],[cultureinfo]::InvariantCulture)
 $fast=@(($log -split "`n") | Where-Object {$_ -match "HORDE_RT_SCALE_RESIZE.*scale=$scale(?: |$)"}) | Select-Object -Last 1
 if($RequireFastPath -and -not $fast){throw 'Missing fast-path preservation witness.'}
 $reportDeadline=[DateTime]::UtcNow.AddSeconds(15)
 do {
  $afterReportText=(& $adb -s $serial shell run-as $package cat files/reports/vulkan_capability_report.json) -join "`n"
  $afterReport=$afterReportText | ConvertFrom-Json
  $packet=$afterReport.rtFrameEvidence.completedFrame
  $reportFresh=$packet.identity.sceneEpoch -gt $beforeEpoch -and $packet.presentation.presented -and $packet.pipeline.executionBackend -ceq $expectedBackend -and $afterReport.rtScene.dispatchWidth -eq (1440*$scale/100) -and $afterReport.rtScene.dispatchHeight -eq (2980*$scale/100)
  if(-not $reportFresh){Start-Sleep -Milliseconds 100}
 } until($reportFresh -or [DateTime]::UtcNow -gt $reportDeadline)
 $afterReportText | Set-Content -LiteralPath (Join-Path $out "capability-after-$seq.json")
 if(-not $reportFresh){throw 'No new-epoch, correct-extent, same-backend presented owning packet.'}
 & $adb -s $serial shell uiautomator dump /sdcard/horde-resize-ui.xml *> $null
 $afterXml=(& $adb -s $serial shell cat /sdcard/horde-resize-ui.xml) -join "`n";$afterXml | Set-Content -LiteralPath (Join-Path $out "ui-after-$seq.xml")
 $afterUi=[xml]$afterXml;$actualLabel=@($afterUi.SelectNodes('//node[starts-with(@text,"Render resolution")]'))
 if($actualLabel.Count -ne 1 -or $actualLabel[0].text -cne "Render resolution  $scale%"){throw "Wrong actual scale label: $($actualLabel[0].text)"}
 $rows += [pscustomobject]@{sequence=$seq;scale=$scale;pid=$appPid;requestDeviceEpochSeconds=$startSeconds;presentationLog=$present;requestToPresentedMs=($endSeconds-$startSeconds)*1000;surfaceRestart=$false;fastPathLog=$fast;beforeEpoch=$beforeEpoch;afterEpoch=$packet.identity.sceneEpoch;executionBackend=$expectedBackend;resources=$packet.resources}
 Write-Output "Settings scale$scale request-to-present=$([math]::Round($rows[-1].requestToPresentedMs,2))ms"
}
[pscustomobject]@{schema=2;model='SM-S948B';apkSha256=$hash;cooled=$false;method='Observed Settings SeekBar, same process/surface; device shell timestamp preceding touch to first successful RT swapchain presentation. Includes input/debounce/whole-device idle; excludes compositor/touchscreen latency. Not Shipping FPS. Subsequent owning-packet/extent/backend validation is outside the timed interval.';rows=$rows} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $out 'summary.json')
