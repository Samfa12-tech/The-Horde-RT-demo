# Horde 1.7 feature status - 18 October 2026

Working folder: `.worktrees/horde-1.7-wp2-vertical-proof`, existing
`codex/horde-1.7-wp2-vertical-proof` branch. This inventory now includes the
completed host save/session bridge, Android three-slot menu/native integration,
Android Back confirmation and bounded crouch/root implementation recorded in the
linked receipts. Implementation and
physical acceptance remain separate. This document adds no feature authorization,
device claim or new milestone ceremony. Later tested receipts supersede older
dated observations below. The
[current design index](HORDE_1_7_CURRENT_DESIGN_INDEX.md)
remains the chronological evidence entrypoint.

## Authority and status meanings

The scoped authority is the preserved [Beyond the Tomb plan](../references/horde-1.7/d37f1638/docs/superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md),
particularly experience sections 4-8, controls/hearts section 10, saves section 12
and work packages WP0-WP10 in section 15, reconciled with the latest confirmed
owner direction forwarded by the coordinating parent on 18 October. That trusted
reconciliation supersedes older plan exclusions where stated below. Dated gap
notes and unchecked boxes describe their original snapshots, not later
code/evidence. The [phase plan](../PHASE_PLAN.md) is predominantly historical
engine/release history, not a current 1.7 completion ledger.

Latest confirmed owner direction, forwarded in this task on 18 October:
**three independent slots, each retaining one manual record and its latest
autosave; autosave never replaces manual; no history archive**. This supersedes
the preserved plan's earlier unresolved retention proposal. Manual arbitrary
position snapshots are not implied. Do not ask Sam to decide retention again.

Other firm 1.7 directions from the same reconciliation: accurate low-detail world
and major terrain locations including rivers/lakes; a distant Bellwether shell;
licensed free forest species ambience; Kit voice; character-named top/bottom
subtitles; hearts; automatic small-root stepping/mantle and crouching;
alternating hands and low-ceiling sword clearance; Android Back exit confirmation;
removal/fix of broken Low shadows (issue 23); reduced paused mobile heat/drain;
deliberate environmental storytelling in place of the opening material collection;
and simpler mobile graphics with optional explanations and one Apply/save action.
Small tomb-floor stone/rubble/debris is also a firm visual requirement. Nearby
kick reaction is desired where bounded and feasible, with static/instanced distant
scatter; this does not authorize full physics for every stone. Free jump/slide,
skill-tree/dodge-slash combos, playable town, PC VR/path tracing, vine physics and
RTGI remain exploratory, deferred or outside 1.7. Later approved Mureka Kit WAVs
supersede the earlier subtitle-only restriction; no new paid generation is needed.

| Latest decision | Older statement it supersedes |
| --- | --- |
| Root stepping/mantle and crouching are firm | Old low-clearance posing-only/no-crouch exclusions do not reject the later request. Free jump/slide remain deferred. |
| Kit voice includes the later approved WAVs | Earlier subtitle-only/source-only restrictions do not describe current authority or implementation. |
| Three manual plus three latest-auto records | Retention is decided; old plans' open choice is no longer a Sam blocker. |
| Distant shell plus accurate low-detail regional geography | Neither a playable 1.7 town nor a trail-only substitute for approved rivers/lakes is intended. |
| Storytelling and small floor debris | Opening material collection is to be replaced; conditional nearby kick reaction does not imply unlimited physics. |

In the table, **implemented** means a production/shared code path exists;
**partial** names missing integration or acting; **unaccepted** means current
physical/owner acceptance is not established, even where older scoped evidence
exists. These labels do not turn all remaining proposals into required features.

## Playable feature map

