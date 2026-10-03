$ErrorActionPreference = 'Stop'

$repo = 'C:/Users/sam_s/.codex/worktrees/horde-whistle-listening/the Horde RT Demo'
$baseline = 'C:/Dev/tmp/horde-fast-resize-20261001/HordeLanternRT-fast-resize-final-debug-arm64.apk'
$baselinePin = 'c11ff703794d74dbfb75dc7a0186af0f6c5c416e8383755a5cd8f84310daf0fe'
$logPath = 'C:/Dev/tmp/horde-music-instrumentation-20261001/whistle-bank/listening-package-inspector-reserved.log'
$sdk = 'C:/Users/sam_s/AppData/Local/Android/Sdk'

if (Test-Path -LiteralPath $logPath) { throw "Refusing overwrite: $logPath" }
$android = Join-Path $repo 'android'
$apk = & (Join-Path $repo 'tools/resolve-android-apk.ps1') -AndroidRoot $android -Variant debug
$apkHash = (Get-FileHash -LiteralPath $apk -Algorithm SHA256).Hash.ToLowerInvariant()
if ((Get-FileHash -LiteralPath $baseline -Algorithm SHA256).Hash.ToLowerInvariant() -cne $baselinePin) {
    throw 'C11 baseline APK SHA-256 differs from the supplied immutable pin.'
}

. (Join-Path $repo 'tools/music-asset-policy.ps1')
$manifest = Assert-HordeMusicPackage -RepositoryRoot $repo -ArchivePath $apk
$env:VULKAN_SDK = 'C:/VulkanSDK/1.4.350.0'
$strippedArm64 = Join-Path $android 'app/build/intermediates/stripped_native_libs/debug/stripDebugDebugSymbols/out/lib/arm64-v8a/libhorde_rt_probe_android.so'
$packageScanner = & (Join-Path $repo 'tools/InspectAndroidRtPipelineBundlePackage.ps1') `
    -Scanner (Join-Path $repo 'tools/InspectRtPipelineBundleContainment.ps1') `
    -StrippedLibraryPath $strippedArm64 -ApkPath $apk -Instrumentation Diagnostic -Quality Mobile |
    ConvertFrom-Json
