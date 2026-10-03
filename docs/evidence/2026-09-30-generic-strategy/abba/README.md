# Complete whole-strategy Shipping phone profile

Investigation only; not a production change, optimisation admission or glass pass.
This follow-on directory supplements the frozen [build/image evidence](../README.md)
without rewriting its manifest. Exact source `eafbf8262442a82e0835edf5cf5306d718e64633`,
plus only the documented forced-Generic C++ selection for B. All actual packaged
Shipping shader bytes equal control. Same scene, Mobile, 75%, 1080x2235 internal,
1440x2980 presentation, native RayTracingPipeline, strict ASTC and MAILBOX.
Exact device SM-S948B / Adreno 840 / Android 16, no external cooling reported.

## Results and limits

| Run | Route cycle median / p95 ms | Route GPU median ms | Opening GPU median / p95 ms |
| --- | --- | --- | --- |
| A1 OpaqueFast | 63.3133 / 81.4164 | 52.0154 | 70.696118 / 79.753436 |
| B1 forced Generic | 164.6901 / 184.0694 | 153.2847 | 164.3430425 / 176.924319 |
| B2 forced Generic | 193.5229 / 215.6507 | 182.5988 | 193.3582765 / 211.804161 |
| A2 OpaqueFast | 70.7659 / 94.9492 | 59.6732 | 80.3609875 / 90.465467 |

All 7,352 owning measured-lap completions pass strict admission: presented, valid
GPU timestamps, seven exact identity joins, null compiled-out diagnostics and no
missing/rejected/cancelled frames. All 640 opening rows have the expected strategy.
All opening GPU intervals exceed 33.333 ms. The previous control warmup is excluded
from ABBA statistics. `comparison.json` and full losslessly compressed raw ledgers
retain the calculation and evidence; aggregate statistics are not pooled medians.

Mean-of-run opening GPU medians: control 75.52855275 ms, profile 178.8506595 ms
(+136.80% descriptive); route cycle 67.0396 -> 179.1065 ms (+167.17%). These are
**not causal ratios**: battery temperature ranges differ (A1 28.4->34.9 C,
B1 32.7->42.9, B2 37.8->40.7, A2 36.1->38.3), thermal status spans 0-2 and GPU
thermal power level spans 0-9. Context is sampled, not aligned to each measured
frame. No GPU clocks, steady-state certification, display pacing or input-latency
acceptance are inferred from native-cycle or command-buffer timestamps.

The forced Generic opening image fails the unchanged max3/fraction-over-one0.001
gate (max214, fraction0.036708). Generic includes different opaque spawn offsets,
ordered shadow traversal and compiler treatment: this does not isolate dielectric
transport, dynamic ray counts, compiler occupancy or a single lighting routine.
Its diagnostic full replay timed out at300s; focused capture/Home-resume passed,
not that replay. The Shipping benchmark completes independently and does not
retroactively turn the timeout or image mismatch into a pass.

## Decision / next experiment

Keep OpaqueFast on the ordinary production scene. Forced Generic remains excluded
from production. Large whole-path cost is a profiling priority, not permission to
remove physical glass/shadows or promise that this entire difference is recoverable
in a lantern scene. Next isolate compiler treatment on identical OpaqueFast source:
retain its material routes, shadow maths, origins and budgets, changing only
compiler optimisation/inlining. Compile/disassemble first; any future phone trial
needs exact APK provenance and unchanged image/correctness admission. No estimated
net saving exists yet. Other glass/interface/recovery/backend gates stay open.

## Provenance / restoration / CI

A APK `ab3e2261fd081f87e667a6e4967e2476077fa96554702330e6aa49baa8133eae`;
B APK `6c2621dc2b233f339773d1a686ad832cf16c875e3ea7a7718c70d08e9ba9fc2f`.
Every install was pulled back byte-for-byte. Optional Play Protect upload prompts
on A1/A2 were declined for the known validation APK; no global security setting
changed or APK upload authorised. Installation delays precede timing and are not
counted. Stable app/data were untouched. A2 restores normal Shipping control.
Normal Debug389d6954 is also restored/pulled back identically: focused opening
run20260930-234438 passes capture/Home-resume and honest native RT presentation.
Its scene-only PNG hashd3ec98a838e61194c340343ae4e293aea0ce8b39697962dbac17e134c62dbbcf
equals the existing same-artifact control; no duplicate PNG retained. This is not
a new full replay, glass acceptance or build-from-current-head claim. Runner
sourceCommitb56 metadata identifies the harness checkout, not reused APK provenance.

CI receipts apply to pushed `b56d1bfb3eccbac31550a8b0960e706f9d061455`, not this
future documentation commit: push36721846883/PR36721855646 actual logs both pass
46 portable and 11 Vulkan CPU-host tests, plus supplemental guards. Host coverage
does not prove device RT, glass, music playback or a complete candidate.
Audio/haptic manual revalidation: NO (no source/event/playback change).

`SHA256SUMS.json` binds exact disk/Git bytes and decompressed originals. The method
copies only the four explicit runs and current CI, no APK/ELF, signing material,
private media or synthetic fixture masquerading as a measured run.
