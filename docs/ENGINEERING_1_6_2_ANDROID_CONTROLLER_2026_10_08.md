# Android controller and wider-view checkpoint, 8 October 2026

**Current installed RT recovery checkpoint — 8 October:** source `ac468f5136d978152a9953f490a631453a037c89`, Debug APK SHA-256 `3a4a7f16c606d442d825ddfb9c2720ed98f57da147940495a29f5cf03a5850b3` (138,462,724 bytes), exact SM-S948B / Android 16 install/pullback with zero preference changes. Brief Activity pauses suspend the valid RT generation; full Stop/destruction still retires it. Four affected host checks, 232 Java tests/36 classes, lint, four-ABI and Windows builds pass. Controlled Home/full-retirement/resume presents on both phone backends with saved settings unchanged. The Windows EXE is byte-identical to the retained 6c8 capture binary. **Owner BB-51 reconnect check passes: seamless attachment/reconnection, with matching same-generation suspend/resume logs.** The full controller matrix remains open. Source CI [push 37747195457](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37747195457) / [PR 37747203704](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37747203704) currently has 10/12 jobs passed; both MSVC jobs remain in progress. [Exact recovery evidence and cache backend split](ENGINEERING_1_6_2_ANDROID_PIPELINE_RECOVERY_2026_10_08.md). Minor Torch walking-arm wiggle is retained/deferred at the owner's request; earlier appearance/route feedback remains bound to its package. Sustained phone quality, moving secondary views, remaining physical controller matrix and Eric's independent audit remain open. No release approval/publication.

The owner confirms seamless BB-51 reconnection in this package. Owned logs show two brief pauses (102/61 ms), successful native idle/park and same-generation resume with no teardown. The first interval has a subsequent presented marker; the second has no repeated marker, so continued visual play is owner-reported. Later genuine background destruction still cancels the generation. Saved preferences remain unchanged; the owned app is stopped after retaining the logs. [Sanitized receipt](evidence/2026-10-08-android-rt-recovery/receipts.json).

The owner's queue addition is implemented on existing PR18. Physical Android
controller support has **partial owner acceptance on the identified BB-51**, with
the full physical matrix still open. The earlier owner-tested 6c8 repair candidate retains
sheathed start and the accepted wider/closer arms. Earlier f2 owner findings and
the remaining physical gates are recorded below. The earlier 6282 package retains its landscape menu/guard/parry feedback. Earlier preliminary c8
feedback keeps its package identity. Earlier checkpoints below retain their own
source, package and evidence identities.

## Installed repair candidate — 6c8ecc39

Source `6c8ecc39e63f17d4716da0d8f6e994cb872831a5`, tree `fb032369288002ac9f5baed2051a9ef1bbc9faa1`, pushed to the existing PR18 branch. Debug APK SHA-256 `f178daf46431f28d2aa8f997dc178274ec9653aca182e992f27f127765cbd945` / 138,462,724 bytes. Windows EXE SHA-256 `a87067220a1ab80ff1cb421f321ade19c4d4ac5f79c7b856a60f054ebb2ae5c2`. Four-ABI Android and Windows builds pass; SDK 37 16 KiB alignment, fullUser 13 and manifest configuration mask 0x4f0 pass. All 91 packaged asset entries and the manifest match f2; the complete two-dex roster changes for the diagnostics and is recorded. The install at **2026-10-08T07:10:16Z** on verified SM-S948B / Android 16 pulls back the exact APK, preserves every saved preference and free rotation, and leaves other app variants untouched. No controller is certified by connection to the computer.

Normal cold entry Activity launch reports 463 ms / 466 ms wait, **not RT initialization or displayed time**. The owned bounded startup excerpt records generation1 successful native initialization at **15,043.726 ms CPU wall time**, followed by `HORDE_SURFACE_PRESENTED`, with no VUID/fatal markers. This is limited startup evidence, not an exact phone frame/image join, motion route or sustained performance pass. Current owner custom 50%/Mobile/Glass On/Current/cap30/Mist On/Dust Low remains intact; fresh defaults stay unchanged.

The new sealed desktop captures complete **13/13 Pipeline (37.110 s) and 13/13 Compute (36.609 s)**, owned processes 68988 / 29772 exit 0, zero VUID/synchronization markers. Parent verification joins all **26 successful frame identities, PNG dimensions/hashes and 52 actual uploaded viewmodel/world-body mesh hashes**. Opening and waterfall guard PNGs were inspected: Torch Grip and empty right arm are primary-visible and the actual route-aware guard view is correct. This does not accept moving arms, secondary-view geometry, physical phone quality or performance.

The owner has completed the combined check on this exact build: the left arm still wiggles while walking and does not seem noticeably different; they explicitly classify it as minor and ask to note it rather than spend hours tuning. Retain the bounded CPU discontinuity reduction, record the residual appearance issue, and defer further tuning. Right-hand rest has tentative feedback, not a firm appearance pass. **Parry and low ceilings are owner-accepted. Reconnect still rebuilds RT, possibly even when attaching at the main menu.** **Hot-plug is not fixed by this candidate.** Passive diagnostics add `stop` and focus gain/loss to resume/pause/surfaceDestroyed/destroy reasons and viewport dimensions, preserving native suspension and exact Graphics acknowledgement/timers. Current source CI push [37741496149](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37741496149) and PR [37741501338](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37741501338) complete successfully, **12/12 aggregate jobs** (push 6/6, PR 6/6), separately from a22's completed 12/12. CI does not close the physical gates. [Selected exact desktop frames](evidence/2026-10-08-torch-arm-repair/README.md).

The returned owned-process excerpt now identifies two brief pause/resume intervals (106 ms and 79 ms) on the **same Activity**, with `available=true` and no surface destruction between them. `onPause` directly cancels generations1/3. New generations3/5 initialize in **17,393.267 / 17,964.362 ms**, with `compiled_pair_reused=0`, then present. Within-generation resize/rotation reuses pipelines in **45.183–298.386 ms**. A later genuine pause is followed by `surfaceDestroyed` and `stop`. This establishes the existing pause teardown as the restart trigger for those captured intervals; it does not identify which Android/system component issued the brief pause. Do not retain active GPU/background rendering as a workaround. A bounded, compatible CPU-only Vulkan cache seed is being investigated; its benefit remains unproven. Owner model/firmware/complete physical matrix gaps remain intact.

## Combined owner pass — f2cdbfd4

