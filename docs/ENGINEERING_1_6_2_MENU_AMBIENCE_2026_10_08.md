# Lantern menu ambience checkpoint — 8 October 2026

**Current owner-selected follow-up:** the exact Debug tuning build `7e6662c6fe2906787fdd645bfbcf4474eb2e60bd`, APK `19a275c587d2065b8064fbd760048e816e062d7725b5547a77926bd5bcb998a4`, let the owner save Flame 0%, Room 7%, Chain 21% under SFX 70%. The owner rejects the flame character and requests its complete removal. The safe follow-up removes its WAV, asset admission, processing and both platform voice paths; Room 7%/Chain 21% become the ordinary relative levels. Room/chain waveform bytes are unchanged. The Debug tuner now has two sliders and retains the owner-saved values in its separate development preferences. Existing main settings remain unchanged.

The flame-free source passes five affected host checks (7.84 s), 240 Java tests/37 classes with no failures/errors/skips, lint, four-ABI Android/Windows builds and all 145 closed asset cases, including explicit rejection of the removed flame on both platforms. Source/package seal, exact install and its CI are being recorded separately. Tuning source 7e CI [push 37757772653](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37757772653) / [PR 37757778958](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37757778958) passes 12/12 aggregate jobs; earlier failures below are retained. [Exact owner-saved tuning receipt](evidence/2026-10-08-menu-ambience/tuning-receipt.json).

The remaining quiet room loop and occasional motion-derived chain keep the existing SFX/focus/native-Play-fade envelope. No replacement flame, haptic change or music handover is added. Windows audible and competing-focus checks remain open; the owner selection does not certify those platforms or final release. The following sections retain the original fa5 candidate, checks and partial feedback as historical evidence.

Owner-authorized initial menu-only addition. Runtime source `fa5f46a7cf01e2492593dc26ad1c50883ab18280`, tree `120b9007d2cc6aef4d1a3d43cbf16a8518de6639`. Test-only follow-up `45bd4d401e738390e4352bfc16786a5800aee1ac` changes asset fixtures, not runtime. PR18 remains draft; no release.

## Behavior and scope

Two quiet owned SFX loops accompany the real entry scene: filtered flame hiss and indoor air. A single soft chain cue follows an actual shared 60 Hz lantern turning point after at least eight seconds, with subsequent cues at least twelve seconds apart. Reduced motion suppresses creaks. Settings/More preserve the loop session; separate Graphics preview, loss of focus, background, mute and Play completion stop owned voices. Gains follow saved SFX volume and the actual native Play fade. No new preferences, gameplay/audio authority, mixer fork or seamless music handover is introduced.

Android uses its existing SoundPool with two protected loop voices and one bounded chain voice, under the existing music audio-focus owner. Native ambience packets carry surface/reset/tick/serial/fade and current Entry presentation identity; stale generations cannot resume sound. Windows uses the existing keyed XAudio loops/one-shot path and foreground gate. Both platform paths baseline cues after interruption and discard missed creaks rather than replaying a burst. Denied/permanent Android focus loss retains the existing no-polling reacquisition policy: an eligibility transition permits a fresh request, and transient GAIN can restore a fresh baseline.

## Assets and provenance

The local FilmCow collection was inspected first. It supplied no dedicated flame or natural room-tone file. `gas leak.wav` and `ambience - air conditioner.wav` are carefully filtered **listening candidates**, not claims of recorded fire or accepted natural ambience. Both produce four-second mono 48 kHz PCM loops with a corrected cyclic crossfade; the manifest records exact source hashes, windows, filters and levels. A synthetic ramp regression catches the initial loop-seam ordering error found before the sealed runtime.

The supplied Hammy01 `chain-287197` contributes a quiet one-second derivative; the other supplied chain-rustling original is not packaged. Selection used waveform RMS/peak measurements, not an asserted aural comparison. Originals stay outside the repository. Source licences, links and voluntary Hammy01 credit are recorded in `ASSET_LICENSES.md` and credits. The closed menu roster contains three WAVs plus its manifest, 864,132 PCM-container bytes total. Owner approval of character, loudness and loop seams remains pending.

| Runtime asset | SHA-256 |
| --- | --- |
| menu_flame.wav | `d4f6d11b9c12c4628daeb7624775da64a834a143e643acff0d6b35b8c375c0be` |
| menu_room.wav | `9aa9a9631f7dbaf724ecd46bfbbea17142af20497299a83db08055751c7cc361` |
| menu_chain.wav | `6c85d6c97a21e9de69100b9969dd8f844af927b229ee91bf585c42b4d0ae146d` |
| asset.manifest.json | `63563578dd68f4f244a3b8f82435fe1517dccb8be19306956c551b1ceb597eca` |

## Exact artifacts and checks

Debug APK SHA-256 `38f5abab55163be0e548a22927a51921dd8d31cd58825838bd6eaf0dff69bf8a`, 138,463,040 bytes. Windows Debug EXE SHA-256 `822d6a46bf80e651757416c1a75356410d2b3dd92dbad0641ca9f4f90d20fac7`, 11,569,664 bytes. Package roster: 95 assets; four added menu files, only ASSET_LICENSES changed among the prior 91 entries, 90 byte-identical prior assets. Android manifest remains byte-identical to ac468f51. These are development artifacts, not production-signed release packages.

