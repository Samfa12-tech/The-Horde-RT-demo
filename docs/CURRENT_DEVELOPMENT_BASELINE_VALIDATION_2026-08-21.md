# Current development baseline validation

**Date:** 2026-08-21

**Status:** Final exact-source Windows/Android automation passed. Sustained phone performance is reported as evidence rather than judged against the former hard 20 ms rule. The final exact candidate's owner audio/haptic/stagger check remains pending.

**Publication:** None. Showcase Alpha 0.1.3 artifacts, version identity, signing identity, and public downloads remain unchanged.

## Source

- Starting local `main`: `b9212e77f7dd5eeb98a6527ff9b3c945ca90a820`.
- Prior fully provenance-bound two-skeleton phone candidate: `b3428a7cb9b4f4d1ed3d77940bb7d4b177e4b5d6`.
- Exact runtime/authority candidate: `4a4d360318d6b61900e51f3b2322afc47bcb48af`, clean at build and device validation time.
- Final exact runtime correction and validation commit: `547d89d00efa476b533176576c1cedc3a1d44cbd`, clean before and after the Full gate.
- Principal runtime correction commits: `068ee50` and `547d89d`.
- Embedded raygen SHA-256: `e0c79c848210a76d30e1036a67ded744dc9d51fe37827a52f89b5385028419d1`.
- Final Debug APK SHA-256: `f68fe4cccf2755ef579826080bf76364fe2a48744b23aba765df02a17e2d1dfa`.
- Installed `base.apk` SHA-256: exact byte-for-byte match.

The diff from `b3428a7` adds animation-owned swing/parry/lich contact semantics, Android press-down parry publication, the bounded animated stagger renderer mapping, event-time listener data, matched platform skeleton-hit/fall routing, bounded Android transport, stronger tests and diagnostics, validation-tooling changes, and authority-document corrections. It does not change raygen/shader ABI, runtime model/audio assets, published release artifacts, signing material, native RT dispatch, ray-query path tracing, recursion depth, ASTC policy, or presentation semantics.

The evidence-document commit necessarily follows the runtime commit it records. That documentation-only commit is not substituted for the exact installed runtime provenance above; the Full runner recorded `547d89d`, a clean tree, and the matching APK hashes directly.

### Difference classification from the prior exact phone candidate

| Category | Current differences |
|---|---|
| Gameplay-semantic | `SwordCombat` and `GameSimulation` action phases, active-window contact, parry/stagger, event-time listener state, snapshots and diagnostics. |
| Renderer | `CharacterRenderSlot`, frame adapter, scene slot/descriptor/TLAS mapping, animated stagger composition, and overlay exposure. |
| Shader | No later shader-source or ABI change; binding 10/custom index 18 and the embedded raygen identity were rechecked. |
| Audio/event | Bounded ordered events, Android transport, Windows/Android positional cue mapping, fixed 140 ms impact/fall delay, and stale delayed-cue cancellation. |
| Validation tooling | Schema-7 sustained performance bands, exact artifact/device provenance, capture comparison, and safety/package gates. |
| Tests only | Mailbox, simulation/event, spatial audio/platform mapping, renderer/shader mapping, timing, route, and overlay regressions. |
| Documentation only | Current authority, historical evidence qualifications, owner-test trigger policy, licensing risk, and signing-safety checklist. |

## Architecture

- `GameSimulation` remains the owning deterministic 60 Hz authority.
- Android input remains a coherent single-writer/multi-reader two-slot mailbox with independent monotonic swing, parry, reset, and retry commands.
- Gameplay feedback remains a bounded ordered semantic event queue. Each event now retains source position plus listener X/Z/yaw from its own fixed tick, and both Android and Windows spatialise from that immutable event-time data.
- The opening encounter remains capped at stable `SkeletonA` and `SkeletonB`, with independent deaths, deterministic 0.70 m separation, one distance/ID-selected attacker token, and nearest-valid single-target sword contact.
- The lich remains singular and retains three health, active-window sword contact, two-second accepted-hit lockout, recoil/cries/death/finale behavior, and non-parryable ranged damage.
- A successful parry keeps the attacker stationary and token-owning for the gameplay-authoritative 800 ms stagger. Rendering advances the existing Attack clip from contact toward recovery and adds an early bounded recoil/lean, so the stagger is visibly animated without a new clip or resource bucket.

