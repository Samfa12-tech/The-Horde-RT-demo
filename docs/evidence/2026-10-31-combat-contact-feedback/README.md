# Contact-driven combat audio candidate

Base `0e9ca093` on draft PR37 passes all six jobs in [41174519633](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/41174519633). This next bounded slice addresses missing ordinary-block audio and deterministic successful-hit cue selection. It does not finish the requested visual combat-feel pass.

## Change and authority

An ordinary frontal held block previously recorded `GuardBlocked` only in the contact trace. The shared fixed-step owner now appends `PlayerGuardBlocked` event 37 exactly at that accepted contact and joins its sequence to the trace. IDs 0–36 and the existing compact metadata layout are unchanged; the type count becomes 38. No event comes from merely holding guard. Rear hits, yellow guard breaks, red attacks, dodge, parry windows, damage, practice protection and recovery rules are unchanged. The source/listener tuple is captured on that tick at the guarding player's supported/crouched eye anchor, with the actual attacking skeleton identity. It is an actor anchor, not a newly measured blade-contact point.

Both platforms consume one semantic contact cue. Skeleton hits retain admitted `sword_hit_1`; Keeper hits retain their own `lich_hurt` cue without a fencing layer. Android no longer chooses a hit sound from mutable UI swing state. Normal blocks use a quieter `sword_hit_1` mix; timed parry retains `sword_hit_2` and its existing Android haptic. Block does not grant that haptic, a parry benefit or hit immunity. Contact cues drained while gameplay is unavailable are discarded rather than postponed.

Only existing FilmCow runtime WAVs and their existing loader/SFX/spatial paths are reused. [ASSET_LICENSES](../../../ASSET_LICENSES.md) records their provenance and project licence; no asset bytes, imports, sound bank, renderer capacity or quality tier changes. New block gains are provisional listening choices, not an accepted balance/mix or performance result. No particle, hit-stop, material-strike, camera-shake or forest implementation is included.

## Evidence

- The strengthened real held-guard fixture failed before event emission (three assertions: missing per-contact semantic sequence/count), while cue metadata checks passed. After wiring, combat-foundation checks pass with one event per actual frontal contact, correct tick/attacker/player tuple and trace link. Rear and red damage, yellow guard break, pause and edge-spam parry negatives remain checked.
- Four affected native tests pass: combat foundation, spatial/metadata/platform routing, bird and foliage append-only ABI. Motion-evidence transport and desktop-input checks also pass. Windows Release application builds.
- Eight focused Java tests pass: contact cue selection/inactive/malformed rejection plus existing equipment and footstep routing. Production Java compiles. These are host checks, not Android RT/audio acceptance; new-head CI supplies separate full build/contract evidence.
- Initial failed output, cue hashes and raw local checks remain in ignored `reports/combat-feedback-20261031/`. This receipt is compact source/host evidence. No phone, foreground input, real audio playback or live report send occurred.

**Audio/haptic manual revalidation required: YES** — a new event, spatial anchor and block mix plus changed hit-cue routing can affect perceived feedback. Automated contracts do not establish audibility, contact feel or vibration. Haptic patterns themselves are unchanged.

## Next bounded acceptance

On an approved exact Windows/Android candidate, hold guard past the fresh parry window: each normal contact should clank once; idle holding and misses should remain silent. Compare fresh timed parry and its existing benefit/haptic, rear/red damage and successful skeleton/Keeper hits. Check SFX mute, pause/background/resume and no delayed contact replay. Keep controller-to-keyboard/mouse handoff acceptance separate. No device use is authorized by this receipt.

Directional white fragments/dark goo remain next visual work. Existing `EnemyHit` supplies an actor anchor, not an impact point/normal: establish actual sword-contact position/direction before spawning convincing RT particles. Stone/tree strike feedback needs actual world-contact classification, not an effect on every miss. No broad renderer experiment is reopened and normal experimental feature gates remain OFF.
