import sys,subprocess
for config,strip,instrumentation,apk in [
 ('debug','debug/stripDebugDebugSymbols','Diagnostic','debug/app-debug.apk'),
 ('release','release/stripReleaseDebugSymbols','Shipping','release/app-release-unsigned.apk')]:
 args=['pwsh','-NoProfile','-File','tools/InspectAndroidRtPipelineBundlePackage.ps1',
  '-Scanner','tools/InspectRtPipelineBundleContainment.ps1',
  '-StrippedLibraryPath',f'android/app/build/intermediates/stripped_native_libs/{strip}/out/lib/arm64-v8a/libhorde_rt_probe_android.so',
  '-ApkPath','android/app/build/outputs/apk/'+apk,'-Instrumentation',instrumentation,'-Quality','Mobile']
 code=subprocess.call([sys.executable,'reports/wp2a-combat/run.py','final-android-'+config+'-arm64-package',*args])
 if code:sys.exit(code)
sys.exit(subprocess.call([sys.executable,'reports/wp2a-combat/run.py','final-android-all-abi-package',
 'python','reports/wp2a-combat/android_package_identity.py']))
