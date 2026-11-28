# Validation and evidence guide

For performance work, first consult the [current experiment ledger](PERFORMANCE_EXPERIMENT_LEDGER.md) for closed negatives, retained wins, exact-cohort limits and raw-artifact availability. Historical audit proposals do not automatically reopen rejected trials.

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

The portable workflow has GCC/Clang Linux and MSVC Windows entries. Windows builds
the portable targets and retains every native C++ test, including Core-backed PCM,
shared gameplay, reporting and the compiler-produced provider-object evidence
wrappers, plus Windows OS/UI fixtures such as the real report form with an
injected file destination. The existing 15 additional `WIN32` tooling exclusions
remain; some require shader SDK/Android tools outside this compiler lane.
Exactly 21 compiler-independent source/asset/policy CTests and five portable
post-check steps run in both Linux entries instead of repeating on Windows. The
APK resolver still runs on Windows. The exact roster and bounded evidence are in
the [MSVC selection checkpoint](evidence/2026-10-23-msvc-ci-selection/README.md).

`tools/run-windows-host-ci-tests.ps1` verifies each portable exclusion occurs once
in the complete registered roster and checks CTest's filtered result against the
intended partition. Other registrations, including new native tests, enter by
default. One CTest execution owns the selection and its log; `--no-tests=error`
and an explicit nonempty-selection guard remain. `-DryRun` reports the partition
without executing fixtures. The 15-minute job cap, compiler/build settings and
permissions are unchanged. This is additive MSVC coverage, not a replacement for
the separate full Windows tooling/shader gate; local selection checks do not
establish hosted completion or measured time savings.

The additive `selected-host-sanitizers` Ubuntu24.04 lane runs the finite target
roster in `tools/run-host-sanitizer-validation.sh` with Clang ASan/UBSan on both
compile and link, frame pointers, fail-fast diagnostics and leak detection. It
covers context-retirement ownership, shared simulation, animation/grips, physical math, Core/Horde PCM ownership,
worker/mailbox and consent-bounded reporting contracts. Each required CTest
registration is selected individually with `--no-tests=error`; a missing fixture
cannot silently shrink coverage. This is CPU memory/undefined-behavior evidence,
not Vulkan allocation/driver validation, race detection, hardware RT or listening.
Do not add sanitizer flags to production presets or infer phone performance from
instrumented host runs. Current-source job results, not YAML presence, prove a pass.

For report changes, select `horde_rt_playtest_report_tests` and, on Windows,
`horde_rt_windows_playtest_report_ui_tests`. The latter opens only its own native
form, with bounded waits; it is not real Windows file-picker/write, remote delivery
or RTX evidence. Android `PlaytestReportExportTest` covers approved-byte ownership,
consent, explicit retry/cancellation and destination failure. Actual Android
picker/export/contrast evidence and exact artifact identities are retained in
[the reporting record](ENGINEERING_1_6_1_REPORTING_2026-10-01.md). Neither local
export nor mocked destinations close a backend/real-delivery gate.

The mailbox stress fixture retains 125,000 publications, four concurrent readers,
coherence/monotonic/final-publication checks and its 30-second deadline. Readers
yield after each 64 validated samples to share oversubscribed CPUs; timeout output
reports writer and sampled reader progress. A local pass is not proof of the
precise cause of a Windows CI timeout or a substitute for fresh current-source CI.

The same workflow also runs on pushes to `codex/horde-1.6.1-engineering-pass`, so
PR merge conflicts cannot suppress all current-source compiler coverage. Its
`player-vulkan-host` lane enables Vulkan targets with the Ubuntu development
package and builds/runs the focused player contracts, actual skinned-player smoke
and malformed/reordered GLB fixtures. Focused initialization preflight and scene
resource-inventory tests also cover output-only resize rollback and ownership.
The investigation-only staged-primary owner has CPU fault-injection coverage in
this focused lane. Current ec13876 push/PR jobs pass17/17, including dynamic
buffer mapping/placement and driver metadata contracts; future roster changes
must use their actual job results rather than this historical count. Its tiny non-SPIR-V header and
fake Vulkan/resource functions test cleanup/resize ownership only, not real
allocation, shader validity, device limits or image/performance acceptance.
The tests do not create a Vulkan device:
SDK-enabled host compilation is **not** physical RT presentation, backend image
parity or Android-device acceptance. Keep the portable lane as separate coverage.

