# 1.6.2 tomb finish — active post-reset work

**Owner combat disposition — 9 October:** bounded moving-target witness `5a2a4661` preserves the positive visible blade/mesh gaps: about216 mm frontal and18 mm at −15° at the current damage pulse. Late counterfactual samples do not support a uniform delay. The owner explicitly chooses **keep current combat for1.6.2 and document the forgiving hit-detection limitation**; precision animation/contact correction is deferred. Existing shared60Hz timings, range/cone, immediate riposte and parry feedback remain unchanged. This is a scope/acceptance decision, not an exact-contact pass. [Exact queries, artifacts and limitations](ENGINEERING_1_6_2_MOVING_CONTACT_2026_10_09.md).

**Integrated stand/default checkpoint — 9 October:** source `ac97da91` adds shared physical collision for the existing two Keeper torch stands, preserves full-height wall acoustics, and sets fresh/staged-reset Dust Low on Android and desktop without overwriting saved Off/custom or genuine legacy preferences. Mobile resolution remains **50%**; 33%/40% stay experimental. Seven affected host suites, 256 Java tests/39 classes, lint, four-ABI/Windows builds and package checks pass. Exact SM-S948B/Android 16 install/pullback and normal entry pass with settings/menu mix unchanged; the owner confirms ordinary player/Keeper stand collision works as intended. Owner waterfall, audio/haptics and 33% manual play acceptance are recorded; the long sustained phone programme is owner-deferred. Exact ac97 runtime CI passes12/12 aggregate jobs; later test-only/documentation heads have separate CI. [Exact source/packages, regressions and limits](ENGINEERING_1_6_2_STAND_DUST_2026_10_09.md).

**Owner-accepted torch-water correction — 9 October:** exact `8edca4bb` bounds fire diffuse by the existing entrained-air fraction at the water material, preserving shared opaque lighting, warm highlights and Fresnel transport. 255 Java tests/39 classes, lint, four-ABI/Windows builds, three affected host checks and all 16 backend shader variants pass without widening frozen budgets. Sixteen matched phone pairs and eight Windows pairs cover near/far/oblique/catchment views on both hardware RT backends; saved settings/mix and captured resource payloads are unchanged. The owner accepts the waterfall appearance. Retained setup failures and owning-frame GPU observations are scoped explicitly; no sustained-FPS or causal performance claim. Integrated runtime `ac97da91` includes this refinement and passes12/12 aggregate CI jobs, separately from the isolated zero-diffuse source's12/12 pass. [Exact source/packages, images and limits](ENGINEERING_1_6_2_TORCH_WATER_2026_10_09.md).

**Current owner decisions — 9 October:** ordinary dungeon play at experimental 33% is owner-approved as surprisingly good; 50% remains the mobile resolution default. The owner now requests Dust **Low by default in the final packaged build**, preserving saved Off/custom preferences; shared fresh/reset defaults and the stand collision fix pass integrated host/Java/build checks in `ac97da91`; exact normal-phone install passes and the owner accepts the stand collision. This supersedes the earlier prototype-Off requirement. Owner audio and haptics are approved. The long sustained phone performance/thermal programme is deferred from this goal at the owner's direction; retain exact measured gaps and do not claim sustained 30 FPS. The torch-water correction is owner-approved; the reported player/Keeper stand clipping fix passes integrated host checks and is installed and owner-accepted in ordinary phone play. Final integrated checks and Eric's independent audit remain required. No merge or release is authorised.

**Ordinary experimental choices — 9 October:** source `40677738` adds clearly labelled 33%/40% choices to Android and Windows Graphics for owner manual play. Fresh/reset phone default stays 50%; saved/custom preferences and Use/native ACK/Keep/Restore remain authoritative. 252 Java tests, lint, both builds, seven affected host suites and 12/12 runtime CI jobs pass. Exact normal-phone installation and both preview/Restore paths retain saved 50/custom effects and menu mix. The build is reopened for manual play; sustained performance and any upscaling decision follow feedback. [Exact artifacts and limits](ENGINEERING_1_6_2_EXPERIMENTAL_SCALES_2026_10_09.md).

**Motion comparison and owner decision — 9 October:** exact `a6439e0b` Shipping/Mobile APK completes matched-start 50/40/33 runs on SM-S948B/Android 16, real Pipeline, output 1440×2980, with native traced extents 720×1490 /576×1192 /475×983. Short ~6.5-second actual-presentation spans measure 24.288 /31.205 /46.171 images/s respectively, including milestone-readback overhead; these are not sustained FPS. Three separate encoded moving clips are shown, and the owner accepts **33% appearance for further sustained testing**. The phone default remains **50%**. Isolated historical 75/Mobile/Glass On is restored through normal ACK/Keep; primary settings/menu mix are byte-identical, owned apps/recorders stopped. Earlier thermal refusal remains recorded. Runtime CI is 12/12; sustained ordinary-play pacing/thermals, Keeper/reward overlap and final profile acceptance remain open. [Exact cohort, hashes and limits](ENGINEERING_1_6_2_MOTION_PRESENTATION_2026_10_09.md).

**Bounded contact investigation — 9 October:** test-only `f0cd8cfc` adds an opt-in imported-mesh witness, not production combat changes. One local Windows run makes 24 exact pose queries: frontal pulse 17 gap 275.347 mm (minimum 36.289 mm by tick 23); −15° pulse gap 234.658 mm, with intersections at 19/20/23; idle control stays separated. Discrete bearing-dependent contact does not justify a uniform timing shift, wider parry window or new sweep/collider. Local investigation passes; its explicit target is excluded from ordinary builds/CI execution, while source CI separately passes 12/12. Moving-contact calibration remains open. [Exact rows, source and interpretation](ENGINEERING_1_6_2_CONTACT_REGIONS_2026_10_09.md).

**Heart-only HUD checkpoint — 9 October:** runtime `7766ea3a` removes visible vitality text on Android and Windows, using red full hearts and hollow lost-point outlines. Accessible current/max health remains; the icon count follows maximum health and can wrap for future increases. Health/gameplay rules and all 94 runtime assets are preserved. 243 Android tests/38 classes, lint, four-ABI and Windows builds plus three affected host checks pass. Exact SM-S948B / Android 16 installation/pullback and ordinary Play confirm the text-free HUD; game settings/menu mix are unchanged and the owned app stops. This UI checkpoint does not close physical damage/orientation/controller, Windows appearance or sustained-performance gates. [Exact artifacts, image and limits](ENGINEERING_1_6_2_HEART_HUD_2026_10_09.md).

