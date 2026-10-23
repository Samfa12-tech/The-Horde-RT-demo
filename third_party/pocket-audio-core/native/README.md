# Pocket Audio Core native PCM utilities

This directory contains a small backend-neutral C++20 native target for
caller-owned stereo PCM16 clips. It is separate from the JavaScript package
entry points and does not provide an operating-system playback backend,
decoder framework, synthesis, resampling, or file I/O.

The package remains private and `UNLICENSED`; see the monorepo `LICENSES.md`.
This work does not change package licensing or authorize redistribution.

## API

- `include/pocket_audio/PcmFormat.h` provides shared `PcmClipLimits` and format
  constants.
- `include/pocket_audio/PcmLoopStream.h` provides `PcmClip`, `PcmBankLimits`,
  `PcmSelection`, `PcmStatus`, and `PcmLoopStream`.
- `include/pocket_audio/PcmWave.h` provides `PcmWaveStatus` and
  `DecodePcmWave`.
- CMake target: `pocket_audio_native_pcm`; focused test:
  `pocket_audio_native_pcm_tests`.

The stream uses fixed storage for at most 32 clip slots; slot zero is empty
silence. Clip spans are non-owning, immutable interleaved stereo PCM16 at
48 kHz and must outlive stream use. Body and optional tail frame counts are
derived from their spans, default to a 576,000-frame limit per span, and a tail
may not exceed its body. Per-clip looping is explicit metadata and must agree with a
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
16-bit input, with an exact caller-supplied frame count from 1 to 576,000 by default. It
checks declared RIFF/chunk bounds and padding, permits a bounded 64 KiB of
container metadata, handles chunk order and harmless `fmt ` extensions, and
reads signed samples bytewise as little-endian. Output is cleared on failure
and replaced transactionally on success. Decode files off the audio thread.

## Explicit longer-clip admission and memory contract

The original decoder signature and stream constructor retain their 12-second
defaults. The old `kPcmWaveMaximumFrameCount` and `kPcmMaximumClipFrames`
constants still describe those defaults. Longer clips require the new explicit
overloads; pass the **same** `PcmClipLimits` to decoding and bank admission.
This is a source-level extension; rebuild all native consumers together.

`PcmClipLimits` applies independently to each body or tail WAV/span:

- `maximumFrames` and `maximumPcmBytes` are inclusive, positive limits; both
  must pass. PCM bytes are exactly `frames * 4` (two signed 16-bit channels).
- The opt-in hard ceiling is 5,760,000 frames / 23,040,000 PCM bytes (120 s).
  This bounds the supported full-buffer path; it is not a default allocation.
  Consumers should specify their exact smaller asset budgets.
- Zero, above-ceiling, or non-frame-aligned byte policies fail closed. Frame
  admission checks division before multiplication and checks `size_t` capacity,
  including the container allowance. Extreme integer inputs cannot wrap.
- `DecodePcmWave` returns `InvalidLimits` for invalid policy and
  `InvalidExpectedFrameCount` when an expected count exceeds either budget.
  All policy, container, format, and exact data-length checks precede allocation.
  Actual WAV data must still equal the caller's expected frame count exactly.
- Total WAV file bytes are bounded by the **expected** PCM size plus 65,536
  bytes of container overhead (including headers/padding), independent of any
  looser policy ceiling. There is no resampling, streaming, or alternate codec.

`PcmBankLimits` adds an inclusive aggregate `maximumPcmBytes` checked once at
stream construction. It counts every admitted body and tail, including aliases
in different slots; it does not deduplicate pointers. Accumulation subtracts
from the remaining budget before adding. Its default is 142,848,000 bytes,
preserving all 31 usable slots with 12-second bodies and equally long tails.
Its hard ceiling is 1,428,480,000 bytes (31 bodies plus 31 tails at the per-span
hard ceiling). An invalid or exceeded policy makes `IsValid()` false, selection
returns `InvalidClips`, and rendering returns `InvalidClips` with silence.
Slot count, tail/body relation, crossfade ceiling and selection rules are unchanged.

The bank borrows spans and allocates no PCM memory. These limits validate
logical asset sizes, not process RSS, allocator capacity, or concurrent loads.
The caller owns file reading, allocations, bank replacement, lifetime and
loading-peak budgets. Load/decode serially off the audio thread; reject an
oversized file before reading it into memory. Before each decode, budget the
live bank, input WAV bytes, new decoded PCM bytes, and any retained output
vector capacity not already counted in the live bank. `clear()` on failure
empties output but retains capacity; transactional success temporarily keeps
that capacity alongside the new buffer. Container overhead, vector capacity,
allocator overhead and device buffers are additional to the logical bank size.
Destroy/swap caller-owned banks only after their stream no longer borrows them.
No policy, loading, or size-dependent work was added to `Render`.

### Horde outdoor consumer checkpoint

For the approved main mix only, use an exact frame count, rather than rounding
106.6666667 seconds back into frames:

```cpp
constexpr std::uint32_t outdoorFrames = 5'120'000u;
constexpr pocket_audio::PcmClipLimits outdoorLimits{
    outdoorFrames, 20'480'000u};
std::vector<std::int16_t> outdoorPcm;
// Off the audio thread; check status before creating any spans.
const auto status = pocket_audio::DecodePcmWave(
    wavBytes, outdoorFrames, outdoorPcm, outdoorLimits);
// After successful decode; storage must remain immutable and alive.
const std::array<pocket_audio::PcmClip, 2> clips{{
    {}, {outdoorPcm, {}, true}}};
const pocket_audio::PcmBankLimits bankLimits{outdoorLimits, 20'480'000u};
pocket_audio::PcmLoopStream stream(clips, bankLimits);
// Check stream.IsValid(), then use the existing selection and gain controls.
```

The snippet assumes successful decode and an outdoor-only bank with no tail.
For a mixed bank, set its aggregate budget to the reviewed sum of all body and
tail bytes, and keep exact expected counts and per-asset admission policies in
the loader. Do not globally opt every decoder call into the largest duration.
If a tail is supplied, budget it separately and retain `tailFrames <= bodyFrames`.
One standard 44-byte-header WAV for this main is 20,480,044 bytes; PCM is
20.48 MB / 19.53125 MiB. Input plus decoded PCM during a fresh decode is
40,960,044 logical bytes, or at most 41,025,536 with the full container allowance,
before existing banks/capacity/allocator overhead. Free input bytes after decode
when no longer needed. Installer compression and other assets need a separate
consumer measurement; this target does not estimate installer size.

Ship only the approved main mix and use existing runtime gain ducking. Quiet
and main are different balances; this change does not establish mix equivalence.
Re-vendor the complete native target from the tested commit, including all
three headers and both `.cpp` files, and retain its CMake/test/docs provenance.
Rebuild the consumer, decode the approved WAV through this exact shared path,
verify expected frames/bytes and its bank/loading peak, then test actual loop,
seek, cue transition, ducking and listening behavior in Horde. This canonical
checkpoint contains no Horde code or asset changes and does not claim that
consumer playback/listening validation has happened.

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

Tests use synthetic clips and generated in-memory RIFF data, including one
5,120,000-frame fixture. They cover legacy defaults, explicit frame/byte/bank
limits, malformed/overflow inputs, a full approved period with irregular
callbacks, end seeks, wrapping, tails, crossfade and gain, and zero ordinary
heap allocations during construction/control/render. They do not
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
