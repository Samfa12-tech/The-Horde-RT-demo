# Audio-worker session and copied-event admission

2026-10-01, engineering branch; parent `c122e4a22d3e66d0e85cc30d615da337ff538631`.
This packet validates the shared worker bridge only, not platform playback,
audible continuity, exact-phone performance or owner listening.

`MusicPlaybackInbox` copies immutable snapshots and ordered events before the
existing SFX queue is drained. Latest snapshots may coalesce; copied event edges
are deduplicated and retained (128 entries, newest-drop overflow reported and
playback rejected). Queue replacement, retry and explicit checkpoint import
advance an audio epoch. Core/session storage has one noncopyable worker owner.

`MusicPlaybackSession` retains Horde's cue decisions and delegates all PCM
loop/tail/crossfade work to the unchanged pinned Pocket Audio Core. Its counter
is **generated content**, not device consumption. Sinks must separately track
accepted and device-played frames and bound their queues. Ordinary suspension
does not advance the generated clock; a session reset cancels old cursors/tails.

Irregular buffers cross C→D at exactly144000 and G→H at exactly288000 frames,
even with stale gameplay publication. Last one-shot samples are retained before
the next cue plus natural tail. Director's 1 ns boundary epsilon compensates
double accumulation only (less than0.00005 of a48kHz sample); the test rejects
completion one whole frame early. Twenty12s PCM loop periods match direct Core
output exactly. Tests cover coherent concurrent publication, copied-event
ordering/lifetime/retry/import and both copied/original-queue overflow rejection.

## Current-run results

- MSVC Debug: seven focused music/Core CTests PASS,24.92s.
- MSVC Release: same seven PASS,15.52s.
- MSVC AddressSanitizer Debug: session test PASS,3.74s; no sanitizer diagnostics.
  Build uses `/fsanitize=address /Zi /EHsc`, `/debug /INCREMENTAL:NO`, matching
  VS14.44.35207 runtime on PATH. No prior loader/link failure is counted as a pass.
  Exact ASan test executable SHA-256:
  `3f2aaf0b359fac241a7c4a441d2e8c63cbecf08918d6a5f61d5fc511f12872c2`.

The six raw final logs are retained here. Earlier initial CMake Threads-target,
test-template compilation and premature not-run attempts remain in the external
working log directory; they are not the results reported above. Android and
Windows sinks are separate uncommitted candidates at this checkpoint. No phone
actions were taken while the owner disconnected it. Audio/haptic manual
revalidation required:NO for this unwired bridge; YES once playback is integrated.

## Reproduce

Build Debug/Release targets `horde_rt_music_playback_session_tests`,
`horde_rt_music_director_tests`, `horde_rt_music_pcm_stream_tests`,
`horde_rt_music_pcm_wave_tests`, `horde_rt_music_pcm_asset_bank_tests` and
`pocket_audio_native_pcm_tests`, then run:

```text
ctest --test-dir <build> -C <Debug|Release> -R "horde_rt_music_|pocket_audio_native_pcm_tests" --output-on-failure
```

The asset admission PowerShell test is included in that seven-test selection.
