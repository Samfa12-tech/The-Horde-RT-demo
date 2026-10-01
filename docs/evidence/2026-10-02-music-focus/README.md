# Android music focus: finite admission record

Implementation `ef812d060509dd7e9f959aaeaf38aee2682e9504`; CI addition
`3cf25b3c6b0a1217147bea401cec10ab32801046`. No soundtrack, Core mixer,
cue resolver, SFX/haptic events, shader, geometry or renderer change.

## Change and local gates

`MusicAudioFocus` owns one lifecycle-eligible OS request on the main thread.
The PCM worker checks both lifecycle and focus before rendering/playing; native
control polling continues with the suspension flag. Transient loss pauses without
flushing accepted PCM; gain resumes the same stream. Permanent loss abandons
until a new eligibility edge. Stale callbacks cannot revive an old request.
Modern automatic ducking remains platform-owned; API24/25 pauses on duck requests.
Failed requests are explicitly abandoned, not silently leaked or repeatedly stolen.

Debug and unsigned Release each pass **38/38 Java tests**, no failures/errors/skips,
including eight focus contracts. Both four-ABI builds and lint pass; each lint
report retains 38 warnings and zero errors. Initial Debug lint failed nine API26
checks. The fix scopes `@TargetApi(26)` to the isolated helper and explicitly guards
abandonment by SDK version; minimum SDK24 and legacy tests are preserved.

Both APKs pass the actual runtime/licence/package-header gate. Debug SHA256:
`2c93d30322b5ca7b176d7e3543b770582828cf7945d57d3d35be9bf44d94e04e`.
Unsigned Release SHA256 (not installed or published):
`362b4a3104681081774ff8558c35c7b626676f85b96ca24c18fc26d0735de2a5`.
Debug was built on5942e94 with the reviewed Java delta subsequently committed as
ef812d0. All70 assets and all four native libraries match exact A627 control bytes.
An initial package comparison accidentally selected zero libraries; it was not
accepted as native proof. The corrected comparison requires four and checks:

| ABI | Native library SHA256, identical to A627 |
| --- | --- |
| arm64-v8a | e8c05db83e6f591f028b6382b6651ba63d09c037ad02a95ba2c036aae259d785 |
| armeabi-v7a | 8dffa920659905290a5255eac114eae8baf891eb634b5bcfb2ab951471dc10df |
| x86 | c311749a6de3611fa349870edb017346607f84772ece6af1974eca8b05987fa3 |
| x86_64 | eac9c4a9ee0a3f4e49e9902da9149a7dd25b9e61b890392d34697f9210ae9602 |

No unchanged Windows/shader/Core gate is rebuilt merely for this Java change.

## Exact SM-S948B check, October2 local time

Debug installed/pulled byte-identically with app data/system volumes preserved.
Used existing `finale-roof`11 then ordinary Continue, not a frozen benchmark,
invulnerability or an idle live enemy. Fresh UI before/after resume shows native
RT active and vitality3. Both `am start -W` waits timed out at10 seconds before
later readiness; this is not fast-launch/resize evidence or a successful wait result.

| Local time | Actual result |
| --- | --- |
| 02:10:19 Continue | Android focus stack has one GAME/MUSIC GAIN request, loss none. |
| 02:10:29–02:11:41 | Epoch2 consumed periods1–7, zero underruns, capacity5766 frames. Consecutive intervals11.997–12.002s. |
| 02:11:50 Menu; inspect02:11:52 | No active Horde focus entry. |
| 02:12:55 paused inspection/Home | Period-row count remains7 after65s; no active focus entry on Home. |
| 02:14:52 ordinary Resume | One fresh GAME/MUSIC GAIN request, loss none. |
| 02:14:55–02:15:43 | Same epoch2, periods8–12, zero underruns, subsequent intervals12.000–12.001s. The pause-crossing interval is excluded. |
| 02:15:51 Menu/Home cleanup | No active Horde focus entry; worker remains normally suspended. |

Retained scoped music logs and fresh UI/focus excerpt support these rows; full
audio-service history/private device dumps are not added. No S24 operation was
performed for this music check. This is not sustained Shipping performance, a
new20-loop gate, all-cue/SFX perceptual acceptance or real external focus-loss proof.
Reuse the already-completed20-period cueH gate and accepted whistle listening.

## Hosted coverage and next unfinished step

The new Android CI job builds all Debug ABIs, runs Java tests/lint and validates
actual package contents/music pins without production signing credentials. Debug
uses AGP's debug key. Explicit SDK licenses/pinned packages and36 runtime LFS
payloads are used. It requires at least38 passing tests and complete focus/buffer
suites. Local actionlint1.7.12 passes (external shellcheck/pyflakes unavailable,
explicitly disabled); the exact post-build PowerShell step passes locally. Hosted
results are pending, not inferred from the existing C++ CI. First687dda9 push
36892419452 / PR36892429964 Android jobs fail before compilation: `sdkmanager`
is not on the runner PATH (exit127). The correction resolves the existing tool
explicitly under ANDROID_HOME and requires it to be executable; no SDK/version
upgrade or old-job rerun. Raw failed job logs are retained with the local evidence.

Next: one owner external-audio interruption/return check on installed S26 candidate,
then full route/transitions/combat clarity when convenient. Do not resend the same
check or repeat admitted renders/long loops after resumption. S24 visibility/Shipping,
High glass/backend and final-candidate gates remain separate.
Audio/haptic manual revalidation required: **YES**, specifically music interruption
and recovery; no new timbre audition or haptic/SFX-event reapproval.

Raw builds, failed lint, unit reports and immutable APKs remain in
`C:/Dev/tmp/horde-music-focus-20261002`; GitHub is not a backup of compiled artifacts.
No main merge, release, signing recovery or publication is authorized.
