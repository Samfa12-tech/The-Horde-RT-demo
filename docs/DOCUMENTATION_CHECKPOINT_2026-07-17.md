# Documentation checkpoint - 2026-07-17

## Purpose

This is the consolidation map after Showcase Alpha 0.1.1 publication and the first live Android automation runs. It distinguishes current authority from dated implementation history so old “pending” language is not mistaken for the present backlog.

## Current release authority

| Claim | Authoritative record |
|---|---|
| Published artifacts, hashes, certificate, itch builds | `SHOWCASE_ALPHA_RELEASE_VALIDATION_2026-07-17.md` |
| Public release contents and known limits | `SHOWCASE_ALPHA_RELEASE_NOTES_2026-07-17.md` |
| Windows route, visual, combat, mirror and audio validation | `HORDE_SHOWCASE_WINDOWS_VALIDATION_2026-07-16.md` |
| Android hands-on route, lifecycle and warm sustained measurements | `HORDE_SHOWCASE_ANDROID_VALIDATION_2026-07-17.md` |
| Android deterministic checkpoint/replay procedure | `ANDROID_SHOWCASE_AUTOMATION_2026-07-17.md` |
| Android cool/status-0 automation baseline | `ANDROID_SHOWCASE_AUTOMATION_VALIDATION_2026-07-17.md` and `validation/android-showcase-automation-2026-07-17/` |
| Asset provenance/licences | root `ASSET_LICENSES.md` |
| Remaining tooling backlog | `BUILD_TEST_DEMO_CYCLE_PLAN_2026-07-17.md` |
| Day-to-day agent constraints | root `AGENTS.md`, `PROJECT_MEMORY.md`, and `PROJECT_DECISIONS.md` |

## Consolidated current state

- Public version: Showcase Alpha 0.1.1 / `0.1.1-alpha.1`; Android `versionCode 2`.
- Itch builds: Windows `#1801016`, Android `#1801017`.
- Route: skeleton -> shadow corridor -> lantern drop -> blue skylight -> four coloured bays -> open threshold -> hero mirror -> staff-lit lich -> sliding roof.
- The stained pane was rejected and removed. Water remains deferred.
- Player body includes torso, articulated arms, procedural lower body/gait, head shadow/reflection, and wall-aware props.
- Skeleton and CC0 lich are selected sequentially; only one skinned enemy is animated/refit/rendered at once.
- Android release packages both enemy GLBs, strict environment/lich ASTC, and 17 FilmCow cues. Windows packages both enemies, raw textures, 17 cues, and the licence manifest.
- Warm 75% Android hands-on certification and cool deterministic automation are separate evidence classes; neither should be relabelled as the other.
- The Android Debug harness supplies 12 presets, a default five-checkpoint three-window pass, and a 13-waypoint replay. Release rejects the request path.

## Historical records

The following dated families are intentionally preserved rather than rewritten: 0.1.0 release/readiness documents; Phase 1C, skeleton, PBR, combat, material, player-body, lighting and route-blockout slice evidence; the Windows audio diagnostic; and the 2026-07-13 codebase audit. Their metrics, build IDs, clip counts, and “pending” statements describe the milestone at that date. Later closure is found through the authority table above.

`NATIVE_RT_SHOWCASE_PLAN_2026-07-14.md`, `PHASE_PLAN.md`, and `COLOURED_LIGHT_ROUTE_PLAN_2026-07-15.md` now carry reconciliation notes because they are frequently used for roadmap navigation.

## Honest remaining items

- Independently back up the release JKS and both passwords.
- Keep the Hotstrike public raw-GLB/history permission question open until the creator responds or the owner chooses remediation.
- Do not raise the one-active-skinned-enemy limit without a dedicated phone measurement.
- Android audio events execute on-device, but perceived stereo directionality/distance is not separately certified.
- Water, authored hands/full character rig, broader AI, block/dodge, player death, and the staged textured sword runtime remain deferred.
- Developer overlay, integrated cross-platform clean-build/package/stale-shader/licence gates, and fixed video/presentation capture remain tooling work.
- Future publishing must bump all version surfaces and Android `versionCode`; the current package scripts default to the immutable published 0.1.1 identity and must not be used to republish changed source under the same version.
- The unchecked historical `v0.1.0-alpha.1` tag task is superseded; do not create a retroactive tag on a later commit.

## Audit scope and outcome

- Audited all 59 Markdown files plus the Windows release README and current source/package metadata.
- Verified all Markdown links resolve.
- Preserved dated evidence, corrected living package commands/counts/feature descriptions, promoted the reviewed automation digest, and separated cool automation from warm certification.
- Verified current source/package version surfaces agree on 0.1.1 and current local work is based on published commit `08194759dfc96fb6a0007012f7c6d834d71289d9`.
- Rebuilt Android Debug/Release, passed Release lint, and verified the automation-only `singleTop` launch mode exists in Debug's merged manifest but not Release's. Windows Debug/Release builds and all five CTests pass.
- The live itch editor was audited: uploads/platform flags, public visibility, tags, AI disclosure, install instructions, and licences were sound. The stale long description/tagline were replaced in the editor with the consolidated 0.1.1 copy and correct warm/cool performance wording; the external Save action remains pending explicit owner confirmation.
