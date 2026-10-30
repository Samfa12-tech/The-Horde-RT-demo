# Keeper death presentation and submission-safe refit

Development milestone on PR27, based on `5694eec69d4b35b9ea6cf165157ff1ddf9a81308`.
The chapter is **incomplete**. This removes the completed Keeper presentation;
it does not yet transfer the third slot to Kit or retire GPU zone resources.

Previously, every Dead Keeper remained visible indefinitely. Chapter play now
retains the full 2.967-second authored death animation, holds for 0.25 seconds,
then contracts its existing triangles over 0.75 seconds before masking the actor.
These are reversible presentation defaults, not owner-approved final timing.
At most 24 additional pose buckets use the existing vertex upload/BLAS refit.
No shader, extra geometry, texture, light, instance capacity or budget change.
Legacy captures remain unchanged. Damage, rewards, death audio and the existing
post-death mist schedule remain separate and unchanged.

A native regression reproduced a lost-refit hazard: after uploading a pose,
an abandoned frame followed by the same pose cleared its pending BLAS refit.
Pending work now survives until successful queue submission. Windows and Android
both acknowledge only after `vkQueueSubmit` succeeds. The regression uses actual
native skinning and CPU-backed mapped upload; failed Vulkan submission is modeled
at that boundary, not claimed as a physical driver fault injection.

## Inspected actual RT views

NVIDIA GeForce RTX 5050 Laptop GPU; Diagnostic High optics, HIGHER shadows,
100% scale, 960x540, identical camera `(-31.9, -15.2)`, yaw -pi/2, pitch -0.35.
These diagnostic seeds use live recognition and three accepted hits, then 193,
216 and 240 shared fixed ticks after death. They seed normal defeat/chest-unlock
owners and freeze. They are not a player-completed encounter or motion capture.

| Stage | Pipeline | QueryCompute |
| --- | --- | --- |
| Full final pose, before dissolve | ![Held](held-pipeline.png) | ![Held Query](held-query.png) |
| Partially dissolved | ![Dissolve](dissolve-pipeline.png) | ![Dissolve Query](dissolve-query.png) |
| Actor removed | ![Retired](retired-pipeline.png) | ![Retired Query](retired-query.png) |

All six original frames were opened and inspected. Keeper geometry and its
mirror reflection break up and disappear; chest/walls remain. The effect is
restrained in this dark scene. Frozen images cannot establish smoothness or owner
acceptance. Backend pairs differ at only 15/16/17 pixels respectively, but are
not byte-identical. No exposure adjustment was applied to these images.

Per-frame manifests retain shader identities, allocation counts, diagnostics
and timing. Twelve settling frames are not sustained performance evidence;
concurrent host builds further prevent interpreting their timing as a benchmark.
[Exact source, EXE/APK, capture and preserved-backup hashes](identities.json).

## Validation

- Debug: combat foundation, character render slot and development capture tests
  **3/3 passed**, 9.75 seconds. Additional scene-resource inventory, rescue
  integration, attachment and adversarial regressions **4/4 passed**, 67.49 seconds.
- Release: the three directly affected tests **3/3 passed**, 3.79 seconds;
  Release game built successfully.
- Android: all four native ABIs and Debug APK built; Java compile and lint ran.
  Unit tests were **UP-TO-DATE**, not a newly executed run. Runtime held-item/world
  package checks passed. No device install or physical validation was performed.
- Failure evidence retained locally: `reports/chapter-death-red.log` (two
  behavior failures), `chapter-refit-red.log` (lost-refit reproduction), first
  build's fixed-span template error, and sandbox-denied Gradle lock attempt.
  The first six dark captures omitted the seed's reward event; corrected seed
  has a regression assertion and the gallery above uses only corrected r2 output.
- Shader sources/binaries, quality settings, frozen budgets, original torch
  test coverage/deadline and unrelated source masters remain unchanged.

Owner check next: defeat Keeper, watch the complete fall and short disappearance,
check the reflection, then claim the lantern and complete rope ascent/descent.
Confirm there is no corpse reappearance after backtracking/retry. Phone checks
for this presentation, Current/Higher shadows and saved Lower-to-Current migration
remain pending. Prior chapter voice/haptic acceptance is still open; this change
introduces no new audio/haptic semantics.