Windows configure/build/test presets are in `CMakePresets.json`:

```powershell
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug
ctest --preset windows-x64-debug
```

The corresponding Release preset is `windows-x64-release`. Shader regeneration uses `tools/compile-raygen.ps1`. An Android build-only command, from `android/`, is `.\gradlew.bat assembleDebug --console=plain`; adding `installDebug` changes a device and is a separate action.

### Isolated interactive Windows development acceptance

`HORDE_RT_WINDOWS_ACCEPTANCE=ON` is an explicit development-only build option,
default OFF. Stage that executable and verified copies of the existing runtime
assets in a plain directory named `reports/horde-windows-acceptance-<suffix>`
directly under the same checkout's `reports`. It still requires packaged assets;
it does not enable source-asset fallback, capture fixtures or new gameplay.

The executable accepts either no arguments (for supported desktop launch tools)
or exactly `--acceptance-root <absolute-package-directory>/state`. Both paths
require that same dedicated package and use only its `state/campaign-v1` and
`state/settings.ini`. A fresh state must be empty; repeat launches require its
exact ownership marker. Linked/nonregular contents, hardlinks, traversal,
alternate roots and concurrent profile use fail closed before normal settings,
known-folder or Vulkan startup. A build outside that package cannot fall back to
owner careers. Valid Windows DOS short-name aliases resolve to that same
canonical package/state; they do not authorize another state directory. Inactive
profiles expose no save/settings paths. The exclusive marker lease lasts until process exit. This guards
test isolation; it is not a security boundary against another same-user process
mutating files during a run.

Ordinary builds reject the acceptance option. Development acceptance windows are
labelled `[isolated acceptance]`, their provenance includes the build flag, and
release packaging rejects that marker. Keep source/package hashes and actual
screen/input observations separate from CPU/fixture results. Do not seed earned
progress and call it natural journey acceptance. Preserve the isolated state for
same-package relaunch checks, and close owned processes after validation.

For a requested broad candidate gate, `tools/run-foundation-validation.ps1 -Mode Host` runs the foundation's host-side programme; it is not the lightweight portable CI lane. `-Mode Full` includes device work. Inspect current parameters and choose the intended device explicitly rather than trusting a historical default serial. The runner writes evidence beneath ignored `reports/` and labels its staged artefacts as unpublishable/unsigned; its success does not authorise release publication.

### Private Bellwether playtest packages

Build the Windows Debug executable from a clean commit with
`HORDE_RT_SHIPPING17_CONTENT=ON` and Shipping/High shaders. Pass it to
`tools/package-bellwether-playtest.ps1` with the hash-pinned Windows 1.7 ZIP,
the five Windows resident folders and the thirteen base audition WAVs. The
six follow-on takes and the two Kit hearth takes form separate optional groups. The record keeper’s individual shrine-errand and directions takes can each be supplied by its stable ID and pinned WAV hash. The packager stages
the current runtime roster and existing flask and revival assets in a ZIP under
`reports/`. Pass `-ArchiveOnly` when you only need the ZIP and verification
receipt. By default, the packager also extracts a runnable copy beside the ZIP.
To move the playtest to another folder, extract the entire ZIP and open
`HordeLanternRT.exe`. The executable
detects the local playtest marker, validates the packaged overlays, and uses the
separate `bellwether-playtest/campaign-v1` save profile.

For a private optimized Windows package, build from a clean commit using the
dedicated preset.

```powershell
cmake --preset windows-x64-bellwether-candidate
cmake --build --preset windows-x64-bellwether-candidate
```

The preset builds `horde_rt_diagnostic_window` in Release with the standard
Shipping/High policy and writes `build/bellwether-windows-candidate/Release/HordeLanternRT.exe`.
Keep the preset's single MSBuild worker. Windows secondary workers can drop
empty values from the inherited Git configuration, causing Git to reject the
source identity query. Pass the executable to the playtest packager with
`-OptimizedCandidate`.
The pinned voice overlay is optional. Extract the candidate ZIP to a writable
folder and start from the ordinary menu. Each extraction keeps its own
`state/campaign-v1` and settings. The candidate admits only its packaged
residents and does not enable Debug automation.

