# Kit companion animation plan

The Horde 1.7 | Prepared for Sam | 4 October 2026

**Status:** Reviewable Markdown edition of the delivered animation plan. Planning and candidate selection only, after the accepted 1.6.2 start gate; no runtime implementation, asset acquisition, credit spend or motion-quality acceptance is claimed. Eric owns animation selection and technical checks; Sam reviews the result in game.

**Current planning links:** [1.7 master contract](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md), [asset checklist](ASSET_PLAN_1_7.md), [roadmap](ROADMAP.md), [dialogue bank](CAMPAIGN_DIALOGUE_BANK.md). The immutable repository links in the source list preserve the research snapshot; the updated master contract governs implementation.

Start with the existing Warden idle and walk clips if the model, rights and rig pass validation. Select subtle Meshy talk, listening and turn motions where needed. Use Blender only for retargeting, loop repair, transitions and small acting adjustments. Quaternius free CC0 packs are useful alternatives when they fit better.

### Simpler rope staging

The rope can deploy during lantern pickup and then hang from a fixed world anchor, such as a tree or rock fixture chosen during scene integration. Kit can idle nearby without holding it. No visible Kit throw, hand-release or recovery animation is needed. This follows Sam’s latest direction on 4 October 2026 and replaces the more elaborate throw sequence in the earlier plan. [16]

### What Kit needs to communicate

Kit should feel attentive, reassure the player at the safe summit, react to a fresh manual lantern raise, and lead toward the lookout. Readable idle, walking, turns and a few restrained gestures cover those beats without taking control away from the player. [1–3,16]

- The early call through the small grated WALL panel is voice only. Kit is explicitly unseen there. The urgent, worried opening and optional audible movement acknowledgment follow the [performance brief](dialogue/en/PERFORMANCE-DIRECTION-English-en.md). An acknowledgment may use real nearby player footsteps; no new animation, look-at, stopping or response is required. Any relief follows a perceivable cue, never assumed arrival or an unverified sightline. Keep missed-call compatibility and the later fresh player-controlled lantern raise. This is conditional authoring direction, not completed runtime.

- Kit does not know about the lich. His reactions should reflect the rescue, the player and the lantern.

- The rope belongs to the simulation. A one-time runtime deployment event creates the anchored, dangling rope independently of Kit’s animation.

- Kit waits safely on the authored route and at the lookout. He must not block the only path or visibly teleport. The distant village remains a shell.

### Candidate model and first validation

The Briarhold Warden remains a reuse candidate. The asset plan reports 24 joints, 25,968 triangles, one mesh and material, four 1K textures and seven clips: idle, walk, run, jump, fall, slide and mantle, with horizontal root motion removed. Kit’s rights, appearance, rig and Horde import still need validation. [2]

Check rights, Kit’s appearance, the actual skeleton, skinning and Horde import before acquiring more motion. The mask covers the nose and mouth, so head, torso, hands and breathing can carry restrained acting without exposed-mouth lip sync. Existing traversal clips do not authorize new Kit mechanics.

### Production order

1. Validate the actual model and rig with idle, walk and a turn. Confirm scale, feet, shadows and motion ownership in the game.

2. Integrate the anchored rope deployment and Kit’s idle, then add the summit reunion, reassurance and lantern reaction.

3. Finish departures, stops and waits. Review the result in game with Sam, then adjust motions that look wrong or feel too exaggerated.

Animation selection and technical cleanup are handled during implementation. Sam reviews the in-game appearance and feel; he does not need to choose action IDs, rigs or animation settings.

## Kit animation matrix

This reduced set covers the current 1.7 staging. It is a practical starting selection rather than a fixed clip count. Every external motion remains a candidate until it passes the actual Kit rig and scene tests.

| Family | Minimum motion content | First route | Key acceptance check |
| --- | --- | --- | --- |
| Idle and waiting | Relaxed idle; attentive wait; bounded look-at | Reuse Warden; CC0 idle alternatives | Stable loop, planted feet, restrained head range |
| Walk and approach | Approach, gentle start and stop | Reuse walk; author transitions in Blender | Stride matches route speed; no foot sliding or root double movement |
| Turn and reorient | Left and right turns into a route or conversation | Test Meshy turns; Blender cleanup | Correct facing without snapping or foot pivots through the ground |
| Listen and attend | Small attentive idle or listening gesture | Meshy Listening_Gesture 47 | Restrained body motion; no oversized gesture or fixed gaze |
| Reassure and talk | Restrained concern and conversational gestures | Meshy Stand_and_Chat 56; CC0 fallback | Fits dialogue duration; does not reveal knowledge Kit lacks |
| Lantern recognition | Small recognition and wonder beat | Blender from an idle or gesture base | Runs only after the fresh player lantern raise is visible |
| Depart and lead | Turn away, depart, walk, stop and wait | Reuse walk and turn; Blender joins | Collision-aware route, safe waits, no sole-path obstruction |

