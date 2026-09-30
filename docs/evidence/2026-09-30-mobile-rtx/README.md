# Mobile-dielectric RTX subset evidence

Exact committed snapshot `f50c32fe2f9b5de8347f40b0c18399fe32b3a52f`;
restored production traversal, not the rejected first-blocker experiment.
VS2022/x64 Debug, supported Diagnostic + Mobile CMake overrides,
RTX5050 Laptop GPU / driver610.188.0. Native build passes; actual executable
`fd063981f1d91b11b59c1cbca82ed8f5d46ac33da87f9d0deaba2bcb0c482987`
contains the four expected Mobile modules, all validated/disassembled.
[Artifact receipt](artifact-cmake-module-receipt.json),
[containment](containment-debug-mobile.json), [method](run-method.md).

One existing `lantern-glass-production` capture per backend, each honestly
presented from the RT storage image at540x960 /100%. The capture path imposes
High WaterQuality, distinct from the **Mobile dielectric** modules. Pitch-0.32
is the existing resolved limit. This is not phone1080x2235 evidence.

Both complete manifests' entire dielectric counter objects match: interface
budget0 (closed0), volume budget0, mismatched exit0, primary certified recovery20
with reason mask2, TIR166, bounded TIR termination74, finite shadow endpoints6817.
The phone's remaining closed-interface failure1 / recovery80 is **not reproduced**.
Nonzero recovery remains an investigation item, not correctness acceptance.
No budget, diagnostic, material, geometry or pixel tolerance changed.

Main inspected both images and independently verified artifact/log/module/
manifest/PNG hashes and presentation identity. Pixel-only backend comparison:
32/518,400 pixels differ, maximum RGB delta1; zero pixels exceed1. This single
checkpoint is within the unchanged foundation limits (RGB max3 and fraction
over1 <=0.001); **not** a complete13-capture or all-backend parity pass.
[Verifier](verify-paired-capture.ps1), [result](paired-capture-verification.json).
Captured timings include startup/settling and are **not performance evidence**.

The first configure did not launch CMake because plain `cmake` was absent from
PATH; only its tool-transcript error survives. Method records that limitation;
no fabricated raw red log. Retry used VS's bundled CMake and succeeded. Worker
observed capture exits0/0; no raw process-code sidecar was saved. Retained stdout
and complete manifests corroborate capture completion; main did not infer exits
from the startup pre-initialization capability text.

Files below are byte-preserved, with lengths/SHA-256 in [manifest](manifest.json).
PNG originals are LFS-backed; executable, build/cache, source snapshot and OBJ
geometry stay external under `C:/Dev/tmp/horde-mobile-assessment-20260930`.
No private phone media/device activity was read or retained. No phone used.
Shipping/Diagnostic, phone compute, S24/S25 and live/geometric-shadow/changed-image
gates remain separate and open. Audio/haptic manual revalidation: NO (evidence-only).
