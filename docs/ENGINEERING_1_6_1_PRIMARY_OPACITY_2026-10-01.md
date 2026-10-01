# Bounded Mobile primary opacity admission experiment

Owner permits only one or two further materially promising trials, not another
broad investigation. This is candidate 1; no performance result exists yet.
Normal source control is 8c65afa694067e56b09ccb09c427fffd0b5ee570 plus prose-only
steering checkpoint. Original dirty engineering worktree/probe is untouched.

## Hypothesis and scope

Normal primary traceScene forces all triangles through shader candidate handling
to filter a few world-body regions. Let hardware accept ordinary opaque hits;
only named PlayerWorldBody remains forced non-opaque. Apply that instance flag
after all instance clones, in initial and animated TLAS uploads. Unfiltered
secondary rays still explicitly force opaque. Keep NoOpaque for ignoreWater and
all existing physical shadow/dielectric queries. Water-off filtering must not
be bypassed. Mobile-only shader change; High modules must remain byte-identical.

This is not the rejected first-confirmed-blocker experiment: no terminate-on-first
hit, shadow shortcut, light/ray/sample reduction, geometry/material change or
quality cut. No new buffers, dispatches, synchronization or CPU/GLSL ABI.
Intended saving: fewer shader-side primary candidate callbacks. Magnitude is
unknown; shader bytes/instruction counts do not prove occupancy or timing gains.
[Vulkan opacity precedence](https://docs.vulkan.org/spec/latest/chapters/raytraversal.html#ray-opacity-culling)
supports the mechanism, not image/performance acceptance on either phone.

Retain separately identifiable experimental source/artifacts; do not promote
automatically. Restore the normal candidate after testing. Source allowlist:
rt_hit_decode.glsl, PresentableTinyRtScene.cpp, affected contract tests and their
required generated shader artifacts. No platform/music/player tuning.

## Finite gate matrix

| Gate | Planned evidence | State |
| --- | --- | --- |
| Source contract | Water-off, body filtering, secondary opacity and clone isolation | Open |
| Shader | Actual compilation/validation; Shipping diagnostics absent; High unchanged | Open |
| Windows | Current Mobile primary/body/water/lantern images, pipeline and compute, unchanged tolerance | Open |
| Exact S26 | Same authored image fixtures, primary ownership and physical-query containment | Open |
| Shipping timing, only after image admission | Opening route and glassless held-high; interleaved A1/B1/B2/A2 warm runs at75%, same backend/extent | Open |
| Memory | Active RAM/PSI separate from GPU bandwidth/cache/stall gaps; no intermediate allocation | Open |
| Restore/decision | Normal APK/config restored; preserve negative results, achieved30FPS gap and next action | Open |

S24 missing-hands/enemy follow-up is independent and does not certify S26 or this
experiment. Do not repeat its completed physical-vs-open-pane timing matrix.
Exact new candidate/control hashes and completed rows belong here once produced.
Rebuild unchanged artifacts only for a stated validity problem. Current UI/report
source differs from the old pane benchmark; a current Shipping control may be
built once to eliminate that difference, not to repeat the old experiment.

## Next unfinished step

Review the bounded flag change and affected regression contracts; then produce
one candidate. No timings until unchanged-tolerance images pass. Stop on image
failure, incomparable runs or absent/small gain; no tuning sweep. Audio/haptic
manual revalidation required: **NO**, semantic playback inputs are unchanged.
No main merge, release or publication is authorized.
