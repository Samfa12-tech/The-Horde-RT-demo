[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$ArchiveRoot)
$ErrorActionPreference='Stop'
$archive=(Resolve-Path -LiteralPath $ArchiveRoot -ErrorAction Stop).Path
$profile='C:\Dev\tmp\horde-opaque-secondary-profile-20261001'
$expectedArchive='C:\Dev\tmp\horde-opaque-secondary-profile-20261001\curated-v3'
if($archive -ine $expectedArchive){throw "Unexpected source packet: $archive"}
$fixture=Join-Path $profile 'classification-negative-fixture-v2'
$receiptPath=Join-Path $profile 'classification-negative-test-receipt-v2.json'
if(Test-Path -LiteralPath $fixture){throw "Refusing preexisting negative fixture: $fixture"}
if(Test-Path -LiteralPath $receiptPath){throw "Refusing overwrite of negative receipt: $receiptPath"}
$manifest=Get-Content -LiteralPath (Join-Path $archive 'manifest.json') -Raw|ConvertFrom-Json
foreach($entry in $manifest.files){
    $relative=[string]$entry.path.Replace('/','\')
    $source=Join-Path $archive $relative
    $destination=Join-Path $fixture $relative
    $parent=Split-Path -Parent $destination
    if(-not(Test-Path -LiteralPath $parent)){New-Item -ItemType Directory -Path $parent -Force|Out-Null}
    if($relative -in @('build-receipt.json','manifest.json')){Copy-Item -LiteralPath $source -Destination $destination}
    else{New-Item -ItemType HardLink -Path $destination -Target $source|Out-Null}
}
$buildPath=Join-Path $fixture 'build-receipt.json'
$build=Get-Content -LiteralPath $buildPath -Raw|ConvertFrom-Json
$build.classification='physical-production-accepted'
$build|ConvertTo-Json -Depth 20|Set-Content -LiteralPath $buildPath -Encoding utf8
$entry=@($manifest.files|Where-Object path -CEQ 'build-receipt.json')
if($entry.Count -ne 1){throw 'Archive manifest lacks unique root build receipt'}
$entry[0].bytes=(Get-Item -LiteralPath $buildPath).Length
$entry[0].sha256=(Get-FileHash -LiteralPath $buildPath -Algorithm SHA256).Hash.ToLowerInvariant()
$manifest|ConvertTo-Json -Depth 20|Set-Content -LiteralPath (Join-Path $fixture 'manifest.json') -Encoding utf8
$message=''
try{
    & (Join-Path $archive 'verify-curated-opaque-secondary.ps1') -Root $fixture 2>&1|Out-String|ForEach-Object {$message=$_}
    throw 'Verifier unexpectedly accepted the physical/production classification fixture'
}catch{
    if($_.Exception.Message -like 'Verifier unexpectedly accepted*'){throw}
    $message=$_.Exception.Message
}
if($message -notmatch "build classification expected 'nonphysical-opaque-primary-secondary-omission-investigation-only', found 'physical-production-accepted'"){
    throw "Verifier failed for an unexpected reason: $message"
}
[ordered]@{
    schema=1
    classification='synthetic-negative-fixture-only'
    result='PASS: exact verifier rejects physical/production classification'
    sourceArchive=$archive
    fixturePath=$fixture
    fixtureUsesHardlinks=$true
    manifestAndBuildReceiptCopied=$true
    alteredReceipt='build-receipt.json'
    alteredClassification='physical-production-accepted'
    expectedDiagnostic=$message.Trim()
    originalArchiveModified=$false
}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $receiptPath -Encoding utf8
Write-Output "PASS: physical/production label rejected; hardlink fixture retained at $fixture"
