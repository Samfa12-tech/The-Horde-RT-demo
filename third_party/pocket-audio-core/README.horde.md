# Pocket Audio Core native subset

Canonical implementation: `packages/pocket-audio-core/native/` in
[Pocket-Chordsmith](https://github.com/Samfa12-tech/Pocket-Chordsmith/tree/b77a9d0bfe3051c6e352815295164b9b80586f56/packages/pocket-audio-core/native).
Horde pins commit `b77a9d0bfe3051c6e352815295164b9b80586f56` without local
modifications. `manifest.json` records exact upstream paths, sizes and SHA-256s;
Windows and Android CMake validate this admission before building the target.
Updates are deliberate: review/test the canonical Core change, replace only the
listed subset with those exact upstream bytes, update the manifest and CMake pin,
then run Core, Horde adapter, decoder and pin-admission tests. No network fetch
occurs during a game build. Do not implement reusable PCM changes in this copy.

Only the eight native utility/build/test/documentation files are included, plus
unchanged upstream licence metadata. No Chordsmith editor, JavaScript runtime,
synth recipes, application scheduler or other app is vendored. `MusicDirector`
and the thin Horde adapter retain gameplay cue decisions and the A-H asset/handoff
contract; sample cursors, looping/tails, bounded mixing and WAV decoding belong
to Core. OS playback will feed this same target, not introduce another mixer.

Core remains private and UNLICENSED. `upstream/LICENSES.md` and the scope notice
in `upstream/LICENSE` are preserved verbatim; the root MIT licence does not apply
to Core. The owner explicitly directed this shared-Core integration. That is
not a general third-party reuse grant, package licence change, or production
publication authority. The owner's Horde-only score grant is a separate asset
permission and does not relicense Core or any other software.

Canonical PCS JSON/PCS1 remains the revisable musical source. The unchanged
rendered A-H body/tail evidence is a derivative, not a new composition source.
This subset does not establish platform playback, native-clock/drift/seam
acceptance, audible SFX clarity or owner listening acceptance.

2026-10-23: the reviewed canonical update adds explicit bounded long-PCM opt-in
shared by decoder and mixer, retaining legacy 12-second defaults and playback
semantics. Horde may opt into only its admitted full outdoor main, with exact
per-clip and aggregate PCM budgets. No synth, streaming decoder or Core fork is
introduced. Canonical Debug/Release proof is separate from Horde integration,
platform playback, listening and phone acceptance.
