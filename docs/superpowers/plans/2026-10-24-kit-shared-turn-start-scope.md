# Proposed bounded Kit turn/start integration

Implementation follow-up: the [opt-in shared-state checkpoint](../../evidence/2026-10-24-kit-shared-turn-start/README.md)
now exercises this plan on actual route terrain/RT. It remains OFF/unadmitted;
the 193-tick large departure leaves pacing acceptance open. The scope/evidence
history below is retained rather than rewritten as a production acceptance.

24 October 2026. Scope only, based on `3fd5ef6cf835d6708663235addaa92d6760487f0`.
No production bindings, code, assets, save schema or timing have changed in this
pass. The preceding exact-head CI40055011876 passed all six jobs. This plan is not
implementation/admission evidence or permission to expand into another system.

Initial first-gate follow-up: the [partial-angle study](../../evidence/2026-10-24-kit-partial-contact/README.md)
retains increased contact-point motion at 22.5 degrees and low swing clearance at
2 degrees, plus the large-angle 1x preview. Its first measurement-only disposition
withheld admission and proposed another authored pair. That asset recommendation
is withdrawn by the visual reassessment below; neither a derivative nor a
production controller has been built.

Subsequent [visual reassessment](../../evidence/2026-10-24-kit-partial-contact/VISUAL_REASSESSMENT.md)
finds no objectionable visible defect at gameplay distance. It withdraws the
extra small-angle asset recommendation and recommends the existing matched
recipe for a bounded shared-state trial. Contact diagnostics remain unchanged;
the initial numerical gate was not a demonstrated visual failure. Large-angle
pacing and actual joins/lifecycle remain acceptance work; this review starts no
implementation.

## Recommendation and current seams

Add a small Kit-specific turn/start state to the existing shared route owner.
Keep `GameSimulation` at its current authority boundary, one Kit actor, the
existing skin/blend/upload/refit path and original Idle/Walk. No state-machine
library, IK system, root-motion motor, new worker or GPU actor lane is needed.

Current `CompanionRouteFixture::StepRoute` moves at 1.8 m/s immediately, then
`Step` caps facing at 2.4 rad/s. This causes the retained 153-degree first-moving-
tick departure mismatch. `CompanionLocomotion` already owns actual-distance
phase (1.56 m/cycle) and a 0.18-second blend. The accepted waypoint bookkeeping
fix must remain. The two new clips are 45-degree/0.75-second, Idle-zero-ended
joint-only takes; their external model-yaw curve must be applied once.

The successful isolated RT trial proves those exact takes and sampled joins,
not arbitrary-angle contact or an interruptible gameplay controller. In
particular, changing external yaw without matching the joints, truncating the
clip and blending out while a foot is in flight are not acceptable shortcuts.

## A. Resolve partial-angle feasibility before production activation

Use the existing native intake/pose checker for one finite comparison, both
directions at 2, 10, 22.5 and 45 degrees plus the zero/full endpoints. Proposed
recipe: sample the complete authored turn against frozen original Idle zero,
with amplitude `abs(angle)/45 degrees`; scale the matched exported yaw curve by
the same amount. This is a **hypothesis**, not proven foot locking. Both the
joints and external yaw change together; the full-step duration initially stays
0.75 seconds to avoid stacking retiming into this question.

Measure each support sole's world drift/height, endpoint agreement, finite PBR
frames and a single yaw application through the existing loader. Compare to the
unmodified 45-degree take; do not invent a looser contact threshold or claim the
earlier 6.856 mm measurement applies to partial blends. Inspect representative
small/mid/full turns in the existing native RT view. Keep original results.

If partial blending visibly slides or otherwise fails, stop this approach before
enabling the shared candidate. Report the specific angle/contact failure and
scope a small-angle authored derivative from the existing recipe. Do not add
world-foot IK, a large clip library, quantized 45-degree residual snaps or an
unbounded tuning loop. That is a technical asset gate, not a question for Sam
about rigs or animation IDs.

## B. One shared finite controller

In `src/gameplay/dialogue/CompanionRouteFixture.h`, separate route **intent**
from the accepted support/movement step so facing can be resolved before travel.
Keep waypoint positions/order, proof/tether conditions (six metres), endpoint
tolerance, obstacle checks and safe-support resolver unchanged.

Use four named modes: Idle, Turning, Starting and Walking. Turning may contain
a short entry-settle interval; stopping is deceleration within Walking followed
by Idle, not another general graph. Private state holds start heading, latched
signed angle, elapsed phase and current speed. Publish only the finite transient
motion recipe/phase/amplitude needed to reproduce the pose in `CompanionSnapshot`.

