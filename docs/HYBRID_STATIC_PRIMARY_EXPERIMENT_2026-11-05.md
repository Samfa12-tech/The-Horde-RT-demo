# Actual Horde assets: partial hybrid experiment

This document preserves the first delivery snapshot, including its then-denied
commit status. Sam subsequently explicitly authorised the experiment commit.
See [regression analysis and follow-up](HYBRID_REGRESSION_ANALYSIS_2026-11-05.md)
for the measured attribution, rejected camera trial and current source closeout.
The sealed first-delivery artifacts remain unchanged for exact-build review.

5 November 2026. This is a playable native Horde experiment using actual GLB
assets, production materials/textures and existing lighting/optics. It supersedes
the synthetic lab as the representative test option. It is a **partial hybrid**,
not a complete forward renderer or evidence of a phone speedup.

Development branch: codex/horde-hybrid-lab-20261104, based on canonical
b48f1ac368c5afe8e2bb4917c6a8c835ae284d08, retaining the earlier synthetic lab
checkpoint. Published builds and the separate 1.7 worktree remain unchanged.
No merge, push, publication, device installation, signing changes or new worktree.

## What runs

Full RT stays the startup default and reference in the same executable/APK.
It uses the current canonical production renderer/shader path, not the published
application binary. Normal builds keep HORDE_RT_HYBRID_STATIC_PRIMARY OFF.
Both experiment modes use Shipping instrumentation / Mobile dielectric quality.

The candidate rasterises immutable opaque StaticPbr owner instance 21 using
existing production vertex/index buffers. A 16-byte RGBA32UI visibility target
stores distance, draw/primitive IDs and packed 16-bit barycentrics, with D32 depth.
An adapter reuses production material/texture/normal decoding and shading in the
existing raygen or compute launcher. All secondary queries use the original
BLAS/TLAS. Original production shader sources and frozen variants are unchanged.

An additional TLAS masks out owner 21 for remaining **primary RT**. The nearest
RT or raster hit wins. Procedural world owner 0, actors, held equipment, water,
glass and other owners remain primary RT. Reflections, shadow quality, fire,
dust and mist were not deliberately reduced. This preserves the visual model
while bounding integration risk; considerable primary RT work remains.

The three scenes record 11 real raster draws / 751,868 triangles per candidate
frame. Correctness-first nonindexed vertex pulling avoids a Geometry capability
requirement, but repeats vertex work and is not an optimised mobile draw path.
Transmitting owner-21 materials, nonidentity primitive transforms, ambiguous
duplicate owner IDs and replaced geometry buffers are rejected with diagnostics.
The admission rules need revisiting if the asset/material owner layout changes.

Windows has a Rendering comparison menu, request-labelled title/HUD, CLI mode,
isolated timing CSV and experiment saves. Android has a session-only Graphics
request and render-thread mailbox. Switches drain old GPU/evidence work and
reset timing generations. Submitted-mode logs do not certify GPU completion or
presentation. Capture evidence separately joins shader identity, completed GPU
submission and successful presentation. Public benchmark/report submission is
disabled throughout both experimental modes. Generic frame benchmarkEligible
fields are diagnostic contracts, not permission to publish a production score.

## Research and alternatives