Exact source `f2cdbfd42c01f591d725ac41ec217b4ffc3fd593`, tree `44e9710e3766fdc96b06573e510bc350dfacb967`. Sealed APK SHA-256 `c73252d00551225b7b4372123b2cb6001677bfec641e24478658d68361520eb4` / 138,462,724 bytes; original Windows binary SHA-256 `fdfe911aeefebaefd362f7a381257623610b4563aa140f0e40f7aa4b01b8122d`. The 91 asset entries match the prior 6282 APK byte for byte. The primary `classes.dex` matches 6282 but `classes2.dex` changes (`c8c1a8aa3af166404edd0b9481b58dc15fd0776a48e657d95711035d63717249`); the complete multidex roster and manifest intentionally change for input-configuration handling. The original private seal's primary-dex-only comparison is supplemented by the complete roster verification; packaged `configChanges=0x4f0`, `fullUser=13` and 16 KiB zip alignment pass. Four native ABIs build. The phone install at 2026-10-08T05:55:53Z preserves every preference entry and free rotation; the pulled installed base matches the seal. Cold entry Activity launch reports 422 ms / 424 ms wait, which is **not RT scene initialization or displayed frame time**. The owned portrait entry still is 1440 x 3120, SHA-256 `b8d18bc10be7712f2da7b276b429b083a910cc55b4427a1b1b381e42d5d1bb0a`, with real Play/More/Settings controls. The saved owner-selected 50%/Mobile water+fire/Glass On/Current/cap30/Mist On/Dust Low tuple remains custom; fresh defaults are unchanged.

Implementation commits: `c626333c` enables only the existing production stowed-start mechanism; `da7ee841` handles keyboard/keyboard-hidden/navigation configuration on the existing Activity and preserves input-only UI/focus/Graphics acknowledgement state; `f2cdbfd4` changes the shared aspect comfort cap from 3 cm spread to **5 cm per hand outward plus 5 cm back**, smooth above square through 16:9, capped thereafter. Hands, props, arm IK, torch flame/light and reward carry share the same targets before existing roof clearance. Released torch world trajectories, combat ranges/cones/pulses and parry timing remain independent.

Owner evidence class: **qualitative combined hands-on feedback on that exact installed Debug APK**, SM-S948B / Android 16 / owner-identified BB-51. The owner confirms the sword starts sheathed, likes the draw sound, and says the wider/closer landscape view is better and works nicely. They are unsure whether the right hand is visible/natural while sheathed; do not mark that gate accepted. No manual sheath control is exposed; automatic draw and an early Swing's existing queued draw/attack behavior remain the implemented route.

The physical recovery gate **fails to close**: after reconnecting BB-51, Resume still starts Vulkan RT and pauses for initialization. Touch Resume while detached does not show that startup pause. The owner also reports a **very floppy walking left arm despite a stable hand**, possibly pre-existing and newly noticeable in landscape. Investigate the actual solved arm/skin; do not change the accepted Grip or suppress authored walking/body behavior without evidence. Explicit no-phantom/no-stuck, Home/resume, cold connection, all menus/dialogs/sliders/scrolling and full secondary-view coverage remain unconfirmed.

A bounded owned-PID 2782 return-log excerpt has no VUID/fatal markers. It records surface cancellation followed by generations 3/5/7 successful native initialization at **17,360.165 / 17,742.531 / 18,243.408 ms**, each with `compiled_pair_reused=0`. Resize/configuration callbacks retain a generation and reuse the pair (recorded examples 46.210–267.425 ms). These are native initialization CPU wall times, not displayed latency or sustained FPS. The new configuration callbacks say `viewportChanged=true`; the excerpt alone does not identify whether the subsequent cancellation came from background pause, surface destruction or another configuration/lifecycle event. Debug-only lifecycle reasons (resume, pause, stop, focus gain/loss, surface destruction and Activity destruction) and viewport-dimension instrumentation now passes the same 225 Java tests and lint in a separate source checkpoint; it is not installed or physical cause evidence; no background-suspension, Graphics ACK or confirmation timeout is weakened.

Validation: current immutable f2 source passes **8/8** selected host checks in **43.13 s** (held transitions, motion scenarios, actual held sockets/rig, sword authority agreement, simulation timing/gameplay, normal waterfall route and showcase route). The wide Rag torch matrix passes **3,287** final-rig poses, including **1,812** actual skinned roof cases, worst measured headroom **32.3659 mm**, maximum Grip error **0.346647 mm**, zero failures. Actual parry triangle checks now cover portrait, 4:3 and 16:9; wide draw/sheath/reversal/death-interruption attachment checks pass. Before-change 3 cm/no-depth comfort assertions fail; a first after-change test wrongly assumed roof corrections could not vary with depth. Its failures and diagnostic are preserved, and exact-offset fixtures now use a horizontal view while independent pitched roof matrices retain real clearance checks.

Android passes **225/225 tests in 35 classes**, no failures/errors/skips, plus lint. The input-only manifest test reproduces before the fix. A parent review regression also catches an early unchanged layout pass incorrectly consuming the pending rotation listener; the original changed-size listener behavior is preserved. PowerShell package/foundation checks require the new manifest mask and parse successfully.

The first f2 Windows Pipeline capture preserves a **failure** at `finale-roof` after eleven successful checkpoint records: the old capture contract requires a primary-visible sword even when the authoritative checkpoint sword is fully body-stowed. Final lantern Grip/authority errors are zero, not a Grip alignment defect. f2 push/PR Vulkan CI also fails its old ready-equipped fixture because it omits the required Hips mount for a now-stowed sword. Follow-up `cf0ffb86` explicitly keeps those carry/reward fixtures ready-equipped, retains separate actual stow/right-arm coverage, and qualifies only sword primary visibility by a strict stable BodyStow tuple. Partial/drawing/held/detached/invalid tuples cannot waive it; all reward-ring/body/player masks and Grip tolerances remain. Three affected checks pass in **36.53 s**. Exact cf0 aggregate CI completes **12/12** (push `37735930283`, PR `37735934731`). The earlier f2 runs `37734588107` / `37734593798` remain **10/12**, with both Vulkan jobs failing that fixture. The cf0 Pipeline rerun passes the corrected finale record, then preserves a new failure at `two-enemy-combat`: zero primary arm pixels, with the old camera still in the first room while the production guards are in the waterfall room. Twelve checkpoint records complete before the failed thirteenth attempt; the frozen image shows the torch without a visible hand. Its owned process exits 1 after 36.380 s with zero VUID/synchronization markers; Compute was not started after the failure. Binary SHA-256 `979570447924d68dabae2138a0b1133618a8d13e81cbbbc509d6f4a2b7fcc1c0`. This was the failed capture/visibility gate before the route-aware a22 follow-up below; preserve it and do not assign either Windows capture to the installed phone APK.

