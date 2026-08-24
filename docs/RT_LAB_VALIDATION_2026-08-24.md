# Unlockable RT Lab Validation — 2026-08-24

## Final-review artifact closure

Final whole-branch review removed the two Android RT Lab-only `ui_select` / `ui_back` playback calls under a focused source-region RED/GREEN contract. The corrected clean runtime source is `e37a3bad2cfd4c91f17a642f92fab12bc459ae8a`; its rebuilt Debug APK SHA-256 is `cab1c5c7f12ae4851a65e6ae9b2774c268abd2caa8a10e568e71f1f300db8aeb`.

Debug and Release each pass all 13 CTests; Android Debug/unsigned Release/lint and shader freshness also pass. After the authorised phone reconnected, the corrected APK was installed and pulled back byte-for-byte, then passed the full authored route in `run-20260824-203215` and all nine matched workload cases in `run-20260824-203502`. The earlier `c0d3539` / `0efdb2...` runs remain historical evidence for the immediately preceding candidate and are superseded by these final-artifact runs.

Audio/haptic manual revalidation required: **NO** — the corrected Android RT Lab open/back paths are now source-proven silent, while existing non-lab menu/audio behavior is unchanged.

## Result

The post-lich RT Lab implementation passes the current automated host, Windows renderer, Android build, and exact-phone gates. The lab keeps gameplay paused while hardware-RT presentation continues, exposes truthful scene/light/workload controls, and leaves render scale and water quality under Settings.

This is a development-candidate validation record. No version was changed, no release candidate was signed or packaged, and nothing was uploaded, published, or deployed.

## Candidate identity

- Final runtime/UI source used for exact-phone evidence: `e37a3bad2cfd4c91f17a642f92fab12bc459ae8a`; the clean evidence HEAD was documentation-only `e07475409f034cec2cec28c4d25dcc0b2f8fb72a` and did not alter the APK.
- Final Debug APK SHA-256: `cab1c5c7f12ae4851a65e6ae9b2774c268abd2caa8a10e568e71f1f300db8aeb` (59,661,575 bytes).
- Pulled installed `base.apk` SHA-256: the same value, byte-for-byte.
- Embedded raygen include SHA-256: `60e33f2b81450cc8f76a07b809893dbc5473a4d2fd5bd2d0489df65c1db3f7ea`.
- Compiled raygen SPIR-V SHA-256: `ef827b71e240cfef50fad601985b0d8c03dc06c4fd40ef4116b6831c0620f19b`.
- Raw Android model code: `SM-S948B`; serial `R5GL219SZGK`; Android 16/API 36; Adreno 840; Vulkan 1.4.295.
- Final documentation commit is intentionally later than the runtime commit and does not alter the APK.

## Host and build gates

The final integrated command was:

```powershell
.\tools\run-foundation-validation.ps1 -Mode Host
```

Run `reports/foundation-runs/run-20260824-185921` passed from a clean `c0d3539` worktree:

- `tools/compile-raygen.ps1 -Check` confirmed the embedded include matches the shader.
- Fresh Windows Debug build and all 13 CTests passed in 48.55 seconds.
- Fresh Windows Release build and all 13 CTests passed in 16.65 seconds.
- Thirteen authored deterministic Windows captures completed.
- Android `assembleDebug`, unsigned `assembleRelease`, and `lintRelease` passed for the four configured ABIs.
- Negative release-safety, validation-package/licence, and evidence-hash stages passed.
- The generated validation artifacts are explicitly marked `UNPUBLISHABLE`.

After the final Android-silence review fix, the focused source-region contract passed, incremental Windows Debug and Release builds each passed all 13 CTests, Android `assembleDebug`, unsigned `assembleRelease`, and `lint` passed, and raygen freshness passed. The fix changes only two Android Java playback calls; the final exact-phone runs below bind the rebuilt APK to the corrected runtime.

NVIDIA Nsight is not installed, so register pressure was not numerically measured. Shader structure, host contracts, Windows GPU timing, and exact-phone matched timing are the available evidence.

## Renderer and UI inspection

Debug-only deterministic tuning was used for repeatable captures; it cannot grant or save the release unlock.

Windows capture families were produced with the exact Debug executable and `--capture-showcase`, combined with `--debug-rt-lab` and the relevant tuning arguments. Android used typed Debug intent injection and native state JSON. The inspected matrix covered:

- waterfall width at 25%, 100%, and 200%;
- finale roof at 0% and 100%;
- Torch, Skylight, Passage, and Staff at hue +120 degrees and intensity 200%;
- Lean, Authored, and Max workloads;
- the Android panel at its top and bottom scroll positions.

Inspection findings:

- Waterfall width changes the falling-stream intersection geometry around its authored centre on both platforms. The catchment, drain, runnel, roof slot, and collision remain fixed. Transparent/refraction continuity remained visible at all three widths.
- Roof 0% physically closes the finale opening and removes its corresponding light; roof 100% physically opens it and admits the same effective skylight. Geometry and lighting agree.
- Each selected light group changes its intended authored family while the other groups remain at their defaults. Passage sconces preserve their relative authored colours.
- Workload presets produce distinct measurements. The Windows capture manifests reported approximately 0.845 ms Lean, 1.190 ms Authored, and 1.380 ms Max average GPU RT time.
- The final Android workload layout presents full-width, minimum-48dp `LEAN`, `AUTHORED`, and `MAX` targets without the earlier clipped Authored label. Top and bottom views retain readable controls, live telemetry, `RESTORE AUTHORED`, and `BACK` over a translucent scene.
- With the Android panel open, GPU telemetry sample count advanced from 8,569 to 8,662 over one second while the UI stated that gameplay was paused. This is direct evidence of continuing RT work; source/host contracts establish the paused simulation path.
- All retained deterministic captures reported `presented: true`; the standard phone run also recorded fresh `RT frame reached Android swapchain presentation` markers before and after Home/resume.

