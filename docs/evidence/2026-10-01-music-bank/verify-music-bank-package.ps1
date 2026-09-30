$ErrorActionPreference = 'Stop'
$hordeRepo = 'C:/Users/sam_s/Documents/the Horde RT Demo/.worktrees/horde-1.6.1-engineering-pass'
. (Join-Path $hordeRepo 'tools/music-asset-policy.ps1')
$hordeApk = & (Join-Path $hordeRepo 'tools/resolve-android-apk.ps1') -AndroidRoot (Join-Path $hordeRepo 'android') -Variant debug
$hordeApkHash = (Get-FileHash -LiteralPath $hordeApk).Hash.ToLowerInvariant()
if ($hordeApkHash -cne '450d2cf6a331bc482302a81a27339f812d6f7a44c29772e6f5b151d04dfb3f2d') { throw 'Bank APK identity differs; rebuilt artifacts need a new receipt' }
$null = Assert-HordeMusicPackage -RepositoryRoot $hordeRepo -ArchivePath $hordeApk
$hordeBaseline = 'C:/Dev/tmp/horde-fast-resize-20261001/HordeLanternRT-fast-resize-final-debug-arm64.apk'
if ((Get-FileHash -LiteralPath $hordeBaseline).Hash.ToLowerInvariant() -cne 'c11ff703794d74dbfb75dc7a0186af0f6c5c416e8383755a5cd8f84310daf0fe') { throw 'Baseline APK identity differs' }
$hordeContainment = Get-Content 'C:/Dev/tmp/horde-pocket-audio-core-native-20261001/music-bank-actual-module-validation.log' -Raw | ConvertFrom-Json
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
    if ($hordeCount -ne 52) { throw "Expected 52 unchanged render/SFX assets, got $hordeCount" }
    if ((Get-HordeMusicZipEntrySha256 -Entry $hordeNewZip.GetEntry('assets/ASSET_LICENSES.md')) -cne (Get-FileHash -LiteralPath (Join-Path $hordeRepo 'ASSET_LICENSES.md')).Hash.ToLowerInvariant()) { throw 'Packaged licence is stale' }
    $hordeNativeEntry = $hordeNewZip.GetEntry('lib/arm64-v8a/libhorde_rt_probe_android.so')
    if ((Get-HordeMusicZipEntrySha256 -Entry $hordeNativeEntry) -cne $hordeContainment.arm64Sha256 -or
        $hordeNativeEntry.Length -ne $hordeContainment.packaged.targetBytes -or
        $hordeContainment.arm64Sha256 -cne '5b122656de753abaac94f7ceeff872e3d964bbeb2c17dea95cca78f59827619d') { throw 'Fresh containment receipt does not identify this APK native entry' }
} finally { $hordeOldZip.Dispose(); $hordeNewZip.Dispose() }
$hordeExpectedModules = @('c1e4622ae5df0c87a06c36d803617c7d4e0227845cd33a9dd38de5f0d956ff58', 'e18171591d4b07ccc3e7fb071400a6581e1fc132aec06dd37c09f88c7792e302', '8dbebd86eeed23b388626719e6f9946b8027df754869b5811673e24036ae12c9', '9865690c11de315c1c4e49af7d14eb8963b24dc522abfc01c53979bb1a5f5a91')
if ($hordeContainment.packaged.spirvVal -cne 'passed' -or $hordeContainment.packaged.spirvDis -cne 'passed' -or $hordeContainment.packaged.modules.Count -ne 4) { throw 'Actual module validation incomplete' }
foreach ($hordeModule in $hordeContainment.packaged.modules) { if ($hordeModule.sha256 -cnotin $hordeExpectedModules) { throw 'Shader identity differs from accepted C11' } }
[pscustomobject]@{
    classification='immutable-music-bank-compile-and-package-only-not-playback-or-device-acceptance'
    baseCommit='199697bf52b811e7f45ab18476093485166feff0'
    sourceState='authorised bank edits over base; final commit identifies reviewed source, not pristine base artifact'
    apk=$hordeApk
    apkBytes=(Get-Item -LiteralPath $hordeApk).Length
    apkSha256=$hordeApkHash
    build='Debug universal, four ABIs'
    musicManifestSha256=$script:HordeMusicManifestSha256
    musicEntries=17
    musicWaveBytes=20160704
    musicPcmBytes=20160000
    sourceExcludedFromPackage=$true
    existingRenderAndSfxAssetsMatched=$hordeCount
    packagedLicenceMatchesCurrentSource=$true
    arm64Bytes=$hordeContainment.packaged.targetBytes
    arm64Sha256=$hordeContainment.arm64Sha256
    actualDiagnosticMobileModules=4
    moduleValidation='fresh val/dis PASS, identities equal accepted C11'
    installed=$false
    playbackWired=$false
    physicalDeviceAcceptance=$false
} | ConvertTo-Json -Depth 5
