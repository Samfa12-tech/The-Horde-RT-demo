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
| Brief dark world at 28.25-28.75s | Original clip cause remains unknown. Separate Windows RT reproduction proves carried torch Light can cross a wall. Whole-hand translation and the subsequent cant/tuck trial are both withdrawn: lit stone improves, but retained diag11 loses its torch. Corrected cant contains all 60 actual-rig physical cases but fails six required wall framing checks, inherited maximum lowering and turn continuity (124.327 mm step versus 2.601 mm control). Accepted runtime is restored; all new witnesses and failures remain in compressed patches. [Latest rejection and matched RT evidence](../evidence/2026-10-23-torch-cant-withdrawn/README.md). Video frame gaps are not engine FPS. |
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
| Directional locomotion | Owned player runtime/source rigs contain Idle and forward Walking only. A bounded continuous reverse-phase candidate reduces actual stance-foot world-speed RMS from 3.36144 to 0.560598 m/s over the same 4.05322 m backward path; residual sliding remains. Initial uninterrupted forward RT images are byte-identical; blocked motion now returns to Idle. No new clip, save field, movement timing or footstep-event input. Animation/directional tests (including blocked-to-Idle) and the static first-person portal pass. Full Windows Debug selection is 226/230 across two fixed-source segments: two route deadlines and two inherited shader-hash checks fail. Exact accepted-source step control also exceeds 30 s, with identical 73-check output; final candidate CTest timeout remains retained. Exact source-changing-head CI must be read separately. Dynamic Windows first-person now passes 18/18 RT captures across backward/forward changes, dodge and idle (97 rendered fixed ticks; visible arms 511–6,232 pixels). Five focused animation/rig/lifecycle gates pass; 55 lifecycle checks cover backward into both actual rope directions and restore/retry/pause/reset without runtime changes. Rope remains host motor evidence, not an earned/native RT journey; phone feel and an authored backward run remain unaccepted. [Continuity checkpoint](../evidence/2026-10-23-backward-continuity/README.md). [Evidence and retained initial failures](../evidence/2026-10-23-backward-walk/README.md). No reflection-renderer fault is demonstrated. |
| Meshy assessment | Kit source/runtime already retain idle/walk/run/fall/jump/mantle/slide; runtime selects idle/walk. Same-rig `Warden|run` is a worthwhile genuine running/catch-up candidate, not a pivot shortcut. No inspected crouch transition exists. Foreign skeleton backward/turn clips need retargeting and retain unresolved redistribution limits. No import/generation, playback or cost acceptance. |
| Existing optimizations | Player normal caching, enemy refresh and BLAS UPDATE already exist. Current phone skin-stage wall median remains 12.06 ms (player 6.05/character 5.92); old 8.7 ms is stale. Six CPU child stages now separate pose/palette, world/viewmodel vertices and character copies; upload/AS-record/waits remain separate. One generic-character reuse of the existing transport helper reduces laptop Kit vertex medians 6.73–8.33% versus closing control, but misses its 1.55 ms target at 1.5567 ms. Retained candidate, one iteration exhausted; no frame or Android gain claim. Two frozen RT pairs match exactly. [Evidence](../evidence/2026-10-22-skin-attribution/README.md). GPU AS execution and per-owner attribution remain open. First-blocker/Opaque inverse-transform closed without adoption; PineLOD2 rejected, LOD1 retained. |
| Mobile quality | MistOff default and optional enable remain; saved choices are preserved, desktop unchanged. Explicitly requested candidate comparisons are RT ON,40%,MistOff,FireLow,DustOff,Current shadows,existing Mobile water, individual and combined. Prior mixed trials are not attributed wins. 480p was illustrative; native HUD remains display-resolution/readable. |
| Natural acceptance | Natural Keeper victory, full journey, earned save/load/death recovery, actual listening and sustained warmed-phone performance remain open. Passing Windows post-Keeper continuation fixtures are parked. |
| Kit/ambience/debris | Stationary turn-step trial withdrawn/unadmitted; foot sliding remains. Night ambience/finite foliage source exists but listening is unaccepted. Six floor chips are static; kicking and a player-facing toggle are absent. |
| Outdoor music | **Full approved main implemented locally through the existing shared PCM engine.** Canonical Core `b77a9d0b` is vendored byte-exact through GitHub; the denied sibling folder was not retried. First chapter ascent preserves G then selects the full 106.667-second Outdoor cue; return F and legacy showcase/dawn H remain. One main uses existing dialogue ducking; no quiet duplicate or source master ships. Core Debug/Release, 8 focused Horde gates, 25 admission cases, full Windows application and four-ABI Android build pass. Java: 405 executed, zero failures/errors, 6 existing skips. Actual XAudio2 consumed a complete loop at gain zero and passed pause/resume/retry/join; this is not listening. Bank PCM is 40,640,000 bytes; one host load 387.434 ms, not phone timing/RSS. Main payload is 20,480,044 bytes stored in APK or 13,546,616 compressed in a Windows-style ZIP fixture; actual incremental Debug APK is 200,120,444 bytes including a separately identified 5,410,380-byte archive gap. Original source transfer/policy failures remain retained. Actual Kit mix, transitions/seam listening, phone loading/lifecycle and warmed cost stay open. [Integration and size evidence](../evidence/2026-10-23-outdoor-main/README.md), [source provenance](../evidence/2026-10-23-outdoor-music-source/README.md). |

