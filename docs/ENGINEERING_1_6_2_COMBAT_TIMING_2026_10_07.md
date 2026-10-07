# Combat timing slice — 7 October 2026

This slice preserves damage, health, ranges, cones and parry eligibility. Physical input timing and rendered blade-contact calibration remain open in the active [tomb finish plan](ENGINEERING_1_6_2_TOMB_FINISH.md).

## Measured timeline

The imported skeleton Attack clip is 2.80 seconds, with 24 joints and RightHand at index 12. Its world-space hand samples in metres were: 0.80 s `(0.093, 1.723, 0.200)`, 1.00 s `(0.000, 1.547, 0.791)`, 1.12 s `(0.115, 0.383, 0.753)`, 1.20 s `(0.132, 0.399, 0.739)` and 1.30 s `(0.150, 0.413, 0.734)`. The asset has no attached blade. These measurements describe hand motion, not a rendered contact or damage observation.

The existing skeleton windup is 1.12 seconds; at 60 Hz it crosses on tick 68, approximately 1.1333 seconds. The renderer's former `1.20` contact-named constant applies to stagger recovery after a parry, rather than the ordinary attack edge. It is now named accordingly. Simulation and ordinary clip mapping share the phase constants in `CombatTimeline.h`; no 80 ms visible error is asserted or compensated.

The player windup crosses on tick 11, approximately 0.1833 seconds. Existing damage resolves at SwingActive entry, while most of the 0.16-second visible downstroke follows that edge. Approximate production anatomical blade-tip coordinates in view space (right, up, forward), from the authored grip and blade axis, are `(0.118, 0.708, 1.069)` at the edge, `(-0.183, 0.276, 1.175)` mid-cut and `(-0.328, -0.249, 1.250)` at its end. Real RT presentation, target geometry and contact must be inspected together before moving the damage edge or adding a bounded sweep.

## Parry presentation regression

An immutable-baseline reproduction exported `SwordCombat.h` from `5fe070e4` and stepped it at 60 Hz. It observed success on tick 80 with reaction `Parried`, remaining time 0.12 and success pulse; tick 81 returned to Idle with reaction None while the enemy remained staggered. The existing immediate-riposte behavior erased the visual reaction after one tick. The baseline reproduction was executed after the patch against the exported old source; it is not claimed as a pre-patch test run.

The simulation now retains a separate 120 ms presentation record with semantic event sequence, entity and authoritative tick. It ages once per presented frame using elapsed frame time, independently of catch-up ticks and discarded hitch time. Events emitted later in a frame begin a full envelope for the first resulting pose. Direct fixed stepping also ages the envelope. Gameplay can riposte immediately on the next tick. Pause, route/checkpoint import and death clear this presentation state. No eligibility window was widened.

## Validation and limits

- The affected Debug simulation-gameplay and held-socket CTests passed, 2/2, after correcting copied command-counter setup and old torch fixture expectations. The original failed run is retained.
- Gameplay regression cases cover 15/30/60/120 Hz, catch-up batches, zero-tick 120 Hz frames, 100 ms and over-cap hitches, ordered event identity, immediate riposte and lifecycle clearing.
- Windows Debug app builds with the integrated worktree. This is build evidence, not owner feel, physical controller evidence or a rendered contact calibration.
- Logs remain in the existing task-4 evidence directory: `post-reset-original-parry-repro-02.log`, `post-reset-combat-rag-native-tests-01.log`, `post-reset-combat-rag-native-tests-02.log` and `post-reset-combat-rag-native-build-04.log`.
