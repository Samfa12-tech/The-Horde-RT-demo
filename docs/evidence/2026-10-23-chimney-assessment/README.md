# Chimney-smoke assessment and exact scenery CI follow-up

This is a read-only runtime/effect assessment at scenery head
`b9019b43a555a3f5994bcd603bc862999613a961`, in the existing WP2 worktree.
No smoke, lighting, material, shader, quality, campaign or settings change is
admitted by this checkpoint. The accompanying code correction changes only an
exact inventory fixture's expected content total, described below.

## Existing reusable RT path

`shaders/raytracing/include/rt_fire.glsl` already integrates bounded analytic
flame/smoke volumes against the actual RT hit distance. `rt_frame.glsl` applies
them to primary rays; the dielectric paths use the same integrator for secondary
views. Both real Vulkan backends use this shared shading. No alternate renderer,
billboard or opaque plume is required or proposed.

It cannot produce an independent chimney cue unchanged:

- `integrateOneFireEmitter` returns immediately when flame intensity is zero.
  Smoke height/start are derived from flame height, and the same loop computes
  flame body, core, turbulence and emission before smoke extinction.
- `BuildFireEmitterUpload` requires positive strength/fuel, the selected zone
  and a source within 24 m of the camera. The actual renderer supplies 24 m.
  Zero-strength smoke is excluded before shading.
- `PackFireEmitterGpu` gives the flame and direct-light channels equal radiance.
  `rt_lighting.glsl` uses positive lights for surface illumination and real
  visibility queries. Making a fire merely dark/invisible is not a free smoke
  mode; it can retain unwanted absorption, light work or emissive behavior.
- `ValidRtFireLightingEvidence` intentionally requires positive light strength,
  equal intensity and coherent identities. A volume-only emitter must not be
  misreported as an invalid/zero-strength fire or admitted by weakening this gate.

The CPU and GPU capacities are already four emitters; the configured simulation
prefix currently has three (carried torch and two Keeper torches). The reward
lantern is appended separately. A future single distant source must keep these
ceilings and existing lights across all phases. The existing 160-byte GPU record
and four-record buffer are reusable storage, not permission to expand budgets.

Low/Mobile/High use 2/4/10 integration steps. Reflections use at most 2/4/8 steps,
respectively. Adding an active source creates another analytic bound check on
primary and applicable reflected rays, then work for rays through that bound.
No new BLAS/TLAS is necessary for this existing analytic path. These are source
work bounds, not measured GPU time or a claim that additional smoke is cheap.

## Actual outlets and reference camera

The helper verifies all eight authored box corners for each chimney against the
actual admitted v4 GLB (966 triangles, identity node); maximum position error is
below 0.00002 m. It uses the retained successful native RT lookout camera and
rendered eye, with the current shader projection. It changes no asset.

| Chimney | Cap centre XYZ, metres | Distance from lookout | Roof above cap centre |
| --- | --- | ---: | ---: |
| B01 tavern | 95.36, -0.27955, 109.28 | 109.880 m | 10.933 mm |
| B03 house A | 38.98, -3.87853, 116.76 | 84.662 m | 26.545 mm |
| B06 smith | 111.44, -6.74261, 153.52 | 153.121 m | 20.216 mm |

Every existing chimney is outside the current 24 m selection. The closed pitched
roof crosses each cap centre, so its centre is not a clear smoke outlet. This
does not claim every corner of the chimney is buried.

CPU segment/triangle tests against the **town mesh only** find all cap centres,
and probes 0.3 m and 1 m above them, occluded. Four metres above B01 clears the
town mesh; equivalent B03/B06 probes remain occluded by other town geometry.
This makes B01 the useful first candidate, not a proven full-scene visible cue:
the helper excludes terrain, trees, volume shading and actual plume animation.

In the retained 960x540 desktop view, B01's cap-to-4 m probe spans about 17.32
vertical pixels. Scaling that same viewport to 33%/20% would leave about
5.71/3.46 pixels. Those are reference-camera projections, not actual phone
dimensions or image-quality acceptance. The low-resolution shimmer/readability
gate remains relevant even if a tiny distant volume measures well.

## Smallest credible next implementation

Use one B01 source after correcting that chimney's outlet with an owned,
provenance-preserved derivative. Preserve town footprints, terrain, silhouettes
outside that outlet, route, camera and material ownership.

Extend the existing reusable emitter path with explicit smoke-only visibility,
extent/density and zero light/flame/embers. Keep normal fire behavior and its
24 m selection unchanged; smoke needs its own finite declared range and region
admission. Retain real RT hit-distance clipping, primary/secondary consistency,
fixed-step state, lifecycle cleanup and truthful light-versus-volume evidence.
Do not repurpose the Keeper Mist toggle, add hidden fire or enlarge all torches.

Only then compare one bounded candidate with an unchanged control at identical
dimensions, settings, route and thermal context. Record covered pixels, CPU/GPU
timings and allocation deltas; investigate regressions using the existing
validation rules. Inspect actual RT motion, outlet contact, occlusion and low-
resolution readability, including reflections. Phone visual/warmed acceptance
requires a newly authorized device window. No added time budget or acceptance
threshold is granted here. The shared smoke-only extension and outlet correction
are required implementation scope, not an already-working configuration switch.

## Exact CI defect and focused correction

The terminal [scenery run 39901839862](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39901839862)
has five successful jobs and one failed Vulkan-host job, which passes 71/72
registrations. MSVC completes 169/169 selected cases, its isolated asset-policy
case and subsequent required offline steps. The integrated inventory fixture
expects 1,292,118 indices but actual intake contains 1,292,550: exactly 432 indices
for the added 144 road triangles. Local Debug reproduces that original failure.

The expected sum now includes exactly 144 triangles. Frozen fixture primitive
31/material 30 counts, both 32-row ceilings, entry bounds and all other assertions
remain unchanged. The same focused registration passes after rebuilding, 22.92 s.
This is test-only; the already inspected runtime and APK remain unchanged.
Original hosted/local failures and terminal job receipts are retained. The
test-only corrected head requires a separate complete hosted result; this local
pass does not certify all hosted jobs.

The preceding roster-correction run `39900686663` ends cancelled after about
15 minutes. Five jobs pass; MSVC's last surfaced result is 111/169, with case 112
active at cancellation. Existing YAML has a 15-minute job cap. This is incomplete
coverage, not proof that case 112 or gameplay hangs. No timeout or CI structure
is changed. The separate Debug 30-second route gate and PR conflicts remain open.

Audio/haptic manual revalidation required: **NO**; assessment and fixture content
count only. No phone, install, runtime effect, merge, deployment or release action.
