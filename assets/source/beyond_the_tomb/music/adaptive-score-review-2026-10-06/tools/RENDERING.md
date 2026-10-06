# Pocket Chordsmith reference rendering

This harness calls unmodified Pocket Audio Core source from Samfa12-tech/Pocket-Chordsmith commit `0c6975cadcd4aba0047ab94170a2adabc722517f`. It contains no replacement instrument recipes, scheduler, or synthesizer. Engine source remains separate from this composition deliverable and is subject to its private/UNLICENSED status.

## Run

Requires Node 24 or a compatible ES-module runtime. No npm install or third-party Node dependencies. MP3 additionally uses the environment's installed ffmpeg/libmp3lame.

```sh
node tools/render-core-reference.mjs scores/02-tomb.raw.json \
  --section A --loop --out previews/tomb --name tomb-A-core-reference --mp3

node tools/render-core-reference.mjs scores/02-tomb.raw.json \
  --section-ids B,C,B,D --loop --out previews/tomb --name tomb-exploration

node tools/render-core-reference.mjs scores/02-tomb.raw.json \
  --sequence --out previews/tomb --name tomb-sequence-audition --mp3 --full-metrics

node tools/test-render-core-reference.mjs

# Two concurrent render processes; pilots first, then remaining families.
node tools/render-all-banks.mjs
node tools/render-all-banks.mjs --events-only
python3 tools/verify-render-delivery.py
```

These examples assume the working directory is the soundtrack root. Set `POCKET_CHORDSMITH_ROOT` to a separately authorized Pocket-Chordsmith checkout pinned to the commit above. The single-render CLI also accepts `--core-root /path/to/Pocket-Chordsmith`. No engine checkout or credentials are included.

Library API: `renderSelection(raw, options)` returns raw/normalized projects, timeline, original buffer, processed buffer, metrics, and the loaded Core exports. `writeRender(result,{outDir,name,mp3,bitDepth})` writes 24-bit stereo WAV by default, event trace, and metrics/provenance JSON. Optional `gainDb` is a disclosed scalar gain, not mastering. Neither function mutates the canonical score. To render stems, use the actual renderer's `renderPocketAudioStemBuffers` or filter the exact timeline events through `renderPocketAudioEventBuffer`; do not use the legacy sine-only `export/stems.js` helper.

The delivery batch uses 16-bit PCM at 44.1 kHz stereo, with lossless FLAC companions. It renders the manifest's `loop` and `bridge` entries as periodic clips and `one_shot` entries with at least 0.6 seconds of tail allocation. Each bank receives one common scalar gain, chosen so its loudest section peaks at -3 dBFS; all quieter states retain their relative levels. A full-review clipping guard can only reduce that common gain, with any adjustment recorded. The full review sequences use the identical bank gain. No per-section normalization or limiter is used. These are listenable reference copies, not a loudness-mastered final soundtrack.

Every FLAC is decoded by ffmpeg and its PCM SHA256 compared to its source WAV. Per-bank manifests capture this evidence, exact frames, source/asset hashes and the common gain. The aggregate verifier checks 136 sections across 15 family banks and 2 optional event banks, verifies the corresponding source scores stayed unchanged, and verifies 17 full-review WAV/MP3 pairs. Engine source is not included in the delivery bundle.

## What is actually verified

The smoke fixture produces byte-identical PCM16 WAV to both `renderPocketAudioWavBytes` and `renderChordsmithWavWorkerJob`, the current app's preferred WAV-export path. Deterministic repeat, chromatic MIDI, exact timeline/sample duration, clipping/non-finite metrics, loop-fold arithmetic, and MP3 encoding are exercised. Evidence is under `validation/renderer-smoke/`.

Current app source `apps/chordsmith-web/src/audio/08-offline-renderer.js` first tries that Core worker. If unavailable it falls back to a different Web Audio OfflineAudioContext implementation with its own filters, envelopes, gain staging, compressor, note spreading, and releases. The accepted historical Horde title render used retained v68 live voices and FX in an OfflineAudioContext. The Node/Core render is genuine current shared-engine reference output; this does not make it the accepted old render or prove identical timbre.

