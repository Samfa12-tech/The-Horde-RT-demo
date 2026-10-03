# Native Core pin and thin Horde adapters — no playback acceptance

Canonical Pocket Audio Core commit534a6e6811ce653efd5422138c5772b967263ed0 is
reviewed/pushed, with [draft PR75](https://github.com/Samfa12-tech/Pocket-Chordsmith/pull/75).
Its fresh [six compiler/configuration CI jobs](https://github.com/Samfa12-tech/Pocket-Chordsmith/actions/runs/36781667753)
pass GCC/Clang/MSVC Debug+Release. Only native utility/build/test/docs code is
reused: no Chordsmith editor/synth app, score change or package relicensing.
Unchanged private/UNLICENSED metadata is retained; owner-directed integration
is distinct from the score's Horde-only permission.

MusicDirector is unchanged. MusicPcmStream maps the exact A-H bank and natural
C→D/G→H policy; MusicPcmWave delegates decoding. Core owns reusable cursors/loops/
tails/bounded fades and strict bytewise PCM16 RIFF parsing. Configure-time pin
admission rejects modified, missing, extra or repinned files; seven checks pass.
No build-time network fetch. Change reusable behaviour upstream, review/test and
deliberately update the pin, not a Horde-only mixer fork.

## Actual validation

- Standalone Core: MSVC Debug1/1, Release1/1, ASan Debug1/1. Installed MSVC ASan
  runtime placed on PATH; earlier loader failure is not a pass.
- Horde integration: Debug8/8 in3.24s, Release8/8 in12.29s: director, stream,
  decoder (all16 actual WAV prototypes), Core, pin, simulation timing/gameplay
  and existing spatial feedback. Real Windows RT executables compile/link in
  both configurations; no new RT image/device run.
- Android assembleDebug: SUCCESS2m54s, four ABIs,41tasks. Two SDK XML v4/v3 tooling
  warnings, no C++ build failure. Universal APK
  eab79c4a5f6757b600824f85e08b290ce3fe57e735022647c41377c69589d21a;
  ARM64 library b09c010dc9d879a24d739bb175ee9b6a0599ad6ba7bd9ba15577811adc84cd42.
  All53assets and four actual Diagnostic/Mobile module hashes match C11;
  packaged/stripped modules independently val/dis PASS. No install.
- Actual CTest inventory:82 local registrations =50 common CPU tests,15 extra
  Windows tooling and17 Vulkan-specific. Current-source Horde CI follows push.

Retained failed checks: guessed nonexistent build target; PATH-only pin lookup;
rerun before CMake regeneration. Final configured-tool test passes. Exploratory
Shipping scanner correctly rejects Diagnostic Debug modules: the subsequent
Diagnostic check passes, not a Shipping build. Core's new-test output-span overrun
and early-tail port regression were fixed before its committed pin; that modal
does not demonstrate a game-loading bug. Original failures are retained locally.

[Source receipt](source-and-build-receipt.json) and [SHA manifest](manifest.json)
bind source/build/package scope. No APK/library/build cache/duplicate audio is
archived. The sixteen prototypes are unchanged in the earlier rendering evidence;
canonical PCS JSON/PCS1 remains the revisable score, not the WAV derivatives.

Remaining: canonical/runtime asset admission, platform playback/sample clock,
pause/focus/retry/event routing, separate persisted Music Volume,20-loop/drift and
listening acceptance. No phone, Shipping performance, pacing, backend parity or
final-candidate claim. Audio/haptic manual revalidation required:**NO** for this
unwired seam; **YES** when music becomes audible, preserving warning/SFX clarity
and existing haptics. Physical glass/phone performance remain open.
