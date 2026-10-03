# Android showcase automation run 20261001-032259

- Mode: Benchmark
- Device: SM-S948B (Android 16 / API 36)
- Debug APK SHA-256: `14b352ebd85d573f2af2f571a2cd506b4c3ad4da9787787553a34218d816f0d3`
- Installed base APK SHA-256: `14b352ebd85d573f2af2f571a2cd506b4c3ad4da9787787553a34218d816f0d3` (exact match)
- Source: `71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95` with a dirty worktree recorded
- Selected RT pipeline pair: `diagnostic_mobile_opaque_fast@8dbebd86eeed23b388626719e6f9946b8027df754869b5811673e24036ae12c9 | diagnostic_mobile_generic_dielectric@1252cf30195e906b7e8643fb3ed2ed39a1b50df83818696ac08ae959acb750f7`
- Scale: 75%
- GPU timestamp instrumentation: enabled (RT rendering unchanged)
- Evidence type: automated deterministic checkpoint/replay evidence; visual quality and perceived spatial audio remain hands-on checks.
- Result: PASS

## Timing classification

Descriptive references: 16.667 ms ~= 60 FPS, 20.000 ms = 50 FPS, and 33.333 ms ~= 30 FPS. Crossing a reference line is reported, not treated as an automatic product failure. Compare matched runs and investigate regressions above 15%.

- 75% lantern-glass-production [authored-standard]: 230.664 ms, BELOW 30 FPS REFERENCE

See `timing.csv`, `summary.json`, `logcat.txt`, checkpoint state JSON, screenshots (when requested), and the private Vulkan capability report in this directory.
