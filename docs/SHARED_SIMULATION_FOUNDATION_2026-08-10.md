# Shared deterministic simulation foundation

Date: 2026-08-10 to 2026-08-11
Baseline commit: `98e684c984e4e0f4ca19696baae961a9769a5c2a`
Branch: `codex/shared-simulation-foundation`

## Purpose

Milestone 1 replaces the duplicated Windows and Android gameplay orchestration with one deterministic, fixed-step simulation. It deliberately preserves Showcase Alpha `0.1.3-alpha.1`, the existing renderer and shader ABI, one active skinned enemy, native `vkCmdTraceRaysKHR` presentation, phone-safe `rayQueryEXT`, strict Android ASTC, one frame in flight, and honest `rtScene.presented` reporting.

No new enemy, shader, visual effect, asset, gameplay thread, ECS, renderer rewrite, or fallback renderer is part of this change.

## Previous ownership

`DiagnosticWindow.cpp` and `android_probe_bridge.cpp` each independently owned player movement, collision, walk animation state, lantern progression, encounter selection, skeleton and lich actions, vitality, death/retry, finale sequencing, footsteps, semantic audio decisions, and assembly of renderer inputs. Android also allowed JNI to mutate fields in `gSwapchainContext` while the render thread accessed the same object outside the JNI mutex, and Android movement advanced by a fixed amount per rendered frame.

## New ownership boundary

The intended flow is:

```text
platform input and lifecycle
  -> coherent InputSnapshot publication
  -> 60 Hz GameSimulation on the existing render/application thread
  -> immutable SimulationSnapshot + bounded GameplayEvent queue
  -> renderer, audio, haptics, diagnostics and native UI adapters
```

The shared simulation owns gameplay pose, normalized movement and collision, walk state, lantern state, encounter state, `SwordCombat`, lich progression, vitality, retry state, finale progression, and semantic event creation. Platform code retains Win32/Android lifecycle, input collection, surfaces and swapchains, native menus, audio playback, vibration playback, benchmarking presentation, and RT presentation.

## Input mailbox and memory ordering

Android publishes complete trivially-copyable input snapshots through two POD slots. A publication uses an unpublished slot, obtains exclusive slot ownership, copies the complete snapshot, releases slot ownership, and then release-stores the published token. A reader acquire-loads the token, pins that slot, verifies that the token did not change, copies the snapshot, and releases its pin.

The per-slot ownership handshake matters: release/acquire on only a published index is insufficient because a fast writer can publish twice and begin overwriting the original slot while a slow reader is still copying it. Continuous input may coalesce, while attack, reset, and retry use monotonic counters so repeated edges remain observable.

## Fixed-step rules

- Fixed simulation delta: `1 / 60` second.
- Maximum render-frame contribution: 100 ms.
- Maximum catch-up work: eight ticks per render advance.
- Invalid, negative, or excessive contributions increment an overrun diagnostic.
- Unsupported accumulated excess is discarded deterministically while retaining only a sub-tick remainder.
- Pause, lifecycle loss/resume, route reset, retry, and checkpoint import clear the accumulator.
- Two-axis movement is normalized before applying the shared movement speed and route collision.
- Rendering consumes the latest completed snapshot; this milestone adds no interpolation.

## Event queue

Gameplay events use stable player, skeleton, and lich entity IDs and monotonically increasing event sequences. The fixed-capacity queue holds 64 events without per-tick allocation. Overflow is explicit, increments a diagnostic, and fails focused deterministic coverage instead of overwriting an unrelated event. Platform adapters drain events in order and choose clips, spatial gains, SoundPool/XAudio playback, and vibration effects.

## Renderer migration boundary

`PresentableTinyRtScene` and `RtSceneFrameInputs` remain intact. One shared adapter copies a `SimulationSnapshot` into the existing renderer snapshot, including the exact lantern-strength multiplication and vitality damage-flash override. The raygen source and embedded SPIR-V are not changed.

Deterministic checkpoints continue to import the existing `BuildShowcaseCheckpointState` result directly and then freeze simulation during the twelve settling frames. They are not reconstructed through new 60 Hz ticks because that would change historical floating-point state and capture hashes.

## Pre-edit baseline

