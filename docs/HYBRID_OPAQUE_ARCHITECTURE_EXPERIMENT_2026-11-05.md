# Horde: complete opaque architecture, raster primary + RT shadows

5 November 2026. Experimental development option; no release, merge, publication or default change.

Sam's intended split is implemented for a bounded actual-Horde architectural fixture: raster finds visible surfaces, then hardware ray queries supply selected direct-light visibility and shadows. The candidate does **not** trace a camera ray for the remaining scene underneath a raster overlay. It is slower on the tested RTX 5050 laptop; phone performance is unmeasured.

## What the work does

Loading meshes, preparing a ray intersection index, and rendering an image are different work. A BLAS/TLAS acceleration structure is an index of geometry used to answer ray intersection questions, including shadows from objects outside the camera view. Keeping it does not mean rendering the camera image twice. Animated shadow casters would still need appropriate AS updates in a live hybrid game. [Khronos AS specification](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_acceleration_structure.html).

The earlier owner21-only experiment retained ordinary scene AS updates, added a second TLAS each frame, rasterised only static assets, traced a remaining-scene primary query at every pixel, and kept all secondary shading. Its measured regression remains recorded in `HYBRID_REGRESSION_ANALYSIS_2026-11-05.md`; it is preserved, not relabelled as this result.

The new forward path rasterises every surface in the **declared complete static opaque architectural fixture**: procedural owner0 plus actual StaticPbr owner21, using the production source mesh buffers and material decoding. This is 2,136 retained world triangles plus 751,868 asset triangles, 754,004 triangles in 12 draws. Of 2,254 source world triangles, 118 flame/glass/water triangles are excluded. All owner21 primitives must be opaque with baked identity geometry transforms; incompatible assets fail admission.

Fragments call the shared production direct-light/shadow routines with genuine private-TLAS ray queries. No remaining-primary dispatch, full-screen G-buffer or deferred shading resolve is recorded in forward mode. It uses an RGBA8 color target and D32 depth, then copies color into the existing output image. The extra color copy is included in timing. Optimized SPIR-V has two shadow/lighting query sites in the fragment module versus three including camera visibility in each reference module; a site count is not a runtime rays-per-pixel count.

A filtered world BLAS and a two-instance private scene TLAS are built once. It borrows the actual static-asset BLAS. Source primitive remapping preserves world materials. All steady sampled frames have zero skin work, dynamic upload bytes, BLAS refit recording and TLAS update recording in **both** comparison modes, verified in completed-frame evidence. Original game initialization still allocates/builds other resources. No startup or memory saving is demonstrated. This static-fixture policy is not suitable for moving actors or changing geometry.

## Fair reference and quality scope

The new reference is labelled **RT primary + RT shadows**. It traces primary visibility over precisely the same private scene and uses the same selected direct shading, lights, camera, resolution, explicit texture LOD0, material decoding, exposure and tone mapping. It is **not the complete shipping Full RT shader**. Normal production Full RT and the earlier comparison binaries remain separately preserved.

Both new modes omit dynamic actors, held props, gameplay objects outside the declared owners, water/glass, mist/dust and volumetric flames, indirect bounces and reflections. Mirror geometry receives direct opaque/specular shading, without mirror transport. Some legacy quality fields still describe uploaded game settings even when that effect is absent. Legacy telemetry fields `wholeRtGpuMs`, `rtDispatchRecorded`, and `honestlyPresentedRtFrame` are reused for the RT-lit command-buffer protocol; they do not certify ray-traced primary visibility. Capture metadata explicitly records actualCommandPath, selected shader SHA, scope and `fullShippingReference:false`. Actor/held-geometry certification is explicitly absent, not fabricated.

The game shell still supplies navigation, settings and diagnostics; this is an architectural inspection profile, not a playable hybrid campaign. Dynamic gameplay and interactions are not acceptance-tested. Room lights are retained from first submission while the inspection camera can move. Changing world geometry requires scene recreation. This limitation must remain visible during testing.

