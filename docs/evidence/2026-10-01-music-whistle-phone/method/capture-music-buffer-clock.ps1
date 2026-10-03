param(
    [ValidatePattern('^[0-9]+$')][string]$TaskProcess = '17073',
    [ValidateSet('controlled','announced')][string]$TaskRun = 'controlled',
    [ValidateSet('Activity-teardown','Death-overlay')][string]$TaskIncompleteReason = 'Activity-teardown'
)
$ErrorActionPreference = 'Stop'
$taskAdb = 'C:/Users/sam_s/AppData/Local/Android/Sdk/platform-tools/adb.exe'
$taskRoot = 'C:/Dev/tmp/horde-music-instrumentation-20261001/whistle-bank'
$taskLog = Join-Path $taskRoot ("music-buffer-$TaskRun-clock.log")
$taskReceipt = Join-Path $taskRoot ("music-buffer-$TaskRun-clock.json")
if ((Test-Path -LiteralPath $taskLog) -or (Test-Path -LiteralPath $taskReceipt)) { throw 'Completed clock capture exists; do not repeat.' }
if ((& $taskAdb -s R5GL219SZGK shell getprop ro.product.model).Trim() -cne 'SM-S948B') { throw 'Wrong physical device.' }
$taskPid = (& $taskAdb -s R5GL219SZGK shell pidof com.samfa12.hordelanternrt.debug).Trim()
if ($taskPid -cne $TaskProcess) { throw 'Controlled run process changed; retain interruption rather than restart.' }
$taskLines = @(& $taskAdb -s R5GL219SZGK logcat -d --pid=$taskPid -v threadtime HordeLanternMusic:I '*:S')
if ($LASTEXITCODE -ne 0) { throw 'Scoped logcat capture failed.' }
$taskPeriods = @()
foreach ($taskLine in $taskLines) {
    if ($taskLine -match '^(\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3}).*Music progress epoch=(\d+) periods=(\d+) generated=(\d+) accepted=(\d+) deviceHeadObserved=(\d+) capacity=(\d+) underruns=(\d+)') {
        $taskPeriods += [pscustomobject]@{ time = $Matches[1]; epoch=[long]$Matches[2]; period=[long]$Matches[3];
            generated=[long]$Matches[4]; accepted=[long]$Matches[5]; head=[long]$Matches[6]; capacity=[long]$Matches[7]; underruns=[long]$Matches[8] }
    }
}
if ($taskPeriods.Count -lt 20) {
    $taskUiHash = $null
    if ($TaskIncompleteReason -eq 'Death-overlay') {
        $taskUiPath = Join-Path $taskRoot 'music-buffer-announced-death.xml'
        [xml]$taskUi = Get-Content -Raw -LiteralPath $taskUiPath
        if ($null -eq $taskUi.SelectSingleNode('//node[@package="com.samfa12.hordelanternrt.debug" and @text="YOU FELL"]')) {
            throw 'Death-overlay classification requires the actual app UI evidence.'
        }
        $taskUiHash = (Get-FileHash -LiteralPath $taskUiPath).Hash.ToLowerInvariant()
    }
    $taskLines | Out-File -LiteralPath $taskLog -Encoding utf8
    [pscustomobject]@{ status="INCOMPLETE-$TaskIncompleteReason-before-twenty-periods"; exactModel='SM-S948B'; serial='R5GL219SZGK'; process=[int]$taskPid;
        apkSha256='35ec7e46b12a55474ad66f11509a1aa4566e2f25d86d6c34992a4a5ee18058d6'; requiredPeriods=20; observedPeriods=$taskPeriods.Count;
        stopObserved=(@($taskLines | Where-Object {$_ -match 'Music stop requested;'}).Count -gt 0); rows=$taskPeriods;
        ownerReported='Music sounds perfect now'; twentyPeriodAcceptance=$false; lifecycleAcceptance=$false;
        nextUnfinishedStep='Twenty uninterrupted periods require a legitimate safe unpaused live state; the idle opening reaches death and suspends music. Do not repeat that invalid setup or completed renders/tests. Affected pause/Home/resume and audio focus remain separate.';
        deathOverlayUiSha256=$taskUiHash;
        logSha256=(Get-FileHash -LiteralPath $taskLog).Hash.ToLowerInvariant() } |
        ConvertTo-Json -Depth 6 | Out-File -LiteralPath $taskReceipt -Encoding utf8
    Get-Content -Raw -LiteralPath $taskReceipt
    exit 2
}
$taskIntervals = @()
for ($taskIndex=0; $taskIndex -lt 20; ++$taskIndex) {
    $taskPeriod = $taskPeriods[$taskIndex]
    if ($taskPeriod.period -ne $taskIndex+1 -or $taskPeriod.epoch -ne 1 -or $taskPeriod.underruns -ne 0 -or
        $taskPeriod.capacity -ne 5766 -or $taskPeriod.head -ne ($taskIndex+1)*576000 -or
        $taskPeriod.generated -lt $taskPeriod.accepted -or $taskPeriod.accepted -lt $taskPeriod.head -or
        $taskPeriod.accepted-$taskPeriod.head -gt $taskPeriod.capacity -or
        $taskPeriod.generated-$taskPeriod.accepted -gt 480) { throw "Clock/order/queue/underrun gate failed at period $($taskPeriod.period)." }
    if ($taskIndex -gt 0) {
        $taskNow=[datetime]::ParseExact('2026-'+$taskPeriod.time,'yyyy-MM-dd HH:mm:ss.fff',[Globalization.CultureInfo]::InvariantCulture)
        $taskBefore=[datetime]::ParseExact('2026-'+$taskPeriods[$taskIndex-1].time,'yyyy-MM-dd HH:mm:ss.fff',[Globalization.CultureInfo]::InvariantCulture)
        $taskSeconds=($taskNow-$taskBefore).TotalSeconds
        if ([math]::Abs($taskSeconds-12.0) -gt 0.05) { throw "Consumed period differs by more than50ms: $taskSeconds seconds." }
        $taskIntervals += $taskSeconds
    }
}
$taskPrefsText = (& $taskAdb -s R5GL219SZGK shell run-as com.samfa12.hordelanternrt.debug cat shared_prefs/horde_lantern_alpha_settings.xml) -join "`n"
if ($LASTEXITCODE -ne 0) { throw 'Could not read scoped test settings.' }
[xml]$taskPrefs=$taskPrefsText
$taskRender=$taskPrefs.SelectSingleNode('/map/int[@name="render_scale"]')
$taskMusic=$taskPrefs.SelectSingleNode('/map/int[@name="music_volume"]')
$taskRenderValue=if ($null -eq $taskRender) {75} else {[int]$taskRender.value}
$taskMusicValue=if ($null -eq $taskMusic) {70} else {[int]$taskMusic.value}
if ($taskRenderValue -ne 75 -or $taskMusicValue -ne 70) {throw 'Controlled saved settings changed; report actual values instead of nominal conditions.'}
$taskLines | Out-File -LiteralPath $taskLog -Encoding utf8
[pscustomobject]@{
    status='PASS-twenty-device-consumed-periods'; exactModel='SM-S948B'; serial='R5GL219SZGK'; process=[int]$taskPid
    apkSha256='35ec7e46b12a55474ad66f11509a1aa4566e2f25d86d6c34992a4a5ee18058d6'
    renderScalePercent=$taskRenderValue; musicVolumePercent=$taskMusicValue; checkpointFrozen=$false; benchmark=$false
    effectiveFrames=5766; minimumBytes=46128; sampleRate=48000; periods=20; consumedFrames=11520000; underruns=0
    measuredIntervalCount=$taskIntervals.Count; minimumSeconds=($taskIntervals|Measure-Object -Minimum).Minimum
    maximumSeconds=($taskIntervals|Measure-Object -Maximum).Maximum; averageSeconds=($taskIntervals|Measure-Object -Average).Average
    loggingBoundSeconds=0.05; generatedAcceptedHeadOrder='passed'; partialWriteAndEffectiveQueueBounds='passed'
    sourceHead='97f07e2a5791346f2b4563221909a87ab5948086'; javaOnlyBufferPatch=$true
    ownerReported='Music sounds perfect now'; audibleRouteAndSfxMaskingNotCertified=$true
    logSha256=(Get-FileHash -LiteralPath $taskLog).Hash.ToLowerInvariant(); rows=@($taskPeriods|Select-Object -First 20)
} | ConvertTo-Json -Depth 6 | Out-File -LiteralPath $taskReceipt -Encoding utf8
Get-Content -Raw -LiteralPath $taskReceipt