**Earlier warm quality observation — 8 October:** on the same exact isolated 2a408613 benchmark APK, newly traced 50% / Mobile / Glass Off / Current / cap30 / Mist On / Dust Off completes the two-lap route with 1,838 exact measured timestamp joins. A warm 82.137502104-second span supplies 22.364936 actual presented images/s, 41.666667 ms median and 75 ms p95 intervals; thermal status is 1. This is a measured short-route gap, not thermally matched/sustained ordinary play or owner quality acceptance. The isolated original 75% save is restored and owned app stops. 40/33 comparison, sustained/physical gates and owner decisions remain pending. [Exact observation and limits](ENGINEERING_1_6_2_MOBILE_QUALITY_MEASUREMENTS.md).

**Earlier presentation-timing checkpoint — 8 October:** runtime `2a408613668a4936dd1592d79cd141e1d6baf5dc`, isolated Shipping/Mobile benchmark APK `faf35f482b78150b7157335b56c3a0e5c499ec1d2205b9dc6a82bd90ce82992f`. Optional actual image timestamps join all 600 lantern and 1,838 complete-route measured frames on SM-S948B / Android 16 at the benchmark’s preserved 75% profile. Collector/retirement fixtures, admission/build/package checks and 12/12 runtime CI jobs pass; nine offline analysis tests pass. Ordinary builds remain Off, assets/defaults/custom saves are preserved, and owned apps are stopped. This establishes measurement, not 50/40/33 or sustained-30-FPS acceptance. [Exact subject, intervals, retained failures and remaining gates](ENGINEERING_1_6_2_PRESENT_TIMING_2026_10_08.md).

**Earlier owner-accepted grate floor checkpoint — 8 October:** runtime `7c9b8779f1eb0af9d20e4f1d326d92f2d8a3f412`, Debug APK `c7074c790d30563fb920415f9e879a5720f2af062096d5b69adb83a785d9420d`. One closed stone base fixes the confirmed downward sky gap, preserving the bars, upper light well and shared collision. Five host fixtures and 241 Android tests/38 classes, lint, builds and package checks pass. Eight Windows and eight phone captures cover lower/oblique/upward views on both real RT backends; phone route/recovery and saved-settings checks pass. The owner accepts the appearance. Test-only `5301e4a7bcc8cbe280d6277ea183deb3d1178d92` corrects a stale unknown-pose assertion without changing the accepted APK; original runtime CI failures remain recorded and corrected-source CI is separate. Broader 1.6.2 gates remain open. [Exact grate evidence and limits](ENGINEERING_1_6_2_GRATE_FLOOR_2026_10_08.md).

**Earlier owner-accepted menu cue — 8 October:** exact source `ddd5a453f7adb4e34fca7ff72e43a064eb84428a`, Debug APK `1ab10e1c547e12e3f2e0bd2df77dd820c858b7279a04d1852d59a159f173eee0`. The supplied Irhouen Metal Creaks replaces Hammy01; flame remains fully removed and Room 7%/creak 21% are preserved as the starting mix. Five affected host tests, 240 Java tests/37 classes, lint, four-ABI/Windows builds and 145 asset cases pass. All 94 assets are verified, with 91 prior assets and Android code/native/manifest unchanged from completed f448. Exact phone install and one room loop/spaced creak submissions pass the bounded ownership check; the owner accepts the new sound/level. The verified owned app is stopped, with main settings and saved mix unchanged. Exact runtime source CI passes 12/12 aggregate jobs; later documentation CI remains a separate record. Earlier 7e/f448 CI passes 12/12 each; retained failures stay recorded. Windows listening, physical competing focus and larger 1.6.2 gates remain open. [Exact artifacts, selection and remaining limits](ENGINEERING_1_6_2_MENU_AMBIENCE_2026_10_08.md).

**Earlier installed menu ambience checkpoint — 8 October:** runtime `fa5f46a7cf01e2492593dc26ad1c50883ab18280`, Debug APK `38f5abab55163be0e548a22927a51921dd8d31cd58825838bd6eaf0dff69bf8a`. Quiet owned flame/room candidates and occasional actual-motion-timed chain creaks follow SFX/focus and the native Play fade. Five affected host tests, 239 Java tests/37 classes, lint, four-ABI and Windows builds pass. Exact SM-S948B / Android16 install and ordinary Settings/More/Home/Play checks preserve preferences and stop the owned app. **Owner accepts Settings/More continuity and Play fade; the room bed needs a much quieter balance. Owner-requested live mix sliders/Save are being prepared.** Original source CI fails six aggregate host jobs on the stale asset-count fixture; test-only `45bd4d40` passes 149 admission cases locally and 11/12 aggregate CI jobs, with one pre-build SDK CMake archive-extraction failure retained. This is an implementation checkpoint, not the final review candidate. [Exact artifacts, checks, retained failure and listening limits](ENGINEERING_1_6_2_MENU_AMBIENCE_2026_10_08.md).

**Earlier owner-tested RT recovery checkpoint — 8 October:** source `ac468f5136d978152a9953f490a631453a037c89`, Debug APK SHA-256 `3a4a7f16c606d442d825ddfb9c2720ed98f57da147940495a29f5cf03a5850b3` (138,462,724 bytes), exact SM-S948B / Android 16 install/pullback with zero preference changes. Brief Activity pauses suspend the valid RT generation; full Stop/destruction still retires it. Four affected host checks, 232 Java tests/36 classes, lint, four-ABI and Windows builds pass. Controlled Home/full-retirement/resume presents on both phone backends with saved settings unchanged. The Windows EXE is byte-identical to the retained 6c8 capture binary. **Owner BB-51 reconnect check passes: seamless attachment/reconnection, with matching same-generation suspend/resume logs.** The full controller matrix remains open. Source CI [push 37747195457](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37747195457) / [PR 37747203704](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37747203704) passes 12/12 jobs after one unchanged Windows report-dialog CI retry. The original timeout is retained; the same source passed the PR run and the focused local check. [Exact recovery evidence and cache backend split](ENGINEERING_1_6_2_ANDROID_PIPELINE_RECOVERY_2026_10_08.md). Minor Torch walking-arm wiggle is retained/deferred at the owner's request; earlier appearance/route feedback remains bound to its package. Sustained phone quality, moving secondary views, remaining physical controller matrix and Eric's independent audit remain open. No release approval/publication.

