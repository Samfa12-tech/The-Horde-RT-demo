# Bounded phone check — October30 AEDT

Source checkout04dc8346; installed phone source **f27982cd**, Debug code10 /
1.6.2-debug, APK SHA256
`53af984fe8ce7fe275e2ea523c9fe7e2ccc7c433997c6ff708635f1cafa8224e`.
The installed identity was read from the phone, not assumed from the checkout.
No APK was built or installed. Private evidence is retained under ignored
`reports/phone-acceptance-20261029-r1` (UTC date), including custody, screenshots,
UI trees, capability JSON, reload log and restoration hashes; owner preferences
and save archives are not committed.

SM-S948B, Adreno840, Vulkan1.4.295, driver2150932499. Normal cold launch presented
the hardware-RT menu. Play -> Continue -> existing Slot1 Safe Exit autosave ->
Resume presented the outdoor scene with three hearts. Touch look and a short
movement gesture responded. An ADB gamepad-Y event used the normal lantern action
and the raised lantern/Lower icon appeared. This is injected input, not physical
controller or simultaneous two-thumb acceptance.

The normal pause/Save checkpoint UI wrote the latest existing checkpoint to
previously empty manual Slot2, preserving Slot1 autosave. Its162-byte record was
retained privately. After stopping/restarting the app, normal UI loaded Slot2,
showed Loaded/Resume, and presented Safe Exit again with three hearts. This is a
real disk/process/UI round trip, not newly earning that checkpoint or completing
the dungeon/forest journey. No crash was observed in this bounded check.

The phone already had saved33% resolution: logs report475x980 internal to
1440x2971 output. No graphics/system choices were changed. Capability-report
timing fields span menu/loading/paused work and are **not a usable FPS or matched
lit/unlit benchmark**. No optimisation gain, light-cost attribution, complete
chapter or current-checkout runtime acceptance is claimed. The prepared Windows
package was not retried during the time-limited phone slot.

Full filesystem access independently allowed the formerly blocked focused
`GraphicsPreferencesTest` and `RenderScalePreferenceTest` to run on04dc8346:
**24 tests passed, zero failures/errors/skips; Java compilation succeeded**.
This closes the local cache/test blocker, not device acceptance of the new33%
default. The tested phone retains the older APK and its own saved33% choice.

Restoration finished20:21:08UTC October29 /07:21AEDT October30, ahead of the
20:33UTC deadline. The first cleanup guard detected both the new manual record
and its expected atomic-storage lock and stopped before deletion; only those two
test-owned paths were then removed. All three original campaign/preferences
file hashes and the exact roster matched the backup. No data clearing/uninstall
occurred. Rotation stayed automatic1/user_rotation0; no system settings were
changed. Test app stopped, owned UI XML removed, launcher foreground; no owned
monitor/transfer remains. **Phone released**; no further device calls in this block.

Remaining owner findings stay queued: tutorial persistence, special yellow/red
ring visibility (normal special attacks remain gated), input-aware Raise Lantern
guidance, Kit dialogue/endpoint pacing, and forest depth/fireflies. Neither AMD
published1.6.2 Play failure nor the separate RTX3090 benchmark report is fixed or
reproduced by this older Android-build check. No new systems work, merge,
deployment, report submission or publication occurred.
