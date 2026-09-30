param(
    [Parameter(Mandatory=$true)][ValidateSet('control','opaque-retained-profile')][string]$Artifact,
    [Parameter(Mandatory=$true)][string]$ReceiptId
)
$ErrorActionPreference='Stop'
$adb='C:/Users/sam_s/AppData/Local/Android/Sdk/platform-tools/adb.exe'
$serial='R5GL219SZGK';$package='com.samfa12.hordelanternrt.benchmark'
$choice=switch -Exact($Artifact){
    'control' {@{path='C:/Dev/tmp/horde-shipping-ab-20260930/active-strategy/HordeLanternRT-eaf-active-strategy-benchmark-arm64.apk';sha='ab3e2261fd081f87e667a6e4967e2476077fa96554702330e6aa49baa8133eae'}}
    'opaque-retained-profile' {@{path='C:/Dev/tmp/horde-opaque-retained-profile-20260930/artifacts/benchmark/HordeLanternRT-opaque-retained-investigation-shipping-mobile-arm64.apk';sha='4827a3c2e26d3328ed53608e15c77afe12a34077e96dcd0564c8d65532e21ff4'}}
}
if((Get-FileHash -LiteralPath $choice.path).Hash.ToLowerInvariant() -cne $choice.sha){throw 'Immutable trial APK changed'}
if((& $adb -s $serial shell getprop ro.product.model | Out-String).Trim() -cne 'SM-S948B'){throw 'Wrong physical device'}
if($ReceiptId -notmatch '^[a-zA-Z0-9_-]+$'){throw 'Invalid unique receipt id'}
$destination=Join-Path $PSScriptRoot "installs/$ReceiptId"
if(Test-Path -LiteralPath $destination){throw 'Never overwrite installation evidence'}
New-Item -ItemType Directory -Path $destination | Out-Null
& $adb -s $serial install -r -t $choice.path *> (Join-Path $destination 'install.log')
if($LASTEXITCODE){throw 'Installation failed'}
$remote=((& $adb -s $serial shell pm path $package | Out-String).Trim() -replace '^package:','')
if($remote -notmatch '^/data/app/[^\r\n]+/base\.apk$'){throw 'Unexpected installed APK path'}
$pulled=Join-Path $destination 'installed-base.apk'
& $adb -s $serial pull $remote $pulled *> (Join-Path $destination 'pull.log')
if($LASTEXITCODE -or (Get-FileHash -LiteralPath $pulled).Hash.ToLowerInvariant() -cne $choice.sha){throw 'Exact APK pullback mismatch'}
[ordered]@{artifact=$Artifact;package=$package;deviceModel='SM-S948B';serial=$serial;
    installedApkSha256=$choice.sha;utc=[DateTime]::UtcNow.ToString('o');
    externallyCooled=$false;appDataCleared=$false;stableAppTouched=$false;publishable=$false
} | ConvertTo-Json | Out-File -LiteralPath (Join-Path $destination 'receipt.json') -Encoding utf8
Write-Output "$Artifact exact APK installed/pulled back identically; no data clear or stable-app mutation."