| Situation | Proposed behavior |
| --- | --- |
| Idle / waiting | Retain current player-facing intent, with a small hysteresis/deadband to avoid continual tiny steps. Numeric bands are trial parameters, not owner-approved constants. No stationary Walk substitution. |
| Depart / lead | Route bearing takes priority over looking at the player. Settle, turn while root X/Z stays fixed, then begin travel only when aligned. Never spend the first 0.6 m walking backwards relative to facing. |
| Partial or large angle | If gate A passes, use one complete matched-amplitude step cycle for angles up to 45 degrees. Split larger errors into completed signed cycles of at most 45 degrees, then the exact supported residual. Re-evaluate intent only at planted cycle boundaries. Use shortest-angle wrap with a deterministic 180-degree tie rule. |
| Start | Ramp from rest to the existing 1.8 m/s ceiling. About 0.2 seconds is a candidate starting point, not a fixed target. Drive walk weight from speed fraction; continue phase from actual accepted distance, retaining the existing 1.56 m cycle. No phase reset merely to hide a join. |
| Ordinary walking | Retain original Walk and path/support rules. Do not overlay a stationary turn on a translating actor. Small supported steering stays in walking; a corner requiring a large reorientation uses the same stop/turn/start path. |
| Sharp waypoint | Use the known next segment for bounded braking before arrival, retain the accepted stationary bookkeeping tick's travel-facing/phase, then turn at the supported root. Do not teleport or round the route through untested terrain. Check the existing arrival tolerance rather than creating an asymptotic never-arrive brake. |
| Obstacle / tether wait | Translation stops immediately through the existing collision/tether decision. With valid stationary support, finish the current modest foot exchange, then consume at most the latest pending intent. No growing turn queue or immediate mid-step direction flip. |
| Pause / zero delta | Freeze root, speed, phase, yaw and blend exactly. Resume the same cycle, without replaying recognition or advancing a clock twice. |
| Support/readiness loss or leaving the exterior | Freeze the transient pose rather than snapping a raised boot to Idle; suppress it through existing visibility/readiness policy. Revalidate support/generation before resuming. Load/reset retires it completely. Do not introduce a recovery teleport or fallback support. |

Settle the incoming nonzero Idle/stop pose to Idle zero before the partial-turn
recipe; its active blend is then Idle-zero -> authored turn at the chosen
amplitude. Both ends are Idle zero. Starting can use the existing two-clip blend
from that endpoint into Walk. This avoids requiring a three-way mixer or a
second vertex skin. Actual arbitrary retained walk phases remain an acceptance
gate, not an assumed natural join.

Keep Kit on the existing chapter-update clock supplied by
`GameSimulationDialogue.cpp`; do not introduce render-time advancement or alter
dialogue timing. That caller already has alive/traversing/readiness context.
Verify dead/inactive-player handling explicitly: the route currently receives
pause/exterior/proof but no alive argument, whereas acting receives alive.
Freeze the new motion when inactive; do not create Kit combat/death mechanics.
Do not infer that ordinary speech pauses Kit: existing dialogue remains live
during walking, with the current acting and source/listener anchoring.

## C. Minimal immutable render and asset seam

Exact production paths to touch if implementation is authorized:

- `src/gameplay/dialogue/CompanionRouteFixture.h`: finite motion owner and
  intent-before-travel ordering; `CompanionLocomotion.h`: speed-weighted start/stop
  while retaining phase from accepted distance.
- Proposed `src/gameplay/dialogue/CompanionTurnProfile.h`: small immutable
  duration/contact/yaw metadata derived from the owned take. Sample the exported
  yaw keys with matching interpolation; do not approximate a differently sampled
  clip with an unrelated easing curve. Keep profile/export identity checked.
- `src/gameplay/simulation/GameSimulationDialogue.cpp`: pass the required active
  context while preserving proof/acting/VO ordering. `SimulationSnapshot.h` and
  `src/vulkan/raytracing/SimulationFrameAdapter.cpp` already carry the companion
  snapshot by value; audit them rather than adding platform mutation/JNI state.
- `src/scene/CompanionPose.h`: one pure snapshot-to-clip/time/blend recipe used
  identically by serial and worker paths. Apply model yaw only through the
  existing `CompanionModelYaw` transform; simulation owns actual logical facing.
- `src/scene/assets/SkinnedMeshAsset.{h,cpp}` and `SkinnedPbrActor.h`: add named
  TurnLeft/TurnRight slots without reinterpreting production Attack/Dead. The
  loader already stores clips in a vector, but the public binding roster is
  fixed at four. Smallest additive seam: retain the legacy four-binding overload
  unchanged and add a bounded six-binding companion overload/span; preserve
  numeric values 0..3 and required/missing-clip validation. Kit's six-entry roster
  leaves combat slots unmapped and maps only its two new named slots. No
  arbitrary runtime animation catalogue or extra actor instance is required.
- `src/vulkan/raytracing/PresentableTinyRtScene.{h,cpp}`: load the admitted
  derivative, consume that recipe, and include the complete motion identity in
  both preparation and upload-cache predicates. Publish keys only on successful
  upload. Retain worker join/reload/move/destruction and GPU ownership unchanged.

