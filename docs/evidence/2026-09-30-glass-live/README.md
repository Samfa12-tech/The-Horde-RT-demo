# September30 live glass counter evidence — correctness gate remains open

This continues the accepted corner/candidate/surface and geometric-shadow fixes.
It does not restart Phase1 or change shaders, physical budgets, materials,
geometry, player ownership, mounting, animation, ray masks or CPU/GLSL ABI.

## Narrow evidence repair

The existing benchmark ledger retained only Diagnostic availability, whereas
the existing owning completion already contained41 fence-latched counters.
`RtBenchmarkEvidenceRun::Complete` now copies that array only after canonical
validation and exact pending-row association. It never samples the latest
observer during serialization. JSON adds `diagnosticCounters` beside the owning
completion identity: a full array for valid Diagnostic samples, `null` for
Shipping/failed unavailable samples. All-zero available data is not null.
The bounded benchmark-only storage adds164 payload bytes per allocated row.
No new counters/readback/telemetry framework or Shipping shader overhead.

Tests prove delayed association, slot reuse, value-copy independence, wrong
diagnostic serial rejection, zero/unavailable distinction and nonzero array
serialization with the exact completion identity. Omitting the copy fails both
focused tests (RED); restoration passes2/2. Lantern scenario test passes1/1;
Android export/Intent unit tests pass8+3, with no Java source change. Native
High build and ARM64 Debug/unsigned Release builds pass. Actual APK scans
validate/disassemble both backend module pairs: Diagnostic5/41 atomics;
Shipping0/no Binding22. Shader identities are unchanged from the shadow fix.

## Exact runs, not interchangeable platforms

All three existing `lantern-reveal-sequence-v1` runs complete600 warm-up plus600
measured frames. Reports contain600 valid counter arrays, CPU/GPU samples and
presented outcomes, matching submitted/completed identities and strictly
increasing submission/completion serials. No rejected/cancelled/outstanding rows.
This establishes report integrity, not glass correctness or image parity.

- RTX5050 Laptop GPU / Vulkan1.4.341, Diagnostic/High,100%,1232x803;
  pipeline report235544 and compute report235715, exact EXE3b33ce56.
- SM-S948B / Android16 / Adreno840 / driver2150932499, Diagnostic/Mobile,
  pipeline, strict ASTC,75%,1080x2235 internal/1440x2980 presentation.
  Run `glass-geometric-live-20260930-01`, exact APKad6ebaa5 installed/pulled
  back byte-identically. Ordinary Debug remains installed; phone returned Home.
  Stable/owner candidate apps and data remain untouched. No S24/S25 claim.

| Live measured event | RTX pipeline | RTX compute | SM-S948B pipeline |
| --- | ---: | ---: | ---: |
| Transport / production stack failures |107 /93 rows |105 /90 rows |918 /458 rows |
| Primary volume-budget events |106 /92 rows |105 /90 rows |911 /458 rows |
| Primary mismatched exits |1 |0 |7 |
| Certified primary recoveries |193 /157 rows |192 /155 rows |301049 /600 rows |
| TIR events |1493633 |1493649 |5439140 |
| Bounded TIR terminations |194753 |194757 |1857996 |
| Shadow overflow / unclosed |0 /0 |0 /0 |0 /0 |

Interface-budget, open-miss and open-opaque failure counters are0 in these
three live views, not universally. Certified reason masks OR to17 on both RTX
backends and19 on phone. These are masks, never additive event counts. Phone
reason bits1/2/16 occur in422/600/458 rows respectively. Recoveries/TIR
termination are retained bounded events, not erased or equated with solved
physical transport. Older live exports lacked these arrays and cannot establish
whether these failures are new; this row-only patch does not modify rendering.

The precise witnesses are phone row20/tick623/submission622 (42 volume
events) and RTX row230/tick833/submission831 (3 events). [RTX ray proof](rays/README.md)
now reproduces the latter on both backends and isolates three numerical
continuation failures using captured native object rays. Phone first volume
failure is row1/tick604; first mismatched exit is row107/tick710. Preserve exact
report identities when reproducing a pose; do not infer a volume overlap or
missed exit from the counter name alone. `checks/live-analysis.json` retains
the first/last/max witnesses and bounded windows for every nonzero counter.

## Live images, thermal context and limitations

Two180-second raw screen recordings remain local. Only game-only excerpts are
retained here: clip1 seconds20-179 and clip2 seconds0-40, reencoded solely to
exclude startup/Home/private wallpaper. No geometry, in-game interval or pixels
are hidden to pass an image gate. The first excerpt covers the continuous
warm-up cycle (HUD1/2 frame1 through transition to2/2), then the early measured
frames; the second covers later measured frames. There is a gap in recorded
measured coverage. Selected original decoded game frames are in `frames/`.
Sampling those frames is not exhaustive motion/owner-feel acceptance, and
video reencoding is not a raw RT-storage/pixel-parity comparison.

Phone context has58 samples: thermal status0, GPU thermal levels0-1, battery
26.2-34.7C. The report's Diagnostic median245.9272ms /p95258.7660ms is a
descriptive observation with recording enabled, not matched Shipping evidence.
RTX timings likewise are not a matched performance or backend equivalence gate.
No speedup is claimed. Shipping/Diagnostic parity and pipeline/compute image
parity remain separate, unchanged gates.

Run `python docs/evidence/2026-09-30-glass-live/analyze-live.py` from repo root
to verify source ABI field order, rows, identities, marker and counter summaries.
Its stdout is the retained analysis; original reports remain unchanged.
`artifacts.json` identifies the exact precommit development builds and clips.
The reporting-fix sourcefec73b8 CI is freshly green: push36649013510 /
PR36649016938,45 portable +11 Vulkan-host tests, with both current logs inspected.
Remote head matches the local commit; PR15 stays draft/unmerged. Later source
requires its own validation; these jobs do not substitute for it.

Next: trace the demonstrated live volume/mismatch witnesses without changing
budgets or suppressing diagnostics, reconcile contact/certified truncation and
remaining isolated-lantern exhaustion, then matched Shipping performance and
separate backend divergence. Loader closed-manifold certification does not
prove universal disjointness or cross-instance placement; the admitted current
lantern is not replaced and this is not evidence that its panes overlap.
Unknown-initial-medium/no-boundary shadow absorption remains open.
The full music/reporting/resource/pacing/final-candidate scope remains intact.
Audio/haptic manual revalidation required: **NO**, reporting-only, semantic
inputs, events, audio/haptic routing and playback unchanged.