The owner confirms seamless BB-51 reconnection in this package. Owned logs show two brief pauses (102/61 ms), successful native idle/park and same-generation resume with no teardown. The first interval has a subsequent presented marker; the second has no repeated marker, so continued visual play is owner-reported. Later genuine background destruction still cancels the generation. Saved preferences remain unchanged; the owned app is stopped after retaining the logs. [Sanitized receipt](evidence/2026-10-08-android-rt-recovery/receipts.json).

**Earlier owner-tested repair candidate — 8 October:** source `6c8ecc39e63f17d4716da0d8f6e994cb872831a5`, tree `fb032369288002ac9f5baed2051a9ef1bbc9faa1`; Debug APK SHA-256 `f178daf46431f28d2aa8f997dc178274ec9653aca182e992f27f127765cbd945` (138,462,724 bytes). Exact installed-base pullback on SM-S948B / Android 16 matches, with zero preference changes and free rotation retained. This adds a measured 10° preferred elbow bend for the held original Torch and passive Debug lifecycle logging. Both portrait/wide 3,287-pose rig matrices, affected host checks, 225 Java tests/lint, four-ABI and Windows builds pass. The sealed Windows EXE `a87067220a1ab80ff1cb421f321ade19c4d4ac5f79c7b856a60f054ebb2ae5c2` passes 13/13 Pipeline and 13/13 Compute frozen checkpoints, all 26 frame/image and 52 actual uploaded mesh joins, zero validation errors. These are static desktop captures, not phone motion/performance acceptance. Current source CI is tracked separately in the controller ledger.

**6c8 owner feedback:** walking still wiggles with no noticeable difference; retain it as a minor residual issue and defer further tuning at the owner’s request. Parry/low ceilings pass their check; natural empty-right-hand rest remains tentative. Slow BB-51 reconnect startup is still unresolved; the new logs catch brief onPause teardown on the same Activity with a valid surface, without changing background suspension or Graphics acknowledgement rules. No manual resheath action is exposed. Full controller recovery, moving equipment/body/shadows/reflections, combat contact and sustained phone quality/cost gates remain open. [Exact record](ENGINEERING_1_6_2_ANDROID_CONTROLLER_2026_10_08.md).

**Earlier combined phone checkpoint — 8 October:** exact installed source `f2cdbfd42c01f591d725ac41ec217b4ffc3fd593` (tree `44e9710e3766fdc96b06573e510bc350dfacb967`), APK SHA-256 `c73252d00551225b7b4372123b2cb6001677bfec641e24478658d68361520eb4`. This single owner-requested build combines production sheathed start/automatic draw, aspect-based maximum 5 cm outward per hand plus 5 cm retraction, and controller configuration retention. Portrait spacing, combat timing/reach, saved preferences and all 91 packaged assets are preserved. Eight affected host checks pass in 43.13 seconds; the wide actual torch/rig/ceiling matrix passes 3,287 poses; Android passes 225 tests/35 classes and lint, and four-ABI/Windows builds pass. Exact install/pullback matches with zero changed preference entries.

The owner on SM-S948B / Android 16 / Backbone One PlayStation Edition USB-C **BB-51** confirms the sheathed start, prefers the wider/closer landscape pose and likes the draw sound. Right-hand visibility/natural rest is uncertain. **Reattaching the controller still triggers slow Vulkan RT startup; detached touch Resume does not. The left arm looks very floppy while walking with a stable hand. Both are open defects.** There is no exposed manual sheath action. Full body/secondary views, complete controller recovery and sustained phone quality/cost remain separate gates. Windows-only validation follow-up `cf0ffb865d8cc496f63c99f86abe3de2f33b66f8` corrects a ready-sword CPU fixture and frozen stowed-sword visibility contract; it does not replace or claim new acceptance for the installed phone APK. Its aggregate CI is 12/12; its Pipeline run preserves a failed legacy two-enemy camera. Follow-up `a22aaae1` shares the actual waterfall-entry camera across import/metadata and completes 26 frozen Pipeline/Compute frame/PNG/mesh joins with zero validation errors. Those desktop captures do not close moving or phone gates. A host-validated 10° preferred elbow bend reduces the measured wide Torch-arm discontinuity; both 3,287-pose portrait/wide rig matrices pass. Walking appearance still needs owner review. Debug lifecycle diagnostics pass Java/lint but await installation and physical reconnect cause evidence; hot-plug startup remains unresolved. Details: [combined controller/equipment record](ENGINEERING_1_6_2_ANDROID_CONTROLLER_2026_10_08.md).

**Earlier installed normal waterfall candidate — 8 October:** source `6282ab66289775c6c8360a7feb3afd6d554f1787`
now puts the accepted two lateral, east-facing guards in normal play and retains
their distinct walking phase. Bay/Keeper imports and retries preserve that
placement. Sword-starts-stowed stays independently disabled. Seven affected
native checks, Windows/four-ABI builds and 26 frozen Pipeline/Compute checkpoint
frame/PNG/mesh joins pass with zero validation errors. This is not moving or
phone acceptance.

