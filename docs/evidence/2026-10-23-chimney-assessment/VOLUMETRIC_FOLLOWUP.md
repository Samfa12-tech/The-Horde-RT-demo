# Shared volumetrics follow-up: proposal, not runtime admission

Owner direction on 23 October requests an attractive Windows High option and a
runnable mobile Low option, adaptable to chimney smoke, ground mist and later
forge/foundry smoke. The phone window took priority. No new medium, shader,
quality setting or source has been enabled by this assessment.

## What is actually shared now

| Existing path | Implementation and cost controls |
| --- | --- |
| Distance fog | Surface-colour blend in `rt_lighting.glsl` and `rt_dielectric_common.glsl`, not density integration. Preserve it until an explicit migration avoids double fogging. It is not a volumetric fallback. |
| Keeper mist | `rt_atmosphere.glsl` clips a world-space AABB to actual RT geometry, then marches density. Current workload selects 2/6/8 steps. Midpoint incident lighting can query sky, staff and up to four fire sources. |
| Fire smoke | `rt_fire.glsl` bounds and marches flame/smoke together. Low/Mobile/High use 2/4/10 steps. Smoke has procedural extinction and a small fixed colour contribution, without the mist lighting model. |
| Indoor dust | `rt_indoor_dust.glsl` already reuses mist visibility/attenuation and selects one/two strongest light sources. Its tile-list approach is useful precedent, but camera-inside fog cannot inherit its near-plane rejection unchanged. |

`rt_frame.glsl` currently composites surface, fire, Keeper mist, electricity and
dust in fixed order. This is not a general solution for overlapping media. Both
real Vulkan RT backends share these shader paths. Primary geometry clipping and
actual RT visibility must remain; no billboard, opaque plume or baked lighting
is proposed.

## Smallest useful foundation

Prototype one bounded medium descriptor, independent of flame and direct-light
enable, and a shared segment integration function returning radiance and
transmittance. Reuse the existing ray/box interval, segment transmittance,
active-sky and admitted light-visibility helpers. Keep stable source identity,
bounded extent, extinction/scattering parameters, density profile, drift and
deterministic simulation time. Start with a rising-plume profile and make a
ground-layer profile possible without adding future content.

Use integrated segment extinction so different sample counts preserve physical
opacity. Sort disjoint intervals and combine coefficients for overlaps rather
than stacking independent colour overlays. Handle a camera inside a bound and
clip against the actual ray segment. Preserve the accepted fire marcher and
Keeper behavior initially; moving existing mist into the common function needs
a separate matched parity check. Existing distance-colour fog stays distinct.

Do not encode smoke as an invisible fire: zero intensity exits before smoke,
packing couples flame and light strength, the fire light receipt correctly
requires positive coherent lights, and the current source cutoff is 24 m.
B01 is about 110 m away. Medium range/visibility and light admission need
separate bounded semantics and truthful evidence. Do not weaken fire receipts
or enlarge ordinary torches to admit a chimney.

Do not use the Lean workload switch as a volume Low tier: it also changes
ordinary bounce/reflection behavior. New volume quality must be explicit and
independent of existing fire quality, Mist Off, shadows and saved preferences.

## Provisional comparison presets

These are experiment inputs, not approved performance budgets or proven visuals.

| Control | Mobile Low starting point | Windows High starting point |
| --- | --- | --- |
| Density integration | Four samples per occupied interval, broad band-limited flow | Eight samples with an additional bounded detail band |
| Incident visibility | One dominant admitted source at one interval anchor | Up to two sources and two anchors |
| Existing secondary ray segments | Two density samples if secondary participation is admitted | Four samples with the same physical medium coefficients |
| Distance/footprint | Coarser density and smooth subpixel fade | Preserve useful detail longer |

Beauty must come from coherent buoyant flow, extinction and spatial lighting,
not sample count alone. Low must retain recognizable motion and smoke shape.
Anchor lighting is an approximation requiring roof/wall-edge checks. Glass,
reflections and camera-inside behavior need explicit acceptance: current mist
is primary-only, whereas fire already participates in selected reflections.

The important cost bounds are covered pixels, overlapping intervals, distance
sampling, visibility evaluations and secondary-path work. Static-record upload
can avoid redundant CPU work; a lower upload cadence does not save per-pixel
marching. Do not add temporal history/reprojection as an incidental optimization.
It requires separate motion/depth, disocclusion and invalidation work.

## Feasibility and next experiment

The fresh [phone receipt](../2026-10-23-phone-skin/README.md) gives control GPU
medians of 17.86/39.65/61.77 ms at 20/33/40 percent. Starting temperatures differ;
this is not an admitted common-temperature scaling cohort. The phone later
reached thermal status 1 and the candidate comparison was rejected. These
measurements supply no accepted extra mobile volume budget and no smoke cost.
Twenty percent also remains visually unadmitted. Windows gains cannot certify
phone feasibility.

Next: one B01 prototype behind explicit investigation admission, using matched
unchanged Windows controls at exact dimensions/settings. Measure On/Off GPU and
CPU times, bounds/covered pixels, light queries and memory; inspect fresh RT
motion, roof occlusion and distant readability at both provisional tiers. Test
camera-inside/full-screen and overlap cases separately before claiming reuse for
fog or a foundry. Keep or withdraw the prototype based on that evidence, then
repeat the admitted workload on a cooled owner-offered phone. No larger
framework, future content or quality reduction follows from this proposal.

An isolated v5 geometry derivative was prepared but remains unadmitted and
untracked: it raises twelve B01 top-position Y values by 0.6 m and adjusts eight
side V values, retaining 966 triangles, 1,954 vertices and two materials. The cap
clears the surveyed roof by at least 0.3837 m but remains sealed; this is not a
rendered or physically open flue. Source/tool/receipt bytes are retained locally.
Its four incomplete package-path edits are parked separately; v4 is still the
accepted runtime. No smoke was present in any phone measurement.