Capture follow-up `a22aaae12babbf2fb06287d3de33895bd4f95870`, tree `5cefc96122fbbada2718705acd0034e402c4c103`, resolves the named production combat camera to the waterfall entry (-3, -15.2, west-facing, pitch -0.06) through one shared route-aware checkpoint value. The historical first-room comparison remains unchanged. Simulation import, Windows stored metadata, Android Debug resolution and runner zone agree. Before-fix production camera regression fails; three affected host checks pass afterward in 13.86 s, witness policy passes 87 injected assertions, and Windows/four-ABI builds pass. Sealed Windows EXE SHA-256 `f864681c7720415da5f55b2e330d6cb088b5815aadebc7cb965b5250fdb9a5f3` completes **13/13 Pipeline and 13/13 Compute** checkpoints in 39.244 / 39.719 s, owned processes 61924 / 54636 exit 0, zero VUID/synchronization errors. All 26 frames join to successful submission/completion/presentation, PNG dimensions/hash and 52 actual uploaded viewmodel/world-body mesh hashes. The actual guard view has primary-visible arms and the authoritative body-stowed sword. No visibility assertion is waived. These frozen desktop captures do not close moving, phone, sustained performance or owner gates, and the installed phone remains f2.

Actual-rig walking diagnosis uses 137 deterministic 60 Hz samples (2.25 gait cycles) at portrait and 16:9, fixed clearance geometry and ordinary Torch carry, with a fixed-Grip comparison. The raw clip moves the shoulder 96/79/93 mm while segment lengths remain constant within 0.0000003 m. Production wide carry reaches a 33.9 mm adjacent elbow-contour step and 40.1 mm arm-vertex P95 step; maximum Grip error stays below 0.011 mm. These are CPU skin measurements, not a rendered appearance verdict or performance pass. An idle-arm local-rotation blend barely improves wide P95 (40.11 to 38.17 mm) and is discarded. The fixed-target/production comparison then traces the wide peaks to four near-full-extension chain-stretch transitions. A constant 10-degree ordinary-Torch bend reduces measured wide elbow-contour step 33.91 to 17.86 mm and arm P95 40.11 to 21.25 mm; portrait stays about 21.1 mm and the Grip remains exact. This supports a bounded shared-animation repair under validation, not an owner acceptance claim. Body locomotion, ceiling/weapon clearance, transitions and physical review remain required.

## Held-Torch walking repair — host-validated, phone review pending

The shared animation input now explicitly identifies the held original Torch. It uses a constant **10° preferred left-elbow flexion** to reduce the measured near-full-extension discontinuity. Reward-lantern policy and empty/released-hand policy retain their existing authority. Torch Grip/pole/target, body walking clips, sword timing/range, ceiling clearance, flame/light and the accepted wider/closer spacing are unchanged.

A new actual-rig paired regression checks 137 samples at 60 Hz with identical non-flexion pose histories. It fails with the repair removed. With production carry, the measured boundary elbow step falls **33.91 → 17.86 mm**, arm-vertex P95 **40.11 → 21.25 mm**, and maximum vertex **63.72 → 33.71 mm**. The fixed-target comparison also improves; maximum production Grip error is 0.00036 mm. An initial after-change invocation reused a stale MSBuild object after a timestamp-preserving source restoration and failed; it is retained as a build/invocation provenance failure. Explicit recompilation and the subsequent focused/full actual-rig tests pass. This is CPU skin evidence, not owner appearance acceptance.

Parent validation passes five affected checks (held sockets, sword authority, simulation timing/gameplay and waterfall route) in **46.56 s**. Fresh full portrait and wide final-rig matrices each pass **3,287 poses**, including **1,812 actual skinned roof cases** with 15,855 viewmodel vertices per pose. Worst headroom is **32.3757 / 32.3758 mm**; maximum Grip error **11.8211 / 0.346647 mm**, both within the existing 15 mm tolerance; zero failures. Animation-policy and full skinned-model smoke tests pass. Android's unchanged 225-test/35-class roster and lint pass in 27 s with the extra passive stop/focus diagnostics. No lifecycle behavior or confirmation timeout is changed.

The route-aware a22 source separately passes **12/12 aggregate CI jobs**, push [37738788156](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37738788156) and PR [37738793667](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37738793667). The installed 6c8 package above carries the repair and diagnostics, with a complete multidex roster and exact installed-base receipt. **Walking appearance, natural sheathed right-hand rest and the cause/fix of slow BB-51 reconnect remain open.**

## Owner landscape check — 6282ab66 / Backbone BB-51

Evidence class: **qualitative owner hands-on feedback on the exact installed
6282 Debug APK**, SM-S948B / Android 16, **Backbone One PlayStation Edition USB-C,
model BB-51** (owner-identified). Generation/firmware and Android InputDevice
descriptor are still unrecorded; no other Backbone/controller model is certified.
The owner explicitly confirms D-pad and stick select the expected neighboring
controls on Play/More/Settings and Settings → Graphics → Back: “yes it all works
well”. Their normal-route report places the skeletons correctly in the waterfall
room, and landscape parry no longer hits the torch. This accepts that observed
placement/menu/landscape clearance, not every pose or secondary view.

The owner intentionally switches **Dust to Low** and accepts its appearance.
This is an owner custom preference, not a new default or matched sustained cost
result. Dust remains default Off, and motes motion/camera/light/secondary-view
coverage and Off-versus-Low sustained thermal/power/memory remain separate.

The sword is still drawn at dungeon start. The owner asks whether start-sheathed
and draw-on-spot belongs here or a future update. The existing behavior remains
production-disabled while the repaired hand and transitions are reviewed; a
bounded normal-route/reset/defense readiness check will precede enabling it for
this 1.6.2 scope. No timing/window/reach or extra enemy change is implied.

The BB-51's fixed attachment prevents the owner from exercising portrait and
the opposite landscape direction with this controller. At their request, the
same installed base hash is rechecked and the owned app is reopened for **touch**,
without reinstall, force-stop, session reset or settings changes. The owner then
reports portrait/opposite-landscape menus/Graphics, move + look + actions and
parry hand/hilt clearance all work well, with no issue. This accepts those touch
observations; it does not close controller orientation coverage. Saved Dust Low
is confirmed in the returned preference XML (50%, Mobile water/fire, Glass On,
Current, cap30, Mist On); fresh/reset source defaults remain unchanged.

The owner subsequently detaches the BB-51 during walking with the stick held,
and separately while holding LT/Parry. Disconnection pauses the game; quick
reconnection restores controller navigation in the pause menu. **Resume also
shows “building Vulkan RT” and a long rebuild**, so overall recovery is not
accepted. LT triggers the existing timed parry on press; holding it does not
extend the animation. Touch Resume, Home/return, explicit no-phantom/no-stuck
observations, cold/hot connection, focus and the full retry/ending/confirmation/
slider route remain unconfirmed. Current stow/wide body/shadows/reflections,
sustained performance and independent audit remain open.

