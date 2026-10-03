# Horde prototype loop/tail overlap check

This is an external offline composition of the retained prototype WAVs, not a
native playback test, listening/owner acceptance, runtime-asset admission, or
licensing approval. Source files remain unchanged.

`compose.py` validates each source WAV against the repository's
`SHA256SUMS.json`, then converts the original stereo 48 kHz PCM16 samples to
stereo IEEE-float32 WAV. For A/B/D/E/F/H it places 20 unchanged 576,000-frame
(12 s) bodies at exact 576,000-frame intervals and adds the previous unchanged
144,000-frame (3 s) tail to the next body's first 144,000 frames. The twentieth
body's tail is appended. No fades, gain changes, normalization, resampling,
or score/FX-rate changes are applied. C and G are each emitted once as one full
body followed by one full tail. G's source receipt still records first F#4
(MIDI 66) at 4.50 s.

## Measurements

- Each loop output is exactly 11,664,000 frames / 243 s. Its 20 body starts
  remain at 0, 12, ..., 228 s, so period drift is exactly zero by construction.
- Each loop has 19 tail/body overlap windows. The 12 s body-wrap adjacent
  sample deltas for naive repeat are 0.000580–0.001404 full scale (FS); at the
  first overlap start they are 0.000031–0.000092 FS. This avoids cutting the
  previous tail at each 12 s body boundary.
- It does **not** establish a seamless musical loop: tail-off at 15 s remains
  a boundary. Its measured adjacent-sample delta is 0–0.001495 FS (A 0.001495,
  B 0, D 0.000702, E 0.000671, F 0.001495, H 0.000916). The source tail's
  final sample is exactly zero for all six cues, so the final appended tail
  reaches silence at zero amplitude; listening/phase compatibility remains
  untested.
- The greatest positive single-channel sample-peak rise against the identical
  body sample inside an overlap is 0.002747 FS (E); tail peaks range from
  0.000702 to 0.002960 FS. Overall output peak equals the corresponding full
  body peak for every loop, no channel samples exceed 1.0 FS, and all six
  outputs have zero overrange samples. Thus this run found no clipping risk at
  unity gain; it does not substitute for native mixer/headroom validation.
- C is 6 s and G is 9 s (body plus tail, no repetition). The G source-receipt
  timing check passed.

The exact per-source and per-output SHA-256 values, frame timelines, peak
windows, and all measured boundary deltas are retained in
`loop-tail-composition-metrics.json`. The eight output WAV hashes are recorded
there; loop outputs were 243 s each. Output files were independently checked
for RIFF/IEEE-float32 shape, sample count, duration and recorded hash, then
removed to avoid retaining hundreds of megabytes of analysis audio. The hashes
therefore identify the verified transient output, not a currently retained
runtime asset.

## Artifacts

- `compose.py` — small standard-library-only offline compositor/measurement.
- `loop-tail-composition-metrics.json` — exact source/output hashes and metrics.
- The eight float output WAVs were transiently generated for independent
  boundary/header/hash checks and are not retained.
