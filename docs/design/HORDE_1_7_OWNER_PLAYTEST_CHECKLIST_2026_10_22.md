# Horde 1.7 owner playtest checklist - 22 October 2026

Current checklist after resumption from `3e29b599`, in the existing WP2 worktree,
branch and draft PR 27. Owner assigns the phone to Horde until approximately
20:00 UTC; release target was **19:55 UTC / 06:55 AEDT**. Released at19:44 UTC,
with all operations stopped and custody verified. Device connectivity was
verified as SM-S948B / Android 17 API 37. Preserve RT-only hardware rendering,
owner campaign/settings, independent test namespaces and review boundaries.
No release, merge, paid generation or spending is authorized.

## Immediate playtest and performance

| Item | Actual state / remaining work |
| --- | --- |
| Landscape captions at 100% Auto | Earlier712f phone build shows one real complete Kit question at100% Auto/font1.7 without overflow diagnostic, but residual control overlap remains. Source now fixes cutout coordinate handling and removes visible Skip under later owner direction. Nine caption tests, two icon tests and all394 Java tests pass. Final UI cold207 fails missing-terrain before inspection; corrected final portrait/landscape acceptance remains pending. [UI checkpoint](../evidence/2026-10-22-caption-ui/README.md). |
| Kit quiet at Speech100/SFX20/Music55 | Speech is independent of SFX; healthy PCM is retained. Earlier exterior-domain correction preserves .36 base gain and ducking. Continuous reunion/forest speech now follows the current grounded Kit and listener/yaw; Windows updates the exact existing voice matrix without restarting PCM. Actual terrain-backed nearby reminder distance changes from the obsolete 5.34 m point to the real 1.00 m actor; landing correction can lower gain because Kit is farther away. Four focused Release tests, Windows app and Android arm64 native builds pass. Endpoint playback, actual listening/mix and phone pause/resume remain pending. [Anchoring evidence](../evidence/2026-10-22-kit-live-voice/README.md). |
| Touch clutter / auto crouch / speed | Shared automatic head-clearance crouch and touch/controller deflection speed are implemented; keyboard Shift/Caps Lock retained. Ordinary Run/Crouch buttons and manual stance bindings are inactive, removing the truncated Running label. Seven native tests and 97 focused Android tests pass, including collision/step/roof negatives and input/lifecycle. Two blocked transition ticks occur on entry in the terrain fixture; physical play feel and final portrait/landscape UI remain pending. [Checkpoint](../evidence/2026-10-22-auto-crouch/README.md). No-Skip and compact56dp lantern icon remain unaccepted on phone. Briarhold circular/translucent reference reviewed read-only; broader redesign deferred by owner19:35. |
| Brief dark world at 28.25-28.75s | Original owner clip is local/ignored and original frames retained. HUD remains visible; faint world geometry is visible in the inspected dark frame. Cause is unknown; no asserted presentation/lighting/camera diagnosis or runtime fix. Video frame gaps are not engine FPS. |
| Outdoor 20/33/40% | Owner33%/MistOff preserved. New exact-build live, muted, stationary holds measure detailed CPU stage wall times, GPU and actual image presentation intervals.20 GPU16.08ms/CPU cycle31.91ms; initial33 controls GPU35.49–35.51/CPU47.75–47.86;40 GPU48.20/CPU64.71. Closing33 GPU43.93/CPU58.28 is an unresolved greater-than15% rise despite start thermal gate.20 is visually unadmitted, not a default/recommendation. No OS/thread CPU utilisation or separate animation/AS profiling. [Receipt](../evidence/2026-10-22-outdoor-scale/README.md), [reconstruction readiness](HORDE_1_7_UPSCALER_READINESS_2026_10_22.md). Earlier frozen CPU means40.338/40.359/40.362 remain narrower separate evidence. |
| Scenery for road/smoke lines | Actual treasury-road/chimney-smoke cues remain unaccepted/unimplemented. Inspect current vista before adding cues. Preserve planned northwest lookout to southeast Bellwether entry, no river crossing, vista, safe backtracking and terrain bend/occluder for forest unload/town load. No playable-town expansion. |

Phone released19:44 UTC with Speech100/SFX20/Music55/scale33/MistOff byte-exact,
all18 prior save/lock hashes unchanged and zero new save paths. Original font1.7
and portrait rotation0 match; production APK unchanged. Ordinary Quit leaves
launcher/no Horde render surface and no active install/capture/transfer. Debug
may remain cached with no activity. No further phone work is pending. Staged,
frozen or muted tests do not establish natural victory/full route/listening.

## Saved commitments and implementation gaps

