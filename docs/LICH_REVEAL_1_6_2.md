# 1.6.2 — The Fourth Keeper: lich reveal and encounter presentation

**Planning brief: 2 October 2026.** Requested by the owner for the 1.6.2 update bank. This is the implementation target for the reveal, not evidence that it exists. Begin only after 1.6.1 is completed, accepted, merged and released. Read [ROADMAP.md](ROADMAP.md), [CAMPAIGN_DESIGN.md](CAMPAIGN_DESIGN.md) and the released simulation/audio contracts first.

## Creative intent

### Owner lighting correction, 5 October 2026

The physically occluded mist makes the dormant/revealing Keeper too dark. Add two torches behind and on either side of its current staging anchor. Ignite both on the existing reveal-start tick alongside the movement/sword hold, cue and title sequence. Keep both lit throughout the encounter and through the actual death animation; extinguish only when that animation completes, allowing the existing chest spotlight to become the visual focus. Preserve the reward timing, roof occlusion and chest spotlight.

Reuse admitted torch bodies, shared flame/light sockets and ordinary hardware-RT shading. Admit the real simultaneous-light budget explicitly; do not evict the player's torch or reward lantern. Reset, retry and authored checkpoint imports must restore lighting from the authoritative encounter state, with no premature ignition in the dormant chamber. Capture reveal, sustained combat and the death-to-reward handoff on the exact candidate for owner appearance review. Overlapping mist lighting and matched phone cost remain required checks. This addition supersedes only the lighting target, and grants no merge or release authority.

The player has entered a guarded resting place and disturbed its last custodian. The lich is the cursed fourth keeper; the two earlier skeletons are its sworn guards. Replace the current corner placement and abrupt combat start with a clearly staged, approximately six-second awakening in the existing final chamber. It should feel solemn, deliberate and threatening rather than like a random monster spawning.

The keeper places itself between the intruder and the reward. That action becomes meaningful when the campaign later reveals its role as jailer. Do not explain the prison, the deceptive voice, or the king's bargain here. No spoken player line, Kit appearance, new dialogue recording or pre-reward lantern voice is required for this 1.6.2 slice.

## Room layout and sightlines

- Use the existing final chamber and reward chest. Move the lich from its side/corner position to a far-middle staging anchor on the approach-to-reward axis, with room behind and beside it for the existing chest and accepted combat movement. Do not enlarge this into a new dungeon or add another enemy.
- Start the keeper still, in a restrained forward-tilted resting pose above a shallow stone keeper's plinth. Reuse suitable admitted stone geometry. Keep the plinth low and walkable or give it explicit collision; it must not become a snag, unreachable melee target or pathfinding obstacle.
- Face the dormant keeper toward the normal entrance. Frame its head, shoulders and raised hand against a contrasting stone recess. The approach must expose the whole silhouette rather than just a face peeking out beside a wall.
- Provide a clear foreground-to-keeper sightline from the trigger threshold at standing eye height and supported FOVs. Preserve walkable combat clearance and the chest interaction space. Exact world coordinates come from the accepted release's room geometry; record final anchors and collision dimensions in the implementation evidence. The currently inspected finale bounds are x[-36.9,-30.5], z[-18.4,-12.0], with the chest at (-35.30,-17.55). These are audit references, not immutable future anchors. The existing mirror retry checkpoint spawns inside this chamber at (-33.70,-15.20): explicitly move the playable retry spawn to safe arrival-side clearance if staging would overlap it, while preserving legacy deterministic capture imports.
- Keep the actual lantern unavailable until the existing post-lich reward sequence. The resting place may be suggested by the chest and keeper's position; do not add a second lantern, an early pickup, or a lantern in the player's hand.
- Use actual world-space lighting and existing bounded effects. The keeper must remain legible after the existing torch-loss beat. Reposition or tune admitted room lighting if needed; no camera-facing spotlight, exposure cheat, screen-space silhouette or permanent extra costly light stack.
- The inspected torch-loss sequence happens well before the coloured bays and final threshold; this reveal assumes neither held torch nor lantern. Existing approach, torch loss, death, reward and exit progression remain authoritative. Reconcile their exact order against released source before choosing the final trigger anchor.

## Trigger and six-second first reveal

The keeper is already present before entry; no spawn pop, opaque fog curtain or instant reveal of previously missing geometry.

Use a named entrance threshold in the final chamber, after the approach/torch-loss sequence has reached its settled state. First crossing starts the reveal exactly once for that encounter attempt. Entering the zone must no longer immediately start normal lich attacks.

The following timings are the initial authored target. Small timing adjustments for animation and an approved sound's actual duration are allowed, but document them; do not replace this sequence with an unrelated cinematic.

