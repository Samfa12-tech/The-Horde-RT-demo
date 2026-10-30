# Stationary planted pull-up pose proof — 12 October 2026

This is an **inactive native pose capability**, preparing the grounded stake/rim
handoff. It does not replace the overhead apparatus or complete the chapter.
Source parent: `1578519d506c476f7ba268986747093a1ef083d8`. Exact source, existing
asset and candidate binary hashes are in [identities.json](identities.json).

The old standing shoulder height cannot reach a ground-level rope during the
final pull-up. The new optional `EvaluatePlayerPose` constraint moves the entire
Hips subtree, folds both actual imported leg chains at their unchanged lengths,
and restores the original ankle/foot descendant transforms. Arms solve after the
torso moved. The resulting immutable palette includes its real Hips attachment
and is shared by body and viewmodel. Zero remains the old palette; it performs
no leg lookup, articulation or authored-palette copy.

Reversible engineering recipe: Idle only, pelvis drop in `[0, 0.72]` metres,
setback along model -Z at 0.25 times drop (maximum 18 cm), separated forward knee
poles. These are pose bounds, not owner-approved gameplay tuning. Positive
requests on moving/other clips reject and invalidate the old pose. This adds no
ordinary crouch control or new movement authority.

## Reproduction and correction

- [Initial failing behavioural test](planted-stance-before-test.log): the requested
  planted leg output was absent.
- [Vertical-only failure](planted-stance-knee-before-test.log): the deep Idle fold
  put knees below the standing mesh's support plane. The original GLB's ankles
  are behind the hip joints, so merely lowering the pelvis selected the wrong
  physical fold for this application.
- [Setback experiment](planted-stance-setback-test.log): Idle became viable but
  a walking pose still failed clearance. Stationary traversal already selects
  Idle. Restricting the new constraint to Idle preserves ordinary locomotion
  and explicitly rejects unsupported use rather than silently changing its feet.
- [Final native proof](planted-stance-final-proof.log): 18 samples include twelve
  positive stationary poses and six byte-identical zero-pose compatibility
  cases. Both ankles retain exact position/orientation/scale; thighs/shins keep
  their imported lengths within 0.1 mm; knees clear the actual standing mesh
  support plane by at least 5 cm. Both real arms reach low targets in three Idle
  phases. Independent and shared body/viewmodel skinning agree byte for byte.
  Invalid range/nonfinite input, stale-palette rejection and arm-only rigs are
  covered. This numerical proof does not certify boot-sole, armour or scenery
  clearance during a moving traversal.

## Current-source checks

- Windows Debug and Release application/affected rig targets rebuilt.
- Six affected CTests pass in [Debug](planted-stance-admitted-debug-tests.log) and
  [Release](planted-stance-admitted-release-tests.log): directional rig, analytic
  arm reference, skinned character smoke, viewmodel admission, rescue rig and
  adversarial rescue regression. Deadlines/assertions remain unchanged.
- [Android](planted-stance-admitted-android.log): four native ABIs rebuilt;
  assembleDebug, Java unit-test and lint tasks successful. Unchanged Java/lint
  analysis was UP-TO-DATE. No installation or phone execution occurred.
- Closed package and [held-item/source separation](planted-stance-admitted-held-item-package.log)
  checks pass. Original assets, source/backups, three actors, 18-layer ceiling,
  shader words and frozen budget file are preserved. The original torch sweep
  and its 180-second policy remain unchanged; its prior checkpoint evidence is
  retained rather than being claimed as a new run here.
- Exact pushed-commit CI results are reported in PR27/handoff after execution.

## Inspected normal-play RT comparison

The optional pose remains zero in gameplay. Hidden, input-free captures from
`chapter-landing-north` were opened and inspected through both real hardware
backends on RTX 5050 Laptop: 960×540, 100% scale, Diagnostic High bundle, twelve
frozen settling frames. Both have **zero changed pixels** versus the matching
1578519d checkpoint. This preserves its floor shadow and shaft carry appearance.
Camera/state, shader identities, resource observations and presentation facts
are in the manifests. These short captures are not sustained performance data.

| Backend | Inspected frame | Exact manifest |
| --- | --- | --- |
| RayTracingPipeline | [Normal pose](pipeline-normal.png) | [Manifest](pipeline-normal.json) |
| RayQueryCompute | [Normal pose](query-normal.png) | [Manifest](query-normal.json) |

[Pixel comparison receipt](planted-stance-image-comparison.json). The new folded
pose has **not** been rendered/accepted in a live traversal yet.

## Next integration and acceptance gates

Choose the actual grounded stake/rim route and solve its tether/contact/load,
deployment and hanging span together. Publish a bounded stationary pose through
the traversal snapshot; use the same solved hips for stowed equipment/light,
the same stance for boot grounding and appropriate camera/capsule clearance.
Verify ascent/pull-up and descent separately, including pause, reset, stale
readiness and repeat traversal; inspect both RT backends before adoption.
Graveyard dressing, genuine GPU zone retirement, full chapter gallery, sustained
measurements and owner/phone acceptance remain open. Mist issue28 and replacement
tree generators remain deferred.
