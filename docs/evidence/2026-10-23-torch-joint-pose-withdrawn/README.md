# Joint torch pose trial withdrawn — 23 October 2026

**No runtime change is admitted.** One joint hand-position/Grip-orientation
candidate was built and compared with `b27546d0`. It fits the torch into the
tested wall volumes, but fails actual left-hand framing, brings torch geometry
too close to the camera, and introduces larger target discontinuities. The
accepted runtime, original tests and capture source are restored byte for byte.
The opening-menu benchmark remains unchanged and unadmitted.

## One candidate, retained failures

The trial adjusts hand XYZ and two Grip rotations together. A fixed, eight-step
coordinate solve uses the same view axes throughout a turn; it does not normalize
the wall tangent that vanished in the earlier trial. A conservative envelope
includes the actual Rag mesh bounds, full flame box, Light socket, all four shadow
sample endpoints, and existing rig/orientation reserves. One overhead evaluation
avoids compounding fractional anticipation. IK and every socket consume the same
pose. The camera, collider, light power, quality and renderer are unchanged.

The solve uses soft pivot-framing preferences, not a guarantee of visible
geometry. The unchanged archived actual-rig oracle still decides containment,
30 mm roof clearance, socket agreement, and positive clipped area of both the
authored left gauntlet and torch. Its failures are not replaced by a lower
objective score.

| Evidence | Accepted control | Joint candidate |
| --- | --- | --- |
| Actual rig: 60 poses, square/wide, three pitches, two phases | 52 failing cases | 10 failing cases |
| Required actual left-gauntlet **and** torch framing, 10 cases | Six wall framing cases fail | The same six wall framing cases fail |
| Centre upward-pitch roof witness | 29.4214 mm minimum; fails 30 mm gate | Same retained shortfall; not fixed |
| Broad synthetic turn sweep: 7,536 adjacent samples at 0.01 rad | Maximum hand-target delta 278.333 mm | 384.384 mm; Grip-up chord 1.43049 |
| Ordinary native bend/centre images | Baseline | Byte-identical PNGs |

The broad turn sweep spans both walls, three pitches and two aspects over nearly
a full revolution. Its 10 mm target-delta diagnostic fails for **both** versions.
It is wider than the earlier limited wall-turn study; the 278.333 mm control
finding must be localized before attributing it to a particular gameplay turn.
It does not invalidate or replace that older study's distinct 2.601 mm result.
The candidate is rejected without another tuning loop. No performance gain is
claimed; these Debug diagnostic durations are not a gameplay timing benchmark.

The six framing failures have zero actual left-gauntlet projected area. At wide
aspect, diag13's Light depth is -5.577 mm and diag21's is -34.199 mm: containment
has again been bought with the light behind the camera. Positive torch area
alone does not establish a usable view. The original and candidate logs are
retained verbatim alongside the complete compressed runtime/test patch.

## Actual Windows RT comparison

Two fresh native hidden runs retain ten images each: the five original views
11/13/21/40/48, then five keyframes from 81 positive fixed-delta turn steps
(yaw 1.17 to 1.97 radians in 0.01 increments). Every turn step is rendered; only
five are retained as images. There is no video or visual acceptance of every
intermediate frame. The simulation owns movement and look snapshots; historical
completion flags and owner campaign state are not imported.

All 20 PNG hashes, owning completion/submission/presentation identities, and
actual RT dispatch/copy flags pass. Each pair has the same camera, simulation
tick, shader module hashes, actual dimensions/settings, and emitter power. See
[matched views](matched-views.csv) and [receipt](receipt.json). The native bend
capture (40) has yaw **0**, whereas the separate rig oracle's bend case has yaw
**-pi**; they are separate witnesses, not a claim of an identical rig/RT view.

Direct inspection of original RT images confirms:

- Diag11 loses the previously visible torch, leaving only a sliver at the edge.
- Diag13 lights the stone but fills much of the view with extremely close torch
  geometry. Diag21 has a similar obstruction and remains largely dark.
- The retained face-on and closing turn images exhibit the same obstructive
  geometry. No hand-framing or continuous-motion improvement is accepted.
- Bend and centre retain their exact baseline image hashes.

These are RTX 5050 Laptop diagnostic captures, 960 × 540 internal/output,
RayTracingPipeline, High shader/fire, Higher shadows, Mist On and Dust High.
They are not production-bank performance measurements, physical input testing,
phone evidence, or an explanation of the owner's original recorded black world.
No foreground activation, screenshot fallback, phone access or owner interaction
was attempted. Existing hidden capture guards remain unchanged.

## Restoration and bounded next step

The candidate and temporary gates/include are withdrawn; the accepted app and
affected tests are rebuilt. Source and settings hashes are in the receipt.
The four original transition, clearance, final-rig and socket CTest registrations
pass **4/4 in 171.80 seconds**, including the final-rig test in 138.01 seconds.
[Restoration results](restored-tests.txt) describe the accepted implementation;
they do not close the archived wall failures.
The 151 parked owner files remain outside this change. Native capture mode has
no campaign session; no owner save is used. Raw PNGs, binaries and full ledgers
remain ignored under `reports/torch-joint-pose-20261023/`, not shipped or pushed.

This rejects one bounded pose family and objective, not every possible torch
pose. A mathematical prerequisite review also found that, at the exact face-on
wall stance, one near-wall-parallel family cannot fit both Grip and Flame
**centres** in the frustum even with optimistic reserves (maximum separation
0.54877 m, actual socket distance 0.56518 m). That limited result does not rule
out partially visible real geometry; it is not used to weaken the framing gate.

The next useful torch step is a bounded authored compact-carry pose feasibility
check on the actual rig, requiring the hand, torch head and Light to stay in
front before wiring another runtime response. Keep the original wall/roof and
turn witnesses, and first localize the control's wide-sweep target jump before
claiming a particular turn is smooth. A pivot-only objective is insufficient, and this iteration
does not justify another runtime search, camera/collider adjustment or torch
asset alteration. That next proposal is recorded, not started here.

Debug route timeout, PR27 conflicts, benchmark visible/DPI/controller/listening
acceptance, natural full journey/recovery and warmed-phone acceptance remain
open. No merge or release. Audio/haptic manual revalidation required: **NO** for
the restored, evidence-only checkpoint; prior listening gaps remain pending.
