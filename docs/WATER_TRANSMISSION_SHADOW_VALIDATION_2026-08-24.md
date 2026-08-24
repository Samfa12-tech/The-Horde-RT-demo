# General RT Water Lighting Validation - 2026-08-24

## Outcome

The current development candidate removes the scene-specific water-lighting shortcuts that failed in changing torch, moon, camera, and player-shadow conditions. Water remains a real geometry-backed RT dielectric: the primary path solves the air-water-air refraction, traces the opaque receiver, evaluates it with the shared material/direct-light/visibility path, and combines it with one bounded reflected path and Schlick Fresnel.

Three deeper faults were found after the first shared-light candidate:

- `ignoreWater` and transparent visibility queries used `gl_RayFlagsNoneEXT` against opaque BLAS triangles, so candidate rejection was not guaranteed. They now use `gl_RayFlagsNoOpaqueEXT`, explicitly confirm opaque receivers/casters, and skip water/clear glass.
- Secondary hit distance was measured only from the water exit. Reflected and transmitted hits now carry accumulated camera-path distance into fog and distance lighting.
- The transmitted cobble was evaluated as another full primary surface, adding a second glossy scene bounce underneath the water surface. That bounce dominated the blocked direct-light result and filled the player shadow. Transmission now uses `shadeOpaqueSecondary`: the same material BRDF, active local/sky lights, real visibility, fog, and local haze, but no further reflection/refraction/bounce query.

The ordinary non-finale moon is now a directional light. Roof slabs, openings, the player, and other caster geometry determine its visibility through the shared RT query. The waterfall roof breach was extended to include the actual moon-ray footprint at the catchment. This produces the lit cobble patch through physical geometry; it is not a water-only light or baked square.

## Renderer contract

- `activeLocalLight` selects the held torch, coloured passage light, or lich staff for opaque and water paths.
- `activeSkyLight` uses `kMoonDirection` outside the authored moving finale aperture. No skylight-room coordinate target bypasses roof geometry.
- `shadeOpaqueDirect` owns material response plus local and sky visibility.
- Refracted opaque hits use terminal `shadeOpaqueSecondary`; reflected High hits use the same terminal function. It contains no `traceScene`, recursive water call, or indirect bounce.
- Interface highlights use the same active lights and real visibility. The runoff highlight remains a surface BRDF term, not a brightness overlay.
- All direct visibility rays use world-space caster mask `0x35`: world, player body/limbs, shadow/reflection head, and moving roof. Held props remain excluded from their own light estimate.
- `gl_RayFlagsNoOpaqueEXT` forces transparent filtering through candidate handling for both transmission and direct visibility.
- Transmitted and reflected hits preserve accumulated path distance.
- `vkCmdTraceRaysKHR`, `rayQueryEXT`, recursion depth one, one-frame ownership, strict ASTC, TLAS count, material ABI, gameplay authority, lantern timing, and checkpoints remain unchanged.

High water therefore has one bounded transmission scene query and one bounded reflection scene query, plus bounded direct-light visibility queries at their terminal hits and the water interface. Mobile omits the reflected scene query and retains analytic environment reflection. Off skips primary water candidates. No resolution or RT-quality reduction was used.

## Test-driven evidence

The focused source contract was made red before each production correction. It now requires:

- Terminal shared opaque lighting for refracted and reflected hits, with no second glossy bounce.
- No legacy `waterTransmissionSample`, `waterMoonVisibility`, `torchTransport`, `skyTransport`, or `waterSecondarySample` helper.
- `gl_RayFlagsNoOpaqueEXT` candidate filtering for water/glass.
- Accumulated reflected/transmitted path distance.
- A constant complete caster mask with no screen-Y decision.
- Directional moon lighting through the real roof, plus a water-slot footprint that admits that ray.
- No secondary `traceScene` recursion.

The focused `horde_rt_character_render_slot_smoke` passed after the final implementation.

## Final shader identity

- Raygen source SHA-256: `0c5746ef961e42bf951ff591b5e294581c1e97d32f5daf79c0cfe44d0a0efaf5`.
- Embedded raygen include SHA-256: `4001254e8a073067ad9f2cce70a164cbf1cb4612d6b922a0302479b3683866b3`.
- Compiled SPIR-V: 400,020 bytes / 100,005 words / 22,639 instructions / 3,460 branch operations / 24 loops / 1,396 selection merges.
- Compiled SPIR-V SHA-256: `869bc0c88d5208b97803e3c54e2afa88a92a3a95fe2d4fc744a41ec74f30c3d5`.
- `tools/compile-raygen.ps1 -Check`: pass in the fresh Host gate.

Removing the unintended transmitted primary/bounce path reduced the compiled shader from the rejected approximately 25,300-instruction candidate to 22,639 instructions while keeping real transmission, reflection, and visibility queries.

## Deterministic Windows image evidence

Two complete High-water capture sets are preserved at:

- `build/water-real-light-final-20260824-a/`.
- `build/water-real-light-final-20260824-b/`.

Both manifests are complete, identify embedded raygen `4001254e8a073067ad9f2cce70a164cbf1cb4612d6b922a0302479b3683866b3`, and report honest native RT presentation on the NVIDIA GeForce RTX 5050 Laptop GPU. All 13 corresponding PNG hashes match byte-for-byte.

