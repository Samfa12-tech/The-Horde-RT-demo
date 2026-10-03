param([string]$EvidenceRoot = $PSScriptRoot)
$ErrorActionPreference = 'Stop'
# Offline projection only. Reuse the earlier phone clock's unchanged 50 ms,
# consumed-frame and bounded-queue checks; select this observed epoch explicitly.
$lines = Get-Content -LiteralPath (Join-Path $EvidenceRoot 'final-scoped-logcat.txt')
$periods = @($lines | ForEach-Object {
    if ($_ -match '^(10-01 [0-9:.]+).*Music progress epoch=4 periods=(\d+) generated=(\d+) accepted=(\d+) deviceHeadObserved=(\d+) capacity=(\d+) underruns=(\d+)') {
        [pscustomobject]@{ time=$Matches[1]; period=[long]$Matches[2]; generated=[long]$Matches[3];
            accepted=[long]$Matches[4]; head=[long]$Matches[5]; capacity=[long]$Matches[6]; underruns=[long]$Matches[7] }
    }
})
$first20 = @($periods | Where-Object period -le 20)
if ($first20.Count -ne 20) { throw 'Missing/duplicate first twenty epoch-4 periods.' }
$seconds = @(for ($index=0; $index -lt 20; ++$index) {
    $row=$first20[$index]
    if ($row.period -ne $index+1 -or $row.head -ne ($index+1)*576000L -or
        $row.underruns -ne 0 -or $row.capacity -ne 5766 -or
        $row.generated -lt $row.accepted -or $row.accepted -lt $row.head -or
        $row.accepted-$row.head -gt $row.capacity -or $row.generated-$row.accepted -gt 480) {
        throw "Clock/order/queue/underrun gate failed at period $($row.period)."
    }
    if ($index -gt 0) {
        $now=[datetime]::ParseExact('2026-'+$row.time,'yyyy-MM-dd HH:mm:ss.fff',[cultureinfo]::InvariantCulture)
        $before=[datetime]::ParseExact('2026-'+$first20[$index-1].time,'yyyy-MM-dd HH:mm:ss.fff',[cultureinfo]::InvariantCulture)
        $delta=($now-$before).TotalSeconds
        if ([math]::Abs($delta-12.0) -gt 0.05) { throw 'Consumed period exceeds existing 50 ms logging bound.' }
        $delta
    }
})
$apk=(Get-Content -LiteralPath (Join-Path $EvidenceRoot 'installed-apk-sha256.txt') -Raw).Trim().Split(' ')[0]
if ($apk -cne 'e10b02034e0a0a78828c6ff885d03b79dd2058f77c6f20346a7c977edd794af4') { throw 'Wrong installed APK.' }
foreach ($name in @('after-motion-ui.xml','post-resume-ui.xml')) {
    [xml]$ui=Get-Content -LiteralPath (Join-Path $EvidenceRoot $name) -Raw
    if ($null -eq $ui.SelectSingleNode('//node[@text="VITALITY  3 / 3"]') -or
        $null -eq $ui.SelectSingleNode('//node[@text="LOWER"]')) { throw "Missing live HUD in $name" }
}
[xml]$settings=Get-Content -LiteralPath (Join-Path $EvidenceRoot 'settings-restored75-ui.xml') -Raw
if ($null -eq $settings.SelectSingleNode('//node[@text="Render resolution  75%"]') -or
    $null -eq $settings.SelectSingleNode('//node[@text="Music volume  70%"]')) { throw 'Normal settings not restored.' }
$latency=Get-Content -LiteralPath (Join-Path $EvidenceRoot 'surface-latency-cleared-live.txt')
$present=@($latency | Select-Object -Skip 1 | ForEach-Object {
    if ($_ -match '^\s*\d+\s+(\d+)\s+\d+\s*$') {
        $value=[long]$Matches[1]
        if ($value -gt 0 -and $value -lt [long]::MaxValue) { $value }
    }
})
if ($present.Count -lt 2) { throw 'No usable actual-present intervals.' }
$milliseconds=@(for($index=1;$index -lt $present.Count;++$index) {
    if($present[$index] -le $present[$index-1]) { throw 'Nonmonotonic actual-present history.' }
    ($present[$index]-$present[$index-1])/1e6
})
$sorted=@($milliseconds | Sort-Object)
$middle=[int][math]::Floor($sorted.Count/2)
$median=if($sorted.Count%2) {$sorted[$middle]} else {($sorted[$middle-1]+$sorted[$middle])/2}
[pscustomobject]@{
    evidenceHead='0ff58606ed6499d73b2f0ccbf035f22f51e99870'; runtimeSource='1da483d01124ca673badf6c13939d99736c4a658'
    exactModel='SM-S948B'; serial='R5GL219SZGK'; process=26202; apkSha256=$apk
    profile='Diagnostic/Mobile'; backend='RayTracingPipeline'; scalePercent=75; musicVolumePercent=70
    liveState='authored finale-roof; benchmark finished; ending Continue; normal inputs; no new cheats'
    music=@{status='PASS-twenty-device-consumed-periods'; epoch=4; periods=20; consumedFrames=11520000;
        intervalCount=19; minimumSeconds=($seconds|Measure-Object -Minimum).Minimum;
        maximumSeconds=($seconds|Measure-Object -Maximum).Maximum; loggingBoundSeconds=0.05; underruns=0;
        beforePausePeriod=28; afterHomeResumePeriods=@($periods|Where-Object period -ge 29|Select-Object -ExpandProperty period)}
    display=@{status='short Debug stationary observation, not Shipping A/B'; timestamps=$present.Count;
        intervals=$milliseconds.Count; medianMilliseconds=$median; p95Milliseconds=$sorted[[int][math]::Ceiling($sorted.Count*.95)-1];
        durationSeconds=($present[-1]-$present[0])/1e9; averageFps=$milliseconds.Count/(($present[-1]-$present[0])/1e9)}
    openGates=@('owner live feel','pickup-to-reveal inputs','matched Shipping display pacing','audio focus loss',
        'full cue-route/SFX balance','sustained active RAM and GPU memory counters','S24 hands/enemies','S25 exact device')
} | ConvertTo-Json -Depth 6
