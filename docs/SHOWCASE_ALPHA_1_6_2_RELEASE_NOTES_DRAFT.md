# Horde Lantern RT 1.6.2 development notes

**Integrated stand/default checkpoint — 9 October:** source `ac97da91` adds shared physical collision for the existing two Keeper torch stands, preserves full-height wall acoustics, and sets fresh/staged-reset Dust Low on Android and desktop without overwriting saved Off/custom or genuine legacy preferences. Mobile resolution remains **50%**; 33%/40% stay experimental. Seven affected host suites, 256 Java tests/39 classes, lint, four-ABI/Windows builds and package checks pass. Exact SM-S948B/Android 16 install/pullback and normal entry pass with settings/menu mix unchanged; the owner confirms ordinary player/Keeper stand collision works as intended. Owner waterfall, audio/haptics and 33% manual play acceptance are recorded; the long sustained phone programme is owner-deferred. Current ac97 source CI is pending, separate from earlier green runs. [Exact source/packages, regressions and limits](ENGINEERING_1_6_2_STAND_DUST_2026_10_09.md).

**Owner-accepted torch-water correction — 9 October:** exact `8edca4bb` bounds fire diffuse by the existing entrained-air fraction at the water material, preserving shared opaque lighting, warm highlights and Fresnel transport. 255 Java tests/39 classes, lint, four-ABI/Windows builds, three affected host checks and all 16 backend shader variants pass without widening frozen budgets. Sixteen matched phone pairs and eight Windows pairs cover near/far/oblique/catchment views on both hardware RT backends; saved settings/mix and captured resource payloads are unchanged. The owner accepts the waterfall appearance. Retained setup failures and owning-frame GPU observations are scoped explicitly; no sustained-FPS or causal performance claim. Refined runtime CI is pending publication/current-run results, separately from the isolated zero-diffuse source's 12/12 pass. [Exact source/packages, images and limits](ENGINEERING_1_6_2_TORCH_WATER_2026_10_09.md).

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

**Earlier owner-tested RT recovery checkpoint — 8 October:** source `ac468f5136d978152a9953f490a631453a037c89`, Debug APK SHA-256 `3a4a7f16c606d442d825ddfb9c2720ed98f57da147940495a29f5cf03a5850b3` (138,462,724 bytes), exact SM-S948B / Android 16 install/pullback with zero preference changes. Brief Activity pauses suspend the valid RT generation; full Stop/destruction still retires it. Four affected host checks, 232 Java tests/36 classes, lint, four-ABI and Windows builds pass. Controlled Home/full-retirement/resume presents on both phone backends with saved settings unchanged. The Windows EXE is byte-identical to the retained 6c8 capture binary. **Owner BB-51 reconnect check passes: seamless attachment/reconnection, with matching same-generation suspend/resume logs.** The full controller matrix remains open. Source CI [push 37747195457](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37747195457) / [PR 37747203704](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37747203704) passes 12/12 jobs after one unchanged Windows report-dialog retry; its original timeout is retained. [Exact recovery evidence and cache backend split](ENGINEERING_1_6_2_ANDROID_PIPELINE_RECOVERY_2026_10_08.md). Minor Torch walking-arm wiggle is retained/deferred at the owner's request; earlier appearance/route feedback remains bound to its package. Sustained phone quality, moving secondary views, remaining physical controller matrix and Eric's independent audit remain open. No release approval/publication.

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

