# Android showcase automation - durable evidence digest

This tracked digest preserves the reviewed milestone evidence from the otherwise Git-ignored routine run directories.

- Device: `SM-S948B`, Android 16 / API 36, Adreno 840
- Package: `com.samfa12.hordelanternrt.debug`, `versionName 0.1.1-alpha.1-debug`, `versionCode 2`
- Debug APK: 57,093,003 bytes; SHA-256 `1db493ac0362ef4a49dcc2286d31d7388e98751b12607f833a36f5abdfebe962`
- Source base before the uncommitted harness/checkpoint consolidation: published commit `08194759dfc96fb6a0007012f7c6d834d71289d9`
- Corrected fast invocation: `.\tools\run-android-showcase-validation.ps1 -SkipBuild -SkipInstall`
- Full invocation: `.\tools\run-android-showcase-validation.ps1 -Include100 -Capture`
- Validation script after Samsung thermal fallback: SHA-256 `e3b041d3e9c7bd0ce3e4a0389b171abb27962cd65f87fee32600a052d6fce3e3`

`timing.csv` combines the corrected 75% default set with the full run's report-only 100% opening. `state-evidence.json` preserves the five completed 75% native states, the completed replay state, and the 100% opening state. `vulkan_capability_report.json` preserves the renderer/device claim. Full raw logcat, system dumps, and PNGs remain in local ignored run directories; their reviewed hashes are listed in `evidence-hashes.txt`.

These status-0/cool automated figures are deterministic regression evidence. The warm sustained route certification remains `docs/HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md`.
