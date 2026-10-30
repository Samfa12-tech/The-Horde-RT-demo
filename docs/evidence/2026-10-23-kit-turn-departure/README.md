# Kit stationary turn and departure: retained reproduction

Base `6124a374783aaae61ea2eb8f7142428443e75d40`, existing PR27 worktree
`.worktrees/horde-1.7-wp2-vertical-proof`. The accepted `478f6c2` waypoint-facing
correction remains intact. **No production motion correction is admitted.**
This checkpoint adds eight snapshots to the existing finite hidden native motion
fixture, not another animation system, gameplay state or renderer experiment.

## What the actual RT sequence establishes

The original start/stop/turn/waypoint sequence passes 535 checks / 37 RT renders
in 18.30 seconds. Its initial departure starts only about 6.34 degrees from the
route bearing: that alone is a weak witness for the reported awkward departure.
The stationary turn is readily visible: the root stays fixed, walking weight is
zero and phase stays fixed while the idle actor swivels through 161.56 degrees.
The current clips do not provide turn-specific foot articulation/contact.

The extended fixture establishes **PlayerLagging** at seven metres, with proof
true, grounded root unchanged and no walking for 100 steps. Moving the diagnostic
player inside the existing six-metre tether then resumes the same supported
production route. Proof stays true throughout this wait/departure section.
There is no completion edit, teleport, save write or change to the loaded paused
campaign. This remains seeded immutable production-route evidence, **not an
earned journey or an observed owner playthrough**.

At fixed 60 Hz, retained native snapshots show:

| Departure tick | Elapsed time | Horizontal travel | Facing error relative to travel |
| --- | ---: | ---: | ---: |
| 1 | 0.017 s | 0.030 m | 152.91 degrees |
| 8 | 0.133 s | 0.240 m | 136.89 degrees |
| 20 | 0.333 s | 0.600 m | 109.39 degrees |
| 40 | 0.667 s | 1.200 m | 63.55 degrees |
| 60 | 1.000 s | 1.800 m | 17.71 degrees |
| 80 | 1.333 s | 2.400 m | approximately zero |

These values derive from the native JSON's rounded root/yaw fields, not screen
pixel estimates. Fresh original RT frames at departure 0/20/80 were inspected,
alongside the earlier start/turn frames and the narrower departure attempt.
Kit visibly uses the forward walk while facing sideways/backwards to travel.
This is a reproducible presentation defect, not proof of a collision, path,
save, reflection or gameplay-timing defect.

The final fixture passes **643 checks / 45 RT renders in 19.50 seconds**. All
26 older motion snapshots retain identical root/yaw, walking, waypoint/wait,
clocks and gait phase/weight. **All 30 older PNGs remain byte-identical**.
The older timing cohort is excluded from the new section and unchanged; the
90-second / 180-render limits and read-only/foreground guards are retained.
Only the eight specifically named diagnostic images enter the output allowlist.

The first, narrower extended attempt passes 542 checks / 44 RT renders, but
inherits proof=false stop/turn staging from the old pose fixture. It remains
recorded and is not substituted for the later real tether-wait witness.

## Why no yaw-only patch is banked

`CompanionRouteFixture::StepRoute` moves immediately at 1.8 m/s while `Step`
limits facing to 2.4 rad/s. `CompanionLocomotion` keeps the existing 1.56-metre
distance phase and 0.18-second blend. These explain the large-angle departure
without requiring a shader or pose-cache fault.

An idle-only easing suggestion would soften the last few yaw ticks, but would
not plant feet or correct the departure. It was assessed and **not implemented
or RT-tested as a candidate**. Snapping facing would introduce an abrupt pivot;
waiting to align would change the explicitly preserved motor/story timing.
The old walk-at-rest shortcut, Run-at-walk-speed substitution and withdrawn
six-joint turn-step experiment remain closed. No replacement is silently active.

The next useful motion work needs an authored **same-rig turn/contact study**,
then a distinct directional departure solution if immediate travel is retained:

- Runtime rig: `assets/models/characters/runtime/kit-warden-v1/kit-warden-lod0.runtime.glb`,
  SHA-256 `1ba628235814b5912ae1439b180347dd8e1cce85b0f4d1fae5bbacb41e7d3713`.
  Source: `assets/models/characters/source/kit-warden-v1/briarhold-warden-1k.glb`.
  Existing 24-joint clips are fall, idle, jump, mantle, run, slide and walk;
  there is no turn, backward or side-departure clip to select.
- Preserve bind hierarchy, conversational upper-body acting, silhouette, grounded
  root, collision and original asset. Author supported left/right foot exchanges
  with explicit stance/swing/contact intervals and known yaw coverage. A lifted
  boot alone does not prove the support boot is planted in world space.
- Establish contact and joins at partial/aborted turns, direction reversals and
  return to idle/walk on the existing terrain. Tie any authored counter-rotation
  to actual applied yaw; a fixed-duration clip cannot be assumed to match every
  capped turn. Avoid adopting a broad IK/retargeting system for this study.
- At unchanged immediate travel, facing may differ by over 150 degrees. A
  forward-only clip cannot present correct backward/side foot travel throughout
  that interval. Assess a bounded directional departure asset/blend separately;
  changing the motor or facing policy would need an explicit scope decision.

Those are requirements/proposals, not newly implemented states or acquired clips.

## Validation and custody

The Debug native application builds successfully. Existing acting, locomotion
and actual-rig/source-intake CTests pass **3/3 in 9.97 seconds**. No assertion,
quality setting, deadline or coverage gate was weakened. Functional fixture
passes do not close the visible motion acceptance failures described above.

Both control and final fixture use real `RayTracingPipeline` hardware RT at
1232 x 803 internal/output pixels with DRS off. Initial inventories match:
52 buffers, 63 allocations, 20 BLAS, one TLAS, 28 instances, 49,622,144
host-visible and 407,976,928 device-local bytes. These are matching inventory
observations, **not a sustained timing or performance comparison**.

External foreground ownership and owner settings/campaign custody pass. Both
owned native processes exit normally; there is no phone operation or activation
attempt. All **151 parked owner files** retain their original hashes. Production
route/locomotion/acting/pose/renderer sources, Kit/player meshes, torch and
benchmark implementation remain unchanged. Audio/haptic manual revalidation
required: **NO**; only diagnostic capture coverage and documentation change.

[receipt.json](receipt.json) retains compact exact-build/source identities,
native counters, departure samples, all final PNG hashes and custody results.
Original images/logs/isolated fixture data remain under ignored
`reports/kit-turn-departure-20261023/` and
`reports/horde-campaign-fixtures-kit-turn-departure-20261023-{control,departure-witness,tether-witness}/`.
No binaries, raw images or private saves are added to Git.

The preceding exact [6124 CI40032512766](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/40032512766)
is terminal with all six jobs successful. The new checkpoint requires its own
exact-head result after backup; prior green CI does not certify the added fixture.
PR conflicts, benchmark WIP acceptance, authored torch clearance/framing, natural
full journey/recovery, phone controls/audio and sustained warm performance remain
separate open items. No merge, release, phone or foreground action is authorized
by this evidence checkpoint.