The [Khronos sample](https://docs.vulkan.org/samples/latest/samples/extensions/ray_queries/README.html)
uses vertex/fragment rendering and actual fragment ray-query shadows. The
[ray-query specification](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_ray_query.html)
supports secondary traversal from graphics, compute and RT shader stages with
acceleration structures. This supports technical feasibility; it promises no
speedup on a particular phone.

| Option | Fit for this engine |
|---|---|
| Forward raster + genuine ray-query lighting | Plausible future path without a material G-buffer. Needs shared shader refactoring, all primary-geometry ownership and overdraw profiling. The earlier synthetic lab explored only the mechanism. |
| Compact raster visibility + existing shading | Implemented here with real assets to limit shading divergence; costs a full-resolution visibility attachment, depth and extra TLAS. |
| Deferred material G-buffer | Possible, but adds attachments/bandwidth. Existing StagedPrimaryPass stages RT-primary records, not a ready raster backend. No phone evidence favours it here. |
| Shadow maps, baked lighting, screen-space reflections | Different visual/feature comparison; not implemented in this equal-effects test. |
| Fewer rays/lights or temporal denoising | Separate quality/history investigation; bundling it here would confound visibility costs. |

Before expanding, measure GPU primary/secondary/AS/raster stages separately on
the same phone. Investigate indexed/shared vertex work, removing the extra TLAS
and a direct forward path only if the measured primary cost warrants it.
This prototype does not settle the general merits of hybrid rendering.

## Final laptop results

Only audited-* runs in matched-raw-evidence.zip are final. Earlier synthetic,
unoptimised and first-frame-identity-debug runs are excluded. Candidate compiler
settings match production: OpaqueFast glslang -Os plus spirv-opt -O; Generic uses
the retained-function cleanup list read from the frozen variant catalog.
All six generated modules pass spirv-val Vulkan 1.2; no Geometry capability.
shader-manifest.json records dependency and module SHA-256 values.

RTX 5050 Laptop GPU; Vulkan 1.4.341, raw driver 2559295488. Each fixed checkpoint
ran Full RT → Hybrid → Hybrid → Full RT in separate processes, 960×540 at scale
1.0, dynamic resolution off, identical camera, geometry, textures, lights,
effects, fixed simulation tick/time and settings. Each run has 180 completed/
presented frames; first 60 excluded, 120 measured. Every final frame joins valid
GPU time to completion/submission and the correct shader identity.

Values are the mean of the two run medians for each mode. Lower is better.

| Checkpoint | Full RT GPU ms | Hybrid GPU ms | Change |
|---|---:|---:|---:|
| Forest lookout | 1.298 | 1.487 | +14.5% |
| Niche arch | 1.750 | 1.960 | +12.0% |
| Stone portal | 1.635 | 1.821 | +11.3% |

The candidate is slower in all three median comparisons. GPU time covers the
whole command buffer: dynamic AS work, candidate extra TLAS/raster/shading and
copy. CPU stage medians, per-run GPU p95 and raw samples are retained; no
independent GPU pass decomposition is claimed. Both modes allocate dormant
experiment resources, so their inventories do not establish memory savings
versus a normal production build. Visibility/depth alone require 20 bytes/pixel
before alignment; existing geometry/material/texture buffers are reused.

CPU cycle medians are about 5.94–5.96 ms, roughly 168 cycles/s, mostly including
presentation waiting. These are short frozen-scene cycle rates, not sustained
gameplay FPS or display scanout. Reciprocal GPU time is not FPS. The first forest
reference has a higher p95 than its median; all run distributions remain in the
report. Power, clocks and thermals were not measured/controlled. This is not a
dynamic animation/gameplay or phone battery/thermal test.

Images preserve the lighting/material model closely, with raster coverage and
barycentric quantisation differences. Mean absolute RGB error is about
0.0036–0.0699 out of 255; 0.013–0.136% of pixels exceed 8/255 in any channel.
Some isolated edge errors are larger (maximum 158/255). Pixel metrics do not
certify perceptual equality or temporal stability. The niche's existing
held-high equipment obscures both modes equally. Portal/forest give more useful
views. comparison.html includes A/B images and amplified differences.

## Validation and limitations

- Windows Release build and native Vulkan storage-image/swapchain captures.
- Desktop 540x960 portrait capture smoke checks passed in both modes (180
  completed/presented frames each), with candidate validation layer loaded.
  This does not test physical resize, rotation or a phone.
- Twelve matched 180-frame runs plus final candidate RT-pipeline and
  ray-query-compute validation runs. Khronos instance/device layer loading is
  confirmed; no VUID/validation errors or validation warnings. Loader environment/
  registry warnings remain separately in the logs.
- Six SPIR-V modules validated with matched optimisation policies.
- Offline Android assembleDebug and lintDebug passed: 0 lint errors, 96 existing
  app warnings. SDK XML/deprecated API warnings remain.
- APK signature, unique app identity, arm64-only library, 53 real GLBs and no
  runtime asset LFS pointers checked. Existing debug certificate reused.
- Windows graphics contracts: 516 checks; benchmark summary contracts: 43.
- Four Android property-policy checks: default off, separate opt-in ID, invalid
  boolean rejection, conflicting benchmark experiment rejection.
- Experiment-off syntax checks for changed renderer and Windows entry point
  using the existing compiler. This is not a full ordinary-build link/test run.
- Scoped git diff --check.

No Android device is connected. **Phone runtime/performance is unmeasured.**
Windows menu interaction, physical resize/rotation, runtime-switch UI, Android
suspend/resume, allocation failure and all mobile driver/descriptor combinations
remain untested. Hidden native Windows presentation/captures work; screenshots
exclude overlays. Android arm64 packaging does not certify an RT-capable driver.

Eight existing runtime files were LFS pointers. Verified local cached objects
restored exactly those paths: 7,149,588 bytes, no download or new asset intake.
runtime-lfs-receipt.json records them. The supplied voice source folders were not
imported into this rendering experiment.

## Try it locally

Artifacts:
C:\Users\sam_s\Documents\Codex\2026-11-05\task\native-hybrid\deliverables

Open comparison.html. Run "Run Horde - Full RT.cmd" or
"Run Horde - Partial Hybrid.cmd". Both use the same native EXE and enter the
existing development rescue journey. Use Rendering comparison to switch; keep
resolution/scale/quality fixed. F1 explains scope. Live capture toggles are
blocked. The existing reports directory beside the EXE receives
windows_hybrid_static_primary_timing.csv. Saves are isolated under
hybrid-static-primary-state/campaign-v1 beside the EXE.

These Windows launchers are local to Sam's laptop and depend on the canonical
checkout's actual assets through the explicit experiment source-asset fallback.
They are not portable distributions. No second runtime asset tree was created.

Android: Horde-Hybrid-Static-Assets-Experiment.apk, about 186 MiB, label
Horde Hybrid Static Assets Experiment, package
com.samfa12.hordelanternrt.debug.hybridstatic, arm64-v8a. Separate identity permits
coexistence with com.samfa12.hordelanternrt; installation itself is untested.
Inherited debug version 1.7.0-debug/code 11 is not a released 1.7 build.
binary-manifest.json contains exact binary hashes. Nothing has been installed.

After manually transferring/installing on a compatible RT phone, use Graphics →
Partial hybrid experiment → Request Hybrid static-assets. Startup default is Full
RT; mode requests are session-only and do not alter quality or scale. Graphics
preview Details show presents/s, CPU/GPU/loop time and tracked allocations. These
are preview metrics; an empty owner-21 preview does not exercise this asset slice.
Use actual campaign locations for the real-assets visual comparison.

For phone measurements preserve model/OS/driver, APK hash, submitted mode,
camera/checkpoint, internal/output dimensions and every quality setting. Let it
settle, run RT/Hybrid/Hybrid/RT without changing settings, and repeat with
several minutes of thermal behaviour. Check switching, pause/resume and rotation.
Retain unsupported-hardware diagnostics. Logcat emits
HORDE_HYBRID_STATIC_PRIMARY_SUBMITTED with mode/epoch/generation; it is submission
evidence only. A hardware session is needed to validate the best gameplay timing
export route; do not substitute preview FPS for gameplay evidence. Production
benchmark sharing is disabled in this APK.

## Reproduce

PowerShell 7, VS2022 C++ tools, Vulkan SDK and Python 3; Pillow for image analysis.
Reuse the existing build directory/SDK/assets. Generator command:

~~~powershell
$repo='C:/Users/sam_s/Documents/the Horde RT Demo'
$trial='C:/Users/sam_s/Documents/Codex/2026-11-05/task/native-hybrid'
$cmake='C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
python "$repo/tools/compile-hybrid-static-primary.py" $repo "$trial/generated"
$configure=@('-S',$repo,'-B',"$trial/build",'-G','Visual Studio 17 2022','-A','x64',
  '-DBUILD_TESTING=OFF','-DHORDE_RT_CHAPTER_MEASUREMENT=ON',
  '-DHORDE_RT_HYBRID_STATIC_PRIMARY=ON',
  "-DHORDE_RT_HYBRID_STATIC_SHADER_DIR=$trial/generated",
  '-DHORDE_RT_INSTRUMENTATION_OVERRIDE=Shipping',
  '-DHORDE_RT_DIELECTRIC_QUALITY_OVERRIDE=Mobile',
  "-DHORDE_RT_WEBVIEW2_SDK_ROOT=$repo/.worktrees/horde-1.7-wp2-vertical-proof/build/dependencies/Microsoft.Web.WebView2/1.0.4258.31")
& $cmake @configure
& $cmake --build "$trial/build" --config Release --target horde_rt_diagnostic_window --parallel 2
$env:JAVA_HOME='C:/Program Files/Android/Android Studio/jbr'
$env:ANDROID_HOME='C:/Users/sam_s/AppData/Local/Android/Sdk'
Set-Location "$repo/android"
$props=@('-PhordeHybridStaticPrimary=true',"-PhordeHybridStaticShaderDir=$trial/generated",
  '-PhordeRtInstrumentationOverride=Shipping','-PhordeRtDielectricQualityOverride=Mobile')
.\gradlew.bat --offline --no-daemon :app:assembleDebug :app:lintDebug @props --console=plain
~~~

Capture into fresh paths; harness refuses to overwrite evidence. Run a complete
12-scene set plus two validation runs, with names ordered 0-rt, 1-hybrid,
2-hybrid, 3-rt for each of the three checkpoints above.

~~~powershell
$capture="$repo/tools/experiments/hybrid-static-primary/run_capture.ps1"
$args=@{Executable="$trial/build/Release/HordeLanternRT.exe";RepoRoot=$repo;
  Output="$trial/evidence/retest-chapter-stone-portal-front-1-hybrid";
  Checkpoint='chapter-stone-portal-front';Hybrid=$true}
& $capture @args
# Two validation runs: -Hybrid -Validation, then -Hybrid -Query -Validation.
python "$repo/tools/experiments/hybrid-static-primary/analyse_comparison.py" "$trial/evidence" "$trial/new-report" --pattern 'retest-*'
~~~

The analyzer requires 12 matched runs plus 2 validation runs, checks all identity
joins, mode hashes, sample ordering/counts, settings equality, positive raster
work and validation-layer logs, then builds the report. It cannot certify UI or
phone behaviour.

Storage snapshot: real experiment workspace ~0.74 GiB, earlier lab ~0.12 GiB;
canonical Android app/build ~1.33 GiB, app/.cxx ~0.94 GiB (total existing trees,
not claimed incremental growth). About 98 GiB free. No worktree duplication,
bulk runtime asset copy or deletion. Superseded local captures/intermediates are
separate from final evidence and remain for owner-reviewed reversible cleanup.


Source closeout: the real-assets implementation is uncommitted on the existing
development branch. Automatic approval review rejected the local checkpoint
commit, stating explicit user commit permission was unavailable. No part of
that rejected command executed. source-checkpoint.zip contains the explicit
18-file source snapshot; source-changes.patch covers tracked modifications,
while the ZIP also includes new files. The manifest records every source hash.
The binaries were built from this working-tree implementation, not from a new
commit. Loose .cpp/.h bootstrap copies in the parent task directory are historical
construction intermediates; canonical repository paths and the source archive
are authoritative.
