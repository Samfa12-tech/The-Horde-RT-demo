# Lantern rebind diagnosis and Android camera repair

The [original phone failure](../2026-10-23-touch-hud-phone/rotation-continuity-gap.json)
is retained and **not fixed or accepted by this checkpoint**. Fixture 116 lost its
lantern control after Home, full surface replacement and landscape rotation.
Independent portrait/landscape restaging had passed, but replaces the state under
investigation. No production equipment restore or forced button visibility was
added on that evidence.

Work continues in the existing `.worktrees/horde-1.7-wp2-vertical-proof` checkout,
based on `8937eed5`. No terrain, shell, parked chimney, save, preference, quality or
gameplay policy changed. The worker stays OFF. The phone connection check returned
an empty device list and `device 'R5GL219SZGK' not found`; this checkpoint performed
no installation, capture, input or transfer and has no owned device work remaining.

## What the bounded investigation established

- Native surface retirement replaces `SwapchainContext`, not the process-lived
  `GameSimulation`. Paused input synchronization preserves reward ownership and
  pose; it consumes stale commands. Presentation aspect re-resolves kinematics.
- A fixture's inspection-only `productionRewardPropsRequested` flag resets with
  its context, but a claimed lantern renders from chest and held-light ownership
  independently. That flag alone cannot explain this observation.
- The new context also drops the debug fixture's frozen sampling mode. Restaging
  therefore changes more than orientation; it is not a valid continuity repair.
- Java touch cleanup deliberately hides contextual controls. The existing runtime
  poll refreshes them every 180 ms when RT and gameplay are eligible. The new
  callback test restores the control after readiness without restaging. This rules
  out a permanent missing-refresh bug in that host scenario, not every phone path.

Two native cases now exercise three combined pause → readiness invalidation →
aspect change → resume cycles, including an interrupted lowering transition:

| Case | Setup | Result and limit |
| --- | --- | --- |
| Fixture 116 | Existing `StageDevelopmentCheckpointSimulation` | Claim, exact paused pose progress, finite coherent hinge/body, stale-command discard and high → low → high pass |
| Production interaction | Production config; imports an unlocked pre-reward chest and defeated-Keeper setup, then actual open/wait/claim commands | Same continuity checks pass; **not** a natural Keeper victory or earned full journey |
| Real Activity callbacks, fake native bridge | `onPause`, `onStop`, `onConfigurationChanged`, `onResume`, unready/ready polls, both settled poses | Control/label recover; cleared axes/pointers remain neutral; menus, transitional poses and rescue traversal suppress it as intended |

Neither native case introduces a rescue prompt or mode. These are CPU and
Robolectric seams: no JNI integration, RT image or physical continuity pass.

## Small diagnostics for the next physical reproduction

Debug builds now log only control-state/generation changes. The simulation owner
emits `HORDE_CONTEXTUAL_STATE` with surface generation, tick, contextual bits,
chest/light/pose, rescue prompt and consumed reset/retry counters. Java emits
`HordeContextualHud` with generation, allowed flag, bits and presentation gates.
Release has no native diagnostic code; Java's diagnostic body is guarded by the
constant `BuildConfig.DEBUG`. There is no gameplay mutation or new JNI accessor.

Useful native witnesses: claimed chest `4`, reward light `2`, settled High pose
`0` offers Lower bit `4`; settled Low pose `2` offers Raise bit `2`. Transitional
poses `1`/`3` legitimately omit those bits. Rescue mode uses bit `512` and a prompt
in bits 6–8. Java `bits=-1` means controls are ineligible and JNI was not queried,
not that the simulation published a negative state. Its gate mask is allowed `1`,
controller `2`, menu `4`, diagnostics `8`, death `16`, ending `32`, benchmark `64`,
capture suppression `128`.

Next phone batch, when Sam makes it available: preserve both installed APKs and all
saves/preferences/rotation; stage fixture 116 **once**; verify current RT and lamp
state; rotate the same foreground scene, then exercise one Home/full-rebind/return
boundary without restaging. Capture paired native/HUD logs and accessible bounds.
Only use observed lamp/menu targets, stop on missing/changed state, and release
after the bounded check. Separately test actual earned equipment when a proven
earned test save or natural claim is available. A fixture cannot close that gate.

## Verified separate CI regression

Exact prior head [8937eed5 run39933824808](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39933824808)
is terminal: GCC, Clang, selected ASan/UBSan and Vulkan-host pass; MSVC is cancelled
at its existing job cap. Android ran 414 tests with one failure:
`ChapterDebugCheckpointTest.allAuthoredChapterCapturesResolveWithTheirNativeCamera`
at line 29. The two F04 side cameras 228/229 were added to the native registry but
omitted from Android name/pose mapping. Their exact native names and yaw/pitch are
now mapped. The existing exhaustive contract remains unchanged; no gate was
deleted, skipped or relaxed. These are diagnostic cameras, not release content.

The prior phone README also incorrectly transposed the landscape internal extent.
It now quotes its actual retained native receipt, 475 × 984, and leaves buffer
pre-rotation interpretation unverified. Physical UI screenshots and their geometry
receipts were already correct and unchanged.

## Validation and open requirements

The compact [receipt](receipt.json) binds source-file hashes, actual test totals,
Debug APK identity and local raw-log hashes. The native Debug lantern test passes
1/1 in 6.67 seconds. The initial parallel MSBuild attempt stalled and was stopped;
the focused serial `/m:1 /nr:false` build passed. Ordinary Android Debug compilation
for its configured ABIs and lint pass; focused lifecycle/input/camera/icon tests
pass. The full Android unit lane completes in 34 seconds: 415 discovered, 409 pass,
six existing host-symbolic-link assumptions skip on Windows, zero failures/errors.
The skipped case names are retained; no link privilege or test was changed.

No phone performance, listening, ownership-loss cause or visual correction is
claimed. PR conflicts, the unchanged Debug route deadline, MSVC job cap, natural
Keeper/journey/recovery, physical audio/controller/touch feel and warmed-device
performance remain open. Easy Tree/shared-instance forest work remains separately
owner-deferred; this task does not alter its status or the 1.7/1.8 boundary.
