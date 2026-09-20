# Validation and evidence guide

Use the section relevant to the requested change. This guide is not a requirement to run every lane for every edit. Root [AGENTS.md](../AGENTS.md) defines working permissions; [engine contracts](AGENT_ENGINE_CONTRACTS.md) explain the subsystem invariants.

## Choose the smallest sufficient check

| Change or claim | Appropriate starting point |
| --- | --- |
| Documentation/instruction-only edit | Review the diff, relative links/anchors, command references and preserved contracts. No game build, device install, audio check or release packaging is needed solely for prose. |
| Shared gameplay/input/event logic | Relevant host tests; broaden to affected Windows/Android behaviour when platform integration changes. |
| Shader/renderer/material change | Shader regeneration/freshness, relevant Vulkan-enabled host contracts and image inspection, then affected-device presentation/route evidence. |
| Android UI/lifecycle change | Relevant build/lint/contracts and the affected interaction on an authorised test device. A clean build is not a touch/lifecycle pass. |
| Runtime asset change | Relevant importer/texture/budget/licence/package contracts and visual inspection through the real RT path. |
| Release candidate or broad cross-platform change | Applicable complete foundation/package/device gates and exact-artifact provenance; use the release task's acceptance scope. |

Run broader checks when coupling or a failure warrants them. After fixing a change-related failure, rerun the affected check; repeat a full lane only when the fix invalidates its evidence. Do not weaken an assertion, skip a failing gate, or change rendering quality merely to manufacture a pass. Separate pre-existing failures and environment blockers from regressions introduced by the task.

## Host and build commands

Run from the repository root unless stated otherwise. Check current script parameters/presets when the tooling itself has changed; machine-specific SDK paths and historical test counts are not permanent requirements.

The portable, Vulkan-disabled CI lane is defined in `.github/workflows/shared-simulation-host.yml`:

```sh
cmake -S . -B build/host-ci -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DHORDE_RT_BUILD_VULKAN_TARGETS=OFF
cmake --build build/host-ci --config Release --parallel 2
ctest --test-dir build/host-ci --build-config Release --output-on-failure
```

This covers non-hardware shared tests, not Vulkan presentation or phone behaviour. Use CTest selection for an affected subset when appropriate; report the actual selection and result instead of hard-coding a test count.

The same workflow also runs on pushes to `codex/horde-1.6.1-engineering-pass`, so
PR merge conflicts cannot suppress all current-source compiler coverage. Its
`player-vulkan-host` lane enables Vulkan targets with the Ubuntu development
package and builds/runs the focused player contracts, actual skinned-player smoke
and malformed/reordered GLB fixtures. The tests do not create a Vulkan device:
SDK-enabled host compilation is **not** physical RT presentation, backend image
parity or Android-device acceptance. Keep the portable lane as separate coverage.

Windows configure/build/test presets are in `CMakePresets.json`:

```powershell
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug
ctest --preset windows-x64-debug
```

The corresponding Release preset is `windows-x64-release`. Shader regeneration uses `tools/compile-raygen.ps1`. An Android build-only command, from `android/`, is `.\gradlew.bat assembleDebug --console=plain`; adding `installDebug` changes a device and is a separate action.

For a requested broad candidate gate, `tools/run-foundation-validation.ps1 -Mode Host` runs the foundation's host-side programme; it is not the lightweight portable CI lane. `-Mode Full` includes device work. Inspect current parameters and choose the intended device explicitly rather than trusting a historical default serial. The runner writes evidence beneath ignored `reports/` and labels its staged artefacts as unpublishable/unsigned; its success does not authorise release publication.

## Android device evidence

Use `tools/run-android-showcase-validation.ps1` after meaningful Android renderer or gameplay-route changes when an authorised device is available. `ANDROID_SHOWCASE_AUTOMATION_2026-07-17.md` explains checkpoints/replay; inspect the current runner rather than freezing old checkpoint counts into new instructions. Preserve coverage of the two-enemy encounter where the changed route requires it.

For a requested device-validation task, use the already-authorised connected test device without repeatedly asking for the same permission. Identify the exact model/serial before install or automation; do not target an arbitrary connected phone or clear app data without authorisation. If ADB exposes no suitable device, continue applicable safe host checks and report the device lane as not run.

