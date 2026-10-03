# Pocket Audio Core native PCM utilities

This directory contains a small backend-neutral C++20 native target for
caller-owned stereo PCM16 clips. It is separate from the JavaScript package
entry points and does not provide an operating-system playback backend,
decoder framework, synthesis, resampling, or file I/O.

The package remains private and `UNLICENSED`; see the monorepo `LICENSES.md`.
This work does not change package licensing or authorize redistribution.

## API

- `include/pocket_audio/PcmLoopStream.h` provides `PcmClip`,
  `PcmSelection`, `PcmStatus`, and `PcmLoopStream`.
- `include/pocket_audio/PcmWave.h` provides `PcmWaveStatus` and
  `DecodePcmWave`.
- CMake target: `pocket_audio_native_pcm`; focused test:
  `pocket_audio_native_pcm_tests`.

The stream uses fixed storage for at most 32 clip slots; slot zero is empty
silence. Clip spans are non-owning, immutable interleaved stereo PCM16 at
48 kHz and must outlive stream use. Body and optional tail frame counts are
derived from their spans, bounded to 576,000 body frames, and a tail may not
exceed its body. Per-clip looping is explicit metadata and must agree with a
selection. `Render` writes silence on invalid output/clock or suspension,
performs no allocation, locking, file I/O, or OS calls, and is single-owner-
thread only. `SetSelection`, `SetVolumePercent`, `Reset`, and `Render` must not
be called concurrently.

Looping emits the body on the first pass and overlays the tail's opening
frames on each later loop start. One-shots emit body then tail once. A bounded
linear crossfade applies to ordinary non-silence clip changes; rapid changes
replace the one outgoing stream. `naturalTailHandoff` is an explicit adapter
hint and preserves only the unplayed tail when the outgoing non-looping body
has actually ended. It does not infer game-specific cue relationships. A
stale or exhausted tail is not replayed. The configurable crossfade is limited
to one second (48,000 frames), with a 12,000-frame default.

The WAV decoder accepts only strict RIFF/WAVE PCM tag 1, stereo, 48 kHz,
16-bit input, with an exact caller-supplied frame count from 1 to 576,000. It
checks declared RIFF/chunk bounds and padding, permits a bounded 64 KiB of
container metadata, handles chunk order and harmless `fmt ` extensions, and
reads signed samples bytewise as little-endian. Output is cleared on failure
and replaced transactionally on success. Decode files off the audio thread.

## Build and test

From this directory:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure

cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --config Release
ctest --test-dir build-release -C Release --output-on-failure
```

Tests use small synthetic clips and generated in-memory RIFF data. They do not
require Horde assets or couple the Core target to any Horde cue, simulation,
gameplay, or platform type.

## Provenance and scope

The PCM stream behavior is a generic port of the validated Horde PCM stream
implementation at source commit `4870532638e3ca4b6fd89735bdf292c7bba16713`.
The strict stereo PCM16 RIFF decoder was ported from the associated uncommitted
Horde decoder source recorded in
`C:\Dev\tmp\horde-music-wave-20261001\receipt.md` (receipt run 2026-10-01).
The exact original Horde decoder source hashes in that receipt are:

- `src/audio/MusicPcmWave.cpp` —
  `c4f5a85d411aa3bd244e1243ecc8b8604afb53628ff3e11161b351accbf8e0f0`
- `src/audio/MusicPcmWave.h` —
  `5fd5a836c115c2813d44cb25da841a78695d03fd5c6748778cbb8bd1e747f528`

The port removes Horde cue and simulation dependencies: clip lengths and loop
metadata are supplied by the caller, and game-specific natural-tail intent is
an explicit selection flag. These sources remain an internal WIP under the
Core package's existing private/UNLICENSED boundary pending the package
owner's distribution review; no license grant is inferred here.

This target does not itself admit prototype WAVs as final music assets or
establish native playback, device behavior, or listening acceptance.
