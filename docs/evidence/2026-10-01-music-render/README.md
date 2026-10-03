# What the Dark Keeps: A-H rendering prototypes

Owner-supplied; authorised for Horde use only. These are archived **investigation
outputs**, not admitted runtime music, seamless-loop proof or release/publication.
No game package references this directory. The original score and dynamics are
unchanged; no generation, normalization or source-tool code vendoring occurred.

Tool: current local Pocket Chordsmith v68 at source2b87d7b, HTML SHA-256
`b266814fff749bd4d7be9d8725e4d7becc2d944122bc2a302602457fa9b6e2cf`.
Owner archive SHA-256
`e28e5936189919fed25f6208dd7a8b97172eb5f7d25f69732cc7339c45f386fa`;
score entry SHA-256
`bc092a0f7489e52ab1e7e55e42c813ae58517ea4a73e6808de8fbbc71595d4a6`.
The score grant does not relicense Chordsmith/Core source (UNLICENSED), Hotstrike
or any other asset. No upstream application code is included here.

## Method / actual checks

The local adapter uses the actual app's imported score, live synthesizer voices
and FX graph, in a fresh 48 kHz stereo OfflineAudioContext per cue. It manually
calls the existing scheduler, seeds Math.random and advances a virtual clock for
voice pruning. It bypasses Core/WAV export, UI timers and real-time audio output;
this is not a supported app export API or a platform playback test. The receipts
pin source/tool/adapter hashes before and after rendering and retain actual calls.

Forty milliseconds of scheduler leader is excluded, not mislabelled as music.
A/B/D/E/F/H bodies have576,000 frames/channel (12s), C144,000 (3s), G288,000 (6s).
Each separate tail has144,000 (3s); no tail is folded, faded or silently discarded.
`*-loop.wav` is the prototype filename, not proof that C/G loop or any cut is
seamless. Source starts/holds/repeated-pitch masks and scheduler traces agree;
G's first F-sharp4 is at4.50s. All rendered float metrics record finite samples
and no ceiling clipping. Independent PCM audit checks16 actual WAVs, output hashes,
RIFF/PCM1/stereo/48k/16-bit layout, byte/frame lengths, decoded peaks/RMS/seams and
no full-scale PCM samples. Quantisation agreement uses2/32767, not an image tolerance.
Cropped PCM cannot independently recover pre-encoding NaNs or excluded leader data.

The first all-cue receipt had an incorrect61-hex HTML hash literal. It remains
untouched externally and is explicitly rejected as valid provenance. Corrected
renders/checks are a new run, not a retroactive repair. Two schedule-equivalent
runs differ by at most one PCM16 LSB in a small number of samples; no bit-exact
rendering claim. The independent WAV verifier also retains its own initial bad
header-tag assertion before the corrected passing parser; no WAV was changed.

## Remaining acceptance

Nonzero body-cut seams and FX tails remain. Chorus0.09 maps to0.421Hz in the actual
tool, or5.052 cycles per12s; exact musical duration alone cannot prove FX phase or
audible seam continuity. This finding is **not** evidence that an audible click
exists, nor permission to change the score/effect rate. The earlier A-only continuous
20-cycle render is context, not all-cue/native looping, audio drift or listening.
Choose a bounded playback/loop-tail solution and validate20 real loops, cue
crossfades, pause/focus/retry/late ending, separate persisted music volume and SFX
clarity on both platforms. Owner listening remains required once playback is wired.
No new live synthesis framework or source-tool licence change is approved here.

Audio/haptic manual revalidation required: NO for unwired archival/source evidence;
YES when music playback/mix is integrated. Nothing here closes Phase4 glass or the
final candidate matrix. Full render trace is losslessly compressed; `SHA256SUMS.json`
binds exact files, LFS audio objects and the decompressed original receipt.
