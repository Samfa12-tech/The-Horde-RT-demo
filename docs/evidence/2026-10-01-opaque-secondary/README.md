# Opaque-primary secondary-omission profile

This packet preserves a matched four-position C1/S1/S2/C2 investigation on the normal control and a reviewed Shipping/Mobile isolate that omits opaque-primary secondary bounce/reflected-fire work. The isolate is deliberately nonphysical. Its measurements are a workload cost-bound probe only—not an optimization candidate, additive ray cost, causal estimate, or visual/physical-quality acceptance.

## Results

Eight strict Shipping/Mobile runs on SM-S948B at 75% scale: each showcase route has 1,838 completed/presented frames; each lantern-reveal sequence has 600. The aggregate counts all 9,752 owning rows. Values below are medians / p95 per run in milliseconds; group cells are medians across the two run-level statistics, not pooled frames.

| Workload | C1 control | S1 isolate | S2 isolate | C2 control | Control / isolate group median |
|---|---:|---:|---:|---:|---:|
| Route whole-frame cycle | 59.10 / 79.36 | 42.53 / 55.82 | 41.09 / 54.20 | 70.85 / 94.93 | 64.97 / 41.81 |
| Route opening GPU command-buffer interval (160 rows/run) | 66.55 / 73.47 | 40.91 / 46.68 | 38.11 / 46.44 | 80.80 / 88.43 | 73.68 / 39.51 |
| Live whole-frame cycle | 235.06 / 248.69 | 288.14 / 312.55 | 314.08 / 334.99 | 270.23 / 295.57 | 252.64 / 301.11 |
| Yellow-torch-bay GPU command-buffer interval (600 rows/run) | 223.39 / 237.52 | 275.97 / 300.44 | 302.41 / 324.46 | 258.07 / 284.81 | 240.73 / 289.19 |

Descriptive cycle-median deltas are route −35.65% and live +19.18%. Route opening frames at or below 33.333 ms: control 0/320, isolate 23/320. Live focus frames at or below 33.333 ms: 0/1,200 in both groups. GPU-zone values are whole GPU command-buffer timestamps; they include AS work, dispatch and copy, not isolated secondary shading.

## Context and limits

The thermal context is process-sampled rather than frame-aligned. Battery temperatures rose C1 route 25.4→32.7°C, S1 route 36.5→37.4°C, S2 route 41.8→42.8°C, C2 route 41.5→40.9°C; live runs were 32.9→37.2, 37.4→41.9, 42.8→43.4, and 40.8→40.9°C respectively. Route C1/S1/S2/C2 thermal statuses were 0, 1, 2, 2; live were 0–1, 1–2, 2, 2. GPU thermal-power levels and per-run PIDs are preserved in the aggregate. The run order, thermal drift and process changes limit comparison.

The control APK SHA-256 is `a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033`; isolate APK SHA-256 is `ce1c1548ed3937d1ab99b57c523b62a8a1e244538c01e9075bb2f7e56e766cce`. APK and native-library binaries are intentionally omitted: these are receipt identity pins and are not freshly byte-rehashed by the portable archive verifier. Source commit is `71cb366c5cbe5cf6fe338c4cd6fd7bec0bd12d95`.

The isolate removes opaque-primary secondary bounce/reflected-fire contributions. Do not infer causal or additive savings, production suitability, preserved image quality, 30-FPS acceptance, display pacing, or sustained performance. The verifier rehashes curated files, reparses raw ledgers, joins their analyses, and checks the exact packaged SPIR-V hashes against the archived catalogs and module receipts. It can rerun SPIR-V validation/disassembly when the tools are installed. A retained synthetic negative fixture confirms that a physical/production classification is rejected; this is not phone evidence.

## Lead integration and reproducibility

The original external curated-v5 packet is unchanged. Its exact manifest is
retained at `curation/origin-manifest-v5.json`, SHA-256
`67a3f8e47cd4c1afc3a0cd63fc2891f994388a6ae8a77a65ff9834ac5fdf86c7`.
This repository archive adds the lead's verification/replay logs and this note,
then reseals its SHA manifest. Eight fresh canonical v2 reparses of the archived
raw inputs reproduce every analysis field except the relocated raw-directory
path; the fresh aggregate reproduces every field except its creation timestamp.
9,752 owning rows; no original report/receipt overwritten.

`verify-curated-opaque-secondary.ps1 -Root <this directory>` is the portable
offline archive/hash/row/module check (PowerShell7; optional Vulkan tools).
Original methods are retained source evidence: their canonical-layout APK and
external receipt dependencies are not magically made portable by archiving.
The independent canonical replay additionally rehashed the retained local APKs;
the portable verifier correctly makes no such claim for omitted APK/native bytes.
Generated partial-packet verifier/path failures and the corrected synthetic-test
hardlink mistake are disclosed in `curation/initial-verification-failures.json`;
original phone/source receipts were untouched. The physical-label negative test
is synthetic, not a successful glass/quality gate.

Decision: no promotion. Ordinary opening improves descriptively but remains above
the30FPS GPU budget; live lantern gets slower descriptively. Whole-buffer GPU
timestamps cannot assign that change to an individual bounce or geometry update.
Keep the negative/mixed result; investigate a combined, quality-preserving native
Mobile schedule rather than force this omission into Shipping. RTX/compute,
S24/S25, physical glass and final acceptance are not certified here. Audio/haptic
manual revalidation required:NO; semantic inputs/playback are unchanged.
