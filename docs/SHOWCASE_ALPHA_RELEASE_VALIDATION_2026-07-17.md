# Showcase Alpha 0.1.1 release validation - 2026-07-17

## Published identity

- Display: **Horde Lantern RT - Showcase Alpha 0.1.1**
- Package version: `0.1.1-alpha.1`
- Android application ID: `com.samfa12.hordelanternrt`
- Android version code: `2`
- Windows itch channel: `samfa12/the-horde:windows-x64`, upload `#18339908`, active build `#1801016`
- Android itch channel: `samfa12/the-horde:android`, upload `#18341739`, active build `#1801017`

## Exact published artifacts

| Artifact | SHA-256 |
|---|---|
| `Horde-Lantern-RT-Alpha-0.1.1-alpha.1-Windows-x64.zip` | `8a254c9d14b35bf868f1cb96619dc572f3505a9564b668aa55241b33bfeaec2e` |
| `Horde-Lantern-RT-Alpha-0.1.1-alpha.1-Android.apk` | `ae73afec2c75b317187aeb61d81a592ec8bb4d8b5e89ef9b474fb2a60ae1354a` |

The Android candidate verifies with one RSA-4096 signer and the established certificate SHA-256 `8245277a11bca5576f116724507f799d6f4c178ce5fbb7e3981415c9e6b3c245`. APK Signature Scheme v2 verification passed. The guarded upload script additionally passed exact-certificate, SHA-256, 16 KiB APK alignment, packaged native-library, and authenticated-channel checks before Butler was allowed to push.

## Build and host tests

- Windows Debug: full build passed; all five CTests passed.
- Windows Release: full build passed; all five CTests passed.
- Android: `assembleDebug`, `assembleRelease`, and `lintRelease` passed for all configured ABIs.
- Packaging verified the embedded Windows file/product version, Per-Monitor V2 manifest, in-app credit markers, archive entries, Android version/orientation/credit resources, strict runtime assets, 16 KiB APK and ELF alignment, static C++ runtime, and absence of `libc++_shared.so`.

## Exact-candidate launch checks

The published Android APK was installed as an update over the stable public application and launched on `SM-S948B`. Device package state reported `versionCode=2` and `versionName=0.1.1-alpha.1`. All seventeen SFX loaded; the renderer selected strict ASTC environment textures plus strict ASTC 6x6 lich textures and logged `RT frame reached Android swapchain presentation`. No current fatal Java/native or renderer-start failure was observed.

The published Windows zip was extracted to an empty verification directory and launched using only its packaged files. The running window title and executable resource both reported Showcase Alpha 0.1.1 / `0.1.1-alpha.1`. Its generated report selected `RayTracingPipeline`, identified the RTX 5050 Laptop GPU, described the complete skeleton/lich route, and reported `RT scene presented: yes`. Packaged XAudio2 startup found the executable-relative audio tree.

Full route visual, input, combat, audio, lifecycle, and performance evidence remains in `HORDE_SHOWCASE_WINDOWS_VALIDATION_2026-07-16.md` and `HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md`.

## Publication

The first upload attempt was not made when the guarded preflight detected a stale Windows hash record after the signed rebuild. The hash manifest was regenerated from the independently verified exact candidates, the complete preflight then passed, and only those files were uploaded. Butler subsequently reported both new builds active at user version `0.1.1-alpha.1`.

Status: **Windows-validated / Android-device-validated / published to itch**.