Contact calibration is still incomplete: the finite preceding-tick mesh probe
retains positive blade gaps despite current gate-admitted damage. No new sweep,
range/cone or pulse change is included on that evidence.
The later 24-pose real-rig regression finds less than 0.012 mm disagreement
between simulation and rendered swords in its fully engaged fixture; three
affected CTests pass. [Exact diagnostic source and limitations](ENGINEERING_1_6_2_ANDROID_MOTION_2026_10_08.md#simulation-sword-and-rendered-grip-agreement-8-october-follow-up).

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

The exact smaller-dust Windows checkpoint subsequently passes a foreground
waterfall equipment sequence and requested replay. Corrected draw/action images
join exact state/completed ticks. Owner review raises empty-hand framing while
sheathed and possible parry hilt/hand-to-torch interference; these findings have host-tested source corrections above. Current moving
acceptance, production activation and full body/secondary-view/route checks
remain pending.
[Current motion evidence](ENGINEERING_1_6_2_ANDROID_MOTION_2026_10_08.md#8-october-visible-windows-waterfall-sequence-and-owner-findings).

**8 October smaller-dust follow-up:** exact runtime
`998137c94448b28bec1da3f6bf74f875ea26004f`, Debug APK SHA-256
`41d9802db1f44f175e1a1cc8d33e73c8b3e1e39c9398e74ac5ca6370780fad09`,
Windows executable SHA-256
`3a6af703d83dd27620421f22e7769a0c685673b59f3c1e3bd7fd881ce83d3513`.
The owner accepts the smaller/softer phone still. Both RT backends pass affected
frozen and timed-moving checks, with preferences preserved and owned processes
stopped. All12 source CI jobs pass. Dust stays optional/default Off, shafts
stay deferred, and continuous-motion/sustained-cost gates remain open.
[Exact evidence and bounded timing pair](ENGINEERING_1_6_2_INDOOR_DUST_2026_10_08.md#smaller-subtler-immutable-follow-up).

**8 October optional indoor-dust prototype:** bounded deterministic world-space
motes share real RT primary depth and actual light visibility across Pipeline
and RayQueryCompute. Dust Off/Low/Standard is independent and defaults Off;
existing preferences/defaults are preserved. The first exact phone stills on
`71a2a98be945f9bb51ebb432f5ca20762a194eed` render both backends; owner feedback
requests smaller/subtler motes. Follow-up appearance, motion and sustained cost
remain open. No shafts or secondary dust reflections/transmission are included.
All12 source CI jobs pass for this first runtime; build and frozen-frame checks
do not close mobile performance or final acceptance.
[Exact source/package and retained failures](ENGINEERING_1_6_2_INDOOR_DUST_2026_10_08.md).

The owner has returned with the intended phone and accepts the staged waterfall
guard layout image. Production activation still waits for moving body, shadow,
reflection and integrated-route evidence. Historical away/device gaps below
retain their original source/package identities.

**8 October staged waterfall guard preview:** source
`3da5f2ddb7050422ccc083a645afc3a84b17aa4a`, tree
`f9bef90b62d06c27dab015640afb234a4f311a3b`; Debug APK SHA-256
`4c0829af2d1c890455917fd49740f0f0dfd8ea02de923384f329b26e29622db3`
(138,462,724 bytes), Windows Debug executable SHA-256
`d22757047868747d05685318bda42406c599a8e0bb927fa1fc9167a662ed0688`
(11,352,576 bytes). The existing pair now stands to either side facing arrival,
with distinct persistent walking phases and corrected shared room selection.
Three actual staged Windows RT stills pass owning completed/presented-frame and
hash joins, with stopped owned processes and zero validation error markers.
Affected host checks and both builds pass; Android closed assets, four ABIs,
manifest and alignment pass. The owner is away with the phone: this APK is not
installed/device-validated, and earlier CCD owner approvals remain separate.
Production encounter/stow flags remain off; moving body/shadow/reflection,
integrated route, owner layout feedback and sustained quality gates remain open.
[Exact evidence and failed predecessor](ENGINEERING_1_6_2_ANDROID_MOTION_2026_10_08.md#waterfall-room-guards-staged-windows-preview).
Source CI `37685777037` / `37685784607` completes all 12 aggregate jobs;
preceding `38f9d48a` retains six failed registry jobs and six successes.
The corrected moving Windows capture then fails foreground arming, with zero
images and a normal owned-process exit carrying failure code 1. That inspection
gap and the older intermittent report-form timeout remain preserved.
This is an unreleased inspection checkpoint, with no new Release or signing.

Further opt-in approaching-target mesh probes pass 40 walking / 20 early-Attack
queries and confirm unresolved gaps between current root-gated damage and the
visible blade. These are discrete geometry measurements, not production contact
acceptance; hit rules and parry timing remain unchanged. The first invalid
fixture is retained separately. [Evidence and limits](ENGINEERING_1_6_2_ANDROID_MOTION_2026_10_08.md#approaching-target-rangecone-calibration-gap).

**8 October owner playtest and farther floor inspection:** source
`ccd70d3815e0ed08946f9f5ffc52a156bb41ed63`, tree
`f088aff7c39405f986d9898b431e564abcede2a3`; Debug APK SHA-256
`7c637c0b15b95964d9303dd93d69f3a7be77fd4962b791e0acea2af0ad743197`
(138,462,724 bytes), actual installed base matches. Both phone RT backends pass
213/222 exact state/completed-frame rows and 13 images each, with fixed dropped
ownership, packed-light removal, unchanged preferences and stopped automated
apps. A measured framing follow-up changes only the Debug inspection endpoint;
its red/green host regression, Windows build and four-ABI Debug build pass.
[Exact artifacts and retained failures](ENGINEERING_1_6_2_TORCH_DRENCH_2026_10_08.md#farther-floor-inspection-view).

The owner then accepts ordinary native touch comfort, observed torch appearance
and the sword's low-ceiling response on this exact build. Their normal playtest
session is handed over and remains theirs; it is not automatically stopped.
Test-only `367ac7a3` adds 52 bounded interpolated blade/target distance samples:
frontal sampled contact first occurs at +2.5 ticks, while +15 degrees and Idle
controls stay separated. Production contact calibration remains open.
Workflow-only `e4cc12f8` passes all 12 jobs in `37678033058` / `37678041933`;
preceding `53459b27` retains eleven successes and a cancelled Vulkan dependency
install, without build/test execution. Later source/documentation CI is separate.

**8 October completed step-back inspection:** source
`853d31f293f8593e2f74ed325ce4ef1542019139`, tree
`1337cc77fe71ef88e1ca8dae94a173e442326dea`; Debug APK SHA-256
`4bc47c2457a550f4c4fedfb1f24cdbc4262833df8d5efa88d1c40b73233cee65`
(138,462,724 bytes), installed base matches. Windows Debug executable SHA-256
`ab5588c8476d6a32ee5c0ca83300bc7fb55eedb8fe092a5b33364986c6f5f141`
(11,347,968 bytes), built without a new physical Windows moving pass.
Both phone backends pass the revised ordinary step-back/floor inspection:
200/210 exact state/completed-frame rows, 12/13 images, unchanged preferences
and stopped apps. The dropped torch remains hard to distinguish on the dark
floor; this is an open visual gate. Affected host/native and four-ABI Debug
build checks pass. Earlier unsigned Release, lint, input and route evidence
retain their exact identities below. [Torch follow-up evidence](ENGINEERING_1_6_2_TORCH_DRENCH_2026_10_08.md).
The isolated min33 benchmark is prepared but not installed or measured;
[quality measurements](ENGINEERING_1_6_2_MOBILE_QUALITY_MEASUREMENTS.md) retain
fresh 50% and the separate historical 75% baseline. No final candidate,
owner acceptance, sustained-performance, independent-audit or release claim.
Source push `37672017419` passes six jobs; PR `37672026461` records five
successes and one cancelled MSVC job after 97/97 CTests passed, before later
asset checks completed. The host job budget becomes a bounded 15 minutes;
checks and runtime ACK deadlines are unchanged. Later CI is separate.

**8 October torch failure follow-up:** immutable phone inspection source
`408126d4bc2c36d7c5511079d5a9cf6079a664c4`, tree
`ac940653a42a9b4fb2fad8294563049cc2a37d01`. Debug APK SHA-256
`95e12236a352f1c2635ae0dc3b8a2ad4c02b047bdc40b07bf3296c716cda215c`
(138,462,724 bytes); installed base matches. Both phone RT backends pass the
existing automatic drench/drop world-ownership and packed-light removal checks,
with 165/169 exact state/completed-frame rows, 12/13 images, unchanged preferences
and stopped apps. Host cadence/pause/interruption/ledger checks, Windows/Android
builds and lint pass; source CI is twelve successful jobs in `37666426492` /
`37666434171`. Actual four-ABI unsigned Release excludes the added Debug markers,
SHA-256 `114c031016eb93075e0cbc0917ae308a2db719ce5132853a94292be335fe04f7`
(120,007,832 bytes). Windows launcher follow-up source
`7a628fe6e9143c3d6ea2f2bca44ec12a10b22002`, executable SHA-256
`0a7c368919cadd46a618c8aeff0ed88c82fd3bb893d89d92ba618203e5d74cc4`,
passes its reproduced admission regression, but actual native inspection fails
foreground arming and has no motion captures. The dropped floor silhouette is
not clearly resolved in selected phone images. [Exact evidence and retained
failures](ENGINEERING_1_6_2_TORCH_DRENCH_2026_10_08.md). This remains an inspection
checkpoint; owner visual/audio/haptic/comfort, production equipment/contact,
sustained performance, final review candidate and independent audit remain open.

**8 October input follow-up:** immutable inspection runtime
`7368e2c72099c47548076981f313f832de4e83f0`, tree
`56c1e8b0a1dfdb5e0397287532506ab571301ae0`. Debug APK SHA-256
`c04d0f12b4fa507f5171b16355f21fa646f99b3d1e043b7fb249b0e7fc11691c`
(138,462,724 bytes); installed base hash matches. Both phone RT backends pass
16 diagonal move/look/action/release/Cancel/Menu cases each, with unchanged
preferences and stopped apps. Eight additional held-touch Home/fresh-Parry cases
also pass on both backends with sampled zero-input state before test-stream
closure; this is synthetic device evidence, separate from owner comfort.
Dodge now reappears after native warm-up; the
regression fails before the fix and all 177 Android tests pass afterward.
Debug/unsigned Release builds and lint pass. Source CI passes all 12 jobs in
`37655935915` / `37655958874`; documentation after the seal has separate CI.
Unsigned Release APK SHA-256
`a5bcacb8cb94334ac542c00be034ce3d216e4755db17604761f1b34f9922db9b`
(120,007,832 bytes); all four actual native payloads exclude the explicit Debug
input/combat trace markers. Windows executable SHA-256
`c639e713084437f4c4bdb3d481f7bbf5e69284ff6dff5a30a000065923f0cdb3`.
[Exact input evidence and retained failures](ENGINEERING_1_6_2_TOUCH_2026_10_08.md).
This inspection seal does not close owner visual/audio/haptic/comfort,
moving-contact/production-equipment, sustained performance or independent audit
gates and is not a final review or release candidate.

**8 October contact/draw follow-up:** immutable runtime
`ab69537a46248d551420df16f4c3a1f781de6bc5`, tree
`f49720cd5c2fb6afaf2a9fab75cb9bfccf3fae1c`. Debug APK SHA-256
`99e012534a5b442846d7aeb724dbf17e9ea9860492724c1ee100bdfa20fd3895`
(138,462,724 bytes); installed pullback matches. Both phone RT backends pass
scoped waterfall approach/draw/attachment/attack/parry with observed quarter
progress captures, unchanged preferences and stopped apps. The player contact
samples now occur during measured imported-blade close approach; range/cone,
dynamic targets and owner feedback remain open. Both backends also pass the
affected full Keeper death/retry/three-hit/defeat/reward/ending sequence.
Actual four-ABI unsigned
Release excludes Debug harness markers, SHA-256
`03151b019762a140d419509fa8d1aa58d3a21c82ce790febc6d97f4f9f4ec9ad`.
Workflow-only `ebeb9117` fixes host fixture admission and passes 12/12 aggregate
CI in `37641619811` / `37641674418`; initial missing-fixture failures are retained.
[Exact measurements and limitations](ENGINEERING_1_6_2_ANDROID_MOTION_2026_10_08.md#measured-player-contact-follow-up).
Production equipment activation, owner visual/audio/haptic/comfort, sustained
phone performance and Eric's independent audit remain open. No release approved.

**8 October moving-inspection checkpoint:** source
`4e9e5bb7696f4d5d3864835eb35e29f4bce42d8e`, tree
`93df3ea4a5d34f84511f28450fb6806394a49c90`. Debug APK SHA-256
`68741e3d3407d124b6f3ba95f7e84e991d701070fe396922c90014b3ed6e80b7`
(138,462,724 bytes); installed pullback matches. Both phone RT backends pass
ordinary-axis torch/low-passage and full scripted Keeper death/retry/three hits/
defeat/chest/reward/ending/re-entry, with completed-frame combat captures,
unchanged preferences and stopped apps. All 12 source CI checks pass in
`37631996493` / `37632008946`; four-ABI unsigned Release and lint pass with the
Debug motion harness excluded from actual Release payloads. [Exact method,
hashes and retained failures](ENGINEERING_1_6_2_ANDROID_MOTION_2026_10_08.md).
Blade-contact/framing, production equipment activation, owner feel, sustained
performance and the independent audit remain open.

**7 October post-reset status:** PR18 contains the fresh no-preview Graphics restore correction, whole Android gesture/pause cancellation, unfocused Windows controller suppression/reseeding and Graphics scroll/focus retention. Combat/touch, rag torch, shared equipment and selected menu work remain subject to the affected physical and owner gates in [the active plan](ENGINEERING_1_6_2_TOMB_FINISH.md). Original runtime `1334cc9c58ec97940ac10d861f0397143ca7a9f4` keeps its exact completed evidence; those historical results are not assigned to new edits.

The prior UI seal is `fe83c4733e46596c53189e6c77faf34baa920853`, tree `f5bb2dc2bd1146cf365b147aa29b7514ca9ca625`. Four-ABI Debug APK SHA-256 is `8e0b3b7f5a40ebd18a62d05790bfd22385162c05e46fa14520baf26354a290fb` (138,462,724 bytes); installed pullback matches. Its affected phone inspection confirms readable native Settings check marks with every preference entry unchanged and the owned app stopped. [Exact visibility receipt](ENGINEERING_1_6_2_NATIVE_MENU_AUDIO_2026_10_07.md#settings-toggle-visibility-follow-up). All 12 aggregate runtime CI checks pass in runs `37623162765` / `37623170903`; documentation after the seal has separate CI.

The prior exact `ecc16b82` APK (`ccafc11b31ca51e5f6bdc4c6c4e18de14144eca45907cc0ebc8e0908c48e555f`) corrects packaged equipment staging and passes ordinary Play/pause/settings/portrait Home/Resume, 13-waypoint collision replay and five frozen equipment captures on both phone RT backends. Native pause logs measure redundant-render suppression. Those results remain attached to that APK and do not prove moving contact or sustained FPS. [Exact failure/correction and evidence limits](ENGINEERING_1_6_2_ANDROID_EQUIPMENT_STAGING_2026_10_07.md). Native payloads and Windows executable remain byte-identical to `868691fc`, Windows SHA-256 `4d86cd6d0fe628164b0bf97bb194eb1478c94ecda7973d80309b354b075beed9`; its earlier exact source/Entry/Play evidence remains separate. Sustained phone FPS/power/thermals and moving equipment/route activation remain open.

The approved brighter lantern and Play-center / More-left / Settings-right placement are retained. Owner-requested colon labels and a clear scrollbar pass 22 affected Java checks and actual large-font phone inspection. Current-transform RT pre-rotation fixes the repeated landscape recreation loop: both real phone backends now pass bounded rotated presentation, exact landscape Use/Restore ACK and Home return to confirmed Settings with newly presented surfaces. Every preference entry is unchanged and validation apps are stopped. Windows Pipeline landscape and Compute portrait each pass six Entry poses, no-Use Back and actual Play handoff with zero synchronization-validation errors. [Exact recovery evidence and limits](ENGINEERING_1_6_2_SURFACE_RECOVERY_2026_10_07.md) preserve the earlier failures and distinguish physical owner rotation and landscape Home/resume, which remain unverified.

The compiled-pair cache has measured phone hits at the d08 checkpoint: Glass Off 0.367 s / return On 0.326 s with strict ACK and unchanged preferences. The earlier driver-cache negative remains preserved in [the cache ledger](ENGINEERING_1_6_2_COMPILED_PIPELINE_REUSE_2026_10_07.md). Selected FilmCow attachment cues and authored scabbard are integrated; moving inspection, listening, route checks and production equipment/waterfall activation remain pending. Ordinary pause work/thermal reduction and integrated 50/40/33 play comparisons still require evidence and owner quality decisions. This is not a final review candidate, sustained 30 FPS result, independent Eric audit or release approval.

Package version: `1.6.2`

Android version code: `10`

This is an unreleased development candidate. These notes do not authorize
production signing, merging, tagging or publication. Published 1.6.1 source,
packages and evidence remain frozen.

Implemented source includes presentation semaphore ownership and optional
present-completion fences, explicit AS scratch/SBT alignment, complete cgltf
notices, efficient Windows output resizing and corrected report word boundaries.
Shared Graphics settings provide acknowledged Apply/Revert/Keep, persistence,
recovery and a compact production RT preview with bounded telemetry.

Material foundations include normal strength, glTF scale, tangent handedness,
authorable texture scale and ambient-only AO. Higher-tier bounded area shadows
await matched visual/performance admission. Temporal geometry and dormant DRS
policies are feasibility foundations; GPU history, reconstruction and vendor
upscalers are deferred with evidence recorded in the engineering ledgers.

Polish includes coherent held-torch clearance, bounded moving flame shapes,
deeper physically enclosed waterfall shaft with opaque foliage, reused authored
environment art, a deterministic Keeper reveal, native UI updates and admitted
audio derivatives. The collapsed entry remains behind layout and appearance
approval gates; no unfinished study is described as shipped.

See [the current checkpoint](ENGINEERING_1_6_2_HANDOFF.md) and its slice ledgers
for actual checks, artifact identities and outstanding gates. Physical RTX and
SM-S948B validation, current CI, final package checks and owner visual/audio
acceptance are pending. S24 final coverage is deferred and S25 unverified. The
formal presentation-retirement gap on unextended drivers remains explicit.
