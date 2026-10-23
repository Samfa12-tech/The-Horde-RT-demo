# Kit run assessment and one waypoint-facing fix

Base `ca49a762d97f441c64aa6a7322f5caedb1d4fa53`, existing PR27 worktree.
Original Kit, world body and first-person arms remain selected and unchanged.
No new clip, mesh, paid generation, retargeting, catch-up motor or renderer work.

## Retain Walk at the current speed

Kit currently **leads the fixed route at 1.8 m/s** and waits outside a 6 m player
tether. He has no faster catch-up state. Production loads the owned `Warden|idle`
and `Warden|walk`; the same GLB also contains `Warden|run`.

The original asset SHA-256 is
`1ba628235814b5912ae1439b180347dd8e1cce85b0f4d1fae5bbacb41e7d3713`.
An offline Python double-precision estimate samples each Walk/Run cycle at 120
phases, reusing the historical read-only rig/contact helper. At the representative
20 mm semantic-boot clearance:

| Estimate | Walk | Run |
| --- | ---: | ---: |
| Duration | 0.8333 s | 0.5333 s |
| Inferred cycle distance | 1.5608 m | 2.2648 m |
| Inferred speed at 1x playback | 1.873 m/s | 4.247 m/s |
| Foot sweep with current 1.56 m distance phase | 1.801 m/s | 2.613 m/s |

A direct binding substitution would leave about 0.81 m/s estimated mismatch.
Matching Run's inferred stride to the existing motor would instead slow it to
about 0.424x, approximately 95 steps/min. Named-foot estimates agree. Neither
clip provides forward root travel. **Run is not adopted.** It is useful to retain
for a future genuinely faster movement state, not stationary turning.

Run's positional loop closes more closely in this estimate, but its idle hard-cut
boot join is larger. The estimated Walk ankle wrap gap (75/36 mm) needs native
confirmation. These are source/contact proxies: the current 0.18 s blend,
native grounding, visual quality and device cost were not simulated by that
offline analysis. Detailed 10/20/30 mm sensitivity and source hashes remain in
ignored `reports/kit-run-assessment-20261023/offline/`.

## Demonstrated defect and minimal correction

`CompanionRouteFixture::StepRoute` uses one stationary tick to advance a waypoint.
`Step` previously treated that tick like ordinary idle and turned toward the
player, even though route movement resumed immediately. A nearby player can
therefore cause a brief turn opposite the next segment.

The runtime change retains facing only when waypoint identity advances. Ordinary
idle still faces the player. The motor, collision/support, 1.8 m/s ceiling,
2.4 rad/s facing limit, walk phase/weight, clocks, acting, dialogue, reset/restore
and save schema are unchanged. This removes a small one-tick hitch; it does not
solve stationary foot sliding, initial departure alignment or following behavior.

The focused regression first fails at **4/2282 checks**, at the four waypoint
transitions. After correction it passes **2283/2283**, including ordinary idle
facing, at the same **2254 route ticks**. Acting and campaign checkpoint CTests
also pass (the latter 1058 native checks); three selected CTests pass in 25.65 s.
The initial failed control log is retained.

## Paired actual RT inspection

The existing hidden native motion fixture now retains three first-waypoint
snapshots reached through real fixed route steps and actual terrain support.
It preserves the original cases, exact output allowlist and 90 s / 180-frame
limits. Extra waypoint frames do not enter the older timing workload. The
loaded campaign stays paused and read-only. This is scripted immutable
production-route pose evidence, **not natural gameplay or an earned journey**.

Both matched runs pass **535 checks / 37 RT renders**, in 17.42 / 18.88 s:

| Native snapshot yaw, radians | Control | Correction |
| --- | ---: | ---: |
| Last movement before waypoint | 2.00321 | 2.00321 |
| Stationary bookkeeping tick | 1.96321 | 2.00321 |
| Next movement | 2.00321 | 2.04321 |

The 0.04 rad (2.29 degree) wrong-way kick is removed. All 26 matched motion
snapshots retain identical roots, waypoint/wait state, walk phases/weights and
clocks. Quality and resource inventories match. **28/30 PNGs are byte-identical**,
including normal walk, idle, turn and neutral/speaking/recognition views. Only
the two intended waypoint images change. Original RT frames were inspected;
the visible change is small and does not establish broad artistic acceptance.

Both runs retain 51 buffers, 60 allocations, 20 BLAS, one TLAS, 28 instances,
48,298,880 host-visible and 374,422,496 device-local bytes. Foreground, original
assets, isolated source save/manifest, settings and owner campaign custody pass.
No phone, listening, FPS, sustained timing or optimization claim follows.
Audio/haptic manual revalidation required: **NO**; root motion, event timing and
audio/haptic semantics do not change.

Retained narrower attempts: the first native player-on-left control confirms a
facing change but points the same way as the next route turn. One corrected-side
control establishes the wrong-way case; both remain recorded. The first build
command used the EXE name as a target and stopped before compilation; the actual
`horde_rt_diagnostic_window` target then built successfully. No rejected evidence
or acceptance gate was removed.

## CI and remaining scope

The preceding exact [ca49 run39953625341](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39953625341)
is terminal: GCC, Clang, Android, Vulkan-host and sanitizers pass. MSVC explicitly
exceeds its unchanged 15-minute job cap: 100 s configure, 622 s successful build,
142 s partial selection, **118/149 passed**. Test119 starts without a result;
no hang/assertion failure is established. Report UI and APK resolver are not
reached. This is not a green head or evidence about the newer facing fix.
No rerun, CI redesign, deadline or coverage change was made.

[receipt.json](receipt.json) records the compact offline/native/source identities,
all paired PNG hashes and raw evidence references; no binaries/images/private
saves are committed. Phone HUD placement/rotation, natural Keeper/full earned
journey/recovery, warm performance, PR conflicts and the Debug route deadline
remain open. Avoid another animation tuning loop: the next independent desktop
feature candidate is the approved fixed internal/output benchmark profile seam;
inspect its current implementation before choosing a change.
