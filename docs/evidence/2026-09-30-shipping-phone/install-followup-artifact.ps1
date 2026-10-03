param([ValidateSet('active-strategy-export','opening-stage-probe')][string]$Artifact,[Parameter(Mandatory=$true)][string]$ReceiptId)
$ErrorActionPreference='Stop'
$adb='C:/Users/sam_s/AppData/Local/Android/Sdk/platform-tools/adb.exe'
$serial='R5GL219SZGK'; $package='com.samfa12.hordelanternrt.benchmark'
$choice=switch -Exact ($Artifact){
    'active-strategy-export' {@{path='active-strategy/HordeLanternRT-eaf-active-strategy-benchmark-arm64.apk';sha='ab3e2261fd081f87e667a6e4967e2476077fa96554702330e6aa49baa8133eae'}}
    'opening-stage-probe' {@{path='opening-stage-proposal/HordeLanternRT-186-opening-gpu-stages-v3-benchmark-arm64.apk';sha='cbfc5cf40d15b8a7c64561d4eff11a2dbf0a9dd9c7179da9a53850ef74905204'}}
}
$apk=Join-Path $PSScriptRoot $choice.path
if((Get-FileHash -LiteralPath $apk).Hash.ToLowerInvariant() -cne $choice.sha){throw 'Immutable follow-up APK hash changed'}
if((& $adb -s $serial shell getprop ro.product.model | Out-String).Trim() -cne 'SM-S948B'){throw 'Wrong device'}
if($ReceiptId -notmatch '^[a-zA-Z0-9_-]+$'){throw 'Bad receipt id'}
$directory=Join-Path $PSScriptRoot "installs/$ReceiptId"
if(Test-Path -LiteralPath $directory){throw 'Do not overwrite install evidence'}
New-Item -ItemType Directory -Path $directory | Out-Null
& $adb -s $serial install -r -t $apk *> (Join-Path $directory 'install.log')
if($LASTEXITCODE -ne 0){throw 'Installation failed'}
$remote=((& $adb -s $serial shell pm path $package | Out-String).Trim() -replace '^package:','')
if($remote -notmatch '^/data/app/[^\r\n]+/base\.apk$'){throw 'Unexpected exact benchmark APK path'}
$pulled=Join-Path $directory 'installed-base.apk'
& $adb -s $serial pull $remote $pulled *> (Join-Path $directory 'pull.log')
if($LASTEXITCODE -ne 0 -or (Get-FileHash -LiteralPath $pulled).Hash.ToLowerInvariant() -cne $choice.sha){throw 'Installed APK not byte-identical'}
[ordered]@{artifact=$Artifact;package=$package;deviceModel='SM-S948B';serial=$serial;installedApkSha256=$choice.sha;utc=[DateTime]::UtcNow.ToString('o');appDataCleared=$false;stableAppTouched=$false} | ConvertTo-Json > (Join-Path $directory 'receipt.json')
Write-Output "$Artifact exact APK installed/pulled back identically; benchmark package only."
