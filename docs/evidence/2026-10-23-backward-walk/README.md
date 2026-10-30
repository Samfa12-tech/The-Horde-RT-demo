# Backward walk phase — 23 October 2026

The owner-reported forward-running appearance while moving backward is a
shared animation-selection gap, not a demonstrated reflection-renderer fault.
Current player source/runtime assets contain Idle and forward Walking only.
This bounded candidate reuses that owned walk in reverse, preserving a
continuous phase through direction changes. It adds no clip or imported rig
and does **not** implement an authored backward run or strafe animation.

## Runtime change

Shared simulation derives direction from collision-resolved displacement.
Backward travel reverses the effective phase by its accepted distance; blocked
movement freezes it. Forward travel retains the existing phase advance exactly.
An accumulated offset changes future increments without negating or resetting
the current pose. Authoritative capture poses, dodge and suppressed traversal
remain on their existing paths. Checkpoint reconstruction resets the transient
offset; no save field or schema changes.

One duration-aware sampler serves the actual pose, boot grounding, shoulders and
Hips queries for both body/viewmodel consumers. Gameplay, held-item/combat and
footstep clocks remain unchanged. The first candidate held a raised walk foot
after blockage; its corrected version uses the existing idle selection when
the motor stops, while retaining the original movement-intent input to footstep
cadence. The first runtime patch remains [archived](candidate-v1-runtime.patch.gz).

## Evidence and limits

[Focused tests](corrected-focused-tests.txt) pass 2/2. Actual loaded/skinned body
tests retain Grip position/orientation and the established nonpenetrating
25 mm sole bound, plus grounding/shoulder/Hips/held-item/light coherence.

| Measurement | Result |
| --- | --- |
| Actual backward path, same root path and clocks for candidate/old gait | 4.05322 m; 42 trace samples, 37 stance intervals each |
| Stance-foot world-speed RMS, reverse / old forward gait | **0.560598 / 3.36144 m/s** |
| Stance-toe world-speed RMS, reverse / old forward gait | **0.464393 / 3.41502 m/s** |
| Maximum stance-foot world speed, reverse / old | 1.33204 / 3.52175 m/s |
| Actual asset phase pairs, including loop wrap | 17; exact paired pose error 0 at identical published float phases |
| Forward/backward/stopped transition ticks | 48 / 48 / 12; maximum effective phase step 0.0166667 s |
| Natural backward wall approach / blocked ticks | 42 / 54 |

[Full measurements](actual-foot-measurements.txt). Lower skinned soles identify
stance intervals; foot/toe-node world speed measures residual sliding. No
quality threshold was invented to label these numbers natural motion. Sliding
is substantially reduced in this fixture, but remains measurable. This is not
foot locking, full running/strafe coverage, phone motion/feel acceptance or a
performance/FPS result.

The initial [pose-pair failure](focused-tests.txt) and
[numeric diagnosis](paired-failure-diagnostic.txt) are retained. Its supposedly
equal samples differed by 164 ns after float cancellation, producing an 11.15 µm
boot-vertex difference. The corrected fixture compares identical published
float phases against plain forward references, keeping the **10 µm** geometry
gate. A separate analytical sign/wrap check uses a roundoff budget derived from
the bounded float operations: measured maximum error 278 ns versus 1.90735 µs
budget. No inherited assertion or deadline changed.

Three native RT mirror sequences retain 18 matching poses each: accepted
control, first candidate and corrected candidate. All camera poses, simulation
ticks, legacy clocks and rendering settings match. Each variant's first four
initial uninterrupted forward images are byte-identical to control. Forward
after standing or reversing retains the continuous phase offset: its advance,
root path and legacy clocks are preserved, not every historical forward pose.
The corrected blocked body
returns to idle rather than holding the first trial's raised foot. See
[image hashes and identities](matched-mirror-witnesses.csv), process receipts
and [temporary method](mirror-probe.cpp.txt). The temporary include is removed.

These are isolated mirror-room motor diagnostics, not a natural route or combat
playthrough. RTX 5050 Laptop GPU, Windows Debug, RayTracingPipeline, 960×540
internal/output, High shader/fire, Higher shadows, Mist On and Dust High.
Mirror primary-arm counts are zero; these pictures do not pass first-person
arm admission. Their `travelled` field is reset by the rendering owner's
zero-delta publication; use the matched camera path and host motor measurements,
not that field, to assess motion. Raw PNGs, full native ledgers, build logs and
full test output remain ignored under `reports/player-backward-20261023/`.

