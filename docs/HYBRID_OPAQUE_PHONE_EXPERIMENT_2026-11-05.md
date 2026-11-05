# Physical-phone follow-up: raster primary + genuine RT shadows

5 November 2026, following Sam's explicit invitation to test his connected phone. **Both renderer modes work on this device, but this short run does not establish a mobile performance benefit.** No new APK build was needed.

Local evidence directory: `C:/Users/sam_s/Documents/Codex/2026-11-05/task/native-hybrid/opaque-room/phone-20261105`. Open `comparison.html` there for the results and paired screenshots; `results.json` contains exact numbers. The original Windows/APK deliverables and their hash manifest remain unchanged.

Device: **Samsung SM-S948B, Android17/API37, Adreno840**, Vulkan1.4.295, Qualcomm driver raw2150932499, driver build089d643e2a/Icecd8f0534, compilerE031.50.19.27. Device serial and fingerprint are in the private receipt, not the repository report. Installed existing APK SHA256 `271c789fb5bd74f19eee38a6de9969674d70c10b449883a64ca31f4468dcf747`, source `a1f51f9ef42f2733f3f329cc7febec8a599f5b2d`. On-device `sha256sum` matches the local APK exactly. Separate package `com.samfa12.hordelanternrt.debug.hybridroom`; released `com.samfa12.hordelanternrt` remains installed and was not replaced or launched by this task.

## What was compared

One frozen **actual Horde niche architectural scene**, camera(-0.4,-2.6), yaw-1.5708/pitch-0.32, animation time0, same process and retained lights. Raster primary over the complete declared opaque architecture plus genuine hardware ray-query direct-light shadows versus RT primary over exactly the same architecture with the same selected direct shading/shadows. Source counts are754,004 triangles/12 forward draws; no residual-primary ray pass in raster mode. The fixture omits actors, held props, water/glass, volumetrics, indirect light and reflections in both modes. It is not full-game/campaign acceptance.

The reference is **RT primary + RT shadows**, not the complete shipping full-RT effects stack. This comparison isolates primary visibility at the selected profile's quality. The separate full shipping RT app/mode is preserved, not replaced or retimed against a reduced-quality candidate.

50% scale: **720×1486 internal →1440×2971 native portrait surface**; linear presentation upscaling, Shipping/Mobile private shaders, current one-sample local/sky shadows, Mobile water/fire tuple, MistOff, DustLow, fixed explicit textureLOD0. Both use identical exposure, camera, lighting, dimensions, shader instrumentation and effect settings; analyzer asserts them from completed snapshots. The legacy exact-ratio `aspectPreserved` field is false in both at this odd-height half-resolution; the dimensions/settings are preserved explicitly rather than claiming that flag passed. No dynamic resolution or automatic quality reduction was introduced.

Mode changes used the real Settings→Graphics controls, locating nodes from `uiautomator` XML rather than invented coordinates. The controls correctly submit the intended shader identities after switching. While a frozen capture is active, the existing admission gate intentionally rejects mode changes; a supported isolated live inspection checkpoint was used before each switch, followed by the fixed niche capture again. Saved graphics preferences were not applied/changed. UI preparation intervals allowed cooling and contribute to the thermal caveat below.

## Measured result

Order **raster / reference / reference / raster**, four nominal20-second polling windows. GPU interval means and SurfaceFlinger displayed-image statistics have separate boundaries; they are not joined frame-by-frame.

| Window | GPU interval mean ms | Actual native displayed images/s | Present interval median / p95 ms | Thermal status before→after | GPU thermal power level before→after |
| --- | ---: | ---: | ---: | --- | --- |
| Raster0 |43.170|22.31|41.67 /50.00|2→2|1→2|
| RT reference1 |35.478|26.02|37.50 /50.00|1→2|0→5|
| RT reference2 |49.194|20.06|50.00 /58.33|2→2|3→5|
| Raster3 |44.789|20.85|50.00 /54.17|2→2|1→4|

There are **1,752 actual native-surface presentations** across the four trimmed display windows, **1,560 hardware GPU timestamp samples** between their report boundaries, and17 unique sparse completed-frame snapshots with owning shader/submission/dimension metadata. All sparse snapshots have successful presentation, valid GPU timing, zero GPU timer errors/unavailable results and zero steady skin/dynamic upload/BLAS-refit/TLAS-update work. The sparse snapshots are not a complete per-frame JSONL ledger, so no per-frame GPU p95 is claimed.

