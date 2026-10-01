# Android showcase automation run 20261002-000200

- Mode: Benchmark
- Device: SM-S928B (Android 16 / API 36)
- Debug APK SHA-256: `dd69eeb7947d631450780665324380766a85d3d03f42fba661ca21523ac0183c`
- Installed base APK SHA-256: `dd69eeb7947d631450780665324380766a85d3d03f42fba661ca21523ac0183c` (exact match)
- Source: `e8b696591c8ca1fefbc094238867a845846ccd1d` with a dirty worktree recorded
- Selected RT pipeline pair: `rayquery_compute_diagnostic_mobile_opaque_fast@d466e66ee4ecfb3a2f7ee2d22337eb811c8744802ac5076fe7e9176792805bac | rayquery_compute_diagnostic_mobile_generic_dielectric@e18171591d4b07ccc3e7fb071400a6581e1fc132aec06dd37c09f88c7792e302`
- Scale: 75%
- GPU timestamp instrumentation: disabled (RT rendering unchanged)
- Evidence type: automated deterministic checkpoint/replay evidence; visual quality and perceived spatial audio remain hands-on checks.
- Result: PASS

## Timing classification

Descriptive references: 16.667 ms ~= 60 FPS, 20.000 ms = 50 FPS, and 33.333 ms ~= 30 FPS. Crossing a reference line is reported, not treated as an automatic product failure. Compare matched runs and investigate regressions above 15%.



See `timing.csv`, `summary.json`, `logcat.txt`, checkpoint state JSON, screenshots (when requested), and the private Vulkan capability report in this directory.
