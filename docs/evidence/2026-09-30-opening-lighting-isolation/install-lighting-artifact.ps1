param([Parameter(Mandatory=$true)][ValidateSet('control','shadow','volume')][string]$Artifact,[Parameter(Mandatory=$true)][string]$ReceiptId)
$ErrorActionPreference='Stop'
$adb='C:/Users/sam_s/AppData/Local/Android/Sdk/platform-tools/adb.exe'
$serial='R5GL219SZGK';$package='com.samfa12.hordelanternrt.benchmark'
$choice=switch -Exact($Artifact){
    'control' {@{path='active-strategy/HordeLanternRT-eaf-active-strategy-benchmark-arm64.apk';sha='ab3e2261fd081f87e667a6e4967e2476077fa96554702330e6aa49baa8133eae'}}
    'shadow' {@{path='opening-lighting-shadow/HordeLanternRT-nonshipping-fire-shadow-isolation-arm64.apk';sha='23aa0cf67f5f0612290f8607078d47e7e47bdf81202d9290ab15cc796dd73fea'}}
    'volume' {@{path='opening-lighting-volume/HordeLanternRT-nonshipping-fire-volume-isolation-arm64.apk';sha='e063e7cfaef964f8b023cb318f86620597f3b4aeaaab40ad7899bec951e7c749'}}
}
$apk=Join-Path $PSScriptRoot $choice.path
if((Get-FileHash $apk).Hash.ToLowerInvariant() -cne $choice.sha){throw 'Immutable lighting artifact changed'}
if((& $adb -s $serial shell getprop ro.product.model | Out-String).Trim() -cne 'SM-S948B'){throw 'Wrong device'}
if($ReceiptId -notmatch '^[a-zA-Z0-9_-]+$'){throw 'Bad receipt id'}
$destination=Join-Path $PSScriptRoot "installs/$ReceiptId"
if(Test-Path $destination){throw 'Never overwrite installation evidence'}
New-Item -ItemType Directory -Path $destination | Out-Null
& $adb -s $serial install -r -t $apk *> (Join-Path $destination 'install.log')
if($LASTEXITCODE -ne 0){throw 'Installation failed'}
$remote=((& $adb -s $serial shell pm path $package | Out-String).Trim() -replace '^package:','')
if($remote -notmatch '^/data/app/[^\r\n]+/base\.apk$'){throw 'Unexpected APK path'}
$pulled=Join-Path $destination 'installed-base.apk'
& $adb -s $serial pull $remote $pulled *> (Join-Path $destination 'pull.log')
if($LASTEXITCODE -ne 0 -or (Get-FileHash $pulled).Hash.ToLowerInvariant() -cne $choice.sha){throw 'APK pullback mismatch'}
[ordered]@{artifact=$Artifact;package=$package;deviceModel='SM-S948B';serial=$serial;installedApkSha256=$choice.sha;utc=[DateTime]::UtcNow.ToString('o');nonphysicalWorkloadIsolation=($Artifact -cne 'control');publishable=$false;appDataCleared=$false;stableAppTouched=$false} | ConvertTo-Json | Out-File (Join-Path $destination 'receipt.json') -Encoding utf8
Write-Output "$Artifact exact APK installed/pulled back identically; investigation benchmark only."