For the packaged Windows campaign continuity check, create an empty
`reports/horde-campaign-fixtures-<label>` directory in the build's source
checkout. Pass its absolute path to `horde_rt_campaign_play_session_tests.exe
--export-route-fixture`. Run the extracted executable with
`--route-validate-hidden --campaign-test-root <fixture directory>`. Use a second
fresh fixture and add `--require-rayquery-compute` to check the other hardware
backend. The fixture seeds the two tomb victories and lantern claim. The replay
uses the live rope, lantern raise, flask guide, forest route and campaign menus
to earn the lookout, return to the tomb and reload the lookout saved in that run.
It checks that Kit's ankle assessment updates the lookout boundary, follows
the south entrance to the tavern, earns the three-line Talk exchange and reloads
its autosave. Keep `windows-route-validation.json`,
`route-frames.csv`, the backend capability report and milestone PNGs together
with the ZIP receipt. Unseeded New Game evidence, physical-device performance
and listening checks are separate.

To include the tomb opening, leave the isolated directory free of save
files. Write only the following line to `manifest.json` as UTF-8, with one
final LF and no byte-order mark.

```json
{"evidenceClass":"empty profile for the scripted New Game route"}
```

Add `--route-from-new-game` to the packaged Windows validation command.
This mode chooses an empty slot from the native New Game menu, defeats both
guards and the Keeper with damage enabled, claims the lantern and earns the
first on-disk checkpoint. It then continues the same route with the health
left over from combat. The replay uses scripted inputs with tutorials off
and muted audio. Repeat with a fresh directory for the other RT backend.

For the packaged Windows stress routes, pass the stager's
`windows-package.json` to the existing measurement launcher.

```powershell
$receipt = 'reports/<packaged-build>/windows-package.json'
./tools/measure-chapter-shipping-route.ps1 -Output reports/bellwether-east-pipeline -Configuration Debug -BellwetherPackageReceipt $receipt
python tools/analyze-chapter-route-measurement.py --bellwether-resident-roundtrip reports/bellwether-east-pipeline
```

Use a fresh output prefix for each run. Add `-Query` for RayQueryCompute.
Add `-BellwetherWest` and analyze with `--bellwether-west-roundtrip` for the
stock-yard route. The launcher verifies the ZIP and executable hashes,
extracts a fresh settings profile and uses the packaged roster and voices
with Shipping/High shaders at the regular Windows client size. The
analyzer checks both returns and reports timings and RT allocations for
each staffed room visit. The hidden RT results cover GPU and native
frame-cycle durations. Visible display pacing needs its own capture.

On Android, build `:app:assembleDebug` with `-PhordeBellwetherPlaytest=true` and the
`hordeBellwetherResidentDir`, `hordeBellwetherVoiceDir`, `hordeFlaskCandidateDir`
and `hordePrivateRevivalDir` inputs. The full profile uses
`com.samfa12.hordelanternrt.debug.bellwether.playtest` and opens the ordinary
campaign menu. Use Shipping/Mobile shaders for cross-platform acceptance.
The unseeded New Game entry and seeded lookout walkthroughs establish separate
evidence. Keep these packages and their private inputs in local storage.

Use the optimized candidate for Android acceptance through the ordinary menus.
Build `:app:assembleBellwetherCandidate` with `-PhordeBellwetherCandidate=true`,
the same resident, flask and revival inputs, and the optional pinned voice
directory. Omitting the Bellwether voice directory leaves the village dialogue
captioned. The candidate uses standard Shipping/Mobile shaders, the local
development key and the isolated app ID
`com.samfa12.hordelanternrt.bellwether.candidate`. Its package version derives
from the shared `VERSION` file with a candidate suffix. It has no development
checkpoints or seeded-profile entry. Keep the Debug playtest for controlled
route replays and instrumentation.

