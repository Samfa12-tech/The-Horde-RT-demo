# Exact-byte chapter texture reuse

This checkpoint removes duplicate uploads for chest-lid wood/iron and lantern-body iron in the development rescue journey. Every encoded mip must match its provider byte for byte before image creation, including Android ASTC. The packaged arrays, geometry, material factors, UVs, shader binaries and 18-layer ceiling remain unchanged. Legacy Showcase, entry and preview retain their existing layouts.

The chapter's base-color/normal/ORM counts fall from **18/15/15 to 15/12/12**. This makes room for the two terrain families and Kit's family within the approved ceiling; their admission and resource handoff are still incomplete. This change is texture reuse, not zone streaming.

| Evidence | Before | After |
|---|---:|---:|
| Windows encoded PBR mip bytes | 268,435,392 | 218,103,756 |
| Android encoded PBR mip bytes | 41,639,424 | 33,687,312 |
| Windows renderer static texture bytes, including fallback | 274,027,796 | 223,696,160 |
| Windows measured device-local allocations in matched captures | 350,313,072 | 299,956,848 |

The Windows allocation reduction is 50,356,224 bytes on the tested RTX 5050 Laptop GPU. Encoded payload savings and driver allocations are distinct measures. Twelve settling frames do not establish sustained performance; no FPS improvement is claimed. Android payload savings do not prove phone GPU allocation or presentation.

## Matched native RT inspection

Checkpoint `lantern-glass-production` (115), 960×540, Diagnostic High, HIGHER shadows, High water/fire, Standard dust, mist enabled, twelve settling frames. Both native hardware backends reported successful RT presentation. Each backend's before/after image has **zero changed pixels**, with identical shader identities, camera and geometry/material metadata sizes. The captures were opened and inspected; they prove this bounded material comparison, not chapter visual completion.

| Backend | Baseline | Candidate |
|---|---|---|
| RayTracingPipeline | [Before](pipeline-before.png) | [After](pipeline-after.png) |
| RayQueryCompute | [Before](query-before.png) | [After](query-after.png) |

[Machine-readable receipt](receipt.json) retains exact executable and shader SHA-256 values, effective camera/settings, resource counts and completed-frame identities. Baseline Debug executable: `8504463dfa3ee14c1856c407ded7237eb8fd3c05131ef488029f1a87c8d28aeb`. Candidate source is the texture-reuse commit containing this report, based on `3c682619c107464bb1a27b9d781b2850d68e33b2`.

## Validation and retained failures

- Windows Debug and Release build: PASS.
- Three focused CTests in each configuration: PASS (Debug 13.78 s; Release 1.68 s). Actual chapter asset admission remains within 32 High / 31 Mobile primitive limits and 30 static materials. Negative cases reject changed encoded mips, invalid indices, unexpected material families/maps and partial mutation.
- Actual Windows and Android packaged texture checks: PASS for all three pairs, all categories and all eleven mips; every original material route remains byte-identical.
- Android Debug four ABI build, lint and 283 Java tests: PASS, zero failed/errors/skipped. APK `188f901066633dab137bbd959ede47c838a440393409091bb9e1dad44f631d9b`. Held-item attribution/package and complete 1.6.2 asset package checks: PASS. No device installation occurred.
- ABI, native GLB, texture tool, shader manifest/artifact/ownership checks: seven initial passes. QueryCompute compilation initially failed to write a sandbox temporary file; retained locally, rerun separately with ordinary toolchain access. The unchanged QueryCompute check then passed in 42.53 s; all eight selected checks passed across these runs.
- Baseline `lantern-chest-held-high` failed on both backends before this patch: captured torch pixels were 4,890 where the claimed-lantern contract requires none. Baseline `lantern-held-high` also failed (5,654 torch pixels). These failed receipts/images remain under ignored `reports/chapter-texture-alias/before-*`; they are **not** claimed as successful held-equipment acceptance. The independent production chest/glass checkpoint passes unchanged. Investigate the rescue/checkpoint equipment staging separately; do not weaken its assertion.
- An initial local build command used the output filename instead of CMake target `horde_rt_diagnostic_window` and failed before compilation; corrected build passed. No source workaround was applied.

Pending: exact-commit CI, phone runtime, full terrain/Kit integration and two-way resource lifetimes, broader chapter gallery and owner acceptance. Previous playtest acceptance is not transferred to this build. No budget, quality, resolution or deadline was relaxed.
