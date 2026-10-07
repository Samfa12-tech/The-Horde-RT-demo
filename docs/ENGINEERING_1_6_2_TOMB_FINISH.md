# 1.6.2 tomb finish — active post-reset work

Updated 7 October 2026. Authority: the owner's `Horde-1.6.2-after-reset-goal.txt`, explicitly adopted by the latest resume request. Supporting PR16 reference: `48fd8d6e0ee73c480502906104593c4e6d3b8aae`. Remote planning advanced to `c03db1c94dd058e8416d1927c908b6b7298d16e0`; those documentation changes are not merged into runtime wholesale. Work continues in the existing PR18 checkout/branch. No duplicate workspace, reset, release, merge, signing or paid generation is authorized.

## Preserved completed evidence

[Pause checkpoint](ENGINEERING_1_6_2_PAUSE_2026_10_05.md), [review candidate](ENGINEERING_1_6_2_REVIEW_CANDIDATE.md), [final Graphics ledger](ENGINEERING_1_6_2_FINAL_GRAPHICS.md), and [second pass](ENGINEERING_1_6_2_SECOND_PASS.md) retain their exact results. Runtime `1334cc9c58ec97940ac10d861f0397143ca7a9f4` passed both allocated-phone backends' route/capture/Home-resume checks with saved preferences unchanged. Staged mobile defaults, Mist rollback and cold-restart persistence passed. Runtime CI passed all six jobs; documentation `5fe070e42541fe8a4b4bc856360b73e1d94c7489` also has six successful jobs in each refreshed push/PR run. These results do not prove sustained 30 FPS or the fresh no-preview Back defect.

## Remaining work and current disposition

| Slice | Current status | Remaining evidence or decision |
| --- | --- | --- |
| Fresh Graphics restore, Android cancellation, Windows focus, Graphics viewport | Implemented in `e1f1b9a0` and `a73a8084`; [regression evidence](ENGINEERING_1_6_2_INPUT_REGRESSIONS_2026_10_07.md) | Integrated packages and affected physical touch/lifecycle/controller checks |
| Combat contact/edge timeline and parry presentation | Measured shared timeline and persistent parry presentation committed in `a7b500bb`; timestamp scheduler/native receipt and presentation trace under validation | Integrated rendered contact calibration; physical latency and owner feel. Current host timing tests cover 15/30/60/120 FPS, hitches and late catch-up parry misses |
| Native actions and three original hearts | Press-down Swing/Parry/Dodge and original native hearts committed in `b76ce0e6`; 166 Java tests in 28 classes pass, lint 0 errors/55 warnings | Integrated native package, physical move/look/action/cancellation/touch comfort and owner visual acceptance |
| Sword overhead clearance | Shared blade-envelope response and imported-mesh tests in progress; actual final arm rig exposes a reach failure in some lowered targets | Correct reachable shared pose, then prove final grips/blade under ceilings and inspect actual world/shadow/reflection motion |
| Player-only rag torch | Separate production player resource committed in `40f92c6a`; provenance, closed packaging and atlas payload preservation recorded; first exact Windows RT capture complete | Motion/flame/hand fit, drop/drench, shadows/reflections, Android package/device and resource/pacing costs; owner acceptance |
| Shared equipment and waterfall encounter | Not implemented | Start torch held/sword sheathed; draw/sheath/stow/restore authority; safe early cues, relocate the same two skeletons and validate route/recovery |
| Draw/sheath audio | Local FilmCow bank located; no new cue admitted | Audition, finished-game versus public raw-redistribution rights, selected derivative provenance and event timing |
| Compact selected menu scene | Not implemented | Real hanging lantern and native controls; **owner visual/cost feedback before expansion** |
| Remaining UI/loading/Graphics simplification | Pending compact-scene feedback | Functional surfaces, actual loading state/small spinner, reduced motion and safe Graphics acknowledgements |
| Glass apply latency and ordinary foreground pause work | Prior slow apply remains recorded; new investigation pending | Bounded stage timings; measured suppression and recovery, distinct from live preview/background suspension |
| Phone traced 50/40/33 comparison | Allocation question pending; no new device use | Same device/backend/output/route, whole-frame/present pacing, combat/effects, Keeper/reward overlap, sustained thermals/power/memory; **owner comparison decision** |
| Immutable review candidate and Eric audit | Pending integration | Exact source/package hashes, aggregate CI, affected recorded gates and independent audit; owner approval still required for release |

Fresh/reset phone defaults remain 50%, Mobile water/fire, Glass Off, Current shadows, cap30, Mist On. Desktop stays 100%, High water/fire, Glass On, Current shadows, cap30, Mist On. Preserve all saved/custom settings. No silent 33% default or sustained-FPS inference from reciprocal GPU timing.

## Reconciled scope

The player-only disposable rag torch, shared equipment and relocation of the existing two skeletons to the waterfall room are now explicitly included in 1.6.2. The selected central lantern/menu/HUD direction is also included, with its compact-scene feedback gate. Older conflicting future-version clauses are historical, not current exclusions.

Preserve the complete tomb route, four distinct openings, ending, existing health/damage/death rules and accepted Keeper flank-light timing through its actual death then Off. Do not add Kit/voice/dialogue, rope/climbing, forest/night/town/adventure, start-facing-collapse camera, running, additional enemies, healing, campaign saves or optional dressing/cobweb/ambience work.

Closed selective-opacity, Lower-indirect and mirror-to-stone negative experiments remain closed. Preserve [future performance investigation](PERFORMANCE_INVESTIGATION_FUTURE.md); a full historical bisect, texture/LOD programme, temporal/vendor rewrite or broader renderer change is not automatically admitted. A bounded spatial upscaler is conditional on comparison evidence and an owner decision.

Build correctness, physical-device evidence, owner visual/audio/haptic/combat-feel acceptance, sustained performance and Eric's independent audit are separate gates. This ledger records work, not completion certification.

## Current CI and additional evidence

`a2ad00f8840715cb3eedb542764556bc6165a410` passed all six jobs in each push/PR run. At `40f92c6a4f7374a704bcbe0383844fd301dc90f1`, push `37570840989` and PR `37570845018` completed with three successful jobs and three failed host jobs each. GCC/Clang each passed 77/78 cases and failed the former player-torch socket assertion. MSVC passed 86/88 and additionally failed the new hash-pinned manifest after Windows newline conversion. Neither aggregate run is green. The socket test now imports the actual player Rag asset; its integration rerun is pending clearance correction. An explicit LF checkout rule preserves the 830-byte manifest SHA-256 `5316b2072008da441024cda1153f725f5afcd7d366f3a5b89dece98ec74617aa` even with `core.autocrlf=true`; the 83-case closed asset policy passes locally.

The immutable `40f92c6a` Windows Debug executable SHA-256 is `fd8ef6aacce03dfaf61fda746f4557a3905520eb8ccb91421de1e73208d02a7d`. Its Vulkan-validation portrait `player-viewmodel-grips` capture at native 540×960 on RTX 5050 Laptop completed and stopped the owned app. The final accepted/completed RT identity is scene epoch 2, record/submission/completion 12, simulation tick 1; its resources report 19 BLAS, one TLAS and 25 TLAS instances. The image is `task-4/rag-40f92-portrait-grips-20261007/136-player-viewmodel-grips.png`, SHA-256 `ab23bb3c2eb704fa2a1223922a8203639bc4269d83a62ab7fe1b8081e724c3d5`. This frozen grip capture establishes a presented RT frame, not moving torch fit, owner acceptance or sustained FPS.
