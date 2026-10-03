# Exact signed1.6.1 S26 production-update smoke

October3,2026. Owner explicitly made S26 available for this bounded final smoke.
Raw model **SM-S948B**; source/package checkpoint0c7db23. No benchmark, listening,
email, new shader, geometry or gameplay implementation.

## Finite completed matrix

| Check | Result / limitation |
| --- | --- |
| Update compatibility | Observed installed production package1.6.0/code8; `adb install -r` final signed1.6.1/code9 succeeds without app-data clear or downgrade. Old APK hash was not re-pulled; do not infer its exact prior artifact from version alone. |
| Installed bytes | Pullback108,261,409 bytes, SHA256 `bc5c7ce3c755c16ec39e2c16fa9eae01c31983c7393e645f881c5bcc9e6a346c`, exactly the frozen APK. Production ID/non-debuggable and established certificate separately verified. |
| Preferences | Before/after UI retains SFX70%, look100%, render76%, RT water Mobile. New Music slider shows70%; there was no prior Music field to compare. No reset or savefile inspection.76% is a retained choice, not changed default or75% performance run. |
| Native scene | Initial generation1 requests11:21:52.044 and presents11:22:03.397 (11.353s). Strict ASTC,1094x2265→1440x2980, successful RT-produced swapchain presentation; live image has modelled hands/torch/sword and two skeletons. |
| Live interaction | Brief look/sword/parry inputs and active scene/HUD. Every individual input animation was not captured; not a full combat acceptance rerun. First unpaused inspection later reaches gameplay death; restart and the controlled pause/resume check remain in the record rather than claiming uninterrupted survival. |
| Home/resume | Same process7544. Replacement generations3/5/7 present12.730/17.317/13.370s after request. Controlled paused-resume returns to native RT active/vitality3. No instant-ready or sustained-FPS claim. |
| Automation limits | First pre-update dump had null root before menu ready; retry captured it. Diagnostics dump could not reach idle; no complete diagnostics interaction or full capability-JSON export claimed. Render logs/UI/image independently prove presentation. |
| Cleanup | Force-stopped only production Horde; no PID remains. No commands installed/cleared/downgraded the separate Debug app. Owner informed phone is released. |

## Retained evidence and next step

Only bounded version/flag extracts, relevant game UI XML, native lifecycle/ASTC
lines, and original scene PNG are archived. Raw private app data, unrelated logs,
APK copies, credentials and device identifiers are not committed. Receipt hashes
are in `SHA256SUMS.txt`. Full temporary run remains local at
`C:/Dev/tmp/horde-final-signed-s26-20261003/`.

The [release validation](../../SHOWCASE_ALPHA_1_6_1_RELEASE_VALIDATION_2026-10-03.md)
owns frozen hashes and host/shader scope; the Android compatibility record is
updated in the same task. No phone step remains queued for1.6.1. S24 remains
owner-deferred **working but not fully tested**, exact S25 unverified, performance
accepted as-is below30FPS target; High defects deferred, not fixed.
Release-preflight version-source tooling repair, focused fixture/CTest and actual
canonical preflight pass. Verify current checkpoint CI on PR15; merge/publication still
require separate authority.
Audio/haptic manual revalidation required:NO — no changed playback bytes/events.
