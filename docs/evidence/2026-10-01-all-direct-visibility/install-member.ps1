param([Parameter(Mandatory=$true)][ValidateSet('control','isolate')][string]$Member,
      [Parameter(Mandatory=$true)][string]$ReceiptId)
$ErrorActionPreference='Stop'
$adb='C:/Users/sam_s/AppData/Local/Android/Sdk/platform-tools/adb.exe'
$serial='R5GL219SZGK'
$package='com.samfa12.hordelanternrt.benchmark'
$build=Get-Content (Join-Path $PSScriptRoot 'build-receipt.json') -Raw | ConvertFrom-Json
$choice=$build.$Member
$expected=if($Member -ceq 'control'){'a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033'}else{'b57abb9127929665102a9de70b44085d6c469aeeee10fb016f09d6ffbd6d4829'}
if($choice.apkSha256 -cne $expected -or $choice.package -cne $package -or
   (Get-FileHash -LiteralPath $choice.apkPath).Hash.ToLowerInvariant() -cne $expected){throw 'Immutable member changed'}
if(($build.worktree.head) -cne '71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95'){throw 'Wrong paired source'}
if((& $adb -s $serial shell getprop ro.product.model | Out-String).Trim() -cne 'SM-S948B'){throw 'Wrong device'}
if($ReceiptId -notmatch '^[a-zA-Z0-9_-]+$'){throw 'Bad receipt id'}
$destination=Join-Path $PSScriptRoot "installs/$ReceiptId"
if(Test-Path -LiteralPath $destination){throw 'Never overwrite installation evidence'}
New-Item -ItemType Directory -Path $destination | Out-Null
& $adb -s $serial install -r -t $choice.apkPath *> (Join-Path $destination 'install.log')
if($LASTEXITCODE -ne 0){throw 'Installation failed'}
$remote=((& $adb -s $serial shell pm path $package | Out-String).Trim() -replace '^package:','')
if($remote -notmatch '^/data/app/[^\r\n]+/base\.apk$'){throw 'Unexpected APK path'}
$pulled=Join-Path $destination 'installed-base.apk'
& $adb -s $serial pull $remote $pulled *> (Join-Path $destination 'pull.log')
if($LASTEXITCODE -ne 0 -or (Get-FileHash -LiteralPath $pulled).Hash.ToLowerInvariant() -cne $expected){throw 'APK pullback mismatch'}
[ordered]@{member=$Member;package=$package;deviceModel='SM-S948B';serial=$serial;
    installedApkSha256=$expected;sourceCommit=$build.worktree.head;utc=[DateTime]::UtcNow.ToString('o');
    nonphysicalWorkloadIsolation=($Member -cne 'control');publishable=$false;
    appDataCleared=$false;stableAppTouched=$false} | ConvertTo-Json > (Join-Path $destination 'receipt.json')
Write-Output "$Member exact APK installed/pulled back identically; investigation only."