if ($packageScanner.instrumentation -cne 'Diagnostic' -or $packageScanner.quality -cne 'Mobile' -or
    $packageScanner.packaged.spirvVal -cne 'passed' -or $packageScanner.packaged.spirvDis -cne 'passed' -or
    @($packageScanner.packaged.modules).Count -ne 4) {
    throw 'Actual ARM64 Diagnostic/Mobile APK module validation did not pass.'
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
$currentZip = [IO.Compression.ZipFile]::OpenRead($apk)
$baselineZip = [IO.Compression.ZipFile]::OpenRead($baseline)
$tempParent = 'C:/Dev/tmp/horde-pocket-audio-core-native-20261001'
$tempRoot = Join-Path $tempParent ('tmp-abi-exports-' + [guid]::NewGuid().ToString('N'))
try {
    [void](New-Item -ItemType Directory -Path $tempRoot)
    $musicPrefix = 'assets/audio/music/what-the-dark-keeps/'
    $musicEntries = @($currentZip.Entries | Where-Object {
        $_.FullName.StartsWith($musicPrefix, [StringComparison]::Ordinal)
    })
    if ($musicEntries.Count -ne 17) {
        throw "Expected exact 17 music manifest/WAV entries; found $($musicEntries.Count)."
    }
    $oldAssets = @($baselineZip.Entries | Where-Object {
        $_.FullName.StartsWith('assets/', [StringComparison]::Ordinal) -and
        $_.FullName -cne 'assets/ASSET_LICENSES.md'
    })
    if ($oldAssets.Count -ne 52) {
        throw "Expected 52 pre-existing C11 assets excluding its licence manifest; found $($oldAssets.Count)."
    }
    $matchedAssets = 0
    foreach ($old in $oldAssets) {
        $matches = @($currentZip.Entries | Where-Object { $_.FullName -ceq $old.FullName })
        if ($matches.Count -ne 1 -or $matches[0].Length -ne $old.Length -or
            (Get-HordeMusicZipEntrySha256 -Entry $matches[0]) -cne (Get-HordeMusicZipEntrySha256 -Entry $old)) {
            throw "Pre-existing asset mismatch or duplicate: $($old.FullName)"
        }
        $matchedAssets++
    }

    $licensePath = Join-Path $repo 'ASSET_LICENSES.md'
    $licenseEntries = @($currentZip.Entries | Where-Object { $_.FullName -ceq 'assets/ASSET_LICENSES.md' })
    if ($licenseEntries.Count -ne 1 -or $licenseEntries[0].Length -ne (Get-Item -LiteralPath $licensePath).Length -or
        (Get-HordeMusicZipEntrySha256 -Entry $licenseEntries[0]) -cne (Get-HordeMusicFileSha256 -Path $licensePath)) {
        throw 'Packaged ASSET_LICENSES.md differs from current admitted source.'
    }
    if (@($currentZip.Entries | Where-Object {
        $_.FullName.StartsWith($musicPrefix + 'source/', [StringComparison]::Ordinal)
    }).Count -ne 0) {
        throw 'Canonical music source documents were packaged.'
    }

    $nm = Join-Path $sdk 'ndk/28.2.13676358/toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-nm.exe'
    if (-not (Test-Path -LiteralPath $nm -PathType Leaf)) { throw "llvm-nm missing: $nm" }
    $abis = @('arm64-v8a', 'armeabi-v7a', 'x86', 'x86_64')
    $requiredExports = @(
        'Java_com_samfa12_hordelanternrt_HordeMusicPlayback_nativeCreate',
        'Java_com_samfa12_hordelanternrt_HordeMusicPlayback_nativePoll',
        'Java_com_samfa12_hordelanternrt_HordeMusicPlayback_nativeRender',
        'Java_com_samfa12_hordelanternrt_HordeMusicPlayback_nativeDestroy'
    )
    $abiReports = @()
    foreach ($abi in $abis) {
        $entryPath = "lib/$abi/libhorde_rt_probe_android.so"
        $entries = @($currentZip.Entries | Where-Object { $_.FullName -ceq $entryPath })
        if ($entries.Count -ne 1) { throw "Expected exactly one packaged library $entryPath; found $($entries.Count)." }
        $entry = $entries[0]
        $stripped = Join-Path $android "app/build/intermediates/stripped_native_libs/debug/stripDebugDebugSymbols/out/lib/$abi/libhorde_rt_probe_android.so"
        if (-not (Test-Path -LiteralPath $stripped -PathType Leaf)) { throw "Stripped ABI library missing: $stripped" }
        $apkLibHash = Get-HordeMusicZipEntrySha256 -Entry $entry
        $strippedHash = (Get-FileHash -LiteralPath $stripped -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($entry.Length -ne (Get-Item -LiteralPath $stripped).Length -or $apkLibHash -cne $strippedHash) {
            throw "Packaged and stripped library differ for $abi."
        }
        $extracted = Join-Path $tempRoot "$abi-libhorde_rt_probe_android.so"
        $input = $entry.Open()
        try {
            $output = [IO.File]::Create($extracted)
            try { $input.CopyTo($output) } finally { $output.Dispose() }
        } finally { $input.Dispose() }
        $symbolText = (& $nm -D --defined-only $extracted 2>&1 | Out-String)
        if ($LASTEXITCODE -ne 0) { throw "llvm-nm failed for ${abi}: $symbolText" }
        $exports = @($requiredExports | ForEach-Object {
            [pscustomobject]@{ name = $_; present = ($symbolText -match [regex]::Escape($_)) }
        })
        if (@($exports | Where-Object { -not $_.present }).Count -ne 0) {
            throw "Required HordeMusicPlayback JNI export missing from $abi."
        }
        $abiReports += [pscustomobject]@{
            abi = $abi; apkEntry = $entryPath; bytes = $entry.Length; apkSha256 = $apkLibHash
            strippedBytes = (Get-Item -LiteralPath $stripped).Length; strippedSha256 = $strippedHash
            musicJniExports = $exports
        }
    }

    $allAssetEntries = @($currentZip.Entries | Where-Object {
        $_.FullName.StartsWith('assets/', [StringComparison]::Ordinal)
    })
    [pscustomobject]@{
        sourceHead = (git -C $repo rev-parse HEAD).Trim()
        sourceWorkingTreeDirty = $true
        buildContext = 'Detached97f07e2 plus bounded Android music-sink buffer correction only; two manifest checkout line endings restored byte-for-byte from accepted C11 APK after exact canonical Git blob comparison. No dirty renderer experiment. Listening candidate, not release/performance acceptance.'
        variant = 'debug'
        apk = [pscustomobject]@{ path = $apk; bytes = (Get-Item -LiteralPath $apk).Length; sha256 = $apkHash }
        arm64DiagnosticMobile = $packageScanner
        music = [pscustomobject]@{
            manifestSha256 = $script:HordeMusicManifestSha256; rights = $manifest.rights
            runtimeEntryCount = $musicEntries.Count; canonicalSourcesExcluded = $true
            sourceAssert = 'Assert-HordeMusicPackage PASS'
        }
        oldAssetParity = [pscustomobject]@{
            baselinePath = $baseline; baselineSha256 = $baselinePin
            preexistingAssetEntriesCompared = $matchedAssets; allMatch = $true
            licenseMatchesCurrentSource = $true; currentAssetEntryCount = $allAssetEntries.Count
        }
        abiLibraries = $abiReports
        hardwareOrDeviceValidation = $false
    } | ConvertTo-Json -Depth 9 -Compress
}
finally {
    $currentZip.Dispose()
    $baselineZip.Dispose()
    $fullTemp = [IO.Path]::GetFullPath($tempRoot)
    $fullParent = [IO.Path]::GetFullPath($tempParent).TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
    if ($fullTemp.StartsWith($fullParent, [StringComparison]::OrdinalIgnoreCase) -and
        [IO.Path]::GetFileName($fullTemp).StartsWith('tmp-abi-exports-', [StringComparison]::Ordinal) -and
        (Test-Path -LiteralPath $fullTemp)) {
        Remove-Item -LiteralPath $fullTemp -Recurse -Force
    }
}
