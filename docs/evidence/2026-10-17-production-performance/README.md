# Production-shader performance loop

This checkpoint enables measurements of the normal production shader bank. It does not adopt a rendering optimization or establish 30 FPS headroom. Source starts at `28cf707f1558deffcf7a6a1247f5f3d21c11e56a`; earlier inverse-transform and torch-transition measurements used private shader artifacts and remain historical evidence.

## Repeatable control

Build Android Debug with `:app:assembleDebug :app:lintDebug -PhordeRtInstrumentationOverride=Shipping -PhordeRtDielectricQualityOverride=Mobile -PhordeRtRouteTiming=true -PhordeFrameStageProbe=false -PhordeDispatchClockProbe=false`. Do not supply a private region-page root. Native Debug retains its existing `-O2`. This is development instrumentation around production shaders, not a signed Release performance claim.

`chapter-performance-route` reuses the existing collision-resolved out-and-back input schedule: exterior landing hold, descent, lower hold, passage hold, return and second ascent hold. It starts only from the existing admitted reunion checkpoint, waits for current RT presentation, uses ordinary input and shared simulation time, and retains the original 30-second arming, 120-second run and two-second presentation drain limits. No image readbacks occur during timing. Ordinary residency policy remains active. Zero residency saving is an eligible observation; unfinished restoration is not. The distinct streamed-comparison scenario still requires a positive unload/restore pair.

`keeper-torch-transition` remains available for the live room reveal. Neither route is a complete combat, sustained-session or touch-response acceptance test. The strict analyzer joins every completed submission to its actual Android image-presentation timestamp and rejects unresolved, missing, duplicate or discontinuous ownership.

Current phone settings are preserved: 40% dimensions (576×1188 internal, 1440×2971 swapchain), CURRENT shadows, water 1, fire 0, glass off, mist on and dust 1. The saved cap 30 applies to preview policy and does not establish normal-play 30 FPS pacing. SM-S948B/Adreno 840; record each backend separately. Temperature, charging and firmware/driver identities accompany each accepted comparison. GPU duration is distinct from actual display intervals.

## Recorder validation

- Four affected CTests passed in both native Debug and Release, including authored-terrain out-and-back at the original deadlines.
- All 15 strict display-analyzer tests passed, including the new scenario identity and incomplete-tail rejection.
- Android Debug assembly and lint passed.
- Four actual Gradle negative configurations rejected missing Shipping/Mobile policy, malformed booleans, conflicting old/new properties and stage profiling without a private bank.
- Both fresh eight-module shader-bank checks passed with unchanged budgets. A sandbox temporary-output compiler failure was retained; the identical QueryCompute check passed in the normal compiler environment.

Raw APKs, ledgers, logs, temperature samples and full-size images remain in the ignored local `reports/production-performance-2026-10-17-r01/` directory. The pre-existing Debug APK (`f13dda30…4fa527`) and byte-identical saved preferences are preserved there by reference. No release application, app data, rendering quality or production residency defaults were changed.

## One experiment: Opaque inverse transform

**Not adopted. The trial shader, generated artifacts and trial-only test edits have been restored to the benchmark checkpoint.** There is no demonstrated meaningful performance gain. No second optimization is started at this checkpoint.

The single change replaces `transpose(inverse(objectToWorld))` with the transpose of the committed hardware-query world-to-object matrix, only for Opaque variants. Generic keeps its existing calculation. All eight Generic SPIR-V modules are byte-identical; all eight Opaque modules change and pass the original limits. Shipping/Mobile Pipeline grows 576,832→577,312 B and QueryCompute 578,036→578,516 B. This normal bank differs from the earlier private r10 inverse experiment; its reported gain does not transfer automatically. The budget file remains `56936479b2d202dbdb9393ba94b66a1b0bc12d490b700d8af4c63de3460e53bf`.

Runtime source base is exactly `3252820f6f443d127b128840621ed8846282172f`. The baseline APK is `a49955a2638ae5bf0fc5e3ddc8bdf83c43a9335e5ada64f231503e66c95a8d6c` (183,557,978 B). The trial APK is `fbfdf02ac41bc18924f890601e7b5f8769dfa1dbbeb44aa8da3cbfc97ef4e2a7` (183,558,938 B), from that base plus the archived patch `647187ec43e8c80ccdd6d111c5dda1bb4915440be5e2e2492125d28e769a90bb`. All 169 packaged asset payloads match exactly; the development signer/version match the pre-existing app. The [receipt](experiment-receipt.json), [all sixteen module identities](module-comparison.json) and [per-run results](results.json) identify the evidence without placing APKs, full captures or ledgers in Git.

Six completed production-bank runs join **4,164 completed frames** to actual Android presentations. Each lasts about 68–70 seconds. The table reports the first control and the later cooldown-bounded candidate repeat. **These are descriptive observations, not a causal gain estimate:** the control starts cooler, and a thermally matched reverse-order control was interrupted before route input by `NotificationShade` covering the game. Its foreground guard stopped the run. A final matched control remains pending owner availability.

