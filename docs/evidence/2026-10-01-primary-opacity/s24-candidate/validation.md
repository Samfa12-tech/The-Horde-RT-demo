# Android showcase automation run 20261001-223947

- Mode: Benchmark
- Device: SM-S928B (Android 16 / API 36)
- Debug APK SHA-256: `6da2bccf3ec90f4dfe0aef1b9899626c6fb0a13525359c6fc229146f3848fa8b`
- Installed base APK SHA-256: `6da2bccf3ec90f4dfe0aef1b9899626c6fb0a13525359c6fc229146f3848fa8b` (exact match)
- Source: `e78028c999e745e7cfd1695468062bb45f300b21` with a dirty worktree recorded
- Selected RT pipeline pair: `rayquery_compute_diagnostic_mobile_opaque_fast@e7a03381663212491c5483b7e5f4049470a8e3237faf3f7756e4214c68bee3c5 | rayquery_compute_diagnostic_mobile_generic_dielectric@62799da4e0e8f26277926f3289b2cf2f0f717eb05d60a6c30404bc5f24f9edea`
- Scale: 75%
- GPU timestamp instrumentation: disabled (RT rendering unchanged)
- Evidence type: automated deterministic checkpoint/replay evidence; visual quality and perceived spatial audio remain hands-on checks.
- Result: PASS

## Timing classification

Descriptive references: 16.667 ms ~= 60 FPS, 20.000 ms = 50 FPS, and 33.333 ms ~= 30 FPS. Crossing a reference line is reported, not treated as an automatic product failure. Compare matched runs and investigate regressions above 15%.



See `timing.csv`, `summary.json`, `logcat.txt`, checkpoint state JSON, screenshots (when requested), and the private Vulkan capability report in this directory.
