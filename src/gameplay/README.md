# Gameplay

The gameplay layer owns the shared route/collision contract and deliberately bounded sequential-enemy showcase state used by Android and Windows.

`SwordCombat.h` owns attack latching, the player sword hit window/cone, enemy approach/attack/death/respawn timing, and damage-pulse state. Rendering receives an immutable snapshot; it does not decide hits or combat phases.

`ShowcaseGameplay.h` owns player vitality/life phase, lantern failure, the smoothed articulated lower-body pose (opposed stride, pelvis bob/sway, torso counter-twist, knee bend, foot lift, and toe roll), route lighting, the plural roster/one-active-enemy director, lich range/charge/hit/death/roof/dawn/ending state, and shared snapshots. `SpatialAudio.h` owns stereo pan, distance rolloff, route obstruction, and footstep cadence. `ShowcaseCheckpoints.h` and `ShowcaseReplay.h` provide twelve deterministic Debug presets and a 13-waypoint collision-integrated route replay; `finale-roof` intentionally stops just before the completed ending overlay, and release-safe encounter retry remains limited to `opening` (skeleton) and `mirror` (lich).

Implemented combat includes skeleton approach/attack/death/respawn, a three-hit ranged lich finale, and three player vitality with one-second invulnerability, a 0.65-second fatal hold, and retry/restart/quit death flow. Lich death now advances sequentially through its fall, physical roof opening, a 1.75-second RT dawn reveal, and the completed contextual ending. Benchmark, replay, and capture paths are damage-immune. Block/dodge, broader AI, and simultaneous skinned enemies remain deferred.