## Gates and handoff

The complete Windows Debug target build passes. The unhooked standard
`chapter-stone-portal-front` first-person RT fixture passes with **2,151 primary
player pixels**, successful owning presentation and the original visibility
gate. The image was inspected; this is a static portal witness, not dynamic
backward first-person or phone acceptance.

The [complete Windows Debug selection](full-test-summary.json) finishes with
**226/230 passing** across two fixed-source segments. The tool session was
interrupted during test 143; its process was verified stopped before completing
143-230 separately (88/88 pass). No runtime or test source changed between
segments. Four failures remain: campaign and step-movement 30-second timeouts,
plus two frozen generic-shader include hash checks. The latter include is
[canonically identical to accepted 096b23f0](shader-hash-attribution.json), so
the hash mismatch predates this candidate. No expected hash or gate is changed.
The authored-terrain journey passes in 196.12 s, native report UI in 1.80 s.
The later additive blocked-Idle assertion is checked separately; it changes no
runtime source.

The final full build passes. The [final focused run](final-focused-tests.txt)
passes animation and directional/Idle checks; step movement again times out
at its unchanged 30.01 s gate (2/3 pass). No further retries were made. The
fresh final native first-person capture passes the same 2,151-player-pixel guard
and is byte-identical to the earlier portal image. Its image was inspected.
[Final artifact, source hashes and owner-file custody](final-checkpoint.json)
and [capture receipt](first-person-final.process.json) are retained. This still
does not establish dynamic backward first-person or phone acceptance.

The [bounded step comparison](step-comparison-summary.json) completes all 73
checks with byte-identical output: candidate 28.406 s wall / 27.90625 s process
CPU busy; accepted `096b23f0` control 31.641 s wall / 28.90625 s CPU busy. The
accepted control itself exceeds the unchanged 30-second gate. Both direct runs
exit zero; neither is a CTest gate pass, and one sequential pair establishes no
performance win. This supports an existing near-limit Debug workload problem,
not a demonstrated new gameplay hang. The original timeout stays retained.
[Method](step-comparison-method.py.txt) and exact source/executable receipts
are included; all five owned candidate sources were restored byte-for-byte.

The exact preceding evidence-only head `096b23f0` has a final **failed**
[CI run 39877269271](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39877269271):
five jobs pass; MSVC's native report UI times out at 30.02 s before the form
becomes visible in the initial note-only phase. Main selection is 168/169.
The trace collector's later “did not execute” text conflicts with the actual
CTest execution and is not a diagnostic pass. This differs from `4eeafcb6`'s
15-minute overall-job cancellation. Neither gate is relaxed.
[Structured result](prior-head-ci-final.json).

Outdoor music remains **blocked, not integrated**. Exact approved main/bed/source
Library references resolve, but each materialization fails when the required
helper calls `os.setxattr`, unavailable in native Windows Python. All final
destinations are absent; no partial asset was admitted or metadata bypass used.
[Exact action, targets and error](outdoor-music-blocker.json). Existing chapter
G→H arrival/F return logic, ducking, user volume and saves are unchanged. H also
serves legacy dawn, so replacing its PCM globally would be incorrect. A new
106.667-second cue needs explicit routing/duration and memory admission once
the approved bytes can be transferred correctly. No listening occurred.

The [Library skill](skill://plugin_connector_1p_1b8ff8edfc1481918b252c8277e23125/library/SKILL.md)
materialization reference requires: "If the helper is unavailable, stop rather
than producing a local file without its Library metadata." Here the required
helper cannot complete metadata application on Windows; stopping also follows
the parent's explicit no-bypass direction.

The rejected torch trials stay withdrawn. PR #27 conflicts, the existing Debug
route 30-second gate, natural Keeper/full journey/recovery, final touch UI,
listening and sustained phone acceptance remain open. No phone, merge or release
operation occurred. Audio/haptic manual revalidation required: **NO** for the
animation-only behavior; existing listening acceptance remains pending.

Next bounded acceptance work is dynamic backward first-person framing and
backward-to-dodge/rope continuity, with the same rig and current runtime. Read
the source-changing checkpoint's exact CI result rather than infer it from the
preceding evidence-only run. Keep the 30-second Debug gates and frozen shader
hash mismatches open; no base integration or new torch trial is part of this
checkpoint. The existing treasury-road/chimney vista remains a separate approved
player-facing gap after this motion checkpoint.
