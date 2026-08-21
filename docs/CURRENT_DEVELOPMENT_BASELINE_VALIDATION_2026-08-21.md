# Current development baseline validation

**Date:** 2026-08-21

**Status:** Exact Android automation, full Windows Host validation, CI, and owner spatial-audio validation passed. Exact-candidate haptic confirmation remains pending.

**Publication:** None. Showcase Alpha 0.1.3 artifacts, version identity, signing identity, and public downloads remain unchanged.

## Source

- Starting local `main`: `b9212e77f7dd5eeb98a6527ff9b3c945ca90a820`.
- Prior fully provenance-bound two-skeleton phone candidate: `b3428a7cb9b4f4d1ed3d77940bb7d4b177e4b5d6`.
- Exact runtime/authority candidate: `4a4d360318d6b61900e51f3b2322afc47bcb48af`, clean at build and device validation time.
- Runtime correction commit: `068ee50`.
- Embedded raygen SHA-256: `e0c79c848210a76d30e1036a67ded744dc9d51fe37827a52f89b5385028419d1`.
- Debug APK SHA-256: `fa160e84ffd655a3504d4e19e5025b1addddc4f862586c38a2f5743811cd026a`.
- Installed `base.apk` SHA-256: exact byte-for-byte match.

The diff from `b3428a7` adds animation-owned swing/parry/lich contact semantics, Android press-down parry publication, the bounded animated stagger renderer mapping, event-time listener data, stronger tests and diagnostics, validation-tooling changes, and authority-document corrections. It does not change raygen/shader ABI, assets, published release artifacts, signing material, native RT dispatch, ray-query path tracing, recursion depth, ASTC policy, or presentation semantics.

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
- The mailbox stress uses one writer, four synchronized readers, 125,000 publications, full-field coherence, per-reader monotonicity, final-publication visibility, and a 30-second watchdog.
- Renderer tests cover swing/parry composition, animated stagger sampling/recoil/settle, shared stagger pose reuse, split pose buckets, masked slots, two TLAS transforms, and singular lich routing.

## Host and CI

- Shader staleness passed. Negative stale-shader, immutable-release, version-code, upload, and package-layout safety gates rejected their fixtures as expected.
- Canonical clean Host run `reports/foundation-runs/run-20260821-170907` passed fresh Windows Debug and Release builds with 12/12 Vulkan-enabled CTests in each configuration. An earlier run had two Release programs blocked before launch by Smart App Control; the clean retry produced new hashes that launched normally without changing Windows Security.
- A separate local portable configuration passed 8/8 Vulkan-disabled tests.
- GitHub Actions runs `32456717553` and `32457974761` passed the real eight-test portable `shared-gameplay` lane for draft PR 14 at the runtime/authority and evidence commits respectively.
- The exact Windows Debug candidate produced 13/13 deterministic captures. All thirteen were pixel-identical to `reports/foundation-runs/run-20260813-204944/captures/windows`; overall matched capture timing changed from 6.0618 to 6.0531 ms (-0.144%).
- The same Host run passed 13 Windows captures, clean Android Debug/unsigned Release and `lintRelease` across all four configured ABIs, validation packaging/licence checks, release identity checks, shader staleness, negative safety fixtures, and final evidence hashes.

## Android exact candidate

Device evidence is direct local automation on Samsung `SM-S948B`, Android 16/API 36, Adreno 840, driver 512.842.19, Vulkan 1.4.295. The local and installed exact Debug APK hashes matched. Strict environment/lich ASTC, `RayTracingPipeline`, and RT-produced swapchain presentation remained active.

The first default-order run began at battery 29.0 C and ended at 37.0 C. Opening, two-enemy, worst-bend, skylight, and green passed, while last-in-order lich measured 23.568 ms and failed. That hot run is retained rather than relabelled. It separately passed the report-only 100% opening sample, deterministic replay, all captures, and lifecycle checks.

After cooling, the exact installed APK ran the same six 75% checkpoint workloads with lich first to control run-order heating:

| Checkpoint | Three 120-frame averages | Median | Classification | Thermal status |
|---|---:|---:|---|---:|
| lich | 19.522 / 19.695 / 19.702 ms | **19.695 ms** | PASS - LOW HEADROOM | 0 |
| opening | 11.290 / 11.601 / 12.212 ms | **11.601 ms** | PASS | 0 |
| two-enemy-combat | 14.358 / 14.310 / 14.227 ms | **14.310 ms** | PASS | 0 |
| worst-bend | 10.696 / 12.658 / 14.890 ms | **12.658 ms** | PASS | 0 |
| skylight | 8.637 / 8.887 / 8.856 ms | **8.856 ms** | PASS | 0 |
| green | 11.079 / 11.053 / 11.055 ms | **11.055 ms** | PASS | 0 |

