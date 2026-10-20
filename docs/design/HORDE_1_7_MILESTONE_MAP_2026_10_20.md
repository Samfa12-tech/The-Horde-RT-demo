# Horde 1.7 milestone map - 20 October 2026

Working path: `C:/Users/sam_s/Documents/the Horde RT Demo/.worktrees/horde-1.7-wp2-vertical-proof`.
Branch `codex/horde-1.7-wp2-vertical-proof`, continuing existing draft PR 27.
Read with the [accepted master](../references/horde-1.7/d37f1638/docs/superpowers/plans/2026-09-11-beyond-the-tomb-1.7.0.md)
and [feature inventory](HORDE_1_7_FEATURE_STATUS_2026_10_18.md).
This is a current reconciliation, not release approval or new feature scope.

| Milestone | Implemented and evidenced | Remaining acceptance or implementation |
| --- | --- | --- |
| Playable tomb-to-lookout route | Lantern reward, same-night rescue shaft, rope out/back, grounded Kit route/fresh presented lantern proof, thirteen approved voice cuts, forest story and distant Bellwether shell. Forest endpoint now durably retains completed story. | Natural full-route/return-side play, physical earned lower/upper/forest saves, visible death Load, contact/first-time teaching, controls and listening remain unaccepted. Host and seeded RT results do not close these gates. |
| Recovery and interaction | Three slots each retain independent manual/latest-auto records; safe-boundary restore, platform menus/death Load, hearts, subtitles/skip/Voice, optional player-opened lookout objective, run/crouch/18 cm root step and Back confirmation. | Physical interruption/controller/accessibility/comfort matrix and recovery through normal play remain open. No save-retention decision is outstanding. A first healing method is not implemented; capacity/refill/use/death/save rules require the bounded policy decision in master 10.5. No implicit checkpoint healing or full economy. |
| Kit movement/acting | Approved rig, idle/walk and transient head/chest speech/recognition; proper model facing. Current bounded locomotion evidence records measured cadence and clip joins. | Artistic/live sequence/physical acceptance remains open. Stationary foot pivots remain unresolved; straight walk at zero route speed is a rejected substitute because its stance foot slides. Do not remain indefinitely on small acting refinements. |
| Regional world and sound | r23 terrain has 37,846 triangles; 42 atlas-derived finite river/lake/outflow faces and real RT depth/transport have paired native backend surveys. Pond frog/cricket playback uses actual water proximity; owl/cricket cuts also exist. | Full-map/shoreline/lookout composition, night listening and phone warm cost remain unaccepted. Cicadas remain excluded. Playable town stays 1.8. These are existing implementations, not missing work to repeat. |
| Tomb story/decay | Campaign omits legacy material/demo cards and swatches; burial bier, niches, remains, candles, displaced lid and Keeper offerings establish a partial story composition. [Six sparse original floor chips](../evidence/2026-10-20-tomb-floor-debris/README.md) now sit beneath damaged masonry: 72 triangles, 14,688 payload bytes, existing material/primitive and unchanged collision/save ownership. Both native RT backends have matched off/on evidence. | Whole-opening art, live movement/contact and phone readability/warm cost remain open. Kicking and a player-facing debris option are not implemented. Debug omission verifies disabled geometry only; add an actual option or omit the detail if mobile cost proves unacceptable. |
| Player movement presentation | Walk/run/back/strafe, shared four-direction dodge displacement/protection, low-ceiling sword lowering and alternating rope hand targets exist. [Dedicated procedural dodge presentation](../evidence/2026-10-20-player-dodge-poses/README.md) now uses the existing accepted direction/timer, with 562 native rig samples, real rope/restore cancellation tests and paired native RT transitions with torch/lantern. | Full artistic/live movement, physical motion/aim/grip/comfort and warm phone acceptance remain open. Idle/Walking remain the imported locomotion clips; the new dodge is an explicit procedural layer, not new imported run/back/strafe clips. |
| Surface-aware travel sound | Actual support already classifies Stone/Earth/Wood; authored terrain distinguishes earth/stone. Distance cadence and shallow-water contact/cues exist. | Dry step events still carry payload zero and use the same two generic clips. Surface-to-event/cue dispatch is the next planned bounded implementation, subject to suitable existing/licensed material cues and later listening acceptance. No new bridge or audio engine. |
| Performance and final candidate | Exact-zero mist early exit measured about 18% GPU improvement in a cleared tomb. Rejected quality/thermal-confounded trials stay rejected. Sam explicitly selected mobile Mist Off; current startup/reset policy also has it Off. | Keeper two-light cost, exterior display spacing and sustained warm 30 FPS remain unresolved. No new phone action is authorized by this map, no sustained gain is inferred from host captures, and no final release matrix is complete. |

## Next source slice

The sparse debris checkpoint is implemented and locally verified; its rejected
occluded placement and failed launch-test attempts remain in the evidence.
It adds no new RT owner. Short desktop deltas establish no mobile or sustained
performance gain, and no kick interaction is claimed.

The four-direction dodge layer is now implemented and locally verified. It
derives pose weight/direction in final immutable snapshot publication and uses
the existing leg/arm solver. Existing 0.90 m movement, 0.20 s timer, protection,
input, camera, events and save rules remain unchanged. Sixteen focused host
registrations, Windows Debug/Release, four Android ABIs and 256 matched completed
native RT frames pass. Real rope/restore host fixtures cannot establish naturally
earned saves, physical feel or a new hostile-death pass. Pause keeps its existing
dodge-cancellation policy. Short Debug timing is not sustained or phone evidence.

Next finish **surface-aware dry footstep dispatch**, accepted master 5.5.
[Support triangles](../../src/gameplay/simulation/TriangleSupportSurface.h) already
carry Stone/Earth/Wood, and
[authored terrain](../../src/scene/ChapterTerrainSurface.h) assigns earth/stone.
Do not rebuild that classification. The missing link is the actual supporting
surface in dry-footstep event payloads and shared cue selection: current
[emission](../../src/gameplay/simulation/GameSimulation.cpp) uses payload zero,
while platform playback alternates the same two dry clips.

First audit local licensed cue availability, then implement the small typed
surface/event/cue mapping for surfaces genuinely present. Keep the existing
cadence, normalized movement, wall/pause/rope/restore suppression, water contact
priority and safe untagged-ground default. Boundary changes must not replay a
contact. Preserve provenance; if no suitable material cue exists, retain that
asset gap rather than inventing a surface sound or spending. This subsequent
audio/transport change needs listening revalidation; it is not part of the
current visual checkpoint. Low-ceiling weapon lowering and alternating rope
hands are already implemented. Healing still requires the master's bounded
policy decision rather than invented refill rules.

When Sam resumes physical testing, the highest-priority acceptance is the coherent
normal tomb-to-lookout/return and save/death path on the exact candidate, with
mobile Mist Off. The prior denied blind Keeper combat action remains denied;
a new scoped action needs current target/contact evidence and relevant authority.
No device installation, combat/timing retry, public push or release is part of
this local milestone.
