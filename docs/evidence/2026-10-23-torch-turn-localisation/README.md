# Accepted torch turn response: localized diagnosis

The retained **278.333 mm** maximum is a pre-existing, steep carry response at
a grazing wall angle. It is **not a mathematical discontinuity, angle-wrap bug,
stale rig result or view-space comparison artefact**. The finite input step
still produces about **280 mm of actual hand/socket motion**, so the presentation
issue is real. No production correction or new torch pose was implemented.

This diagnosis starts at `979bc71f`. It annotates the exact retained 7,536-sample
grid once, without adding positions, pitches, aspects or angular coverage. It
then evaluates 24 points confined to the localized interval and repeats/angle
equivalence checks, and solves only the two original maximum poses through the
existing imported-rig diagnostic. No new framework, native RT run, broad suite,
phone access or foreground activation is involved.

## Exact witness and source

Camera X = 1.599079967, Z = -4.900000095, pitch = -0.150000006, aspect = 16/9,
support Y = -0.949999988, anatomical mount, held original torch, no lower blend,
idle phase zero. The original grid's adjacent yaw values are:

| Quantity | 3.129999876 rad | 3.139999628 rad |
| --- | --- | --- |
| Forward clearance | 0.941995263 m | 2.700000048 m |
| Shared carry depth | 0.401649267 m | 0.680000007 m |
| Final local hand depth | 0.351649255 m | 0.629982173 m |
| Torch roof lowering | 0 | 0.000044587 m |
| Torch retraction | 0 | 0.000027081 m |

[Location log](localise.txt). The final local target displacement is
0.278332919 m. The depth preference explains almost all of it; the roof/reach
correction is tens of micrometres here, not the source of the large movement.

The chain is:

1. [The walkable union](../../../src/gameplay/CorridorCollision.h#L24) insets
   the physical 1.85 m side wall by the unchanged 0.24 m player radius.
   The float boundary is X = 1.610000014; the camera is **10.920048 mm** inside it.
2. [ComputeRewardLanternForwardClearance](../../../src/gameplay/items/HeldItemKinematics.cpp#L368)
   samples one normalized forward ray to 2.70 m and refines the first blocked
   interval. At these angles its side-wall intersection is approximately
   `margin / sin(yaw)`: 0.941991 m, then 6.854913 m, capped to 2.70 m.
   The second ray is almost parallel to the wall. The first blocked surface
   really changes distance this quickly; increasing binary refinements would
   not remove the effect.
3. [MapContinuousCarryDepth](../../../src/gameplay/items/HeldItemKinematics.cpp#L112)
   maps that whole 0.30–2.70 m interval to 0.30–0.68 m. The fixed 50 mm aspect
   retraction cancels between the two poses. The response is spatially continuous
   but has high angular gain within about one centimetre of the inset boundary.
4. [ResolveHeldItemsFixedStep](../../../src/gameplay/items/HeldItemKinematics.cpp#L1255)
   publishes the evaluated pose directly. [PlayerAnimationState](../../../src/gameplay/animation/PlayerAnimationState.cpp#L171)
   copies that target into IK; no hidden turn smoothing removes it.

Both yaw values are below +pi: the original maximum does not cross angle wrap.
Repeated identical inputs reproduce identical outputs. An equivalent negative
angle differs by only a few micrometres from float trigonometric precision.
The carry mapping and long-ray policy predate this experiment (blame
`332c2158e`); the production file's last change is `1906a733`.

## Continuous response, actual motion

Localized smaller steps converge rather than retain a finite jump:

| Approximate yaw step | Local hand displacement near yaw 3.1375 |
| --- | --- |
| 0.000100136 rad | 10.0901 mm |
| 0.000010014 rad | 1.0314 mm |
| 0.000000954 rad | 0.09865 mm |

The cap begins near yaw 3.13754818; localized samples also cross it continuously.
[Point receipts](refine.txt), [smaller-step values](refine-small.json).
This establishes the cause of the **global maximum pair**, not continuity of
every doorway/corner or a diagnosis of every smaller cohort maximum.

The same original pair, through the actual player/viewmodel rig and coherent Rag
Grip/Light composition, moves Grip **0.280175 m**, Light **0.280344 m**, and the
most displaced authored left-gauntlet vertex **0.280864 m**. Socket agreement
errors stay below 0.5 micrometres; no pose override is used.
[Actual-rig result](rig-pair.stdout.txt). Thus the large result is not caused by
subtracting targets expressed in two slightly rotated camera frames.

The earlier 10 mm-per-0.01-radian diagnostic remains failed and retained. That
finite-step motion criterion must not be confused with mathematical continuity
or the narrower earlier turn study. At 60 Hz, 0.01 rad per tick is about 34.4
degrees/second, a plausible turn input. No new native image or physical-control
claim follows: the retained RT turn images cover yaw 1.17–1.97, not this pair.

## Smallest supported proposal; not implemented

Treat the single-ray distance as a truthful geometric query, not a sufficient
motion policy. Do not change collision insets, the 2.70 m endpoint, bisection
precision or input/camera motion merely to shrink this number. The same helper
also affects sword/reward carry, so a global mapping change needs wider proof.

The smallest defensible next seam is the original torch's **soft carry-depth
preference scalar** at the existing 60 Hz held-item resolution boundary. Keep a bounded
transition in simulation-owned state, then apply authoritative hard clearance
before publishing one pose for hand, torch, flame and Light. Existing
[GameSimulation ownership](../../../src/gameplay/simulation/GameSimulation.cpp#L1588)
already retains the fixed-step state; this does not need a renderer filter,
thread, general solver or new system.

A rate limit alone is insufficient. Smoothing the final hand could delay a
required retreat into masonry; limiting outward motion alone leaves the reverse
turn's inward pop. A 10 mm/tick limit would also take roughly 28 ticks to cover
this displacement. Therefore first prove an authored compact-carry pose and a
feasible transition path, rather than silently adding a half-second lag.

The compact-pose prerequisite is narrow:

- Use the existing rig, root/shoulders and unscaled Rag asset. Keep the gauntlet
  and a readable torch head/shaft in the actual shader frustum at 11/13/21,
  the grazing pair, and centre/bend, in square/wide views. Pivot visibility or
  a tiny clipped sliver is insufficient; Light must remain ahead of the camera
  in the intended close-carry presentation.
- Contain the complete mesh, full flame box, Light and shadow endpoints with
  unchanged rig reserves and 30 mm roof gap. Preserve socket agreement and
  anatomical reach; move every attachment through one shared pose.
- Verify the path in both turn directions, including face-on and angle wrap.
  Preserve ordinary open carry and lowered/soaked/dropped/reward ownership;
  seed/reset any transition at existing lifecycle boundaries. Safe endpoints
  alone do not prove a safe or smooth path.

Do not filter the camera or final world target. If authoritative clearance still
forces a large pose jump, reject the transition proposal rather than accepting
clipping or claiming that a bounded preference alone solved the presentation.

If no such compact pose fits the existing geometry, report that boundary before
changing assets, the camera or player stand-off. No production candidate starts
from this proposal. Torch clearance/framing and native visual acceptance remain
open; the earlier joint-pose rejection is unchanged.

Temporary diagnostics are retained as a compressed patch, then restored byte for
byte and their original targets rebuilt. Production/benchmark source never
changed. The existing exact runtime checkpoint `979bc71f` has six successful CI
jobs; that is not a claim about a subsequent documentation head. Owner files,
saves and settings are untouched. Audio/haptic revalidation required: **NO**.
