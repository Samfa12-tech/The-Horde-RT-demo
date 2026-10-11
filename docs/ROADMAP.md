# The Horde - shared roadmap

**For Sam, Eric and Codex. Reconciled 11 October 2026.** This is the shared source of truth for each update's purpose, approved scope, boundaries, status, acceptance gates and detailed plans. A planned version is not a release promise or permission to begin every work package.

## Authority and maintenance

Record each new owner scope decision here, under its update or **version unassigned**, with its date, status and a link to the detailed plan. Update the relevant plan and source index in the same documentation change; do not create a competing master scope list. Record implementation and acceptance separately. When an update closes, link its exact release receipt and move unfinished work to an explicit deferred entry. Sam resolves product scope; Eric and Codex keep the plans and evidence linked. Merge approved documentation through the normal review process so main remains the shared reference.

Use the [source inventory and reconciliation](ROADMAP_SOURCES.md) to find full plans, maps, dialogue, assets, animation, music and dated evidence. Later explicit owner decisions supersede conflicting historical wording; a newer implementation report only proves the candidate and checks it names. [AGENTS.md](../AGENTS.md), [PROJECT_DECISIONS.md](../PROJECT_DECISIONS.md), the [engine contracts](AGENT_ENGINE_CONTRACTS.md) and [validation guide](AGENT_VALIDATION.md) retain engineering authority. The [Phase Plan](PHASE_PLAN.md) retains history.

The [linked world-map atlas](design/world/atlas/README.md) locates the preserved local sheets within the regional topography. It adds navigation only; metric blockout and unresolved tomb/treasury alignment retain their existing proposal limits.

| Status | Meaning |
|---|---|
| Published / release receipt | Released package and exact evidence, including limitations. |
| Approved scope / decision | Owner-selected requirement or direction; implementation and acceptance may still be pending. |
| Implemented, acceptance pending | Work exists at a named candidate; host checks, images or a scoped playtest do not certify the whole update. |
| Future agreed direction, version unassigned | Preserve the agreed goal without allocating it to an update. |
| Proposal | Reviewable implementation, count, verse, layout or optional idea; no commitment. |
| Unresolved decision | Explicit choice or evidence gap that must be settled before the affected work is accepted. |

## Update overview

| Update | Purpose and current status |
|---|---|
| 1.6.1 | Published engineering baseline; preserve receipts and deferrals. |
| 1.6.2 | Published engine readiness and tomb/demo polish, 9 October 2026. |
| 1.7 - Beyond the Tomb | Approved tomb-to-woodland chapter; draft PR27 is in progress and unaccepted as a complete update. |
| 1.8 - Bellwether, NPCs and light RPG systems | Approved bounded hub direction; detailed implementation scope follows accepted 1.7. |
| Proposed 1.9 - Drowned Abbey | Approved theme, reflector and item-required Bellkeeper; delivery label provisional. |
| Proposed 1.10 - Ashen Foundry | Approved theme, shuttered stand and item-required Master of Coin; delivery label provisional. |
| Proposed 1.11 - Glass Court | Approved theme, focusing aperture and item-required keeper encounter; delivery label provisional. |
| Version unassigned | Treasury/hopeful return, earned recap, later combat-feel pass and retained future investigations/proposals. |

Abbey and Foundry are playable **in either order**. Proposed delivery numbering is not a compulsory play order, date or commitment. Court follows both seals. The existing tomb is the prologue, separate from the three later dungeons.

## 1.6.1 - published engineering baseline

**Purpose and disposition:** complete the engineering programme, genuine RayQueryCompute compatibility, adaptive music, player/reporting foundations and accepted corrections. Published and merged; owner confirmed completion on 3 October.

**Plans and receipts:** [engineering plan](ENGINEERING_1_6_1_PLAN.md), [completion reconciliation](ENGINEERING_1_6_1_COMPLETION_REPORT.md), [handoff](ENGINEERING_1_6_1_HANDOFF.md), [signed release validation](SHOWCASE_ALPHA_1_6_1_RELEASE_VALIDATION_2026-10-03.md).