On USB return, a bounded owned-PID excerpt records completed generation3/5
initializations at 17,723.948 / 19,595.814 ms and their subsequent presentations.
Generation7 records 34,399.761 ms initialization, with 34,217.056 ms in the
pipeline bundle, but that surface generation was cancelled during creation;
it does not own a presented recovery frame. Earlier real rotation/suboptimal
output recreations reuse the compiled pair in 228.985 / 169.530 ms. These are
native CPU initialization durations, not displayed pauses, sustained FPS or
causal proof of which configuration event caused teardown. No VUID/fatal markers
appear in this owned excerpt. Source manifest handles orientation/screenSize
but lacks keyboard/navigation hot-plug changes; that activity-recreation path is
under focused regression investigation. Physical corrected-build A/B remains
pending. The earlier quiet USB-return excerpt has no render markers; absence of
optional markers is not itself a render failure.

## Installed phone handoff — 6282ab66

At 2026-10-08 05:06:23 UTC, the intended **SM-S948B / Android 16** receives the
sealed 6282 APK described below. Installed base SHA-256 exactly matches
`11a49d954a714867cb1e8dbdf8c3690b05d3d7523f4c8efc8bc23b654f974853`.
There are **zero changed preference entries**; owner rotation `free` is unchanged.
Only the owned Debug app is stopped before replacement. The previous c8 paused
session and owned log excerpt are retained as historical evidence; no uninstall,
data clear, rotation override or other app operation is performed.

Normal cold launch succeeds (Android launch report TotalTime 526 ms / WaitTime
528 ms), with no scenario extras. An owned startup log excerpt contains one
`HORDE_SURFACE_PRESENTED` and 12 `HORDE_GPU` markers, zero VUID/fatal markers.
These are excerpt counts, not whole-route validation, an exact owning RT image
join, displayed/sustained FPS or controller acceptance. The foreground is
verified as this app before capturing the portrait entry screenshot: 1440 x 3120,
SHA-256 `add875b165883936d325033890d8d1ec921f2e3445a0bb80b1257722487f1cf2`. Its real separate Play/More/Settings plaques and central
lantern are visually inspected. This is an app-window screenshot, not RT storage
image evidence or a new owner appearance approval.

The owner is asked to test D-pad/stick neighboring controls through entry and
Settings/Graphics/Back, then the ordinary first-room/waterfall route and parry
hilt/hand versus torch in their chosen orientation. No automation runs while
they hold the phone. Backbone One PlayStation Edition USB-C is owner-identified;
generation/firmware/Android descriptor remain unrecorded. Connection/lifecycle/
rotation/unplug/reconnect/Home and first-touch recovery matrix, current moving
appearance, sustained quality/cost and independent audit remain open.

Private receipts in the 6282 seal: `install-receipt-private.json`,
`installed-base.apk`, before/after preference XML, `phone-normal-launch-private.txt`,
`phone-new-candidate-last250-log-private.txt` and `phone-current-entry.png`.
The first screenshot guard looked only for Android's older `mResumedActivity`
field and failed before capture; the corrected bounded check admits Android16
`topResumedActivity`/`ResumedActivity` only when both belong to this app.
No product failure or foreign app screenshot is inferred.

## Prepared normal waterfall encounter — 6282ab66

The owner reports first-room skeletons in the installed c8 build. That build
still uses the historical production encounter flag. This candidate enables the
accepted waterfall placement independently of sword-starts-stowed, which remains
Off. The same Skeleton A/B retain health 1, count two, east-facing lateral lanes
and the authored 0.65 s gait offset. Historical default-config fixtures remain
available; there are no new enemies, combat rules or forced camera changes.

A related reset defect is reproduced: with only placement enabled, bay import,
Keeper import and Keeper retry reset guards at the old first-room location.
The shared reset/import now chooses the authored pair from configuration,
regardless of the currently selected enemy. Ordinary room changes still preserve
state rather than respawning guards. Existing explicit reset/retry health rules
remain intact.

Source `6282ab66289775c6c8360a7feb3afd6d554f1787`, tree `23883a344819f61a8c3f38c7f0e7403520abdd34`.
Prepared four-ABI Debug APK: 138,462,724 bytes, SHA-256 `11a49d954a714867cb1e8dbdf8c3690b05d3d7523f4c8efc8bc23b654f974853`.
Windows Debug executable: 11,559,424 bytes, SHA-256 `b0d07f3c2906b461751ea9301bc3e9695f20a71e684df0c9f87c01e377a4f711`.
Exact artifacts: `task-4/integrated-162-6282ab66-20261008`.
All four native libraries change; manifest, 91 assets and Java dex are identical
to 6fa. SDK37 16 KiB alignment passes. The 222-test/35-class Java and lint result
is retained at **6fa**, with no fresh Java rerun claimed for this native slice.
The prepared APK includes that directional fix. Neither change is physically
accepted at this build checkpoint; the later exact install/handoff is recorded
above. The c8 paused session was preserved throughout this host work.

Seven affected native CTests pass in 52.50 s: simulation gameplay, motion
scenarios, ordinary production route, showcase collision route, simulation
timing, held-item sockets and sword authority agreement. The new route uses
ordinary movement axes and real 60 Hz collision through 13 destinations, with
the final destination at the playable Keeper arrival threshold. It covers all
four bays, first-room isolation with normal damage enabled, actual guard walking
phase, wetline bounds, retreat/re-entry, pause, cold/reset/import and both retry
paths. Route geometry disables player damage while travelling; it is not combat
feel acceptance. The existing full Keeper lighting/death/chest/claim/retry helper
also runs against production configuration. Both Windows and all-four-ABI builds
pass (Android assembly 44 s).

Before-fix failures remain: the new route fails 18 checks against old defaults.
The first config-only run has three genuine checkpoint/retry failures and two
fixture-only phase assumptions: constructor publishes authored Walking samples,
whereas zero-delta reset outside the arena publishes Idle0. The corrected fixture
retains the explicit live gait assertion and isolates exactly the three real
failures before the shared reset repair. Earlier two affected suite failures,
the wrong Windows CMake target invocation and the stale native-exit check after
the PowerShell APK resolver are retained separately; neither invocation failure
is a product/build defect. Corrected commands pass.

