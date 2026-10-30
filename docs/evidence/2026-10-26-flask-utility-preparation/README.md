# Utility equipment preparation: source-only, not integrated

The [shared lease](../../../src/gameplay/items/UtilityEquipmentLease.h) and the
existing [held-item tests](../../../tests/HeldItemTransitionTests.cpp) now prove
a narrow equipment-owner composition. There are **no production callers**;
`GameSimulation::FlaskContextFor`, player IK, snapshots, input, saves, renderer
resources and production defaults are unchanged. Flask stays OFF. This is not
native flask animation, completed controls or permission to admit its assets.

## Ownership proof

The lease records the existing sword transition's semantic sequence, kind and
start tick. It permits only its own active Stow/Restore to count as utility work;
a foreign same-kind transition invalidates the lease without being interrupted
or adopted. It rejects left-light, wrong-hand, world-owned, active/unsettled and
sequence-exhausted admission. Returning waits for explicit hand-profile release
of the utility. Repeated return intent does not restart the transition. A sword
that began sheathed stays sheathed rather than being drawn as a side effect.

The existing `HeldItemState` remains the equipment authority. The lease advances
no timer, grants no healing/mitigation/input edge and introduces no platform or
render clock. Transient owner copies continue deterministically. Its Reset drops
provenance only; the existing owner must still settle/reset equipment on actual
death/load/reset. Resetting this token does not make a moving sword ready.

Debug and Release builds of `horde_rt_held_item_transition_tests` passed locally.
All prior transition tests remain, with added cases for seven early/middle/late
cancellation positions, held return, equipment pause, foreign replacement,
copied-owner continuation, lifecycle token reset and original sheathed state.
These source cases do not claim actual game input, save/load or pause acceptance.
The first sandboxed MSBuild attempt produced only its banner and was stopped;
the normal host-access builds completed. No failed game launch was retried and
no security setting changed.

## The 0.89-second cue misses native readiness

The tests compose the real `HealingFlask` and `HeldItemState` clocks at full-speed
60 Hz. Admission occurs before the first equipment advance, progress is observed
after it, and healing is stepped at the end. The hand-release cue is an explicit
source-profile prerequisite, not a guessed item-detach or renderer event.

| Source composition | Sword ready tick | Heal tick | Remaining charges |
| --- | ---: | ---: | ---: |
| Retained 0.89s cue, no completion-readiness check | 76 | 75 | 1 |
| Retained 0.89s cue, completion-readiness check | 76 | No heal | 2 |
| Provisional 53/60s cue with readiness check | 75 | 75 | 1 |

The first row exposes an unsafe **test composition**, not a newly observed game
bug: this owner is not wired into gameplay. The second rejects premature
completion without wasting a charge. The third shows the smallest timing
candidate: the hand must have returned the flask by 53/60s (0.883333s), so Restore
can begin before tick54's equipment advance and finish by tick75 using the
unchanged 0.36-second transition. The retained source art still uses0.89s and
**has not been retimed**. A test supplying the earlier release cue does not prove
that the hand has physically released the current asset then.

No gameplay duration, world speed, tether, parry window or accepted guard pose
changed. The proof is not a measured wall-clock/FPS result or a slowed-gameplay
claim. The [receipt](receipt.json) pins the source inputs and local test artifacts.

## Separate tick-aligned art derivative

A [separate editable r2 source](../../../assets/models/player/source/flask-use-tick-aligned-r2/README.md)
now implements that source timing candidate. It returns the flask to the original
belt root exactly at53/60s, restores over the unchanged0.36s and holds the final
source pose through1.25s. Original r1 Blend bytes and its0.89s profile remain.
Regenerating without the new explicit authorer option reproduces all original
complete and cancellation pose rows exactly. The r2 cancellation rows also
match the original exactly; its meshes, binds and original runtime/guard inputs
are unchanged.

Reloaded r2 actions pass76 complete/71 cancellation transform samples, exact
start/end right-arm basis and unchanged left-arm matrix checks. All292 selected
head/flask and head/gauntlet intersection observations remain zero. The
[source receipt](retimed-source-receipt.json) pins those results and the new
effective intent. These remain discrete selected-region checks, not continuous
whole-body/prop collision detection.

The body-view frames around the handoff were inspected. The source first-person
belt witness has the hand/object outside its camera crop; it cannot establish
native viewmodel visibility or comfort. The authoring body's final sword tip
also exceeds its camera crop. This and the generic native IK/item
profile still need actual aspect/light/weapon join review.

[Complete r2 source animation preview](tick-aligned-complete.mp4): silent Blender
Workbench,960x540,76 frames at60Hz,1.266667s including the endpoint sample.
Rendering was unpaced. Full decode passed; no generated interpolation, repeated
padding, artificial speedup, game RT frames or measured FPS are claimed. The
unchanged cancellation action retains its [original source preview](../2026-10-26-flask-motion-study/cancel-source-preview.mp4).

## Remaining admission gates

The retimed art is still a source candidate. The caller ordering must be integrated
deliberately: current GameSimulation does final flask admission/stepping after
equipment advancement. Do not move methods wholesale, advance equipment twice,
remove the unrelated-transition gate or postpone healing silently. Keep final
damage/cancel/readiness checks authoritative before resource completion.

Actual utility pose/left-light/sword joins, first-person framing, world clearance,
reflection/shadow and lifecycle remain native acceptance gates. The joint
warning28/flask29 resource proposal still requires specific approval for ceilings
30 definitions /15 assets /38 primitives /34 materials /106 TLAS /23 ordinary and
25 shared BLAS; none changed here. The owner-running desktop game and background
phone app were untouched; no new Computer Use prompt or phone call occurred.