| Fixed view | Control GPU median | Trial GPU median | Control / trial display median |
| --- | ---: | ---: | ---: |
| Pipeline exterior landing | 135.825 ms | 134.586 ms | 150.000 / 150.000 ms |
| Pipeline lower tomb | 71.124 ms | 70.739 ms | 83.333 / 83.333 ms |
| Pipeline passage | 68.607 ms | 74.349 ms | 75.000 / 83.333 ms |
| Query exterior landing | 130.311 ms | 131.077 ms | 150.000 / 150.000 ms |
| Query lower tomb | 67.174 ms | 67.245 ms | 75.000 / 75.000 ms |
| Query passage | 67.745 ms | 67.929 ms | 75.000 / 83.333 ms |

The earlier candidate repeats likewise do not show a useful improvement: Pipeline exterior/lower medians 134.918/70.334 ms, Query 130.807/67.308 ms. Whole-route actual display median/p95 stays approximately 87.5/154.167 ms for Pipeline and 83.333/150 ms for Query. No successful run has a display interval above 250 ms; every interval exceeds 33.333 ms. The per-view p95 tails and repeat ascent are retained in the JSON, including the slower trial passage/ascent observations. GPU samples bracket the complete RT frame, **not an isolated shader-dispatch attribution**. No stage/clock bank, sparse shader timer or reciprocal-GPU FPS is used.

Control route-start AP/BAT/SKIN is 45.1/33.3/37.6°C (Pipeline), 43.1/32.8/36.8°C (Query), framework 0. Later candidate repeats start at 44.9/36.7/39.1°C and 45.8/36.1/39.0°C, framework 0, then reach framework 1. These differences prevent attribution of small timing changes to the code. Separate screenshot work reached framework 2 and was followed by passive cooldown with the game stopped. The repeat admission requires framework 0, AP≤39, BAT≤35.5 and SKIN≤37°C **before preparation**; temperatures are then recorded before and throughout the route. GPU clock access is denied; no governor, charging, brightness, resolution or quality policy was changed. This is not a controlled power/clock or sustained 30 FPS result.

One hotter Query repeat is **ineligible**: all 691 submissions completed, but only 690 actual display rows arrived. Present ID 1, the first armed image, remained unresolved despite successful queries; the unchanged two-second terminal drain rejected it. The hardware/compositor reason that image was not reported is not established. It is not evidence of a missing final GPU completion, nor a valid timing sample. The [failed join receipt](failed-display-join.json) preserves the exact counters, pending identity and thermals. A new focused analyzer regression rejects both the unresolved first image and attempts to hide it by clearing the pending list; all 16 strict analyzer tests pass. No limit or assertion is weakened.

## Correctness, loading and checkpoint

Eight frozen phone image pairs have matching camera/player, animation, light/material settings and exact live module identities. Six pairs are pixel-identical. Both second-bend pairs differ in only 50 of 4,492,800 pixels, by at most 1/255 in a channel. Actual images were opened and inspected, including outdoor terrain/trees/held lantern and the cleared Keeper-room floor. [Camera/module/image hashes](visual-identities.json) and [pixel differences](parity.json) are recorded. Windows Debug and Release compile; four separately inspected scoped 960×540 RT-storage-image captures pass on RTX 5050 Laptop, both backends, using normal Diagnostic/High variants. Windows captures establish correctness, not phone performance or a matched Windows timing gain.

All successful routes unload/restore one ordinary static detail owner, omit at most 84,219 triangles, report 8,017,664 B (7.65 MiB) saved owner bytes and restore their starting allocation totals without readiness waits. This is fresh **production-bank** observation, distinct from the historical private 13.59 MiB result. The shader trial does not change it. Pipeline starts/returns at 160,793,152 device-local and 48,085,184 host-visible accounted bytes; Query is 512 B lower in each classification. Device-local maxima are 262,915,584/262,915,072 B and the owner counter's recorded peak overlap is 180,592,064 B. These overlapping UMA classifications/counters are not additive physical RAM. Broader residency, secondary-ray safety and sustained journey acceptance remain separate gates.

The experiment and restored source both pass Android four-ABI assembly, lint and Java tests. Native Debug/Release application products are restored to the normal bank; the restored Android APK is byte-identical to the archived control. The pushed recorder checkpoint's [exact-commit CI](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39002045686) passed all six jobs. No renderer optimization is committed.

**Recommendation:** do not expand the outdoor workload yet. Even the clearer indoor controls exceed the entire 33.333 ms budget, while these exterior views take about 130–136 ms of GPU time at the existing 40% dimensions. First finish the matched control, then consider the audit's exact-zero cleared-mist exit as a separately measured, output-preserving experiment. It helps cleared-room/backtracking work, not the live two-torch spike. Meaningful 30 FPS across the chapter may require larger reuse/filtering/lighting changes with explicit correctness and visual review; the current evidence does not rank their potential gains. Full live combat, long warm-session pacing, lifecycle, Current/Higher/migration and owner acceptance remain open.

For a short owner check: walk through the tomb to the two-torch reveal, defeat the Keeper, ascend, turn through the clearing, return down the rope and back into the tomb. Check lights/shadows, floor appearance, held equipment and restoration. No visual acceptance is inferred from these timings. No merge, release, asset change or outdoor expansion was performed.
