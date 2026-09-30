param([Parameter(Mandatory=$true)][string]$RunSuffix)
$ErrorActionPreference='Stop'
if($RunSuffix -notmatch '^[a-zA-Z0-9_-]+$'){throw 'Invalid unique suffix'}
$build=Get-Content (Join-Path $PSScriptRoot 'build-receipt.json') -Raw | ConvertFrom-Json
$source='71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95'
$workloads=[ordered]@{route='showcase-route-v1';high='lantern-held-high-v1';live='lantern-reveal-sequence-v1'}
foreach($block in @('c1','s1','s2','c2')){
    $member=if($block.StartsWith('c')){'control'}else{'isolate'}
    # S2 deliberately continues the same isolate installation/process after S1.
    if($block -cne 's2'){
        & (Join-Path $PSScriptRoot 'install-member.ps1') -Member $member -ReceiptId "all-shadow-$block-$RunSuffix" *> (Join-Path $PSScriptRoot "install-$block-$RunSuffix.log")
    }
    foreach($case in $workloads.Keys){
        $id="all-shadow-$block-$case-$RunSuffix"
        & (Join-Path $PSScriptRoot 'run-shipping-trial.ps1') -RunId $id -BuildLabel "all-shadow-$member" -SourceCommit $source -InstalledApkSha256 $build.$member.apkSha256 -Workload $workloads[$case] *> (Join-Path $PSScriptRoot "$id.log")
        Write-Output "$id report retained; analysis/thermal admission remains separate."
    }
}
# Only the authorised benchmark package; do not disturb stable/user app data.
& 'C:/Users/sam_s/AppData/Local/Android/Sdk/platform-tools/adb.exe' -s R5GL219SZGK shell input keyevent KEYCODE_HOME *> (Join-Path $PSScriptRoot "home-$RunSuffix.log")
Write-Output 'ABBA complete; physical normal control retained, phone Home.'
