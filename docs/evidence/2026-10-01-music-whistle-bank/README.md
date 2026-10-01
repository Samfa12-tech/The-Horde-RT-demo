# What the Dark Keeps — owner-selected whistle bank

The owner explicitly selected whistle-lead after hearing both A/E comparisons.
This supersedes the earlier screenshot allocation, not the composition. Rights
remain **Owner-supplied; authorised for Horde use only**. Unpublished development
assets; no general redistribution grant, device listening or release acceptance.

## Change and exact identity

- A/B/D/E/F/H: cowboy_whistle motif, soft_pluck answers, mellow_sax backing;
  felt_piano chords0.20; chorus0/delay0.04/mix0.22/reverb0.46.
- C keeps three authored bell attacks; G keeps four bell answers. Nothing is
  added to the musical grids. Pitches, note starts/holds/repeats, octaves, pan,
  tempo80, sequence, cue mapping and gameplay timings remain unchanged.
- Canonical JSON `f5d4bbc8068ae2865216304efc22337ac190ca5b7732e02a9ed5e9062b7b92f4`;
  PCS1 `747e5a9a21f41f0fc587fe55572b03a6196cd25cece17283775dd03130164dab`.
- Runtime manifest and PowerShell/Gradle admission pins
  `ea7adbc1c248fc37d705239e6bcd529fbf10206ffb77165051658bce4e438c3e`.
  Sixteen48kHz stereo PCM16 WAVs, exact original body/tail lengths:
  **20,160,000 decoded PCM bytes /20,160,704 WAV bytes**. No preview gain or
  normalization in runtime, no PCM playback/backend/volume changes.

Accepted A/E PCM is reused byte-for-byte from the selected
[audition](../2026-10-01-music-instrumentation/README.md), after active-cue source
projection equality. Only B/C/D/F/G/H rendered once. Actual retained v68 app
voices/scheduler/live FX, isolated source2b87d7b1 and HTMLb266814f…b6e2cf;
adapter689fa712…e05aa. No Core/WAV synthesizer, synth/editor vendoring, shared
preset changes, paid assets or Pocket DAW access. Recovered producer scripts
are checked against their recorded hashes, not relabelled with newer code.

`SHA256SUMS.json` binds43 archived records (327,571 bytes) and23 referenced
runtime/source/tool files; raw six-cue receipt gzip/decompressed bytes agree.
Digest `0eb6e9636b0a719b9bc1c6ddac94c7c14edef39ff573924e554e160d98d5ae85`.
Original bank remains recoverable from Git and original rendering evidence.

## Completed checks — do not repeat on resumption

| Check | Actual result and scope |
| --- | --- |
| JSON/PCS1, actual app imports, source holds/onsets and actual scheduler/call traces | PASS all eight cues against original musical events; each import route compared to its own schema reference |
| WAV headers, frame lengths, hashes, float/PCM headroom | PASS16/16; no non-finite or clipped samples; largest native peak E0.646850586FS |
| A/E production Core offline auditions | Release four retained variants and Debug selected whistle PASS: twenty loops each, irregular chunks, pause/resume70%, twelve crossfades per variant. No OS playback |
| Newly rendered bank supplement | Debug and Release PASS: B/D/F/H twenty loops each; C/G body+tail+silence; six natural C→D/G→H handoffs at full/partial/exhausted tail; four early250ms crossfades |
| Sample oracle | Loop/native handoff/early-fade errors below unchanged four-float-epsilon bound; new bank max early-fade error2.19e−8FS |
| Existing native real-WAV and transactional-bank suites against full staged bank | Fresh MSVC Release2/2PASS1.26s, Debug2/2PASS4.10s; includes strict decoder/corruption/frame and immutable-bank checks |
| PowerShell closed-roster/source/hash/stage/package admission |14 passed0 failed after new pins; subsequent direct asset policy PASS. Prior shell wrapper incorrectly read stale native `$LASTEXITCODE` after a successful PS script; not a test failure |
| Android asset-only tasks | `:app:verifyMusicRuntimeAssets :app:prepareRuntimeAssets` SUCCESS11s; no native/shader build or device install |
| Actual Android staged output |17 files byte-identical to new bank,20,165,775 total bytes including manifest; no source/unknown entries |

Native sample-loop boundaries B/D/F/H are respectively0.0000305/0.0002136/
0.0009766/0.0000610FS at100% gain. These are measured waveform deltas, **not**
proof of inaudible seams. Default70% gain and existing runtime memory unchanged.
The selected A/E default70% combat mixing probe was already unclipped; numerical
headroom is not perceptual warning/SFX masking acceptance.

## Demonstrated failures and evidence limits

The first verifier compared legacy PCS schema16/title omission to JSON schema17.
Corrected to separate pinned reference routes; canonical JSON/PCS1 and every
musical field still must agree. No rerender or weakened musical requirement.

The strict native fixture's historical “every WAV nonzero” assertion rejected
C's natural silent tail. Actual float tail peak2.893643874860019e−12FS is far
below PCM16 quantisation, not a dropped render. All144000 frames remain.
Bodies must still be non-silent; a new silent-tail regression retains samples
and duration. The initial correction used a nonexistent helper and failed
compilation; corrected existing helper and fresh2/2 results are retained.
The isolated checker also initially rejected intentionally aligned shared ABI
padding under `/WX`; scoped `/wd4324` retains all other strict warnings without
changing Core or renderer ABI. Original failed logs remain alongside corrections.

Initial Release audition results lack pre-bound executable/input hashes. Debug
whistle and Release-bank bind PCM/source before/after but originally only named
the runtime manifest. Final Debug-bank also hashes the manifest and audition
archive before/after. Do not upgrade historical receipts into stronger proof.
Sequential asset admission is not atomic; manifest-last package/hash gates fail
closed on partial assets, with original bytes recoverable. `runtimeNotChanged`
in historical prepare/verify receipts describes those pre-admission steps;
`admission.json` establishes the later local change.

At bank checkpoint97f07e2, fresh push36823996776 and PR36824003880 each pass
GCC55/55,Clang55/55,MSVC56/56 and focused Vulkan CPU-host15/15. Actual eight job
logs inspected; synthetic integrationf528ce88 has exactbda1b99/97f07e2 parents
and the identical head tree. Compiler evidence is not device/audio acceptance.

No new Windows audio endpoint, phone run, native consumed-clock, physical RT,
memory/pacing or renderer/backend acceptance. Unfinished primary-hit diagnostic
work and unrelated scratch remain outside this commit. No merge/publication.

## Next unfinished gate

Exact-S26 instrumentation/cue preference is accepted, but first6f1be881 phone
playback is slow/crackly and rejected. The Android-only35ec7e46 bounded-buffer
candidate is installed; its [separate finite phone record](../2026-10-01-music-whistle-phone/README.md)
owns consumed-clock/lifecycle and owner recheck. Normal70% Music Volume through
exploration, torch pickup, combat, lich and dawn/finale still needs final balance.
Check repeated loops, transitions, warning/hit audibility, separate persisted
volume, pause/focus/background/resume and native-clock/underrun diagnostics.
Do not ask for S24 now: its glass gate remains separate. S26/DAW are available;
no further DAW use is needed. New device
checks belong in a separate exact-artifact listening record, not this historical
offline receipt manifest.

Audio/haptic manual revalidation required: **YES** for changed audible music
assets/timbre/balance. Existing SFX event authority and haptic implementation are
unchanged; no new haptic retuning requested.