The indoor stills look very close on inspection. RGB mean absolute differences on a 0–255 scale are 0.0146 portal, 0.0113 niche and 0.2792 forest; changed-pixel proportions are 2.95%, 2.35%, 6.76%. Forest foliage/edge coverage differs visibly in detail. These are descriptive image differences, not a perceptual-equivalence pass; stills do not cover motion, disocclusion, aliasing or material variety. Fixed LOD0 in both modes is deliberately matched and can shimmer.

## Measured laptop result

NVIDIA GeForce RTX 5050 Laptop GPU, Vulkan API 1.4.341, driver 610.47. One exact EXE, 960×540 native internal/output resolution, renderScale1, frozen camera/animation/lighting, Shipping/Mobile selected-effects shaders. Per view the order is RT / raster / raster / RT. Every run has 180 completed, presented frames with valid submission-owned GPU timestamps; exclude the first60 and summarize120. Table values are means of the two per-run medians, not independent pooled trials. Compiler/build jobs were complete before this cohort.

| View | RT primary GPU ms | Raster primary GPU ms | Candidate change |
| --- | ---: | ---: | ---: |
| Stone portal | 0.7504 | 1.2384 | +65.0% |
| Niche | 0.7211 | 1.0279 | +42.5% |
| Forest lookout | 0.6378 | 1.0964 | +71.9% |

CPU whole-cycle medians average 5.9416–5.9505 ms, approximately168 diagnostic cycles/s in both modes under presentation pacing. These are **not sustained game FPS** or actual Android presentation rates. Reciprocating the sub-millisecond GPU times would be especially misleading. Per-run p95, command-record CPU time and exact settings are in local `comparison.json`. Clock, power and thermal controls were not captured, runs are short and static, and sub-millisecond timing limits generalisation. The direction is consistent across the six candidate runs. There is no measured speedup.

Source-proven extra work includes 2,262,012 nonindexed vertex invocations/references over 754,004 triangles, no frustum/backface/occlusion optimisation, depth/color targets and an extra RGBA copy. Which dominates the measured regression is **not established**: timestamps currently bracket the whole command buffer rather than separating vertex/raster, fragment/shadow, copy and AS work. The selected-light reference is already inexpensive on this desktop GPU. These observations do not establish that mobile hybrid rendering is inherently worse or better.

