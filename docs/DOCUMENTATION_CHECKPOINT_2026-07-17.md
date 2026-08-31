# Documentation checkpoint - 2026-07-17

Last updated: 2026-08-31

## Purpose

This is the consolidation map after Showcase Alpha 1.6.0 publication. It distinguishes current authority from dated implementation history so old “pending” language is not mistaken for the present backlog.

## Current release authority

| Claim | Authoritative record |
|---|---|
| Published artifacts, hashes, certificate, itch builds, Windows smoke, and Android evidence boundary | `SHOWCASE_ALPHA_1_6_0_RELEASE_VALIDATION_2026-08-30.md` |
| Public release contents and known limits | `SHOWCASE_ALPHA_1_6_0_RELEASE_NOTES_2026-08-30.md` |
| Fire/PBR props, glass, held items, physical lantern, reward route, player foundation, and measured limits | `FIRE_PBR_REWARD_LANTERN_PLAYER_UPGRADE_VALIDATION_2026-08-30.md` |
| Final owner candidate, phone composition, chest guidance, and feedback acceptance | `TASK_9_OWNER_CANDIDATE_VALIDATION_2026-08-30.md` |
| GitHub Release updater implementation and security boundary | `GITHUB_RELEASE_UPDATE_FOUNDATION_2026-08-30.md` |
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

- Public version: Showcase Alpha 1.6.0 / `1.6.0`; Android `versionCode 8`.
- Itch builds: Windows `#1931949`, Android `#1931951`.
- Route: two skeletons -> shadow corridor -> roof-water drench/original torch failure -> rounded catchment and drain runoff -> blue skylight -> four coloured bays -> open threshold -> hero mirror -> misted staff-lit lich -> two-second chest unlock/guidance -> chest open and reward-lantern claim -> opening roof -> returning dawn -> epilogue.
- The stained pane remains rejected. The water is a narrow real-geometry RT feature with bounded reflection/transmission, not a broad fluid simulation.
- Normal gameplay uses the accepted block-arm viewmodel. The reusable skinned-character, animation-layer, IK, bone-socket, CPU skin/refit, body/reflection and wall-aware held-prop foundation remains available through development checkpoints; final skinned gauntlets and arm/body shadow/reflection polish are deferred.
- Player vitality/death/retry is implemented: three points, a one-second damage lockout, short fatal hold, encounter retry, full-route restart, and platform-native death overlays.
- Two skeletons can be active with at most two pose buckets; the later CC0 lich remains singular.
- Android release packages the imported runtime GLBs, strict ASTC texture families, existing FilmCow cues, and credited Pixabay water/chest/fire cues. Windows packages the same gameplay/audio assets with its raw texture route and licence manifest.
- Warm 75% Android hands-on certification and cool deterministic automation are separate evidence classes; neither should be relabelled as the other.
- The Android Debug harness supplies 13 presets, a default six-checkpoint three-window pass, and a 13-waypoint replay. Release rejects the request path.
- A separate player-facing benchmark is release-safe on both platforms: one warm-up lap plus one measured 13-waypoint lap, followed by a selectable/copyable/exportable report and archived JSON. Windows and `SM-S948B` Android device validation pass; see the dated in-app benchmark validation records.
- The integrated `Host|Full` foundation gate is complete. It owns clean cross-platform builds/tests, stale-shader and package/licence/identity checks, validation-only artifacts and hashes, 13 standard deterministic Windows captures, and the connected-phone replay/lifecycle/capture evidence path. Current Vulkan-enabled Debug and Release configurations contain 31 tests each.
- The unreachable stained-glass material route was removed after a bounded A/B: SPIR-V structure improved, all 12 Windows images were bit-exact, Windows median timing was unchanged, and all five comparable-temperature Android 75% checkpoints improved within the required non-regression rule.
- The 1.6.0 Windows ZIP passed an isolated exact-package launch with honest RT presentation and clean exit. The exact 1.6.0 Android APK passed certificate/package guards but was not installed because no ADB device was connected; the exact accepted final Debug runtime on `SM-S948B` is supporting feature evidence, not signed-package proof.

## Historical records

The following dated families are intentionally preserved rather than rewritten: 0.1.0 release/readiness documents; Phase 1C, skeleton, PBR, combat, material, player-body, lighting and route-blockout slice evidence; the Windows audio diagnostic; and the 2026-07-13 codebase audit. Their metrics, build IDs, clip counts, and “pending” statements describe the milestone at that date. Later closure is found through the authority table above.

`NATIVE_RT_SHOWCASE_PLAN_2026-07-14.md`, `PHASE_PLAN.md`, and `COLOURED_LIGHT_ROUTE_PLAN_2026-07-15.md` now carry reconciliation notes because they are frequently used for roadmap navigation.

## Honest remaining items

- Independently back up the release JKS and both passwords.
- Keep the Hotstrike public raw-GLB/history permission question open until the creator responds or the owner chooses remediation.
- Do not raise the validated two-skeleton/two-pose-bucket ceiling without a separately designed and measured phone gate; the later lich remains singular.
- Precise waterfall stereo direction and loop seam were not separately rescored for 1.6.0; source contracts, native gains, packaging, and lifecycle behavior remain verified. The owner accepted the new chest/fire cues and haptics on the exact final Debug runtime.
- Final skinned first-person gauntlets, arm/body shadow/reflection polish, encounters above two skeletons, broader AI, held block, and broad fluid simulation remain deferred. The textured sword is now live through the measured generic GLB/PBR path.
- The developer overlay is device-validated on Windows and Android. Integrated clean-build/package/stale-shader/licence gates and fixed PNG capture are complete; video and orbit-camera presentation remain tooling deferrals.
- Future publishing must explicitly supply and bump all version surfaces and Android `versionCode > 8`; the shared release-version policy rejects the immutable published 0.1.1 through 0.1.5, 1.5.2, and 1.6.0 lines before packaging/upload work.
- No GitHub Release was created for 1.6.0. Activating updater discovery is a separate owner-authorised publication action.
- The unchecked historical `v0.1.0-alpha.1` tag task is superseded; do not create a retroactive tag on a later commit.

## Audit scope and outcome

- Re-audited the living repository authorities, package guards, licences, source/package metadata, exact artifacts, Butler status, and anonymous public itch route on 2026-08-31.
- Verified all Markdown links resolve.
- Preserved dated evidence, corrected living package commands/counts/feature descriptions, promoted the reviewed automation digest, and separated cool automation from warm certification.
- Verified current source/package version surfaces agree on 1.6.0 / Android code 8. Release source `57c81b6` produced the exact published artifacts recorded in the 1.6.0 release validation.
- Exact 1.6.0 source Host run `run-20260831-131431` rebuilt Android Debug/unsigned Release, passed Release lint, and passed fresh Windows Debug/Release 31/31 CTests plus all 13 standard deterministic captures and package/licence/evidence checks.
- The 2026-08-31 itch channel update preserves the existing public page while publishing both 1.6.0 downloads. Anonymous verification returned HTTP 200 and exposed both current artifacts. The repository page copy is prepared but no authenticated description or devlog edit was requested or performed.