Preserve the original master/runtime and all seven clips byte-for-byte. Produce
a separate append-only runtime derivative from
`assets/models/characters/source/kit-step-turn-proof-v1/native/kit-turn-proof.glb`
only after its partial-angle contract passes. Keep existing textures/materials,
weights/binds, one mesh and GPU allocation inventory. Record derivative provenance
under `ASSET_LICENSES.md`; keep source Blend/JSON/reference data out of packages.
Only the eventual admission changes the literal runtime selections in
`android/app/build.gradle`, `tools/package-alpha.ps1`, relevant existing CI LFS
fixture rosters and the production asset binding. Original files remain available.
Do not loosen `prepare-kit-runtime.py`'s original-source hash guard.

## Saves, resets and presentation retirement

Turn phase/speed/queued intent remain transient. The current checkpoint code
reconstructs Kit at Reunion or the completed Lookout; it does not serialize his
intermediate pose. Keep that rule, schema, durable dialogue flags and three-slot
semantics unchanged. Clear new fields in Reset/RestoreCampaignReunion/
RestoreCampaignLookout and advance existing generation floors. A renderer-only
surface rebind consumes the current shared snapshot without restarting the turn.

Audit `GameSimulationCampaign.cpp` and `campaign/LookoutObjective.h` while retaining
their existing grounded-root, non-travel, waypoint/progress and player-safety
eligibility. `walking` must continue to mean actual translation, not "any moving
joint." A grounded turn at the completed endpoint need not invalidate a canonical
save: the current save contract deliberately discards Kit's intermediate pose.
Do not add an animation-settled save requirement or withhold an earned durable
boundary without a demonstrated safety need. Test mid-turn save -> canonical
neutral restore, pending/failed load, reset and scene retirement for stale pose
reuse. No animation phase enters the checkpoint codec, and no LowerClaimed/
SafeExit/recovery policy changes.

## Bounded implementation and acceptance sequence

1. **Native partial-angle gate only.** Finite set above; original full-turn
   control and wrong-yaw rejection retained. If it fails, stop for the specific
   asset correction, rather than implementing a controller around bad contact.
2. **Shared state and host contracts, candidate unadmitted.** Reuse
   `CompanionLocomotionTests.cpp`, `ChapterDialogueTests.cpp`,
   `RescueJourneyIntegrationTests.cpp` and `CampaignCheckpointTests.cpp`.
   Assertions for immediate 0.03 m first-tick travel must change explicitly to the
   approved turn/start behavior; retain speed ceiling, path/support, endpoint,
   accepted bookkeeping fix, distance phase, proof/tether and durable-state tests.
   Preserve unrelated deadlines, including the open Debug route gate. Keep a
   simple isolated test selection until admission; no new player setting.
3. **Actual RT, driven by the shared controller.** Reuse the existing hidden
   nonactivating fixture, replacing hand-authored phase/yaw samples with its
   immutable output. The old scripted run remains the control. One finite set:
   left/right full and partial turns; actual 153-degree tether departure;
   both sharp route corners and one mild corner; opposite request halfway through
   a step; tether loss/regain; mid-turn pause/resume; reset/load at Reunion and
   settled Lookout; renderer rebind/retirement. Capture entry/contact exchanges/
   end and first walking ticks, with root/yaw/speed/phase/intent in the receipt.
   Include a bounded moving-target case to detect starvation or excessive
   backtracking under the unchanged tether. No foreground or phone requirement
   for this isolated proof, and no earned-journey claim from it.
4. **Admission checkpoint.** Inspect full-body and boot/shadow RT views at the
   existing level witness and a real route slope, including speaking/recognition
   and retained nonzero walk phases. Require correct heading before meaningful
   travel, no new visible sole slide/pop/snap, supported route completion,
   pause/load/reset cleanup and original straight-walk byte correspondence.
   Compare serial/worker output and bounded desktop preparation/resource cost,
   with no phone/full-FPS inference. Validate package exclusion and original
   clip/rig guards; run relevant builds plus exact-head required CI. Owner motion
   review and physical endpoint acceptance remain explicit, later gates.

## Decisions and stop boundaries

The accepted plan already calls for gentle starts/stops and turns into routes
and conversation. Immediate full-speed departure is an implementation detail,
not an owner requirement; no new approval to evaluate that change is needed.
Do not ask Sam to choose clip IDs, rig details or thresholds.

The unsettled player-facing matter is **pacing/feel**. At the current full-take
duration, a 180-degree turn can take four cycles (3 seconds), plus entry/start.
That may be too slow and can interact with the six-metre tether. Treat it as a
visible acceptance concern, not an adopted latency. First show the bounded
sequence and measured delay; then tune a proven take or propose a larger-angle
authored clip if needed. Do not hide it by snapping yaw or increasing the tether.

No new healing/save policy, route geography, dialogue trigger, town/forest/smoke
scope, benchmark, torch pose, CPU-worker adoption or general animation framework
belongs here. If acceptable partial contact, sharp-corner support or large-angle
pacing requires those broader changes, stop and return the exact limitation.
