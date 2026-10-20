# Horde 1.7 remaining scope - 20 October 2026

Current reconciliation after the [finite foliage checkpoint](../evidence/2026-10-20-forest-foliage/README.md),
in the existing WP2 worktree/branch and draft PR 27. Read with the
[accepted master](../references/horde-1.7/d37f1638/docs/superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md)
and [milestone map](HORDE_1_7_MILESTONE_MAP_2026_10_20.md). This inventory changes
no gameplay policy, deferral, validation gate or authority to expand content.

## Substantial unfinished implementation or content

| Area | What remains, distinct from the existing implementation |
| --- | --- |
| First recovery, master 10.5/12 | Main gameplay-policy decision, then its bounded implementation. Three-heart damage/reset/save vitality exist; no healing, flask, charges or refill API/resource save fields exist. The six exact choices below are pending. Preserve established encounter resets; no implicit checkpoint heal or full economy. |
| Outdoor effects, master 8.1/8.3/13.1 | Outdoor participating mist/occluded moon shafts and fireflies are explicitly unimplemented in [the current geometry contract](../../src/scene/RescueJourneyGeometry.h). [Current haze](../../src/vulkan/raytracing/SimulationFrameAdapter.cpp) is a surface-distance-fog multiplier; existing Keeper mist does not close this gap. [Trees are baked static LOD1](../../src/scene/ForestDressing.h); selected foliage wind and representative near/mid/far transitions plus combined workload readiness remain unfinished/unproven. Preserve owner canopy/Easy Tree and mist-28 deferrals/performance gates. These are unfinished planned outdoor presentation, not optional effects already accepted or authority to start heavy work now. Reconcile their final 1.7 disposition explicitly. |
| Sound material, master 4.1b/5.5/8.4/11 | Licensed finite drips/rope, species calls, water, surface dispatch, music continuation and shared dialogue ducking now exist. One licensed finite foliage cue now has bounded host/build evidence. Its audible character/mix and real output cleanup remain unaccepted; no listening occurred. Suitable settling grit/structural stone creaks and any continuous forest wind material remain unverified/unadmitted. Stone retains accepted generic dirt 9/14; Earth uses dirt 17/18; Wood retains generic fallback. Do not relabel metal/impact cuts to fill a roster. |
| Authored composition and movement presentation | Whole-opening burial storytelling, canopy/ground/shoreline/lookout composition and final artistic quality remain incomplete/unaccepted. Kit gait cadence and start/stop joins exist, but stationary planted turning remains unresolved; straight walk-as-pivot was rejected for sliding. Imported locomotion remains Idle/Walking; run/back/strafe/dodge use current shared/procedural presentation. This warrants bounded quality work, not endless new animation or content scope. |

The core tomb-to-lookout/return story mechanisms are present: lantern reward,
same-night opening, dynamic rope/equipment ownership, Kit reunion and fresh
presented-lantern lesson, companion route/waiting, forest beats, durable endpoint
and optional player-opened objective. No additional missing mandatory story
progression mechanism was identified in this audit. That is not physical acceptance.

The later [route inspection](../evidence/2026-10-20-outdoor-route-inspection/README.md)
fixes legacy-policy diagnostic staging at the real root and inspects current bend/
lookout RT views without changing production placement or collision. No new
demonstrated float/intersection justified moving geometry in those views. Wind
and runtime LOD remain unimplemented. The subsequent
[one-pine LOD2 comparison](../evidence/2026-10-20-pine-lod2/README.md) is closed and
rejected for production adoption: 57,088 device bytes saved versus 53.67–65.64 ms
adoption and 32,981,504 extra device-overlap bytes, without an attributed steady
GPU win. Six native A/B/A comparisons preserve exact original restoration;
automatic distance transitions and whole near-crown acceptance remain open.
The derivative is unadmitted. The subsequent
[finite foliage checkpoint](../evidence/2026-10-20-forest-foliage/README.md) adds one
0.90-second licensed leaf rustle within the existing voice, with 12 m range and
44/40-second quiet cadence. Host tests/builds and exact package bytes pass; no
listening or actual output cleanup pass is claimed. Renderer wind and heavy
effects remain unchanged. Next inspect the accepted opening tomb's burial-story
composition/grounding in native RT and correct at most one demonstrated defect
using existing owned assets/budgets. Recovery policy remains pending without
blocking independent work.

