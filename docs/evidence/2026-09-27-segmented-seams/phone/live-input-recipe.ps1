$ErrorActionPreference='Stop'
$adb='C:/Users/sam_s/AppData/Local/Android/Sdk/platform-tools/adb.exe'
$serial='R5GL219SZGK'
$destination='C:/Dev/tmp/horde-segmented-seams-android-20260927-b'
$remote='/sdcard/horde-segmented-seam-live-20260927-a.mp4'
$local=Join-Path $destination 'live-motion-a.mp4'
if (Test-Path -LiteralPath $local) {throw 'Never overwrite recorded evidence'}
$focus=(& $adb -s $serial shell dumpsys window | Select-String 'mCurrentFocus').ToString()
if ($focus -notmatch 'com.samfa12.hordelanternrt.debug.viewmodel') {throw 'Game is not foreground'}
$record=Start-Process -FilePath $adb -WindowStyle Hidden -PassThru -ArgumentList @(
    '-s',$serial,'shell','screenrecord','--size','720x1560','--bit-rate','4000000',
    '--time-limit','45',$remote) -RedirectStandardError (Join-Path $destination 'record-a-stderr.txt')
$clock=[Diagnostics.Stopwatch]::StartNew()
$events=[Collections.Generic.List[object]]::new()
function Input-Step([string]$label,[string[]]$arguments) {
    $events.Add([pscustomobject]@{seconds=$clock.Elapsed.TotalSeconds;action=$label;arguments=$arguments})
    & $adb -s $serial shell input @arguments
    if ($LASTEXITCODE -ne 0) {throw "Input failed: $label"}
}
try {
    Start-Sleep -Seconds 1
    Input-Step 'low lantern parry hold' @('swipe','768','2882','768','2882','650')
    Start-Sleep -Milliseconds 600
    Input-Step 'low lantern swing' @('tap','1174','2882')
    Start-Sleep -Milliseconds 350
    Input-Step 'queue next swing' @('tap','1174','2882')
    Start-Sleep -Milliseconds 900
    Input-Step 'look down' @('swipe','1080','800','1080','2250','1300')
    Input-Step 'walk while looking down' @('swipe','300','1600','300','1200','2000')
    Input-Step 'diagonal walk while looking down' @('swipe','300','1600','450','1250','1600')
    Input-Step 'look back up' @('swipe','1080','2250','1080','800','1300')
    Input-Step 'raise lantern' @('tap','642','2609')
    Start-Sleep -Milliseconds 1200
    Input-Step 'high lantern parry hold' @('swipe','768','2882','768','2882','650')
    Start-Sleep -Milliseconds 500
    Input-Step 'high lantern swing' @('tap','1174','2882')
    Start-Sleep -Milliseconds 900
    Input-Step 'look right and up' @('swipe','1050','1400','1180','1150','900')
    Input-Step 'return yaw toward corridor' @('swipe','1180','1150','1050','1400','900')
    Input-Step 'lower lantern' @('tap','642','2609')
    Start-Sleep -Milliseconds 1200
    Input-Step 'look down again' @('swipe','1080','800','1080','2250','1300')
    Input-Step 'walk down-view again' @('swipe','300','1600','300','1200','2000')
    Input-Step 'down-view low lantern parry' @('swipe','768','2882','768','2882','650')
    Input-Step 'down-view swing' @('tap','1174','2882')
    Start-Sleep -Milliseconds 900
    Input-Step 'return to normal look' @('swipe','1080','2250','1080','800','1300')
    $events.Add([pscustomobject]@{seconds=$clock.Elapsed.TotalSeconds;action='input complete'})
} finally {
    $events | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $destination 'live-motion-a-inputs.json')
    if (-not $record.WaitForExit(55000)) {throw 'Screen recording did not finish its bounded duration'}
    & $adb -s $serial pull $remote $local
    & $adb -s $serial shell input keyevent 3
}