## Renderer and shader

- `CharacterRenderSlot` retains one cached frame plan for skin/refit and TLAS routing.
- Matching skeleton poses share one pose/BLAS; divergence remains capped at two pose buckets.
- The scene remains at nine BLAS and nineteen physical TLAS slots. The second bounded skeleton pose route uses instance custom index 18; unused slots retain invertible transforms and zero masks.
- Binding 10 remains the bounded pose-segment selector path. Geometric/shading normals, singular lich routing, native `vkCmdTraceRaysKHR`, `rayQueryEXT`, recursion depth one, one frame in flight, strict ASTC, and honest swapchain presentation are unchanged.

## Automated regression contracts

- Multiple fixed ticks inside one render frame retain distinct listener poses at event emission time.
- Spatial-audio tests retain equal-power pan, deterministic left/right gains, distance rolloff, obstruction attenuation, and event-time listener position/yaw behavior without mutating the source position.
- Event tests retain ordering, sequence, stable entity identity, targetless miss semantics, A/B hit/defeat ordering, fatal-versus-nonfatal damage semantics, and explicit overflow without overwrite.
- Feedback tests cover skeleton footsteps/attack/hit/defeat, lich charge/impact/hit/defeat, both stable skeleton IDs, the exact 140 ms impact/fall boundary, retained event-time source/listener data, repeated same-type ordering, cancellation on reset/retry/lifecycle, and matching Windows/Android platform mappings.
- Android's 128-entry handoff uses a fixed-capacity transport queue that drops only the newest event on overflow; the permanent overflow count remains visible in diagnostics.
- The mailbox stress uses one writer, four synchronized readers, 125,000 publications, full-field coherence, per-reader monotonicity, final-publication visibility, and a 30-second watchdog.
- Renderer tests cover swing/parry composition, animated stagger sampling/recoil/settle, shared stagger pose reuse, split pose buckets, masked slots, two TLAS transforms, and singular lich routing.

## Host and CI

- Final Full run `reports/foundation-runs/run-20260821-184821` passed every stage from clean commit `547d89d`: shader staleness and negative fixtures, fresh Windows Debug/Release builds, 12/12 CTests in each configuration, 13 Windows captures, Android Debug/unsigned Release/lint across four ABIs, package/licence/release-identity safeguards, exact device automation, and final hashes.
- The final 13 Windows captures are pixel-identical to the locked 13-capture baseline. Matched overall capture timing changed from 6.0618 to 6.05565 ms (-0.101%).
- Shader staleness passed. Negative stale-shader, immutable-release, version-code, upload, and package-layout safety gates rejected their fixtures as expected.
- Canonical clean Host run `reports/foundation-runs/run-20260821-170907` passed fresh Windows Debug and Release builds with 12/12 Vulkan-enabled CTests in each configuration. An earlier run had two Release programs blocked before launch by Smart App Control; the clean retry produced new hashes that launched normally without changing Windows Security.
- A separate local portable configuration passed 8/8 Vulkan-disabled tests.
- GitHub Actions runs `32456717553` and `32457974761` passed the real eight-test portable `shared-gameplay` lane for draft PR 14 at the runtime/authority and evidence commits respectively.
- The exact Windows Debug candidate produced 13/13 deterministic captures. All thirteen were pixel-identical to `reports/foundation-runs/run-20260813-204944/captures/windows`; overall matched capture timing changed from 6.0618 to 6.0531 ms (-0.144%).
- The same Host run passed 13 Windows captures, clean Android Debug/unsigned Release and `lintRelease` across all four configured ABIs, validation packaging/licence checks, release identity checks, shader staleness, negative safety fixtures, and final evidence hashes.
- After the performance-reporting scripts/docs changed, focused schema-7 phone run `run-20260821-182247` passed with exact local/installed APK hashes, honest presentation, an 11.303 ms opening median (60 FPS reference or better), thermal status 0, and GPU thermal power level 0. A fresh Host rerun `run-20260821-182339` passed shader/safety gates, builds, and all 12 Debug tests; Windows Smart App Control then refused to launch four newly linked Release test executables while the other eight passed. One unchanged-binary retry produced the same `BAD_COMMAND` launch refusal. No Windows Security setting was changed and this environmental refusal is not represented as a test failure in product code; the preceding canonical clean Host run remains the complete 12/12 Debug/Release evidence for the unchanged native source.

