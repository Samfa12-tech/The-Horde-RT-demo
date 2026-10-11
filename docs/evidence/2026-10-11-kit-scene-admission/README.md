# Kit scene admission, 11 October 2026

Development milestone on PR27, based on `2c23ae49`. **The complete chapter remains
unfinished.** The owner deferred replacement-canopy/tree-generator work; the
supplied grounded trees remain unchanged.

## Implemented and reproduced

Kit's approved indexed Warden derivative now renders with its real four PBR maps,
idle/walk clips and native terrain support. The existing third character BLAS
allocation is sized for either Keeper or Kit. Ownership changes rebuild that
allocation; poses within an owner refit it. Geometry, TLAS placement and material
metadata publish in the same submitted frame. Pending work survives abandoned
recording. Both waterfall actor lanes/corpse state remain independent.

The first actual startup failed at fourteen registered assets against the fixed
thirteen limit ([preserved error](startup-failure.txt)). The chapter now omits the
unused freestanding laboratory glass box; separate RT Lab/nonchapter scenes
retain it. The lantern's actual glass and every chapter surface remain. A chapter
request for that separate test fixture rejects explicitly. No fourth actor,
new descriptor, increased primitive/material/texture capacity or shader budget.
A native CPU preflight now loads the complete chapter roster and checks its
capacity, texture counts and sole emissive-map owner before Vulkan startup.

Kit uses 26,945 indexed vertices and 25,968 triangles. Upload is 18 base-color,
15 normal, 15 ORM and one emissive layer. The actor maps append after terrain;
Windows/ASTC tests prove every preceding encoded mip remains byte-identical.
The old fallback emissive array has no material consumers; Kit is its sole new
mapped owner. The closed package roster gains exactly two model files and four
platform maps. Masters and processing inputs stay outside packages.

Companion motion uses the imported native surface and existing collision checks,
not the old ribbon height. Missing/stale terrain stops motion. Reversible defaults
retain 1.8 m/s travel and bound turns to 2.4 rad/s. Waiting turns toward the player;
pose time freezes on pause and zero-delta finalization. A capture exposed that
zero-delta finalization had selected idle after walking; the regression now
preserves the clip and clock. Reset generations invalidate pose caching.

This is bounded actor-allocation reuse, **not GPU zone streaming**. Keeper source
buffers and world/texture resources remain resident. GPU resource retirement,
reverse zone admission and sustained/phone cost are still required.

## Inspected RT gallery

Actual RTX 5050 Laptop GPU, 960x540 at 100%, High optical bundle, unchanged effects.
Each frame follows the diagnostic accepted-hit/death seed, physical rope ascent
and native-grounded route. These are deterministic inspections, not an owner-
completed encounter. Departure consumes a fresh raise and an explicit diagnostic
presentation acknowledgement. It does not claim owner proof/listening acceptance.
Overlays are deliberately absent from these scene captures.

| State | RayTracingPipeline | RayQueryCompute |
|---|---|---|
| Reunion: lantern Low, grounded idle facing player | [image](reunion-pipeline.png) / [record](reunion-pipeline.json) | [image](reunion-query.png) / [record](reunion-query.json) |
| Fresh raise: departure walking pose and shadow | [image](departure-pipeline.png) / [record](departure-pipeline.json) | [image](departure-query.png) / [record](departure-query.json) |

All four images were opened and inspected. Kit is visible, human-scale and grounded;
the two backends agree visually. The walk pose is distinct and its cast shadow
follows the body. These stills do not certify animation blending/foot sliding,
full-route companion behavior, dialogue audibility or sustained performance.

The scene records 34,112,448 static vertex bytes, 5,815,668 index bytes, 3,840 static
material bytes, 512 primitive-metadata bytes, nine static descriptors and
274,027,796 decoded texture-accounting bytes. This texture accounting is not a
measured GPU allocation/residency budget. The 12-frame GPU timestamp samples are
retained in each record; warmup/frozen samples are not a sustained FPS claim.

## Validation and exact candidate

[Full source-input, executable, APK and capture identities](identities.json).
Windows Debug EXE: `db47150b8c14f498fda5b82977babe71e36eee90747da00f230a52e9bbff4b1c`.
Android Debug APK: `29de9f8cb9bd1b1766b6d0825d64a1445f9c33ea658e7d591e608db438581aeb`.

- Debug focused suite: 8/9 initially passed; the two new capture IDs correctly
  failed the exact registry/unknown-ID assertions. Corrected registry passes in
  [5/5 regressions](kit-debug-regressions.log); [3/3 final affected tests](kit-final-debug-tests.log) pass.
- Original full torch coverage passes in **172.39 s**, within the unchanged 180 s
  policy. Character-slot, rescue attachment and adversarial regressions pass.
- Release: [8/8 focused](kit-release-tests-final.log) plus
  [4/4 final affected](kit-final-release-tests.log) pass; Debug/Release EXEs built.
- Closed asset/staging/archive policy: [182 checks pass](kit-package-tests-final.log),
  including missing/corrupt Kit model/maps and source-file exclusion.
- Android: [four native ABIs, Debug build and lint pass](kit-android-final-r2.log).
  Java unit tests executed successfully in the final run. Exact APK roster/hash and held-item attribution pass.
  No installation or device validation in this milestone.
- [Eight raygen variants](kit-shader-freshness.log) and
  [eight QueryCompute variants](kit-query-freshness.log) freshly compile and match
  their generated artifacts. Frozen production budgets are byte-unchanged.
- Existing untracked `.blend1` remains SHA-256
  `0dae1e8fb803336cef2d045f48c5f4c9195a6463f45161cb0b6c7434350769fb`.

CI on `62a688ae` reproduced an outdated exact texture-inventory expectation in
`horde_rt_scene_resource_inventory_tests`: it still expected 17/14/14/0 despite
the admitted Kit family producing 18/15/15/1. The [failed Vulkan job](kit-62a688-vulkan-ci-failed.log)
is preserved. A freshly rebuilt Windows Release executable reproduced it
([local failure](kit-resource-inventory-rebuilt-before.log)); an earlier stale
Debug executable passing was not evidence for current source.

The corrected test retains every capacity/material/primitive assertion and adds
exact checks that the chapter omits only laboratory row 9, Kit owns row 25, and
Kit alone owns the mapped emissive image. Rebuilt Debug and Release preflight/
inventory tests pass 2/2 ([Debug](kit-resource-inventory-debug-after.log),
[Release](kit-resource-inventory-release-after.log)). No production change or
deadline increase follows from this correction. Exact final CI is recorded in
the PR after push. PR27 remains draft with base conflicts; no merge/rebase,
release, signing or device install.

## Still required

Real two-way GPU zone preparation/retirement; graveyard/lid/stake composition;
complete tomb-to-lookout/return gallery and live traversal; full actor lifecycle
and sustained CPU/skinning/AS/upload/memory evidence; owner audio/haptic review;
physical Android visual/lifecycle/performance acceptance. Current/Higher shadows
and saved Lower-to-Current phone migration remain physically pending. Mist issue28
and replacement trees remain deferred.

Owner route when the next integrated playtest is requested: defeat Keeper and
watch retirement; claim lantern and climb; verify visible Kit, Low lantern and a
fresh raise; walk/pause/backtrack to lookout; descend and repeat. Check Kit's feet,
turns, voice/subtitle timing, both guard corpses and equipment/light restoration.
