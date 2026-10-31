# Directional enemy-hit particle source candidate

Base `0b81df94` passed all six jobs in [41175793605](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/41175793605). This PR37 follow-up adds bounded contact feedback; it is source/build evidence, not native visual acceptance or a release.

## Ownership and limits

The same fixed-step successful sword-damage outcome that emits `EnemyHit` creates a burst identified by that exact semantic sequence and target. No burst comes from holding guard, a swing alone, enemy proximity or a miss. The origin is an explicit player-facing torso-surface proxy (0.22 m radial inset from the enemy anchor, 1.05 m above its root). Direction uses the accepted cut and player-to-target heading, frozen on that tick. Current live combat is forgiving range/cone authority: this does **not** invent a measured blade/skinned-triangle intersection or enable the diagnostic contact evaluator. Physical contact-point refinement remains a visual gate.

The owner retains two bursts, up to eight particles each, for 0.42 active simulation seconds. Motion is deterministic in world space; teaching slowdown applies, render cadence/camera do not reseed it. Pause/lifecycle synchronisation, death, retry, reset and campaign reconstruction discard transient effects. Monotonic contact identity prevents replay after cancellation. Snapshots and prepared frames own their copies; platform audio drains do not own visual lifetime. No gameplay, health, parry or save-schema rule changes.

The existing world-space analytic mote path inside the RT-produced primary frame renders angular bone-coloured chips and dark droplets, lit only by its existing RT-visible sources and depth-limited by actual primary scene intersections. It is **primary-view only**: no reflected/transmitted particles, new particle shadows, secondary recursion, glow or added lights. It has no mesh-fracture simulation, floor decals or collision. These limitations must be judged on actual gameplay views before admission; no native image was inspected in this source-only slice.

The existing graphics option is now labelled **Particles**, with help describing room motes and hit fragments. Saved internal dust values/defaults are preserved. Off does no burst upload/evaluation; Low retains four samples per burst, Standard eight. Both share the existing 64-mote / four-candidates-per-tile allocation. At saturation contacts replace at most 16 ambient motes; `combatMotes`, `combatReplacedMotes` and overflow counts expose the bounded displacement. No BLAS/TLAS/descriptor capacity or GPU buffer allocation increase. Active bursts invalidate the ambient-only cache and upload the same 11,280-byte quality/mote block each render; expiry/cancellation rebuilds it so old fragments cannot persist.

## Checked evidence and cost

- Three affected native CTests pass: real combat outcomes/lifecycle, immutable simulation-to-frame adaptation, and bounded particle projection/quality/material/expiry/snapshot/direction contracts. Windows Release application builds.
- 35 focused Java tests pass (graphics options, compact portrait/large-font layout, prior contact audio); production Java compiles. The Windows graphics source/GDI measurement fixture passes 516 checks, including full option-caption fit. These are not native interaction tests.
- All eight raygen and eight hardware ray-query compute variants regenerate/validate; the compatibility includes and generated adapters are refreshed. Frozen shader limits are unchanged. An initial branching shape version exceeded selection-merges and was rejected. The retained arithmetic selection preserves every variant's selection-merge/ray-query-site count. Raygen SPIR-V grows 576 bytes / 23 instructions for opaque variants, 728 bytes / 31 instructions for generic variants. This is static code size, **not** measured GPU time or phone performance.
- No new art/audio/imported assets, paid work, foreground, audio playback, phone or service submissions. All 151 parked owner-file hashes remain unchanged. Local logs and shader comparisons are under ignored `reports/combat-particles-20261031/`.

**Audio/haptic manual revalidation required for this visual slice: NO.** It adds no audio/haptic event or routing change. The preceding guard/contact-audio checkpoint still separately requires listening acceptance.

## Next exact-candidate acceptance

With an approved Windows/Android candidate, compare Particles Off/Low/Standard on successful downward/upward skeleton cuts and Keeper strikes. Check chips versus dark drops, side/oblique framing, moving camera, corpse hits/misses, depth behind walls/body, and visibility under actual torch/light. Pause immediately after a hit, resume, retry, resize/recreate and background/resume: no old burst or replay. Inspect water/mirror views with the explicit primary-only limit. Measure matched active-contact versus idle/Off cost before making any performance claim. No phone/foreground session or installation is authorised by this receipt.

Wall/tree strike classification, block/impact listening, footstep dust and surrounding forest density remain separate unfinished work; no forest content changed here. Normal shared64/flask/ring/revival experimental gates remain as before.
