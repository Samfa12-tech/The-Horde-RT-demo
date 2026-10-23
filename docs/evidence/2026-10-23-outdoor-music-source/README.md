# Outdoor music: Drive source verified; runtime prerequisite open

The owner-provided [Drive folder](https://drive.google.com/drive/folders/1ar5VQEpp-lgX-kTRULzYx8FOxQaIhKMA)
successfully supplied the two original FLACs and editable source ZIP. All three
expected names and sizes match the approved r2 handoff. The ZIP's score identifies
`Beneath the Same Moon — Outdoor Exploration r2`, 72 BPM. Original Drive names,
IDs and local SHA-256s are retained in `drive-receipts.json`; no sharing settings
were changed. This independent owner source does not bypass or erase the earlier
Library metadata-helper failures. No Library download/helper was retried.

Originals and verbatim editable JSON/PCS1 are preserved under
`assets/source/beyond_the_tomb/music/beneath-the-same-moon-outdoor-r2/`.
`source-manifest.json` records exact bytes and source rights; the original ZIP
retains its rendering/finishing recipes and earlier forest provenance. Imported
scripts were inspected as data and were not executed. FLAC/ZIP originals use LFS.

## Actual verification

Both FLACs decode fully without errors. Each is stereo 48 kHz PCM24 with exactly
5,120,000 frames (106.666667 s), and each complete decoded PCM24 MD5 matches its
FLAC STREAMINFO checksum. Measured peak/RMS agree with the source-pack metrics
within 0.001 dB, with no clipped samples:

| File | SHA-256 | Peak / RMS dBFS | Measured PCM24 wrap jump |
| --- | --- | --- | --- |
| main | `38244c2101d42990e37482e75995b7aa8de05a91a9753b8c1566632da64a7f6a` | −8.000 / −27.136 | −110.870 dBFS |
| quiet bed | `3715eea723931f04b4eaa43e9c7832e82b68de29d57afd8020b4fe2c74c39b27` | −14.484 / −34.266 | −128.931 dBFS |

Source ZIP SHA-256:
`26629cd9478f9a01fc617af16842e2e444c0cf2376bebd48e821b3026340b275`.
These are locally verified checksums. The parent's original WAV hash was not
available, so no independent byte-identity claim against that WAV is made.
Equal frame counts and source-pack provenance support the supplied alignment;
they do not establish in-game switch or listening acceptance.

Offline PCM16 candidates preserve all frames without gain, trimming, normalization
or sample-rate change. Each strict WAV is 20,480,044 bytes and has a zero-LSB endpoint
wrap difference. Hashes/method are in `pcm-candidates.json`; candidates remain
under ignored `reports/outdoor-music-20261023/pcm-candidates/`, outside assets,
packages and Git. A zero endpoint difference is not an audible-loop acceptance.

## Exact integration blocker

Runtime integration is **pending**, not complete:

- The pinned Core loop stream rejects bodies above 576,000 frames (12 s) in
  `third_party/pocket-audio-core/native/include/pocket_audio/PcmLoopStream.h:14`.
- Its decoder independently rejects expected frames above576,000 in
  `.../include/pocket_audio/PcmWave.h:12`. The outdoor file needs 5,120,000.
- Horde's director wraps existing loops at 12 s; its bank/adapter require a 3 s tail
  for every asset. Simply replacing H would also replace the legacy dawn music.

`PROJECT_DECISIONS.md` and `third_party/pocket-audio-core/README.horde.md` require
reusable PCM changes in canonical Pocket Chordsmith, followed by exact reviewed
subset admission. No Core fork, shortened composition, chunk-stitching workaround,
synth import, deadline relaxation or runtime change is made here.

A read-only attempt to inspect `C:/Users/sam_s/Documents/Pocket Chordsmith` returned
**Permission denied**. No files there were read/changed and no retry/escalation was
attempted. The Drive transfer and Horde-local verification completed independently.

The smallest proposed prerequisite is an explicit bounded long-PCM opt-in in
canonical Core's decoder and loop stream, preserving default 12 s limits and their
negative tests, then a byte-exact Horde pin update. This requires its own reviewed
checkpoint. No streaming or synthesis system is required. Both resident PCM16
loops would add 40,960,000 bytes: existing bank 20,160,000 → 61,120,000 bytes; loading
with one encoded temporary would reach about 81,600,044 bytes. These are calculated
payload bounds, not measured process/RSS or phone acceptance.

## Subsequent Horde integration and acceptance

After the canonical prerequisite: introduce separate outdoor main/quiet cue rows,
per-cue duration and optional empty tails; preserve legacy A–H/dawn and natural
G-tail handoff. Keep one sample position across aligned main/quiet switches,
lower-tomb F, repeated ascent, saves, pause/focus/retry/death and independent user
volume/ducking. Add a separate outdoor runtime manifest and aggregate packaging
policy; retain the old seventeen-file roster/pin. Sources and FLACs stay excluded.

Require long-loop wrap/seek and cap negatives, irregular PCM buffers,
phase-continuous variant switching, chapter transitions, bank failure/memory and
source-leak rejection before playback admission. Measure startup/memory and
audition with Kit; quiet-source selection plus existing ducking may be too quiet.
No endpoint listening, phone, natural journey, release or synthesis claim is made.

The current tomb music is Pocket Audio Core PCM playback from 16 pre-rendered
A–H body/tail WAVs. Editable PCS JSON/PCS1 is composition source, not a runtime
sequencer. `MusicDirector` selects cues, `MusicPcmStream` adapts the immutable PCM
to Core's cursor/mixer, and platform outputs retain volume/lifecycle ownership.
