# Unoccluded colour-floor isolation

The owner reported implausible yellow light on hand/arm backs and sword. This
matched native RT A/B isolates a demonstrated contributor; phone validation
of the correction and final lighting acceptance remain open.

## Cause and change

Primary and terminal secondary opaque shading added
`localColour * .16 * clamp(localStrength,0,1) * exp(-hitDistance*.24)` **after**
visibility-tested lighting and fog. This added colour regardless of surface
normal, light occlusion or albedo. At the high/low lantern checkpoints, the
selected local source is the yellow bay light, not the carried flame.

Remove only this unoccluded post-fog colour term from both receiver paths.
Preserve real direct-light shadow/transmission rays, fire emission/integration,
reflections, refraction, IOR, Fresnel, absorption, TIR, fog and geometry. Do not
replace it with a fake shadow factor or material darkening.

An independently investigated stale-torch hypothesis does **not** explain
these captures: checkpoints142/143 begin at settled-torch checkpoint5; the
original torch has strength0 and is filtered from the emitter upload. Normal
continuous progression also extinguishes it before reaching the reward.
Artificial debug jumps bypassing that trigger are a separate, unproven case.

## Native evidence

`before.png` and `after.png` retain manifests. Both are Diagnostic/High,
RayTracingPipeline, RTX5050 Laptop, 540x960, scale100%, fixed high-carry pose,
same .099 gauntlet candidate and paired world. Exact uploaded OBJ bytes match.
Before executable is the staged `54493cf` runtime candidate; after executable
SHA-256 `356c67ae649a35bfd94150e38b696141c4ee8c0e8fc7cdf77971dfa9e884022f`.

High-carry pixel(170,550), visibly on the occluded hand back, changes from
RGBA(86,49,13,255) to(1,0,0,255). The flat wash disappears; actual lit lantern
and floor remain. Low-carry OBJ also matches exactly. The torch-held grips
control is **pixel-identical**, not merely within a loosened tolerance.

Fresh standard-route A/B completed13/13 captures each. Opening, skeleton,
worst-bend, lantern-drop, skylight, mirror, lich and two-enemy-combat are
pixel-identical. Yellow/blue/red/green bays and finale-roof change where the
removed local-colour term was active. No resolution, geometry, material,
transport budget or image-gate adjustment was used.

Local full captures: `C:/Dev/tmp/horde-yellow-wash-native-20260927-a` and
`C:/Dev/tmp/horde-yellow-wash-standard-20260927-a/{before,after}`.
The first attempt to pass standard names through the development-only helper
was correctly rejected; standard captures were rerun through `--capture-showcase`.

## Validation and boundaries

- Eight raygen and eight hardware RayQueryCompute variants freshly compiled,
  validated and disassembled; both compatibility raygen includes regenerated.
- All Shipping variants retain **zero atomics/no diagnostic binding22**.
- Shipping Mobile OpaqueFast:501424 ->500784bytes,27554 ->27525instructions.
  Shipping Mobile GenericDielectric:219304 ->218620bytes,13012 ->12975instructions.
  These are code statistics, **not measured frame-time improvement**.
- Fresh Windows Debug character-render source contract passes; variant,
  provider, bundle, dielectric-math and fire-emitter contracts pass5/5.
- Actual Windows executable bundle extraction passes SPIR-V validation and
  disassembly, with the intended Diagnostic/High raygen and compute pairs.
- This does not prove backend image parity, Shipping/Diagnostic image parity,
  new Android device acceptance or S24/S25 support. Existing gates stay open.
- Phone still has arm candidate APK `fce8b40c...`; no lighting-update install
  was performed during isolation. Audio/haptic manual revalidation required:
  **NO**; lighting-only shader change leaves feedback/event data unchanged.
