# Optimized chapter route baseline and physical ground check

The ground-material checkpoint is `e38a08c5d1d9ff098273cfd621bbb9aa721d939d`.
Its [six CI jobs passed](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/38453146945),
including all seven requested traversal CTests. This successor adds an explicitly
enabled **optimized Release** measurement capability. Ordinary Release retains
its packaged-asset requirement and rejects measurement inputs; the option is OFF
by default. No gameplay, quality, shader limits, actor/layer ceiling or source
master changed. **Normal static-region residency saving remains ZERO.**

## Actual matched desktop results

All three completed runs below used the same optimized executable:
`a6802fcecc1b6374e597458f240b9d96d2b52ac8a1b95e45b344be8044238328`.
Their actual source-input hashes and settings are in the individual launch and
capture receipts. They predate the immediate-present timestamp correction; they
are not falsely relabelled as a later rebuilt executable.

The route runs for about83 seconds after staged Keeper death: landing sweep,
fresh lantern raise/Kit proof, outbound bends/lookout, return, descent, lower
sweep and second ascent. It advances shared60Hz simulation from wall time with
four ticks maximum per render and the existing150-second route/180-second process
deadline. Initial and final native RT images were opened. This is automated
traversal evidence, not fresh combat, a fully inspected moving gallery, owner
feel or chapter completion.

Unchanged settings:960x540,100% scale, CURRENT shadows, High optical/fire/water,
dust OFF, actual Shipping shaders with zero Diagnostic buffer IO. Every retained
frame joins an exact submission/epoch/generation/tick and successful completed
presentation. Client visibility is checked; foreground is recorded independently.

| Actual backend / window | Warm median / p99 / max wall interval | Warm observed throughput | Device-local / host-visible bytes |
| --- | --- | --- | --- |
| Pipeline, authorized foreground |6.068 /10.259 /15.267ms|156.19Hz|371,995,856 /48,084,816|
| Pipeline, nonactivating |6.071 /11.170 /48.547ms|154.96Hz|371,995,856 /48,084,816|
| QueryCompute, nonactivating |6.784 /18.143 /26.597ms|125.24Hz|371,995,856 /48,084,304|

Memory stays constant before/during/after every route phase. These are real live
allocation classifications, which may overlap on UMA; they are not summed into
a fictitious total. Existing Keeper retirement had already occurred before this
route and does not establish static-region streaming.

**Timing erratum:** schema2 raw CSV's `present_return_interval_ns` was sampled
after `RenderFrame` and its identity join, rather than immediately at
`vkQueuePresentKHR` return. The table therefore reports **post-frame observation
intervals**, including CPU/WSI waits and overhead, not GPU-only timings or exact
scanout/FPS. Original raw files and original analyses remain unchanged;
`analysis-v3.json` records the corrected interpretation. Its warmup excludes the
initial interval. Current schema3 source timestamps actual successful present
returns, explicitly marks the first interval unavailable, and records aggregate
machine CPU busy time once per second. It does not identify other tasks or infer
GPU load. Physical verification of that corrected clock remains pending.

Earlier half-second stalls remain in [retained failures](windows/retained-failures/).
The owning ImageAcquire stage contains the long wait while GPU samples are only
about2–4ms. Neither matched Pipeline run reproduced those500ms stalls, so **focus
is not established as their cause**. The owner's report of earlier100% CPU use
makes contention plausible; no contemporaneous machine-load journal exists to
prove it. A directory historically named `r01-query` actually selected Pipeline;
the backend receipt, not the directory name, is authoritative.

The completed nonactivating measurements used `SWP_NOACTIVATE` but raised their
window. The current source only raises it for explicit foreground authorization.
The first corrected nonactivating attempt failed closed on visible Z-order
overlap; it is retained and is not a pacing pass. Another focused comparison is
awaiting owner permission. No covered/hidden window result becomes playable FPS.

## Phone ground evidence

The owner-authorized separate Debug app on **SM-S948B** was updated to
APK `1a00b34d61ce63423748c5b05434ae1a7744cbb55d196c6be5fa40e82089be6b`
(183,170,474 bytes), with the same development signer/versionCode10, app data
preserved and release app untouched. Installed bytes were verified once.
[The sealed receipt](android-ground/receipt.json) records all four actual native
views and hashes: landing north and second bend on both RT backends. All four
screenshots were opened; ground/trail/tree/lantern rendering is present.

Original saved40% scale and other settings stayed unchanged:576x1192 into
1440x2980, CURRENT shadows, Mobile optical/fire/water, Diagnostic instrumentation.
Last owning GPU samples: landing55.919/54.698ms and second bend139.624/166.012ms
for Pipeline/Query respectively. These single frozen-view samples are **not
sustained FPS or acceptable mobile performance**. Debug was stopped and original
preferences byte-restored before the owner removed the phone. No further ADB
work is authorized until it returns.

## Checks and remaining gate

Default Windows Debug and Release applications build. Eight affected CTests pass
in each: launch/asset fallback/first-present interval, scene record evidence,
GPU work leases, scene preflight, static-owner cutover, resource inventory,
rescue integration and authored-terrain journey. Negative coverage includes
ordinary Release refusal, invalid/duplicate/unsupported launch combinations,
missing packaged assets, absent/backward timestamps and first-interval exclusion.
No deadline/assertion was weakened. Exact successor CI is separate from e38.

Android four-ABI Debug native build and lint pass against this source. The
unchanged Java unit-test task succeeds as UP-TO-DATE; no fresh execution is
claimed. APK closed-runtime inventory/bytes and exact stripped-versus-packaged
ARM64 shader containment pass, with external SPIR-V validation. The default
Windows Release bundle controls/registration also pass2/2. The first Android
build attempt hit a sandbox Gradle-cache lock denial; the authorized cache retry
passes and both logs remain. [Current build/input hashes](current-build-receipt.json)
identify these pre-commit binaries; they were not installed on the removed phone.

The next completion milestone remains **ordinary two-way static-region GPU
loading and retirement**, with safe readiness, stale/failure rejection, coarse
terrain/landmark and all relevant shadow/secondary-ray retention, return reload,
actual transition overlap/savings and sustained optimized route measurements.
Current/Higher and saved Lower migration on the current phone candidate, full
phone route/lifecycle and owner visual/audio/haptic acceptance remain pending.
Easy Tree/canopy and mist28 stay deferred. Audio/haptic manual revalidation for
this measurement/material slice: **NO**; existing chapter cue acceptance is open.
No merge or release.

Next owner route: tomb/guards → Keeper/claim → rope → fresh lantern raise/Kit →
both bends/lookout → return/descent → second ascent. Report ground/tree balance,
equipment ownership, any missing contributor or loading hitch.
