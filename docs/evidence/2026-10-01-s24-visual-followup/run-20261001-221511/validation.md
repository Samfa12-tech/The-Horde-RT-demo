# Android showcase automation run 20261001-221511

- Mode: Benchmark
- Device: SM-S928B (Android 16 / API 36)
- Debug APK SHA-256: `8cb976891e5719eceb7fd809ec91aac939ff3ec58eb2d6df44f15b5526f4ff87`
- Installed base APK SHA-256: `8cb976891e5719eceb7fd809ec91aac939ff3ec58eb2d6df44f15b5526f4ff87` (exact match)
- Source: `6fa1c53d1f0e4ec3938983f2cad7bd2ece233f4a` from a clean worktree
- Selected RT pipeline pair: `rayquery_compute_diagnostic_mobile_opaque_fast@c1e4622ae5df0c87a06c36d803617c7d4e0227845cd33a9dd38de5f0d956ff58 | rayquery_compute_diagnostic_mobile_generic_dielectric@e18171591d4b07ccc3e7fb071400a6581e1fc132aec06dd37c09f88c7792e302`
- Scale: 75%
- GPU timestamp instrumentation: disabled (RT rendering unchanged)
- Evidence type: automated deterministic checkpoint/replay evidence; visual quality and perceived spatial audio remain hands-on checks.
- Result: PASS

## Timing classification

Descriptive references: 16.667 ms ~= 60 FPS, 20.000 ms = 50 FPS, and 33.333 ms ~= 30 FPS. Crossing a reference line is reported, not treated as an automatic product failure. Compare matched runs and investigate regressions above 15%.



See `timing.csv`, `summary.json`, `logcat.txt`, checkpoint state JSON, screenshots (when requested), and the private Vulkan capability report in this directory.