**Boundaries and remaining evidence:** accepted performance is as recorded, with no sustained-30-FPS claim. Historical S24 final coverage was deferred and exact S25 was then unverified; later 1.6.2 field reports are separate evidence. High-glass defects, directional locomotion and other deferrals remain in the unassigned backlog below. Older prospective 1.6.1 start gates must not reopen completed work.

<a id="16-2--engine-readiness-and-demo-polish"></a>

## 1.6.2 - Engine readiness and demo polish

**Status:** accepted within its recorded limits, merged and published on itch and as the [GitHub v1.6.2 prerelease](https://github.com/Samfa12-tech/The-Horde-RT-demo/releases/tag/v1.6.2), 9 October. Package version 1.6.2 / Android code 10; frozen validated source `db62032d9ab54ebaa5bd12d17dc7e987fc8f54c9`.

**Purpose and accepted scope:** the post-release audit corrections, usable Graphics choices/preview, themed menus/HUD, shared equipment transitions and player rag torch, accepted collapse/opening/tomb polish, two waterfall guards, Keeper reveal/audio, dust/mist choices and Android controller/lifecycle work. The receipts decide what shipped; the historical update bank never promised every investigation would ship.

**Plans and acceptance evidence:** [executable goal](IMPLEMENTATION_GOAL_1_6_2.md), [tomb finish/disposition](ENGINEERING_1_6_2_TOMB_FINISH.md), [final technical review](ENGINEERING_1_6_2_FINAL_REVIEW_2026_10_09.md), [signed validation](SHOWCASE_ALPHA_1_6_2_RELEASE_VALIDATION_2026-10-09.md), [itch publication](evidence/2026-10-09-release-publication/README.md), [GitHub closeout](evidence/2026-10-09-github-release/README.md), [device compatibility record](ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md). The [source inventory](ROADMAP_SOURCES.md#published-updates-and-historical-plans) links UI, material, collapse, reveal, controller and experiment detail.

**Accepted limits:** fresh/reset mobile defaults remain 50%, Mobile water/fire, Glass Off, Current shadows, cap 30, Mist On and Dust Low; desktop stays 100%, High and Glass On. Preserve saved/custom preferences. 33%/40% are experimental. Current forgiving combat was retained with its documented contact limitation; precision animation/contact correction, walking torch-arm wiggle, shafts, seamless music handover and long sustained phone testing were deferred. Neither a sustained 30 FPS result nor completed Eric independent audit is claimed. Publication closed the release action gates; retained technical/device gaps remain explicit.

<a id="graphics-menu-and-measured-quality-choices"></a>
<a id="authored-graphics-preview-room"></a>
<a id="material-depth-foundation"></a>

**History:** the [original main roadmap and full 1.6.2 update bank](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/0ebd608b619217696e6b00fbab263905b0684387/docs/ROADMAP.md) preserve every older checkpoint, proposed control and investigation, including [Graphics choices](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/0ebd608b619217696e6b00fbab263905b0684387/docs/ROADMAP.md#graphics-menu-and-measured-quality-choices), the [preview room](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/0ebd608b619217696e6b00fbab263905b0684387/docs/ROADMAP.md#authored-graphics-preview-room) and [material depth](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/0ebd608b619217696e6b00fbab263905b0684387/docs/ROADMAP.md#material-depth-foundation). Earlier default-Off, planned-release and pre-publication hold statements describe their dates, superseded by the receipts above. Existing section links land here so historical plans remain discoverable without becoming new scope.

## 1.7 - Beyond the Tomb

**Purpose:** turn the existing tomb into a deliberate prologue, then deliver a physical rescue, visible Kit reunion and short moonlit forest journey ending at the lookout toward Bellwether.

**Approved update scope and locked decisions:**

- Recontextualize/remove the opening material-display arrangement while retaining shared material systems and technical fixtures; burial-purpose niches and restrained decay have a world purpose. Kit's optional concerned call comes through the small wall grate; the silent player retains control. Preserve distinct openings, both sworn guards and lantern reward after Keeper defeat.
- Shared vertical support/movement, walk/run and material-aware footsteps; coherent body, hands, equipment, emitter and listener. Focused combat timing/parry/riposte and parry/dodge teaching, readable Keeper wind-up/lightning and short dodge protection belong to 1.7. Exact tuning and contact calibration remain evidence-dependent; optional teaching slowdown is separate from later earned perks.
- Separate reward/rescue opening, physical gravity/contact rope deployment after the warning pause, burial-shaft slab/lid and grounded stake/tree tie-off. Both sword and lantern are visibly stowed for two free hands; light follows the physical lantern. Real ascent/pull-up and descent restore equipment only at a safe grounded landing.
- Prepare and retire zones safely in **both directions**, with no normal rope loading screen. Retain shared shaft/rim/rope/clearing and off-camera RT contributors; readiness precedes traversal commitment, failure preserves a safe side, GPU completion precedes retirement. CPU readiness/shared resident buffers are not completed streaming.
- Visible, collision-aware Kit with restrained acting, offline speech, route waits and a fresh manual lantern raise before the reunion response. No mandatory Kit combat, tomb escort or escort-failure mechanic. Current owner decisions permit the exact supplied model and 13 Kit cuts; this is not approval of the whole future voice bank.
- Continuous authored night, visible moon aligned with genuine RT-occluded illumination, topo-derived terrain and bounded 3D woodland, real detail levels, bounded participating mist/fireflies/wind and chapter music, and a fixed distant Bellwether shell. Normal gameplay stops at the lookout; the village is not playable in 1.7.
- Selected physical-lantern/carved-stone UI direction, readable stable subtitles and separate Dialogue gain, simpler phone Graphics/help, low-power pause and heart/accessibility presentation, reusing accepted systems. Durable Save/Load/New Game and safe reward/summit/endpoint recovery are update requirements. **Three independent local campaign slots are owner-requested; decision to reconcile:** the older master/WP0 records call the count provisional while the current PR27 index lists three-slot durable recovery. Align those records before WP9 rather than silently downgrading the request. Manual-save-at-checkpoint versus arbitrary safe-position saving remains unresolved. Scope the first usable recovery method and its saved health/resource representation through the master's small WP0 slice before WP9; initially preserve three health unless that slice explicitly changes it. Wider health growth/flask/food/rest balance remains later direction, not a committed healing economy.

**Detailed authority:** [full 1.7 master](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md), [current PR27 design index](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/c7c0d24cfcc7aeb1dc033563ca29a6d0f6f189d4/docs/design/HORDE_1_7_CURRENT_DESIGN_INDEX.md), [asset register A01-A17/T01-T06](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/ASSET_PLAN_1_7.md), [Kit and player animation plan](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/KIT_ANIMATION_PLAN_1_7.md), [WORLD_LAYOUT](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/WORLD_LAYOUT.md), [maps, UI, dialogue, score and work-package evidence](ROADMAP_SOURCES.md#17-plans-production-and-pr27-evidence).

**Implemented, acceptance pending:** at the indexed [draft PR27](https://github.com/Samfa12-tech/The-Horde-RT-demo/pull/27) snapshot, vertical/combat foundations, tomb teaching, physical two-way rope, logical companion and 13-cut speech exist. The [11 October scene-repair receipt](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/c7c0d24cfcc7aeb1dc033563ca29a6d0f6f189d4/docs/evidence/2026-10-11-scene-repair-milestone/README.md) records bounded three-character/18-layer repairs, caption/audio-path/HUD fixes, scoped Windows backend images and actual checks. Its Debug torch run missed the 180-second gate; the unchanged Release run passed. The later [exact-commit CI run on 3c682619](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/38096549617) adds route/support/rescue host coverage; it does not establish device acceptance. Where the index still calls those repairs uncommitted, the subsequent receipt governs that batch only.

**Current execution boundary:** PR27 owns the continuing world-building work. Its unfinished terrain/support and woodland composition, visible Kit and Keeper-to-Kit lifetime handoff, actual two-way GPU preparation/retirement, final moon/occluder consistency and grounded anchor/lid presentation must not be marked complete. Campaign-save implementation is not authorized by the current repair package merely because it is in the update master. Mist interval issue #28 is owner-deferred. The new later combat-feel direction below is not automatically part of 1.7.

**Acceptance gates:** finish and inspect the complete tomb-to-lookout journey, fresh raise/reunion, return and second ascent; safe restore/process death when persistence is implemented; both hardware RT backends, supported phone touch/orientations/controller and Windows input/UI; exact-candidate motion, audio/haptics and lifecycle checks. The master's six outdoor gates require cheap geography/hero-view proof, one representative combined workload, real 3D LODs, separate camera/secondary/shadow/light relevance and residency, and measured sustained frame/memory/transition budgets before expansion. Green host CI, imports or old scoped owner feedback cannot close whole-chapter acceptance.

## 1.8 - Bellwether, NPCs and light RPG systems

**Owner clarification, 11 October 2026:** consolidate this update around **Bellwether and NPCs plus light RPG systems**. The useful, compact hub is approved direction; begin concrete implementation only after 1.7 is made, tested and accepted and its actual systems/budgets are reconciled.

**Approved scope and boundaries:** extend the same road from the 1.7 lookout into the same unwalled Bellwether. Keep the central tavern/well, northern church/active graveyard, western gardens, eastern river/mill, southeast smith and nearby starter-shrine spur. Selected interiors, a small cast, useful conversation/services and repeat returns make it a place worth visiting. The Veyrlands, Bellwether and King Veyr are approved names; unpulled-bell folklore is setting lore, not a promised supernatural bell system.

Bounded light RPG direction means ordinary treasure currency, scarce named components, gear/training/upgrades and access preparation through mixable **Magic, Tech and Constitution**. Preserve viable mixed builds, independently sufficient access alternatives, recoverable named-relic sell/return choices and meaningful hub consequences. Exact service menus, prices, XP/skill-point/respec rules, crafting, component quests and rewards remain unresolved or proposed. No automatic general inventory grid, simulated economy, universal quest engine or extra currencies.

Kit is injured on the approach and recovers based in Bellwether, remaining a useful major-return partner. Exact injury/cause/timing is undecided; no baseline later dungeon/treasury escort or unexplained healing. Tavern/service roles may combine into a bounded cast: caravan-leader keeper, record/shrine keeper, artificer/smith, salvager and relic dealer. Supporting names remain provisional; avoid Mara. Natural dialogue remains distinct from the optional ballad.

Approved village-life direction includes brief closed-privy/rag humour, a man in a hole with contradictory unverified rumours, domestic animals and kitchen plots. [The appendix](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/BELLWETHER_VILLAGE_LIFE.md) proposes counts, placement, favour mechanics, extra gags and lines; these are not locked production packages. Preserve humane ambiguity, food/water/waste coherence and clear routes. No farming, hunger, breeding, crowd/daily-schedule, crime or livestock-combat systems are implied.

**Plans:** [full village plan](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/superpowers/plans/2026-09-11-village-hub-1.8.0.md), [campaign canon/progression](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/CAMPAIGN_DESIGN.md), [connected town/river/mill maps](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/design/world/connected/README.md), [English bank and state contracts](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/dialogue/en/README.md), [source inventory](ROADMAP_SOURCES.md#18-bellwether-and-campaign-authoring). Approximately 6-10 visible buildings, 2-3 interiors and 4-7 cast are authoring envelopes, not quotas or measured simultaneous actors; the maps' eight principal masses are coordinated references whose metric details need blockout testing.

**Explicit reconciliation:** older main's village plan says mythology is undecided and excludes all shops/economy. Later approved campaign and full village sources supersede that wording: narrative direction is approved and bounded spending/services/training are part of the useful hub. Broad economy/crafting and detailed scripts are still uncommitted. Later dungeons, bosses, full optical framework, multiplayer, weather/day cycle and general open-world streaming remain outside 1.8.

**Acceptance gates:** first accept one arrival-to-tavern stress slice with representative actors/fire/glass, exchange, checkpoint, return outdoors and repeat visits, using the six-gate contract's hub gate. Expand only after combined residency/LOD/RT relevance and phone/Windows budgets pass. Then prove admitted interiors and NPC interactions, subtitle/audio fallbacks, progress-aware returns, spending/reward/save integrity and viable alternative progression. Final service selection, cast/interiors, quest details, injury staging and numeric budgets must be scoped before acceptance. No 1.8 implementation or release acceptance is claimed here.

## Proposed 1.9 - Drowned Abbey

**Status:** approved setting/item/boss-use direction; provisional delivery label, no date. **Purpose:** a small access quest plus complete drowned-religious-house expedition, seal, plunder evidence and safe hub return.

**Locked scope:** river-fed southern lake, broken-tower vista, short submerged approach with independently sufficient Magic ward, Tech equipment or finite trained breath; predominantly dry walkable rooms. Acquire the persistent placeable reflector early, teach shutter/redirection puzzles and require actual reflected-light exposure of Bellkeeper armour. Award one peer seal and evidence with a living claimant; Abbey works before or after Foundry without its tool/upgrade.

**Plans/maps/assets:** [chapter chain](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/DUNGEON_CHAPTER_PLANS.md), [Abbey staged plan](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/DROWNED_ABBEY_PLAN.md), [dungeon atlas and 106-package planning register](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/design/world/dungeons/README.md), [archived enemy/source candidates](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/assets/source/beyond_the_tomb/README.md), [dialogue/reveal ledger](ROADMAP_SOURCES.md#18-bellwether-and-campaign-authoring).

**Proposals/open choices:** flood cause/date; exact section/air-pocket geometry; guided versus bounded free steering, breath limits and retreat controls; reflector placement/recall rules, room/encounter/timing/boss model and budgets. Free lake swimming, boats, underwater combat, water-level puzzles and fluid simulation are not automatic scope. Existing waterfall optics do not certify a lake or underwater camera. E01/E02/E05 archived source trials are not shipped enemies or accepted boss animation.

**Acceptance gates:** vista/descent/crossing/dry-room blockout with credible floodline/air volumes; all three recoverable access routes both ways; bounded lake optics with analytic waves first and FFT only if a measured need justifies it; combined combat/light/water/residency slice; complete learned reflector-to-boss chain, one persistent seal, retreat and return; exact Android/Windows motion, sustained budgets and lifecycle/recovery. Static atlas checks are planning evidence only.

## Proposed 1.10 - Ashen Foundry

**Status:** approved theme/item/boss-use direction; provisional delivery label, no date. **Purpose:** access complication and complete royal mint/forge expedition, peer seal, binding evidence and useful return.

**Locked scope:** eastern gorge/terrace; independently sufficient Tech bridge repair, Constitution maintenance climb or Magic crossing direction. Cursed labour and machinery explain the place. Acquire a persistent shuttered lantern stand early; learn leaving/controlling light while moving. Actual maintained illumination/mechanism use and repositioning expose the Master of Coin's furnace shell. Foundry works first without Abbey's reflector/seal; heat/smoke safeguards remain recoverable.

**Plans:** [Foundry approach-to-return chain](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/DUNGEON_CHAPTER_PLANS.md), [atlas/floorplan/item and production registers](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/design/world/dungeons/README.md), [canon](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/CAMPAIGN_DESIGN.md), [dialogue and score sources](ROADMAP_SOURCES.md#18-bellwether-and-campaign-authoring).

**Proposals/open choices:** room geometry, machine cycles, watchers/light-shadow rules, exposure/filters/wards, boss model/actions and budget. No unbounded machinery, fluid or separate combat engine. **Acceptance:** prove access/retreat, recoverable stand placement/shutters and save state, safe lesson/escalation, visible hazard/query agreement, combined machinery/actor/fire/smoke costs, item-required boss, single seal and either-order narrative/hub return.

## Proposed 1.11 - Glass Court

**Status:** approved theme/item/boss-use direction; provisional delivery label, no date. **Purpose:** mirrored palace/winter-garden culmination after both peer seals, revealing the false royal identity and treasury location.

**Locked scope:** northeast high ridge; both Abbey and Foundry seals plus authored access interaction. Acquire the focusing aperture early, teach bounded receiver/mirror/ward selection and require its actual use to break keeper protection before defeat. Earlier tools may combine because both peer chapters precede Court. Keeper defeat is required; death is not automatically required. Dry treasury lies beneath/behind this ridge, separate from the Abbey lake.

**Plans:** [Court chain and shared chapter gates](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/DUNGEON_CHAPTER_PLANS.md), [Court atlas/floorplan/register](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/design/world/dungeons/README.md), [canon/evidence reveal](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/CAMPAIGN_DESIGN.md), [decision-blocked dialogue](ROADMAP_SOURCES.md#18-bellwether-and-campaign-authoring).

**Proposals/open choices:** detailed access alternatives, ascent/rooms, finite optical rules, keeper speaking interval/fate/staging, model/actions and budgets. No infinite mirror recursion, unexplained boss-only rule or alternate ending. **Acceptance:** two-seal gate cannot be bypassed; retreat/recovery and cross-tool save/restore work; complete taught aperture-to-boss chain, final seal and essential truth remain accessible if speech is skipped; combined palace/garden/keeper RT and exact-platform gates pass.

## Version unassigned - future agreed directions and proposals

### Treasury, final confrontation and hopeful return

**Approved campaign direction:** real desirable treasure, a deceptive lantern entity claiming to be King Veyr, release of the bound dead with the dangerous consequence of freeing the entity, defeat through established tools, and one hopeful playable Bellwether return. Compact finale, not a promised fourth full dungeon or alternate keeper ending. [Canon](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/CAMPAIGN_DESIGN.md) and [dialogue/reveal/knowledge contracts](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/dialogue/en/README.md) preserve fair clues and Kit's town-based reunion. Exact entity form/name, ritual/counterplay, distribution, losses and final staging are unresolved; optional final-boss voice is unapproved. Acceptance must prove learned counterplay, actual reveals, recovery and return without invented abilities or cross-save spoilers.

### Heathen-inspired combat-feel pass

**Owner-approved future direction, 11 October 2026; version unassigned, not automatically 1.7.** Use [Heathen's Steam presentation](https://store.steampowered.com/app/4702700/Heathen/) and the supplied [developer interview](https://www.gamespark.jp/article/2026/06/30/168630.html) as inspiration. Begin with **one sword and one skeleton**: readable preparation, fast contact and follow-through; synchronized hit reaction and material-specific sound; distinct miss, bone, block and parry feedback; visibility around hands and lantern. Test at **normal speed on phone with touch and Backbone**, then against several enemies. Keep the shared simulation, exact-candidate feedback and measured renderer limits. A kick or shove remains an **optional idea**, not committed scope. This research does not verify Heathen's input responsiveness or collision implementation; its systems are not proof of Horde's eventual feel. Detailed timing, contact model, multiskeleton scope and admission gates require the later bounded plan.

### The Tale Thus Far

**Approved concept:** an earned rhyming old-fable recap and loading excerpts of events actually experienced. [Full design](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/TALE_THUS_FAR.md) preserves either-order stanzas, earned knowledge, choice truth, save isolation and accessible factual recap. Exact verses, title, presentation, narrator/voice and production packages are proposals. Its suggested 1.7/1.8 slices require a scope decision; no silent addition to those updates. Small spinner only, no artificial load/recital delay or future-title/spoiler leak. Acceptance follows real earned events and restored saves, not version/seal count alone.

<a id="continuing-performance-work-after-162"></a>

### Retained engineering, presentation and progression backlog

| Direction / status | Detailed home and acceptance boundary |
|---|---|
| Continue performance work - agreed, unassigned | [Future investigation](PERFORMANCE_INVESTIGATION_FUTURE.md) and [dated audit](MOBILE_PERFORMANCE_AUDIT_2026-10-04.md): bounded historical regression, texture fetch versus classification, mips/footprints, actual 33% versus 50% phone/upscaling, contextual lights and possible Low mist. Preserve selective-opacity NO-GO, unproven Lower gains and deferred production FSR/LOD; reconcile later experiment receipts before selecting another run. No guaranteed FPS/savings/default change. |
| High glass and device/platform follow-up - retained deferrals | [Technical backlog](../FUTURE_WORK.md), [glass witness records](evidence/2026-10-02-high-row43/README.md), [device record](ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md): unresolved contact/receiver and near-edge defects, exact-device coverage, selection/enumeration/diagnostics hardening. Retain failures and separate reported field results from complete acceptance. |
| Animation, health/recovery and visual polish - agreed direction with proposed detail | [1.7 master](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md) owns directional-motion, hand/clearance and scoped health/save foundations, including the small first usable recovery/save-representation decision. Later capacity growth, wider flask/food/rest/healing, earned parry/focus perks and balance are not all 1.7 gates. [Historical roadmap](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/ROADMAP.md) retains carried-torch ember detachment as a reported, unreproduced investigation; [technical backlog](../FUTURE_WORK.md) retains stone-corner polish, visible feet/look range and walking-arm concerns. |
| Optional effects - proposals within their stated gates | [1.7 master and asset plan](ROADMAP_SOURCES.md#17-plans-production-and-pr27-evidence): reactive vines/water response, RTAO/RTGI and extra atmosphere detail remain bounded proposals, with current mist interval work deferred. No automatic new effect, actor ceiling, ray budget or simulation. |
| Expanded adaptive score - authored review candidates | [Score archive/manifest](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/assets/source/beyond_the_tomb/music/adaptive-score-review-2026-10-06/README.md) and [integration gaps](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/assets/source/beyond_the_tomb/music/adaptive-score-review-2026-10-06/ENGINE-GAPS.md): compositions/previews do not replace the accepted eight-cue bank. Bass/editor parity, section-end joins, stems/ducking, state and device listening remain separate. |
| Engine as core product - agreed strategy; packaging proposed | [Preserved product proposal](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/ROADMAP.md#engine-product-and-demogame-packaging-proposal): demo/game targets over shared modules are a proposal; target/module/repository/package/save/version policies unresolved. Preserve old release artifacts and do not delay 1.7 for speculative extraction. |
| Optional Windows path tracing - maybe one day | [Preserved feasibility brief](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/ROADMAP.md#maybe-one-day--optional-windows-path-tracing): static integrator proof, then motion/history/denoising and measured transport scope; existing hardware RT remains baseline. No version, device preset or promised integration. |
| Optional PC VR - last, after game completion | [Preserved PC VR brief](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/ROADMAP.md#maybe-one-day--optional-pc-vr-very-last): uncommitted possibility; PSVR2 ownership does not certify PC setup. No PS5 port promise. |

## Common design brief for each later dungeon

The [shared chapter contract](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/d37f16381311c6e620f0c4a9d01c1cce83260dc8/docs/DUNGEON_CHAPTER_PLANS.md) owns approach/access, early local-item acquisition, safe teaching, escalation, **actual item-required boss use**, one persistent treasury seal and safe return. Utility items and seals have different roles. Main-route retreat works before the seal; optional cross-item secrets and post-seal shortcuts never become hidden mandatory dependencies. Test both peer orders, all allowed access alternatives, placement/recall, spending, death/retry and restore before acceptance.

## Ray-traced light as a gameplay pillar

Light causally changes what the player discovers, traverses, exposes or defeats. Puzzle truth uses bounded world-space source/surface/blocker queries under shared fixed-step authority, agreeing with visible RT geometry. Screen brightness, noise, render FPS, resolution or a proximity trigger cannot stand in for a real optical path. Preserve readable feedback across supported quality tiers and recoverable tool/puzzle states. This does not start a general optical framework in 1.7/1.8.

## Reuse suitable Briarhold assets

Assess suitable existing sources before producing replacements, while preserving Horde identity. Verify exact rights, provenance, scale/rig/material/import/collision and combined phone cost; availability, source QA and another game's performance do not constitute Horde admission. [Asset/source inventories](ROADMAP_SOURCES.md#17-plans-production-and-pr27-evidence) and [ASSET_LICENSES.md](../ASSET_LICENSES.md) own the detail. Dungeon registries are production proposals, not asset orders or runtime capacity.

## Decisions intentionally left open

The update sections identify their own unresolved choices. Shared open decisions include reconciling the owner-requested three-slot Save/Load count with older provisional records, the bounded first recovery/save-health policy, supporting names/voice casting, exact injury, prices/XP/respec/health balance, item/query/encounter rules, measured content budgets, finale/entity staging and demo/game packaging. Approved geography, named dungeon items/boss-use, either-order progression and single-ending canon are not open for silent reinvention. Signing, release and other external actions require their own authority; this documentation does not grant it.
