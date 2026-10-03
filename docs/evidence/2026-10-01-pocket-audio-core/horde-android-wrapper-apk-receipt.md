# Horde Pocket Audio Core wrapper APK check

Scope: package-only verification of the uncommitted thin Horde wrappers and pinned Pocket Audio Core integration. No install/device or runtime claim.

## Build and artifact provenance

- Worktree: `C:\Users\sam_s\Documents\the Horde RT Demo\.worktrees\horde-1.6.1-engineering-pass`
- HEAD: `baa8bfecba25de509e25803e3b4f231c54834745`; wrapper/pin edits were still uncommitted.
- Build log: `C:\Dev\tmp\horde-pocket-audio-core-native-20261001\horde-android-debug.log`
- `:app:assembleDebug` succeeded in 2m54s (41 tasks: 16 executed, 25 up-to-date).
- Warnings: Android tooling emitted `[CXX5304]` twice because it encountered SDK XML v4 while understanding through v3. No C++ compiler warning/error or build failure was found in the log.
- Resolved with `tools\resolve-android-apk.ps1 -AndroidRoot <worktree>\android -Variant debug` (not a guessed path): `android\app\build\outputs\apk\debug\app-debug.apk`, 89,334,059 bytes, SHA-256 `EAB79C4A5F6757B600824F85E08B290CE3FE57E735022647C41377C69589D21A`.
- The resolved Debug APK is universal: it contains arm64-v8a, armeabi-v7a, x86, and x86_64 native entries. It is not an ARM64-only APK.
- Accepted baseline: `C:\Dev\tmp\horde-fast-resize-20261001\HordeLanternRT-fast-resize-final-debug-arm64.apk`, 78,232,323 bytes, SHA-256 `C11FF703794D74DBFB75DC7A0186AF0F6C5C416E8383755A5CD8F84310DAF0FE` (arm64-only).

## Package comparison

- Enumerated and SHA-256 hashed every ZIP entry in both APKs. All 53 `assets/**` entries exist in each and are byte-identical (0 asset additions, omissions, or hash differences).
- Other package differences: AndroidManifest.xml differs; the current ARM64 native library differs as expected from the Core audio integration; the current APK adds the three non-ARM64 ABI native libraries absent from the ARM64-only baseline. No asset changes are indicated.
- Current APK's `lib/arm64-v8a/libhorde_rt_probe_android.so`: 3,714,976 bytes, SHA-256 `B09C010DC9D879A24D739BB175EE9B6A0599AD6BA7BD9BA15577811ADC84CD42`.
- Baseline APK's ARM64 library: 3,681,056 bytes, SHA-256 `324529792FBBC7F9C6BA4DEA54C921F55689315433086A9BE326377BDF8CECA2`.

## Embedded shader verification

Used existing `tools\InspectAndroidRtPipelineBundlePackage.ps1` with the exact resolved APK and that build's `stripped_native_libs\debug\stripDebugDebugSymbols\out\lib\arm64-v8a\libhorde_rt_probe_android.so`, selecting the actual `Diagnostic / Mobile` Debug catalog. Both the stripped library and exact APK-extracted ARM64 entry passed `spirv-val --target-env vulkan1.2` and `spirv-dis`; their summaries and hashes agree.

The four selected modules (same exact hashes in candidate and accepted baseline C11) are:

| Module | SHA-256 |
| --- | --- |
| RayQueryCompute, generic dielectric | `c1e4622ae5df0c87a06c36d803617c7d4e0227845cd33a9dd38de5f0d956ff58` |
| RayQueryCompute, opaque fast | `e18171591d4b07ccc3e7fb071400a6581e1fc132aec06dd37c09f88c7792e302` |
| RayTracingPipeline raygen, generic dielectric | `8dbebd86eeed23b388626719e6f9946b8027df754869b5811673e24036ae12c9` |
| RayTracingPipeline raygen, opaque fast | `9865690c11de315c1c4e49af7d14eb8963b24dc522abfc01c53979bb1a5f5a91` |

The existing package inspector receipt/log is `C:\Dev\tmp\horde-pocket-audio-core-native-20261001\horde-apk-four-module-validation.log`. Module offsets differ because the native library layout changed; module byte hashes, selected semantic keys, and recorded instrumentation/reflection properties match the baseline. This verifies packaged module identity/validity only, not Vulkan device execution or RT presentation.

An initial exploratory `Shipping / Mobile` scanner call rejected the Debug library because it contains Diagnostic modules; this was the wrong catalog selection, not a Shipping-build test. The correctly selected `Diagnostic / Mobile` check passed. No Shipping build was performed.
