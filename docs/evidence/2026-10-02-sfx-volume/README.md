# Independent SFX volume and quieter stone footsteps

Owner-requested bounded audio follow-up over `8b4433e`. No score/render/Core,
gameplay event, listener/source, haptic, animation, shader or geometry changes.

## Implementation

- Windows Settings replaces the SFX toggle with a persisted0–100 slider,
  independent of music. Missing `audio.sfxVolume` migrates legacy enabled to100
  and disabled to0; an explicit new value wins. Other cue gains retain their
  baseline. The same SFX mastering voice controls centered/positional effects,
  active one-shots and waterfall ambience. Lazy initialization applies the value
  before playback. Failed XAudio2 playback logs a diagnostic, never an unscaled
  `PlaySoundA` bypass. Zero is mute.
- Both Windows audio sliders have0–100 controller steps; render resolution
  retains50–100. Native focus/visibility/layout/labels are wired. Source and
  helper checks do not establish real keyboard/controller or audible acceptance.
- Only player-footstep cue gain changes: Windows1.0→0.45 (about6.94dB down),
  Android0.62→0.45 (about2.78dB down). These are calculated gain changes, not
  acoustic measurements. Android's existing persisted independent SFX slider,
  default70, SoundPool spatial gains and event timing remain intact. Combat,
  enemy footsteps, waterfall cue gains, music and haptics are unchanged.

## Fresh evidence / finite matrix

Retained MSVC build: `build/foundation-validation/20261002-183151`.

| Check | Actual result |
| --- | --- |
| Debug and Shipping Windows game build | PASS; actual platform source compiled in both. |
| Debug selected CTests |7/7 PASS,2.42s after Android gain/source admission. |
| Release selected CTests |7/7 PASS,2.65s; [receipt](release-contracts.txt). |
| Selected contracts |SFX clamp/migration/gain, desktop controller, spatial feedback, MusicDirector, music session, simulation gameplay and timing. |
| Android assembleDebug / unsigned assembleRelease / Java / lint |PASS,21s;47 executed/64 up-to-date tasks. Both Java variants freshly compile; native4-ABI CMake tasks reuse unchanged native outputs. Java test task executes:76 total,0 failures/errors/skips. Lint analysis executes; unchanged report has42 warnings/0 errors. [Build](android-build.txt). |
| Debug and unsigned APK held-item/player package admission |Both PASS; exact current cuff/player entries and existing attribution admitted. No device installation. |
| Listening/UI/device acceptance |OPEN; tests are not an audible balance, active-voice slider or physical-device pass. |

SFX-stage Windows executables (before any later startup-observation-only changes):
Debug `e5f596ed081707115404c9cf1cb6f4738b933bd854e242af33636351c6cdc0a7`,
Shipping `3837fbcb91032c220fc1c3f9ad042efb56be902a09634a1a2705a423aa6f6dad`.
Both are development artifacts, not new release packages or RT live proof.

Current Android artifacts include the accepted paired cuff assets:
Debug APK `8f960fc871a44ac077d1509fba2f73d4f928a792e5566d2fc412e1e00f9f22d2`;
unsigned Shipping APK `d4f403f2d17cd75cfc59eecc9b2ded06592c6ebb8fcf38010e18d3cb02312003`.
No exact-device/owner pass follows from their build/package checks; older accepted
APK evidence remains tied to its older hash. No phone use in this run.

## Next unfinished step

On the prepared Windows candidate: verify independent SFX/music settings,
0/low/full SFX levels, persistence, and footstep/combat balance in normal play.
No soundtrack retuning or unrelated haptic acceptance requested. Phone gain/cuff
acceptance remains separately open when authorised device time becomes available.

Audio/haptic manual revalidation required: **YES**, only for SFX gain/control
behavior and the separately reported uncertain Windows startup/refocus music
observation. Haptic routing itself is unchanged. No publication or main merge.