Every required median remained strictly below 20.000 ms with honest presentation at thermal status 0. The 18.500 ms threshold is advisory only; lich correctly reports low headroom without weakening the hard gate or RT path.

The report-only 100% opening median from the exact default-order run was **23.621 ms**. It is not part of the 75% pass/fail decision.

## GPU-timing A/B

Matched cooled isolated lich runs used the same device, source, shader, APK, installed artifact, scale, checkpoint order, and thermal status. Starting AP/SKIN/BAT differed by 0.6/0.3/0.4 C:

| GPU timestamps | Median | Classification |
|---|---:|---|
| Enabled | **19.686 ms** | PASS - LOW HEADROOM |
| Disabled | **19.587 ms** | PASS - LOW HEADROOM |

Enabled was 0.505% slower than disabled, below the 15% investigation threshold. The comparator passed and did not identify GPU timestamp instrumentation as a material cause.

## Two-enemy proof, replay, captures, and lifecycle

- `two-enemy-combat` reported two active/rendered enemy entities, attacker entity ID 2 (`SkeletonA`), one shared pose bucket, 3/3 player vitality, and honest presentation.
- Deterministic replay reached 13/13 waypoints, completed in the finale, and retained honest presentation.
- All 13 Android captures completed after 12 stable RT-presented frames. The two-enemy capture retained SHA-256 `404d1bcf04380c3565449b88d204c4b2da9cd320bd470163cfd7c3e9cddfada0`.
- Home/resume recreated the surface and produced a new honest presentation marker.

Primary ignored evidence bundles:

- full default-order functionality/captures/lifecycle and hot performance context: `reports/android-showcase-runs/run-20260821-163426`;
- cooled six-checkpoint pass: `reports/android-showcase-runs/run-20260821-164739`;
- matched timing-disabled lich: `reports/android-showcase-runs/run-20260821-165402`;
- matched timing-enabled lich and comparator: `reports/android-showcase-runs/run-20260821-165603`;
- Windows capture comparison: `reports/current-reconciliation/windows-capture-comparison.json`.

## Audio and haptics

`Audio/haptic manual revalidation required: YES` because listener-at-event-time routing materially changes the semantic data used for spatialisation on both platforms.

With the exact candidate still installed and open, the owner reported: “the audio sounds good.” This is owner-reported physical-device perception covering the requested spatial-audio check; automation alone did not establish it.

**Spatial audio owner baseline: PASSED.**

**Haptic owner baseline on this exact candidate: PENDING.**

After this exact check passes, the accepted baseline is change-triggered rather than milestone-triggered. Recheck only changes that can materially affect source/listener event data, spatialisation, audio cues/assets/backend/gain/timing, semantic event transport/mapping, haptic routing/pattern/intensity, or damage/death feedback. Unrelated RT, visual, UI, asset, build, packaging, documentation, telemetry, unrelated AI, or unrelated animation changes do not automatically invalidate it when automated contracts pass.

## Evidence boundary and remaining risks

- Automated evidence proves only the exact source/APK/device/driver and recorded deterministic routes. It does not prove another device, subjective comfort, artistic quality, or untested future source.
- Exact-candidate haptic perception remains pending; automation cannot replace it.
- Lich passes below 20 ms only with low headroom; retain the 18.5 ms warning and investigate any matched regression above 15%.
- Hotstrike Studio skeleton finished-game use is credited, but public raw-source redistribution permission remains unresolved in GitHub issue 13. Do not publish source model files without explicit owner-approved resolution.
- The owner-only signing backup/recovery checklist remains deliberately unchecked in `OWNER_RELEASE_SAFETY_CHECKLIST.md`. No signing secret was accessed, copied, or committed.
- Published Showcase Alpha 0.1.3 artifacts were not rebuilt, replaced, signed, uploaded, or published by this work.

## Next milestone

The master audit's proposed “explicit combat action states and animation events” milestone is already present in current source through animation-owned swing/parry/enemy phases. Do not reimplement it or raise the enemy ceiling. After this baseline closes, the next genuinely current gameplay/visual slice should be separately planned and phone-measured; bounded fire remains the leading deferred route item.
