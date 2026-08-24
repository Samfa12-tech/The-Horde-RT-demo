# Documentation checkpoint - 2026-07-17

Last updated: 2026-08-25

## Purpose

This is the consolidation map after Showcase Alpha 1.5.2 publication. It distinguishes current authority from dated implementation history so old “pending” language is not mistaken for the present backlog.

## Current release authority

| Claim | Authoritative record |
|---|---|
| Published artifacts, hashes, certificate, itch builds, Windows smoke, and Android evidence boundary | `SHOWCASE_ALPHA_1_5_2_RELEASE_VALIDATION_2026-08-25.md` |
| Public release contents and known limits | `SHOWCASE_ALPHA_1_5_2_RELEASE_NOTES_2026-08-25.md` |
| RT Lab implementation, controls, performance, and owner Windows acceptance | `RT_LAB_VALIDATION_2026-08-24.md` |
| General water-light transport, shadows, bounded queries, and phone measurements | `WATER_TRANSMISSION_SHADOW_VALIDATION_2026-08-24.md` |
| RT waterfall, catchment/drain, lich mist, controller, dodge, and positional-loop implementation evidence | `RT_WATERFALL_LICH_MIST_VALIDATION_2026-08-23.md` |
| Player vitality/retry implementation and evidence boundary | `PLAYER_VITALITY_RETRY_SLICE_2026-07-31.md` |
| Rebuilt player body, gait, dawn ending, and exact Debug device evidence | `PLAYER_BODY_AND_FINALE_SLICE_2026-07-31.md` |
| Android device compatibility and evidence classifications | `ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md` |
| Windows route, visual, combat, mirror and audio validation | `HORDE_SHOWCASE_WINDOWS_VALIDATION_2026-07-16.md` |
| Android hands-on route, lifecycle and warm sustained measurements | `HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md` |
| Android deterministic checkpoint/replay procedure | `ANDROID_SHOWCASE_AUTOMATION_2026-07-17.md` |
| Android cool/status-0 automation baseline | `ANDROID_SHOWCASE_AUTOMATION_VALIDATION_2026-07-17.md` and `validation/android-showcase-automation-2026-07-17/` |
| Integrated foundation gate, deterministic captures, and raygen A/B | `FOUNDATION_VALIDATION_2026-07-22.md` |
| Player-facing benchmark contract and Windows evidence | `IN_APP_BENCHMARK_WINDOWS_VALIDATION_2026-07-17.md` |
| Asset provenance/licences | root `ASSET_LICENSES.md` |
| Remaining tooling backlog | `BUILD_TEST_DEMO_CYCLE_PLAN_2026-07-17.md` |
| Day-to-day agent constraints | root `AGENTS.md`, `PROJECT_MEMORY.md`, and `PROJECT_DECISIONS.md` |

## Consolidated current state

