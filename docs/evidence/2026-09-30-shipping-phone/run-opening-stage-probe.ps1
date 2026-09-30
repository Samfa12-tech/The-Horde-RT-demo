$ErrorActionPreference='Stop'
$id='shipping-opening-gpu-stages-v3-route-20260930'
$probeSha='cbfc5cf40d15b8a7c64561d4eff11a2dbf0a9dd9c7179da9a53850ef74905204'
$probeRoot=Join-Path $PSScriptRoot 'opening-stage-proposal'
$log=Join-Path $probeRoot 'phone-stage-logcat-v3.txt'
if(Test-Path -LiteralPath $log){throw 'Do not overwrite probe log'}
& (Join-Path $PSScriptRoot 'install-followup-artifact.ps1') -Artifact opening-stage-probe -ReceiptId opening-gpu-stage-v3-20260930
$adb='C:/Users/sam_s/AppData/Local/Android/Sdk/platform-tools/adb.exe'
$observer=Start-Process -FilePath $adb -ArgumentList @('-s','R5GL219SZGK','logcat','-T','1','-v','brief','HordeRtProbeBridge:I','*:S') -WindowStyle Hidden -RedirectStandardOutput $log -RedirectStandardError (Join-Path $probeRoot 'phone-stage-logcat-v3.stderr.txt') -PassThru
try {
    & (Join-Path $PSScriptRoot 'run-shipping-trial.ps1') -RunId $id -BuildLabel opening-stage-probe -SourceCommit 18616f4fdab2da24e822c999006f39f6cff258eb -InstalledApkSha256 $probeSha -Workload showcase-route-v1 *> (Join-Path $probeRoot 'phone-trial-v3.log')
    & (Join-Path $PSScriptRoot 'analyse-trial.ps1') -RunId $id
} finally {
    # Stop only this read-only host log observer, never the Android application.
    if(-not $observer.HasExited){Stop-Process -Id $observer.Id}
}
