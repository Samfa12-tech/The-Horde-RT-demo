param([Parameter(Mandatory=$true)][string]$RunSuffix)
$ErrorActionPreference='Stop'
if($RunSuffix -notmatch '^[a-zA-Z0-9_-]+$'){throw 'Invalid unique suffix'}
$build=Get-Content (Join-Path $PSScriptRoot 'build-receipt.json') -Raw | ConvertFrom-Json
$source='71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95'
$workloads=[ordered]@{route='showcase-route-v1';high='lantern-held-high-v1';live='lantern-reveal-sequence-v1'}
$controlRetained=$false
try {
    foreach($block in @('c1','p1','p2','c2')){
        $member=if($block.StartsWith('c')){'control'}else{'reference'}
        # P2 deliberately continues the same reference installation/process after P1.
        if($block -cne 'p2'){
            $controlRetained=$false
            & (Join-Path $PSScriptRoot 'install-member.ps1') -Member $member -ReceiptId "primary-hit-$block-$RunSuffix" *> (Join-Path $PSScriptRoot "install-$block-$RunSuffix.log")
            $controlRetained=($member -ceq 'control')
        }
        foreach($case in $workloads.Keys){
            $id="primary-hit-$block-$case-$RunSuffix"
            & (Join-Path $PSScriptRoot 'run-shipping-trial.ps1') -RunId $id -BuildLabel "primary-hit-$member" -SourceCommit $source -InstalledApkSha256 $build.$member.apkSha256 -Workload $workloads[$case] *> (Join-Path $PSScriptRoot "$id.log")
            Write-Output "$id report retained; analysis/thermal admission remains separate."
        }
    }
    Write-Output 'ABBA reports complete; physical normal control retained.'
} finally {
    # Restore normal rendering even if the reference trial fails; never reset data.
    if(-not $controlRetained){
        & (Join-Path $PSScriptRoot 'install-member.ps1') -Member control -ReceiptId "primary-hit-restore-$RunSuffix" *> (Join-Path $PSScriptRoot "install-restore-$RunSuffix.log")
    }
    & 'C:/Users/sam_s/AppData/Local/Android/Sdk/platform-tools/adb.exe' -s R5GL219SZGK shell input keyevent KEYCODE_HOME *> (Join-Path $PSScriptRoot "home-$RunSuffix.log")
    if($LASTEXITCODE -ne 0){throw 'Cannot confirm Home return; inspect device before further work'}
}
