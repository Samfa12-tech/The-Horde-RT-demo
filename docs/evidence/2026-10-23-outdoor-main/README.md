# Outdoor main through shared Pocket Audio Core - 23 October 2026

The approved full **Beneath the Same Moon — Outdoor Exploration r2** main now
uses the existing Pocket Audio PCM bank, director and platform workers. This
checkpoint was developed over Horde `5b9dc70583eb48b314ce4105c326b2216ac810c8`
in the existing `.worktrees/horde-1.7-wp2-vertical-proof` checkout and PR 27 branch.
It does not add a second music system, a quiet duplicate or a synthesizer.

## Provenance and runtime behavior

The exact eight-file native subset was fetched through the authorized GitHub
repository API at canonical commit
[`b77a9d0bfe3051c6e352815295164b9b80586f56`](https://github.com/Samfa12-tech/Pocket-Chordsmith/commit/b77a9d0bfe3051c6e352815295164b9b80586f56).
Each decoded response matched its Git blob ID; the imported bytes match the
dependency manifest. Reconstructed upstream SHA256SUMS roster hash is
`6271eb9ba088a115b1f6031f6ccf349c2bdb016b191d799340e0b951dd1d6c0b`.
See [the receipt](upstream-receipt.json). The previously denied local sibling
folder was not accessed or retried. Core retains its 12-second default, explicit
bounded opt-in, 120-second hard ceiling, aggregate bank budget and unchanged
allocation-free control/render path. License metadata is unchanged.

The [verified original sources](../2026-10-23-outdoor-music-source/README.md)
remain intact outside package inputs. The previously verified full main PCM16
candidate is admitted without trimming, gain, normalization or resampling:

- 48 kHz, stereo PCM16, **5,120,000 frames / 106.666667 seconds**.
- WAV 20,480,044 bytes, SHA-256
  `14fe0a94e665cf25b4fb4aa08a45e26e18990794740b670906adfecfaf6aee15`.
- Runtime manifest SHA-256
  `5318990386a47c7ef815cfcd5165e779bd01b0994cdca3f76926882d1d4e1657`.
- Dependency manifest SHA-256
  `d67dc2f01f9f010c2612e73d195774786c276ab23c737a4b6d4906d230b34da6`.

Only rescue-chapter exterior music changes: first ascent retains G, then its
natural tail hands off to Outdoor; forest/lookout retain that full-period loop.
Return to the tomb selects F; repeat ascent selects Outdoor without replaying G.
The legacy showcase G-to-H dawn transition and every A-H WAV remain unchanged.
The director uses each cue's actual period, rather than wrapping Outdoor at 12 s.
The existing smooth dialogue gain ducks the same main PCM, with no alternate bed
asset. User volume preferences and the shared pause/focus/retry/death/reset
contracts remain in place. No save schema, simulation or movement change.

## Measured loading and package size

The immutable bank owns **40,640,000 PCM bytes**: legacy 20,160,000 plus one
outdoor 20,480,000. A Debug host actual-file load took **387.434 ms** in this one
run. The largest encoded read and observed temporary input capacity were each
20,480,044 bytes. Bank plus that temporary input is 61,120,044 logical bytes;
this excludes allocator, output queues and other process memory. It is not an
RSS measurement, Android loading measurement or sustained performance claim.
The temporary WAV is released before playback. Full-clip decoding still occurs
off the audio callback; this is not compressed streaming.

The [actual four-ABI Debug APK](android-apk-receipt.json) is **200,120,444 bytes**,
SHA-256 `9a92cdedc393e600f70b0a513bf9cf997b6ddb8723af9cf4543e1d65eb2c200a`,
still version 1.6.2-debug/code 10. The prior saved APK was 174,177,635 bytes,
SHA-256 `d4cd5b021cc2b71b65f6ce828cbc560e0380ebd804f112ee2bb248eda4de27c8`.
Neither is a new release artifact. The current total delta is 25,942,809 bytes:

| Source of APK delta | Bytes |
| --- | ---: |
| New main WAV, stored without ZIP compression | 20,480,044 |
| New manifest, ZIP compressed | 672 |
| Other changed entry payloads (native libraries, DEX and licenses) | 38,954 |
| Unreferenced gap in the incrementally updated archive | 5,410,380 |
| Other local-header/alignment/central-directory delta | 12,759 |
| Total | 25,942,809 |

[Local-record inspection](apk-layout-audit.json) locates the gap between byte
15,805,081 and 21,215,461; the signing block remains 4,096 bytes in both APKs.
This space is separate from referenced asset payload. No repack, destructive
clean or new release build was performed to remove it. Do not call the whole
25.94 MB delta the intrinsic music cost or predict a clean APK's exact size.
The [entry deltas](apk-entry-deltas.json) and [gap receipt](apk-incremental-gap.json)
retain the distinction.

The [Windows-style audio-only positive ZIP fixture](windows-audio-fixture.json)
contains the 19 admitted music entries. Its new WAV compresses to **13,546,616
bytes**, and its manifest to 654 bytes, with the existing .NET optimal ZIP path.
This is measured fixture compression, not a complete current Windows
distribution ZIP or a new installer. ZIP/FLAC compression does not reduce the
full decoded PCM residency described above.

Both package policies admit exactly the old 17 files plus the new manifest and
main WAV. The actual APK passed byte/hash/inventory inspection: no source FLAC,
editable source ZIP, quiet mix or metadata master is included. CI hydration adds
only the runtime WAV, not the source originals.

## Validation and limits

| Check actually run | Result |
| --- | --- |
| Canonical vendored native Core, MSVC Debug / Release | 1/1 each, 4.38 / 1.61 s |
| Focused Horde Debug CTest selection | 8/8, 32.41 s |
| Closed source/runtime/package admission | 25/25, including source/quiet/missing/corrupt/pin negatives |
| Windows full application and affected test targets | Built successfully |
| Actual XAudio2 output, one complete Outdoor period at silent gain | Passed, 108.291 s, no timeout |
| Android offline Debug build and generated-asset verification | Passed, all four configured ABIs |
| Android Java tests | 411 declared, 405 executed, zero failures/errors, 6 existing host-link skips |
| Actual resulting APK hash, format, inventory and source exclusion | Passed |

[Detailed host output](focused-debug-details.log) includes actual full-main WAV
decode/sample parity, exact full-period wrap and seek, legacy default rejection,
late failure cleanup/retry, loaded-side routing, natural G tail, pause/focus,
death/retry/reset and unchanged legacy dawn checks. The endpoint smoke consumed
5,120,160 played frames with at most three queued buffers, froze the device clock
through two 150 ms paused intervals, resumed, retried and joined its worker.
It published an isolated audio snapshot, not earned gameplay state, and ran at
gain zero. [Output](windows-outdoor-smoke.stdout.log) and
[process identity](windows-outdoor-smoke.process.json) are retained.
An analytical sum of actual G tail plus the first three seconds of Outdoor at
gain one peaks at -11.6585 dBFS, with no clipped samples; this is
[sample arithmetic](g-outdoor-headroom.json), not actual listening.

Reproduction uses the existing Windows presets and the targets/CTest names in
the retained logs. Core target is `pocket_audio_native_pcm_tests`; the initial
mistyped target failure is retained in `core-debug-wrong-target-retained.log`.
For Android, from `android/`, with the installed SDK in ANDROID_HOME:

```powershell
./gradlew.bat --offline --no-daemon --max-workers=2 :app:verifyMusicRuntimeAssets :app:prepareRuntimeAssets :app:assembleDebug :app:testDebugUnitTest
```

For the actual Windows audio endpoint, the built `horde_rt_windows_music_playback_smoke.exe`
takes `<absolute-assets-root> outdoor 1`. Archive layout can be reproduced with
[the read-only inspector](inspect-apk-layout.py). Original masters, test APKs,
raw build trees and generated ZIP fixtures stay outside Git evidence. Log copies
normalize line endings and trailing whitespace only; raw reports remain ignored.

**Audio/haptic manual revalidation required: YES — the new audible outdoor main
and its G/return transitions need actual listening with Kit and current volume
settings.** This does not request unrelated haptic revalidation. No phone was
queried, reconnected, installed or tested. Android sound/lifecycle/loading,
natural victory-to-journey, owner mix/loop seam and warm performance remain open.
The inherited Debug 30-second route gate, torch clearance/framing defect and PR
conflicts remain open. Existing source-head CI passes do not certify this new
checkpoint; its exact-head hosted result must be recorded separately.

[Custody checks](custody.json) confirm canonical owner documents and desktop
settings unchanged; the owner desktop campaign directory remains absent.
Untracked owner world assets were preserved. No merge, release or publication.
The next independent approved player-facing gap is the treasury-road/chimney
vista cue, after inspecting the current RT vista; no scenery edit was started
while this audio prerequisite was being integrated.

## Hosted failure and focused fixture correction

The first integration head `f21b030eea2fe112080f3a2482303929bd3d422b`
[run 39896071822](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39896071822)
fails the same dialogue-ducking registration in GCC and Clang: **156/157 pass**
in each lane, with 18 failed checks within that one registration. Sanitizer,
Vulkan-host and Android jobs pass; Android executes all 411 Java tests with zero
skips/failures/errors. The complete six-job conclusion remains in the workflow.
The [original hosted excerpts](first-hosted-failure.log) and
[Windows reproduction](ducking-before-tests.log) retain the failures.

`DialogueDuckingTests.cpp` still constructed a three-second synthetic tail for
EVERY cue. Outdoor deliberately has no tail, so strict bank admission rejected
the fixture before PCM/gain/pause assertions. This fixture was missed in the
first local affected selection. The correction uses each admitted row's
`tailFrames`, matching the existing body-length construction. An explicit bank
validity assertion now diagnoses malformed fixture data before output checks.
All 160 original assertions are retained; **161 checks, zero failures** pass in
the corrected Debug registration ([details](ducking-after-details.log)). No
production source, cue, envelope, gain, cap or deadline changes in this fix.
The earlier eight focused passes plus this corrected registration cover the
nine affected gates across separate runs; no full unrelated local rerun was
performed. Hosted results for the correction require their own exact head.

The corrected music head `e954116917c236d2199aee9e38209bd3936c076e`
[run 39897323313](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39897323313)
finishes **cancelled**, with **five successful jobs**. GCC and Clang now pass,
as do Android, Vulkan-host and ASan/UBSan. MSVC hits the unchanged 15-minute
job cap: the last surfaced completed case is 112/169, and case 113 (Windows
glass graphics contract) is active when cancellation occurs. There is no final
Windows selection or subsequent six offline-check completion. This does not
prove case 113 hung and is not a six-job pass. The [final receipt](hosted-final-receipt.json)
and [cap excerpt](corrected-msvc-cap.log) retain both changed-head outcomes.
No deadline, assertion, workflow gate or runtime was relaxed for this result.
