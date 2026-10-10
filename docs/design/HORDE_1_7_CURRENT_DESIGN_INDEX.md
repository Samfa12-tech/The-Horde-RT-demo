# Horde 1.7 current design index

Reconciled 11 October 2026 against implementation checkpoint
`59eb44c692783e71362aa7dd068ba462703bcdc5`, the current uncommitted scene/subtitle/tree
repairs, and the owner's later decisions. This index is the entry point to the
full design, not a replacement master or a claim that all of 1.7 is complete.
The continuing development branch is `codex/horde-1.7-wp2-vertical-proof`,
[draft PR27](https://github.com/Samfa12-tech/The-Horde-RT-demo/pull/27).

## Authority and preserved sources

The owner's current instructions and explicit later decisions govern execution.
The full pinned design supplies the chapter requirements; current engine/asset/
validation contracts govern implementation and evidence. Dated implementation
reports describe their own candidates. WP0 summaries and old checkboxes are
navigation aids, not substitutes for the full master.

The following are byte-preserved copies from planning commit
`d37f16381311c6e620f0c4a9d01c1cce83260dc8`. PR16 has not been merged or imported as
implementation. Existing abbreviated documents and implementation notes are
unchanged. Historical “planning only”, “recording approval pending” and
“unimplemented” statements describe that snapshot; they do not revoke later
implementation or distribution authority. Historical tentative counts also do
not enlarge today's admitted budgets.

| Reference | Current use |
|---|---|
| [Full master, all sections and work packages](../references/horde-1.7/d37f1638/docs/superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md) | Behavior, transitions, acting, atmosphere, UI, persistence and acceptance requirements; read in full |
| [Full WORLD_LAYOUT](../references/horde-1.7/d37f1638/docs/WORLD_LAYOUT.md) and [regional image](../references/horde-1.7/d37f1638/docs/design/world/the-horde-world-layout.png) | Approved topology; illustrative image is not a metric survey |
| [Full ASSET_PLAN](../references/horde-1.7/d37f1638/docs/ASSET_PLAN_1_7.md) | A01–A17 and T01–T06 reuse/production register and admission requirements |
| [Connected map set and detailed notes](../references/horde-1.7/d37f1638/docs/design/world/connected/README.md) | Forest, town terrain, services and river/mill continuity; all four map sheets visually inspected |
| [Coordinates](../references/horde-1.7/d37f1638/docs/design/world/connected/shared-coordinate-layout.json), [building register](../references/horde-1.7/d37f1638/docs/design/world/connected/building-feature-register.csv), [generator](../references/horde-1.7/d37f1638/docs/design/world/connected/build_maps.py) | Editable metric proposals; preserve originals and smooth versioned production derivatives |
| [Selected UI boards](../references/horde-1.7/d37f1638/docs/design/ui/README.md) | Physical lantern/carved stone direction; painted loading bar superseded by small spinner |
| [Kit and separate player animation matrix](../references/horde-1.7/d37f1638/docs/KIT_ANIMATION_PLAN_1_7.md) | Reuse existing clips first; do not claim missing directional/acting families complete |
| [Reference intake](../references/horde-1.7/d37f1638/reference-intake.json) | Original paths, Git blobs, actual payload hashes/sizes and LFS identities for every copied file |

The copied historical documents retain their original relative links and text.
For an adjacent document outside this selected reference tree, resolve its
original path using the intake's pin at
[the immutable planning tree](https://github.com/Samfa12-tech/The-Horde-RT-demo/tree/d37f16381311c6e620f0c4a9d01c1cce83260dc8).
Do not run the archived map generator in place: it writes beside its source.

## Later decisions and current scope

| Earlier wording or assumption | Governing later decision |
|---|---|
| Preparation-only WP0 and unchecked master tasks | Implementation has been explicitly authorized in bounded milestones. The accepted 1.6.2 baseline remains protected; no wholesale historical branch import. |
| Voice casting/listening/public distribution pending | Owner approved 13 exact Kit cuts and explicitly confirmed public game/repository distribution rights. `59eb44c6` admits speech through existing chapter audio. This does not approve every canonical dialogue line or a blanket MIT asset license. |
| Warden only a rights-gated candidate | Owner supplied the exact model and confirmed public Horde redistribution. Actual native import/skinning passed. Visible Kit still needs integration and acting/physical acceptance. |
| Meshy stump public entitlement unresolved | Owner confirms ownership and paid-plan creation, resolving public Horde redistribution for this exact stump. Meshy terms §3.2 were checked and the pinned text-only provenance read. Record creator-owned paid-plan output; no blanket MIT or automatic CC0 license is asserted. Native scene/performance admission remains separate. |
| Fourth simultaneous character assumed necessary | Owner intends Keeper disappearance before above-ground Kit. Reuse the dedicated third slot through explicit actor/zone lifetime ownership; do not assume a fourth slot or 19th texture layer. Current dead-Keeper visibility does not yet implement disappearance. |
| Texture ceiling 16 | Owner approved only the bounded ceiling of 18 for the original pine/alder families. All frozen shader cost/structural limits remain unchanged. |
| Static or permanently resident exterior blockout | Rope is a seamless preparation/commit/retirement gate in both directions. This agrees with master §§4.3/5.1 and WORLD_LAYOUT. CPU staging with shared GPU residency does not complete loading/unloading. |
| Immediate rope appearance / no required throw acting clip | Owner wants a pause after the call “Stay clear. Rope coming down”, then a physically unfurling rope. Existing physical deployment is accepted in a scoped Windows playtest; no on-screen warning is requested and no visible Kit throw animation is required by current staging. |
| Tutorial/normal subtitle plaque | Owner requests compact edge text, no default large backing, no flicker. Android Auto: bottom for keyboard/mouse/controller, top for touch; touch landscape bottom only when controls genuinely leave space. Explicit Top/Bottom and per-line stability remain. |
| Mist interval investigation | Deferred by owner. Preserve correct floor shadows and production rendering. Post-Keeper mist clearing is an art mitigation, not a repaired volumetric shadow. Outdoor M/F markers are not effects or cost evidence. |
| Conservative sword contact proxy | Remains inactive. Forgiving unobstructed contact is provisional; authored masonry correctness protection remains. Arbitrary 3D shaft occlusion and final contact calibration are not proven. |
| Waiting for Eric's terrain package | Superseded on 11 October: that task never started and no package is forthcoming. This job owns versioned Blender terrain authoring from the supplied topo and runtime integration, with one implementation writer. Preserve existing masters and use the measured interfaces below. |

Current authorized work is the complete tomb-to-woodland chapter: scene integrity,
subtitles, topo-authored continuous terrain, admitted woodland dressing, visible
Kit, and actual two-way GPU preparation/retirement at the rope gate.
The full master is the design target, not automatic permission for every later
work package. No new acquisition/generation, release, merge, budget increase,
device installation or campaign-save implementation follows from this index.

Current repair milestone: [11 October scene repair evidence](../evidence/2026-10-11-scene-repair-milestone/README.md). It records the tested three-character/18-layer repair batch, dialogue path and HUD corrections, inspected frozen Windows backends, and the unfinished terrain/Kit/streaming gates. This supersedes the table below only where its dated evidence explicitly says so.

## Reconciled implementation and remaining work

“Implemented” below identifies code at the named checkpoint, not blanket device
acceptance. The [wooded chapter report](../superpowers/plans/2026-10-11-horde-1.7-wooded-chapter.md)
remains historical: its missing-payload and pending voice-rights statements are
superseded by the recoveries and owner decisions above. Its measured results
remain scoped to its exact source/artifacts.

| Full design area | Completed or established foundation | Remaining gap / current repair |
|---|---|---|
| WP0 / accepted 1.6.2 | Protected runtime, dependency inventory and baseline/torch receipts; no release rollback | Keep exact-build evidence and failures; never turn read-only analysis into passes |
| WP1 / asset contracts | Native Blender/import route, source/runtime separation and negative fixtures | Selected sources still need their individual material, scene, cost and owner gates |
| WP2 / support, movement | Shared fixed-step height, coherent camera/body/hands/items/light/listener transport; walk/run, route support and safe-side generations | Real GPU zone preparation/unloading/fence retirement; final terrain support correspondence; directional clip families and both-platform lifecycle acceptance |
| WP2a / combat teaching | Simulation-owned dodge protection, parry/riposte, readable Keeper charge and short damage-aligned burst, optional teaching slowdown | Contact calibration remains provisional; moving-target/wall/corner evidence and exact-candidate owner/Android/backend gates remain explicit |
| WP4 / tomb continuation | Lantern-gated rescue, same night, separate opening/lid, no legacy ending takeover; small backed niches and dressing | Portal underside and occupied-tomb intrusion repairs are uncommitted; waterfall light change and unexplained live two-heart observation remain open |
| WP5 / physical rope | Anchored gravity/contact deployment, two-way traversal, both hands/grips, stowed gear and physical light, grounded restore and safe-side recovery | Current owner round-trip feedback is scoped; full platform/backend/capture/lifecycle gates and final motion polish remain |
| WP6 / Kit and speech | Logical terrain-following companion and bounded line/queue/generation ownership; 13 offline cuts admitted at `59eb44c6` | Visible Kit, safe third-slot/texture handoff, foot/stride/turn/acting review. Missing voice no longer explains an invisible actor. |
| WP7 / UI | Existing controls/preferences, Voice gain, subtitle size/position/skip and themed lesson presentation | Compact stable Windows captions and Android input-aware Auto are in working changes, not physically accepted; selected full physical-menu theme and low-power pause are not completed by captions |
| WP8 / forest and water | Winding F01–F04 route, distant shell blockout, wet step authority/audio, bounded water contact work, existing adaptive music | Actual continuous terrain envelope; admitted dressing/LOD placement; visible Kit; full combined moon/mist/firefly/wind workload. Wet steps received positive feedback, but splash/ripple visibility was not established. |
| WP3 / production budgets | Source counts and CPU resident/staged preparation evidence | Sustained exact-target CPU/GPU, skinning/AS/upload/memory/overlap/hitch measurements, real LOD and separate relevance policies; current ABI bounds are not device capacity |
| WP9 / durable recovery | Session/reset/retry and logical safe-side recovery | Three-slot durable Save/Load, atomic storage/migration and process-death checkpoint acceptance; not implemented or authorized by current repairs |
| WP10 / final acceptance | Historical host/package checks and scoped Windows owner feedback retained | Finished chapter art/acting, exact final Android and Windows RT presentation, motion, UI, lifecycle, audio/haptic and combined workload acceptance |

Both guards and Keeper must remain correctly rendered together in the tomb,
including corpses/backtracking. The authorized three-character change is in
working changes, reusing existing pose/BLAS resources. It has not yet completed
final native/package/device validation. Current `EvaluateCharacterFramePlan`
still regards every non-Dormant Keeper phase, including Dead, as visible; the
owner's disappearance/Kit reuse needs an explicit subsequent lifetime change.

Seamless gates must prepare forest during reward/approach and tomb on the return
approach; commit only collision/render-ready destinations; retain the shared
upper room/shaft/rope/rim/moved stone/immediate clearing and off-camera ray/light
contributors; retire only after GPU completion. Failed preparation keeps the
last safe side. This design does not mean unloading the whole tomb at rope grip,
discarding corpse state or using camera visibility alone as residency policy.

## Map correspondence and terrain handoff

The supplied revision-2 bundle contains the actual planning heightfield omitted
from the pinned repository. All 15 manifest entries passed byte/hash checks.
Its forest and terrain PNG decoded pixels exactly match the pinned lossless
WebPs. The four pinned sheets and regional concept were visually inspected.
Northwest tomb → southeast trail → F04 lookout → fixed distant Bellwether is
preserved. Bellwether is nonplayable in 1.7; no town relocation, giant road wall
or playable later dungeon follows from the maps.

Existing correspondence: `X = East + 78.3`, `Y = Up - 31.55`,
`Z = 126.2 - North`. F01 is the safe landing, not shaft centre. Proposed route
length is 76.264196 m and maximum straight grade 7.269005%; these do not certify
smoothed terrain/collision. The 0.5 m grid is not final collision or permission
to move the accepted tomb. See the [measured integration handoff](../superpowers/plans/2026-10-11-topo-terrain-integration-handoff.md)
for F01–F04 transforms, shaft/apron keepouts, all nine occupied tomb volumes,
support/eye conventions and export/material/resource requirements for this job.

## Actual asset disposition before substitutions

[Actual-byte receipt](../evidence/2026-10-11-design-reconciliation/asset-bytes.json)
records current hashes, GLB/Blender/WAV payload checks and package-manifest
verification. Native outcomes below refer to actual earlier intake runs; byte
presence, importer success, scene admission and physical acceptance are separate.
No replacement master was generated during this reconciliation.

| Supplied source | Verified payload and rights | Native/integration disposition |
|---|---|---|
| T03 original funerary kit | Seven GLBs plus seven Blender masters match manifest; 5,460 source triangles | Existing tomb uses validated native derivations. Preserve bowls, urn pieces, lid and cold nonemissive candles; not missing. |
| T02 skeletal source kit | Seven GLBs, two Blender sources, all 55 checksum entries verified; Gord Goodwin CC0 geometry notice retained | Skull, femur, humerus and coarse alternatives exist. Existing admitted derivations are distinct from animated enemies; whole-core topology is not a collision volume or production enemy. |
| Original pine/alder pair | All 48 archive payloads match; original Horde authorship retained | Original mapped-normal GLBs lacked tangents. Separate Blender derivatives passed native import without replacing masters. Ten real-scale placements/four texture families are in uncommitted integration; final checks/visual cost remain pending. |
| Original forest dressing | All 59 manifest payloads match: ferns, rocks, log/root kit and editable sources | Two ferns imported; mapped-normal candidates need tangents. Not absent and not automatically admitted. |
| Dead snag | All 18 manifest payloads match; Quaternius Nature CC0 geometry, original bark; mismatched supplied license-copy heading recorded honestly | Actual GLBs present; mapped-normal tangent repair and placement/LOD/native visual cost remain, not a license to regenerate the master. |
| Meshy stump | All 18 manifest payloads match, plus unlisted README | Native import passed. Owner now confirms this exact stump was created on the paid plan and may be publicly reused; the rights blocker is resolved. Scene/material/placement/cost admission remains pending; retain origin/repair metadata and record paid-plan creator ownership. |
| Exact Kit Warden | 7,034,964 bytes, SHA-256 `47e249840ed9dd438da386137257050a5bd463e7fe54bf94873e1fd21715835e`; owner confirmed public Horde rights | Native 24-joint model, seven clips/35 sampled skinned poses passed; idle height 1.8 m. Source candidate only; no visible Kit or automatic additional capacity claimed. |
| Approved Kit cuts | All 13 actual runtime WAVs match exact policy hashes; mono PCM16 24 kHz; owner distribution confirmation recorded | Speech integrated at `59eb44c6`. Canonical English IDs/text/state/schema remain authority, not older prose. Fresh listening/mix/device review remains pending. |
| Chapter terrain | Planning topo/grid/map bytes present | This job now authors a new versioned Blender master and native derivative. No external delivery is pending. Production terrain/support and inspected RT acceptance remain unfinished. |

The [Meshy terms §3.2](https://www.meshy.ai/terms-of-use) and [official commercial-use guidance](https://help.meshy.ai/en/articles/9992001-can-i-use-meshy-assets-commercially-license-copyright-explained) were checked on 11 October 2026. The [pinned stump provenance](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/e6cf2e97e5577aa051c5a18014477598762d7b75/assets/source/beyond_the_tomb/forest_dressing/stump-v01/RIGHTS.md) records original text-only generation with no third-party model/image. Its historical account-verification gap is closed by the owner's paid-plan confirmation; it is not a current blocker.

Across the four forest archives, 15 original GLBs were actually inspected by the
native importer: three accepted and twelve rejected for missing tangents on
normal-mapped materials. The two selected tree derivatives then passed. Those
results do not certify final textures, all-ABI packaging or GPU performance.
Original masters/backups and unrelated untracked candidates remain untouched.

## Current feedback and verification boundary

Latest owner feedback accepts niches 1/2, teaching slowdown, wet-step sound,
Keeper wind-up/death cue and restored lantern on an earlier tested Windows
candidate. It also identifies large/flickering subtitles, the threshold underside,
tunnel/Keeper intrusions, missing Kit and unfinished exterior. Earlier reports
carry exact build identities. Acceptance does not transfer to current working
changes or to Android/Query/lifecycle tests automatically.

Before a new playtest checkpoint: finish affected native/package checks for the
working repairs; render matched portal/tunnel/Keeper/landing views through actual
RT; verify subtitle line stability and three-character retention; then integrate
terrain against the reviewed topo and measured interfaces. Preserve all failed evidence, shader ceilings,
quality and the original torch sweep/180-second policy. No placeholder or source
hash can count as art, sustained performance or physical presentation acceptance.

This reconciliation changes documentation/reference custody only. Audio/haptic
manual revalidation required: **NO for this diff**; the wider changed speech,
combat/water mix and next gameplay candidate retain their pending owner gates.