### Conditional work

Add a jog only if the route needs one. Use a look-down pose only if the final staging shows Kit at the opening and it improves readability. A waystone gesture or lookout acknowledgment can reuse a base gesture. Defer hub emotions, seated rest and extra prop interactions until their scenes require them.

Kit has no approved combat, escort-failure, attack, hit or death set. Rope climbing, swimming and vaulting are not approved Kit mechanics. Player rope grip, climb, crest and equipment motions belong to the player plan. [1–3]

### Rope deployment without a character clip

Choose a fixed world anchor clear of the landing path and deploy the simulated rope once during lantern pickup when lantern ownership and sufficient rescue-opening clearance are established; retain the master plan's climb/readiness gates. Kit can remain idle. The rope must arrive credibly even if the player looks up; do not force the camera away or make progression depend on the player watching a particular view. No hand socket or Kit release marker is required. [16]

## Acquisition shortlist and rights

Reuse the baseline first, then test the selected Meshy gestures and turns through the existing subscription. Use free CC0 alternatives where they fit better. “Catalog verified” means the listing, tier chart or ID was checked; motion quality and retarget suitability remain untested.

| Source | Publisher listed content | Rights and acquisition decision |
| --- | --- | --- |
| Existing Warden candidate | Reported idle, walk and run baseline | Validate provenance and Kit approval first. Reuse avoids unnecessary retargeting. [2] |
| Quaternius Universal Animation Library Standard | Normal, talking and torch idles; normal and formal walk; forward jog and sprint; seated motions; interactions | Free CC0 assets. FBX and GLB; root-motion and in-place variants advertised. Blender source is a paid tier. First free supplement to inspect. [4–5] |
| Quaternius Universal Animation Library 2 Standard | 42 free motions in the tier chart, including folded-arms, lantern, yes/no and railing-call idles; carry walk | Free CC0 assets. The advertised 130+ library is not all free. Eight-direction walking and 180-degree turns are paid-tier content. [6–8] |
| Meshy animation catalog | Selected listening, restrained chat and turns; other candidates below | Existing subscription confirmed by Sam. Check rig-task dependency, included credits, usage rights and any additional cost before generation. Availability verified; fit untested. [9–11,16] |
| Mixamo | Free with Adobe ID; exact current clip shortlist not verified | Royalty-free commercial use, proprietary terms. Do not put standalone/raw or retargeted outputs in a public repository. Private production only after the terms check. [12–13] |

### Useful Meshy candidates

Start by testing Listening_Gesture 47, Stand_and_Chat 56 and the left/right turn candidates below. Keep the existing idle and walk if they work. These names and IDs were verified in the live catalog; their motion previews and Kit retargets remain untested. [9]

| Need | Exact candidate names and action IDs |
| --- | --- |
| Idle and conversation | Idle 0; Idle_02 11; Idle_03 12; Listening_Gesture 47; Stand_and_Chat 56 |
| Restrained hand gesture | Talk_with_Hands_Open 313; Talk_with_Right_Hand_Open 314 |
| Walk and turn | Casual_Walk 30 / in-place 613; Walk_Turn_Left 572; Walk_Turn_Right 583; Idle_Turn_Left 576; Idle_Turn_Right 586 |
| Conditional transition | Run_to_Walk_Transition 116 / in-place 667; Sprint_and_Sudden_Stop 531 |

No complete generic start-and-stop suite was found. Finish any necessary joins in Blender. A visible rope throw is no longer needed, so there is no reason to acquire or author a throw or rope-holding clip for Kit.

## Meshy and Blender production workflow

| Tool | Best use here | Constraint to plan around |
| --- | --- | --- |
| Meshy | Obtain a selected base motion or prototype an alternative | Animation API requires a successful Meshy rig task. Do not assume a catalog motion can be applied directly to the existing arbitrary rig. [10–11] |
| Blender | Retarget onto the chosen Kit rig; repair feet and hands; improve loops, transitions and restrained personality | Use only the cleanup and custom adjustments the game actually needs. No dedicated rope-throw sequence is required. |
| Game runtime | Route motion, collisions, blending, bounded look-at, semantic events and rope simulation | Verify additive and IK support before designing around them. Keep body and simulation responsibilities explicit. |

### Meshy intake check

The rigging API accepts an external textured humanoid GLB facing +Z and documents a 300,000-face limit; an A or T pose is helpful. This is a rigging route, not proof that the Warden’s current skeleton will survive unchanged. Rigged outputs include GLB and FBX and can include basic walking and running. [10]

