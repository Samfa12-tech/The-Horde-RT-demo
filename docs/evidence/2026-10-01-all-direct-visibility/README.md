# All-direct-visibility cost investigation: no production promotion

October 1, exact SM-S948B / R5GL219SZGK, Adreno840. Twelve uncooled Shipping/Mobile
pipeline runs at unchanged75%,1080x2235 internal/1440x2980 presentation, strict ASTC,
MAILBOX. Source71cb366; later CI/documentation commits do not change these shaders.
Each block runs ordinary showcase route, held-high lantern, then live reveal;
blocks are C1/S1/S2/C2. S2 continues S1's process; C1/C2 are separate normal processes.

Control APK SHA256 `a6329657e585e9605098e667fc07fa1ef278626a4242f81a94f9f79d1d3cd033`.
Isolate APK SHA256 `b57abb9127929665102a9de70b44085d6c469aeeee10fb016f09d6ffbd6d4829`.
Install/pullback receipts verify C1/S1/C2 byte identity. Both are non-debuggable,
development-signed `.benchmark` artifacts, not signed/public release candidates.
Normal control is retained after C2; the harness issues Home. Normal DebugC11 was
already restored separately; stable app, app data and personal phone media untouched.

## Deliberately omitted work

The retained patch returns unity from opaque visibility and GenericDielectric
shadow transmittance for Shipping/Mobile only. It removes **real geometric shadows
and glass shadow attenuation**. This is nonphysical and must never ship. Primary/
secondary traversal, geometry/materials, Fresnel/refraction/IOR/TIR, budgets, fire
volume, player ownership and C++ timing code are unchanged. All53 asset payloads
are byte-identical; only the packaged ARM64 library differs.

Four Shipping/Mobile shader modules change, all12 other generated modules retain
their hashes. Eight actual packaged modules are retained as SPIR-V and independently
validate/disassemble: zero diagnostic atomics, zero image reads, no binding22.
Pipeline opaque words125253->118525, functions1->1, static query sites23->6;
Generic words60963->57093, functions63->60, sites2->1. Compute equivalents are
in the audit. Static sites are **not dynamic ray counts**. Compiler footprint/
occupancy changes confound a simple subtraction of shadow-ray cost.

## Completed measurements

All12 strict analyses pass:12,152 owning, presented, GPU-valid rows, zero rejected/
cancelled/outstanding rows, every diagnostic row compiled out. All640 ordinary
opening rows actually execute OpaqueFast. The route has1,838 measured frames per
run; held/live each600. Completion/submission identities, exact loaded shader pair,
APK/source/extent/encoding, monotonic GPU serials and denominators are checked.

Entries below follow C1/S1/S2/C2 order. Opening values are opening-zone statistics,
not the whole-route median. GPU is the **complete GPU command-buffer interval** in
that zone, including AS work, RT and copy; it is not isolated torch/shadow cost.

| Workload | Native cycle median ms C1/S1/S2/C2 | GPU median ms C1/S1/S2/C2 | GPU p95 ms C1/S1/S2/C2 |
| --- | --- | --- | --- |
| Ordinary opening |81.8631/42.2540/41.8268/82.2171|71.40/31.30/30.86/71.18|80.28/33.20/33.08/80.10|
| Held-high lantern |225.0650/58.0105/58.1393/240.3811|223.18/56.22/56.32/238.45|238.20/58.00/58.19/256.32|
| Continuous live reveal |242.9220/65.7211/65.8856/249.9314|231.91/56.34/56.41/238.32|254.21/58.75/58.87/240.85|

Control opening0/320 GPU frames meet33.333ms; isolate307/320. All control and
isolate held/live GPU frames exceed33.333ms:0/1200 in each group/workload. This
does not measure compositor/display pacing. Opening isolate cycles remain roughly
42ms; the whole-route33.5085ms median-of-run-medians is **not opening30FPS**.
Live skin CPU medians8.53-8.69ms remain; held-high freezes skin and has zero median.
CPU BLAS/TLAS record timings cannot be relabelled GPU AS timing.

Battery temperatures span36.4->43.5C; thermal statuses1-3, differing GPU power
levels including7 in C2held-high. Context sampling starts after launch observation
and is not measured-lap/frame aligned. No external cooling. Preserve per-run
context/order/process IDs; ABBA does not erase thermal drift or compiler changes.
The aggregate reports medians of per-run statistics, not pooled-frame percentiles.

## Decision and remaining gates

No optimisation gain or visual acceptance is claimed. Removing **all** direct
visibility still leaves roughly31ms ordinary/56ms heavy GPU and42/66ms live native
cycles. Direct-light reduction alone is not a credible30FPS proposal for this
workload. A dedicated Mobile RT schedule must also address residual primary/
secondary/material/fire/dispatch cost, with measured eligibility and overhead.
See the [quantified feasibility assessment](../../ENGINEERING_1_6_1_MOBILE_RT_FEASIBILITY_2026-09-30.md).
Source ceilings in `mobile-source-census.md` do not prove per-frame active counts.

The true fifth-interface witness, physical-shadow/live acceptance, corrected-
transport image attribution, Shipping/Diagnostic parity, and pipeline/compute pixel
divergence remain open. No budget lift, diagnostic suppression, fake/scalar glass,
tolerance change, player tuning or material/geometry change is promoted. This is
phone pipeline evidence, not RTX, phone compute or exact S24/S25 certification.
Audio/haptic manual revalidation required:NO (unchanged semantic inputs/playback).

## Archive and reproducibility

The archive contains179 explicit copied/hash-checked files plus this report,
fresh offline checks and the SHA manifest. It excludes APKs/native libraries,
full detached source, personal media, redundant synthetic reports and incomplete
aggregates. Actual SPIR-V can be revalidated/disassembled with Vulkan SDK tools;
do not import investigation catalogs into production. Original paths/receipts
remain exact, so build/audit/install scripts describe this machine and are not
portable installer commands.

`verify-offline.ps1 -WorkDirectory <new-external-directory>` revalidates all eight
retained modules, reparses all12 raw ledgers and compares the aggregate, without
phone access or modifying this evidence. It refuses existing output directories.
The retained synthetic7-case regression receipt proves
parser behaviour, not device evidence; its script explicitly depends on earlier
historical fixtures outside this archive. Frozen raw owning ledgers remain here.
Initial generator/build RED logs and green retries are retained where available;
some initial stdout paths named by build receipts were not retained by the builder.
Final actual-module extraction/hash/val/dis proof does not claim those missing logs
or an earlier containment-wrapper failure passed.
