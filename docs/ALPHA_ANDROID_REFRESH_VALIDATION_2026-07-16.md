# Android alpha refresh validation - 2026-07-16

## Device and artifact route

- Device: Samsung `SM-S948B`, Android 16, Adreno 840.
- Validation app: side-by-side debug application ID `com.samfa12.hordelanternrt.debug`; the stable public package remained installed.
- Orientation: portrait, rotation `0`, `1440x3120` display with `1440x2980` RT surface.
- Build gates: `assembleDebug`, `installDebug`, `assembleRelease`, and `lintRelease` passed.
- Published signed APK SHA-256: `13bced0aa40e4a102e25aa1c57083f7feb4b12ca7a8d492c46cc7c6cfdda932a`.
- Itch channel/build: `samfa12/the-horde:android`, upload `#18341739`, build `#1798652`.

## Runtime checks

- Branded portrait menu, settings, scrolling Credits & Licences, and compact diagnostics fit at the device's accessibility font scale.
- Thirteen FilmCow SoundPool clips loaded without failure.
- Strict ASTC material arrays selected.
- The real RT frame reached Android swapchain presentation.
- Render scale starts at 100%; live interaction recreated the target at 84% and Reset Defaults restored 100%.
- Diagnostics now publish the first 30-frame sample promptly. The captured fast panel records `1440x2980`, `78.01 fps / 12.82 ms`, `RT scene presented: yes`, and `RayTracingPipeline`.

## Android 16 / 16 KiB compatibility

Reference: https://developer.android.com/guide/practices/page-sizes

- The C++ runtime is statically linked, so the r26 `libc++_shared.so` is not packaged.
- Every arm64 release ELF `LOAD` segment is aligned to `0x4000`.
- `zipalign -c -P 16 -v 4` succeeds for the APK.
- `tools/package-alpha.ps1` now rejects a candidate if APK alignment, ELF alignment, or the no-shared-r26-runtime requirement regresses.

## Evidence

- `validation/android-final-2026-07-16/entry-portrait.png`
- `validation/android-final-2026-07-16/settings-portrait.png`
- `validation/android-final-2026-07-16/settings-render-scale.png`
- `validation/android-final-2026-07-16/credits-portrait.png`
- `validation/android-final-2026-07-16/credits-portrait-bottom.png`
- `validation/android-final-2026-07-16/diagnostics-portrait-fast.png`
- `validation/android-final-2026-07-16/signed-release-entry.png`

The exact stable-key-signed published APK was installed over the public package and launched beside the separate debug package. It remained portrait, showed the complete entry menu, dispatched at 100% (`1440x2980`), selected strict ASTC, and logged successful RT swapchain presentation. Warm native timing windows were approximately 12.4-12.9 ms before later device activity changed the load.

The earlier Samsung compatibility warning shown for the pre-fix debug process named both the project library and r26 `libc++_shared.so`. The rebuilt APK contains only the aligned project library; the old system dialog was dismissed before the final evidence capture.