The canonical Host foundation gate passed on 2026-08-11 in `reports/foundation-runs/run-20260811-171121`:

- Windows Debug and Release: build pass; 7/7 existing CTests pass in each configuration.
- Deterministic Windows RT captures: 12/12 generated at 960 x 540 with honest presentation and fixed animation time.
- Shader staleness: compiled words exactly match the embedded raygen include.
- Android: clean Debug, unsigned Release, and `lintRelease` pass.
- Package, licence, release-identity, negative-safety, and evidence-hash gates pass.
- ADB reported zero devices, so the `SM-S948B` Full/device gate was not run.

## Tests added

Three focused host test targets are now part of both Debug and Release CTest suites:

- `horde_simulation_timing_tests`: exact 60 Hz stepping, frame clamping, catch-up bounds, deterministic excess discard, pause/reset timing, normalized diagonal movement, and direct-versus-mailbox parity.
- `horde_simulation_gameplay_tests`: checkpoint import, reset/retry while paused, stable entity selection, combat serialization, semantic event order, event overflow diagnostics, and snapshot parity at authored route points.
- `horde_input_mailbox_stress`: one serialized writer and concurrent readers stress coherent snapshot publication while preserving monotonic command counters.

All three pass in Debug and Release. The complete suite is now 10/10 in each configuration; the seven pre-existing combat, showcase, audio, character, overlay, and benchmark tests remain green.

## Validation performed after migration

The canonical Host foundation gate passed on 2026-08-11 in `reports/foundation-runs/run-20260811-175521` in 233.9 seconds:

- Windows Debug: fresh configure/build and 10/10 CTests pass in 22.11 seconds.
- Windows Release: fresh configure/build and 10/10 CTests pass in 10.85 seconds.
- Windows RT capture: all twelve 960 x 540 PNGs are byte-for-byte SHA-256 identical to the pre-edit baseline. Every capture remains scene-only, overlay-free, fixed-time, and honestly presented from an RT swapchain frame.
- Shader staleness: `minimal.rgen` remains unchanged; source SHA-256 `152819128a56f1ca31bd82adefcbd38a4a49eb55259d69d6e00d5605d714bb95`, compiled SPIR-V SHA-256 `440ae38774b95e33b078bdb1acdd475033403f3bbdb13a8c031e1332b660a0bc`, and embedded include SHA-256 `59bf3ce8503c772d96bdc65cec70e6917090062df5cbef4170805a2e1fd8cdf5`. The embedded words exactly match a fresh compilation.
- Android: clean `assembleDebug`, validation-unsigned `assembleRelease`, and `lintRelease` pass for all four ABIs. The validation APK SHA-256 is `3ec441d4d6863cf89fdff121ee69212e13adf2c8a8ba387dd7611f4927f8fb44`.
- Package identity remains `com.samfa12.hordelanternrt`, version code 4, version name `0.1.3-alpha.1`. Licence, package-layout, negative-safety, evidence-hash, and strict validation gates pass.
- `git diff --check`, `git lfs fsck`, the standalone raygen check, and the Vulkan-disabled host-only configure/build/CTest lane pass. The Release executable contains no source-checkout asset fallback path.

The final twelve-capture run recorded an overall median of 6.055700 ms and mean of 6.682708 ms, compared with the pre-edit 6.056650 ms median and 6.776201 ms mean. This is no material host performance regression; it is not phone evidence. New diagnostics expose simulation tick count, accumulator, overrun count, event queue depth/high-water/overflow, input publication sequence, and consumed command sequences.

ADB reported zero connected devices. The Full `SM-S948B` gate, physical touch feel, haptics, perceived spatial audio, lifecycle handling, and current phone performance were therefore not run or claimed. No install, signing, merge, or publication was performed.

## Known limitations

- The published renderer still supports one active CPU-skinned/refit enemy and one enemy TLAS slot.
- No visual interpolation is introduced.
- The simulation continues to compose the existing bounded skeleton and lich authored behaviours; combat redesign is deferred.
- Physical touch feel, haptic distinction, audio perception, camera comfort, lifecycle behaviour, and phone performance require a connected-device and owner pass.

## Next smallest milestone

Extract renderer resource/character-slot seams and add GPU timing without adding a second enemy.
