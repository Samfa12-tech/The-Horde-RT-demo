# Measured two-skeleton combat slice

**Date:** 2026-08-11

**Status:** Host-validated development candidate; Android-device performance and hands-on promotion gates pending

**Publication:** None. Showcase Alpha 0.1.3 artifacts and itch builds remain unchanged.

## Outcome

The first genuine multi-enemy encounter is implemented as a hard-capacity pair, not a general ECS or broader horde system. The normal development route starts `SkeletonA` at `(-0.75, -4.65)` and `SkeletonB` at `(0.75, -4.65)`. Each has stable identity, independent action/health/death state, ordered entity-aware events, persistent death until retry/reset, and a shared encounter completion condition.

Movement retains the authored corridor collision resolver. The pair maintains deterministic 0.70 m separation, including after one entity dies: the corpse remains fixed and the survivor is sweep-corrected or reverted without crossing walls. One attack token is selected by strict distance, with exact ties resolved to the lower stable entity ID. A player swing can kill only the nearest valid alive target in the existing range/cone; missed swings target `Invalid` in semantic event data.

Historical capture checkpoints 0-11 deliberately import the original singular skeleton at `(0, -4.65)` and finalize at zero delta. Checkpoint 12, `two-enemy-combat`, imports the pair. Live route reset and encounter retry restore both entities.

## Renderer contract

`SimulationSnapshot` publishes a bounded two-entry skeleton array, renderable count, alive count, attacker ID, and encounter completion. The compatibility `swordCombat` projection remains mapped to Skeleton A for unchanged consumers.

`CharacterRenderSlot` emits up to two skeleton instances while the lich route remains singular:

- matching clips/times share pose bucket 0 and its BLAS;
- divergent action/death states may use pose bucket 1;
- Dead times clamp to the loaded Dead clip duration, so final corpses converge to a shared pose and stop redundant refits;
- physical TLAS slots use custom indices 2 and 18; semantic indices 0-17 are unchanged;
- unused instances are mask zero but retain identity/invertible transforms and stable custom indices;
- the scene now reports nine BLAS and nineteen physical TLAS slots.

The shader reads the bounded skeleton pose segment selected by instance custom index. Native `vkCmdTraceRaysKHR`, phone-safe `rayQueryEXT`, recursion depth one, one frame in flight, strict ASTC, BGRA presentation correction, and honest `rtScene.presented` semantics are unchanged.

## Diagnostics and evidence tooling

The developer overlay and Android state evidence report active enemy entities, attacker entity ID, and skeleton pose-bucket count. The default 75% device gate now measures six checkpoints: `opening`, `two-enemy-combat`, `worst-bend`, `skylight`, `green`, and `lich`. Deterministic capture order now contains 13 entries.

Debug Android automation accepts `-GpuTiming Enabled|Disabled`. Disabled mode removes timestamp-query initialization, reset/writes, collection, and readback only; RT rendering and the strict below-20 ms CPU gate remain unchanged. Every run now:

- hashes the local Debug APK and installed `base.apk` and rejects a mismatch;
- records source commit, dirty state, embedded raygen SHA-256, device, scale, checkpoints, and instrumentation mode;
- retains raw before/after thermal and battery evidence.

`tools/compare-android-gpu-timing-ab.ps1` requires one enabled and one disabled run with matching artifact/source/shader/device/scale/checkpoint provenance, starting AP/SKIN/BAT temperatures within 2 C by default, thermal-status difference no greater than one, all checkpoint medians below 20 ms, and no enabled-versus-disabled regression above the 15% investigation threshold.

## Host acceptance evidence

The final exact-tree Host foundation run is `reports/foundation-runs/run-20260811-223230` and passed all seven stages:

- embedded raygen matched the compiled shader; source SHA-256 `2c748d6631d6e0d23436974127a6c6ac08638a108df04d302992924fea5a7e67`, embedded include SHA-256 `2e889222178fba1d973d1a9b9795c42d725f75972b29b712ac2001702499f7e4`;
- Windows Debug and Release each passed 12/12 CTests;
- all 13 Windows 960x540 scene-only captures honestly presented RT frames;
- Android Debug and unsigned Release built for all configured ABIs and `lintRelease` passed;
- negative release-identity, stale-shader, packaging, licence, and evidence-hash gates passed;
- validation artifacts were marked unpublishable.

The initial capture immediately following the ten-minute clean build was pixel-exact but had an unmatched 6.549250 ms overall CPU median, 8.2% above the accepted 6.052750 ms host timing baseline. An idle rerun of the same already-built executable at `captures/windows-idle-rerun` measured 6.051050 ms overall CPU median and 0.566675 ms average GPU RT command-buffer time. The repository comparator passed against the published 12-capture baseline with zero changed pixels. The explicitly reviewed `two-enemy-combat` PNG is SHA-256 `56b40f5118109d58e17976133789b41814e045c36c9ad3c6945f3d46a9d3708a` and measured 6.050800 ms median.

The generated validation artifacts are evidence only:

- unsigned Android APK SHA-256 `2525905f974579afa2e7de9d8367082d8722d548b5ee2668bfa7237cc357205e`;
- unpublishable Windows ZIP SHA-256 `13662f64629ae92f2e81b8df4972f93ade0c73e8106604814700b53c6a29fb3a`.

## Pending Android promotion gate

The authorised `SM-S948B` was connected but remained locked/dozing during this task, so no new RT presentation, timing, lifecycle, capture, audio, haptic, or touch evidence is claimed. The earlier 23.604 ms lich failure belongs to the pre-two-enemy development build and cannot yet be attributed to GPU timing overhead because its temperature rose materially during the run.

Before this candidate becomes the new playable phone baseline:

1. Run matched cooled 75% `lich` sessions from the same APK with GPU timing enabled and disabled, then pass the A/B comparator.
2. Run the complete six-checkpoint 75% gate with honest presentation, thermal status no worse than 3, and every median-of-three 120-frame window below 20 ms.
3. Report 100% separately and complete deterministic replay, all 13 captures, and Home/resume recovery.
4. Hands-on verify touch/camera comfort, two-enemy readability, distinct positional audio/haptic feedback, persistent corpse/survivor spacing, lifecycle recovery, and combat feel.

Do not publish, sign, upload, or describe the two-skeleton candidate as Android-device-validated until those steps pass.
