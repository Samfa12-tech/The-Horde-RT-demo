# Low-resolution reconstruction readiness - 22 October 2026

Owner observed20% and requires acceptable image quality before choosing it.
It remains diagnostic only. No default change or substantial upscaler
implementation is admitted by the [phone timing checkpoint](../evidence/2026-10-22-outdoor-scale/README.md).

## Current source

`shaders/raytracing/include/rt_frame.glsl` uses one centred primary ray per
internal pixel; there is no jitter/history/TAA resolve or exported motion-vector
buffer. Tone mapping and sRGB conversion already occur in raygen's RGBA8 output.
`PresentableTinyRtScene.cpp` enlarges that image with `VK_FILTER_LINEAR` blit.
Primary hit distance exists locally, but reconstruction depth is not exported.

Material PBR fetches in `rt_hit_decode.glsl` use implicit `texture()` without
ray footprints/gradients. Non-fragment implicit sampling selects the base mip
under the [Khronos GLSL rules](https://docs.vulkan.org/glsl/latest/chapters/builtinfunctions.html).
The material sampler has linear texel filtering, nearest mip selection and no
anisotropy. Uploaded mip chains therefore do not establish useful distance
filtering. Environment roughness sampling already uses explicit LOD/trilinear;
do not describe that existing capability as absent. Filtering alone cannot
restore missing geometry edges or detail.

## Options to evaluate, not adopted integrations

| Option | Verified support / boundary |
| --- | --- |
| Snapdragon GSR2 | [Official Android Vulkan source/sample](https://github.com/SnapdragonGameStudios/snapdragon-gsr/tree/main/sgsr/v2) and [Qualcomm overview](https://www.qualcomm.com/developer/blog/2024/10/introducing-snapdragon-game-super-resolution-2). Worth evaluating on this Adreno device; variants include two-pass compute and three-pass compute. Requires colour, depth, velocity, jitter, reprojection/exposure metadata and history/reset handling. No Horde performance or image acceptance exists. |
| Arm ASR | [Generic Vulkan integration](https://github.com/arm/accuracy-super-resolution-generic-library) supports Android Vulkan. Requires colour/depth/motion, jitter and history; reactive masks help changing blended effects. Verify exact-device FP16 capability for explicit FP16 paths and measure Adreno cost. |
| FSR2/3 | [FSR2 Vulkan](https://gpuopen.com/fidelityfx-superresolution-2/) and [FSR3.1 Vulkan release](https://gpuopen.com/learn/amd_fsr_3_1_release/) exist; this does not certify Android/Adreno. Pin a known Vulkan-capable version: the [current SDK](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK#known-issues) lists Vulkan sample limitations. |

These temporal options do not require AI hardware. No DLSS or phone NPU support
is assumed, and no paid software/generation is authorized. Vendor sample
timings are not Horde evidence.20% means a roughly5x enlargement per dimension;
an integration must prove that exact input/output pair rather than borrow a
different vendor quality preset's result.

Before implementation review, establish a measured budget for correctly
projected depth and camera/object/skinned motion, including arms and Kit;
colour space/exposure and history ownership; resets on rotate/resize/load/
teleport/pause/failed presentation; reactive lighting from fire/water/foliage;
ghosting, disocclusion, thin geometry and newly revealed surfaces. Measure all
added GPU passes, CPU work, peak/live memory, bandwidth and prolonged thermal
behaviour at fixed actual dimensions. Reconstruction cannot remove fixed
skinning or AS costs. Preserve RT-produced imagery and display-resolution UI.

A bounded texture-footprint/mip audit is also useful before a temporal system.
Merely enabling trilinear or anisotropy while rays still sample mip0 is incomplete.

## Verified Briarhold touch reference

Read-only source review of `C:/Users/sam_s/Documents/Briarhold` found circular
action backgrounds in `styles/game.css:499`: radial fill54% centre to80% edge,
45% brass border, solid icons. Movement background uses20–26% opacity; the knob
follows deflection and resets on release/cancel. Fire/Use pictograms occupy58%
of the button, with separate accessible labels. Active Aim/auto-target states
use a brighter ring/halo. No generic authored touch pressed tint/scale was found.
An existing August `design-qa/android-device-combat.png` demonstrates the
silhouette, but is not a fresh runtime capture; Git metadata access was denied,
so branch provenance is unverified. No Briarhold writes or processes occurred.

For Horde's deferred redesign, use translucent fills with solid readable icons,
independent comfortable Android hit regions and explicit pressed feedback.
Do not fade the whole control or copy tiny7–8px labels. Circle/opacity refinements
are saved owner direction, **not urgent** under the later19:35 instruction.