## Loop handling

- Preserve frame zero and intentional leading rests. Never trim onset to the first audible sample.
- Audition output keeps the whole musical timeline and at least 0.6 seconds of tail allocation, extended when an event actually ends later. Allocating tail time does not synthesize reverb.
- Loop output is rounded to the nearest sample at the exact musical bar boundary. It circularly adds any event tail beyond that boundary into the beginning. It adds no silent padding and applies no crossfade/fade. Duration rounding error is recorded and bounded by half a sample.
- A tail-wrapped loop represents periodic playback. Use the unwrapped audition render for a cold-start entrance if hearing a previous-cycle tail at time zero would be wrong.
- The metrics include each channel's end-to-start amplitude jump, adjacent slopes, 10 ms edge RMS, and a relative discontinuity warning. A numeric seam test cannot establish musical or perceptual loop quality. Audition at least two repeats and every intended adaptive join.
- MP3 is for listening only. Codec delay/padding makes it unsuitable as the sample-accurate loop master. WAV frame counts and explicit loop boundaries are authoritative.
- Continuous Lofi hiss/crackle is excluded from the loop-folding path: folding allocated texture-tail samples would double the ambient noise near the start. This harness rejects that case rather than silently producing a bad loop. The Horde banks use the Standard profile without continuous Lofi texture.

## Measured/inspected limitations

- Core reference output is not accepted v68 mastered-tone parity. The package README explicitly says backend-specific listening gates remain necessary.
- The current Core offline renderer ignores project master volume and master FX (including reverb/delay/chorus). The smoke test proves that changing masterVolume to zero and enabling those FX leaves rendered bytes identical. Composition stem levels affect the timeline; they are not a substitute for the missing master chain.
- Standard melodic/chord voices use simple recipe oscillators and shared decaying envelopes. The offline path does not apply those recipes' Web Audio filters/ADSR or generic pitch slides. It does not implement a bowed-string or flute sample library. Acoustic-seeming palette labels are composition intent and synthesis approximations.
- Chord play-mode metadata is present in events but the Core chord waveform starts its oscillator layers together; live/fallback spreading/arpeggiation must not be assumed from metadata alone.
- Expression/technique metadata and a capability report with `exact: true` do not prove each intended audible gesture is reproduced. Preserve source intent and report loss gaps.
- Zero event velocity is treated as a fallback velocity by current Core synthesis (`event.velocity || 0.5`). Use rests or genuinely disabled/muted compact parts for silence; do not depend on volume zero. Rich-track mixing needs separate verification because rich events own a stem and carry their own velocities.
- No loudness maximization, EQ, limiting, synthetic reverb, or waveform substitution is performed by this harness. Peak/RMS are raw reference measurements. Optional full Core LUFS/true-peak values remain explicitly estimated and uncalibrated.

## Source anchors

- Core renderer: `packages/pocket-audio-core/src/engine/offline-renderer.js`
- Current app worker: `packages/pocket-audio-core/src/export/chordsmith-wav-worker.js`
- Canonical event builder: `packages/pocket-audio-core/src/events/timeline-events.js`
- Normalizer: `packages/pocket-audio-core/src/schema/normalise-project.js`
- Current app export routing/fallback: `apps/chordsmith-web/src/audio/08-offline-renderer.js`
- Pitch contract: `packages/pocket-audio-core/src/music/pitches.js`; chromatic index 2 with track octave -1 is MIDI62/D4, while octave0 is MIDI74/D5.
- Authoring contract: `apps/chordsmith-web/skills/pocket-chordsmith-composer/SKILL.md`, `packages/pcs-format/README.md`

Do not call these reference audio previews production-mastered soundtrack assets or claim the original accepted title bank has been replaced.
