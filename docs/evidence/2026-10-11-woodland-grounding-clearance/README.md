# Woodland grounding and native-terrain held-item clearance

Development checkpoint following `d24a9971016050964a7a9a1f2cf1ac383b411324`.
The complete chapter is **unfinished**. This closes reproduced placement and
held-item defects; it does not admit the sparse tree pair as the final forest.

## Reproduction and correction

The owner's floating-root report reproduced on actual native imported geometry:
2,982 of 4,992 root-base vertices were insufficiently embedded, with a worst
2.16087 m gap. The former centre-height/minimum-bounds test passed because it
tested its own placement arithmetic, not the root footprint.

The versioned v3 placement receipt preserves v2 and every original GLB/Blender
master. Fifteen of 64 trees move, at most 2 m; the trail retains its 2.4 m clear
width plus capsule margin. Native runtime intake measures the imported root-tip
base band (lowest 20 mm), fits an upright rigid tree with 20 mm embedding, and
rejects missing support, burial over 0.65 m, or a collision-receipt mismatch over
2 mm. Maximum actual burial is 0.598622 m. All 4,992 checked base vertices now
embed; worst signed gap is -0.0199988 m (floating-point tolerance included).
This is vertex-sampled root contact, not a continuous soil/root intersection
proof. No terrain deformation or per-vertex tree distortion was introduced.

The shared collision bounds clip actual bark triangles through the horizontal
slab from centre terrain height to 2 m above it. They no longer enclose all the
buried root tips. Both renderer and simulation use the same generated receipt;
native import independently checks the resulting bounds. This conservative
trunk proxy is not a terrain-conforming root collider or general branch physics.
Missing footprint and steep-plane negative tests preserve the prior static
owner byte-for-byte on failure.

At the lookout, player movement already used exported terrain, but held-item
clearance still used the obsolete ribbon support. A native regression proves
2.7 m clear ahead while the old held-item query returned 0.610558 m. Kinematics
now borrows the generation-validated terrain during its synchronous fixed-step
solve. No terrain pointer enters immutable snapshots. Existing shaft, obstacle,
terrain-gap and tree tests remain active. A successful continuous full-reach
sweep avoids 108 redundant incremental sweeps in unobstructed authored terrain;
blocked paths retain the bounded search and refinement. No shader change.

## Inspected actual RT images

Debug, NVIDIA RTX 5050 Laptop GPU, 960x540, 100% scale, Diagnostic High shader
profile, unchanged settings recorded in each adjacent manifest. Route checkpoints
really ascend and walk to the route station, then settle at the final view for
120 fixed steps before freezing; render settling alone cannot settle a pendulum.
Historical failed images and messages remain preserved. No visibility exemptions
or relaxed capture assertions were added.

| View | Before | Corrected |
|---|---|---|
| Second bend, Pipeline | ![Roots before](before-second-bend.png) | ![Roots embedded](after-second-bend.png) |
| Lookout, Pipeline | ![Retracted hands](before-lookout.png) | ![Restored carry](after-lookout-pipeline.png) |

![Matching lookout, QueryCompute](after-lookout-query.png)

![West landing, Pipeline](after-west.png)

Every displayed image was opened and inspected. Corrected second-bend, west
landing and lookout QueryCompute captures pass strict player/grip/lantern
checks. The matching Pipeline lookout also restores the visible carry; its
final same-build manifest is retained beside this report. These frames do not
complete the requested full 360-degree/return/second-ascent gallery or owner
visual acceptance. The town still shows placeholder blocks, and the supplied
trees still have sparse crowns. Easy Tree is a researched fuller-canopy candidate,
not installed, exported or admitted by this checkpoint.

## Validation and exact identities

- Windows Debug: 10/10 affected CTests pass, 122.80 s total.
- Windows Release: same 10/10 pass, 15.06 s total. The first parallel MSBuild
  attempt repeatedly created idle worker nodes; its process tree was inspected,
  then that build alone stopped and rerun serially. Original log retained at
  `reports/woodland-root-clearance-release-build.log`; no test deadline changed.
- Included: held sockets, development capture registry, native forest dressing,
  source/root placement freshness, RT resource inventory, legacy/native terrain
  journeys, rescue rig and adversarial rescue regressions.
- Native terrain journey: 15,684 cases, zero failures. Tree collisions, open
  shaft, safe traversal, repeat cycle and readiness regressions remain covered.
- Android Debug: four ABIs rebuilt; Gradle build/lint/test succeeded. The unchanged
  Java and lint work was up-to-date; retained XML reports contain 284 tests,
  zero failures/errors/skips. Actual APK world/held-asset/strict texture package
  checks pass. APK was **not installed or physically tested**.
- No shader, atlas layer, material/primitive capacity, quality, render-scale,
  source-model or original torch-test deadline/coverage change.
- [Full artifact/source hashes and capture results](identities.json). Debug EXE:
  `25da7fbd4276e05a90a2d9094c39a87b0b20c1ba3d26f2d0af3cd7e718af0d6f`;
  Release EXE: `85e85d5161fbeca030b0982f3ccea1d76445ca7ad1ffdd9c080bbf5bd4748e71`;
  APK: `4f69959f8951d10b7f8be7c7520198c40940b7614df8d0d557c870edce3fb6e5`.
- The unrelated untracked `tomb-dressing-v01.blend1` remains byte-identical:
  `0dae1e8fb803336cef2d045f48c5f4c9195a6463f45161cb0b6c7434350769fb`.

64 trees retain four material/primitive groups and 323,264 source triangles,
compared with the previous ten-tree 50,510 triangles. Actual captured static
vertex/index bytes grow from 12,976,128/2,231,148 to 32,388,480/5,504,196;
production-prop BLAS bytes from 7,547,008 to 24,416,128. Texture allocation remains
257,250,584 bytes and descriptor count nine. These are allocations, not a mobile
budget acceptance. Twelve-frame capture timings are not sustained performance;
tree instancing/LOD and fuller-canopy costs still need deliberate measurement.

Audio/haptic manual revalidation required: **NO for these corrections** (no
event/audio changes). Previously pending chapter voice/cue listening remains open.
Current/Higher phone shadows and saved Lower-to-Current migration remain pending.
Visible Kit, real GPU zone retirement, coherent distant structures, final wooded
composition and the full chapter acceptance still require further work.

Next owner route: inspect roots at both bends, turn toward the lookout and verify
the lantern/arm no longer retreat, then backtrack and complete a rope round-trip.
Do not infer phone, full-chapter or final woodland acceptance from these captures.