## Implemented, with exact-candidate acceptance still open

- **Critical playable path:** natural Keeper victory through lookout and return;
  physically earned lower/upper/forest saves; visible death Load and complete
  recovery/return-side play. Logical CPU/seeded RT proofs do not close these gates.
- **Combat and movement:** first-time teaching, timing/contact calibration,
  two-guard/Keeper readability, dodge/parry, low clearance and rope grip/stow,
  touch/controller controls, motion/aim/comfort. WP2 contact remains provisional;
  do not retry a previously denied blind combat action without relevant authority.
- **Saves and lifecycle:** three independent manual/latest-auto slots and platform
  menus/death Load exist. Physical overwrite/cancel/failure isolation, interruption,
  process restart/power loss, surface/lifecycle recovery and accessibility remain.
  Retention is decided and must not be reopened as an owner question.
- **Audio, speech and haptics:** voice/subtitles/skip and independent gains exist.
  Listening must cover cue suitability, spatial balance/planar pan, music tails,
  new ducking, speech/combat intelligibility, mute/zero, pause/focus/menu/restore
  and actual finite-voice cleanup on Windows and Android. Haptic patterns are
  unchanged; exact-candidate damage/death/feedback acceptance remains distinct.
- **Presentation and platform:** complete route art/secondary views, night/water
  and shoreline/lookout readability; font/DPI/orientation/insets/accessibility,
  controller coverage and HUD/menu/graphics Apply/save/Back behavior. Existing
  implementations/builds are not a blanket artistic or interaction pass.
- **Performance and final candidate:** sustained warm Keeper with both lights and
  exterior pacing at the chosen settings, pause heat/drain, resource growth and
  combined outdoor readiness. Mobile **Mist Off** stays owner-selected. The ~18%
  cleared-tomb gain is not a warm Keeper/two-light or final combined-workload pass.
  The complete exact-artifact Windows/Android/package/device/release matrix remains;
  this host-only checkpoint performs no phone, publication or release action.

## Conditional details and deferred/nonrequired expansion

Nearby debris kicking is feasibility-dependent; a player detail option or
omission is conditional on unacceptable measured cost. Six static chips already
exist; neither extra physics nor that option is a progression blocker.
`forest.wait` is admitted and mapped catalog-only, with no accepted trigger;
wait-for-player behavior already exists. Do not add a trigger merely to play it.
Playable town/onward chapter stays 1.8. Free jump/slide, skill trees, companion
combat, vine physics, RTGI, a full healing economy and new paid score/assets are
not required additions to finish this 1.7 slice.

## Six exact healing questions for morning

Copied without selecting policy from the [morning decision record](HORDE_1_7_RECOVERY_DECISIONS_FOR_MORNING_2026_10_20.md):

| Decision | Choice to settle before implementation |
| --- | --- |
| First method and availability | Limited flask, authored safe-rest interaction, or another bounded method. Specify the existing chapter milestone/location at which it first becomes usable. |
| Capacity and amount | Initial capacity/charges and hearts restored per use; whether a full-health use is rejected or consumes a charge. Keep maximum health at three unless separately chosen. |
| Refill/rest trigger | Name exact safe places or actions, repeatability and any cost. Decide whether a refill is independent of checkpoint activation. A generic “safe area” needs an authored eligibility rule. |
| Use and interruption | Safe-only or usable during combat; action duration; allowed movement/attacks/parries/climbing; damage interruption; exact point when health is restored and a charge is consumed. |
| Death and retry | Which committed health/charges return on death/load/retry, and whether the existing encounter resets described below remain. Loading an earlier save must not manufacture a new grant. |
| Save migration and entitlement | Fields for charges/capacity/refill identity, old-save initial entitlement and one-time ownership. Specify whether old records begin empty, receive a single grant at a safe milestone, or use another explicit migration. |

"Existing encounter resets described below" refers to the linked morning record's
unchanged current health/save audit. No charge, amount, use timing, entitlement,
refill, automatic recovery or migration policy is chosen by this inventory.