## Android exact candidate

Device evidence is direct local automation on Samsung `SM-S948B`, Android 16/API 36, Adreno 840, driver 512.842.19, Vulkan 1.4.295. The local and installed exact Debug APK hashes matched. Strict environment/lich ASTC, `RayTracingPipeline`, and RT-produced swapchain presentation remained active.

### Final exact sustained Full run

The standard single-process order was retained. The device stayed at Android thermal status 0 while Samsung's GPU thermal power level rose from 0 to 2. The lich crossed the descriptive 20 ms / 50 FPS line; it did not fail the run because state, workload, RT presentation, and provenance remained valid.

| Checkpoint | Three 120-frame averages | Median | Derived FPS | Band | Thermal / GPU power |
|---|---:|---:|---:|---|---:|
| opening | 11.410 / 11.340 / 11.308 ms | **11.340 ms** | 88.183 | 60 FPS reference or better | 0 / 0 |
| two-enemy-combat | 12.145 / 12.074 / 12.167 ms | **12.145 ms** | 82.338 | 60 FPS reference or better | 0 / 0 |
| worst-bend | 9.159 / 9.267 / 9.439 ms | **9.267 ms** | 107.910 | 60 FPS reference or better | 0 / 1 |
| skylight | 9.698 / 10.390 / 10.375 ms | **10.375 ms** | 96.386 | 60 FPS reference or better | 0 / 2 |
| green | 13.035 / 13.423 / 12.999 ms | **13.035 ms** | 76.717 | 60 FPS reference or better | 0 / 2 |
| lich | 23.604 / 22.943 / 23.069 ms | **23.069 ms** | 43.348 | 30-50 FPS reference band | 0 / 2 |

The separately reported final 100% opening windows were 24.227 / 22.837 / 22.645 ms, with a **22.837 ms** median (43.789 FPS, 30-50 FPS reference band) at thermal status 0 / GPU power level 3.

### Earlier attribution controls

The first default-order run began at battery 29.0 C and ended at 37.0 C. Opening, two-enemy, worst-bend, skylight, and green passed, while last-in-order lich measured 23.568 ms and failed under the hard gate then in force. That historical result is retained rather than relabelled. It separately passed the report-only 100% opening sample, deterministic replay, all captures, and lifecycle checks.

After cooling, the exact installed APK ran the same six 75% checkpoint workloads with lich first to control run-order heating:

| Checkpoint | Three 120-frame averages | Median | Classification | Thermal status |
|---|---:|---:|---|---:|
| lich | 19.522 / 19.695 / 19.702 ms | **19.695 ms** | 50-60 FPS reference band | 0 |
| opening | 11.290 / 11.601 / 12.212 ms | **11.601 ms** | 60 FPS reference or better | 0 |
| two-enemy-combat | 14.358 / 14.310 / 14.227 ms | **14.310 ms** | 60 FPS reference or better | 0 |
| worst-bend | 10.696 / 12.658 / 14.890 ms | **12.658 ms** | 60 FPS reference or better | 0 |
| skylight | 8.637 / 8.887 / 8.856 ms | **8.856 ms** | 60 FPS reference or better | 0 |
| green | 11.079 / 11.053 / 11.055 ms | **11.055 ms** | 60 FPS reference or better | 0 |

