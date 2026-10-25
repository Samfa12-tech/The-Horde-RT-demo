# Attacker-ring scope before implementation

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
ordinary attacks need a distinct, uncluttered baseline. Full normal blocking and
yellow guard-break remain a balance proposal, not an implemented rule change.

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

The concrete capacity question is pending: authorise a measured opt-in single
ring owner (ordinary/metadata 28 to 29, TLAS capacity 104 to 105, static assets 13
to 14, one additional immutable BLAS), or keep current bounds and assess another
honest RT ownership route. Primitive/material headroom must be established before
any further change. The question does not authorise broader effects, extra
enemies, a quality reduction or admission without evidence. Independent startup,
encounter and tutorial work continues while that decision is pending.

Refinement of that original question: the validated 64-placement route proof
uses 92 TLAS instances, so one ring would be 93 and fits the existing 104 limit.
The 105 request is needed only to retain simultaneous support for the unadmitted
76-tree sector study already using 104. It is not required merely to test a ring
on the 64-placement route. Metadata/static ownership and the full primitive/
material roster remain separate unresolved bounds; this count is not a measured
GPU cost or permission to change any limit.
If a dedicated ring primitive/material is required, the concrete additional
bounds would be37/33; they are not approved merely by authorising an owner slot.
Do not disguise format conversion, material aliasing or an inactive instance as
free capacity. The later flask prop must still have honest simultaneous ownership.
