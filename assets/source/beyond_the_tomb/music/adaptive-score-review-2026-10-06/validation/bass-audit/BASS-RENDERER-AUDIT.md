# Bass renderer diagnostic

The feedback is consistent with a genuine renderer mismatch. The four tested excerpts preserve the intended bass notes; the main discrepancy is the timbre/envelope/mix path. No score, octave, or previously delivered audio was changed.

## Actual rendered evidence

The diagnostic uses the same canonical scores and exact Core event objects for original title A, Tomb B, Fourth Keeper B, and Bellwether B. Only events whose stem is `bass` are passed to the real `renderPocketAudioEventBuffer`. The legacy sine-only stem helper is not used. Each clip keeps its existing bank gain, with no additional per-stem normalization.

- `01-what-the-dark-keeps-A.core-bass.wav` and `.mp3`: MIDI 38, 38, 46, 45; 4 events; existing gain +12.0018 dB
- `02-tomb-B.core-bass.wav` and `.mp3`: MIDI 38, 38, 46, 45; 4 events; existing gain +15.4286 dB
- `03-fourth-keeper-B.core-bass.wav` and `.mp3`: MIDI 38, 38, 38, 38, 46, 46, 45, 45; 8 events; existing gain +15.0207 dB
- `06-bellwether-B.core-bass.wav` and `.mp3`: MIDI 43, 43, 36, 36, 43, 43, 38, 38; 8 events; existing gain +15.4921 dB

`comparison-inputs.json` contains every exact event, note duration, score hash, original beat/master volume, fixed bank gain, and audio hash. `signal-measurements.json` contains the signal analysis. The stems have no clipping or non-finite samples. Their peaks are approximately -18.2 to -14.6 dBFS at the same bank gains as the supplied full mixes.

## What differs in the source

1. **Missing bass filters.** The Classic bass recipe specifies a 420 Hz main low-pass and a 220 Hz sub low-pass. The current app's `playBass` passes those filters to `playTone`, which creates actual Web Audio biquad filters. Core's `bassVoice` selects the waves/relative levels but ignores both cutoff fields. Its `bassSample` sums an unfiltered mathematical sawtooth and octave-lower sine.

2. **Different oscillator quality.** Core's sawtooth is the direct expression `phase * 2 - 1`, with hard discontinuities and no bandlimiting. This supports a bright/buzzy timbre and aliasing risk. The app uses a Web Audio oscillator followed by the specified low-pass filters. No actual browser render has yet been measured in this environment, so the audible magnitude of that improvement is still unverified.

3. **Different envelope.** Core gives the bass a generic attack occupying 8% of the authored duration, followed by a continuous power-law decay to zero at the event end. For these first notes the attack is 14.4, 94.0, 58.75, and 67.14 ms. The app uses a fixed 10 ms attack, 60 ms decay to 70% sustain, holds that level until note duration, then adds a 200 ms exponential release. Core's envelope additionally jumps from approximately 1.0 to `0.92^1.8` at the attack/decay boundary: a 13.9% envelope discontinuity. It has no separate release beyond the event duration.

4. **The bass mixer level is not honored by Core.** Compact bass events use fixed velocity 0.34/0.42. Core's final stem scalar uses the default bass volume 0.86 rather than the normalized score's beat volume. A direct 100-fold beatVolume change from 0.01 to 1.0 gives byte-identical bass PCM in all four tests. The actual app applies its beat bus and master controls. This is a mix-control compatibility bug, independent of musical notes.

5. **The octave-lower component is intentional.** Both paths explicitly add a sub oscillator at MIDI minus 12. Measured first-note dominant peaks are about 73.4 Hz for D2 and 98.0 Hz for G2, matching the event MIDI. The separate editor melody-octave normalization issue does not establish a bass-octave error and is not changed here.

## Signal measurements and limits

- The first D2 notes have approximately 1.34% of Hann-windowed spectral energy above 3 kHz. Bellwether's first G2 note has about 1.78%. These are high-frequency energy measurements, not pure aliasing measurements.
- The same exact first event was also rendered by Core at 176.4 kHz and downsampled with ffmpeg to 44.1 kHz. The interior difference from direct 44.1 kHz Core rendering is about -28 to -24 dB RMS relative to the direct signal. This shows sample-rate dependence consistent with the unbandlimited waveform, but includes resampling and sample-grid/envelope differences. It must not be quoted as an isolated alias-distortion percentage.
- Adjacent-sample deltas and the envelope-junction sample are recorded separately in `signal-measurements.json`. Large periodic sawtooth deltas are part of this oscillator's shape; they must not all be labelled note-on clicks.
- WAV is the authoritative diagnostic. The MP3 copies are audition conveniences.

## Faithful candidate to test next

Render these unchanged events through the actual current app's `playBassPhrase` → `playBass` → `playTone` → `adsr` functions, with the app's specified low-pass filters, existing score beat/master controls, and documented bank gain. Also keep a graph-controlled comparison that records any compressor gain reduction. Do not change pitches, durations, bass tone, or add a new synth. If that comparison confirms the improvement, use the real app render path for reference delivery or fix the shared Core renderer to honor the same recipe/filter/envelope/mixer behavior, with regression tests.

The appropriate fix is not established merely by choosing a different instrument preset, arbitrarily transposing the bass, or normalizing each stem independently.

## Current blocker

The audit worker verified that cloud Chromium cannot create the sockets needed to start, including an escalated attempt, and that the supported cloud browser rejects localhost with `ERR_BLOCKED_BY_CLIENT`. No blocked browser attempt was repeated or bypassed. Consequently this folder contains real Core diagnostic audio and source-backed app analysis, **not a completed audible Core-versus-app A/B**. App rendering remains an explicit unverified step.

## Exact sources

All source references are pinned to Pocket Chordsmith commit `0c6975cadcd4aba0047ab94170a2adabc722517f`:

- `packages/pocket-audio-core/src/engine/offline-renderer.js`: `bassVoice`, `bassSample`, `oscSample`, `envelope`, `renderEventToChannels`
- `packages/pocket-audio-core/src/events/timeline-events.js`: `addBassEvents`
- `packages/pocket-audio-core/src/performance/stem-mix.js` and `src/constants.js`: default offline stem gain/headroom
- `packages/pocket-audio-core/src/sounds/lofi-registry.js`: `CLASSIC_BASS_TONE_CONFIG`
- `apps/chordsmith-web/src/audio/04-live-engine.js`: `playBass`, `playBassPhrase`, `playTone`, `adsr`, `applyVolumes`, and `ensureAudio`

Reproduce the completed tests with `node tools/audit-core-bass.mjs` and `python3 tools/measure-bass-audit.py` from the soundtrack directory. These scripts write only this new diagnostic folder.