Installed APK SHA-256 `11a49d954a714867cb1e8dbdf8c3690b05d3d7523f4c8efc8bc23b654f974853`; Windows executable SHA-256 `b0d07f3c2906b461751ea9301bc3e9695f20a71e684df0c9f87c01e377a4f711`.
The manifest, 91 assets and Java dex match 6fa; its 222-test/35-class Java/lint
result and completed 12/12 CI remain scoped to that source. New 628 source CI
completes 12/12 (push 37729370167 / PR 37729376579). On the owner's return,
the exact APK was installed on SM-S948B / Android 16: pulled base hash matches,
zero saved-preference changes, free rotation preserved. Ordinary cold launch
reaches the real portrait entry menu. The owned startup excerpt contains one
surface-presented marker, with no VUID/fatal markers; this is not an exact RT
frame/hash join, route or performance pass. The owner accepts corrected D-pad/
stick menus, normal guard placement and parry-to-torch clearance in landscape
with the Backbone One PlayStation Edition USB-C **BB-51**. Owner-selected Dust Low
looks good; source default stays Off. Touch in portrait/opposite-landscape also
works well, including parry spacing. Those BB-51 orientations cannot be exercised
with its fixed mounting. Held-stick/LT disconnect pauses and reconnect restores
menu control, but Resume rebuilds RT slowly; recovery remains open. The physical matrix,
current stow/body/secondary views and combat contact calibration, phone quality/
cost and independent audit remain open. Dust stays Off by default;
shafts deferred. No release. Documentation CI is separate from runtime CI.
[Exact install, artifacts, failure and validation ledger](ENGINEERING_1_6_2_ANDROID_CONTROLLER_2026_10_08.md#owner-landscape-check--6282ab66--backbone-bb-51).

**Earlier 8 October integrated controller/pose checkpoint:** runtime
`454d58ee0f71bdb837a75ce486d6c139e03e0b02` adds native-dialog focus suspension
to the Android controller path and makes contact diagnostics consume the actual
shared skeleton render poses. Standard input, native navigation, HUD separation,
recovery and aspect-based equipment spacing remain in place; portrait is unchanged.
213 Android tests, lint, four-ABI Debug assembly and Windows build pass. Shared
pose contracts and corrected renderer smoke pass; eight fresh RT stills and
uploaded meshes match the preceding a6 candidate exactly. The sealed APK remains
uninstalled while the owner is away. Runtime CI passes all 12 aggregate jobs; test-only registration `03fe9ae2`
and documentation have separate CI.
Physical Android/controller, moving equipment/secondary views/full route,
contact calibration, sustained phone quality/cost, changed audio/haptics and
independent final audit remain open. Dust stays optional/default Off; shafts
deferred. No release.
[Exact sources, package hashes, retained failures and acceptance matrix](ENGINEERING_1_6_2_ANDROID_CONTROLLER_2026_10_08.md#current-integrated-follow-up--454d58ee).

**Earlier 8 October equipment inspection checkpoint:** runtime
`c2de5d7d2f8c25e4ed12152b9cd86fe53e2d6cd1` retains the sheathed right hand through
shared IK and keeps the parry hilt/right sleeve clear of the normal torch.
Ten affected native checks,183 Java tests, Windows/four-ABI Debug builds and
eight frozen desktop RT captures pass. The new sealed APK is not installed:
the owner is away with the phone. Source CI passes all 12 aggregate jobs.
Owner moving appearance, full body/secondary views, production activation/route,
matched phone 50/40/33 quality and sustained thermals/power/memory, changed audio/
haptics and independent final audit remain separate open gates. Dust stays
optional/default Off; its accepted smaller phone still retains 998137 c 9 identity;
shafts remain deferred. No release follows.
[Exact source, package hashes, failures and images](ENGINEERING_1_6_2_ANDROID_MOTION_2026_10_08.md#parry-hilt-and-gauntlet-clearance-candidate).

Updated 8 October 2026. Authority: the owner's `Horde-1.6.2-after-reset-goal.txt`, explicitly adopted by the latest resume request. Supporting PR16 reference: `48fd8d6e0ee73c480502906104593c4e6d3b8aae`. Remote planning advanced to `c03db1c94dd058e8416d1927c908b6b7298d16e0`; those documentation changes are not merged into runtime wholesale. Work continues in the existing PR18 checkout/branch. No duplicate workspace, reset, release, merge, signing or paid generation is authorized.

## Accepted queue addition — Android controller and wider views

Owner instruction8October expands the remaining1.6.2 scope after the sealed
hand/parry checkpoint: standard Android controller input through the existing
coherent mailbox, complete native menu/dialog/slider navigation, active-input
HUD separation, safe disconnect/focus/lifecycle recovery, both landscape
directions and portrait. Connection/drift must not take over touch; saved HUD
and Graphics acknowledgement/persistence rules remain authoritative.

Slight extra equipment separation in wider viewports must use aspect ratio and
shared hand/Grip pose ownership, preserve portrait, combat reach and hit timing.
Android physical acceptance requires an exact phone/OS/APK/controller matrix;
Windows, synthetic events and builds cannot close it. The owner returned and
the intended phone was verified on 8 October; 6282 is installed with preferences
preserved and the normal entry menu is handed to the owner. Earlier c8 feedback
retains its original package identity. The full physical matrix is pending the
owner-controlled USB/Backbone cable swap and hands-on observations. If the owner
takes the phone away again, hold all device work and
owner test requests until explicit return and verification. Continue safe host
work and push coherent checkpoints on PR18. No release is authorized.

The queue implementation and corrected frozen aspect delivery are now checked
on the host; the exact physical matrix remains outstanding in the
[controller/aspect ledger](ENGINEERING_1_6_2_ANDROID_CONTROLLER_2026_10_08.md).

## Preserved completed evidence

Earlier immutable Windows guard preview is `3da5f2dd`: two lateral guards face
arrival with a persistent measured walk-phase offset. Three staged RT stills
pass exact completed/presented-frame and hash joins; affected host checks,
Windows and four-ABI Android Debug builds pass. The APK is not installed because
the owner is away with the phone. Production encounter/stow activation and
moving attachment/body/shadow/reflection remain open. Source CI passes all 12
aggregate jobs; the later moving Windows attempt fails foreground arming with
zero images and its owned process exits. [Exact preview, reproduced
room-selector defect, artifacts and retained failures](ENGINEERING_1_6_2_ANDROID_MOTION_2026_10_08.md#waterfall-room-guards-staged-windows-preview).

Earlier immutable floor-inspection package
`ccd70d3815e0ed08946f9f5ffc52a156bb41ed63` uses one metre farther ordinary
step-back after a retained projection/framing check. Both phone RT backends pass
213/222 exact state/completed-frame rows, 13 images each, fixed dropped world
ownership and packed-light removal; preferences remain unchanged and automated
apps stop. The endpoint regression fails before the schedule correction, then
affected host checks pass 2/2 and Windows/four-ABI Android Debug builds pass.
The owner's subsequent ordinary playtest of this exact APK accepts native touch
comfort, observed torch appearance and the sword's low-ceiling response. That
session is handed to the owner rather than automatically stopped. [Exact
artifacts, caller failures and acceptance limits](ENGINEERING_1_6_2_TORCH_DRENCH_2026_10_08.md#farther-floor-inspection-view).

Test-only `367ac7a3` samples the bounded pulse-to-+3-tick blade path against the
two existing live/Idle controls. The 52 floating-point triangle-distance queries
pass: frontal interpolated contact first appears at +2.5 ticks, while +15 degrees
and both Idle controls stay separated. This does not yet justify a production
sweep/tolerance or global damage delay. [Method and limits](ENGINEERING_1_6_2_ANDROID_MOTION_2026_10_08.md#bounded-interpolated-stroke-samples).

New finite approaching-target probes pass 40 walking and 20 early-Attack mesh
queries, but confirm unresolved visual-contact gaps in the current root gate:
outer walking hits remain at least .3765 m separated, and close/angled early
Attack poses differ substantially. No production range/cone or timing changes
are justified by these endpoint samples alone. [Exact measurements, rejected
fixture and next bounded step](ENGINEERING_1_6_2_ANDROID_MOTION_2026_10_08.md#approaching-target-rangecone-calibration-gap).

Workflow-only `e4cc12f8` passes all 12 aggregate jobs in push `37678033058` /
PR `37678041933`. The preceding `53459b27` has eleven successes and a cancelled
PR Vulkan dependency-installation job; its build/tests never start. The bounded
network-wait/error correction and earlier cancellations retain separate identities.
Later Debug source and documentation have their own CI.

Earlier immutable torch-inspection follow-up
`853d31f293f8593e2f74ed325ce4ef1542019139` waits for the ordinary step-back
endpoint and stationary inspection before completion. Both phone RT backends
pass 200/210 exact state/completed-frame rows and 12/13 images, including fixed
settled world ownership, held-light loss and packed-light removal. Preferences
remain unchanged and apps stop. Affected builds and host tests pass; the dropped
torch still is not clearly resolved in selected final images. The isolated
min33 benchmark package is prepared, without installation or a new performance
comparison; [current quality authority and measurement limits](ENGINEERING_1_6_2_MOBILE_QUALITY_MEASUREMENTS.md)
preserve fresh 50% and the separate historical 75% baseline.
Source push `37672017419` passes all six jobs; PR `37672026461` has five
successes and one cancelled MSVC job after all 97 CTests passed. The overall
ten-minute job deadline interrupted a later asset check, which remains
uncertified for that run. A bounded 15-minute host job budget preserves every
required check and all runtime acknowledgements/timeouts.

Earlier immutable torch-inspection runtime
`408126d4bc2c36d7c5511079d5a9cf6079a664c4` observes the existing automatic
drench/release/fall/settle on both phone RT backends, with 165/169 exact
state/completed-RT rows and 12/13 images. The held-light loss, fixed settled
world attachment and removal of stable torch light ID 1 from packed RT uploads
pass. Preferences remain unchanged and owned apps stop. Affected host tests,
Windows/Android builds, lint and all twelve source CI jobs pass. Selected phone
images do not clearly resolve the dropped torch on the dark floor; that visual
gate remains open. Windows launcher follow-up `7a628fe6` passes its reproduced
admission regression, while its actual native inspection fails foreground
arming and provides no motion images. [Exact artifacts, failures and
limits](ENGINEERING_1_6_2_TORCH_DRENCH_2026_10_08.md). Its CI retains eleven
successful jobs and one cancelled push Vulkan dependency-installation job;
the cancellation is not promoted to a pass.

Earlier immutable input-inspection runtime
`7368e2c72099c47548076981f313f832de4e83f0` adds the explicit Debug consumed-input
observer and fixes Dodge remaining hidden after native warm-up. The regression
fails before the fix; all 177 Android tests in 30 classes, Debug/unsigned Release
builds and lint pass afterward. Both phone RT backends pass a 32-case combined
diagonal move/look/action/release/Cancel/Menu matrix, with unchanged preferences
and stopped apps. Eight additional held-touch Home/fresh-Parry cases pass with
known test-stream timing on both backends. Source CI passes all 12 aggregate jobs
in push `37655935915` /
PR `37655958874`. [Exact artifacts, method, retained failures and
owner gates](ENGINEERING_1_6_2_TOUCH_2026_10_08.md). Successful queue-present
observations do not measure scanout, touch latency or sustained FPS.

Earlier immutable motion-inspection runtime
`ab69537a46248d551420df16f4c3a1f781de6bc5` passes scoped waterfall draw,
attachment, attack and parry on both phone RT backends, with actual observed
draw-progress captures and preferences unchanged. Measured imported-blade
regressions move player contact into the down/up strokes; moving-target range
and cone calibration remain open. Both backends also pass the affected full
Keeper death/retry/three-hit/reward/ending sequence on this exact APK.
Workflow-only `ebeb9117` corrects the missing
host LFS fixture and passes all 12 aggregate CI checks. [Exact artifacts,
failures and limits](ENGINEERING_1_6_2_ANDROID_MOTION_2026_10_08.md#measured-player-contact-follow-up).

Earlier moving-inspection seal `4e9e5bb7696f4d5d3864835eb35e29f4bce42d8e`
passes actual torch/low-passage and Keeper death/retry/three-hit/reward/ending
scenarios on both phone RT backends, with exact completed-frame action captures,
unchanged preferences, stopped apps and all 12 source CI checks passing. [Exact
artifact, corrected harness regressions and remaining limits](ENGINEERING_1_6_2_ANDROID_MOTION_2026_10_08.md).
Production equipment/waterfall activation, contact calibration, physical owner
feel and sustained 50/40/33 quality comparison remain open. Owner questions stay
deferred while the owner sleeps; safe independent work continues.

Earlier UI seal `fe83c473` passes affected physical Settings check-mark
inspection with preferences unchanged and the app stopped; all 12 aggregate CI
checks pass in `37623162765` / `37623170903`. [Exact receipt and package hash](ENGINEERING_1_6_2_NATIVE_MENU_AUDIO_2026_10_07.md#settings-toggle-visibility-follow-up).
The prior exact `ecc16b82` collision replay/frozen poses/pause checks remain under
that package and do not close moving contact, owner acceptance or sustained FPS.

[Pause checkpoint](ENGINEERING_1_6_2_PAUSE_2026_10_05.md), [review candidate](ENGINEERING_1_6_2_REVIEW_CANDIDATE.md), [final Graphics ledger](ENGINEERING_1_6_2_FINAL_GRAPHICS.md), and [second pass](ENGINEERING_1_6_2_SECOND_PASS.md) retain their exact results. Runtime `1334cc9c58ec97940ac10d861f0397143ca7a9f4` passed both allocated-phone backends' route/capture/Home-resume checks with saved preferences unchanged. Staged mobile defaults, Mist rollback and cold-restart persistence passed. Runtime CI passed all six jobs; documentation `5fe070e42541fe8a4b4bc856360b73e1d94c7489` also has six successful jobs in each refreshed push/PR run. These results do not prove sustained 30 FPS or the fresh no-preview Back defect.

## Remaining work and current disposition

Latest owner disposition supersedes the historical calibration/performance gates in this table: precise mesh-contact correction and the long sustained phone programme are deferred; audio/haptics, waterfall and stand collision are approved. Final integrated screen/lifecycle/secondary-view checks, remaining specific controller cases, immutable packages/current aggregate CI and independent audit preparation remain.

| Slice | Current status | Remaining evidence or decision |
| --- | --- | --- |
| Android controller and wider views | Shared Android mappings/publication, native navigation, active-input HUD and recovery implemented; owner BB-51 landscape gameplay/menus, corrected directional focus and seamless reconnect accepted. Touch portrait/opposite landscape and parry spacing accepted; historical failures and exact packages retained in the [controller ledger](ENGINEERING_1_6_2_ANDROID_CONTROLLER_2026_10_08.md) | Remaining explicit physical cold-connected launch, complete dialogs/sliders/Use/Keep/Restore and death/retry/ending coverage; exact controller/phone scope only, no blanket hardware certification |
| Fresh Graphics restore, Android cancellation, Windows focus, Graphics viewport | Implemented in `e1f1b9a0` and `a73a8084`; [regression evidence](ENGINEERING_1_6_2_INPUT_REGRESSIONS_2026_10_07.md). Current `7368e2c7` passes physical-device synthetic whole-gesture Cancel and Menu/Resume for both finger orders on both phone RT backends | Held-touch Home and subsequent fresh move/look/Parry now pass both phone backends; physical Windows controller/focus and owner comfort remain |
| Combat contact/edge timeline and parry presentation | Shared timeline, timestamped input/tick/pose/presentation authority and parry persistence pass host15/30/60/120FPS and hitch/late-input tests; owner combat feel/audio/haptics accepted. [Moving contact evidence](ENGINEERING_1_6_2_MOVING_CONTACT_2026_10_09.md) retains positive mesh gaps | Owner explicitly retains current forgiving combat for1.6.2; precision contact correction is deferred, not passed. Keep current timings/range/cone/parry and disclose unmeasured physical end-to-end latency |
| Native actions and three original hearts | Press-down Swing/Parry/Dodge and original native hearts committed in `b76ce0e6`; `7368e2c7` fixes warm-up Dodge visibility, passes 177/177 Android tests in 30 classes and the combined 32-case diagonal synthetic touch matrix on both physical phone RT backends. Owner ordinary playtest of `ccd70d38` accepts touch comfort; earlier `ab69537a` active-draw Home evidence remains separate | Measured touch latency and successful parry/riposte; owner audio/haptic acceptance approved on 9 October; synthetic held-touch Home/fresh Parry passes on both phone backends |
| Sword overhead clearance | Shared blade-envelope response `0e98a8bb`; corrected Rag sockets, imported arm reach and continuous roof response `c078ad39` pass affected host checks. Owner accepts the low-ceiling response on `ccd70d38` | Remaining actual moving grips/world/shadow/reflection inspection and integrated package/device costs |
| Player-only rag torch | Separate production player resource committed in `40f92c6a`; provenance, closed packaging and atlas preservation recorded. Earlier moving/low-passage captures and later `ccd70d38` drench/drop ownership and packed-light removal pass on both phone backends. Owner accepts observed torch appearance on `ccd70d38` | Remaining floor/moving hand/body and world shadow/reflection inspection, Windows foreground arming and resource/pacing costs; the owner statement does not specify every attachment/reflection condition |
| Shared equipment and waterfall encounter | `cdcb300f` integrates phased Grip and authored Hips scabbard; corrected offline mesh/pose and socket checks pass, prior failed mounts preserved. `ab69537a` passes scoped moving warning/draw/attachment/attack/parry on both phone backends. `3da5f2dd` adds owner-directed lateral facing guards, persistent walk offset and corrected shared room selection, with three exact staged Windows RT stills. `6282ab66` enables normal guard placement and repairs bay/Keeper import/retry ownership; real movement route/Keeper/reward and 26 frozen backend captures pass. `c626333c` enables production stowed start and `f2cdbfd4` integrates accepted 5 cm wide spread/retraction; 8/8 affected host checks and 3,287 actual wide torch/rig poses pass | Owner confirms f2 sheathed start, prefers wider/closer landscape and likes draw audio; right-hand rest uncertain; 6c8 owner sees no noticeable walking-arm improvement and asks to defer this minor issue; ac468f51 owner confirms seamless BB-51 reconnect with same-generation idle/park/resume logs. Normal placement, corrected menus and landscape parry clearance are owner-accepted on 6282 + BB-51. Stow/moving body/shadow/reflection, contact calibration and full controller recovery remain open. Same guard identities/count/stats retained |
| Draw/sheath audio | Licensed FilmCow derivatives, semantic physical-attachment events and provenance retained; owner approves draw sound and all audio/haptics | Preserve attachment-event timing and existing focus/background checks in final integration; no repeated listening gate solely for documentation/material changes |
| Compact selected menu scene | Owner-approved brighter central lantern with Play centred, More left and Settings right; recorded Windows/phone native scene and recovery checks retained. Accepted Room7%/Metal Creak21%, no flame, correct navigation/Play fade; audio/haptics approved | Final integrated reduced-motion, large-font/cutout and loading/error spot-checks remain; long sustained thermal programme is deferred |
| Remaining UI/loading/Graphics simplification | Native menu/loading/audio `d08d3c48`; colon labels and clear scrollbar `359a5711` pass 22 affected Java tests. [Pre-rotation `868691fc`](ENGINEERING_1_6_2_SURFACE_RECOVERY_2026_10_07.md) passes actual ready phone rotations, exact landscape Use/Restore ACK and unchanged preferences on both RT backends. `4e9e5bb7` ordinary portrait Controls/Credits/Report navigation reaches Back and preserves unchecked consent and every preference | Remaining screen/input/accessibility/loading/error checks and integrated cost; no sustained-FPS claim |
| Glass apply latency and ordinary foreground pause work | Exact `ab69537a` passes acknowledged ordinary Menu/Settings/Back/Resume on both backends; three-minute pause and one-minute Settings process CPU are 2.84–2.97% of one logical CPU, with 49 steady native cadence intervals per backend. [Work, failed setups and sensor/memory limits](ENGINEERING_1_6_2_PAUSE_WORK_2026_10_08.md). Earlier `ecc16b82` portrait Home recovery remains separate. Driver-cache negative retained; exact d08 compiled-pair hits measure Off 0.367 s / return On 0.326 s; exact ACK/preferences preserved | Broader cache correctness and controlled sustained thermal/power benefit, distinct from live preview/background suspension; transient PSS differences unresolved, no sustained-FPS or pause memory-savings claim |
| Phone traced 50/40/33 comparison | Matched short moving comparison `a6439e0b` and ordinary experimental options `40677738` retain exact artifacts and measured limitations. Owner accepts 33% moving appearance and ordinary dungeon play | Long sustained pacing/thermals/power/memory programme is owner-deferred on 9 October. Keep existing measured gaps; no sustained-30-FPS claim. 50% remains default; 33%/40% experimental |
| Immutable review candidate and Eric audit | Accepted Debug runtime `ac97da91` is sealed, installed exactly, and passes12/12 aggregate CI. Test-only contact witness and owner decisions are committed separately | Prepare final unsigned Shipping/package/foundation evidence and exact hashes; complete applicable integrated gates, retain device gaps and prepare Eric independent audit. No self-certification or release approval |

Fresh/reset phone defaults remain 50%, Mobile water/fire, Glass Off, Current shadows, cap30, Mist On, now Dust Low by explicit owner request. Desktop stays 100%, High water/fire, Glass On, Current shadows, cap30, Mist On, now Dust Low. Historical baselines and saved/migrated Off/custom tuples retain their own policy. Preserve all saved/custom settings. No silent 33% default or sustained-FPS inference from reciprocal GPU timing.

## Reconciled scope

The player-only disposable rag torch, shared equipment and relocation of the existing two skeletons to the waterfall room are now explicitly included in 1.6.2. The selected central lantern/menu/HUD direction is also included, with its compact-scene feedback gate. Older conflicting future-version clauses are historical, not current exclusions.

Preserve the complete tomb route, four distinct openings, ending, existing health/damage/death rules and accepted Keeper flank-light timing through its actual death then Off. Do not add Kit/voice/dialogue, rope/climbing, forest/night/town/adventure, start-facing-collapse camera, running, additional enemies, healing, campaign saves or optional dressing/cobweb/ambience work, except the separately owner-authorized bounded lantern-menu ambience recorded above.

Closed selective-opacity, Lower-indirect and mirror-to-stone negative experiments remain closed. Preserve [future performance investigation](PERFORMANCE_INVESTIGATION_FUTURE.md); a full historical bisect, texture/LOD programme, temporal/vendor rewrite or broader renderer change is not automatically admitted. A bounded spatial upscaler is conditional on comparison evidence and an owner decision.

Build correctness, physical-device evidence, owner visual/audio/haptic/combat-feel acceptance, sustained performance and Eric's independent audit are separate gates. This ledger records work, not completion certification.

## Current CI and additional evidence

Remote planning reference advanced to `4f134d5e` on 7 October. Its changed roadmap
and UI notes concern earned story verse and future Bellwether life, and explicitly
add no current engineering or release scope. They were inspected without merging;
the authorized standalone tomb scope and small-spinner loading contract remain.

Subsequent ordinary phone Play on `868691fc` exposes a real Showcase asset-staging
failure, despite valid Entry/Graphics presentation. [The staging ledger](ENGINEERING_1_6_2_ANDROID_EQUIPMENT_STAGING_2026_10_07.md)
records the reproduction, regression and correction; the ordinary pause-work
measurement did not start on that failed build. Corrected immutable `ecc16b82`
passes affected ordinary Play/pause/settings/portrait Home/Resume on both phone
backends, with measured foreground pause render suppression and unchanged saved
preferences. All 12 source CI checks pass in runs `37619496990` / `37619504576`.
Native payloads and Windows executable bytes are unchanged from `868691fc`.
Sustained thermals/power and broader physical gates remain open.
Three Windows moving captures also fail to arm because
the owned window lacks foreground focus. Both gaps are retained separately from
the earlier affected passes. Owner questions are deferred while the owner sleeps.

Earlier sealed runtime `868691fc11e7ecf52b0e6ff99d7bab31e2b63997` has **all 12 aggregate CI checks successful** in push `37614824777` / PR `37614831521`. Its [exact Android/Windows artifact and device ledger](ENGINEERING_1_6_2_SURFACE_RECOVERY_2026_10_07.md) records affected rotation/ACK/Home results separately from broader gates. Prior `359a5711` and test-only inventory/witness correction `e4704d16` also passed all 12 in their recorded runs; d08's failed fixtures remain recorded. Earlier `422c1b1a`, `01377031`, `b3ea5ed0` and `f5835b5d` passed all 12 in their recorded runs. These green runs do not certify owner/performance or the remaining physical gates. Documentation after the seal has its own CI. The owner requested periodic Git backups; checked slices are pushed to existing PR18 without merging or publishing.

Historical `a2ad00f8840715cb3eedb542764556bc6165a410` passed all six jobs in each push/PR run. At `40f92c6a4f7374a704bcbe0383844fd301dc90f1`, push `37570840989` and PR `37570845018` completed with three successful jobs and three failed host jobs each. GCC/Clang each passed 77/78 cases and failed the former player-torch socket assertion. MSVC passed 86/88 and additionally failed the hash-pinned manifest after Windows newline conversion. At `0e98a8bb83a08e7d519a0d6d4870fac1caa56077`, push `37573897886` and PR `37573902167` each recorded one successful Android job and five failed host jobs, including portrait/Rag clearance. The socket test now imports the actual player Rag asset. An explicit LF checkout rule preserves the 830-byte manifest SHA-256 `5316b2072008da441024cda1153f725f5afcd7d366f3a5b89dece98ec74617aa` even with `core.autocrlf=true`; the 83-case closed asset policy passes locally.

The exact new foundation and the failed/corrected phone menu inspection, with package hashes and separate limitations, are recorded in [the phone menu and Glass ledger](ENGINEERING_1_6_2_MENU_PHONE_2026_10_07.md). This remains finishing work, not a final immutable review candidate.

The sealed `0e98a8bb` development APK is SHA-256 `2d515ec7ce35b633c30752f0f9fce699c97f189ec06bb0b2894abcd663fd26d0`; Windows Debug executable is `4699ec9abe20b3b5fcac39096f690d07705c9fe859c7174a9750b49b2ad5968e`. Four-ABI Android assembly and the 128-target native build passed. Its interrupted broad host run recorded 59 passing and two failing tests, not a full pass. Owned validation apps/processes were stopped and recorded in `task-4/foundation-0e98a8bb-20261007`.

Java evidence now includes corrected full 172/172 in 29 classes and later affected Graphics 42/42 in six classes, lint zero errors/62 warnings. Two earlier 3/172-failure runs and the real null-description crash/regression are retained; no full 173-case rerun is claimed. The earlier affected native run recorded 12/13 passes, with torch walking continuity failing. Correction `c078ad3945d30968fb53215e2aaaac453823aace` then passes both original reach witnesses, continuity (3.98 s), all 3,287 actual-rig poses (168.00 s, max Grip error 0.0118211 m, worst headroom 0.0323658 m), and affected animation/socket/preview checks (3/3). See [the exact Rag clearance ledger](ENGINEERING_1_6_2_RAG_CLEARANCE_2026_10_07.md). Failed intermediate attempts remain outside Git under `task-4`; an incorrect CTest filter found no tests and is an invocation gap, not a pass. Entry handoff, renderer preflight and pipeline-policy checks also pass locally (3/3, 57.39 s).

Dirty Windows menu executable SHA-256 `6d4c70aeff96f4c7fc65286c1fb3253171dc52a23e100e9825c3e92d93ad57a7` produced the landscape and portrait inspection sets `entry-menu-dirty-inspection-landscape-20261007-05` and `entry-menu-dirty-inspection-portrait-20261007-02`. Both recorded six poses with 30 actual RT presents each, Graphics no-Use Back returning to Entry, saved Graphics unchanged, Play handoff epoch 7, zero synchronization-validation errors and stopped owned processes. Scene resources were three BLAS/one TLAS/three instances and about 56.5–56.7 MiB tracked device-local memory. Diagnostic fixed-step captures do not establish displayed/sustained FPS, phone cost or thermal behavior. The subsequent brightness change is not included in those hashes.

The requested 10% brighter emitted-light version was then rebuilt as dirty executable SHA-256 `6bbb8fe4049403081c93cc681ac4cd0a28ae07da6cb9b0534c2b9d6b730a75ff`. Both `entry-menu-brighter-landscape-20261007-01` and `entry-menu-brighter-portrait-20261007-01` completed six poses, saved-settings preservation, no-Use Graphics Back and actual Play handoff epoch 7, with zero synchronization-validation errors. Owned processes 53760 and 48632 exited successfully. These inspections do not include the subsequent pause-cadence implementation or constitute immutable integrated candidate evidence.

The immutable `40f92c6a` Windows Debug executable SHA-256 is `fd8ef6aacce03dfaf61fda746f4557a3905520eb8ccb91421de1e73208d02a7d`. Its Vulkan-validation portrait `player-viewmodel-grips` capture at native 540×960 on RTX 5050 Laptop completed and stopped the owned app. The final accepted/completed RT identity is scene epoch 2, record/submission/completion 12, simulation tick 1; its resources report 19 BLAS, one TLAS and 25 TLAS instances. The image is `task-4/rag-40f92-portrait-grips-20261007/136-player-viewmodel-grips.png`, SHA-256 `ab23bb3c2eb704fa2a1223922a8203639bc4269d83a62ab7fe1b8081e724c3d5`. This frozen grip capture establishes a presented RT frame, not moving torch fit, owner acceptance or sustained FPS.

## Owner-authorized indoor dust expansion, 8 October

The optional, default-Off indoor-dust prototype is added to this goal by explicit
owner request. See [the bounded dust record](ENGINEERING_1_6_2_INDOOR_DUST_2026_10_08.md).
Motes start in two small authored zones; shafts require a later visual/cost go/no-go
and are deferred. This does not authorize other optional dressing or adventure
scope. The owner is away with the phone: no ADB/install/device tests or wait loops.
Preserve all prior exact passes and failures. Dust owner/mobile/sustained gates
remain pending independently of safe source, host, shader and desktop work.

The owner returned on 8 October and reported the phone connected. Read-only
identification verifies the intended SM-S948B / Android16 / API36 before resumed
validation; no new mobile pass follows from availability alone. The owner also
approved the waterfall skeleton image as looking great. This accepts the staged
3da5f2dd room layout shown previously, including one left/right guard facing
arrival. It does not close moving-body, shadow/reflection, production activation
or integrated-candidate gates. Prior CCD touch/torch/clearance approvals retain
their original package identity.

### 8 October smaller indoor-dust appearance checkpoint

Source `998137c94448b28bec1da3f6bf74f875ea26004f` reduces authored mote radii
by roughly a third and opacity25%, preserving seeds/drift/bounds/counts and
all other quality/defaults. The owner accepts its exact first-box phone still.
Both Windows backends pass the box/ellipsoid/reframed-wall captures; phone
frozen and timed moving RT checks pass with unchanged preferences and stopped
automated apps. Source CI passes all12 aggregate jobs.
[Exact package, measurements and retained failures](ENGINEERING_1_6_2_INDOOR_DUST_2026_10_08.md#smaller-subtler-immutable-follow-up).
Dust remains optional/default Off; appearance approval is separate from
continuous motion and sustained cost. Shafts remain deferred. No release or
independent-audit completion follows.

The same sealed Windows source subsequently completes the visible foreground
waterfall-equipment inspection and replay.13 PNGs/150 exact rows close the
previous missed-action-image coverage for this sequence. Owner review raises
sheathed empty-hand framing and parry hilt/hand-to-torch clearance; both are
under investigation. Production activation and world-body/secondary-view/route
gates remain open. See the current
[motion record](ENGINEERING_1_6_2_ANDROID_MOTION_2026_10_08.md#8-october-visible-windows-waterfall-sequence-and-owner-findings).
