# Hybrid rendering investigation — 4 November 2026

**Outcome:** a runnable Windows/Android native Vulkan lab is built. It compares
forward raster visibility plus real hardware ray queries with a full hardware RT
reference in the same bounded tomb scene. Laptop results are mixed. Android
packaging succeeds; phone runtime/performance remains unmeasured. This experiment
does not establish a production hybrid renderer or a phone speed improvement.

## Research and choice

Khronos explicitly supports combining raster primary visibility with secondary
ray queries, available in graphics, compute and RT shader stages. Its official
[ray-query sample](https://docs.vulkan.org/samples/latest/samples/extensions/ray_queries/README.html)
uses ordinary vertex/fragment rendering and fragment ray-query shadows. The
[extension specification](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_ray_query.html)
defines the acceleration-structure dependency and inline traversal results.
This directly supports a bounded experiment in Horde's Vulkan engine without
adopting another engine or a browser renderer.

[NVIDIA's RT guidance](https://developer.nvidia.com/blog/?p=50632) recommends
first-hit shadow termination, opaque geometry where correct, limited live state,
bounded payloads and profiling the real workload. The lab follows first-hit
termination for visibility, opaque triangles, a single static BLAS/TLAS and one
nonrecursive reflection. Async AS builds, mesh LOD and new culling policies are
separate experiments, since changing them now would confound visibility costs.

[Arm's mobile MegaLights investigation](https://developer.arm.com/community/arm-community-blogs/b/mobile-graphics-and-gaming-blog/posts/lighting-at-scale-bringing-hundreds-of-dynamic-lights-to-mobile-with-unreal-megalights)
illustrates another direction: budget the number of evaluated shadow lights
using stochastic selection and temporal feedback. It concerns Unreal and Mali-G1,
not Horde or this laptop/phone. Its performance claims are not transferred here.
The relevant inference is that shadow/light cost can remain dominant even after
primary visibility is rasterised. This first lab keeps three deterministic lights
and identical ray budgets; it does not implement MegaLights or ReSTIR.

| Option | Practical fit and decision |
|---|---|
| Forward raster visibility + ray-query lighting | Chosen for this first opaque slice. Reuses hardware AS concepts and needs only colour and depth targets, without a full-screen material G-buffer. It can duplicate shading through raster overdraw, so a speedup is a hypothesis. |
| Deferred raster G-buffer + ray-query resolve | Plausible when lighting/material reuse warrants it, but adds attachments, passes and bandwidth. The existing experimental `StagedPrimaryPass` already demonstrates lifecycle/timing infrastructure, but it stages **RT primary** records, not raster primary. Its 128-byte record/page contract is not a ready hybrid backend or a mobile win. No old trial was rerun. |
| Raster + shadow maps, baked light or screen-space reflections | Possible for a broader low-end renderer, but would change the lighting/visibility mechanism and visual contract. Not selected for this equal-effects comparison. No such fallback was added to the game. |
| Fewer RT lights/rays, temporal denoising, reservoir lighting | A separate quality/stability/temporal-history investigation, potentially useful later. Not bundled into the visibility experiment. |
| Another engine / browser WebGPU demo | Does not measure Horde's native Vulkan path or hardware feature requirements. Not used. |

The forward choice is an engineering inference from the architecture and these
references, not a measured claim that forward is universally better on phones.

## Current engine and scope boundaries

Inspected canonical checkout:
`C:\Users\sam_s\Documents\the Horde RT Demo`, initially clean for tracked files
on `main` at `b48f1ac368c5afe8e2bb4917c6a8c835ae284d08`.
The existing 1.7 worktree remains at
`.worktrees/horde-1.7-wp2-vertical-proof`, branch
`codex/horde-1.7-published-closeout-20261103`, head
`6a593254b9675d454608ae3b6cdd3d94958dbb84`. Its untracked asset work is untouched.
The task inventory showed `Continue Horde 1.7 development` idle. No new worktree
or game checkout was created. A small standalone source lab was developed in the
task workspace and checkpointed under the canonical repository's experimental
tools directory on a separate development branch.

Read root `AGENTS.md`, engine contracts, validation guide and the current
performance ledger. The owner's explicit request authorises a labelled hybrid
experiment in addition to full RT; it does not replace the game's RT contract or
reopen rejected primary-opacity/lookup-guard flag trials. No main merge, release,
publication, deployment, device install, signing-setting change or game version
change was performed.

Relevant current code:

- `PresentableTinyRtScene.cpp` owns scene buffers, AS/dispatch/presentation and
  resource lifetime; platform paths share its snapshots and shader metadata.
- `shaders/raytracing/include/rt_frame.glsl` calls `traceScene` for primary
  visibility and shared lighting/optics. Pipeline raygen uses real ray queries;
  `RayQueryCompute` is another real hardware RT launcher.
- `experimental/StagedPrimaryPass.*` is opt-in RT-primary staging and does not
  implement raster draw lists, depth or a fragment graphics pipeline.
- Native static geometry already has reusable vertex/normal metadata and
  authored roof/bevel geometry. The lab consumes the existing geometry headers,
  while leaving the production shaders and runtime structures untouched.

The lab's **684 triangles**, procedural material and three light sources are a
mechanism/appearance proof. They omit production textures, water, glass, alpha
foliage, volumetrics, actors, viewmodel/body masks and dynamic AS work. The RT
pipeline reference uses `traceRayEXT` primary hits, whereas the production
pipeline uses primary ray queries from raygen. Neither its CPU/GPU cost nor the
lab's extremely high isolated frame-rate ceiling is shipping gameplay evidence.

## Measured laptop results

Device: **NVIDIA GeForce RTX 5050 Laptop GPU**; Vulkan reported API version
`4211029`, vendor `4318`, device `11672`, driver version raw `2559295488`.
All final native runs share build identity
`a5beedd2c59a4983d81db6ea4fa60b1c65097d0acbdeebfc7a5bb3a492e1980d`.

Eight ABBA/ABBA blocks per scene, eight warm-up frames/block, 96 measured
frames/block, 384 samples per mode/scene. Four performance sessions were run
serially in 360p/720p/720p/360p order. No validation layer was enabled for these
performance sessions. This is repeatability in short offscreen runs on one
laptop; clocks, power and temperature were not independently instrumented.

| Resolution / view | Full RT median GPU ms (runs 1 / 2) | Hybrid median GPU ms (runs 1 / 2) | Hybrid render-time change |
|---|---:|---:|---:|
| 640×360 opening | 0.194560 / 0.194624 | 0.182624 / 0.182208 | −6.1% / −6.4% |
| 640×360 portal | 0.158144 / 0.158400 | 0.279744 / 0.279840 | +76.9% / +76.7% |
| 640×360 mirror | 0.199040 / 0.199168 | 0.197440 / 0.197504 | −0.8% / −0.8% |
| 1280×720 opening | 0.722816 / 0.721344 | 0.603840 / 0.603232 | −16.5% / −16.4% |
| 1280×720 portal | 0.584320 / 0.584384 | 0.996992 / 0.997504 | +70.6% / +70.7% |
| 1280×720 mirror | 0.743552 / 0.743968 | 0.664704 / 0.664320 | −10.6% / −10.7% |

Negative means less GPU time. Raw per-frame GPU/CPU-record/submit-wait samples,
p95s and isolated GPU ceilings are retained with every run. For example, 720p
run 1's full/hybrid render ceilings are approximately 1383/1656 opening,
1711/1003 portal and 1345/1504 mirror frames/s. **These are mathematical ceilings
for a tiny offscreen operation, not measured application or game FPS.** No WSI,
UI, simulation, AS animation, VSync or sustained thermal cost is included.

The portal regression is consistent with extra raster coverage/overdraw being
costly in this draw order, but that is an **unproven explanation**: no pipeline
statistics or overdraw counter capture was taken. Do not present it as diagnosed
or hide it by tuning the scene. It demonstrates why a whole-scene phone test is
needed before choosing a renderer.

Live lab-owned Vulkan allocation is 3,036,160 bytes at 360p and 11,699,200 at
720p; it includes colour, depth, readback, geometry, AS, scratch and SBT, excludes
driver overhead, and is not the complete process footprint. Setup-only static
BLAS build GPU time in the four performance runs was 0.211–0.234 ms; TLAS
0.0249–0.0313 ms. They are not per-frame animated-AS timings.

## Visual and automated validation

All six shaders compiled and passed `spirv-val --target-env vulkan1.2`.
Windows Release compilation passed. Both pipeline and explicitly selected
ray-query-compute baselines ran with `VK_LAYER_KHRONOS_validation`: **zero errors,
zero warnings**. The explicit query run verifies the alternate baseline code path
on this GPU, not an Android driver. Four performance runs contain 9,216 measured
frames; two validation runs contain another 192. The evidence checker verified
phase/mode pairing, identities, percentile reconstruction, CPU timing sums and
all 54 final capture files.

Side-by-side full/hybrid captures were visually inspected: warm/cool lighting,
portal shadows and the reflected room remain coherent in this simplified slice.
At 720p, RGB mean absolute errors are 0.0027–0.0057 on the 0–255 scale, with
0.0064–0.0129% of pixels exceeding 8 in any channel. At 360p the corresponding
fraction is 0.0069–0.0178%. Some maximum edge errors are much larger; all pixels
and amplified differences remain in the report. This is no claim of exact pixel
parity or owner acceptance of the game's production appearance.

Android `assembleDebug` and `lintDebug` passed using the installed NDK
28.2.13676358 and cached AGP 8.7.2/Gradle 8.11.1. The first Android link attempt
exposed the Android API-29 loader's missing statically exported buffer-address
entry; loading that Vulkan 1.2 device function through `vkGetDeviceProcAddr`
fixed the issue. Ordinary debug APK signature verification passed. Lint warnings
cover the deliberately bounded target/arm64 test scope, backup/icon metadata and
English prototype strings; they do not establish runtime readiness. The SDK
tools also emit an XML schema-version warning. No toolchain or signing settings
were changed to suppress these.

`adb devices -l` returned an empty device list. No Android installation, native
GPU execution, UI/export operation, lifecycle interaction, temperature or phone
performance was validated. Report export is implemented through an explicit
owner-operated Android document picker and needs a device check.

## Exact commands and evidence locations

Task source/artifacts:
`C:\Users\sam_s\Documents\Codex\2026-11-05\task\hybrid-lab`.

```powershell
python hybrid-lab\compile_shaders.py
$cmake='C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
& $cmake -S hybrid-lab -B hybrid-lab\build -G 'Visual Studio 17 2022' -A x64 `
  '-DHORDE_SOURCE=C:/Users/sam_s/Documents/the Horde RT Demo'
& $cmake --build hybrid-lab\build --config Release --parallel 2
$labExe=(Resolve-Path hybrid-lab\build\Release\horde_hybrid_lab.exe).Path
& $labExe hybrid-lab\evidence\final-validation-pipeline 640 360 4 --validation
& $labExe hybrid-lab\evidence\final-validation-query 640 360 4 --validation --query-baseline
& $labExe hybrid-lab\evidence\performance-640-run1 640 360 96
& $labExe hybrid-lab\evidence\performance-1280-run1 1280 720 96
& $labExe hybrid-lab\evidence\performance-1280-run2 1280 720 96
& $labExe hybrid-lab\evidence\performance-640-run2 640 360 96
Get-ChildItem hybrid-lab\evidence -Directory |
  Where-Object Name -NotMatch '^windows-validation' |
  ForEach-Object { python hybrid-lab\verify_results.py $_.FullName }
```

An earlier `windows-validation-640x360` run predates the final stage/source
metadata. It is preserved as exploratory evidence and is excluded from all final
tables. Final report folders have `results.json`, `frames.csv`, `index.html`,
`build-identity.txt`, six unfiltered A/B BMPs and three amplified difference BMPs.
`deliverables/manifest.json` records artifact hashes; published packages were not
read or overwritten by build/package scripts.

## Remaining work and decision

1. Run the separate APK on Sam's phone at both resolutions; inspect UI/export and
   actual hardware presentation and retain multiple paired reports with thermal
   context. A lack of physical-device evidence is the present validation blocker.
2. If that result justifies further work, build an **opt-in full-engine opaque
   adapter** sharing production materials/light selection, static primitive data,
   dynamic actor transforms and correct primary masks, then handle transparent
   water/glass and effects explicitly. Keep original RT selectable and default.
3. Compare the actual engine paths using fixed captures and the existing benchmark
   identity/CPU/GPU/AS/presentation evidence, at identical scale/settings. Inspect
   temporal aliasing, thin geometry, shadows and reflections and perform a
   sustained thermal run. Do not infer the game's performance from this lab.
4. No renderer adoption, default change, merge or release is recommended from
   these laptop microbenchmarks alone.
