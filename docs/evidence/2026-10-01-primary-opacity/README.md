# Primary-opacity admission: bounded evidence

Experimental source e78028c999e745e7cfd1695468062bb45f300b21 is separate from
normal1e20f438. See the [single experiment record](../../ENGINEERING_1_6_1_PRIMARY_OPACITY_2026-10-01.md)
for immutable APK identities and the finite Shipping matrix. No automatic
promotion, quality reduction, main merge or publication.

## Completed image admission

- RTX Mobile Diagnostic:13 pipeline and13 compute checkpoint images are
  byte-identical. Existing pipeline comparator still **fails** its unchanged2%
  capture-timing gate at+4.766%; compute−0.963% passes. The two raw receipts are
  under `windows/`, along with all26 pipeline control/candidate PNGs and all four
  capture manifests. Compute PNGs have the same pixel-equality comparator;
  full raw compute images remain in the stated scratch root. Image equivalence is not a Shipping speedup or full foundation
  pass. Actual GPU is RTX5050 Laptop; outputs960x540, not phone dimensions.
- Exact SM-S948B75% Diagnostic pipeline: control222521/candidate223006 each has
  six frozen captures, strict ASTC, honest native RT presentation and Home/resume.
  All six PNGs are byte-identical. `s26-control/` and `s26-candidate/` retain the
  actual capture manifests, all12 PNGs and native states. Full raw logs are retained at
  `C:/Dev/tmp/horde-primary-opacity-20261001/phone-images/`. Native mutable serials
  and skin-update counts are not pose differences. The candidate was compiled and
  captured before the source commit; its raw manifest truthfully records6fa1c53
  plus dirty worktree, exact APK6DA2BCCF and candidate module hashes.
- Exact SM-S928B75% Diagnostic compute: two selected candidate223947 captures
  under `s24-candidate/` are byte-identical to normal221511 combat/high-lantern
  captures. PNG hashes respectively2f59f7dd65953ea2dedb9417ef6b3c88e7a2baf443962491580d9f4569d731bb
  and d6fe26c2557867bba69a1e8fb0ecd1737c0f3f37d3a6a36328237f48c88d673a.
  Both report12 stable RT frames and zero primary-player pixels: missing hands
  remains open; two native combat enemies are not distinguishable in this saved
  view. This experiment neither fixes nor diagnoses that defect. Normal8CB Debug
  was restored with replace/allow-test, pulled back with identical full SHA256,
  and Home confirmed; data and stable package untouched. No S24 timing rerun.

## Performance and memory

All eight Shipping rows complete. Reviewed analyser passes9752 measured-lap2
CPU/GPU submission/presentation joins, four1838-frame routes and four600-frame
held-high workloads, actual OpaqueFast, fixed pipeline/Mobile75%1080x2235 and
unchanged assets. Candidate e78028c; control6fa1c53 (prose-only ancestor of1e20f438).
The complete native reports, result markers, identities,294 thermal samples,
timed active RAM/PSI/system-memory context and before/after PSI are under `phone/`.
No captured video or data clearing. `opacity-matrix-analysis.json` is integrity
only, original SHA2565ba4b1eb23b13d7aca060ffceb83234c0389a1a36b189a69b034f42b54965fb5.
Its active/boundary PSS/RSS summaries retain the original own-app snapshots here;
they are not GPU counters or a leak/plateau pass. One wrong-source-identity
negative analyser check rejects before writing output; no device rerun.

Lead decision: **NO-GO for promotion**, not proof the shader is intrinsically
slower. Descriptive means of run medians: opening cycle82.277→85.785ms (+4.26%),
GPU71.497→74.751ms; held-high cycle104.664→100.962ms (−3.54%), GPU102.992→99.114ms.
Route cycle63.497→66.155ms (+4.19%). The first heavy pair is slightly worse;
later control reaches thermal3, battery28.9–43.2C and power levels0–8 across the
matrix. Warmup exclusion does not make these thermally stationary. The apparent
heavy3.7ms saving is not admitted as a repeatable causal gain. No display-pacing,
physical High, live-motion or30FPS acceptance. Preserve result; no tuning sweep.

Active PSS461–651MiB/RSS580–770MiB,24KiB swapPSS,2.33–2.77GiB system MemAvailable
and0–0.18% instantaneous PSI avg10 are sparse RAM context, not sustained severe
pressure or an explanation of GPU cost. Larger B1 RAM is retained. Bandwidth,
cache misses, stalls, registers/spills/occupancy remain unavailable/unsampled gaps.
Zero new intermediate allocations/passes/synchronization by architecture; no
measured GPU-traffic reduction. The30FPS GPU planning budget22–24ms still needs
roughly66–69% less opening GPU work and77–79% less frozen-heavy work. These are
required reductions, not promised optimisation savings; live CPU/motion differs.

Normal S26 Debug8CB restored with installed pullback equality; last A2 already
restored normal Shipping benchmark3CB, verified in its receipt. Test benchmark
force-stopped, Home launcher independently confirmed; data/stable package untouched.
Experimental runtime source remains on its own branch, not a production admission.

Direct reuse of normal RTX pipeline/compute images additionally leaves5/13 pixel
gates failing, maxRGB4/116/120/6/92 in worst-bend/blue/red/finale/two-enemy, fractions
over-one1.157e-5/1.93e-6/1.93e-6/5.79e-6/7.72e-6. Full comparator also fails capture
timing+3.880%; see `windows/control-backend-comparison.json`. Candidate's within-
backend byte equality preserves those same differences. No tolerance loosened.

### October2 bounded backend-coordinate review

Offline extraction of the retained controls locates six pixels with RGB delta>3
across those five failed images. [Exact coordinates/colours](windows/backend-pixel-coordinates.json),
SHA256 `4b8d8b386e5b66251d7fbee1193c6d333736853fe0b8208c09d5c2f8aca6eece`,
matches all five original maximum-difference and over-one counts. Lead independently
checks all six coordinate/RGB pairs against both original PNGs. Full-frame viewing
does not confidently classify these one-pixel locations or establish their cause.
Blue/red share (396,262), but that alone is not a hit/material/driver diagnosis.

Retained manifests agree on scene/camera, Mobile Diagnostic quality,960x540 extent,
settling frames, CPU geometry and assets. Primary ray arithmetic is common in
`rt_frame.glsl`: pipeline launch ID/extent and compute invocation ID/image extent
feed the same calculation. The8x8 compute dispatch guards its four padded rows
before any trace/write. Source review shows no differing primary formula; it does
not prove identical hardware intersections, intermediate shading or float results.
No new capture, build, shader edit, tolerance change or inference from shader size.
Next unfinished discriminator is first-hit/material or transport identity at these
specific pixels, rather than repeating the full matrix or speculative global math
changes. Shipping/Diagnostic parity is separate; all five backend gates stay open.

Next: normal source/evidence integration, then bounded S24 numeric primary hit
counts by named instance before metadata classification. Current aggregate player
count depends on metadata after hit decode, so zero alone does not isolate AS
traversal from decode faults. Separate player/viewmodel buffers, coherent host
uploads, independent scratch and BLAS→TLAS→shader dependencies are present; no
generic missing-flush/barrier cause is demonstrated. Opening skeleton uses a
dynamic path successfully; the two-enemy state has one pose bucket, not two tested
independent pose-buffer branches. No arm/asset retuning or new diagnostic framework.

Private process/package/device/log rosters remain outside Git. Capture summary
and validation are actual producers, not a replacement for image inspection or
owner playtesting. Audio/haptic manual revalidation required:**NO**, no changed
semantic inputs, playback or assets.
