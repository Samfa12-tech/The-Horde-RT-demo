# Gameplay

The gameplay layer owns the shared route/collision contract and deliberately bounded sequential-enemy showcase state used by Android and Windows.

`SwordCombat.h` owns attack latching, the player sword hit window/cone, enemy approach/attack/death/respawn timing, and damage-pulse state. Rendering receives an immutable snapshot; it does not decide hits or combat phases.

`ShowcaseGameplay.h` owns player vitality/life phase, lantern failure, the smoothed articulated lower-body pose (opposed stride, pelvis bob/sway, torso counter-twist, knee bend, foot lift, and toe roll), route lighting, the plural roster/one-active-enemy director, lich range/charge/hit/death/roof/dawn/ending state, and shared snapshots. `SpatialAudio.h` owns stereo pan, distance rolloff, route obstruction, and footstep cadence. `ShowcaseCheckpoints.h` and `ShowcaseReplay.h` provide twelve deterministic Debug presets and a 13-waypoint collision-integrated route replay; `finale-roof` intentionally stops just before the completed ending overlay, and release-safe encounter retry remains limited to `opening` (skeleton) and `mirror` (lich).

`simulation/GameSimulation.cpp` is now the cross-platform gameplay authority that composes those bounded authored systems at a fixed 60 Hz. `InputSnapshot`/`InputMailbox` provide coherent continuous and monotonic edge input, `SimulationSnapshot` is the immutable renderer/UI view, and the fixed-capacity `GameplayEvent` queue is the semantic audio/haptics boundary. Platform code must not recreate this orchestration.

Accepted nonfatal player hits emit `PlayerDamaged`; the lethal hit emits only `PlayerKilled`, so platform feedback does not layer damage and fatal haptics for the same hit. Android preserves the authored 140 ms separation between a skeleton sword impact and its subsequent fall cue.

Implemented combat includes skeleton approach/attack/death/respawn, a three-hit ranged lich finale, and three player vitality with one-second invulnerability, a 0.65-second fatal hold, and retry/restart/quit death flow. Lich death now advances sequentially through its fall, physical roof opening, a 1.75-second RT dawn reveal, and the completed contextual ending. Benchmark, replay, and capture paths are damage-immune. Block/dodge, broader AI, and simultaneous skinned enemies remain deferred.
