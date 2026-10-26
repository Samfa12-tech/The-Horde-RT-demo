# Horde 1.7: full scope and remaining delivery

Current inventory reconciled against `e4de5a5ef7620c9013a29d357ebc2668be19887d`.
The original `8814e866` baseline passed all six jobs in
[CI40212337980](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/40212337980);
later checkpoints retain their own exact-head results. That is source validation, not chapter,
visual, phone or release acceptance. Work remains in the existing WP2 worktree
and draft PR27; its directory name does not limit the full 1.7 obligation.

## Authority and chapter boundary

Read the complete scope through the [accepted plan](../references/horde-1.7/d37f1638/docs/superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md),
especially sections3/4/8/10/12/13.1 and WP0–10, together with the later
[selected owner decisions](HORDE_1_7_OWNER_DECISIONS_2026_10_24.md#selected-direction),
the approved flask policy and current implementation evidence. Older plan/status
paragraphs do not override later decisions or prove integration.

1.7 delivers a coherent tomb → earned lantern → rope rescue and return → Kit
reunion/lantern lesson → moonlit forest → lookout chapter. Its modest playable
trail must sit convincingly within the wider world. It is not sufficient to
have a traversable strip with scattered trees. The regional/topographic layout,
silhouettes, route enclosure and lookout vista remain part of 1.7 presentation.

The lookout reveals a low-detail Bellwether exterior in its fixed location.
The dry northwest-to-southeast approach stays west of the river, without a new
crossing. Playable town streets/interiors, villagers/services, forge/foundry
gameplay and hub quests belong to 1.8 or later. Parked source assets for that work
are not integrated 1.7 content and must not be counted as completed scenery.

26October owner clarification: the1.7 low-cost/preloaded town must represent
the actual1.8 town with minimal shell/layout change. One authoritative placement,
scale, roofline and landmark layout must drive both representations. The approved
regional topology, coordinated R2 metric blockout and best available R17 editable
assembly are distinct levels of authority; none establishes a finished production
town master. Compare them with current v4 before town/terrain edits; sharing
the nine mapped footprints alone is not full silhouette congruence. Full residency
versus a derived distant shell/streaming is a measured choice within approved
bounds, not an assumed architecture or new budget grant. The full low-detail
world and accurate major-location presentation remain required for1.7; separate
playable1.8 content does not close a missing distant landmark.

## Current inventory

| Area | Implemented/tested evidence | Still required or explicitly held |
| --- | --- | --- |
| **Forest and wider landscape** | r23 local/macro terrain, route, grounded original 64 trees, understory/deadwood/rocks, F03 kerbs/waystone and distant village shell exist. | **Major visible gap:** surrounding woodland mass, fuller crowns, middle/far depth, region silhouettes and controlled lookout reveal. Actual F01/F03/F04 views fail the intended woodland impression. Traversal and topo import do not close this. Near/mid/far representation and combined cost/visibility need admission. |
| Cross-version town/landmarks | Normal production selects v4:966 triangles/two reused materials, eight village masses plus shrine from the common coordinate register. Existing F04 RT views show church/roofs. B06 Smith at engineX107.3/Z155.2 is a town mass. R2 supplies proposed footprints/routes/water levels; R17 is the best identified editable assembly, not a runtime town GLB. | R17/v4 detailed congruence remains unverified; roofs/terrain/LOD/collision and mill-bank/route conflicts remain unresolved. Smith is not the regional Foundry across the eastern gorge; no production Foundry shell or accepted player-height reveal is established. Local v3 is a rejected footing study and v5 a chimney-outlet prototype, not1.8 master admission. Preserve mapped geography/protected interfaces; wider terrain, woodland, shorelines and skyline remain versioned/revisable rather than wholly locked. |
| Shared/batched trees | Two source masters are reused in authoring; normal gameplay still world-bakes64 placements into four material groups (323,264 duplicated triangles). The new shared-master/BLAS owner, whole-instance retention and Windows route/recreation proof exist. | Shared64 stays OFF pending Android correctness review; cost measurements guide optimisation, not a release FPS target; Windows allocation savings are not phone/FPS gains. The extra 12-tree trial remains sparse and collision-unadmitted. Private Easy Tree and rejected pineLOD2 are not production solutions. |
| Crowns/forest art | Original masters and protected roots/wood remain. Branch-supported R4 has an independently frozen private ordinary retention/recreation and staged-route proof. | Forest mass remains unfinished; R2/R3 stay rejected and no broad rollout is admitted. Grouped F01 is source/grounding-ready but native launch is blocked by Application Control4551. The earlier exporter denial is separate; neither may be bypassed. |
| Atmosphere and story smoke | Shared night/moon environment and existing effects remain; outdoor music/ambience are integrated. | Chimney smoke is approved unfinished work: the High/Low renderer trial failed unchanged shader gates and remains withdrawn. New outdoor mist/shafts, fireflies and visual wind were explicitly deferred on 24 October, not implemented or completed. Latest owner feedback raises whether to revisit that disposition; this review does not silently reinstate them. |
| Combat/teaching | Guard→attack→guard is implemented and owner-approved on the recorded build. Current source fully blocks eligible normal attacks, retains timed parry/red avoidance and teaches four actual outcomes with protected practice/explicit safe exit. | Enemy-anchored animated special-attack rings and heavier readable red motion remain. Joint ring/flask capacity is now approved and implemented with unchanged ceilings beyond that bounded grant. The ring adapter/material path is CPU-tested; normal presentation and yellow policy stay OFF pending actual visual/gameplay admission. New-rule balance/phone feedback and full natural encounter acceptance remain. |
| Revival/cry | Private append-only stand passed native import/RT; gated shared owner passes real-outcome and restore tests plus167 actual-owner RT frames. | Normal revival remains OFF. Platform current-resource clip/cue readiness, private game asset admission and natural/listening acceptance remain. The private scream is not publicly shipped; a diagnostic pose/record is not a normal playable feature. |
| Rescue, lantern and campaign | Claim/opening, physical rope/ascent/descent, stowed owned equipment, Kit reunion/lantern lesson, forest endpoint/backtracking, hearts/subtitles/VO, three slots and checkpoint/recovery mechanisms exist. | Full natural human Keeper victory→claim→journey→earned save/load/death recovery, device lifecycle and combined residency/pacing acceptance. Seeded post-Keeper fixtures do not close the preceding journey. |
| Flask | Complete two-drink/Rest/upgrade/death/load/migration policy is approved. Backend, exact resources/schema3, gated actions and confirmed-record recovery owner/tests exist. | OFF in gameplay: native clip binding and right-arm-only target proof exist. The live utility/equipment owner is under focused validation; native RT joins, deliberate touch control, physical upgrade presentation and renderer-owner admission remain unfinished. The policy itself is no longer awaiting approval. Balance and physical feel remain. |
| Character/held-item presentation | Auto head-clearance crouch, stick speed, keyboard run, backward phase/dodge, Kit follow and stationary casual/travel turns are implemented with host/native evidence. | Useful lateral/run art and native joins remain where studies failed. Close-wall/ceiling torch containment **and** visible hand/torch framing remain a real defect/tradeoff; rejected carry trials stay withdrawn. The reported phone dark-world cause is unproven. |
| UI/menus | Subtitle repair, clearer save-slot copy, compact lantern, translucent sword/shield hierarchy, pause symbol and boot-readiness/cancel protection are implemented. | Owner-requested title/button cohesion and equal More/Settings weight; physical thumb reach, simultaneous move/look/action, rotation/earned-state continuity and accessibility acceptance. Visual polish must follow inspection of the supplied screenshot. |
| Audio/benchmark/release acceptance | Outdoor main uses the shared Pocket Audio route; owner liked that track in one phone session. Fixed-profile benchmark/rollback implementations and local reports exist. | Kit mix, movement/loop/ducking/transition listening; Windows visible/DPI/controller and Android actual benchmark sizing/return; ongoing warmed production-phone performance measurements (not a release blocker). Reports remain provisional/custom-unranked/local. PR conflicts, source-redistribution/signing and final release gates remain separate. No leaderboard/report transmission or release is authorized. |

The initial plan described four varied tree archetypes; only two source masters
currently supply the route. Variety must be judged with silhouette and grouping,
not satisfied by downloading a quota of unrelated models.

Owner clarification, 26 October: sustained phone performance is ongoing measured
optimisation, **not a release blocker and not a reason to reduce approved1.7
scope**. No target frame rate is required before release. Continue intended
forest/world/visual features with honest costs and quality options. Correctness,
save integrity, crashes, asset readiness and explicit publication approval remain
separate requirements; this does not claim performance solved or authorise release.

This inventory separates unfinished implementation from missing acceptance and
explicit deferral. It does not call 1.7 ready or turn rejected work into completion.

## Current readiness and admission decisions

The [current admission queue](HORDE_1_7_ADMISSION_QUEUE_2026_10_26.md) separates
implemented boot/four-skill/revival/flask mechanisms from their actual admission
gates. It also reconciles the separate ring and flask +1 proposals: both cannot
claim the same future owner28. A joint two-owner roster/material-packing review
is needed before a capacity decision; no limit change is approved by this note.

## Grouped F01 follow-up

[Three-stand R4 source candidate](../evidence/2026-10-26-f01-grouped-r4/README.md)
keeps the original64 and original twelve-root sparse control intact. Build and
actual-terrain host grounding pass. Windows Application Control4551 blocked the
new executable before any capture; visual/moving-camera, reflection, recreation
and supplemental collision acceptance remain open. No denied-path retry, global
shared enablement or capacity increase was made.

## Branch-supported R4 prerequisite

[Native R4 proof](../evidence/2026-10-26-forest-branch-crown-r4/README.md) resolves
the changed-geometry retention dependency for one independently frozen private
pine candidate: ordinary eviction/restore, exact pixels, recreation and staged
full route pass. Production is unchanged. The foreground-window wrapper caveat
is explicit, and actual F01 still lacks woodland mass. Grouped background content
is next; this does not close forest presentation or phone acceptance.

## Retained R3 clustered crown/F01 result

[The clustered r3 sample](../evidence/2026-10-26-forest-crown-clusters-r3/README.md)
adds real3D canopy volume within unchanged triangle/material limits. It remains
unadmitted: bulky near lobes and sparse eye-level/middle distance fail the forest
impression. Four frozen76 control/candidate recreation checks and a final tangent
recheck pass; the separate moving baked64 route retains33 completed images but
fails final unload/restoration. Exact prior-head CI is green across all six jobs.
No production tree, setting, terrain, capacity or shared64 default changed.

The failure is a content-proof dependency: changed crowns alter fully-beyond-Z24
triangle membership. A new exact reviewed safety/retention proof is needed, not
removal of its count/geometry checks. Preserve original near detail. Next useful
work is branch-supported crown detail and grouped background/lower-layer content
with shared collision/lifecycle ownership; a larger canopy or12 placements alone
is insufficient. Performance remains optimisation, not a release target gate.

## Retained R2 bounded crown result

[The same-topology r2 study](../evidence/2026-10-26-forest-crown-r2/README.md)
was rejected: fuller but flat near sprays, unchanged bare middle distance, and
no automatic static-detail unload/restoration under the unchanged safety gate.
Both isolated native runs and failures are retained; production assets remain
unchanged. Next needs genuine clustered crown/background structure, not another
leaf-enlargement or tree-quota pass. No phone target holds back that content work.

## Forest evidence and the next bounded work

The [actual-view sheet](../evidence/2026-10-23-forest-view-sheet/README.md),
[12-root comparison](../evidence/2026-10-24-f01-forest-sector/README.md) and
[equalized shared-route proof](../evidence/2026-10-24-shared-forest-route-admission/README.md)
remain distinct. Reviewed the original forest-track map and actual north before/
after pixels again for this reconciliation. Extra thin open-crown trees do not
create convincing forest mass. No fresh renderer/device run was needed to repeat
that observation; source screenshots are dated exact-build evidence, not a new
cohort. Regional terrain is present; sparse vegetation must not be diagnosed as
an entirely missing topo import or hidden short primary draw distance.

Next select **one F01/F04 near/mid/far composition**, preserving route, clearing,
landmarks and lighting, with a small owned fuller-crown/cluster sample before
another population rollout. Keep crown art and instance-count changes separately
reviewable. Respect the current export denial; source-only authoring may continue,
but runtime validation cannot be claimed until that boundary is legitimately
resolved. No new quotas, map-wide scatter, terrain wall or mist concealment.

| Distance role | Practical candidate | Evidence needed before adoption |
| --- | --- | --- |
| Near/reachable | Real3D grounded trunks, branches/canopy, original physical root contact and matching collision; shared archetypes. | Look-up/look-down/turn/backtrack, path/Kit/held-item clearance, shadows/reflections and root support. No billboard replacement. |
| Middle/enclosure | Fuller lower-detail 3D crowns/clusters, shared masters/BLAS with variation and explicit LOD boundaries. | Moving-camera parallax/overlap and transitions; actual population/metadata/material/BLAS allocation, startup overlap and paired RT cost. Sharing data does not remove traversal cost. |
| Far regional vista | First compare very simple world-fixed3D woodland masses/clumps using ordinary opaque materials and a few reusable regional references. | F04 panorama/side views, landmark gaps and terrain integration, silhouette changes under translation, secondary visibility/relevance and actual cost. Nonplayable scenery is not new playable town. |
| Far cutout cards/impostors | A bounded optional study, **not a ready drop-in or adopted plan change**. Keep nearby forest 3D. | Existing importer accepts only OPAQUE; alpha-masked foliage needs a real shared material/traversal extension and consistent primary/shadow/reflected/transmitted rejection. Avoid camera-facing shadow rotation, flat side views and duplicated imagery. Compare overlap/candidate filtering/texture/mip/BLAS/TLAS cost against simple 3D, not triangle count alone. |

Distant cards can still be genuine RT geometry; they are not inherently a raster
fallback. However a transparent PNG on an opaque quad is not supported cutout
foliage, and a painted forest skybox would not satisfy the accepted 3D contract.
Khronos describes explicit acceptance/rejection of non-opaque ray candidates in
the [traversal specification](https://docs.vulkan.org/spec/latest/chapters/raytraversal.html)
and [alpha-masked foliage guidance](https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Advanced_Topics/Ray_Query_Reflections_and_Transparency.html).
No claim that cards are faster on this phone follows from those APIs.

Terrain-led sightlines are approved: ridges/bends/rock faces can limit the views
that must be convincing while preserving the topo and bounded lookout reveal.
Visual hiding alone saves neither residency nor RT shadow/reflection work.
Separate primary visibility, secondary reach, moon/local-light influence and
resource retention; prove turning/backtracking/transition recovery. Current104
TLAS storage already fits exactly the 76-tree trial, not arbitrary extra trees.
Grouped opaque far geometry may offer a different cost tradeoff, but neither
that nor any capacity increase is admitted by this review.

## Current access/coordination boundary

Reef owns the laptop foreground/controller; no Horde phone lease. No foreground,
controller, ADB or device action was used for this review.151 parked files and
campaign/settings remain separate.

The menu screenshot `libfile_ffcd448368e48191936b8eed98dd8568` could not be
materialized through the unchanged required Library helper: the scoped network
attempt failed before a file existed; the authorized network attempt then reached
metadata application and failed because native Windows Python lacks
`os.setxattr`. No final image exists at the requested destination. No metadata
helper bypass, alternate transfer or visual edit was attempted. Menu visual work
needs a supported image handoff; source inspection alone does not count as having
seen the screenshot. This blocker does not prevent the scope/forest assessment.