For an Android touch walkthrough from a seeded lookout, use the host test
executable's `--export-lookout-fixture` with a fresh fixture directory. Import
the exact `slot-1-auto.hrd` under `files/campaign-tests/<unique-name>` in the
isolated Debug app. Launch with `--es campaign_test_namespace <unique-name>`
and load the slot from the ordinary menu. The optional
`--ez horde.debug.overlay true` shows the player's X/Z position and heading.
With the test namespace and overlay active, `HordeCampaignWalk` in logcat
mirrors changed poses from the simulation. This lets a touch walkthrough
verify its route without authoring a forced camera path. Keep the seed hash,
walkthrough captures and resulting autosave together, and use an unseeded
entry separately when checking the full campaign.

#### Sampling the staffed-hearth dispatch on Android

To investigate the normal 1.8 geometry layout, prepare the observer from the
same checkout as the playtest. From the repository root, choose a fresh
directory under `reports/` and use the Windows Vulkan shader tools.

```powershell
python tools/prototype-dispatch-clock.py . reports/hearth-clock-shaders
python tools/compile-dispatch-clock.py reports/hearth-clock-shaders
```

For the Bellwether shadow comparison, add `--hearth-shadow-split` to the
first command. The sidecars and analyzer report `direct_fire_shadow` for
visibility queries from direct fire-emitter lighting and `other_shadow`
for the other visibility queries. Those two categories together cover
the previous `shadow` category. Mist, electricity and dust share a slot
in this profile. The analyzer rejects mixed profiles within one run.

Build the private Bellwether APK with its usual assets, Shipping/Mobile,
`-PhordeRtRouteTiming=true`, `-PhordeFrameStageProbe=true`,
`-PhordeDispatchClockProbe=true` and `-PhordeDispatchClockShaderRoot=<absolute path to reports/hearth-clock-shaders/source>`.
This installs separately as `com.samfa12.hordelanternrt.debug.bellwether.clock`.
Check the compiled observer modules against the APK. Compare a frozen
`bellwether-repeat-visit` capture against the ordinary package at the
same backend, orientation and scale.

Run `bellwether-west-roundtrip-measurement` with GPU timing enabled. During
the first staffed-room hold, the app writes a binary grid and JSON sidecar
for each of three settled frames to `files/reports/`. Copy the sidecars,
their `.bin` files and the corresponding `bellwether-west-roundtrip.csv`
to one directory. Each JSON becomes visible after its own successful
presentation and CSV row. The sample set can be verified during the first
visit. A full-route timing claim still requires the two returns.

```powershell
python tools/analyze-chapter-route-measurement.py --android-bellwether-hearth-clock <observer-SPIR-V-SHA256> <capture-directory>
```

The verifier joins each grid to its frame and checks the ticket, pixel
coordinates, category totals and resident identities. Read each share as
a fraction of sampled invocation time. Instrumentation changes the shader
and can affect scheduling and register use. Use an ordinary shader build
to measure an optimization.

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

## Historical 1.6.1 programme sequencing and final disposition

The owner's 2026-09-13 update supersedes older task briefs that demand a full Host/device programme after each implementation slice. Use relevant targeted checks during development, broaden only when coupling or failures warrant it, and reserve the comprehensive Windows/Android/cross-device/release matrix for the complete final candidate. S24/S25 compatibility, adaptive Pocket Chordsmith music and cross-platform player reporting are required before that final pass. This changes validation scheduling, not any RT, correctness, exact-artifact or final acceptance requirement.

October3 owner decisions explicitly defer final S24 coverage as working but not
fully tested, leave exact final S25 unverified, accept measured performance as-is
and move remaining High glass defects to future investigation.1.6.1 is published
and normally integrated; see the [completion report](ENGINEERING_1_6_1_COMPLETION_REPORT.md).
Retain the original evidence/failures and do not turn deferred gates into passes
or restart them after resumption. Main now requires the actual six workflow checks
and up-to-date PRs, with no force-push/deletion and explicit admin bypass. Device,
shader and owner-feel evidence remain separate from these host/build checks.

Existing 1.6.1 evidence remains tied to its original source/build: `ENGINEERING_1_6_1_ANDROID_OBSERVATION_BASELINE_2026-09-05.md` is Diagnostic/Debug evidence, not Shipping/Release performance. Preserve matched A/B build, pipeline, workload, scale and thermal identity. A window-average median must not be relabelled as a per-frame median. The full route exceeds the 128-sample window collector; retain all intended samples with explicit capacity/invalid-run handling.