Set A measured a 6.053 ms overall median. `lantern-drop`, `skylight`, and `lich` measured 6.054 / 6.048 / 6.051 ms. Set B measured 6.054 ms overall. These desktop values are below the 15% matched-regression investigation threshold and do not substitute for phone timing.

Image review confirms:

- Torch-lit wall/floor detail remains transmitted through the falling strands at `lantern-drop`.
- The settled-lantern `skylight` frame has a cold cobble patch under the physical water breach, with surrounding roof-occluded ground remaining dark.
- The water/cobble result no longer receives the hidden second wet-cobble bounce.
- The `lich` mist, sword silhouette, and staff lighting remain readable.

On 2026-08-25 the owner launched the exact fresh Host-gate Debug executable and accepted the corrected Windows water/cobble result: "looks right to me." This is owner visual acceptance of the moving desktop candidate, not phone visual acceptance.

A separate close-camera diagnostic used the same final SPIR-V and physical roof geometry. Removing player bits from only the visibility mask changed 33 pool pixels in the expected caster footprint, by up to 30 channel levels, while the primary hand/sword silhouette remained. This proves the pool receives player geometry through the RT light-visibility query. The diagnostic mask and camera change were then reverted; they are not present in production source. Owner motion/feel acceptance remains a separate gate.

## Fresh Host gate

`tools/run-foundation-validation.ps1 -Mode Host -TimeoutSeconds 180` produced `reports/foundation-runs/run-20260824-232553/` with result PASS:

- Shader freshness and the deliberately stale negative fixture passed.
- Fresh Debug and Release builds passed all 13 CTests in each configuration.
- All 13 Windows RT-storage captures completed.
- Android Debug, unsigned Release, and Release lint passed for every configured ABI.
- Packaging/licence and evidence-hash stages passed.
- Validation packages remain explicitly unpublishable.

No release version, signed artifact, itch upload, public page, or deployment changed.

## Exact Android Debug evidence - 2026-08-25

One authorised target was connected: serial `R5GL219SZGK`, raw model code `SM-S948B`, Android 16/API 36, Adreno 840. Current dirty development source rooted at `a8088b67daa532a85b267adcece0a538e0528c41` produced Debug APK SHA-256 `326e024adcb8d5bfb9c5a66fecde9fbb137f19af44cda5c203468fb21dde76d7`. The pulled installed `base.apk` matched byte-for-byte. Embedded raygen SHA-256 was `4001254e8a073067ad9f2cce70a164cbf1cb4612d6b922a0302479b3683866b3`.

`reports/android-showcase-runs/run-20260825-041641/` passed with no warnings or failures:

- strict environment/lich ASTC selection and honest RT swapchain presentation;
- focused 75%/Mobile/Authored medians of 27.770 ms at `lantern-drop`, 18.944 ms at `skylight`, and 29.692 ms at `lich`;
- Android thermal status 0 and Samsung GPU thermal power level 0 for all three rows, with checkpoint battery temperatures of 25.3 / 26.1 / 27.5 C;
- all 13 deterministic replay waypoints, all 13 stable captures, and Home/resume with a new honest presentation marker;
- cleanup force-stopped the Debug package after the run.

Static Mobile capture review confirms that torch-lit transmitted detail remains shaded at `lantern-drop`, the moonlit `skylight` cobble patch is confined by physical roof geometry, the surrounding receiver remains dark, and the lich/mist silhouette remains readable. This is direct screenshot inspection, not owner hands-on phone artistic approval.

Because the corrected `lantern-drop` result was more than 15% slower than the rejected fixed-transport APK, the exact installed APK was repeated without rebuild or reinstall in `run-20260825-042147/`. It measured 27.845 / 19.012 / 30.776 ms at `lantern-drop` / `skylight` / `lich`; thermal status remained 0, GPU power was 0 / 0 / 1, and the run passed. The first two results differ from the complete run by only +0.27% / +0.36%, ruling out a one-off sample. Against rejected run `run-20260824-213143`, the accepted shader is approximately +51.0% at `lantern-drop` and +15.8% at `skylight`.

The increase is an explained, bounded RT cost: a primary Mobile-water pixel traces the refracted receiver, terminal receiver shading evaluates the active local and sky lights through real visibility, and the interface evaluates its own visible highlights. The path contains no transmitted glossy bounce and no recursive water call. The results remain in the descriptive 30-50 FPS and 50-60 FPS bands respectively. Correct lighting was retained as the tech-demo priority; RT quality and render scale were not reduced to manufacture a faster number.

## Evidence boundary

The contracts, deterministic Windows images, exact Android artifact/run evidence, and inspected Mobile captures prove that the candidate uses real geometry/light visibility and no longer fills a blocked direct-light result with a second hidden glossy bounce. The owner accepted the moving Windows result. The automated phone evidence does not claim owner hands-on phone visual judgment or another device/driver.

Audio/haptic manual revalidation required: **NO** - the change affects shader lighting and static roof geometry only. Listener/source state, spatialisation, playback, event timing, controls, haptic routing, and damage/death feedback are unchanged.