Every recorded median remained below the 20.000 ms / 50 FPS reference in this controlled order with honest presentation at thermal status 0. This is useful diagnostic evidence, but cooling and putting the heaviest checkpoint first do not describe an ordinary sustained play session and are no longer used to manufacture a binary pass.

The separately reported 100% opening median from that earlier exact default-order run was **23.621 ms**.

## GPU-timing A/B

Matched cooled isolated lich runs used the same device, source, shader, APK, installed artifact, scale, checkpoint order, and thermal status. Starting AP/SKIN/BAT differed by 0.6/0.3/0.4 C:

| GPU timestamps | Median | Classification |
|---|---:|---|
| Enabled | **19.686 ms** | 50-60 FPS reference band |
| Disabled | **19.587 ms** | 50-60 FPS reference band |

Enabled was 0.505% slower than disabled, below the 15% investigation threshold. The comparator passed and did not identify GPU timestamp instrumentation as a material cause.

## Sustained performance attribution and revised policy

The former strict-below-20 ms rule was an engineering guardrail chosen during development, not an owner requirement or a device/API constraint. Its useful meaning is simply the 50 FPS reference line. It is superseded by evidence-first reporting:

- 16.667, 20.000, and 33.333 ms are descriptive approximately 60/50/30 FPS lines, not quality cliffs;
- the standard six-checkpoint run stays in one process and reports sustained behavior, checkpoint order, battery/thermal context, and Adreno GPU thermal power level where readable;
- a fresh-process or cooled result is labelled as a diagnostic control rather than substituted for sustained behavior;
- exact matched regressions above 15% trigger investigation, not automatic rejection; growing memory/resource counts, changed workloads, crashes, invalid state, or dishonest presentation remain real failures;
- 100% remains separate, and no automatic resolution reduction, RT weakening, or gate-raising is used to improve a label.

Exact clean source `88868f4` with non-game Debug APK SHA-256 `fb3a5727a0f439f51482e569625a17252170c9c77b1b5232143d1751d01db37a` was installed byte-for-byte for the attribution runs. In sustained order `lich/opening/two-enemy/worst-bend/skylight/green`, medians were **19.669 / 20.962 / 21.610 / 17.698 / 17.487 / 20.656 ms** (`run-20260821-175811`) while battery temperature rose from about 33 C to 36 C. Temporarily changing Samsung's `restricted_device_performance` owner setting did not remove the pattern (`run-20260821-180228`); the original setting was restored.

A one-process telemetry sweep showed the opening windows transition from **11.297 / 16.428 / 21.407 ms** and the final lich reach **39.438 / 36.380 / 34.803 ms**. Across that sweep, graphics allocation stayed about **233.9 to 233.2 MB**, native heap fluctuated within about **60-72 MB**, PSS/RSS stayed about **336-347 / 452-463 MB**, and threads moved from 31 to 29. There was no accumulating memory or resource-count signature.

Repeated identical opening measurements then exposed the cause more directly. The first iteration held the GPU at **1300 MHz** for all 21 samples and produced roughly **11.3 ms** windows at GPU thermal power level 0. During the next iteration the GPU fell to **578-646 MHz**, thermal power level rose to 7, and the third window reached **23.552 ms**. Force-stopping and relaunching temporarily restored the fast opening even while warm: isolated opening medians were **11.370 ms at 35.1 C** (`run-20260821-180814`) and **11.346 ms at 37.1 C** (`run-20260821-181046`).

This is strong evidence that the sustained slowdown was Samsung GPU governor/power-level behavior with a fresh-process boost, not game heap/graphics bloat. It does not prove the renderer has no future optimisation opportunities; it means those opportunities should be found through matched workload changes and scaling tests rather than repeated cooling loops.

A Game Mode experiment was also rejected rather than retained. Standard and Performance modes were compared on an otherwise matched temporary build; Performance increased the lich median from **19.674 to 31.301 ms** (59.1%). All manifest/resource/GameState changes were reverted, the owner device-performance setting was restored, and the currently installed package is again not classified as an Android game.

