# The Horde — experimental hybrid lab

A native Vulkan **offscreen comparison demo**, separate from the game and its
published 1.7 builds. This is an initial mechanism test, not a complete hybrid
backend or a playable campaign build. Full hardware RT remains an option.

## Try the prepared artifacts

- Windows: extract `Horde-Hybrid-Lab-Windows.zip`, then double-click
  `Run comparison.cmd` (640×360) or `Run comparison 720p.cmd` (1280×720).
  The executable renders both modes, saves every measured frame duration and
  opens a local HTML report with full-resolution A/B and difference captures.
- Android: `Horde-Hybrid-Lab-arm64-debug.apk` is a separate, normally debug-signed
  app, `com.samfa12.hordehybridlab`. Open **Horde Hybrid Lab**, choose the same
  resolution as Windows, and tap **Run full RT / hybrid comparison**. Use **Save
  report ZIP** to save the local HTML, captures, JSON and CSV through Android's
  document picker. No network permission, upload or telemetry service is used.
  Installation, physical presentation and report export have **not** been
  tested on a phone: no device was attached. The APK build is not a device pass.
- Existing game packages, saves, settings, version assignments and renderer
  defaults are untouched. Do not replace them with this lab.

The Windows executable needs Vulkan 1.2 hardware RT; Android needs arm64,
Android 10+ and the required Vulkan extensions/features. The OS minimum does
not establish RT support. The lab requires acceleration structures, ray queries,
buffer device address, a graphics+compute queue and valid timestamp bits. It
prefers the full RT pipeline baseline; if that feature is absent it explicitly
labels and uses a full hardware ray-query compute baseline. A pipeline creation
failure is reported as a failure, not silently treated as successful rendering.
Devices lacking the required features receive diagnostics; no software BVH or
raster-only fallback exists. Feature support is not independent proof of a
vendor's dedicated hardware implementation.

## What is compared

The shared geometry adapter consumes the existing Horde checkout's
`TombStoneBevels.h`, `ShowcaseOverheadGeometry.h` and `ShowcaseRoute.h`, including
the authored opening room coordinates, six roof patches, stone bevels and portal
pieces. It adds an explicitly labelled metal mirror and emissive markers as lab
fixtures. There are **684 triangles**. This deliberately small geometry slice
does not represent the full production asset, actor or material workload.

Both modes use identical buffers/TLAS, camera/projection/clipping, resolution,
procedural masonry, light phase, hard-shadow rays, one bounded mirror bounce,
exposure, gamma and ambient approximation. No temporal history, stochastic
denoising, dynamic resolution, mip differences or quality switches confound the
comparison. One light moves through the same phases; mesh and camera poses are
fixed. The screenshots show phase zero. Animation of the game's characters and
dynamic BLAS/TLAS updates are outside this first test.

- **Full RT:** `vkCmdTraceRaysKHR`, with a primary `traceRayEXT` hit and shared
  ray-query shadow/reflection shading. On devices without RT pipelines, a compute
  shader traces the primary hit using a hardware ray query, then uses the same
  shared shading.
- **Hybrid:** ordinary forward raster primary visibility, a depth attachment and
  early fragment tests; the fragment shader casts real BLAS/TLAS-backed hardware
  ray queries for shadow and mirror lighting. No full-screen G-buffer is stored.

The production Horde pipeline currently uses **ray queries from raygen for
primary visibility** (`rt_frame.glsl` / `traceScene`), with much richer shared
shading and filtering. This lab's pipeline baseline is a simpler reference, not
the shipping binary or an exact reproduction of its primary traversal strategy.
No measured percentage here predicts shipping-engine performance.

Visual compromises relative to the game: procedural stone replaces production
PBR textures, hard point-light shadows replace richer effects, the ambient term
is a shared approximation, and water, dielectric glass, volumetric fire/mist,
foliage alpha, animated characters, viewmodel/body masks and gameplay are absent.
Both paths make the same compromises. Raster edge coverage/precision can still
differ from ray intersections; the report retains these differences rather than
filtering them away.

## Measurements and evidence