## Hosted baseline and final review

Outdoor integration `f21b030e` is backed up on PR 27. Its first hosted run
[39896071822](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39896071822)
fails the same dialogue-ducking fixture in GCC and Clang (156/157 each; 18 checks
fail inside that registration). Sanitizer, Vulkan-host and Android jobs pass;
Android executes all 411 Java tests with no skips. Local reproduction identifies
an invalid synthetic Outdoor tail, corrected to per-cue metadata with all
original assertions retained; 161/161 Debug checks then pass. No runtime audio
change. The correction's exact-head hosted result is separate; retain the first
failure, inherited Debug route gates and PR conflicts. See the integration
record's hosted-failure section for logs.

Current pre-integration heads `b9b98fb966327656cace019e911488a2d28f3bed`
and `5b9dc70583eb48b314ce4105c326b2216ac810c8` each completed **six of six
hosted jobs successfully**, including MSVC's main selection and subsequent
offline checks: [39887604991](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39887604991)
and [39888357455](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39888357455).
The older failures/cap cancellations below remain historical evidence. These
passes do not certify the new outdoor integration; its exact changed-head result
must be recorded separately. Local focused music results do not close unrelated
Debug route deadlines, physical/audio acceptance or PR conflicts.

[CI run39482295647](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39482295647)
tests exact3e29b599 and fails multiple lanes. Confirmed checkout/fixture issues
include omitted LFS ambience/foliage payloads, a Windows cp1252 read of UTF-8
licence text, stale terrain/root and resource-inventory expectations, and a
staging contract which does not follow its extracted helper. Native pine223-225
Android name/pose mapping now passes locally; physical views are not verified.
Waterfall production-route tests now pass locally with automatic crouch and
unchanged route assertions; hosted current-source rerun remains. The eight
CharacterRenderSlotSmoke source failures are now corrected locally as stale
regional-water expectations, with actual-source numeric checks and negative
mutations. Three focused Release tests and Windows build pass; no runtime
sources change. [Diagnosis and retained failures](../evidence/2026-10-22-source-contracts/README.md).
The [checkout/fixture diagnosis](../evidence/2026-10-22-ci-checkout/README.md)
now corrects the missing PCM fetch/checkout roots, explicit UTF-8 reads,
file-specific LF checkout, one terrain hash/pin, stale exact terrain/debris
content totals, helper staging and nested timing-fixture clock. Four Release
registrations (including 199 archive cases) and two fresh native fixtures pass.
All capacities, tree placements and renderer/PCM payloads are unchanged; original
failures remain. Corrected hosted all-lane results remain pending. Do not label
all hosted failures runtime bugs or relax gates. Debug campaign 30 s timeout
remains open. PR description/conflict
and final exact-candidate checks remain; no merge.
The separate [API 24 survey-export correction](../evidence/2026-10-22-survey-api24/README.md)
preserves exclusive creation/path custody while removing API 26 calls at min 24.
Local Java results: 405 passed, 6 host-link cases skipped; lint zero errors and 75
warnings. Focused API 24/34 modeled syscall contracts pass; actual device export
and Linux link-case coverage remain pending. No phone operation.

