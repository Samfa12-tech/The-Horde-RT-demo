# What the Dark Keeps - instrumentation auditions

Owner-supplied; authorised for Horde use only. **Non-runtime**, unpublished.
No shared presets, synth/editor code, renderer/player work or gameplay changed.
Current source JSON/PCS1, sixteen-WAV manifest/pin and20,160,000-byte PCM bank are
unchanged. See the [finite record](../../ENGINEERING_1_6_1_MUSIC_INSTRUMENTATION_2026-10-01.md).

Actual retained v68 app voices/live FX at2b87d7b1 /HTMLb266814f…b6e2cf, not Core
or built-in WAV export. Existing adapter689fa712…e05aa is reused with exact
configuration anchors and added import/trace evidence. New wrappers never copy
voice recipes or replace the scheduler. Compressed receipts retain raw source,
import/export and exact calls; SHA256SUMS binds their decompressed bytes too.
Original bank repeat differs at most1PCM16 LSB (A95/E117samples), tails byte-exact.

## Listen

- [Owner's preferred combo](previews/owner-combo.wav): felt_piano chords at the
  original0.28; melodies1/2 soft_pluck, melody3 cowboy_whistle.
- [Whistle-lead alternative](previews/whistle-lead.wav): cowboy_whistle motif,
  soft_pluck answers, mellow_sax held backing; felt_piano chords0.20.

Both49.5s: **0:00 old A;0:12.5 candidate A;0:25 old E;0:37.5 candidate E**.
Twelve-second bodies and0.5s gaps; not an audible loop test. Constant gain only,
matching original A−28.38/E−22.51LUFS within0.02LU. Owner combo preview gain
A+4.07/E+0.73dB, true peaks−3.57/−2.70dBTP; these gains are not in runtime PCM.
Whistle alternative A+5.51/E+0.86dB. The tested mellow_sax-lead candidate is also
retained as a separate source/body/tail/receipt; do not repeat its audition.

[Combat context](previews/owner-combo-combat.wav) uses **unmatched** owner-combo E
at normal70% music: existing lich_charge at3s /gain0.42, sword_swing_1 at7s /gain1.
Peak0.57222, no clipping. Offline near-field/no spatial attenuation, not live
game/device timing or masking acceptance. Lich warning/music RMS margin in its
fixed window is+2.29dB versus old+1.48; perceptual masking cannot be inferred from
RMS alone. Owner must check warning/impact clarity during actual gameplay.

## Checks and remaining gate

All four A/E pairs preserve source and actual scheduled pitches/onsets/holds/
repeated attacks, bars80BPM/cue mapping; candidate JSON/PCS1 and actual app
import/export agree. Sixteen WAVs independently pass48k stereo PCM16 header,
576000-body/144000-tail frame, hash and float/PCM quantisation checks; no NaNs or
full-scale samples. Production closed-roster asset policy still passes.

Twenty-period previous-tail-over-body arithmetic stays unclipped. Owner combo
peaks A0.41496/E0.63979; boundary deltas0.000641/0.000671 and tail-off deltas
0.000397/0.000855 are measured, not asserted inaudible. Actual-clock/seam listening
remains open. Review caught incomplete illustrative transition arithmetic in
the initial receipts: original receipts retained; named supplements include
outgoing loop wrap/previous-tail overlap at first/repeated-cycle phases. No
render or preview repeated.24owner-supplement/36initial-supplement250ms cases
stay unclipped, max peaks0.44076/0.44372. No actual Core-clock claim.

Chorus0, delay0.04 and mix0.22; reverb remains0.46. No notes, onsets, holds,
octaves, pan, tempo, sequence, cue timing or shared preset changed. Sparse A/E
bell-answer events become plucks, without deleting events; other sections are
not re-rendered. No realistic acoustic-voice claim based on preset names.

Owner's screenshot and read-only Pocket DAW0.6.49 MCP confirm the preferred
allocation/original gains; the unsaved DAW project and transport were untouched.
DAW audition is not certified v68/native-game sonic parity. **Next:** listen to
the two comparisons, choose the instrumentation and assess whether it remains
too synthetic. Only then derive the full bank from consistent canonical JSON/
PCS1, update real asset hashes/pins and validate loops/transitions/SFX/native
acceptance. No paid/new voice without a bounded demonstrated need/approval.

Audio/haptic manual revalidation required: **YES** for new music timbre/balance;
no SFX/haptic/event implementation changed. No device installation or release.
