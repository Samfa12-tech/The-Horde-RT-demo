# Android Core-backed music output candidate

2026-10-01 authorised Android changes initially built over `6385288`, then rebuilt
over Windows checkpoint `84f111e` (both dirty working-source receipts retained).
This is build,
package and static review evidence only. **Not installed/tested on a device.**
No S26/S24/S25 playback, RT-presentation, pacing or owner listening claim.

Horde gameplay/render owner copies snapshot/events before the existing SFX drain;
there is no new JNI gameplay authority. One Java audio worker owns bounded
AudioTrack float48k stereo writes and the native immutable bank/Core session.
All PCM/loop/tail/crossfade/strict WAV behaviour remains in the pinned Core.
Actual accepted writes and unsigned device-playback-head frames are distinct
from generated content. Short/nonblocking writes retain their exact float offset;
they are never regenerated or counted as fully accepted. Device-head inconsistency,
event overflow, native/backend failure disables/logs music only.

Ordinary menu/background suspension pauses without flushing PCM; explicit
retry/import/queue-identity epochs pause+flush old output before new submissions.
Activity stop requests cleanup; only the worker releases AudioTrack then native
session/bank. A slow startup cannot make Activity free storage still in use.
Independent persisted `music_volume`0–100/default70 is a real Settings control,
not tied to SFX toggle/volume. Normal benchmarks include music; deterministic
frozen captures suspend output. Actual OEM buffer capacity is logged and must be
validated, not presumed to equal the requested1440-frame/30ms capacity.

## Fresh checks

- Initial `assembleDebug lintDebug`: SUCCESS,1m17s,53 tasks; all four ABI native builds.
- Final build after bounded Debug device-clock progress logging: SUCCESS,4s,
  53 tasks (21 executed/32 up-to-date); native libraries unchanged. Debug logs
  once per12s consumed period expose generated/accepted/last-observed device head,
  actual capacity and underruns, not a new timing authority or audible-seam proof.
- Final lint:0 errors,38 warnings; report task up-to-date, analysis tasks executed.
  Initial and final report bytes are equal. No zero-warning claim.
- Exact seventeen manifest/WAV entries admitted, source excluded; current
  attribution matches package; all52 older render/SFX assets equal immutableC11.
- All four ABI packaged libraries match stripped libraries and export the four
  new music JNI methods. Actual ARM64 Diagnostic/Mobile pipeline/compute modules
  freshly validate/disassemble, all four hashes unchanged fromC11.
- Ten relevant shared music/Core/simulation/spatial host tests pass both; see
  [Windows packet](../2026-10-01-windows-music/README.md). Independent static review
  found no further demonstrated ownership/order bug; not device acceptance.

Final exact APK:113,361,631 bytes, SHA-256
`509b7981109bf384fee2651bbd480c576ca8bd341504d1bef29ab4c5ccd1bae4`.
Retained at `C:/Dev/tmp/horde-platform-music-20261001/HordeLanternRT-music-debug-509b7981.apk`.
The earlier APK283b0159…7902d remains separately retained, not overwritten.
ARM64 library:3,729,248 bytes, SHA-256
`79202ea2770589f44c25c5f2207a771974fc2d3a1e42e420764820a6538e5519`.
Both original and final containment receipts name their actual dirty build state;
neither is silently relabelled as a pristine final-commit artifact. The final
receipt freshly repeats music/old-asset/library/JNI/actual-module checks. No score, Core pin,
modelled player, material, transport budget, RT shader or licence change.

## When the authorised phone returns

Use the retained exact APK (or prepare a new explicitly hashed candidate if
source changes), verify raw modelSM-S948B and installed APK pullback before tests.
Preserve existing settings/app data. At75%/Mobile validate:

1. Enter gameplay: A; independent Music Volume0/70/100, unchanged SFX toggle/gain,
   persistence through restart. Restore normal70 after testing.
2. Native clock and audible seams: remain at quiet opening for at least245s
   (twenty12s periods). No clicks/drift/gaps; collect `HordeLanternMusic` logs and
   actual accepted/consumed counts, OEM capacity and underrun evidence. A Windows
   silent-clock pass does not close this gate.
3. Actual route: engagement B; actual torch failure C→D (not premature guttering),
   lich E, reward F, skylight G→H, interruptions/crossfades and warning/SFX clarity.
4. Pause/menu/Settings, Home/resume, death/retry/restart, checkpoint import and
   surface recreation: no background audio or old-session replay, secure storage
   lifetime. Confirm G resume from durable finale timing.
5. Measure current music-on candidate separately in matched Shipping ordinary/
   held/live workloads after correctness; no reuse of old no-music timings as an A/B.

Android audio-focus acquisition/loss/return still needs a focused integration
follow-up; lifecycle suspension alone does not certify interruptions by other apps.
Automated native-clock/package checks do not replace listening. Audio/haptic manual
revalidation required:YES for playback/gain/mix; haptic cues/routing are unchanged.
Glass correctness/parity,30FPS target, S24/S25, reporting and final matrix remain
open; publication/signing/Hotstrike decisions remain owner-controlled.
