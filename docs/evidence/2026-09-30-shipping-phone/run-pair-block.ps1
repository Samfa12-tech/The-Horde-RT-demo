param([ValidateSet('baseline','candidate')][string]$Member,[ValidateSet('a1','b1','b2','a2')][string]$Block,[string[]]$Cases=@('route','high','live'))
$ErrorActionPreference='Stop'
$source=if($Member -eq 'baseline'){'d504bfed51976e71e496ead81dbd284bec03da33'}else{'8cbe0ac44d4fb3a168ca891985ca560b43fe4541'}
$sha=if($Member -eq 'baseline'){'13a8676998c14fed139a14174757a5b1aecb48e50b902834799ab4c970875cf3'}else{'b5a4344b6f860259d88a4d7bae3d349f462704c9f5a689773626f8a93dc39c6f'}
$workloads=@{route='showcase-route-v1';high='lantern-held-high-v1';live='lantern-reveal-sequence-v1'}
foreach($case in $Cases){
    if(-not $workloads.ContainsKey($case)){throw "Unknown bounded workload: $case"}
    $id="shipping-$Block-$case-20260930"
    & (Join-Path $PSScriptRoot 'run-shipping-trial.ps1') -RunId $id -BuildLabel $Member -SourceCommit $source -InstalledApkSha256 $sha -Workload $workloads[$case] *> (Join-Path $PSScriptRoot "$Member/$Block-$case.log")
    & (Join-Path $PSScriptRoot 'analyse-trial.ps1') -RunId $id
}