The animation API currently accepts 1–10 action IDs in one request, with a listed cost of 3 credits per action. Text-to-motion retargeting for bipeds is also documented, with GLB output, but remains an optional unproven route for Kit. The inspected CLI 0.4 exposes a single action ID and does not expose dedicated multi-action or text-to-motion flags. Do not assume API features are available through that CLI. [11]

### Production steps

1. Freeze the approved model version and inspect its real skeleton. Record scale, rest pose, root convention, clip names and existing rights evidence. Keep one final Kit rig; other characters can have separate validated rigs.

2. Test a small sample of each source. Retarget idle, walk and one gesture before expanding the library. Compare deformation, hand orientation, foot contact, root behavior and silhouette.

3. Place and test the fixed rope anchor in the scene. Deploy the simulated rope during lantern pickup independently of Kit’s idle. Check visible arrival as well as the intended offscreen timing.

4. Export named actions with loop flags, speed expectations and event semantics. Use common naming and events across rigs where useful; shared naming does not imply a shared skeleton.

5. Play the complete sequence in game, then use Sam’s visual feedback to tune the chosen motions. Retain editable sources, exports, source links and validation evidence with the approved handoff.

### Free does not mean every tier is included

CC0 allows copying, modification and commercial distribution, making these packs suitable for a repository that includes raw animation assets. Save the creator page, tier chart, license and downloaded version as provenance. UAL Standard excludes backward and diagonal jogging, strafing, 90-degree turns, sprint enter/exit and directional crouch additions shown in higher tiers. A file-format conversion does not change a restrictive license. [4–5,12–13,15]

## Acceptance checks and player appendix

### Kit acceptance checklist

- Rig and render: correct size, skinning, mask and costume deformation; no clipping at hands, feet or props; stable shadows and acceptable phone skinning and ray-tracing update cost.

- Locomotion: measure stride against route speed; check turns and blends at actual speeds; prevent double movement from both root animation and controller translation.

- Story and events: early grate call stays unseen; summit approach preserves player control; only a fresh, visible manual lantern raise triggers recognition.

- Rope: deployment occurs once from the fixed anchor, independently of Kit. Check pause, skip, save/load and repeated triggers. No duplicate rope, broken anchor or view-dependent softlock is acceptable.

- Robustness: interrupted talk returns cleanly to idle; waiting and look-at remain bounded; the sole path stays open. Sound must not be a progression dependency.

### Player directional movement

The inspected 1.6.2 baseline has Walk and Idle only; its clip manifest explicitly excludes running, and directional selection is absent. The 1.7 plan adds run controls and body/equipment animation, plus alternating rope hands and sword clearance. Forward/backward running and strafe/walk clips are a separate player expansion, not a reason to buy that full suite for Kit. [1,14]

| Player need | Candidate or gap | Planning decision |
| --- | --- | --- |
| Forward walk and run | Existing Walk; Meshy basic Walking 2 and Running 1; UAL free forward jog and sprint | Inspect current player rig first. Select a consistent speed and posture family. |
| Backward walk | Meshy Walk_Backward 544 / in-place 679 | Catalog verified. Retarget, feet, equipment clearance and blend compatibility untested. |
| Backward run | Meshy BackLeft_run 5 and BackRight_Run 6 / in-place 606 and 607 are diagonal | These do not prove a straight backward run. Find or author the exact motion if the control design needs it. |
| Left and right strafe | No clearly named neutral upright pair found in the Meshy review; relevant Quaternius directional sets are paid | Keep as a gap for a free-source search or Blender production. Do not relabel diagonal running as strafing. |
| Stops and direction changes | Meshy run-to-walk and turn candidates; Blender transition work | Define a small initial direction set and test blends before adding diagonals. |

Player rope candidates such as Climb_Up_Rope 449, Rope_Hang_Idle 477 and Swing_on_Rope_to_Ground 494 are catalog references only. They do not prove a safe descent/landing or the real grip, climb, crest and equipment contract. Kit does not need a rope-use animation under the current staging. [1,9,16]

**Shared foundation, 5 October 2026:** Reuse the post-reset 1.6.2 draw/sheath/stow/restore state, animation events and sound mechanism; this chapter adds rope-specific carry attachments, poses and physics integration, not a duplicate equipment system. The 1.6.2 addition does not implement rope traversal or change the PCVR boundary.

**Locked player transition direction, 5 October 2026:** Stow **both sword and lantern** on visible, safe carry attachments before either hand grips; both hands are free for the rope. Exact hip/back placement remains prototype work on the actual rig. The lantern remains owned, physically present and lit, with its emitter moving with the prop and coherent shadows/reflections/transmission; never leave a phantom hand light or duplicate/drop/delete gear. Restore the prior valid held-item configuration only when grounded, retaining the first reunion's lowered-lantern/fresh-raise rule.