- Public version: Showcase Alpha 1.5.2 / `1.5.2`; Android `versionCode 7`.
- Itch builds: Windows `#1913191`, Android `#1913192`.
- Route: two skeletons -> shadow corridor -> roof-water drench/lantern drop -> rounded catchment and drain runoff -> blue skylight -> four coloured bays -> open threshold -> hero mirror -> misted staff-lit lich -> opening roof -> returning dawn -> epilogue.
- The stained pane remains rejected. The water is a narrow real-geometry RT feature with bounded reflection/transmission, not a broad fluid simulation.
- Player body includes a layered travelling coat, articulated capsule arms and legs, pelvis/boots, foot lift/toe roll, restrained torso counter-rotation, head shadow/reflection, and wall-aware props.
- Player vitality/death/retry is implemented: three points, a one-second damage lockout, short fatal hold, encounter retry, full-route restart, and platform-native death overlays.
- Two skeletons can be active with at most two pose buckets; the later CC0 lich remains singular.
- Android release packages both enemy GLBs, strict environment/lich ASTC, 17 FilmCow cues, and the DRAGON-STUDIO/Pixabay waterfall loop. Windows packages both enemies, raw textures, the same audio set, and the licence manifest.
- Warm 75% Android hands-on certification and cool deterministic automation are separate evidence classes; neither should be relabelled as the other.
- The Android Debug harness supplies 13 presets, a default six-checkpoint three-window pass, and a 13-waypoint replay. Release rejects the request path.
- A separate player-facing benchmark is release-safe on both platforms: one warm-up lap plus one measured 13-waypoint lap, followed by a selectable/copyable/exportable report and archived JSON. Windows and `SM-S948B` Android device validation pass; see the dated in-app benchmark validation records.
- The integrated `Host|Full` foundation gate is complete. It owns clean cross-platform builds/tests, stale-shader and package/licence/identity checks, validation-only artifacts and hashes, 13 deterministic Windows captures, and the connected-phone replay/lifecycle/13-capture evidence path.
- The unreachable stained-glass material route was removed after a bounded A/B: SPIR-V structure improved, all 12 Windows images were bit-exact, Windows median timing was unchanged, and all five comparable-temperature Android 75% checkpoints improved within the required non-regression rule.
- The 1.5.2 Windows ZIP passed an isolated exact-package launch with honest RT presentation and clean exit. The exact 1.5.2 Android APK passed certificate/package guards but was not installed because no ADB device was connected; 0.1.5 remains the latest public signed APK byte-matched and lifecycle-tested on `SM-S948B`.

## Historical records

The following dated families are intentionally preserved rather than rewritten: 0.1.0 release/readiness documents; Phase 1C, skeleton, PBR, combat, material, player-body, lighting and route-blockout slice evidence; the Windows audio diagnostic; and the 2026-07-13 codebase audit. Their metrics, build IDs, clip counts, and “pending” statements describe the milestone at that date. Later closure is found through the authority table above.

`NATIVE_RT_SHOWCASE_PLAN_2026-07-14.md`, `PHASE_PLAN.md`, and `COLOURED_LIGHT_ROUTE_PLAN_2026-07-15.md` now carry reconciliation notes because they are frequently used for roadmap navigation.

## Honest remaining items

- Independently back up the release JKS and both passwords.
- Keep the Hotstrike public raw-GLB/history permission question open until the creator responds or the owner chooses remediation.
- Do not raise the one-active-skinned-enemy limit without a dedicated phone measurement.
- Precise waterfall stereo direction, loop seam, and Android audible pause/resume were not separately scored; source contracts, native gains, packaging, and lifecycle behavior are verified.
- A separately authored full-character rig, encounters above two skeletons, broader AI, held block, broad fluid simulation, and the staged textured sword runtime remain deferred.
- The developer overlay is device-validated on Windows and Android. Integrated clean-build/package/stale-shader/licence gates and fixed PNG capture are complete; video and orbit-camera presentation remain tooling deferrals.
- Future publishing must explicitly supply and bump all version surfaces and Android `versionCode > 7`; the shared release-version policy rejects the immutable published 0.1.1 through 0.1.5 and 1.5.2 lines before packaging/upload work.
- The unchecked historical `v0.1.0-alpha.1` tag task is superseded; do not create a retroactive tag on a later commit.

## Audit scope and outcome

- Re-audited the living repository authorities, Windows release README, package guards, licence markers, and current source/package metadata on 2026-08-25.
- Verified all Markdown links resolve.
- Preserved dated evidence, corrected living package commands/counts/feature descriptions, promoted the reviewed automation digest, and separated cool automation from warm certification.
- Verified current source/package version surfaces agree on 1.5.2 / Android code 7. Release source `f4891c4` produced the exact published artifacts recorded in the 1.5.2 release validation.
- Rebuilt Android Debug/Release, passed Release lint, and verified the automation-only `singleTop` launch mode exists in Debug's merged manifest but not Release's. Windows Debug/Release builds and all seven CTests pass.
- The 2026-08-25 itch channel update preserves the existing public page while publishing the 1.5.2 Windows and Android downloads. The repository's prepared page copy is current, but no authenticated description Save was requested or performed; anonymous artifact verification belongs to the 1.5.2 release record.