| Feature | Actual code and evidence | Current disposition / remaining work |
| --- | --- | --- |
| Lantern reward, same-night opening | [GameSimulationRescue.cpp](../../src/gameplay/simulation/GameSimulationRescue.cpp), [RescueLidFinish.h](../../src/scene/RescueLidFinish.h), plan 4.2-4.4 | Implemented claim/opening/night continuation. Preserve the displaced lid, equipment/light sockets and retired Keeper on repeated trips. Whole-route art/physical acceptance remains open. |
| Rope ascent and return descent | [RescueTraversal.cpp](../../src/gameplay/traversal/RescueTraversal.cpp), [simulation integration](../../src/gameplay/simulation/GameSimulationRescue.cpp), [RescueJourneyIntegrationTests](../../tests/RescueJourneyIntegrationTests.cpp), plan 6 | Implemented safe-side motor, deployment, grip targets, stow/restore and readiness gates. The [owner clearance observation](../evidence/2026-10-09-wp2-windows-owner-clearance-review/owner-observation.json) confirms returning hands only; it explicitly does not accept all directions, gear, lifecycle or audio. Physical full out/back acceptance remains separate. |
| Kit visible model and route | [CompanionRouteFixture.h](../../src/gameplay/dialogue/CompanionRouteFixture.h), [Kit scene admission](../evidence/2026-10-11-kit-scene-admission/README.md), plan 7 | Visible approved Warden derivative, real PBR, native-grounded idle/walk, waiting/turn behavior and shared Keeper-to-Kit actor allocation are implemented. This supersedes old 'Kit missing/source only' rows. Speaking/lantern reaction acting, stride/turn quality and full route acceptance remain partial. |
| Kit VO and narrative | [ChapterDialogue.h](../../src/gameplay/dialogue/ChapterDialogue.h), [GameSimulationDialogue.cpp](../../src/gameplay/simulation/GameSimulationDialogue.cpp), [admitted audio disposition](../evidence/2026-10-11-wooded-chapter/asset-disposition.json), plan 7.3-7.4 | Thirteen approved Kit cuts, stable line IDs, grate/rescue/reunion/trail progression, ordered continuation and fresh presented-raise proof exist. Playback/listening/mix/subtitle interruption acceptance is open; do not regenerate already admitted VO. |
| Hearts and health | [PlayerVitals](../../src/gameplay/ShowcaseGameplay.h), [Android heart drawable](../../android/app/src/main/java/com/samfa12/hordelanternrt/VitalityHeartsDrawable.java), [Windows layout](../../src/platform/windows/WindowsVitalityHudLayout.h), plan 10.5 | Three-heart authoritative health and both platform presentations exist. Logical saves preserve 1/2/3 current health. A usable healing method, consumable state and permanent upgrades are not implemented by this slice; the plan leaves their first bounded policy unresolved and does not require a full healing economy. |
| Subtitles, line skip and Voice gain | [shared layout](../../src/gameplay/dialogue/SubtitleLayout.h), [Windows dialogue helper](../../src/platform/windows/WindowsChapterDialogue.h), [Android MainActivity](../../android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java), [chapter validation receipt](../evidence/2026-10-11-wooded-chapter/receipt.json) | Character-named captions, independent persisted Voice control, subtitle enable/size/Auto-Top-Bottom/skip, generation-owned playback and placement policies exist. The receipt explicitly retains physical audio/haptic and whole-chapter acceptance gaps. Current bright/dark/DPI/font/touch and mute combinations need acceptance, not another subtitle architecture. |
| Parry/dodge/riposte teaching | [CombatTeaching.h](../../src/gameplay/simulation/CombatTeaching.h), [CombatFoundationTests](../../tests/CombatFoundationTests.cpp), [WP2a closeout](../evidence/2026-10-10-wp2a-closeout/receipt.json), plan prologue teaching and WP2a | Shared timing, cues, safe practice, skip/replay, optional 0.25 teaching time scale and existing defensive mechanics are implemented. WP2a is explicitly not fully complete. The receipt provisionally retains forgiving contact for first playtest and records seeded wall-contact defects whose production reachability is unproven. First-time learning, live contact/pacing and supported touch/controller feel remain acceptance work; later skill-tree combos/perks remain proposals. |
| Walking, run and outdoor pace | [GameSimulationConfig](../../src/gameplay/simulation/GameSimulation.h), [UpdateMovement](../../src/gameplay/simulation/GameSimulation.cpp), [InputSnapshot](../../src/gameplay/simulation/InputSnapshot.h), plan 5.5 | Implemented analog/diagonal movement, explicit run hold/toggle and interruption/reset logic. Current defaults are 1.9 m/s walking and 3.2 m/s running with acceleration. No automatic indoor/outdoor speed switch is required: the plan explicitly chooses player-controlled running. Actual route timing, stopping/clearance, Kit timing and owner comfort remain unaccepted tuning; do not silently increase speed or add stamina. |
| Player directional motion / low-clearance hands | [PlayerAnimationState](../../src/gameplay/animation/PlayerAnimationState.h), [held-item kinematics](../../src/gameplay/items/HeldItemKinematics.cpp), [directional rig test](../../tests/PlayerDirectionalRigTests.cpp), [rope hand alternation](../../src/gameplay/traversal/RescueTraversal.cpp) | Directional rig fixtures, `SwordOverheadLowering` and alternating rope grip advancement exist. Full direction/speed/dodge acting, held-prop clearance and physical acceptance remain bounded gaps. Crouching is now firm owner direction; the old plan's exclusion no longer decides its scope. |
| Root stepping/mantle and crouch | [shared motor](../evidence/2026-10-18-small-root-motor/README.md), [playable crouch/root checkpoint](../evidence/2026-10-18-crouch-playable/README.md), [movement](../../src/gameplay/simulation/GameSimulation.cpp), [clearance](../../src/gameplay/traversal/ChapterTerrainMovement.h) | Implemented bounded 18 cm stepping, 1.8/1.2 m posture, blocked standing, camera/held-item/listener lowering and grounded moving-body pose. Touch toggle, Ctrl hold and controller LB/L1 hold share simulation authority. One original 10 cm trail root uses the same 20 triangles for RT and STEP geometry. Save restore clears transient stance/input; rope traversal owns its separate pose. Physical route/comfort/audio acceptance and sustained performance remain open. Free jump/slide remain deferred. |
| Free jump, slide and arbitrary auto-hop | [InputSnapshot](../../src/gameplay/simulation/InputSnapshot.h), [GameSimulation](../../src/gameplay/simulation/GameSimulation.h), old plan 5.5/5.7 | No general jump/slide command or motor is implemented. Free jump and slide are deferred exploratory ideas. Automatic small-root traversal above is firm and must not be conflated with arbitrary jumping or unrestricted auto-hop. |
| Themed touch/desktop controls and menus | [Android MainActivity](../../android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java), [Windows owner](../../src/platform/windows/DiagnosticWindow.cpp), plan 10 | Existing movement/look/action controls, mobile Run, native settings/preferences and controller routing are implemented. Full theme/layout/accessibility/one-hand-held-input acceptance remains partial. Android PLAY now selects a New Game slot; Entry > More and pause expose Load, and pause exposes manual Save. [Java menu evidence](../evidence/2026-10-18-android-save-menu/README.md) covers separate manual/latest-auto rows, confirmation, cancellation and input/camera handoff. Windows Save/Load adapter/menu and death-screen Load entry remain incomplete; physical touch/controller/load acceptance is separate. |
| Pause power policy | [ForegroundPauseRenderCadence.h](../../src/graphics/ForegroundPauseRenderCadence.h), [policy test](../../tests/ForegroundPauseRenderCadenceTests.cpp), plan 10.3b | Implemented policy suppresses redundant foreground-paused Showcase RT frames, with 500ms fallback and wake/invalid-output handling. It claims no FPS/power result. Final physical device pause/wake/battery acceptance is distinct from source-level policy. |
| Android Back / graphics simplification / Low shadows | [MainActivity](../../android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java), [admitted choices](../../android/app/src/main/java/com/samfa12/hordelanternrt/GraphicsPreviewOptions.java), [retired-tier receipt](../evidence/2026-10-11-retired-lower-shadows/README.md), [cycle correction](../evidence/2026-10-18-mobile-shadow-cycle/README.md) | Firm 1.7. [Back confirmation](../evidence/2026-10-18-android-back-confirmation/README.md) now uses a native Stay/Quit dialog at both top-level boundaries, with Stay focused, stale/lifecycle actions revoked and 60 focused Java checks passing. [Sealed current-build SM-S948B checks](../evidence/2026-10-18-android-back-device/README.md) pass Back-key/touch Stay/cancel, Home/resume revoke, explicit Quit and relaunch. Gesture/controller/large-font acceptance remains open. Graphics already has optional Details, but separate Use/Keep remains: single Apply/save is missing. Low shader/settings tier was retired; the remaining ordinary Higher-to-invalid-0 crash is corrected locally, reproduced before the fix and covered by 38 passing graphics UI/preference tests. No quality/default change or physical shadow-cycle acceptance is claimed. |
| Woodland and atmosphere | [ChapterTerrainSurface](../../src/scene/ChapterTerrainSurface.h), [ForestDressing](../../src/scene/ForestDressing.cpp), [understory](../../src/scene/ForestUnderstory.cpp), [deadwood](../../src/scene/ForestDeadwood.cpp), [grounding evidence](../evidence/2026-10-11-woodland-grounding-clearance/README.md), plan 4.6/8 | Authored connected trail, imported support, grounded dressing and native route are implemented. Full combined moon/mist/firefly/wind, final composition and physical warm pacing are not accepted. Existing canopy/Easy Tree and mist issue28 deferrals remain; more heavy outside content is not the next default task. |
| Regional terrain, river/lake representation | [approved geography](../references/horde-1.7/d37f1638/docs/WORLD_LAYOUT.md), [terrain admission](../../src/scene/ChapterTerrainAdmission.h), [terrain surface](../../src/scene/ChapterTerrainSurface.h), [Bellwether shell](../../src/scene/BellwetherShell.cpp) | Accurate full low-detail world and major locations are firm 1.7; a connected trail and distant shell alone do not prove that contract. Existing `chapter-terrain-v1` ground admission and local stream/contact code need an explicit approved-map versus runtime survey of river/lake silhouettes, location/distance and lookout sightlines. No full-world acceptance established here. Playable town remains 1.8. |
| Forest species ambience | [native audio integration](../../android/app/src/main/cpp/android_probe_bridge.cpp), [Windows audio integration](../../src/platform/windows/DiagnosticWindow.cpp), [asset licences](../../ASSET_LICENSES.md) | Firm 1.7 and missing in this inspection: source/asset searches found no admitted crickets/cicadas/owls/frogs forest beds or water-proximity frog route. Existing waterfall/menu-room beds and Kit forest-named speech do not satisfy it. Requires free commercially safe provenance, bounded shared zone/proximity policy and actual listening/mix acceptance; no paid acquisition authorized. |
| Opening storytelling and tomb-floor debris | [TombDressing](../../src/scene/TombDressing.h), [scene contract](../AGENT_ENGINE_CONTRACTS.md#renderer-and-assets), latest owner direction above | Tomb niches/burial dressing exist, but a deliberate opening composition replacing the material-tech-demo collection and small floor stones/rubble/debris are firm remaining art work. Nearby kick reaction is conditional on a bounded design; use static/instanced distant scatter, sleep/cull inactive pieces, preserve traversal/nav/combat readability and deterministic save behavior. Require baseline/candidate frame and RT geometry cost; optional detail setting if cost is material. No unlimited per-stone physics or paid assets. |
| Water contact and surface feedback | [WaterContact](../../src/gameplay/effects/WaterContact.h), [WaterContactTests](../../tests/WaterContactTests.cpp), plan 5.5/6.1 | Shared bounded wet-step, stream-entry, drop/ripple state and semantic ownership exist. This is not acceptance of splash/ripple visibility or mix on current phone. The plan's vines/contact additions are optional measured exploration; do not require a new destruction/physics engine. |
| Town / lookout endpoint | [BellwetherShell](../../src/scene/BellwetherShell.cpp), [shell admission](../evidence/2026-10-12-bellwether-shell/README.md), plan 4.6/approved geography | Distant grounded village/shrine shell and lookout composition exist. The admitted shell is 342 triangles, with inspected desktop RT views. Playable town pads, doors, interiors, NPC activity and onward route remain 1.8; town art source candidates are not permission to expand 1.7 gameplay. |
| Checkpoints and Save/Load | [logical restore](../../src/gameplay/simulation/GameSimulationCampaign.cpp), [codec/storage](../evidence/2026-10-18-campaign-storage/README.md), [Android integration](../evidence/2026-10-18-android-campaign-integration/README.md), plan 12 | Implemented lower/exterior logical restore, six-record worker/storage, shared safe-boundary/autosave facade and Android menu/private-root/resource/input/presentation transaction (`7d894450`, readiness fix `6d4033d3`). Full selected host matrix 15/15 Debug + 15/15 Release; Java initial 81/81 + final affected 24/24; Windows apps and four Android ABIs build. [Phone candidate ca08919a](../evidence/2026-10-18-android-campaign-device/README.md) confirms lower manual/one heart/RT, cancellation isolation and exact Slot 2 manual replacement with the other five unchanged and Slot 1 still active. Upper auto/three hearts/night/RT/same-generation Home passed, but Descend stayed Preparing and restart was not run. [Resident Dungeon readiness repair](../evidence/2026-10-18-android-campaign-integration/readiness-fix.json) passes 4/4 focused suites in both native configurations, with 25 new adapter checks. Separate [5eb3282a follow-up](../evidence/2026-10-18-android-campaign-readiness-followup/README.md), source `6d4033d3`, passes upper RT/three hearts/enabled Descend with one actual Traversing admission, then pause/Quit before landing; a proven fresh process reloads the earlier saved Slot 2 manual with one heart/current RT/lower pose/enabled Climb/normal controls. All six records and settings remain unchanged; owner saves remain absent. Bounded isolated persistence is accepted, while the earlier ca08919a readiness failure stays historical. Windows adapter, death Load, forest-end/full-route, interruption/power-loss, audio and performance acceptance remain incomplete. |

## Critical path after the bounded useful save integration

The Android three-slot path is implemented and locally committed at `7d894450`,
with resident-Dungeon readiness correction `6d4033d3`. The
[integration receipt](../evidence/2026-10-18-android-campaign-integration/README.md)
joins the selected Debug/Release host matrix, separate initial 81-test and final
24-test Java runs, Windows applications and four-ABI Android builds. The
[phone receipt](../evidence/2026-10-18-android-campaign-device/README.md) preserves
the rejected first opening camera and the corrected view accepted on `bb4175ff`;
the camera correction is no longer awaiting a rebuild.

Candidate `ca08919a` physically restored the seeded lower manual record with one
heart/RT and the opened rescue view, preserved all six records on Keep records,
and replaced only Slot 2 manual while the other five and Slot 1 active selection
remained intact. Its upper autosave restored three hearts/night/RT and returned
through same-generation Home, but Descend remained Preparing. That real blocker
stopped testing before restart. The adapter had omitted Dungeon zone 0; the
[bounded repair](../evidence/2026-10-18-android-campaign-integration/readiness-fix.json)
now publishes its actual resident scene readiness, retaining real GPU proof.
Four focused suites pass in both native configurations, including 25 adapter
checks; this does not retroactively pass the blocked physical candidate.

The bounded isolated persistence acceptance is now complete on sealed
`5eb3282a`, source `6d4033d3`, in a
[separate physical follow-up](../evidence/2026-10-18-android-campaign-readiness-followup/README.md).
Upper Load presented three hearts/current RT and enabled Descend; one tap
actually entered Traversing, then testing paused and quit before landing. After
a verified fresh application process, the prior actual Slot 2 manual save loaded
one heart/current RT/lower pose with enabled Climb and normal controls. All six
records and settings stayed unchanged, prior paths were retained and the owner
save root remained absent. The blocked earlier candidate is not relabelled.

Windows adapter/menu and death-screen Load are the next implementation seams.
Full descent/landing, earning checkpoints/autosaves through normal chapter play,
forest-end/full-route recovery, active-IO interruption, power-loss, audio/controller
comfort and sustained performance remain unaccepted. The separate torch motion
probe's [crouch-input repair](../evidence/2026-10-18-live-torch-crouch-probe/README.md)
passes Debug/Release at all four input rates, preserving ordinary collision.
Its later APK was not installed and supplies no new GPU timing evidence. Retention is settled; no history
or migration is needed. Prioritize a confirmed route/performance blocker rather
than letting save infrastructure monopolize the remaining 1.7 update.

The reproduced Android shadow-cycle crash is now corrected locally with a
production UI regression and [focused Java evidence](../evidence/2026-10-18-mobile-shadow-cycle/README.md).
The [small-root motor](../evidence/2026-10-18-small-root-motor/README.md) now has
explicit geometry ownership, production-terrain movement and clearance fixtures.
The [playable crouch/root checkpoint](../evidence/2026-10-18-crouch-playable/README.md)
now connects shared posture, safe blocked standing, coherent platform controls,
grounded moving-body posing and one small production root. Physical route,
low-ceiling comfort and audio acceptance remain separate requirements.
The [Back confirmation](../evidence/2026-10-18-android-back-confirmation/README.md)
is also implemented with focused host UI and bounded exact-APK phone evidence;
broader gesture/controller/font acceptance remains separate. These confirmed gaps do not reopen free jump/slide
or authorize unmeasured heavy outside content.

Rendering acceptance remains a critical blocker alongside these host-safe slices.
The measured rendering gate is already concrete: the [live two-torch reveal](../evidence/2026-10-17-keeper-torch-transition/README.md)
raises dispatch from about 66-67ms to 105-110ms, while normal exterior display
spacing is roughly 142-154ms in the [continuation receipt](../evidence/2026-10-17-checkpoint-admission/README.md).
The retained [exact-zero mist exit](../evidence/2026-10-18-zero-mist-exit/README.md)
saves about 18% in a cleared tomb only; it does not close those gates. Reuse those
pinned views and rejected experiments instead of restarting an indefinite loop.
A rendering change needs a new defensible quality-preserving hypothesis and
paired device evidence; host/static metrics cannot establish a device gain.

Useful safe host work while Sam is unavailable is a bounded production-route
reproduction of an already recorded blocker (for example contact reachability,
held-input interruption, or pending checkpoint activation). Prefer one concrete
reproduction, fix and affected check to adding missing-looking feature rows.
Do not broaden contact/reach based only on a seeded fixture; the existing
provisional combat decision must be respected. The coordinating parent's latest
retrieved owner history should determine which such blocker is next.

## Decisions versus implementation/acceptance work

Already settled: three slots with separate manual/latest-auto retention; explicit
player-controlled run; same-night rope out/back; manual fresh lantern raise;
Kit's admitted thirteen cuts and existing visible model; distant town only in 1.7;
root stepping/mantle and crouching; the other latest firm scope listed above.

Potential owner decisions only when the relevant slice is reached: the first
healing/recovery policy; any later expansion into free jump/slide;
material changes to combat balance or the accepted renderer/product contract.
Manual-save exact-position behavior beyond authored safe boundaries is not
implied by separate retention. Routine adapter, file naming, worker cancellation,
host regression and existing-control implementation do not need repeated approval.
Physical touch/audio/art/learning/performance acceptance needs actual evidence;
it is not another product-design question or a host-test pass.
