# Waterfall encounter ownership correction

Baseline source: `1484d72e8e20e5bfd010e810c450cec3c6121663`.
The owner reported the second skeleton stopping after the first died. A focused
shared-simulation reproduction establishes a matching defect, not the exact
unrecorded authoritative state of the phone video.

## Cause and change

Outside the original 2.30 m activation circle but inside the existing Waterfall
room, engagement depended on the temporary attacker token. Recovery or killing
the holder cleared that token before reassignment, making a surviving guard idle.

`SwordCombat` now retains room engagement after the player crosses the original
activation arena, until room exit or Reset. It does not expand activation,
navigation, collision or the one-attacker rule. Fresh room-edge entry does not
engage. Reset/import/retry paths clear the transient flag. Guard/attack poses,
timings, saves and damage rules are unchanged.

## Reproduction and result

The existing CombatFoundation test uses normal movement from activation to the
west room edge, two aimed attacks that actually kill A, and a living B at health
2. Both teaching lessons are still unlearned. No completion flags are granted.

| Witness | Original | Corrected |
| --- | --- | --- |
| Repeated contacts after recovery at room edge | 0 | 2 |
| A/B health after player's two cuts | 0 / 2 | 0 / 2 |
| B windup / practice contact | false / false | true / true |
| Player health during safe practice | 3 | 3 |
| Player health after explicit lesson Skip and B contact | 3 (B idle) | 2 |

The first fixture attempt did not prove the intended kill because the second cut
selected the nearer B after stagger. That failure remains in the ignored report;
the retained original-code comparison corrects aim using actual actor positions.
It does not modify gameplay to obtain the witness.

Focused Debug CTest: **755 assertions, 0 failures, 6.10 s**, unchanged 30-second
deadline. Room exit, edge-only re-entry, original-arena reacquisition and Reset
are checked. Original-code run failed three assertions in 7.57 s. Sanitized short
test outputs are adjacent; full local build/first-fixture logs remain under
`reports/held-guard-20261025/teaching-room-edge-*`.

The independently rebuilt existing Waterfall production-route test also passes
in Debug in 7.52 s. The first root CTest invocation omitted the multi-config
`-C Debug` argument and did not run; its log is retained. The corrected command
uses the existing configuration and unchanged test/deadline.

Safe practice deliberately suppresses incoming health loss until the teaching
requirements complete or the lesson is skipped/disabled; that approved policy
is preserved. The same live B damaging after Skip distinguishes it from broken
guard arithmetic. Full hearts alone are not evidence of a damage regression.

Physical encounter/playability acceptance remains pending; no phone was used for
this correction. Audio/haptic manual revalidation required: **YES**, because the
resumed enemy attacks and accepted damage restore feedback that could previously
stop. No audio gain, cue asset or playback backend changed.