| Elapsed time | Visible action | Sound / music | Gameplay |
|---|---|---|---|
| 0.0–1.0 s | Keeper remains still for a brief beat, then begins to straighten from its resting tilt. | Duck the existing exploration music gently. One restrained positional awakening cue, if an approved asset fits. Avoid a full-volume jump scare. | Reveal state begins. Movement and look remain available. Neither side can deal damage. |
| 1.0–3.0 s | Keeper straightens and slowly rises about 0.25 m into a controlled hover, keeping its robe and staff coherent as one silhouette. Use a smooth ease-in/out, not a model pop; the visible pose and combat target volume stay aligned. | A single mapped movement/rise cue. Any cloth/bone layer must use an admitted asset and be sparse. | Keep the lich at its staging anchor. Do not chase, fire projectiles or accept hits during the rise. |
| 3.0–4.5 s | Keeper turns toward the player and gives a deliberate, restrained forward inclination of its staff-bearing silhouette as a warning. Its position still blocks the route toward the chest. | Play one approved lich warning/presence cue at the keeper's position. Do not substitute a pain or death sound without an explicit authoring decision. | Present the short title “The Fourth Keeper” with a readable fade. This identifies the encounter without explaining the twist. |
| 4.5–6.0 s | Hold the warning briefly, then settle into the existing combat-ready stance. If the player moved, turn smoothly rather than snapping. | Transition into the existing lich-combat music using its supported cue/transition system. Avoid adding a second score or rebuilding music for this reveal. | Clear title if it obstructs combat. Enable vulnerability and combat together at the end, then start a complete existing first-attack telegraph. No attack may arrive with its wind-up already spent. |

The title is a presentation label, not a new health-bar requirement. It must not obstruct mobile controls. The warning movement and rising silhouette carry the scene with sound disabled; audio must not be the sole indication that combat is about to begin.

## Player control, edge cases and retry

- Keep first-person look, movement and pause available. No forced camera rotation, teleport, automatic weapon swing, mandatory letterboxing or unskippable camera tour.
- Use room composition and positional sound to attract attention. If the player looks away, the reveal continues; never restart or hold them hostage until a camera-alignment test passes.
- Player attacks during the reveal cannot damage the keeper or queue a free hit for the transition. Use a clear non-impact response through existing feedback rather than implying successful damage. Prevent either actor overlapping the other or trapping the player while the keeper rises.
- Retreat over the threshold does not restart the sequence or respawn anything. Preserve the released encounter's retreat boundaries; do not introduce a locking door or invisible wall merely for this reveal.
- A pause or Android background event suspends the reveal timer and sound/music ownership consistently. Resume without replaying one-shot cues, overlapping music or accumulating skipped-time damage.
- Full route reset starts a fresh encounter. Same-run boss retry uses a short, approximately one-second ready/recognition beat rather than replaying the full six seconds, then supplies the full normal first-attack telegraph. Never respawn directly into damage.
- If the released game supports persistence at this point, restore explicit dormant/revealing/combat/defeated state consistently. Do not invent a new save system solely for this scene. Existing deterministic combat capture checkpoints must be able to import their authored state without accidentally triggering the reveal.
- Defeat remains terminal for that attempt. Returning to the chamber must not replay awakening, duplicate the reward or create another actor.

## Animation and combat boundaries

Required presentation states: dormant/resting, awakening/rise, warning, combat-ready, existing combat actions, accepted-hit reaction and defeat. Use the actual lich actor and its shared animation/skinning path. The inspected rig has fused robe/staff geometry and admitted Idle_02 and Dead clips; it does not establish separately articulated staff, fingers or cloth. Build this target around bounded whole-actor hover/tilt/turn and supported clip blending. Do not distort the rig to fake a kneeling rise, opened fingers or planted staff. A later bespoke animation upgrade needs an explicit asset plan and approval for any paid work.

The rise must have continuous root movement and intentional hover clearance, no capsule-like slide, abrupt pose reset, wall clipping or discontinuity at combat handoff. Preserve this hover offset through combat handoff rather than snapping to the old vertical placement. Preserve existing accepted damage, health, range, attack windows, hit lockout and reward rules. At the inspected baseline the lich uses a 0.65 s reposition, 1.20 s staff charge, line-of-sight/range damage pulse and 1.80 s recovery, with three HP and a two-second accepted-hit lockout. Recheck these in the released baseline; the first post-reveal attack must keep its full charge rather than converting the warning animation into an already-running damage countdown. This is a reveal/readability pass, not permission to redesign the boss or add phases, summons, projectiles or extra health.

