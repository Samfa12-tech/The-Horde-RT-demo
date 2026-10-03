# Android showcase automation run 20261002-010411

- Mode: Replay
- Device: SM-S928B (Android 16 / API 36)
- Debug APK SHA-256: `a627c4a6431f40708e14327f9585bd0e6c0e7ca3133a9ded97242dfd7763d761`
- Installed base APK SHA-256: `a627c4a6431f40708e14327f9585bd0e6c0e7ca3133a9ded97242dfd7763d761` (exact match)
- Source: `9f4042f00cfd6227fb1418b92e7f3b1b5ce3e4c4` with a dirty worktree recorded
- Selected RT pipeline pair: `rayquery_compute_diagnostic_mobile_opaque_fast@c1e4622ae5df0c87a06c36d803617c7d4e0227845cd33a9dd38de5f0d956ff58 | rayquery_compute_diagnostic_mobile_generic_dielectric@e18171591d4b07ccc3e7fb071400a6581e1fc132aec06dd37c09f88c7792e302`
- Scale: 75%
- GPU timestamp instrumentation: disabled (RT rendering unchanged)
- Evidence type: automated deterministic checkpoint/replay evidence; visual quality and perceived spatial audio remain hands-on checks.
- Result: PASS

## Timing classification

Descriptive references: 16.667 ms ~= 60 FPS, 20.000 ms = 50 FPS, and 33.333 ms ~= 30 FPS. Crossing a reference line is reported, not treated as an automatic product failure. Compare matched runs and investigate regressions above 15%.



See `timing.csv`, `summary.json`, `logcat.txt`, checkpoint state JSON, screenshots (when requested), and the private Vulkan capability report in this directory.