Fresh frozen Pipeline and RayQueryCompute showcase runs complete 13 checkpoints
each on RTX5050 Laptop, High water/fire, Current shadows, Mist On, Dust Off,
100% at 960 x 540. **26/26** completed/presented submission, PNG and uploaded
viewmodel/world-body mesh hash joins pass; zero synchronization/VUID errors.
Elapsed validation is 38.109 s / 38.228 s. Both owned processes exit normally.
Across backends, all 13 uploaded mesh pairs match; PNGs are not byte-identical.
Descriptive 8-bit channel differences have a largest per-image/channel mean of
0.003311 and a largest isolated channel delta of 78 (mirror checkpoint). No
parity threshold or cause is inferred; the raw comparison remains in the seal.
These are scene-only Debug stills, not live motion, scanout, phone, audio/haptic,
owner acceptance or sustained performance. [Selected stills and exact hashes](evidence/2026-10-08-normal-waterfall/README.md).
The settled-torch room checkpoint is deliberately dark and faces back toward
the waterfall; it does not establish normal moving-combat readability.

Current source CI completes **12/12**: push 37729370167 and PR 37729376579
each pass 6/6. Previous navigation
6fa and documentation 1abd each complete their own 12/12 aggregates. Previous documentation 78b804bc also completes its own 12/12 aggregate (push
37730472200 / PR 37730476379); this install record has separate CI. Current
moving body/shadow/reflection, phone
route/controller recovery, corrected menu acceptance and combat readability
remain open. Dust/cost, audio/haptics, quality decision and Eric's independent
final audit remain open. No release is authorised.

Private evidence logs: `waterfall-production-before-ctest-20261008.log`,
`waterfall-production-route-before-ctest-20261008.log`,
`waterfall-production-config-only-ctest-20261008.log`,
`waterfall-production-config-only-corrected-fixture-ctest-20261008.log`,
`waterfall-production-after-ctest-20261008.log`, corrected Windows build and
four-ABI build logs under `task-4/test-temp`; exact capture admission in the seal.

## Prepared directional navigation follow-up — 6faab4ff

The owner tested the exact installed c8 APK with **Backbone One PlayStation
Edition, USB-C** on SM-S948B/Android 16. Generation, firmware and Android device
descriptor are unrecorded. The prompted menu/gameplay actions worked well overall;
no other missed/stuck input was reported. Joystick/D-pad navigation sometimes
picked an incorrect neighbor and the opening plaques were not directional.
This is preliminary manual feedback, not a completed physical acceptance matrix.
The owner reconnects USB and leaves the same Debug PID 11530 paused, then goes
AFK with the phone connected. No test requests are sent while away.

The root cause is reproduced in four host regressions against c8: activity-wide
focus search can escape into underlying controls, and its creation-order fallback
can turn Right into Down. Selected controls alone were made focusable in Android
touch mode, excluding other native buttons from spatial search. The follow-up
scopes Android FocusFinder to the menu/dialog, temporarily admits its real controls
and restores their original touch policy. A weak UI-thread map retains original
policy for selected controls, restored before intentional touch/fallback. It removes
creation-order navigation, uses vertical scrolling only for Up/Down, and starts
entry selection on Play without changing plaque positions or real callbacks.

Source `6faab4ff26b495ab782d2d2a3cdbaccc4e585026`, tree `e027245aa51d6d09a7c3abc7cb79680eac7b26e0`.
Four-ABI Debug APK: 138,462,724 bytes, SHA-256
`d24dfd8f5f0f88e6a693b9723ca6a6cd8616885f1490999a6d34d394ef52d26e`; sealed in `task-4/integrated-162-6faab4ff-20261008`.
SDK37 16 KiB alignment passes. The manifest, all 91 assets and all four native
libraries match c8 byte-for-byte; Java dex changes. Windows runtime is unchanged
and retains the exact c8 build/hash. **222 tests in 35 classes**, zero failures,
errors/skips, and lint pass in 28 s; APK assembly passes in 2 s. Tests include
scoped geometry/edges, real entry D-pad presses, portrait/landscape/larger text,
vertical offscreen focus/scrolling and touch restoration, plus the existing full
touch/Graphics/controller/lifecycle roster. Independent focused source review
finds no concrete defect and prompts the added scrolling regression; it is not
Eric's final audit or physical acceptance.

Retained failure: the first `requestFocusFromTouch` attempt fails 9/25 host cases
and is abandoned; its log/XML remain separate. Corrected focus-policy runs pass
25 and then 28 focused cases; the final full run includes the scrolling case.
Before-fix logs/XML retain the four reproduced failures. Exact private logs:
`controller-spatial-navigation-before-20261008.log`,
`controller-spatial-navigation-after-20261008.log`,
`controller-spatial-navigation-focus-policy-after-20261008.log`,
`controller-spatial-navigation-orientation-touch-20261008.log`, and
`controller-spatial-navigation-full-tests-lint-20261008.log`.

6fa source push 37727570776 / PR 37727574270 are still running at this checkpoint
(2/6 successful each). c8 push 37725320895 / PR 37725325043 completes **12/12**.
Documentation has separate CI. The 6fa APK is not installed or owner-accepted.
The paused c8 screenshot and owned last-200-lines excerpt are captured after USB
return: 236 lines, one surface-presented marker, two GPU markers and zero
VUID/FATAL markers in this excerpt. These are not completed-frame/hash joins,
full-route or sustained/displayed-FPS evidence. Initial c8 log/capture gaps below
are retained. Moving equipment acceptance and full physical matrix remain open.

The first-room skeleton report matches the production waterfall flag being off.
The existing shared authored pair already has accepted lateral placement/facing
and distinct gait phase. Enable that flag independently of sword-starts-stowed
only with the normal collision-route/Keeper/reward and affected moving/render
checks; the stow flag has its own gates. This follow-up changes neither flag.

## Current installed follow-up — c8cbb4a4

- Runtime source: `c8cbb4a4ca4341b555d2c138008fcb2305011cf7`; tree `bea0121107417eafda2c1aa4f8b6a1b1eaa3f6fa`.
- Windows Debug: 11,553,280 bytes; SHA-256 `7ac86dd2fd2a3535f385b21758a4538eb762a5a270d8886954bad5dbce8c6fc7`.
- Four-ABI Debug APK: 138,462,724 bytes; SHA-256 `2467d05e0042976f7303a31094b9ffff13720f14c93bc811b5602d94b42a4c7a`.
- Exact-source packages are sealed in `task-4/integrated-162-c8cbb4a4-20261008`.
  SDK 37 16 KiB alignment passes. Manifest/fullUser/code 10/1.6.2-debug, all
  91 assets and Java dex are byte-identical to the 454 package. All four native
  libraries change. The original 454 Java 213-test/lint pass is retained with
  its own identity; no fresh Java rerun is claimed for this native-only slice.
