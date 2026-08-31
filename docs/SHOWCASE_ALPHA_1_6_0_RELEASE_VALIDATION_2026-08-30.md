# Horde Lantern RT - Showcase Alpha 1.6.0 Release Validation

Prepared: 2026-08-30
Published and closed: 2026-08-31

## Released identity and source

- Package version: `1.6.0`.
- Android package: `com.samfa12.hordelanternrt`; `versionName 1.6.0`; `versionCode 8`.
- Runtime release source: clean `main` commit `57c81b635a6c10e2772283639026936adac80f8b`.
- Android signing certificate SHA-256: `8245277a11bca5576f116724507f799d6f4c178ce5fbb7e3981415c9e6b3c245`.

## Clean source gate

`reports/foundation-runs/run-20260831-131431` ran from clean source commit `57c81b6` and passed all seven Host stages:

- raygen compilation freshness and negative fixtures;
- fresh Windows Debug 31/31 and Release 31/31 Vulkan-enabled CTests;
- all 13 standard deterministic Windows RT captures;
- Android Debug and unsigned Release builds plus Release lint;
- strict ARM64 runtime-asset sizing and package/licence contracts;
- evidence hashing with clean before/after Git state.

## Exact public artifacts

| Platform | Candidate | Bytes | SHA-256 | Itch result |
|---|---|---:|---|---|
| Windows x64 | `Horde-Lantern-RT-Alpha-1.6.0-Windows-x64.zip` | 105,508,271 | `7b0dcf24b4a47771a9c3a27cbc52e3899c87781109afcef20f7a9a8472411d77` | channel `#18339908`, build `#1931949` |
| Android | `Horde-Lantern-RT-Alpha-1.6.0-Android.apk` | 82,357,855 | `52a64255ad5dec82cc866fb2ea3545be498ca06c73a789019be851c77e5d6c48` | channel `#18341739`, build `#1931951` |

The signed APK passed the established-certificate, package identity, version, signature, ABI, native-library, strict runtime-asset, licence, lint, and compatibility guards. Its 66-entry payload matched the Host unsigned Release APK except for expected signing metadata.

## Windows exact-package smoke

The exact ZIP above was extracted to `reports/release-smoke/windows-1.6.0-20260831-133408` and launched using only packaged files. It reported file/product version `1.6.0`, selected `RayTracingPipeline` on the NVIDIA GeForce RTX 5050 Laptop GPU, presented an RT-produced image through the swapchain, received direct `WM_CLOSE`, and exited with code 0. No game process remained.

## Android evidence boundary

ADB exposed no device at publication. Therefore the exact signed 1.6.0 APK has no install, pullback, strict-ASTC runtime, honest-phone-presentation, Home/resume, performance, or owner-feel claim.

The same feature runtime had already been installed and byte-matched on the sole authorised `SM-S948B` as final Debug APK SHA-256 `0b5a59b6e41d2c4d717eff885aaa310b7f5f1512002f6a89cb77e5989ab7edd3`. That candidate passed strict ASTC, `RayTracingPipeline`, honest RT presentation, deterministic replay/captures, Home/resume, 75% and 100% feature evidence, and owner review of phone composition, fire, chest/reward progression, sword combo, audio, and haptics. This is supporting runtime evidence, not proof that the signed public APK ran on the phone.

## Publication verification

Guarded Butler preflight completed before upload. Final Butler status reported Windows build `#1931949` and Android build `#1931951`, both active at user version `1.6.0`. An anonymous cache-busting request to `https://samfa12.itch.io/the-horde` returned HTTP 200 and exposed both 1.6.0 downloads.

No Git tag or GitHub Release was created. The in-game updater foundation queries public GitHub Releases, so itch publication alone does not announce 1.6.0 through that updater. Creating a `v1.6.0` GitHub Release remains a separate owner-authorised publication action.

## Accepted boundaries

- Normal gameplay retains the accepted block-arm viewmodel; skinned first-person gauntlets and arm/body shadow/reflection polish are deferred.
- Mobile lantern glass keeps its bounded physical model and measured finite terminations; quality or render scale was not silently reduced.
- Two simultaneous skeletons remain the validated ceiling and the lich remains singular.
- The 1.6.0 release line and Android `versionCode 8` are immutable; a future Android release requires a new version and `versionCode > 8`.

Audio/haptic manual revalidation required: NO — release-only version, signing, packaging, and publication did not alter accepted runtime cue/event semantics.
