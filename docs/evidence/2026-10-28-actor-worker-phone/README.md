# Bounded actor-worker phone check - 28 October

Phone **released08:36:55UTC**, before the08:38:16 hard deadline. No owned test
process, monitor or transfer remains. No further phone operations after release.
Source958f5182 and exact frozen APK SHA256
`9466ba1a06437bfaa119cf19c457aa017fab2848e0dc910cb8ed8d95a9fa3296`,
isolated package `com.samfa12.hordelanternrt.debug.actorworker`, development
version1.6.2-debug/code10. Source CI40654343098 passed all six jobs.

SM-S948B, Android17/API37: actual Pipeline RT presented at720x1486 internal /
1440x2971 output,50%, Shipping/Mobile, Water1/Fire0, GlassOff, Current shadows,
MistOff/Dust1. Preview cap setting30 was unchanged; Showcase bypasses its software
preview limiter. These are elapsed/render/present-return observations, not scanout
FPS or a claim of an uncapped display. No quality/scale change between modes,
private shader substitution, GPU ownership/fence change or frames-in-flight increase.

Serial and worker generated-input routes completed with590/589 owning RT rows,
matching submission/completion identities and zero pending submissions. Their
settings and exact production shader-pair hashes match. Logs witness actual
serial and worker CPU preparation, not merely a mode request. Both contain109
landing samples with actual Kit and player preparation. Timing below is that
active-Kit field population, not the whole route or a sum of overlapping medians.

| Median ms | Serial | Worker |
| --- | ---: | ---: |
| Combined skin elapsed critical path | 9.909 | 8.185 |
| Player skin elapsed | 5.181 | 3.709 |
| Kit skin elapsed, overlaps player in worker mode | 4.613 | 4.501 |
| Whole render-entry-through-present cycle | 213.035 | 208.710 |
| Owning GPU duration | 198.328 | 198.843 |

**No causal gain admitted.** Starts were AP43.1/BAT33.8/SKIN37.4C versus
AP47.2/BAT38.5/SKIN40.9C, with a higher skin thermal status. Process/setup differed,
there is no matched closing control and clocks/per-core scheduling were not
measured. Player timing also changed: temperature alone does not establish the
cause. The observed cycle difference is about2.0%, below the3% useful-gain goal
even before these limitations. Preserve this warm-device field evidence; do not
claim thermal equilibrium, a measured worker FPS gain or mobile admission.
Normal worker and flask/ring/yellow/shared64 remain OFF.

Two retained setup failures produced no performance samples: the first timed
request preceded the required reunion, and the repeated same-process checkpoint
request did not establish the required start state after the route. Fresh-process
staging produced the valid worker run. Improve observed start-state acknowledgement
offline before another phone slot; do not weaken scenario gates or turn request
acceptance into readiness. No new build/reduction loop on the phone.

Home suspension returned successful GPU idle and resume was accepted. No new
successful-presentation identity was separately witnessed after that resume;
full lifecycle/visual/owner acceptance is still open. Both routes are seeded
post-Keeper fixtures, not natural journey, save/load/death or combat acceptance.

Original production/debug APK hashes are unchanged; original app data were never
targeted or cleared. The isolated package is left installed and stopped. Rotation
and font scale1.5 are unchanged. Brightness94 was requested before restoring
original automatic mode1; the subsequent automatic-mode readback was73. No exact
numeric brightness-restoration claim. Original automatic mode is restored.

Unscoped package enumeration returned primary-user results then denied secondary
user150 (`Shell does not have permission to access user150`). That operation was
not retried and that profile was not accessed. Subsequent operations were scoped
to authorized primary user0 and the isolated package. No root/security change,
new tracing access, report submission, public score, merge or release.

Sanitized aggregate/identity/hash evidence is in receipt.json. Raw logs, failed
receipts and completed ledgers/CSV stay in ignored reports/actor-phone-20261028;
no duplicate archive or unrelated device media is committed. Next performance
step needs a fresh bounded lease, proven initial-state fixture, equivalent warm
controls and a closing control; this result does not authorize pre-fence changes.