Each scene executes eight blocks in RT/hybrid/hybrid/RT order, repeated twice.
Each block has eight unrecorded warm-up frames and the same measured light-phase
sequence. Prepared desktop evidence uses 96 frames/block (384 per mode/scene);
the quick interactive default uses 24. There is one frame in flight. No pacing,
VSync or swapchain is involved. Short runs are not thermal or battery tests.

GPU timestamps span the complete offscreen render operation, including hybrid
depth clearing and raster overdraw. Separate setup timestamps record static
BLAS/TLAS build cost. CPU command-record and submit/fence-wait timings are in
`frames.csv`; their sum is the CPU cycle. CPU cycle overlaps GPU work. Readback,
report creation, Vulkan setup and pipeline compilation are excluded from render
samples. Setup CPU time is recorded separately. Fused fragment shading cannot be
split into visibility and lighting without changing the workload; no such split
is claimed. GPU `1000 / ms` is an isolated render ceiling, **not gameplay FPS**.

Results include device/vendor/driver identifiers, exact native source/shader
identity, allocation bytes, median/p95, every sample and image errors. The source
identity includes shared geometry-header hashes. No fixed pixel threshold is
treated as owner visual acceptance. Per-frame phase pairing and report statistics
can be checked with `verify_results.py`.

For phone testing: keep power state and orientation fixed, start from comparable
temperature, close heavy background applications, select the same resolution,
run at least three comparisons, and export each ZIP. Inspect the portal shadow,
mirror, thin edges and motion. Record model/OS/driver, temperature/thermal state
and whether the app stayed foregrounded. Interruptions and throttled runs should
be retained and identified. Different devices should be compared within their own
RT/hybrid pair; do not compare raw phone FPS with the laptop as an equivalence
claim. Sustained phone performance requires a longer full-scene integration.

## Rebuild without copying the game

Use the existing checkout as read-only source for geometry headers. Shader
generation requires Python and the installed Vulkan SDK; no provider credentials,
downloads, new worktree or new game asset import is needed.

```powershell
python .\compile_shaders.py
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  "-DHORDE_SOURCE=C:/Users/sam_s/Documents/the Horde RT Demo"
cmake --build build --config Release --parallel 2
.\build\Release\horde_hybrid_lab.exe evidence\new-run 1280 720 96
.\build\Release\horde_hybrid_lab.exe evidence\validation 640 360 4 --validation
.\build\Release\horde_hybrid_lab.exe evidence\query-validation 640 360 4 --validation --query-baseline
python .\verify_results.py evidence\new-run
```

The host has CMake under Visual Studio BuildTools rather than on PATH; its exact
path and executed commands are in `RESEARCH_AND_RESULTS.md`.

Android uses the existing cached Gradle wrapper and SDK, with no changes to game
signing configuration:

```powershell
$env:JAVA_HOME='C:\Program Files\Android\Android Studio\jbr'
$env:ANDROID_HOME='C:\Users\sam_s\AppData\Local\Android\Sdk'
& 'C:\Users\sam_s\Documents\the Horde RT Demo\android\gradlew.bat' `
  -p .\android --offline --no-daemon assembleDebug lintDebug `
  '-PhordeSource=C:/Users/sam_s/Documents/the Horde RT Demo' --console=plain
```

The generated `Shaders.h` is checked in for offline builds; regenerating it
compiles and runs `spirv-val` on all six modules. `shaders.json` records their
hashes. Existing evidence directories are never overwritten when `results.json`
is present. Build outputs and raw captures remain outside Git.

## Research and next integration step

See `RESEARCH_AND_RESULTS.md` for primary references, alternatives, repeat laptop
results and limitations. This prototype does not reopen the previously rejected
primary-opacity or lookup-guard flag experiments. A useful next engine slice
would adapt opaque `RtStaticMeshSlot` primitives/instances to forward draws while
retaining the shared material/light code and real secondary queries. It needs
correct primary/body/viewmodel masks, transparent ordering, surface orientation
and BGRA handling, resource lifecycle/resize ownership, and real swapchain
presentation/benchmark identity. Water/glass and animated geometry require
explicit coverage before any whole-game or phone-performance conclusion.