Approved standardized benchmark/report/leaderboard plan remains implementation
scope after immediate regressions. Require fixed actual internal **and** output
dimensions/aspect, distinct mobile/Windows profiles, DRS off, custom unranked
runs, version/profile cohorts, sample counts, medians/ranges, transparent outliers
and consent/private raw reports. Random report IDs are not unique-device counts;
timing proxies are not measured scanout FPS. Weekly publication is planned, with
no active publication schedule.

## Concrete next local slice

Latest torch refinement is closed without adoption. Joint physical clearance,
visible hand/torch and continuous-turn behavior remain an implementation gap;
no further torch candidate is active. The existing wall witnesses and ordinary
centre pose stay required. The bounded backward-walk checkpoint is recorded
above. Bounded dynamic Windows first-person and backward-to-dodge/rope host gates
now pass with the same rig and movement rules; see the continuity checkpoint.
The source-changing head `90a65044` hosted run 39884452538 finishes cancelled: five
jobs pass; MSVC hits its unchanged 15-minute cap with no surfaced final test total,
and six subsequent offline checks are skipped. Hosted acceptance stays open.
The owner-supplied Drive originals are preserved and the full main is now
integrated as recorded above. Its Core prerequisite is resolved through the
exact authorized canonical commit, with source exclusion and bounded memory
checks. Actual listening and phone acceptance remain open. The earlier Library
metadata failure remains retained; the separate Drive source does not bypass
Library requirements. No phone work was performed. The next unblocked approved
player-facing slice is inspection of the actual treasury-road/chimney vista,
then at most a justified bounded scenery correction. Preserve route boundaries,
landmark visibility and safe backtracking; no scenery edit has started.

Exact evidence-only head `096b23f0`, [run 39877269271](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39877269271),
finishes **failure**: five jobs pass; MSVC main selection is 168/169, with the
native report UI timing out at 30.02 s before the initial note-only form becomes
visible. This does not prove a Unicode WM_SETTEXT defect. The later trace
collector's "did not execute" text conflicts with actual CTest execution and
does not close diagnostics. The original failure and deadlines are retained.

At accepted source 4eeafcb6, hosted run 39871548947 completes **cancelled**: five
jobs pass; MSVC exceeds its unchanged cap of 15 minutes after a successful build,
169/169 main tests and the separate asset-policy test. Six subsequent offline
Windows steps are skipped. Hosted acceptance is incomplete; the cap and all
test gates remain unchanged. No base integration or merge is applied.

Automatic crouch/deflection source, the bounded desktop skin-work slice and Kit's
continuous source/listener correction are complete locally. Physical input/feel,
endpoint speech/listening and phone performance remain open. The bounded
shader/Android source-contract diagnosis is complete locally. Next inspect
corrected hosted checkout/fixture results before safe PR conflict resolution;
this does not claim the full hosted programme passes.
For the next owner-confirmed phone connection, compare exact control/candidate
builds with the appended phase counters and original matched hold/settings.
Keep the 12.06 ms phone stage, unresolved closing 33% slowdown, other inherited
failures and one-candidate target miss explicit. Desktop results do not
certify Android benefit.
Separately retain cold207 missing-terrain diagnosis for the next fixture/device
window; do not infer it caused the owner's brief dark-world video.
Kit listening and physical resume remain open after the source correction. No
phone restart is needed for local source work.
