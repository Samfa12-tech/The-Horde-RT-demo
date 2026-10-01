# Android showcase automation run 20261002-002823

- Mode: Benchmark
- Device: SM-S928B (Android 16 / API 36)
- Debug APK SHA-256: `2904c7e2bf82c3690cd443d2ce58343cd3b236f8da5b8958609ba0c4d9481e55`
- Installed base APK SHA-256: `2904c7e2bf82c3690cd443d2ce58343cd3b236f8da5b8958609ba0c4d9481e55` (exact match)
- Source: `eef3c79d8e80bbdebccb76eebd58b9ad3945b10d` with a dirty worktree recorded
- Selected RT pipeline pair: `rayquery_compute_diagnostic_mobile_opaque_fast@8f3bd5d4b69c1ee82c6e8be308fbe017e4fe7df70a5f3751afdd24cac34657c9 | rayquery_compute_diagnostic_mobile_generic_dielectric@e18171591d4b07ccc3e7fb071400a6581e1fc132aec06dd37c09f88c7792e302`
- Scale: 75%
- GPU timestamp instrumentation: disabled (RT rendering unchanged)
- Evidence type: automated deterministic checkpoint/replay evidence; visual quality and perceived spatial audio remain hands-on checks.
- Result: PASS

## Timing classification

Descriptive references: 16.667 ms ~= 60 FPS, 20.000 ms = 50 FPS, and 33.333 ms ~= 30 FPS. Crossing a reference line is reported, not treated as an automatic product failure. Compare matched runs and investigate regressions above 15%.



See `timing.csv`, `summary.json`, `logcat.txt`, checkpoint state JSON, screenshots (when requested), and the private Vulkan capability report in this directory.