Author real-height ascent, alternating grips and pull-up/mantle to reveal the Keeper gravestone moved aside after defeat. Author return rim entry, descent and lower landing separately rather than reversing an arbitrary candidate clip. Fit the accepted tomb and F01 rescue geometry; dimensions remain unmeasured. Use stable, modest limited-look framing and reduced-motion support, with real shaft occlusion instead of a forced camera tour. The current 4–7 second ascent target is not an I/O guarantee or a reason to loop/stall animation.

The [master §§4.3–5 contract](superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md#43-rescue-and-rope) owns early destination preparation, readiness before commitment, resident shared transition geometry, no normal loading screen in either direction and safe failure recovery. Test pause/orientation/resume, pre-commit cancellation, interrupted ascent/descent checkpoints, gear/light continuity and persistent boss/loot/gravestone/dialogue state. If Kit does not accompany the return, use a bounded safe exterior wait; no Kit climb or dungeon-companion mechanic is added. This remains unimplemented 1.7 planning with no new clip purchase or generation authorization.

## Sources and verification

Primary sources checked on 4 October 2026. The repository supplies the 1.7 baseline; Sam’s latest rope-staging direction below supersedes its visible-throw animation requirement. Publisher inventories and API behavior can change; recheck selected assets and terms at acquisition.

[1] [The Horde 1.7 plan](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/43b2765ed51f9aae22ec94ac8f64eb31e7b32d03/docs/superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md)  Sections 5.5 and 5.7 cover player motion; sections 6 and 7 cover rope, companion animation and dialogue.

[2] [The Horde asset plan 1.7](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/43b2765ed51f9aae22ec94ac8f64eb31e7b32d03/docs/ASSET_PLAN_1_7.md#2-briarhold-reuse-shortlist)  Reported Warden reuse candidate and specifications.

[3] [Campaign design and dialogue](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/43b2765ed51f9aae22ec94ac8f64eb31e7b32d03/docs/CAMPAIGN_DESIGN.md)  Story boundaries. The linked dialogue bank below supplies the rescue and forest beats.

[3a] [Campaign dialogue bank](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/43b2765ed51f9aae22ec94ac8f64eb31e7b32d03/docs/CAMPAIGN_DIALOGUE_BANK.md#4-opening-tomb-rescue-and-forest-17-focus)  Opening tomb rescue and forest focus.

[4] [Quaternius Universal Animation Library](https://quaternius.itch.io/universal-animation-library)  Publisher description, CC0 license and formats.

[5] [Universal Animation Library tier chart](https://img.itch.zone/aW1nLzIwMzQwODM2LnBuZw%3D%3D/original/Hjl7T2.png)  Exact free Standard versus paid-tier content.

[6] [Quaternius Universal Animation Library 2](https://quaternius.itch.io/universal-animation-library-2)  Publisher description and CC0 license.

[7] [Universal Animation Library 2 free tier chart](https://img.itch.zone/aW1nLzI1MTU2ODYyLnBuZw%3D%3D/original/PujWJa.png)  42-motion free Standard content.

[8] [Universal Animation Library 2 Source tier chart](https://img.itch.zone/aW1nLzI1MTU2ODY1LnBuZw%3D%3D/original/UO4aVv.png)  Paid directional walking and turn content.

[9] [Meshy live public animation catalog](https://api.meshy.ai/web/public/animations/resources)  680 entries inspected; exact candidate names and action IDs verified.

[9a] [Meshy animation library documentation](https://docs.meshy.ai/en/api/animation-library)  Official catalog documentation.

[10] [Meshy rigging API](https://docs.meshy.ai/en/api/rigging)  Input requirements, rig-task dependency and bundled basic motions.

[11] [Meshy animation API](https://docs.meshy.ai/en/api/animation)  Action batching, listed credit cost and text-to-motion route.

[12] [Adobe Mixamo FAQ](https://helpx.adobe.com/creative-cloud/faq/mixamo-faq.html)  Account access and royalty-free use.

[13] [Adobe General Terms of Use](https://www.adobe.com/legal/terms.html)  Section 3.6 addresses content-file distribution restrictions.

[14] [The Horde pull request 18](https://github.com/Samfa12-tech/The-Horde-RT-demo/pull/18)  Inspected 1.6.2 baseline head 89fecb; Walk/Idle manifest and missing directional selection.

[15] [Creative Commons CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/)  Public-domain dedication summary, including commercial copying, modification and distribution.

[16] Sam’s planning direction, 4 October 2026. Deploy the rope during lantern pickup; let it hang from a fixed anchor while Kit idles. Animation selection is delegated, with Sam reviewing the result in game. Meshy subscription confirmed.

Before adoption, preview the selected motion files and validate them on the actual rig in the game. Catalog availability and license evidence do not establish hand quality, foot contact, loop continuity, retarget fit or performance. Those checks remain the first production task.