Bind evidence to source commit, build type, exact APK/ZIP hash, device model/driver, settings and workload. Distinguish build/package checks, installation/pullback matching, capability detection, honest RT presentation, deterministic feature captures, lifecycle/touch checks, sustained timing and owner feedback. One category does not imply the others; an unsigned Debug capture does not certify a signed Release APK.

Whenever new Android evidence appears, update `ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md` in the same task. Preserve prior entries and the exact model code; distinguish local confirmation, user report with screenshots, user report alone, vendor/SoC inference and unverified candidates. Do not infer working support from a chipset or another model, or imply that an older build validates a newer candidate.

## Performance evidence

Report median-derived bands against 16.667 ms (60 FPS), 20.000 ms (50 FPS) and 33.333 ms (30 FPS), together with tail timing where available, render resolution/scale, workload, checkpoint order, temperature, Android thermal status and GPU thermal power level when available. Crossing 20 ms alone does not fail a candidate.

Investigate matched regressions above 15%, growing resource use, and unexplained workload changes before accepting them. Match build type, scene/settings and thermal state; do not compare an instrumented Debug capture with a cooled Release run as though they were equivalent. Never silently lower resolution, quality or RT work to obtain a favourable number.

Sustained warm behaviour is the primary player-facing evidence. Fresh-process/cooled results are useful context, not substitutes. Historical `SM-S948B` runs with stable memory/thread counts and thermal clock reductions explain why both workload and thermal evidence matter; do not copy their timings into a newer candidate's result. Image/correctness acceptance is not sustained-performance acceptance.

## Manual audio and haptic validation

For implementation milestones, state `Audio/haptic manual revalidation required: YES/NO` with the change-based reason; default **NO**. A documentation-only task does not need a milestone or an owner check.

Require **YES** only when the change can affect event-time listener/source data or identity, spatialisation/attenuation/pan/obstruction, playback backend/gain/cues/assets, event transport/timing, haptic routing/cues/patterns/intensity, or player damage/death feedback. Relevant automated contracts do not prove that sound or vibration feels correct to the owner.

Unrelated RT, visuals, UI, assets, build/packaging, telemetry, documentation, AI or animation changes do not trigger an owner check when automated contracts pass and semantic inputs are unchanged. Assess the actual impact: a new audible asset or changed event timing is not exempt merely because the task is labelled an asset or animation change. Do not reuse a past owner approval as a current exact-candidate pass or ask the owner to recheck unrelated work.

## Release and historical evidence

The [README](../README.md) package summary, `release/` metadata, current release-version policy and the relevant exact-version report identify published evidence. Search only the relevant sections of `PROJECT_MEMORY.md` and `PROJECT_DECISIONS.md`; their rolling summaries and old plans may describe earlier states. Resolve conflicts using current source/artifact identity and dated evidence, and correct affected claims rather than silently treating plans as shipped features.

Existing reports such as `SHOWCASE_ALPHA_1_6_0_RELEASE_VALIDATION_2026-08-30.md` retain their source IDs, hashes, test counts and limitations. Those details belong in the evidence record rather than an always-loaded instruction file. A past pass does not certify a later commit or device driver.

Signing, public uploads and release identity changes require an authorised release task. Follow [OWNER_RELEASE_SAFETY_CHECKLIST.md](OWNER_RELEASE_SAFETY_CHECKLIST.md): preserve the stable Android certificate; never expose signing material; never inspect/copy recovery material or mark owner-only backup/recovery checks complete. A configured in-memory signing handoff is allowed only as documented for an authorised release and is not proof of independent backup. Asset licences must be recorded before shipping.

## Current 1.6.1 programme sequencing

The owner's 2026-09-13 update supersedes older task briefs that demand a full Host/device programme after each implementation slice. Use relevant targeted checks during development, broaden only when coupling or failures warrant it, and reserve the comprehensive Windows/Android/cross-device/release matrix for the complete final candidate. S24/S25 compatibility, adaptive Pocket Chordsmith music and cross-platform player reporting are required before that final pass. This changes validation scheduling, not any RT, correctness, exact-artifact or final acceptance requirement.

Existing 1.6.1 evidence remains tied to its original source/build: `ENGINEERING_1_6_1_ANDROID_OBSERVATION_BASELINE_2026-09-05.md` is Diagnostic/Debug evidence, not Shipping/Release performance. Preserve matched A/B build, pipeline, workload, scale and thermal identity. A window-average median must not be relabelled as a per-frame median. The full route exceeds the 128-sample window collector; retain all intended samples with explicit capacity/invalid-run handling.
