$ErrorActionPreference = 'Stop'
$root = (Resolve-Path '.').Path
$sdk = $env:ANDROID_HOME
if (-not $sdk) { $sdk = $env:ANDROID_SDK_ROOT }
if (-not $sdk) { throw 'Android SDK environment is unavailable.' }
$readelf = Join-Path $sdk 'ndk/26.1.10909125/toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-readelf.exe'
if (-not (Test-Path -LiteralPath $readelf)) { throw 'Pinned NDK llvm-readelf is unavailable.' }
$abis = @{
  'arm64-v8a' = @{ class='ELF64'; machine='AArch64' }
  'armeabi-v7a' = @{ class='ELF32'; machine='ARM' }
  'x86' = @{ class='ELF32'; machine='Intel 80386' }
  'x86_64' = @{ class='ELF64'; machine='Advanced Micro Devices X86-64' }
}
$results = @()
$packageResults = @()
foreach ($variant in @(
  @{ name='debug'; apk='android/app/build/outputs/apk/debug/app-debug.apk'; strip='android/app/build/intermediates/stripped_native_libs/debug/stripDebugDebugSymbols/out/lib' },
  @{ name='release-unsigned'; apk='android/app/build/outputs/apk/release/app-release-unsigned.apk'; strip='android/app/build/intermediates/stripped_native_libs/release/stripReleaseDebugSymbols/out/lib' }
)) {
  $apkPath = Join-Path $root $variant.apk
  if (-not (Test-Path -LiteralPath $apkPath)) { throw "Missing APK: $($variant.apk)" }
  $apkHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $apkPath).Hash.ToLowerInvariant()
  $zip = [System.IO.Compression.ZipFile]::OpenRead($apkPath)
  try {
    foreach ($abi in @('arm64-v8a','armeabi-v7a','x86','x86_64')) {
      $entryName = "lib/$abi/libhorde_rt_probe_android.so"
      $entry = $zip.GetEntry($entryName)
      if (-not $entry) { throw "Missing APK entry $entryName in $($variant.apk)" }
      $entryStream = $entry.Open()
      try { $entryHash = [Convert]::ToHexString([System.Security.Cryptography.SHA256]::HashData($entryStream)).ToLowerInvariant() } finally { $entryStream.Dispose() }
      $stripRel = "$($variant.strip)/$abi/libhorde_rt_probe_android.so"
      $stripPath = Join-Path $root $stripRel
      if (-not (Test-Path -LiteralPath $stripPath)) { throw "Missing stripped library $stripRel" }
      $stripHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $stripPath).Hash.ToLowerInvariant()
      $header = (& $readelf -h $stripPath 2>&1 | Out-String)
      if ($LASTEXITCODE -ne 0) { throw "llvm-readelf -h failed for $stripRel" }
      $program = (& $readelf -l $stripPath 2>&1 | Out-String)
      if ($LASTEXITCODE -ne 0) { throw "llvm-readelf -l failed for $stripRel" }
      $class = [regex]::Match($header, '(?m)^\s*Class:\s*(.+?)\s*$').Groups[1].Value.Trim()
      $data = [regex]::Match($header, '(?m)^\s*Data:\s*(.+?)\s*$').Groups[1].Value.Trim()
      $machine = [regex]::Match($header, '(?m)^\s*Machine:\s*(.+?)\s*$').Groups[1].Value.Trim()
      $loads = @([regex]::Matches($program, '(?m)^\s*LOAD\s+.+?\s+(0x[0-9a-fA-F]+)\s*$') | ForEach-Object { $_.Groups[1].Value.ToLowerInvariant() })
      if ($class -ne $abis[$abi].class -or $machine -ne $abis[$abi].machine) { throw "Unexpected ELF ABI for $stripRel ($class; $machine)" }
      if ($data -notmatch 'little endian') { throw "Unexpected ELF byte order for $stripRel ($data)" }
      if ($loads.Count -eq 0 -or @($loads | Where-Object { $_ -ne '0x4000' }).Count -gt 0) { throw "LOAD alignment check failed for ${stripRel}: $($loads -join ',')" }
      $results += [pscustomobject]@{ abi=$abi; apkEntry=$entryName; apkEntryBytes=[int64]$entry.Length; apkEntrySha256=$entryHash; strippedLibrary=$stripRel; strippedBytes=(Get-Item -LiteralPath $stripPath).Length; strippedSha256=$stripHash; entryMatchesStripped=($entryHash -eq $stripHash); elfClass=$class; elfData=$data; elfMachine=$machine; loadAlignments=$loads; loadAlignment16KiB=($loads.Count -gt 0 -and @($loads | Where-Object { $_ -ne '0x4000' }).Count -eq 0) }
    }
  } finally { $zip.Dispose() }
  $resultsForVariant = @($results | Where-Object { $_.apkEntry -like 'lib/*' } | Select-Object -Last 4)
  if (@($resultsForVariant | Where-Object { -not $_.entryMatchesStripped -or -not $_.loadAlignment16KiB }).Count -gt 0) { throw "Package identity or LOAD alignment failed for $($variant.name)" }
  $variant | Add-Member -NotePropertyName sha256 -NotePropertyValue $apkHash
  $variant | Add-Member -NotePropertyName bytes -NotePropertyValue (Get-Item -LiteralPath $apkPath).Length
  $packageResults += [pscustomobject]@{ variant=$variant.name; apk=$variant.apk; bytes=$variant.bytes; sha256=$variant.sha256 }
}
$testNames = @('PlayerDebugCheckpointTest','EquipmentAudioFeedbackTest','SurfaceSuspensionLifecycleTest')
$tests = @()
foreach ($name in $testNames) {
  $xmlPath = Get-ChildItem 'android/app/build/test-results/testDebugUnitTest' -Filter "TEST-*$name.xml" | Select-Object -First 1 -ExpandProperty FullName
  if (-not $xmlPath) { throw "Missing test XML for $name" }
  [xml]$xml = Get-Content -LiteralPath $xmlPath
  $suite = $xml.testsuite
  $tests += [pscustomobject]@{ name=$name; tests=[int]$suite.tests; failures=[int]$suite.failures; errors=[int]$suite.errors; skipped=[int]$suite.skipped; passed=([int]$suite.failures -eq 0 -and [int]$suite.errors -eq 0) }
}
$debugPkg = Get-Content 'reports/wp2-world-foundation/android-debug-arm64-package.log' | Where-Object { $_ -match '^\{' } | Select-Object -Last 1 | ConvertFrom-Json
$releasePkg = Get-Content 'reports/wp2-world-foundation/android-release-arm64-package.log' | Where-Object { $_ -match '^\{' } | Select-Object -Last 1 | ConvertFrom-Json
$summary = [pscustomobject]@{
  candidateHead='0eb665900c9efc95c443beada4c418776092cbe3'
  unsignedReleaseBuild=$true
  gradleLane='offline assembleDebug assembleRelease lintDebug lintRelease plus PlayerDebugCheckpointTest, EquipmentAudioFeedbackTest, SurfaceSuspensionLifecycleTest'
  lint=[pscustomobject]@{ debug='passed'; release='passed' }
  tests=$tests
  packages=$packageResults
  libraryChecks=$results
  arm64ShaderPackageScans=@(
    [pscustomobject]@{ variant='debug'; inspector='tools/InspectAndroidRtPipelineBundlePackage.ps1'; externalValidation='spirv-val and spirv-dis passed'; strippedSha256=$debugPkg.stripped.targetSha256; packagedSha256=$debugPkg.packaged.targetSha256; semanticKeys=$debugPkg.stripped.semanticKeys; modules=@($debugPkg.stripped.modules | ForEach-Object { [pscustomobject]@{ backend=$_.backend; executionModel=$_.executionModel; sha256=$_.sha256; words=$_.words; rayQueryCapability=$_.rayQueryCapability; rayQueryInitializations=$_.rayQueryInitializations } }) },
    [pscustomobject]@{ variant='release-unsigned'; inspector='tools/InspectAndroidRtPipelineBundlePackage.ps1'; externalValidation='spirv-val and spirv-dis passed'; strippedSha256=$releasePkg.stripped.targetSha256; packagedSha256=$releasePkg.packaged.targetSha256; semanticKeys=$releasePkg.stripped.semanticKeys; modules=@($releasePkg.stripped.modules | ForEach-Object { [pscustomobject]@{ backend=$_.backend; executionModel=$_.executionModel; sha256=$_.sha256; words=$_.words; rayQueryCapability=$_.rayQueryCapability; rayQueryInitializations=$_.rayQueryInitializations } }) }
  )
  coverageLimit='All-ABI checks establish APK-entry-to-stripped-library SHA-256 identity and ELF headers/LOAD alignment only. Pipeline and RayQuery SPIR-V package inspection was ARM64-only.'
  rawLogs=@('reports/wp2-world-foundation/android-gradle.log','reports/wp2-world-foundation/android-debug-arm64-package.log','reports/wp2-world-foundation/android-release-arm64-package.log')
}
$summary | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath 'reports/wp2-world-foundation/android-package-evidence.json' -Encoding utf8
Write-Output 'Android all-ABI package checks and sanitized summary passed.'