- Five affected host tests pass in 9.75 seconds: actual Entry pendulum/hand-off, cue policy, Windows music focus and spatial audio.
- Android: 239 tests / 37 classes, zero failures/errors/skips; lint and four-ABI Debug assembly pass. New cases cover repeat navigation, fade, mute/focus/current-presentation gating, stale/reset ownership, missed/load-delayed cues and idempotent close.
- Windows build, exact asset/manifest processing verification, staged/APK admission, held-item/licence contract and 16 KiB zipalign pass.
- The original source CI has six successes and six failures across push `37753804182` / PR `37753811742`. Each ordinary host fails only the old closed-roster count: Linux 93/94 and MSVC 103/104. Local reproduction retains the failure. Follow-up updates explicit counts to 30 Windows / 31 Android and adds missing/corrupt/duplicate/foreign menu files for both platforms plus Gradle/package checks. All **149** asset admission cases pass locally. Corrected CI push `37755429268` / PR `37755436548` finishes **11/12 aggregate successes**: all six PR jobs pass, while the push Android job fails before compilation on SDK CMake 3.22.1 extraction (`Error on ZipFile unknown archive`). Its log is retained separately from the corrected roster. No code workaround hides this infrastructure failure; do not label either original aggregate green.

## Bounded phone observation

The exact installed APK is pulled back and hash-matched on allocated **SM-S948B / Android 16**, with zero changed preference entries. While the owner is AFK, ordinary owned controls exercise Entry → Settings → Back → More → Back, Home/return, Play and pause. No controller acceptance is inferred.

Pipeline uses the existing saved profile: native output 1440×2980, traced 720×1490; SFX 70%. The first two positive SoundPool loop handles appear 35 ms after current-generation Entry presentation, at gains 0.112 / 0.063. The seven chain submissions in this bounded interval have observed successive gaps around 14.77–14.82 seconds within each uninterrupted session. Settings/More create no additional loop starts. Home stops both loops and the owned chain handle; genuine background retires the surface. Return presents generation 3 and creates exactly two fresh loops, with no cue burst. This return still has a roughly 16-second Pipeline initialization; it is genuine background recovery, not the earlier fixed brief BB-51 reconnect path.

Play publishes the native fade (111 → 1000 permille); menu-owned handles stop 35 ms after the final fade record. Showcase successfully presents 255 ms after those stops. Host/Java regressions establish the gain envelope; these logs establish submission/ownership boundaries, **not audible loudness, exact acoustic fade or seamless music start**. The owned app is stopped and saved settings remain unchanged. The selected bounded logs have no fatal/VUID marker; this is not a full synchronization-validation pass.

[Sanitized exact receipt](evidence/2026-10-08-menu-ambience/receipts.json) and [selected events](evidence/2026-10-08-menu-ambience/events.log). Full private build/install/owned UI logs remain under task-4. Earlier ac468f51 BB-51 owner recovery and two-backend Home results retain their own package identity; they are not repeated or relabelled as this new build's acceptance.

## Remaining decisions and evidence

**Audio/haptic manual revalidation required: YES**, for the new audible menu assets, gains, motion-timed cues and focus/fade delivery. This slice changes no haptic behavior.

Keep this bounded implementation for listening review. Owner must accept or replace the filtered flame/room candidates and chain level/character, including reduced-motion behavior and Play fade. Windows audible foreground and competing Android audio-focus behavior remain physical checks; compilation and injected focus tests do not close them. Sustained phone 50/40/33 quality/cost, full equipment/secondary-view and controller matrix, final integrated candidate and Eric's independent audit remain open. Dust remains optional/default Off; shafts and seamless music handover remain deferred. No release authority follows.

## Owner listening and requested live tuning follow-up

On the exact fa5 phone build, the owner accepts Settings/More continuity and the Play fade. The room/noise bed is too loud and the flame/chain difficult to hear; overall balance/character is **not accepted**. The owner requests independent live Flame, Room and Chain sliders plus Save mix.

The bounded follow-up lowers the starting room gain from 9% to 2% (about13 dB) and adds a **Debug-only, Entry-only** native tuning page under Settings. It is absent from ordinary production/Shipping benchmark UI. Sliders cover0–100%,0 retires just that owned voice, and the existing SFX/focus/native-fade envelope still applies. Save mix writes exactly three integers to a separate development tuning preference file; existing Audio/Graphics/interface settings are untouched. Production relative levels remain a later explicit adoption of the owner-saved balance. Chain continues to follow occasional real lantern turns; no new random cue or gameplay timing authority is introduced.

The exact 7e follow-up was installed with zero main-preference changes; 240 Java tests/37 classes, lint, four-ABI/Windows builds and five affected host tests passed. Its APK and all 95 assets are identical to the sealed roster in the tuning receipt, with the assets byte-identical to fa5. The owner saved0/7/21 and rejected the flame, as recorded at the top. This closes the requested live-slider selection step, not Windows listening, competing-focus, sustained performance or final release gates.
