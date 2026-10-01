# S24 bounded primary-hit discriminator

Status: compiled/built, device results pending. Normal control source0cf4f05; exact
normal Debug8CB APK and four existing captures are retained in the
[visual follow-up](evidence/2026-10-01-s24-visual-followup/RESULTS.md).
Do not repeat the finished opacity/performance trial or tune accepted arms.

Question: do the normal primary hardware ray queries commit player TLAS hits
before hit decoding, or does metadata-based classification lose those hits?
The original primaryPlayerPixelCount is not independent of metadata.

This separate investigation branch modifies Diagnostic/Mobile/OpaqueFast compute
only, adding seven atomic instructions at the committed-hit boundary before
instance/material/vertex loads. It adds no ray query, ray path, buffer, traversal
budget, geometry/mask/material/animation or shader-quality change. High, Shipping
and the pipeline launcher must have unchanged SPIR-V. Reuse the existing fence-
owned Diagnostic SSBO and capture/native-state harness, not a new telemetry ABI.

**This APK's remapped counters are not physical glass diagnostics.** Do not run
glass gates or performance comparisons against it; never promote the mapping.

| Native state field | Investigation meaning, primary queries only |
| --- | --- |
| primaryOpenOpaqueTerminalInstanceMask | Actual committed TLAS instance-ID bits, clamped31 as overflow marker |
| primaryOpenOpaqueVolumeInstanceMask | Committed custom-index bits, clamped31 as overflow marker |
| primaryMismatchedExitCount | Count of actual ID != custom index |
| primaryOpenMissCount | Actual viewmodel instance20 committed pixels |
| primaryOpenOpaqueCount | Actual world-body instance4 committed pixels |
| primaryInterfaceBudgetCount | Actual skeleton pose0 instance2 committed pixels |
| primaryVolumeBudgetCount | Actual skeleton pose1 instance18 committed pixels |

Slots4/20/2/18 are the current named CPU registry's world-body/viewmodel/skeleton
instances, not a production raw-slot redesign. The original metadata-based
primaryPlayerPixelCount remains intact for comparison. Normal rendering and
the existing primary candidate filtering remain unchanged; zero commits would
not distinguish AS/masks from that filtering without another discriminator.

Finite run matrix: compile/validate eight compute modules once, confirm only the
one intended module's SPIR-V changes; build one Debug APK; pull/install match on
SM-S928B; capture two-enemy-combat and player-viewmodel-lantern-high at75%, same
native scene/pose/settings; compare with already retained normal images. Record
the two raw masks and four counts plus metadata-based count, RT presentation and
actual backend. Home/resume and restore normal8CB with data preserved afterward.
No S26 timing sweep, S24 performance claim or owner listening test.

Fresh eight-module compute generation/spirv-val passes. Exactly one SPIR-V
changes: Diagnostic/Mobile/OpaqueFast506304bytes (+796),12atomics (+7), still
23ray-query sites. SHA256d466e66ee4ecfb3a2f7ee2d22337eb811c8744802ac5076fe7e9176792805bac.
All seven other compute modules, including Shipping/High, have identical SPIR-V
hashes to0cf4f05. Shared source is preprocessor-inactive for the pipeline launcher;
its embedded artifacts are untouched, not claimed as freshly recompiled here.
Android Debug four-ABI build succeeds (1m16s). Frozen APK:
`C:/Dev/tmp/horde-s24-instance-hits-20261001/instance-hits-debug.apk`,
SHA256dd69eeb7947d631450780665324380766a85d3d03f42fba661ca21523ac0183c.
Logs live alongside it; no release/signing/quality override was used.

Next unfinished step: exact two-capture S24 run, raw-mask/count interpretation.
Audio/haptic manual revalidation required: **NO** (unchanged semantic playback).
No main merge, release or publication authorised.
