# What the Dark Keeps - Horde music admission

Owner-supplied; authorised for Horde use only. Confirmed by the owner on
2026-09-30. No general asset redistribution licence or production-publication
authority is inferred. This score permission does not relicense Pocket
Chordsmith, Pocket Audio Core, Hotstrike or other software/assets.

`asset.manifest.json` binds the original owner ZIP, editable schema-16 derivative
JSON and equivalent PCS1 project, rendering source/tool and sixteen WAVs.
The JSON/PCS1 in `source/` remain the revisable musical source of truth and
are deliberately excluded from game packages. No editor/synth code is imported.

The owner-selected whistle palette uses the retained Pocket Chordsmith v68 app
voices/scheduler/live FX. October3 removes only the Melody3 held phrases starting
at step15 in A/D (A: A4, D: D4), including their continuation cells through63.
The separate short step0 notes remain. Only A/D bodies/tails are regenerated;
the other twelve WAVs are retained byte-exact. All remaining musical events,
voices, FX, tempo80, arrangement and cue timing are unchanged. No normalisation,
clipping, tail trimming, Core synthesis or effect-rate change is made.
See `docs/evidence/2026-10-03-music-drone/` for the exact delta/render proof.
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
