# Bounded Mobile primary opacity admission experiment

Owner permits only one or two further materially promising trials, not another
broad investigation. This is candidate 1; finite Shipping collection is running.
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
required generated shader artifacts plus the small tested named-instance flag
helper. No platform/music/player tuning.

First frozen publication correctly rejected a104B/10-instruction Generic Mobile
growth against a byte-exact old compiler ceiling (the initial branching form
was132B). The compact flag selection retains the physical interface/volume/ray
budgets, atomics, loops, functions and calls. On this experimental branch only,
update the two Generic Mobile compiler-size ceilings to the measured104B growth,
not a physical traversal-budget increase. OpaqueFast grows20B/one instruction
within the existing ceiling. All four High SPIR-V hashes are byte-identical.
Independent review agrees on semantics but notes the unfiltered Generic shadow
path can gain body-candidate callback cost; matched timings must include that
tradeoff. Added upload-after-clones and shadow confirmation source contracts.

## Finite gate matrix

| Gate | Planned evidence | State |
| --- | --- | --- |
| Source contract | Water-off, body filtering, secondary opacity and clone isolation | Affected4/4 PASS |
| Shader | Actual compilation/validation; Shipping diagnostics absent; High unchanged | Actual16 modules PASS; eight High hashes identical |
| Windows | Current Mobile primary/body/water/lantern images, pipeline and compute, unchanged tolerance |26 images byte-identical; pipeline capture-timing gate fails+4.766%, compute passes−0.963% |
| Exact S26 | Same authored image fixtures, primary ownership and physical-query containment | Six images byte-identical; strict ASTC/presentation/Home-resume PASS |
| Shipping timing, only after image admission | Opening route and glassless held-high; interleaved A1/B1/B2/A2 warm runs at75%, same backend/extent | Running; no gain claimed |
| Memory | Active RAM/PSI separate from GPU bandwidth/cache/stall gaps; no intermediate allocation | Open |
| Restore/decision | Normal APK/config restored; preserve negative results, achieved30FPS gap and next action | Open |

S24 missing-hands/enemy follow-up is independent and does not certify S26 or this
experiment. Do not repeat its completed physical-vs-open-pane timing matrix.
Exact new candidate/control hashes and completed rows belong here once produced.
Rebuild unchanged artifacts only for a stated validity problem. Current UI/report
source differs from the old pane benchmark; a current Shipping control may be
built once to eliminate that difference, not to repeat the old experiment.

## Next unfinished step

Code and independent review complete. Fresh MSVC Debug affected host4/4 PASS;
full Mobile Windows app and Android Debug/Shipping benchmark four-ABI builds
PASS. First builds correctly reject stale generated catalog adapter; publisher
regeneration resolves it, negative logs retained. No broad testing repeated.
All16 actual pipeline/compute modules compile/validate; eight High hashes remain
byte-identical. Actual candidate ARM64 extraction validates/disassembles four
Shipping/Mobile modules and shared miss/hit stages; zero atomics/Binding22.
Single-artifact audit adapter updates only the two catalog pins, retained with
SHA48b7ce65e5ce9c73a5c7723ba636c1d727de2439752e4d490f0bc3043c19cb20.

Control Debug8cb976891e5719eceb7fd809ec91aac939ff3ec58eb2d6df44f15b5526f4ff87,
Shipping3cb84efb2f71b1c96b8a562ae6a272569e3c76315053144582a617be75bf30eb.
Candidate Debug6da2bccf3ec90f4dfe0aef1b9899626c6fb0a13525359c6fc229146f3848fa8b,
Shippingb9d69ff43c13b0d84ff8fca11132578a27ae710ac2946546677d707655b9a188.
Frozen binaries/raw logs: `C:/Dev/tmp/horde-primary-opacity-20261001`.

RTX Mobile Diagnostic pipeline13/13 and compute13/13 images are byte-identical.
The unchanged foundation comparator's full result fails pipeline capture timing
at+4.766% against its2% gate; compute timing−0.963% passes. Preserve both receipts;
this is image equivalence, not a full foundation/Shipping performance pass.
Exact SM-S948B75% Diagnostic pipeline control run222521 /candidate223006 each
passes six frozen captures, strict ASTC, honest RT presentation and Home/resume:
opening,two-enemy,skylight,lantern-high,low-look-down,glass-edge-fresnel. All six
PNGs are byte-identical, stricter than unchanged maxRGB3/fraction0.001. Mutable
frame serials/skin-update counts are not pose differences. Not High acceptance.

**Next:** finish the eight named S26 Shipping rows already launched from
`C:/Dev/tmp/horde-primary-opacity-20261001/performance/run-opacity-matrix.ps1`,
then its integrity-only analyser and lead performance/thermal admission. A1 route
and held-high complete with1838/600 presented rows and native medians64.6229/
96.9337ms; B1 route is running next. No A/B conclusion yet. Candidate
source is pinned e78028c999e745e7cfd1695468062bb45f300b21, not moving evidence HEAD.
Normal source is6fa1c53 prose-only ancestor of1e20f438. Do not repeat complete rows
or rebuild unchanged APKs; existing incomplete rows halt for exact recovery.
The matched collector adds one sparse active RAM/system-memory/PSI sample after30s,
with retained UTC bounds, separate from lifecycle-boundary before/after snapshots.
It does not measure GPU bandwidth/cache/stalls. Then normal restoration/decision.
The S24 candidate containment also has two byte-identical images, zero player
pixels and restored normal APK; it does not resolve its visibility defect.
[Retained image/state receipts](evidence/2026-10-01-primary-opacity/README.md).
Stop on
incomparable runs or absent/small gain; no tuning sweep. Audio/haptic
manual revalidation required: **NO**, semantic playback inputs are unchanged.
No main merge, release or publication is authorized.