One transient Windows access violation occurred during rapid repeated staff-matrix process churn. Default, hue-only, intensity-only, and two later combined staff retries all completed 13/13 captures, so it was not reproducible. It remains recorded as a diagnostic observation rather than silently omitted.

## Exact-phone authored route

Before every device mutation, `adb devices -l` was checked for exactly one authorised target and the raw model was re-read as `SM-S948B`.

The final untuned command was:

```powershell
.\tools\run-android-showcase-validation.ps1 -Mode Both -Scale 75 -GpuTiming Enabled -Capture -SkipBuild -SkipInstall -TimeoutSeconds 180
```

Run `reports/android-showcase-runs/run-20260824-203215` passed on the exact final APK. It retained 75% render scale, Mobile water, Authored workload, strict ASTC, honest RT presentation, the 13-waypoint replay, all 13 captures, and Home/resume surface recreation.

| Standard checkpoint | Median of three 120-frame window averages | Reference band | GPU power level |
|---|---:|---|---:|
| Opening | 16.955 ms | 50–60 FPS | 1 |
| Two-enemy combat | 19.581 ms | 50–60 FPS | 2 |
| Worst bend | 17.247 ms | 50–60 FPS | 2 |
| Skylight | 19.353 ms | 50–60 FPS | 3 |
| Green passage | 20.147 ms | 30–50 FPS | 2 |
| Lich | 34.339 ms | Below 30 FPS reference | 2 |

Android thermal status was 0 before and after; current HAL AP/BAT/SKIN moved from 36.6/31.1/33.6 C to 45.7/36.7/39.2 C, while row-level Samsung GPU thermal power level rose through 1–3. The lich result is reported as measured and is not converted into a false pass/fail threshold.

## Exact-phone matched workload comparison

The same installed APK then ran:

```powershell
.\tools\run-android-showcase-validation.ps1 -Mode Benchmark -Scale 75 -Checkpoints @() -GpuTiming Enabled -RtLabWorkloadComparison -SkipBuild -SkipInstall -TimeoutSeconds 180
```

Run `reports/android-showcase-runs/run-20260824-203502` passed all nine cases. Every saved state reported 75% scale, water quality 1 (Mobile), the requested workload 0/1/2, the expected checkpoint/zone, and `presented: true`.

| View | Lean | Authored | Max | Lean vs Authored | Max vs Authored |
|---|---:|---:|---:|---:|---:|
| Waterfall (`lantern-drop`) | 20.162 ms | 17.954 ms | 30.547 ms | +12.3% | +70.1% |
| Skylight | 17.861 ms | 23.713 ms | 28.070 ms | -24.7% | +18.4% |
| Finale roof | 13.080 ms | 18.875 ms | 21.875 ms | -30.7% | +15.9% |

Thermal status stayed 0. Current HAL AP/BAT/SKIN moved from 42.7/36.7/38.3 C to 42.2/36.6/38.2 C; the first Authored waterfall row recorded Samsung GPU thermal power level 0 and later rows recorded levels 5–6. These are matched sustained measurements on this exact device/artifact, not universal phone targets. The waterfall Lean row was 12.3% slower than Authored in this ordered run, so the evidence does not claim monotonic cost on every thermally governed sample; Skylight and Finale retained the expected ordering. The runner varies profile order per checkpoint, uses three windows for each, and does not silently alter RT, resolution, or water quality.

## Unlock, reset, and persistence acceptance

Current automation proves the following without manufacturing progress:

- Fresh isolated Debug package state began without `rt_lab_unlocked`; the ordinary release package and its progress were never cleared or modified.
- Android and Windows host contracts accept only genuine live `finaleComplete` and reject checkpoint, capture, replay, benchmark, and Debug-injection contexts.
- Debug checkpoint/tuning runs completed without creating `rt_lab_unlocked` in the Debug package preferences, checked both before and after the final standard/matched runs.
- Ordinary settings persistence preserves the independent progress key, while process/route reset restores authored tuning and leaves progress separate.
- The panel routes simulation through the existing paused input while Vulkan rendering, swapchain presentation, and telemetry continue.

The following player-flow checks remain owner-only/open for this candidate: complete the finale through normal gameplay, observe the first `RT LAB UNLOCKED` card, relaunch, use Begin Again, and use ordinary Reset Defaults while confirming the persisted lab remains available. Debug checkpoints/replay were deliberately not used as evidence for this genuine unlock. Windows persistence contracts were verified without overwriting the owner's existing INI.

After device evidence, the side-by-side Debug package was force-stopped and uninstalled. `pm path com.samfa12.hordelanternrt.debug` is empty; the existing release package remains present. This restores the phone's original no-Debug-package state without touching release progress.

## Evidence boundaries

- Automated screenshots establish deterministic geometry/lighting state and inspected UI readability, not owner artistic preference or motion feel.
- Automated workload timings establish exact-artifact/device differences under the recorded thermal context, not performance on another model or a guaranteed frame rate.
- Home/resume establishes renderer/surface recreation, not a new human touch/audio/haptic judgment.
- Genuine unlock presentation and long-term player-facing persistence remain the owner gameplay checks listed above.

Audio/haptic manual revalidation required: **NO** — the change is renderer tuning, UI, persistence, and Debug evidence automation only. It does not change sound assets, gains, cues, listener/source event-time data, spatialisation, event transport/timing, haptic routing/patterns/intensity, or player damage/death feedback.
