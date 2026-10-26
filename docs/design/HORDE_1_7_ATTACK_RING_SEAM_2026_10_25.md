> Current authority, 26 October: Sam approved the exact JOINT warning28/flask29
> layout for implementation/evaluation. Normal enablement remains OFF. The
> standalone +1 figures and pending language below retain their historical
> context and do not override the [joint decision](HORDE_1_7_JOINT_RING_FLASK_ADMISSION_2026_10_26.md).

# Attacker-ring scope before implementation

**Combined request supersedes the standalone roster below:** use the
[joint warning28/flask29 proposal](HORDE_1_7_JOINT_RING_FLASK_ADMISSION_2026_10_26.md).
Its exact counts/packing sequence and14 rejected offline cases are recorded;
no capacity or normal enablement is approved. Older +1 figures remain history
for the single-feature discussion, not a simultaneous implementation request.

26 October reconciliation: consult the [joint ring/flask admission queue](HORDE_1_7_ADMISSION_QUEUE_2026_10_26.md#ring-and-flask-proposals-must-be-reviewed-together).
The +1 request below describes this feature alone. The two features cannot both
claim owner28 or assume one +1 increase covers simultaneous ownership. No bounds
are changed or approved. Existing immutable material sharing is a possible proof
path, not an established flask saving.

Later owner approval authorizes normal/full block, yellow/guard break and
red/avoid. The [shared rule/practice checkpoint](../evidence/2026-10-25-four-skill-practice/README.md)
implements the authority independently; new yellow attacks remain OFF pending
the ring. Earlier proposal wording below describes this note's original state.
The specific owner/TLAS/static/primitive/material capacity question is still
pending and no bounds or rendering path were changed.

Sam's 25 October follow-up selects an in-world animated ring associated with the
attacking enemy, rather than a text-led HUD alert. Explanatory teaching/settings
text remains accessible. Existing normal/yellow/red gameplay authority and the
owner-accepted guard-to-attack-to-guard transitions remain unchanged. The parent's
research supports enemy rings but did not verify a universal 2018 ring-closure
equals parry-frame contract. Competitor timing does not define Horde's impact or
parry window, and colour does not introduce guard-breaking or other unapproved
mechanics. Sam's latest direction reserves coloured rings for special attacks;
ordinary attacks need a distinct, uncluttered baseline. The later approved rule
checkpoint implements full normal blocking and tests yellow guard-break behind
an explicit diagnostic option; normal gameplay keeps yellow OFF until its ring
is admitted. This note does not enable that option or alter the attack rules.

## Current source constraints

- `CombatTeaching.h::EvaluateAttackReadability` supplies the actual current
  Skeleton attack/Keeper charge classification. Attack ownership is singular;
  the surviving enemy and source identity must remain correct after handoff.
- `GameSimulation` owns 60 Hz attack phases and contact; the renderer consumes
  immutable snapshots through `SimulationFrameAdapter`. Do not animate a separate
  cosmetic countdown from UI wall time or infer an impact from colour.
- `RtSceneAbi.def` currently has 28 ordinary/metadata owners, 104 TLAS capacity,
  13 static assets, 36 primitives and 32 materials. The ordinary owner roster is
  populated. Inactive legacy/player/torch aliases are not free slots to borrow.
  The earlier flask presentation proposal also needs a deliberate owner seam.
  The retained shared-route recreation receipt already uses36/36 primitive rows,
  32/32 material rows and13/13 static assets. That is a known tested shared
  profile, not a fresh inventory of the default baked profile. Preserving that
  option leaves no proven spare primitive/material row for a new ring.
- `PresentableTinyRtScene` already supports static-PBR/emissive mesh BLAS with
  per-frame instance transforms. A real bounded ring mesh can use this route;
  a projected HUD circle, screen-space halo or fake lighting is not equivalent.

## Smallest candidate to review

Use one original reusable annular mesh/material and one transient effect owner
for the singular authoritative attacker, with stable source identity and real
world placement. It must participate in ordinary ray visibility/occlusion and
have deliberately reviewed reflection/shadow behaviour. Preserve full required
actors and existing effects; do not alias a hidden-but-owned slot or increase ABI
capacities silently. Inventory actual primitive/material headroom and the joint
flask/effect ownership need before requesting a specific bounded capacity change.

Publish a narrow attack-presentation record from existing snapshot phase data:
source, actual phase, phase elapsed/duration and authoritative contact boundary.
The ring's closing/emphasis must use that state. Interrupt, death, successful
parry, neutral recovery, room exit and pause/reset must clear it coherently;
holding guard cannot restart it. Decide ring onset/closing semantics against
Horde's own reaction windows, with windup/contact separate from the defensive
window. Do not label every currently parryable normal swing a special merely to
show a yellow ring.

Yellow and red need different shape/motion as well as colour; reduced motion
retains a static distinct shape and no misleading countdown. Preserve the normal
baseline without adding rings to every idle enemy. Heavier red enemy motion is
still a separate unadmitted requirement: the earlier Keeper cast trial stays OFF
until a new bounded visual/cost comparison supports it.

## Admission evidence required

Compare actual RT dark-room silhouettes, near/far enemies, overlap and source
handoff, occlusion and camera re-entry; inspect portrait/landscape and reduced
motion. Align warnings with real windup, active/contact and interrupted phases,
including lesson disabled. Record added geometry/material/BLAS/TLAS allocation,
CPU update and matched GPU/frame costs. Windows evidence does not establish the
Android budget. Do not change quality or effect defaults to disguise that cost.

No ring/runtime/ABI change is made by this note. A small unintegrated HUD-shape
draft was superseded and retained only under ignored reports; it is absent from
the production and test source trees. Artwork is original, not copied from the
reference game. No phone operation is needed for this source assessment.

## Exact pending capacity proposal (25 October follow-up)

Request one additional reusable warning owner, one immutable annulus asset/range
and one independently mutable material, preserving all current scene profiles:

| Bound | Existing | Proposed |
| --- | ---: | ---: |
| Instance metadata | 28 | 29 |
| Unique static assets | 13 | 14 |
| Primitive rows | 36 | 37 |
| Material rows | 32 | 33 |
| TLAS definition capacity | 104 | 105 |
| Ordinary/shared BLAS owner maxima | 21/23 | 22/24 |

**This whole proposal remains pending; no bound is changed.** The primitive
and material increments are explicit, not implied by approving an owner slot.
The later flask still needs honest simultaneous ownership of its own.

### Actual combat and multiple attackers

One owner is sufficient for TODAY'S authoritative combat, not a general promise
about multiple attackers. `SwordCombat.h::SelectAttacker` retains one living
non-Locomotion token through windup, active, recovery and stagger. Non-owner
attack phases are reset; death releases the token. `GameSimulation` selects
Skeleton versus Keeper exclusively, and inactive Keeper combat becomes Dormant.
Thus the two current skeletons cannot require two simultaneous special warnings.
The opt-in every-third-attempt yellow policy stays OFF pending ring admission.

Clear/rebind the ring atomically using the immutable warning source and its
phase/generation. Interruption, death, parry and neutral recovery clear it;
held guard cannot restart it. A future coordinator that permits simultaneous
warnings needs a separately measured pool/capacity revision and coverage for
both sources. Validation must detect that changed contract, not silently drop
the second warning or borrow a live actor/prop owner.

There is no existing instance tint. A dedicated global ring material can switch
its own yellow/red factors because the normal shared path already uploads
`frameMaterials` each frame (`PresentableTinyRtScene.cpp`). This reuses ordinary
emissive material shading, not an extra light/fire slot or a shader object branch.
Two simultaneous differently coloured owners could not share that mutable row
without another reviewed material/ownership arrangement.

### Source roster, actual allocation and proposed cost are different

The normal scene source uses 28 TLAS definitions; shared64 is 28+64=92, and the
retained76 study is 28+76=104. Adding the owner gives 29, 93 and 105. One ring on
shared64 therefore fits the current 104 TLAS capacity, but that does NOT preserve
the full retained76 profile. The 105 request avoids silently dropping placements
or aliasing a masked owner. Normal geometry uses 32 primitive rows; the tested
shared roster uses36. Final material rows are already32 (30 packed plus2 world),
and ordinary unique static assets are13. Shared instancing does not free these
metadata/material requirements.

The current actual RT two-skeleton-room pose control at 960x540/High reports
21 allocated BLAS, one TLAS, 28 TLAS definitions, 52 buffers and63 allocations.
This is genuine renderer inventory in an immutable pose fixture, NOT a live
multi-attacker fight or a measurement of an unbuilt ring. Earlier shared-route
receipts have different own-build live counts; neither those nor source owner
maxima are a new ring allocation query.

A 16-segment annulus prototype would contain32 vertices/32 triangles: 2,048
vertex bytes and384 index bytes, plus known payload rows of16 primitive,
128 material,32 metadata and64 TLAS-instance bytes. These are proposed payloads,
not driver allocation/peak residency or frame-time deltas. BLAS/TLAS backing,
scratch, alignment, update work and RT screen-coverage cost require real queries
and paired captures after approval. Geometry/shape/readability is still a visual
gate; the proposal does not admit a faceted or inaccessible final warning.

No new texture, light, emitter, shader path or reduced rendering quality is
assumed. Required ABI/bank regeneration, full actor/effect preservation, dark-room
and overlap legibility, actual phase timing, Windows cost and later authorized
Android warm-device evidence remain admission gates. The fresh pose control and
its limits are recorded in the [native stand gate](../evidence/2026-10-25-skeleton-stand-native/README.md).
