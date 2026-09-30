# Exact first-blocker trial: no demonstrated phone gain

Phase 4 remains open. The single first-confirmed-opaque-blocker optimisation
iteration was not admitted to production. Preserve accepted glass/player work;
do not restart tuning or interpret this as a completed performance pass.

## Change and admission

Implementation checkpoint `e12aabe` added `TerminateOnFirstHit` only to the
binary `visibilityMask` query, after the existing transparent admission. Masks,
ray bounds, materials, geometry, fire volume, ordered dielectric traversal and
budgets were unchanged. The lead subsequently restored the previous flags:
semantic plausibility and unchanged images do not demonstrate a phone gain.
The stronger binary admission/mask/bounds test remains, with a guard against
reintroducing this unaccepted flag. See [decision](DECISION.md).

Frozen control APK SHA-256:
`ab3e2261fd081f87e667a6e4967e2476077fa96554702330e6aa49baa8133eae`
(export-only source `eafbf826`, shader bytes equal the accepted control).
Frozen candidate APK:
`0383edcad0ffb0ab9e116ff5df7f8e20bbdb4cdca1f06a30986709257e42387e`.
The candidate was built from `3e5bd64` plus the retained reviewed patch, not
rebuilt from a falsely labelled clean later commit. Host compatibility-pin
maintenance followed separately; see [provenance notes](artifacts/provenance-notes.md)
and [full recipe](artifacts/provenance.json).

Actual extracted Shipping/Mobile pipeline and compute modules validate and
disassemble: zero diagnostic atomics, image reads or binding 22. All 53 APK asset
payloads are byte-identical. Each OpaqueFast pair differs at exactly 17 raw words
and 17 normalised instructions: only the ray-query flag operand 2 to 6 at 17 of
23 initialisation sites. No constant-definition or other instruction change;
both GenericDielectric modules are byte-identical. Word/instruction counts are
unchanged. These are static sites, not measured dynamic query counts. See
[sealed flag audit](ray-flag-audit/results-sealed/ray-query-flag-audit.json) and
[asset/module containment](artifacts/benchmark/containment.json).

## Uncooled A/B/B/A Shipping results

Exact SM-S948B/R5GL219SZGK, Android 16, Adreno 840/driver 2150932499,
Shipping/Mobile, native RayTracingPipeline, strict ASTC, MAILBOX,
75% internal 1080x2235 to presentation 1440x2980. Existing route/warm-up protocol,
no recording, new timer queries, frame-work deletion or quality reduction.
Every run admits 1,838 completed/presented rows with valid owning GPU timing,
seven-field identity joins and compiled-out diagnostics; all route rows use
OpaqueFast, including 160 opening rows per run. Total: 7,352 rows/640 opening.

| Run | Route median / p95 ms | Opening GPU median / p95 ms | Battery C | GPU power levels |
| --- | ---: | ---: | ---: | --- |
| A1 control | 70.0435 / 93.2592 | 80.154399 / 86.997029 | 36.4 to 38.3 | 0,2,3,4,5,7 |
| B1 candidate | 79.2882 / 106.2341 | 91.234555 / 105.424268 | 35.4 to 36.4 | 0,4,5,6,7,8 |
| B2 candidate | 78.1312 / 105.9836 | 90.7692945 / 104.147185 | 36.3 to 36.3 | 0,4,5,6,7,8 |
| A2 control | 77.2206 / 103.6876 | 89.4686175 / 98.368123 | 35.9 to 36.3 | 0,4,5,6,7,8 |

Mean of route medians: control 73.63205 ms versus candidate 78.7097 ms (+6.90%
descriptive). Primary opening GPU statistic: 84.81150825 versus 91.00192475 ms
(+7.30%). A1/A2 drift and changing power levels prevent a causal effect estimate;
this is neither a proven 7% regression nor a gain. No further tuning iteration
is justified by this trial. See [comparison](comparison.json) and
[admission script](compare-trials.ps1).

Owner's cooling-removal report was observed at 09:11:04 UTC; actual removal/thaw
time is unknown. These runs start after the report and are all uncooled. Do not
pool earlier externally cooled isolates. Thermal context covers run observation,
not individual measured frames; A1 thermal statuses 0/1, the others 0.
No GPU-frequency evidence or steady-state thermal certification is claimed.

All 640 opening GPU intervals exceed 33.333 ms. Route median-derived rates are
12.6 to 14.3 FPS, not actual display cadence. Render-cycle timing is not display
pacing/input latency; GPU command duration includes AS/RT/copy. The 30 FPS
minimum at 75% remains a target, not an achievable-performance claim. The route
contains no reward lantern and does not replace glass-heavy measurements.

## Validation and separate gates

