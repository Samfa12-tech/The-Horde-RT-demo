param([ValidateSet('baseline','candidate')][string]$Member,[Parameter(Mandatory=$true)][string]$ReceiptId)
$ErrorActionPreference='Stop'
$adb='C:/Users/sam_s/AppData/Local/Android/Sdk/platform-tools/adb.exe'
$serial='R5GL219SZGK'; $package='com.samfa12.hordelanternrt.benchmark'
$filename=if($Member -eq 'baseline'){'HordeLanternRT-03c-shipping-mobile-benchmark-arm64.apk'}else{'HordeLanternRT-d63-shipping-mobile-benchmark-arm64.apk'}
$expected=if($Member -eq 'baseline'){'13a8676998c14fed139a14174757a5b1aecb48e50b902834799ab4c970875cf3'}else{'b5a4344b6f860259d88a4d7bae3d349f462704c9f5a689773626f8a93dc39c6f'}
$apk=Join-Path $PSScriptRoot "$Member/$filename"
if((Get-FileHash -LiteralPath $apk).Hash.ToLowerInvariant() -ne $expected){throw 'Immutable APK hash changed'}
if((& $adb -s $serial shell getprop ro.product.model | Out-String).Trim() -ne 'SM-S948B'){throw 'Wrong device'}
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
if($LASTEXITCODE -ne 0 -or (Get-FileHash -LiteralPath $pulled).Hash.ToLowerInvariant() -ne $expected){throw 'Installed APK not byte-identical'}
[ordered]@{member=$Member;package=$package;deviceModel='SM-S948B';serial=$serial;installedApkSha256=$expected;utc=[DateTime]::UtcNow.ToString('o');appDataCleared=$false;stableAppTouched=$false} | ConvertTo-Json > (Join-Path $directory 'receipt.json')
Write-Output "$Member exact APK installed/pulled back identically; benchmark package only."
