# Android showcase automation run 20261001-031518

- Mode: Benchmark
- Device: SM-S948B (Android 16 / API 36)
- Debug APK SHA-256: `0ec752303b30a5b05dd45bdafc888f18b55c97918ac30439ebc8d090cbaf789a`
- Installed base APK SHA-256: `0ec752303b30a5b05dd45bdafc888f18b55c97918ac30439ebc8d090cbaf789a` (exact match)
- Source: `71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95` with a dirty worktree recorded
- Selected RT pipeline pair: `diagnostic_mobile_opaque_fast@8dbebd86eeed23b388626719e6f9946b8027df754869b5811673e24036ae12c9 | diagnostic_mobile_generic_dielectric@9865690c11de315c1c4e49af7d14eb8963b24dc522abfc01c53979bb1a5f5a91`
- Scale: 75%
- GPU timestamp instrumentation: enabled (RT rendering unchanged)
- Evidence type: automated deterministic checkpoint/replay evidence; visual quality and perceived spatial audio remain hands-on checks.
- Result: PASS

## Timing classification

Descriptive references: 16.667 ms ~= 60 FPS, 20.000 ms = 50 FPS, and 33.333 ms ~= 30 FPS. Crossing a reference line is reported, not treated as an automatic product failure. Compare matched runs and investigate regressions above 15%.

- 75% lantern-glass-production [authored-standard]: 230.85 ms, BELOW 30 FPS REFERENCE

See `timing.csv`, `summary.json`, `logcat.txt`, checkpoint state JSON, screenshots (when requested), and the private Vulkan capability report in this directory.
