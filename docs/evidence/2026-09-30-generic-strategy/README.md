# Whole-Generic strategy: same opaque opening investigation

Status: investigation only; **not a Shipping optimisation or Phase 4 acceptance**.
Exact device: SM-S948B / Adreno 840, 75%, native RayTracingPipeline, strict ASTC.
No external cooling. No material, geometry, shader, budget or player changes.

The isolated profile is detached `eafbf8262442a82e0835edf5cf5306d718e64633`
plus [one forced-strategy assignment](probe/force-generic-investigation.patch).
The exact non-debuggable Shipping/Mobile ARM64 APK is
`6c2621dc2b233f339773d1a686ad832cf16c875e3ea7a7718c70d08e9ba9fc2f`;
Diagnostic/Mobile ARM64 is `6ec1d66cad09eb50a3ce9340a2d2031e259f477f13a949be0e4ce98cdc6f1d96`.
Gradle succeeds with both targets; each uses the development certificate.
No signing recovery or publication occurred. Checkout metadata in capture
manifests is **not** these reused/derived APKs' source provenance.

[Build receipt](provenance/profile-build-receipt.json) and independent lead byte
verification bind source/patch/APK/log bytes and four actual Shipping SPIR-V modules.
All four modules equal the frozen control exactly, validate/disassemble, and contain
zero diagnostic atomics/image reads/binding22. Of53 assets,50 are byte-identical;
licence text changes only line endings and two manifests only JSON formatting.
No asset/material semantic replacement is hidden in the profile.

## Image and run results

Fresh exact-control Debug389d6954 passes13-waypoint replay, opening capture and
Home/resume. Opening PNG equals prior validated bytes (`d3ec98a8...`).
The forced Generic13-waypoint replay hits the existing300s observation timeout;
the harness then force-stops the app. Retained native state/log show incomplete
replay, not a replay pass or evidence of an app crash. A separate focused
capture-only invocation of the existing runner (Benchmark mode, empty checkpoint
timing list) passes the opening capture and Home/resume, **not** route validation.

Both captured authored scenes agree, including accepted modelled-viewmodel,
grips, camera, lighting controls, resources and native RT presentation. Owning
frame packets prove control OpaqueFast versus profile GenericDielectric. The
first35 dielectric counters are zero in both opaque opening captures.

[Unchanged pixel-only comparison](opening-image-comparison.json) **fails**:
780,444/4,492,800 pixels differ;164,922 differ by more than1RGB,
fraction0.036708066, maximum channel difference214. Limits remain max3 and
fraction-over-one0.001. Lead viewed both images; broad composition is retained,
but visual resemblance does not substitute for the failed pixel gate.

[Source seam review](provenance/strategy-seam-review.md) identifies Generic's
bounded ordered shadow traversal on opaque receivers and different secondary
origin handling; material-specific glass reflection/refraction is not entered
for ordinary opaque hits. Upcoming timings measure the **whole strategy**, not
isolated compiler occupancy, dynamic ray counts, pure glass cost, or a
pixel-equivalent production candidate. No faster result can admit this override.

Control warmup completes1,838 owning Shipping rows (160opening), all OpaqueFast,
null compiled-out diagnostics and valid GPU results. It is context-only and
explicitly excluded from the matched A/B/B/A statistics. Strict parser rejects
wrong APK, wrong strategy and missing completion identity; isolated comparator
fixtures verify positive admission and reject an extra strategy property.
Synthetic fixtures are not phone measurements. Full matched A/B/B/A is **open**
at this checkpoint; no gain,30FPS, steady-state or display-pacing claim.

Installation temporarily waited for Play Protect's optional APK-upload prompt.
The lead selected “Don't send” for the known validation artifact; no global
security setting, app data or stable installation was changed. Exact APK
pullback remains required before each trial. The Shipping runner's external
observation deadline is30minutes for this slow profile; frame counts, workloads,
physical traversal budgets and evidence admission are unchanged.

## Retention and boundaries

The manifest binds every curated evidence file's raw bytes; bulky owning-frame
JSON is losslessly gzip-compressed with its source digest also retained.
Game-scene captures only: no personal media, general phone dump, APK/native
binary, paid generation or production credentials included. Compiler-reference
scanner mismatch is worker-observed; the retained actual eaf scanner validates
both packages. Pixel verifier initially lacked .NET10 drawing references; those
failures are tool-transcript observations, not retained raw failed-command logs.
The corrected verifier and green receipt retain unchanged pixel limits.

Phone compute, S24/S25, glass1closed-budget/80recovery witnesses, live glass,
full backend parity and sustained30FPS remain open. Music and all remaining
programme work stay in scope. Audio/haptic manual revalidation required: **NO**
(no audio, semantic feedback or haptic changes in this investigation).