The two RT-reference GPU interval means differ **38.7%**. Battery temperatures span38.2–42.5°C across window endpoints; current HAL skin temperatures39.8–44.0°C; USB charging remained connected. Thermal status and GPU thermal power level change within/between windows. The GPU clock sysfs read is denied to shell; no root, permissions, thermal override or security change was attempted. GPU thermal power-level numbers are recorded raw, not converted to frequency. These conditions prevent a causal percentage speedup/regression claim. Closing raster GPU mean also drifts3.75%, and its displayed rate falls. No run reaches a median present interval within33.333ms/30FPS,20ms/50FPS or16.667ms/60FPS in this fixed profile. This does not establish campaign FPS or sustained thermal stability.

The correct decision is **inconclusive performance attribution; no repeatable mobile gain established**. Keep the laptop negative result and this phone thermal limitation visible. Do not pool these windows to manufacture a speedup or continue indefinite unchanged reruns. A next meaningful experiment needs separate forward-draw/copy GPU stage timestamps and a specified controlled physical-phone warm-state protocol before changing geometry submission/culling. Dynamic gameplay/actors need their own rendering integration.

## Appearance and actual-device acceptance

Native screenshots visibly show the genuine Horde niche materials and shadows in both modes. Cropped surface RGB mean absolute difference is0.3058 on0–255;16.44% of pixels differ by at least one channel value. Paired stills look close on inspection, with fine edge/shadow differences. This is not pixel equivalence, material-wide acceptance or motion/aliasing/disocclusion validation. LOD0 plus50% upscaling is common to both. Both profiles intentionally exclude the effects listed above.

Observed: install/hash match, startup to native presented fixture, both real mode controls, physical raster and RT-primary output, fixed-checkpoint capture and bounded timing. App-scoped final capture logs have no fatal exception, fatal signal, device-loss marker or unexpected native render-loop termination. Initial startup logs do include Adreno shader-compilation failures from capability/probe activity alongside successful subsequent scene initialization; no claim of warning-free Android Vulkan validation is made. Khronos validation layers were not enabled on this phone. There is no gameplay/touch-combat, audio/haptic feel, rotation, lifecycle fault or sustained-session certification. Audio/haptic manual revalidation required: **NO** for this unchanged renderer-only APK test; no audio system or semantic input change.

The owned lab process was force-stopped and Home requested after the closing control so it could cool. The lab remains installed for Sam. No data clear/uninstall, released-app save/settings access, signing/security setting change, merge, publication or duplicate worktree/build.

## Method and reproduction

Existing APK installed with `adb -s <verified-connected-serial> install --user 0 <existing-lab.apk>`. User0 was verified explicitly, avoiding managed-profile access. Campaign namespace `hybrid-phone-20261105` belongs only to the separate lab app.

Native deterministic capture intent:

```powershell
adb -s <serial> shell am start --user 0 --activity-single-top -n com.samfa12.hordelanternrt.debug.hybridroom/com.samfa12.hordelanternrt.MainActivity --es campaign_test_namespace hybrid-phone-20261105 --es horde.debug.checkpoint chapter-niche-arch --ez horde.debug.capture true --ei horde.debug.scale 50 --ez horde.debug.autostart true --ez horde.debug.overlay false --ez horde.debug.gpu_timing true
```

The native capability JSON is read through the lab's debuggable `run-as` interface; cumulative GPU sample count `N` and mean `M` yield interval mean `(N2*M2-N1*M1)/(N2-N1)`. The file publishes roughly every120 rendered frames, so the first/last report boundaries differ from the outer20-second display polling window. Sparse latest GPU samples are retained as context, not a percentile distribution. No Android View/gfxinfo statistic is misrepresented as Vulkan frame time.

The exact observed lab SurfaceView BLAST layer is selected from `dumpsys SurfaceFlinger --list`. Polling `dumpsys SurfaceFlinger --latency '<exact-layer>'` every~0.5 seconds retains the native surface's presentation history. The second timestamp column is actual present time in AOSP's [FrameTracker source](https://android.googlesource.com/platform/frameworks/native/+/refs/heads/main/services/surfaceflinger/FrameTracker.cpp); zero/pending values and duplicate timestamps are removed. History older than the newest entry in the first poll is excluded. Rate is `(frames-1)/(last-first)` and tails are from adjacent actual presentation intervals. There is no GPU→display frame-ID join or latency-to-input claim. Polling overhead is present in each run.

`phone_capture.py`, `phone_mode.py`, `analyse_phone.py` and all raw JSON/SurfaceFlinger rows, scoped logs, UI XML, screenshots and install receipt are retained locally. `physical-phone-evidence.zip` and `artifact-manifest.json` seal the successful cohort. The failed initial sysfs-preflight directory and startup/UI diagnostics remain outside the timed cohort; they are not counted as completed runs.
