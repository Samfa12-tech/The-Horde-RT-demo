# Player normal-palette cache and complete backend round trips

This checkpoint caches inverse-transpose factors once per solved player joint,
then shares them between the independently owned world body and viewmodel.
Production geometry, materials, lighting, shaders, masks, quality and capacities
are unchanged. The chapter remains incomplete; **normal-play residency saving
remains ZERO**.

## Correctness and measured CPU work

The old path computed cofactors, determinant and reciprocal for every positive
vertex influence. The new pose stores unscaled cofactors and reciprocal once per
joint, preserving the original expression order. A singular unused joint remains
legal; a used singular joint fails skinning and clears both streams. Failed
reevaluation invalidates the pose and cache. Consumers do not mutate the cache.
The admitted player palette has 26 joints; its additional factor payload is
approximately 1,144 CPU bytes, not a measured GPU allocation or residency saving.

The existing portable node-pose test now compares 763 cached/legacy arithmetic
cases, including shear, nonuniform/mirrored transforms, determinant-threshold
neighbors and nonfinite values. The analytic arm fixture proves unused/used
singular joints, recovery and stale-pose rejection through native import and
shared-pose skinning. Real world-body/viewmodel streams for 8 poses are byte-exact
before/after in both Debug and Release, including normals and tangents.

An isolated MSVC comparison compiled the original 970858be source and current
source with the same CMake configuration and alternated their order across
4 trials per build type,96 poses per invocation. All 16 streams match in every trial.
Release combined body/viewmodel skinning medians fall from 2.15–2.25ms to
1.80–1.90ms, a 14.1–20.0% reduction. Debug reductions vary 3.2–53.0%; the first
simple sequential comparison overstates the typical gain. These are finite CPU
samples, **not sustained gameplay or phone performance**. Raw samples, original
source identity, executable hashes and stream comparisons are under [cpu](cpu/).

Reproduce stream receipts with `horde_rt_skinned_node_pose_batch_tests
--write-player-skinning-reference <fresh-directory>`. The original source is
`970858bebd2907b2067b9ea11bd769172658d515:src/scene/assets/SkinnedMeshAsset.cpp`;
the isolated pair uses this source and the current source with the same test
driver, C++20, source-root definition and MSVC Debug/Release configuration.

## Actual candidate inspection

Both RTX5050 Laptop GPU backends complete the same native route with ordinary
inputs: landing360, fresh Kit raise/walking, native-terrain bends/lookout360,
return, descent/lower360 and second ascent. Each completes4,961 60 Hz ticks,
1,241 RT frames and 26 captures under the unchanged 180-second process deadline and
150-second route bound. Simulation advances at most 4 fixed ticks per render.
The starting checkpoint imports accepted Keeper defeat/claim; this is not a live
opening/combat or audio/HUD playtest. Audio is locally muted for scoped capture.

All 26 Pipeline images and 25 previously available Query images are pixel-identical
to 970858be. The former Query timeout and its incomplete gallery remain in the
[preceding checkpoint](../2026-10-13-moving-chapter-roundtrip/README.md).
Representative current images from each backend were opened: Kit walking, bend,
lookout, descent initiation and final landing. Raw images omit platform captions.
The previously noted dark foreground shape during stowing remains unchanged and
unattributed; it is not a new normal-cache regression. Deferred canopy work and
current owner visual acceptance remain separate.

| Backend | Render CPU median / p95 | Owning GPU median / p95 | Device-local bytes |
| --- | --- | --- | --- |
| Pipeline |62.12 /114.00ms |5.47 /11.09ms |405,550,288 |
| QueryCompute |54.98 /61.73ms |5.06 /6.61ms |405,550,288 |

Each GPU result joins 1,241 completed moving submissions, excluding the initial
frozen result and including the final owning completion. Debug Diagnostic,
High/Higher, 960×540 at 100%, full resident scene. Pipeline tails remain high and do
not demonstrate a full-frame performance improvement. These runs are not a
controlled sustained Shipping test or acceptance of a frame-time budget.

![Current Kit walking](windows/pipeline/6-kit-walking.png)
![Current route bend](windows/pipeline/10-outbound-6.png)
![Query second ascent](windows/query/26-repeat-ascent-2.png)

## Validation and identity repair

Windows Debug/Release applications build. In each configuration, 4 focused pose
CTests and 8 affected animation, semantic, rescue/native-terrain, attachment and
adversarial CTests pass. The unchanged full torch gate passes in 168.02s Debug
and 17.87s Release, retaining the original 1,475 legacy + 1,812 roof-case roster and
180-second policy. Existing Vulkan-host CI selects the singular arm fixture;
the portable compiler lanes select the node-pose fixture.

Extended shader checks reproduced stale compatibility and four OpaqueFast hash
assertions after the earlier 6e41b014 grave-lid change. Current artifacts are
unchanged from 970858be. Independent fresh compatibility and eight-key catalog
compilation prove the current words and unchanged frozen budgets before the
expected pins are updated. The full manifest and artifact tests, negative
fixtures and QueryCompute shader test pass. No shader source, catalog, artifact,
budget or structural assertion was changed. The initial sandbox compiler-output
failure, absolute-path invocation failure and stale-pin failures are retained
under [validation](validation/), alongside the successful retries.

Android four-ABI native build, lint, 286 fresh Java tests with zero failures/errors/
skips, actual APK inventory, strict admitted texture/held-item contracts and
development certificate verification pass. No phone action was taken. The new
APK is prepared and uninstalled; earlier physical evidence does not certify it.

Exact artifacts and compiled sources are sealed in [candidate-receipt.json](candidate-receipt.json):

- Debug EXE SHA-256: `1cade3574e84cb166d5f90ef7fc2b554431665375bcc60bf1c0cade52b092075`.
- Release EXE SHA-256: `994f218aedaccfb1331f53b6869d677c71e5b545616ed35de4089bf42e692935`.
- Prepared Debug APK SHA-256: `8c9fb2b0fc2cadf1d9cd441ade88f63e4175b06e5a4e0163d55ae11e71a24e7e`.

Audio/haptic manual revalidation required by this cache slice: **NO**. The full
chapter's changed cues, speech, contact and owner touch/haptic acceptance remain
pending. Actual normal two-way GPU readiness/adoption/retirement and conservative
contributor eligibility, sustained Shipping performance and fresh phone journey/
lifecycle acceptance remain required. PR27 retains base conflicts; no merge,
release, main import, asset overwrite or phone/data change occurred.

## Ground brightness follow-up

The owner notices brighter terrain than trees/Kit. They share opaque shading and
sRGB base-color sampling; terrain is nonemissive/nonmetallic. Upward normals,
lighter ground albedo, dark bark and the deferred canopy can explain exposed
ground contrast. Kit also has an authored emissive map, so is not a neutral
lighting control. No material or global-light tuning is changed here.

A separate, confirmed rule needs bounded visual attribution: with the rescue
roof open, `activeSkyLight` selects the legacy aperture by X/Z alone, even for
exterior receivers above the room. Its cool radiance is about 1.5 times ordinary
moon radiance before angular/visibility terms. Dawn-zero does not disable this
rule. It cannot explain ground beyond those bounds. Preserve accepted indoor
surface shadows; compare nearby same-material receivers across the boundary
before choosing a correction or material balance.

Next owner route: check a clean start and teaching/Keeper/claim, climb and freshly
raise for Kit, walk both bends/lookout, return/descent and repeat ascent. Check
ground brightness at the rope and farther along the path, stowing silhouette,
equipment/light restoration and speech/captions. Phone acceptance remains open.
