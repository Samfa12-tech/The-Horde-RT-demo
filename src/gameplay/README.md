# Gameplay

The gameplay layer owns the shared route/collision contract and deliberately bounded sequential-enemy showcase state used by Android and Windows.

`SwordCombat.h` owns attack latching, the player sword hit window/cone, enemy approach/attack/death/respawn timing, and damage-pulse state. Rendering receives an immutable snapshot; it does not decide hits or combat phases.

`ShowcaseGameplay.h` owns lantern failure, lower-body pose, route lighting, the plural roster/one-active-enemy director, lich range/charge/hit/death/roof state, and shared snapshots. `SpatialAudio.h` owns stereo pan, distance rolloff, route obstruction, and footstep cadence. `ShowcaseCheckpoints.h` and `ShowcaseReplay.h` provide twelve deterministic Debug presets and a 13-waypoint collision-integrated route replay.

Implemented combat includes skeleton approach/attack/death/respawn and a three-hit ranged lich finale. Block/dodge, player death, broader AI, and simultaneous skinned enemies remain deferred.