| Commitment | Current state / boundary |
| --- | --- |
| Healing flask | Owner approved **refillable and upgradeable**. Charges, healing quantity, refill locations, upgrade progression, first availability, use/interruption, death and save migration remain unspecified. No healing implementation. The earlier one-charge/one-heart/rope-landing suggestion is archived and unapproved. [Recovery record](HORDE_1_7_RECOVERY_DECISIONS_FOR_MORNING_2026_10_20.md). |
| Directional locomotion | Owner saw backward movement playing forward-running in a mirror. Inspect runtime clip selection and existing backward/turn sources before replacement. No reflection-renderer fault is demonstrated. Preserve weapon/contact, rope and IK ownership. |
| Meshy assessment | Kit source/runtime already retain idle/walk/run/fall/jump/mantle/slide; runtime selects idle/walk. Same-rig `Warden|run` is a worthwhile genuine running/catch-up candidate, not a pivot shortcut. No inspected crouch transition exists. Foreign skeleton backward/turn clips need retargeting and retain unresolved redistribution limits. No import/generation, playback or cost acceptance. |
| Existing optimizations | Player normal caching, enemy refresh and BLAS UPDATE already exist. Current phone skin-stage wall median remains 12.06 ms (player 6.05/character 5.92); old 8.7 ms is stale. Six CPU child stages now separate pose/palette, world/viewmodel vertices and character copies; upload/AS-record/waits remain separate. One generic-character reuse of the existing transport helper reduces laptop Kit vertex medians 6.73–8.33% versus closing control, but misses its 1.55 ms target at 1.5567 ms. Retained candidate, one iteration exhausted; no frame or Android gain claim. Two frozen RT pairs match exactly. [Evidence](../evidence/2026-10-22-skin-attribution/README.md). GPU AS execution and per-owner attribution remain open. First-blocker/Opaque inverse-transform closed without adoption; PineLOD2 rejected, LOD1 retained. |
| Mobile quality | MistOff default and optional enable remain; saved choices are preserved, desktop unchanged. Explicitly requested candidate comparisons are RT ON,40%,MistOff,FireLow,DustOff,Current shadows,existing Mobile water, individual and combined. Prior mixed trials are not attributed wins. 480p was illustrative; native HUD remains display-resolution/readable. |
| Natural acceptance | Natural Keeper victory, full journey, earned save/load/death recovery, actual listening and sustained warmed-phone performance remain open. Passing Windows post-Keeper continuation fixtures are parked. |
| Kit/ambience/debris | Stationary turn-step trial withdrawn/unadmitted; foot sliding remains. Night ambience/finite foliage source exists but listening is unaccepted. Six floor chips are static; kicking and a player-facing toggle are absent. |
| Outdoor music | Owner-approved original **Beneath the Same Moon - Outdoor r2** is available by Library identifiers, not yet materialized in this worktree. Main/quiet bed are aligned48kHz stereo5,120,000 samples,106.667s per parent verification; in-game Kit mix remains unvalidated. Outdoor-only integration, no global music replacement. |

## Hosted baseline and final review

[CI run39482295647](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39482295647)
tests exact3e29b599 and fails multiple lanes. Confirmed checkout/fixture issues
include omitted LFS ambience/foliage payloads, a Windows cp1252 read of UTF-8
licence text, stale terrain/root and resource-inventory expectations, and a
staging contract which does not follow its extracted helper. Native pine223-225
Android name/pose mapping now passes locally; physical views are not verified.
Waterfall production-route tests now pass locally with automatic crouch and
unchanged route assertions; hosted current-source rerun remains. Shader-source contract and Windows
generated-bound mismatch need exact artifact/encoding diagnosis. Do not label
all hosted failures runtime bugs, remove assertions, or move geometry to match
stale receipts. Debug campaign30s timeout remains open. PR description/conflict
and final exact-candidate checks remain; no merge.

Approved standardized benchmark/report/leaderboard plan remains implementation
scope after immediate regressions. Require fixed actual internal **and** output
dimensions/aspect, distinct mobile/Windows profiles, DRS off, custom unranked
runs, version/profile cohorts, sample counts, medians/ranges, transparent outliers
and consent/private raw reports. Random report IDs are not unique-device counts;
timing proxies are not measured scanout FPS. Weekly publication is planned, with
no active publication schedule.

## Concrete next local slice

Automatic crouch/deflection source, the bounded desktop skin-work slice and Kit's
continuous source/listener correction are complete locally. Physical input/feel,
endpoint speech/listening and phone performance remain open. Next desktop work
can diagnose inherited shader/Android source-contract failures against actual
implementations, retaining their gates and PR conflicts for review.
For the next owner-confirmed phone connection, compare exact control/candidate
builds with the appended phase counters and original matched hold/settings.
Keep the 12.06 ms phone stage, unresolved closing 33% slowdown, inherited red
contracts and one-candidate target miss explicit. Desktop results do not
certify Android benefit.
Separately retain cold207 missing-terrain diagnosis for the next fixture/device
window; do not infer it caused the owner's brief dark-world video.
Kit listening and physical resume remain open after the source correction. No
phone restart is needed for local source work.