Next bounded experiment: first profile separate forward draw and copy GPU stages and collect a real phone matched fixed-view cohort with temperature/power context; then test conventional indexed vertex/material interpolation and conservative CPU frustum rejection while retaining offscreen geometry in the shadow AS. Do not simply remove shadow casters to reduce draw work. Avoid switching to a large deferred G-buffer without evidence: it adds bandwidth/attachments, particularly relevant to phones. Khronos demonstrates the selected forward vertex/fragment plus ray-query-shadow design directly in its [official ray queries sample](https://docs.vulkan.org/samples/latest/samples/extensions/ray_queries/README.html). Raster-only loses Sam's requested genuine RT lighting; keeping full primary RT is the preserved reference/alternative. No further gain is predicted here.

## Validation and artifacts

- Windows Release build succeeds. All four generated shaders compile with glslang, optimize with spirv-opt and pass spirv-val for Vulkan1.2; no Geometry capability workaround.
- Five final regular Khronos-validation captures: pipeline reference/forward, compute reference/forward, portrait forward. All180 frames presented per run; **both stdout and stderr** scanned with zero VUID/validation warnings/errors. Portrait internal/output540×960. This is not explicit synchronization-validation or fault/lifecycle coverage.
- Explicit retained-static command protocol test accepts the static sequence and normal dynamic sequence, rejects missing dynamic AS updates and mixed sequences, and accepts only the correct order among six static permutations. No fake TLAS update marker.
- Experiment-disabled syntax checks pass for both changed production translation units. Ordinary production output and command contract remain guarded; no full ordinary build was repeated. Platform source checks passed43 benchmark and516 graphics assertions; six Android build-policy checks pass.
- Android offline `assembleDebug` and `lintDebug` succeed:0 errors,96 existing lint warnings. Separate app `com.samfa12.hordelanternrt.debug.hybridroom`, label `Horde Raster + RT Shadows Lab`, arm64,53 genuine GLBs, no LFS pointer placeholders. Existing Android debug signer verifies (v2); no signing/security settings changed. `adb devices -l` lists no device. No APK installation; native Android startup, rendering, input, mode switching, lifecycle and performance remain untested.

Local package: `C:\Users\sam_s\Documents\Codex\2026-11-05\task\native-hybrid\opaque-room\deliverables`. Start with `comparison.html` and `README.md`.

Windows `HordeRasterRTShadows.exe` SHA256 `9c82d8423506f49ec9f4df8e7d5aa4dae2f59a2f143eee0de7cac6bce16c7043`.
Android `Horde-Raster-RT-Shadows-Lab.apk` SHA256 `271c789fb5bd74f19eee38a6de9969674d70c10b449883a64ca31f4468dcf747`.
`matched-and-validation-evidence.zip` retains all final17 successful capture directories (12 timed+5 validation), shader manifest, APK checks and source checkpoint accompany the package. Earlier failed development captures remain outside that sealed successful cohort and are not used for measurements. All28 previous artifact hashes still match. Existing canonical checkout/build tree reused; no new worktree, deletion or deployment.

## Reproduction

From PowerShell, substitute the canonical paths below. Shader generator path has no required third-party Python package:

```powershell
$repo='C:\Users\sam_s\Documents\the Horde RT Demo'
$task='C:\Users\sam_s\Documents\Codex\2026-11-05\task'
$room="$task\native-hybrid\opaque-room"
$cmake='C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$env:VULKAN_SDK='C:\VulkanSDK\1.4.350.0'
python "$repo\tools\compile-hybrid-opaque-room.py" $repo "$room\generated"
& $cmake -S $repo -B "$task\native-hybrid\build" -DHORDE_RT_HYBRID_STATIC_PRIMARY=ON -DHORDE_RT_HYBRID_OPAQUE_ROOM=ON "-DHORDE_RT_HYBRID_STATIC_SHADER_DIR=$room\generated"
& $cmake --build "$task\native-hybrid\build" --config Release --target horde_rt_diagnostic_window --parallel 2
& "$task\native-hybrid\run_capture.ps1" -Executable "$room\deliverables\HordeRasterRTShadows.exe" -RepoRoot $repo -Output "$room\evidence\NEW-UNUSED-PATH" -Checkpoint chapter-niche-arch -Hybrid -Validation
```

The reused CMake cache also has ChapterMeasurementON, BuildTestingOFF, Shipping/Mobile private overrides and the existing WebView2 SDK; cache/source/build manifest in the checkpoint documents the exact settings. Omit `-Hybrid` for matched reference, add `-Query` for compute reference support, add `-Portrait` for portrait. Choose a fresh evidence path. Rerun `analyse_room.py` only for the defined final matched-* cohort, not mixed/broken development runs.

```powershell
$env:JAVA_HOME='C:\Program Files\Android\Android Studio\jbr'
$env:ANDROID_HOME='C:\Users\sam_s\AppData\Local\Android\Sdk'
Set-Location "$repo\android"
.\gradlew.bat --offline --no-daemon :app:assembleDebug :app:lintDebug -PhordeHybridStaticPrimary=true -PhordeHybridOpaqueRoom=true "-PhordeHybridStaticShaderDir=$room\generated" -PhordeRtInstrumentationOverride=Shipping -PhordeRtDielectricQualityOverride=Mobile --console=plain
```

No device installation command is included as an action taken. Future phone acceptance needs equal initial state/settings, interleaved controls, completed present ownership and heat context; do not compare the reduced-effects candidate against full shipping shading as a primary-visibility speedup.