## Two-enemy proof, replay, captures, and lifecycle

- `two-enemy-combat` reported two active/rendered enemy entities, attacker entity ID 2 (`SkeletonA`), one shared pose bucket, 3/3 player vitality, and honest presentation.
- Deterministic replay reached 13/13 waypoints, completed in the finale, and retained honest presentation.
- All 13 Android captures completed after 12 stable RT-presented frames. The two-enemy capture retained SHA-256 `404d1bcf04380c3565449b88d204c4b2da9cd320bd470163cfd7c3e9cddfada0`.
- Home/resume recreated the surface and produced a new honest presentation marker.

Primary ignored evidence bundles:

- final exact clean Full gate: `reports/foundation-runs/run-20260821-184821`;
- final exact Android run: `reports/foundation-runs/run-20260821-184821/android-device/run-20260821-185117`;
- full default-order functionality/captures/lifecycle and hot performance context: `reports/android-showcase-runs/run-20260821-163426`;
- cooled six-checkpoint pass: `reports/android-showcase-runs/run-20260821-164739`;
- matched timing-disabled lich: `reports/android-showcase-runs/run-20260821-165402`;
- matched timing-enabled lich and comparator: `reports/android-showcase-runs/run-20260821-165603`;
- Windows capture comparison: `reports/current-reconciliation/windows-capture-comparison.json`.

## Audio and haptics

`Audio/haptic manual revalidation required: YES` because listener-at-event-time routing, bounded Android event transport, and Windows skeleton hit/fall timing materially touch feedback delivery.

With the earlier exact candidate still installed and open, the owner reported: “the audio sounds good.” This is valid physical-device evidence for that candidate, but the final `547d89d` transport/timing correction intentionally triggers one final check rather than silently inheriting it.

**Final exact spatial-audio owner baseline: PENDING.**

**Final exact haptic owner baseline: PENDING.**

**Final exact stagger-readability owner check: PENDING.**

After this exact check passes, the accepted baseline is change-triggered rather than milestone-triggered. Recheck only changes that can materially affect source/listener event data, spatialisation, audio cues/assets/backend/gain/timing, semantic event transport/mapping, haptic routing/pattern/intensity, or damage/death feedback. Unrelated RT, visual, UI, asset, build, packaging, documentation, telemetry, unrelated AI, or unrelated animation changes do not automatically invalidate it when automated contracts pass.

## Evidence boundary and remaining risks

- Automated evidence proves only the exact source/APK/device/driver and recorded deterministic routes. It does not prove another device, subjective comfort, artistic quality, or untested future source.
- Final exact-candidate audio/haptic perception and stagger readability remain pending; automation cannot replace them.
- Current sustained results cross the 20 ms / 50 FPS reference in several views as the device governor limits GPU frequency. Track this honestly and investigate matched source regressions above 15%; do not spend development cycles cooling the phone merely to obtain a pass label.
- Hotstrike Studio skeleton finished-game use is credited, but public raw-source redistribution permission remains unresolved in GitHub issue 13. Do not publish source model files without explicit owner-approved resolution.
- The owner-only signing backup/recovery checklist remains deliberately unchecked in `OWNER_RELEASE_SAFETY_CHECKLIST.md`. No signing secret was accessed, copied, or committed.
- Published Showcase Alpha 0.1.3 artifacts were not rebuilt, replaced, signed, uploaded, or published by this work.

## Next milestone

The master audit's proposed “explicit combat action states and animation events” milestone is already present in current source through animation-owned swing/parry/enemy phases. Do not reimplement it. Two skeletons remain the validated ceiling of this milestone, but four/five enemies are a legitimate future game goal requiring a separately measured renderer/simulation design. Bounded fire, water, improved lanterns, and a proper player model remain future costs that should be evaluated through sustained scaling evidence rather than an immutable 20 ms rule.
