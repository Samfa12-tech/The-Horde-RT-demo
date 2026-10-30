# Corrected APK graphics follow-up — 20 October 2026

This closes specific noncombat gaps on the corrected installed APK **c5a11b…**,
SM-S948B / Android 17 API 37. The prior receipt's historical artifact scopes remain
unchanged. [Exact artifact and file hashes](receipt.json).

| Check | Actual result |
| --- | --- |
| Home during Apply | Staged 100%, then one Apply immediately followed by Home. Saved remained 40% / Mist On; pending candidate 100 was retained. Serial 2/generation 1 has no save ACK. Hot resume returned the same task; Graphics recovered saved 40% and cleared pending in generation 3. [Interrupted](06-home-interrupted-custody.json), [recovered](08-recovered-custody.json), [current UI](09-current-recovery.xml). |
| Preview compare / Apply | Live Mist Off reached requested/effective Off while Saved stayed On, with pending=true. One Apply then saved the complete unchanged owner tuple except Mist Off, serial 6/generation 3/state 2, with real RayTracingPipeline presentation. [Compare custody](18-mist-off-preview-custody.json), [saved UI](19-mist-off-applied.xml), [original screenshot](19-mist-off-applied.png), [log](29-final-logcat.txt). |
| Preview controls / rotation | Actual portrait → landscape lock 1 → portrait lock 0. Large font 1.7 controls reflow; fixed Apply/Restore/Back remain reachable. Horizontal options and vertical tools scrolls work. Details opens/closes; Image opens, Restore ACKs saved Off, Controls returns, Back returns to Graphics. [Landscape](14-preview-landscape-stable.xml), [Details](21-preview-details.xml), [Image](23-preview-image-current.xml), [portrait Controls](26-controls-portrait.xml). |
| Ordinary draft after preview | Back preserves the pre-preview unsaved Mist On draft and correctly labels it Not saved, while Saved/effective remain Off. Restore explicitly discards that draft and requested/Saved/effective all become Off. [Before Restore](27-preview-back.xml), [after](28-ordinary-restored.xml). |
| Controller | No external controller/joystick was attached. Actual input inventory was read; no synthetic gamepad acceptance is claimed. This physical gate remains open. |

The direct request from Sam in this Horde thread was:

> mist off on mobile because when it is on, during the keeper fight, we also have those two lights on and all together it was pretty chuggy.

The owner explicitly requested **Mist Off** during this follow-up. It therefore
remains saved Off, superseding the earlier request to restore the original On.
Resolution stays 40%, water/fire Mobile, cap 30, Glass Off, Current shadows and
Dust Low. Only `graphics_mist_enabled` and retained `graphics_pending_mist` change
true → false. Final pending=false. All other settings values are identical;
all 12 campaign records/locks are byte-exact and the production save directory
remains empty. Original rotation lock 0, accelerometer_rotation 0, user_rotation 0,
font 1.7 and density 560 are restored. Only the Debug process launched for these
checks was stopped after Home. [Final custody](30-final-custody.json),
[device state](final-device-state.json).

Limits and imperfect observations are retained: three read-only UI dumps could
not reach idle (08 recovery, 16 and 17 popup); their screenshots were inspected
and mutation taps were not replayed blindly. The first new swipe attempt failed
to connect to ADB before reaching the device; it subsequently ran with the normal
authorized device access. An Image tap immediately after Close Details did not
open Image; the current visible button was inspected and tapped once afterwards.
No approval denial was bypassed. Raw observations stay in ignored
`reports/graphics-followup-2026-10-20/`.

Landscape telemetry reports a portrait-shaped native output while the Android
controls and image reorient; this receipt establishes the observed UI reflow,
not full renderer orientation/aspect acceptance. No alternate font/density,
resource/storage fault, combat, Keeper preparation/arm/timing or sustained
performance was exercised. Preview FPS is not a Keeper result. Audio/haptic manual
revalidation required: NO for these graphics-only checks. No install, data clear,
owner save mutation, desktop/Release/Reef operation or public push occurred.