- Seven affected native CTests and Windows build pass; four-ABI assembly passes
  in 40 s. [Node-query baseline, coverage and limits](ENGINEERING_1_6_2_ANDROID_MOTION_2026_10_08.md#bounded-node-pose-query-follow-up-8-october).

Native payload SHA-256:

- `lib/arm64-v8a/libhorde_rt_probe_android.so`: `6fbbb1f61e7ad9c6e9d1254ffc9355fea20d75ef7c28e51da06da6267227067f`.
- `lib/armeabi-v7a/libhorde_rt_probe_android.so`: `9c77e1ddcde9e1c587ef423f2cae77f84aa59b7de8f8d8fc97c03b56411e6780`.
- `lib/x86/libhorde_rt_probe_android.so`: `d9709ae850a89b653bbf2448ea50826930d2882ad7d738665dfcfc0be5f0d39a`.
- `lib/x86_64/libhorde_rt_probe_android.so`: `fd78d73c5247920a35a71ca77c990100bba02e8a7e302e028e6f03cb1a7620b6`.

SM-S948B/Android 16 was identified before installing only
`com.samfa12.hordelanternrt.debug`. Rotation was `free` and remained untouched.
Install/pullback verification completes at 2026-10-08T04:01:57Z with **zero changed
preference entries** and the exact APK hash above. Normal cold launch succeeds;
the immediate screenshot captures loading, so it does not prove ready RT
presentation. USB disconnects before the ready capture/log collection. The
owner is handed the opened build for a physical Backbone check; logs can be
collected after USB return. No synthetic controller or physical pass is claimed.
The model/edition, cold/hot connection, menus/Graphics, combat, both landscape
directions/portrait, rotation, unplug/reconnect/Home and safe touch fallback
remain outstanding observations.

Pre-install Android input enumeration contains no controller. The computer
reports generic HID controller VID 358A / PID 0204; this is a Windows identity,
not an identified Android device/model. The first log attempt has a malformed
`--pid` argument; its corrected time-filtered client reaches the existing 12 s
deadline and is terminated. The next bounded state check reports USB absent.
No wait loop, unrelated process termination, data clearing or security change
occurs. Exact install/prefs/launch/capture and failed invocation records are kept
privately. Package hashes remain immutable.

Source c8 CI completes successfully at 12/12: PR 37725325043 and push
37725320895 each pass 6/6. Source 453 separately passes 12/12.
No earlier CI, appearance or performance result is assigned to c8. The earlier
454 frozen captures below remain their own evidence; c8 adds no new completed
RT capture, moving/secondary-view acceptance or sustained FPS claim.

## Current integrated follow-up — 454d58ee

- Runtime source: `454d58ee0f71bdb837a75ce486d6c139e03e0b02`; tree
  `28d7b7467305821f7e1b31fa8a9f50893c334d25`.
- Windows Debug executable: 11,553,280 bytes; SHA-256
  `b538d71a559b2c9bec3ebd764e565f880220c02bcdf634b0fb63ecb897e9caec`.
- Four-ABI Debug APK: 138,462,724 bytes; SHA-256
  `12dfe45d1903bce5a6c11ee01a522603f2657d4a248b346e1ae3a7082f14e973`.
  Both packages were built at this exact source and sealed in
  `task-4/integrated-162-454d58ee-20261008`. No installation or phone test occurred.
- Native APK payload SHA-256: arm64-v8a
  `a64b35dc1caca9c0e37d5704636f6bc73ea7bd25fcb9ee8fedaa75fa025a9905`;
  armeabi-v7a `9c46a08ee0baa9b07919e11916f219ee2ee50e75a25ae2b3478ce75efdc0b26d`;
  x86 `d989f237f3b4a063a4d7146fba4cbaae4be15de423ce2062397aea837761e08d`;
  x86_64 `4724341a0296c22822202d82c11b52904577dbf79da064bb80f937c2f4bc5f7c`.
  All four native libraries changed after the shared pose extraction; all 91
  asset entries remain byte-identical to a6/c2. Manifest and SDK 37 16 KiB
  alignment checks pass with the same orientation/version settings.

A native dialog could previously keep polling a held navigation control after
its own Window lost focus. The activity's focus flag alone does not describe a
focused dialog. The follow-up tracks each owned dialog's actual Window focus,
gates key/joystick/poll delivery on it, neutralizes held input on loss and requires
fresh releases/neutral axes on return. Dismissal and parent focus restoration
preserve native callbacks, consent, graphics serial/generation guards and the
exact acknowledgement timer. Two new regressions fail against the a6 activity
and pass after the fix. Robolectric fixtures dispatch both View and
ViewTreeObserver focus notifications, matching the actual framework route;
physical Android Window delivery remains pending.

`SkeletonRenderPose.h` now owns the skeleton clip/time/root transform used by
both `CharacterRenderSlot` and the actual-mesh contact diagnostics. Removing the
diagnostic's copied mapping avoids a second animation convention. The sampled
old/new renderer comparison passes 12/12 cases within 1e-6; retained contracts
also cover zero/negative death duration and action/animation precedence. This
extraction preserves pose behavior and changes no combat range/cone, contact
pulse, parry window or damage rule. It supplies a reliable input for the remaining
moving-target calibration; it does not resolve that calibration by itself.

Current affected checks:

- **213 Android tests in 35 classes**, zero failures/errors/skips, and lint pass
  in `android-controller-dialog-full-tests-lint-20261008.log`. Four-ABI Debug
  assembly passes separately in 8 s; Windows Debug build passes.
- Shared pose edge contracts pass; the earlier full socket check passes in
  17.41 s. Renderer smoke passes in 4.88 s after correcting only its old parry
  expectation to the already-implemented torch-clearance pose.
- Test-only commit `03fe9ae209c4b6b540c5b8df183b22db99ba745b` registers the finite
  shared pose contract command in CTest. Its targeted CTest passes in 0.02 s.
  The two-line registration does not rebuild or relabel the sealed runtime.
- Eight fresh frozen captures from 454: both real RT backends, both recorded
  dimensions and both inspection poses. All eight PNGs, viewmodel meshes and
  player-world-body meshes are byte-identical to a6. Exact completed/presented
  RT submission joins pass, zero synchronization-validation markers, all eight
  owned process IDs verified absent. This retains the corrected a6 appearance,
  not continuous motion, complete secondary views or phone acceptance.

Retained failures include the initial attempt to compile the Vulkan renderer in
a portable fixture (missing `vulkan/vulkan.h`), the stale renderer smoke parry
expectation, and initial focus fixtures that omitted the Window observer event.
The authoritative corrected two-case baseline reproduces both defects against
a6; the focused 18-case and final 213-case fixed runs pass. No failed run is
relabeled. Exact private logs include
`android-controller-dialog-observer-corrected-before-20261008.log`,
`android-controller-dialog-focus-observer-after-20261008.log`,
`aspect-render-plan-shared-helper-equivalence-20261008.log`,
`aspect-render-pose-helper-edge-contracts-20261008.log`,
`aspect-render-pose-helper-edge-smoke-ctest-20261008.log`,
`skeleton-pose-contract-ctest-20261008.log`, and
`controller-pose-final-captures-20261008.log`; the seal contains the hash joins.

Runtime 454 push [37712231672](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37712231672)
and PR [37712236068](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37712236068)
completed successfully: **12/12 aggregate jobs** (push 6/6 and PR 6/6).
Test-registration 03fe has separate push 37712764483 / PR 37712770661 CI,
completed successfully at 12/12. Earlier documentation f000 completed 12/12.
Documentation 6e67be51 first attempt has 11/12 aggregate jobs successful: the
push MSVC native report-form Unicode edit times out at its existing two-second
deadline, while the matching PR job passes. Its single failed-job retry completes
successfully, giving 12/12 current aggregate jobs; the original failed JSON/log
remain preserved. Test-only 22920c19 separately passes 12/12 in push 37714363918 /
PR 37714368236. Documentation a65c03da first attempt passes 11/12 in push
37714583032 / PR 37714590338: the PR Windows job hits the same Unicode-note
timeout. Its original failure is preserved; its single failed-job retry completes
successfully, giving 12/12 current aggregate jobs.
No code or deadline is changed, and successful retries do not establish the
cause of these intermittent native UI failures. Results are not assigned to
other source or package identities. Build correctness, physical acceptance and
sustained performance remain separate.

## Earlier a6 candidate and preserved evidence

- Candidate source: `a6c19938994db95f9764b7fcf3eaa888811ecc1f`.
- Candidate tree: `4d19032a2ed55b2b4ce085a97699027a00a8c7dc`.
- Windows Debug executable: 11,553,280 bytes; SHA-256
  `fa5102bff6acfdbfdfdc2a2990fa3c8898b381d2b75369230eebd9a0d008f313`.
- Four-ABI Debug APK: 138,462,724 bytes; SHA-256
  `71eafd106f46ccc32ae6ca7c47c0da25dc177b3a254692158d7bd8bf906f5bda`.
- APK build source: `cb7c80f6c8d9a525594c088376617e2bc3b4b1d0`. It is reused byte-for-byte:
  the later candidate changes only Windows capture aspect delivery and its host
  regression. Android/runtime assets, shaders, shared gameplay and Android build
  inputs are unchanged between those sources. Do not label it rebuilt or installed.
- Native APK payload SHA-256: arm64-v8a
  `ab0e6d9f3cef1f02e0bd380bef70eac3a7b6619d1d963399d11d3ba4757276f0`;
  armeabi-v7a `6e5790a0007efc7cd00eb05e5b6bda45a7d43ecfac0c8a40e0e5dc0a59adaa97`;
  x86 `fa3035cde818189e2d78e1731b3c2fe18de16d96fadd79abfa70e47c5c8e3b90`;
  x86_64 `5a7527ebb27e1d6b0cee3ee6b0209b85bb5d5b7f9bba3d241ef6d27ecc642a3c`.
- All 91 APK asset entries are byte-identical to the earlier c2 candidate's
  closed asset admission. Manifest remains 1.6.2-debug/code 10, fullUser (13),
  orientation|screenSize (0x480). SDK 37 zipalign verifies 16 KiB alignment.
- The earlier `c2de5d7d` sheathed-hand/parry correction, its exact packages,
  ten native checks, 183 Java tests and 12/12 CI retain their identities.
  [Earlier equipment record](ENGINEERING_1_6_2_ANDROID_MOTION_2026_10_08.md#parry-hilt-and-gauntlet-clearance-candidate).
  Prior owner touch/torch/low-ceiling acceptance on `ccd70d38` and smaller-dust
  still acceptance on `998137c9` do not certify this new input or pose candidate.

Private artifacts and exact logs are retained under
`task-4/integrated-162-a6c19938-20261008` and the preceding cb7 seal. No release,
production signing, merge or independent final audit is claimed.

## Android input and native navigation

`AndroidControllerInput` maps standard Android controls rather than Windows
button numbers. A is Interact/Confirm, B Dodge/Back, X Swing, Y lantern,
L2 Parry, R2 Swing and Start/Menu Pause. Keys act on fresh press-down edges.
Digital and analog trigger reports share one latch; analog press/release
thresholds are 0.50/0.35. D-pad keys and hat input share navigation state.

Actual MotionRange bounds/flat values normalize sticks with a radial dead zone
of at least 0.12. Right Z/RZ is preferred, with RX/RY fallback. Neutral/drift
samples neither take over touch nor steal the active device. Four bounded
inactive candidates may become active only after neutral readiness and meaningful
input. Suspend/reconnect require fresh key releases and neutral axes; repeated,
stale and duplicate routes do not create a new action.

`MainActivity` publishes through the existing ProbeBridge coherent mailbox.
The existing shared 60 Hz simulation continues to own gameplay and timestamped
semantic edges. The UI callback integrates look into the same absolute view
controls, clamps delayed callbacks to 50 ms and resets its clock on suspension.
There is no second controller gameplay simulation.

Controller navigation uses real enabled native controls and callbacks, visible
focus and scroll-to-focus. Left/right adjusts SeekBar through its native user
change route. Native spinner/Graphics choices use focused labelled buttons;
Graphics uses the same presented option, serial, generation and busy guards.
Use/Keep/Restore and the confirmation starting after exact native acknowledgement
are unchanged. Native dialogs route keys/joystick navigation and restore a still
visible parent dialog after dismissal. Menu confirm cannot leak into combat.

Touch action/menu visuals hide only after meaningful controller use. Health,
required context and generic Android prompts remain subject to the saved Show HUD
and interface preferences. Connection alone never changes mode. An intentional
first touch restores actual hit targets before dispatch and neutralizes old axes
and held touch roles. No preferences are rewritten. Prompt names describe the
standard mapping; they do not identify or certify a Backbone model.

Focus loss, Home/pause, rotation and destruction neutralize axes, look and held
actions. Actual active-device removal also pauses gameplay and exposes touch
recovery. A device property-change callback reseeds state rather than treating
a still-present gamepad as disconnected. Native scroll pages resize within
existing insets in both landscape directions and portrait without rebuilding
focused controls or rewriting settings. Existing Vulkan pre-rotation/recovery
contracts remain in place.

## Shared equipment aspect input

`logicalViewAspect` comes from logical output extent and its presentation
transform, independently of render scale and platform. Square and portrait add
zero separation. Smooth interpolation reaches 3 cm outward per hand at 16:9 and
caps there. The offset enters shared held-item targets before anatomical reach,
forward/wall and overhead clearance; the arm IK, item Grip and light consume
those same solved targets. Released torch world trajectories are unchanged.

The owner-thread aspect setter refreshes presentation snapshots at zero delta,
including frozen/paused checkpoints. It does not advance gameplay, damage,
contact timing or reach rules. Host checks sample named idle, swing, upward slice
and parry action states at their default phase times; they are **not a full
animation-time sweep**. Existing bounded torch/wall/walk and sword-roof fixtures
also exercise wide views. Moving equipment and secondary views remain pending.

## Checks and retained failures

- Final Android run: **211 tests in 35 classes**, zero failures/errors/skips;
  `testDebugUnitTest`, `lintDebug`, four-ABI `assembleDebug` pass in 33 s.
  This includes 10 controller policy, 16 activity integration and two native
  navigation tests; existing touch, Graphics, lifecycle and settings regressions
  remain in the full roster.
- Policy coverage includes mappings, actual axis ranges/dead zones, repeats,
  digital/analog deduplication, D-pad/hat union, device/drift ownership and
  suspension rearming. Activity coverage includes move/look/actions, HUD/prefs,
  first-touch fallback, Start/pause, Entry/More/Settings/Play, Graphics, disabled
  controls, native dialogs, death/retry/ending, disconnect/reconnect, focus,
  rotation/page resize and keyboard separation. Native slider callbacks and
  scrolling/focus repair have focused tests. These are synthetic host checks.
- Three affected native CTest targets pass: held-item clearance (5.39 s), socket
  (22.16 s), Windows controller input (0.04 s), total 27.61 s. The Windows-only
  frozen-order regression and diagnostic/socket rebuild pass afterward.
- Eight fresh RTX 5050 Laptop frozen captures: Pipeline and RayQueryCompute,
  landscape 960 x 540 and portrait 540 x 960, sheathed and parry poses. Actual
  completed/presented RT frames, PNG hashes and both uploaded mesh hashes join;
  zero synchronization-validation markers, eight owned processes exited.
  Portrait PNG/viewmodel are byte-identical to cb7/c2; wide PNG/viewmodel change
  on both backends. [Original wide images](evidence/2026-10-08-controller-aspect/README.md).
- Earlier cb7 frozen captures were all byte-identical to c2. They correctly
  presented RT, but did **not** prove the new wide spacing: frozen Windows
  checkpoints skipped the live controls path that received aspect. Candidate
  `a6c19938` supplies aspect before checkpoint import and common frame snapshot
  consumption; fresh evidence above closes that delivery gap only.
- The first 10-case integration run had one assertion failure: native dialog
  click callbacks are posted asynchronously in Robolectric. Waiting for the
  main looper fixes the test; actual native callbacks remain unchanged. A later
  focused 10/10 and final 211/211 run pass; the failed log is retained.
- A first editing command used the Android directory for a source-relative file
  and failed before editing; its earlier build is not the final implementation
  proof. Capture comparison initially used an incorrect prior directory name
  and stopped before remaining launches; corrected paths and all eight captures
  pass. SDK 34 rejects `zipalign -P`; only SDK 37's successful check is alignment
  evidence. No failed attempt is relabelled as a pass.

Exact private logs: `android-controller-final-build-tests-20261008-02.log`,
`controller-aspect-native-ctest-20261008.log`,
`aspect-capture-followup-build-20261008.log`,
`aspect-capture-followup-test-20261008.log`,
`aspect-frozen-runtime-build-20261008.log`,
`aspect-fresh-wide-capture-20261008.log`,
`aspect-fresh-remaining-captures-20261008.log`, and the sealed
`artifacts.json` / `native-inspection.json` / per-case launch and native manifests.

Source `cb7c80f6` passes **12/12 aggregate CI jobs** in push
[37708858800](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37708858800)
and PR [37708865790](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37708865790).
Windows capture follow-up `a6c19938` has separate push
[37710046348](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37710046348)
and PR [37710051071](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37710051071)
CI: **12/12 aggregate jobs successful** (push 6/6 and PR 6/6).
Documentation `f000753d` has separate push 37710860642 / PR 37710864776 CI;
those runs completed successfully, 12/12 aggregate jobs.
Green CI is build correctness, not physical controller or owner acceptance.

Audio/haptic manual revalidation for this input/pose-only slice: **NO**; shared
feedback semantics are unchanged. Earlier changed combat/audio/haptic gates
remain open. This does not establish physical latency, controller comfort,
continuous geometry clearance or a performance result.

## Physical acceptance and final review

| Exact phone/controller checkpoint | Status |
| --- | --- |
| Phone model, Android version, controller identity and exact installed APK/pullback | Pending owner return and intended-device verification; no installation here |
| Cold launch, hot-plug, controller changes; neutral/drift behavior | Pending |
| Move/look/Swing/Parry/Dodge/Interact/lantern/Start together | Pending |
| Entry, More, Settings, sliders, scrolling, dialogs and full Graphics Use/Keep/Restore ACK route | Pending; synthetic routes do not prove Android Window delivery |
| Death/retry and ending/reward with clear prompts | Pending |
| Both landscape directions, portrait and rotation in menu/Graphics; large-font/cutout clarity | Pending; host geometry cannot certify actual presentation |
| Disconnect while moving/holding, safe pause, touch first-interaction fallback, reconnect | Pending |
| Home/resume and focus loss without stuck/replayed input | Pending |
| Idle/moving/swing/parry/low-roof/stow/draw equipment, full body/shadows/reflections | Pending current-candidate motion and owner review |

On return, coordinate one coherent hands-on matrix on the identified device and
controller; one Backbone model cannot certify all Backbone hardware. Keep final
mobile sustained thermals/performance/quality, dust motion/cost, production route
activation, owner audio/haptics and Eric's independent final audit separate.
Fresh Android remains 50%/Mobile/Glass Off/Current/cap30/Mist On; desktop remains
100%/High/Glass On/Current/cap30/Mist On. Custom preferences, historical 75% and
closed experiments remain intact. Dust stays optional/default Off; shafts deferred.

Recommendation: **keep this checked implementation for physical acceptance**,
retain the wider poses for moving review, and defer support certification and
production flags until their exact gates close. No merge, signing, tag or release.

## Owner return and connection logistics, 8 October

The owner returned and the intended SM-S948B phone was verified online on
Android 16. No Android gamepad/joystick was present in the inspected input
roster. The owner reports that the Backbone occupies the phone USB-C port and
is currently attached to the computer. Windows enumerates a generic
HID-compliant game controller, which does not identify a Backbone model or
prove Android support. Install/pullback verification over USB can precede an
owner-controlled cable swap to the Backbone, with owned app logs collected
after USB reconnection. Physical phone/controller acceptance remains pending.
No wireless-debugging, driver, credential or security configuration is changed.

Test-only 45363ac3 push 37716606080 and PR 37716611026 completed successfully:
12/12 aggregate jobs. These CI results belong to that source, separately from
the later node-pose query source and its package.
