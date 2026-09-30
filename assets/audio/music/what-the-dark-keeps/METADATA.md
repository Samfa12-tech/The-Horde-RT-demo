# What the Dark Keeps - Horde music admission

Owner-supplied; authorised for Horde use only. Confirmed by the owner on
2026-09-30. No general asset redistribution licence or production-publication
authority is inferred. This score permission does not relicense Pocket
Chordsmith, Pocket Audio Core, Hotstrike or other software/assets.

`asset.manifest.json` binds the original owner ZIP, verbatim editable schema-16
JSON and equivalent PCS1 project, rendering source/tool and sixteen WAVs.
The JSON/PCS1 in `source/` remain the revisable musical source of truth and
are deliberately excluded from game packages. No editor/synth code is imported.

The runtime files are byte-identical copies of the corrected renders archived
in `docs/evidence/2026-10-01-music-render/audio/`. Only the historical `-loop`
filename becomes `-body`, since C and G are one-shots. No regeneration, remix,
normalisation, clipping, tail trimming, effect-rate or score change is made.
Stereo 48 kHz PCM16: A/B/D/E/F/H have 12s bodies, C3s and G6s, each with a
separate 3s tail. WAV bytes total20,160,704; immutable decoded PCM20,160,000.
Runtime bank admission does not prove audible seams, native sample-clock
behaviour, SFX intelligibility or owner listening acceptance.

Pocket Audio Core owns PCM looping/tails/crossfades/decoding. Horde owns cue
decisions and these measured cue paths/durations; platform playback remains
separate. Updating the score requires new derivatives, manifest/pin review and
affected tests/listening, not editing PCM to conceal an unsuitable cut.

Only the manifest and exact sixteen `runtime/*.wav` files enter Windows/Android
packages. The closed-roster packaging checks reject missing, corrupted,
unrecognised and source files. Publication remains unauthorised.
