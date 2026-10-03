param([Parameter(Mandatory=$true)][ValidateSet('s26','s24')][string]$Device)
$ErrorActionPreference='Stop'
$root='C:/Dev/tmp/horde-mobile-lantern-profile-20261001'
$phoneRoot=Join-Path $root 'phone'
$serial=if($Device -ceq 's26'){'R5GL219SZGK'}else{'R5CXC0G9GBW'}
$model=if($Device -ceq 's26'){'SM-S948B'}else{'SM-S928B'}
$backend=if($Device -ceq 's26'){'RayTracingPipeline'}else{'RayQueryCompute'}
$control=[ordered]@{source='9f4f439e40a9477efe0bac42ee41b74de64b3b1b';
    apk=Join-Path $root 'control-complete-assets-shipping-mobile.apk';
    sha='3a4faf72923e24da9da859ef6cd71c221b818b97d7418a75b1c8dd0ca0540378';member='physical-control'}
$candidate=[ordered]@{source='1da483d01124ca673badf6c13939d99736c4a658';
    apk=Join-Path $root 'candidate-1da483d-shipping-mobile.apk';
    sha='bac94c45bcfe3f8fcc367f21439735387d188959005b5ee873d02f0f1e9cf142';member='open-aperture-candidate'}
$workloads=[ordered]@{route='showcase-route-v1';high='lantern-held-high-v1';live='lantern-reveal-sequence-v1'}
try {
    foreach($block in @('c1','p1','p2','c2')) {
        $choice=if($block.StartsWith('c')){$control}else{$candidate}
        foreach($case in $workloads.Keys) {
            $id="mobile-pane-$Device-$block-$case-20261001"
            # The first S26 route was replaced only after proving its APK had
            # unhydrated LFS pointers. Retain that failure; this is its valid row.
            if($Device -ceq 's26' -and $block -ceq 'c1' -and $case -ceq 'route'){
                $id='mobile-pane-s26-c1-route-valid-20261001'
            }
            $trialPath=Join-Path $phoneRoot "$id/trial.json"
            if(Test-Path -LiteralPath $trialPath) {
                # Already-running C1 collectors own these rows. Observe them,
                # never relaunch or repeat a completed trial after resumption.
                $deadline=[DateTime]::UtcNow.AddMinutes(15)
                do {
                    $trial=Get-Content -LiteralPath $trialPath -Raw | ConvertFrom-Json
                    if($trial.status -ceq 'complete'){break}
                    if($trial.status -ceq 'failed' -or [DateTime]::UtcNow -gt $deadline){throw "Inspect existing unfinished $id; do not restart"}
                    Start-Sleep -Seconds 5
                } while($true)
                if($trial.sourceCommit -cne $choice.source -or $trial.installedApkSha256 -cne $choice.sha -or
                   $trial.member -cne $choice.member -or $trial.serial -cne $serial -or
                   $trial.requestedBackend -cne $backend -or $trial.workload -cne $workloads[$case]){throw "Existing identity mismatch $id"}
                $reportPath=Join-Path $phoneRoot "$id/$id/benchmark.json"
                $report=Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json
                if($report.status -cne 'complete' -or $report.runId -cne $id -or -not $report.presentedEveryFrame){throw "Existing report mismatch $id"}
                Write-Output "$id already complete; preserved without rerun."
                continue
            }
            $install=($case -ceq 'route' -and $block -cin @('p1','c2'))
            & (Join-Path $PSScriptRoot 'run-trial.ps1') -RunId $id -SourceCommit $choice.source -ApkPath $choice.apk -ApkSha256 $choice.sha -DeviceSerial $serial -DeviceModel $model -Backend $backend -Member $choice.member -Workload $workloads[$case] -OutputRoot $phoneRoot -Install:$install *> (Join-Path $root "$id.log")
            Write-Output "$id completed; row/thermal admission remains separate."
        }
    }
    Write-Output "$Device finite C1/P1/P2/C2 matrix retained. No automatic playability acceptance."
} finally {
    # Stop test presentation and return Home; leave app data/stable app untouched.
    & 'C:/Users/sam_s/AppData/Local/Android/Sdk/platform-tools/adb.exe' -s $serial shell input keyevent KEYCODE_HOME *> (Join-Path $root "$Device-matrix-home.log")
}