Candidate focused MSVC Debug/Release CharacterRenderSlotSmoke: 1/1 each PASS.
Compiler strategy, variant manifest, eight-compute freshness and full
artifact/negative/compatibility contracts PASS. Retained earlier failures are
stale compatibility pins and an incorrect build target, not suppressed checks.
Fresh `e12aabe` push and PR CI both PASS: 45 portable/11 Vulkan-host CTests,
10 APK resolver/34 Android combat/16 ownership fixtures. [CI receipt](ci-logs/ci-receipt.json)
records exact heads/log hashes; these are host checks, not physical RT acceptance.

After rejection, all 16 freshly generated pipeline/compute payloads equal the
independent control catalog and checked-in control parent. Compatibility,
eight-mode catalog/artifact freshness, compiler strategy, manifest, artifact
negative tests and Debug/Release CharacterRenderSlotSmoke pass. See
[restoration receipt](restore-control/restore-receipt.json). The original focused
Debug CTest batch was 4/5, not 5/5: the compute wrapper incorrectly asked the
generator to stage inside an in-repository build preset. The generator correctly
rejected it. Removing that CMake argument restores the validator's existing unique
external temporary directory; the generator guard is unchanged. Reconfigure and
the corrected CTest pass 1/1 in 33.51 seconds; [red/green receipt](restore-control/cmake-wrapper-repair.json).
No new-head CI result is inferred from the earlier `e12aabe` jobs.

RTX pipeline: all 13 standard PNGs byte-identical to accepted d63 images.
Compute's 13 capture set completed but differs from pipeline; no fresh
same-backend compute reference was established here, so this does not attribute
the difference to this candidate or pass backend parity. Existing tolerance
stays unchanged. See [pipeline comparison](windows-pipeline-image-comparison.json).

Exact phone Diagnostic/Debug APK
`5b93c3497953d1c938450352f32b9dd0f4a733c41f99b0fe257d21ac180b46c9`
passes replay/nine scene captures/Home-resume at 75% with strict ASTC/native RT.
Opening/worst-bend equal fresh exact-control images; seven glass PNGs equal the
previous exact-artifact counterparts. See [image comparison](phone-image-comparison.json).
No player tuning or new subjective owner-acceptance claim. Frozen images do not
replace glass live-motion/correctness, changed-pixel attribution or physical
shadow attenuation acceptance. Shipping/Diagnostic parity remains distinct from
pipeline/compute parity. S24/S25 and phone compute remain unverified.

The normal immutable Diagnostic APK `389d6954f7a3fddde1ced690470029fa4b14cc6f825fa34bf7269dc617f946e0`
was also restored in `.debug` after the Shipping sequence. First attempt
`run-20260930-204031` timed out at the original 120-second route deadline; the
retained filtered native log shows progress, not a demonstrated crash. Retry
`run-20260930-204834` uses the existing 300-second option and passes replay,
scene-only isolated-lantern capture and Home/resume. No route, count, image,
physics or performance gates changed. The runner's `e12aabe`/dirty metadata
describes its checkout, not a new clean build of this reused immutable APK.
[Offline comparison](debug-restoration-comparison.json) independently verifies
APK identity, owning completion and exact historical PNG equality. The current
epoch6/generation4/submission1866/tick1873 capture retains one closed-volume
interface-budget failure and 80 reason-2 recoveries. These aggregate witnesses
remain open; historical path probes do not explain the current pixels.

[Parser receipt](parser-tests.json): actual B1/B2 positives retain
`quality-preserving-candidate-unaccepted`, gain `none`, acceptance `not-evaluated`.
Three disposable-copy negatives reject wrong shader identity, invalid owning
strategy and mismatched completion submission serial. Copies are not runtime
evidence; immutable raw source reports are untouched.

## Preservation

The exact normal control was reinstalled/pulled back byte-identically before A2;
no B2 reinstall, stable-app mutation or data clear. Evidence is an explicit text
allowlist, with the four full benchmark JSONs losslessly gzip-compressed.
The additive SHA-256 manifest covers curated bytes and each decompressed raw
hash/length. No APK/ELF/SPIR-V binaries, huge disassemblies, private video/Home
frames, synthetic raw copies or unrelated scratch are included.
Audio/haptic manual revalidation: NO (semantic/playback inputs unchanged).
Music/reporting/resource/pacing/final matrix and the deferred live benchmark FPS
counter remain in programme scope. No publication, signing or licence action.
The owner disconnected the phone after the restoration retry. Do not operate or
recoordinate it until the owner reports reconnection. Continue source/host/RTX
profiling and quantify dedicated Mobile native-RT options before implementing
another optimisation; current phone timings do not support a sustained 30 FPS
claim at 75%.