On defeat, retain the existing authoritative death and lantern/chest progression. Improve a readable collapse/release animation and attach the admitted death cue to the correct event. Allow the defeat sound to finish or fade coherently under existing reward music; do not spawn the lantern early or delay the reward indefinitely while waiting for an audio callback.

## Sound asset mapping and admission

The owner reports locally added skeleton and lich sounds, described as documented in PR16. At the inspected remote head, exact new filenames and that inventory have not yet been located. This is an explicit asset-resolution dependency, not permission to fabricate filenames, generate replacements or claim the files were auditioned.

Before implementation, locate the owner's sound inventory/local files and record for each admitted cue: source path, runtime path, provenance/reuse rights, hash, duration, channel/sample format, intended event and audition decision. Keep local-only availability distinct from committed/shipping assets. Link that inventory here when found.

Map the existing and newly supplied assets to:
- Lich awakening/rise, warning/presence, existing attack, accepted hit, defeat.
- Skeleton movement/idle, attack, accepted impact and death, where genuinely needed after auditing existing coverage.

Existing baseline files under assets/audio/filmcow/ are skeleton_step_1.wav, skeleton_step_2.wav, skeleton_attack.wav, enemy_fall.wav, sword_hit_1.wav, sword_hit_2.wav, lich_charge.wav, lich_impact.wav, lich_hurt.wav and lich_fall.wav. These are not verified as the owner's new additions. Preserve the charge cue's association with an actual damaging attack; do not play it during a harmless reveal. Source: [existing import map](../tools/import-filmcow-sfx.ps1).

Not every slot needs a separate recording. Leave an unsupported decorative slot silent or reuse an appropriate accepted cue deliberately; do not use a death scream as a reveal just because it is available. Missing required animation or sound is a specific reported gap, not a reason for Codex to improvise an entirely different encounter.

Reveal progression must not depend on audio playback success or completion. Use entity-aware, event-time positional playback and the existing bounded event transport. Cap idle/vocal repetition, avoid stacking presence loops over combat cries, and do not trigger sounds from every animation/render frame. Preserve established impact/fall separation and cancellation on reset. Skeletons need distinct idle phase/timing and non-synchronised incidental sounds while combat hit windows remain authoritative. Audition the result against the existing whistle-led score on phone speakers/headphones and Windows; do not silently change the accepted music instrumentation.

## Implementation slices and acceptance

1. Audit the released encounter, inventory and collision/lighting anchors. Record any conflict with this brief before substituting a different design.
2. Implement a deterministic reveal state and safe combat handoff through shared simulation snapshots and semantic events. Test it without decorative effects.
3. Add staged animation, bounded lighting adjustments and mapped approved sounds. Integrate existing music transitions and mobile-safe title.
4. Validate the complete approach → torch loss → reveal → combat → defeat → reward → exit route.

Required checks:
- Automated once-only triggering, boundary re-entry, attacks during reveal, full first telegraph, pause/resume, reset, retry, death, capture-state import and reward idempotence.
- Android touch and Windows controls remain responsive; check looking away, walking close, backing out and entering quickly. Validate low FPS and supported graphics settings without skipping the reveal state or creating damage.
- Record continuous first-entry and retry footage, including muted playback, with the keeper readable at the intended lower internal resolutions. Still images alone cannot prove animation/audio timing.
- Matched exact-device frame-time, memory and thermal evidence around approach/reveal/combat. Keep effects bounded; no extra actor or full-screen volume to create spectacle. Validate supported RT backends and real shadow/reflection participation.
- Owner review of the actual reveal, animation, sound balance and combat handoff on the accepted candidate. **Audio/haptic manual revalidation: YES for implementation** because cue content/timing changes; this documentation-only update requires none.

Completion means the keeper is visibly staged and awakens as specified, sound mappings are traceable, combat/reward behaviour remains reliable, and the reveal is accepted in motion on the supported platforms. Report deviations, missing assets and measured costs. Do not call it complete merely because a model was moved out of the corner.

## Source inspection for this brief

Remote PR16 head f66d124a and PR15 head 03a6870 were inspected on 2 October 2026 AEST. The encounter header was identical between those snapshots; this does not establish the user's unpublished local state. Re-audit the released baseline before implementation.

- [Encounter and combat](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/03a6870f66dd9cb4bab62d2f521f4fd663368678/src/gameplay/ShowcaseGameplay.h)
- [Route and torch-loss progression](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/03a6870f66dd9cb4bab62d2f521f4fd663368678/src/gameplay/ShowcaseRoute.h)
- [Existing retry/capture checkpoints](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/03a6870f66dd9cb4bab62d2f521f4fd663368678/src/gameplay/ShowcaseCheckpoints.h)
