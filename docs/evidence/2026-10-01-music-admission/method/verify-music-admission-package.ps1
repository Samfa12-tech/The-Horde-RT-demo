$ErrorActionPreference = 'Stop'
$hordeRepo = 'C:/Users/sam_s/Documents/the Horde RT Demo/.worktrees/horde-1.6.1-engineering-pass'
. (Join-Path $hordeRepo 'tools/music-asset-policy.ps1')
$hordeApk = & (Join-Path $hordeRepo 'tools/resolve-android-apk.ps1') -AndroidRoot (Join-Path $hordeRepo 'android') -Variant debug
$null = Assert-HordeMusicPackage -RepositoryRoot $hordeRepo -ArchivePath $hordeApk
$hordeBaseline = 'C:/Dev/tmp/horde-fast-resize-20261001/HordeLanternRT-fast-resize-final-debug-arm64.apk'
if ((Get-FileHash -LiteralPath $hordeBaseline).Hash.ToLowerInvariant() -cne 'c11ff703794d74dbfb75dc7a0186af0f6c5c416e8383755a5cd8f84310daf0fe') { throw 'Baseline APK identity differs' }
Add-Type -AssemblyName System.IO.Compression.FileSystem
$hordeOldZip = [IO.Compression.ZipFile]::OpenRead($hordeBaseline)
$hordeNewZip = [IO.Compression.ZipFile]::OpenRead($hordeApk)
try {
    $hordeCount = 0
    foreach ($hordeOldEntry in $hordeOldZip.Entries) {
        if (-not $hordeOldEntry.FullName.StartsWith('assets/') -or $hordeOldEntry.FullName -ceq 'assets/ASSET_LICENSES.md') { continue }
        $hordeNewEntries = @($hordeNewZip.Entries | Where-Object FullName -CEQ $hordeOldEntry.FullName)
        if ($hordeNewEntries.Count -ne 1 -or $hordeNewEntries[0].Length -ne $hordeOldEntry.Length -or
            (Get-HordeMusicZipEntrySha256 -Entry $hordeNewEntries[0]) -cne (Get-HordeMusicZipEntrySha256 -Entry $hordeOldEntry)) { throw "Existing asset differs: $($hordeOldEntry.FullName)" }
        $hordeCount++
    }
    # The old APK's 53 assets include ASSET_LICENSES.md. The other 52 must
    # remain byte-exact; the attribution file intentionally gains music admission.
    if ($hordeCount -ne 52) { throw "Expected 52 unchanged render/SFX assets, got $hordeCount" }
    $hordeLicenceEntry = $hordeNewZip.GetEntry('assets/ASSET_LICENSES.md')
    if ((Get-HordeMusicZipEntrySha256 -Entry $hordeLicenceEntry) -cne (Get-FileHash -LiteralPath (Join-Path $hordeRepo 'ASSET_LICENSES.md')).Hash.ToLowerInvariant()) { throw 'Packaged licence is stale' }
} finally { $hordeOldZip.Dispose(); $hordeNewZip.Dispose() }
$hordeContainment = Get-Content 'C:/Dev/tmp/horde-pocket-audio-core-native-20261001/asset-admission-actual-module-validation.log' -Raw | ConvertFrom-Json
$hordeExpectedModules = @('c1e4622ae5df0c87a06c36d803617c7d4e0227845cd33a9dd38de5f0d956ff58', 'e18171591d4b07ccc3e7fb071400a6581e1fc132aec06dd37c09f88c7792e302', '8dbebd86eeed23b388626719e6f9946b8027df754869b5811673e24036ae12c9', '9865690c11de315c1c4e49af7d14eb8963b24dc522abfc01c53979bb1a5f5a91')
if ($hordeContainment.packaged.spirvVal -cne 'passed' -or $hordeContainment.packaged.spirvDis -cne 'passed' -or $hordeContainment.packaged.modules.Count -ne 4) { throw 'Actual module validation incomplete' }
foreach ($hordeModule in $hordeContainment.packaged.modules) { if ($hordeModule.sha256 -cnotin $hordeExpectedModules) { throw 'Shader identity differs from accepted C11' } }
$hordeCanonicalJson = Get-Content (Join-Path $hordeRepo 'assets/audio/music/what-the-dark-keeps/source/What_the_Dark_Keeps_Pocket_Chordsmith.json') -Raw | ConvertFrom-Json
$hordePcs1 = (Get-Content (Join-Path $hordeRepo 'assets/audio/music/what-the-dark-keeps/source/What_the_Dark_Keeps_PCS1.txt') -Raw).Trim()
if (-not $hordePcs1.StartsWith('PCS1:')) { throw 'PCS1 prefix absent' }
$hordePcsJson = [Text.Encoding]::UTF8.GetString([Convert]::FromBase64String($hordePcs1.Substring(5))) | ConvertFrom-Json
if (($hordeCanonicalJson | ConvertTo-Json -Depth 100 -Compress) -cne ($hordePcsJson | ConvertTo-Json -Depth 100 -Compress)) { throw 'Canonical JSON and PCS1 disagree' }
[pscustomobject]@{
    classification='runtime-music-admission-and-build-only-not-playback-or-device-acceptance'
    baseCommit='0db0d0b2b47e6c208911dab8b8129793ed7918cc'
    sourceState='authorised edits over base; reviewed source recorded by commit, not pristine base artifact'
    apk=$hordeApk
    apkBytes=(Get-Item -LiteralPath $hordeApk).Length
    apkSha256=(Get-FileHash -LiteralPath $hordeApk).Hash.ToLowerInvariant()
    build='Debug universal, four ABIs'
    musicManifestSha256=$script:HordeMusicManifestSha256
    musicEntries=17
    musicWaveBytes=20160704
    musicPcmBytes=20160000
    sourceExcludedFromPackage=$true
    canonicalJsonPcs1Agreement=$true
    existingRenderAndSfxAssetsMatched=$hordeCount
    packagedLicenceMatchesCurrentSource=$true
    arm64Sha256=$hordeContainment.arm64Sha256
    actualDiagnosticMobileModules=4
    moduleValidation='fresh val/dis PASS, identities equal accepted C11'
    installed=$false
    playbackWired=$false
    physicalDeviceAcceptance=$false
} | ConvertTo-Json -Depth 5
