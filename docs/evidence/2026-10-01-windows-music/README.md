# Windows Core-backed music output candidate

Development over `638528899805eb4303aaef4dbd2247480258f236`,2026-10-01.
Not a release/publication or owner listening acceptance.

One worker owns the immutable music bank, Horde `MusicDirector`/worker session
and XAudio2 engine/voices. All PCM decoding/looping/tails/crossfades remain in the
unchanged pinned Pocket Audio Core. The existing render owner copies const
snapshots/events **before** SFX drain. Music never drains/mutates gameplay/SFX.
Callbacks only release one of three fixed480-frame slots and signal the worker.
No callback IO/mixing/locks. Submitted PCM is at most30ms; native `SamplesPlayed`
is checked monotonic and bounded by accepted frames, separately from generated
content. Session decisions use the content timeline at the queued audible horizon,
not GPU-frame timing or wall time; underrun intervals cannot advance content.

Ordinary pause/focus loss stops without flushing. Retry/import destroys the old
source voice (callback quiescence) before resetting/reusing slots. Stop joins the
worker before resources/storage/context destruction. Backend/input failures
disable/log music only. Persisted `audio.musicVolume`0–100 (default70) is independent
of SFX; accessible native Settings slider/controller focus controls real output.
Normal gameplay and benchmarks include music; frozen capture mode deliberately
has no output, as with existing SFX. Older no-music performance receipts are not
matched current-music measurements.

## Evidence

- Current-source Debug/Release real Windows RT executables link successfully.
  This packet makes no fresh RT-presentation/image claim.
- Fresh actual-executable pipeline/compute bundle containment checks PASS both
  (Debug8.31s, Release7.99s); source/module containment is not device backend parity.
- Ten affected music/Core/simulation/spatial CTests PASS in Debug20.09s and
  Release10.51s. Original simulation and positional-feedback code is unchanged.
- Native gain-zero Debug/Release smoke PASS: actual device-consumed48kHz frames
  advance; pause retains queue and freezes consumption through two150ms intervals;
  resume advances; retry enters a fresh epoch; Stop joins. This is real local OS
  output-clock evidence, not subjective music/SFX mixing or Android certification.
- The native sink smoke also PASS under MSVC AddressSanitizer, no diagnostics.
  `/fsanitize=address /Zi /EHsc`, `/debug /INCREMENTAL:NO`, VS14.44.35207 runtime.
- Release extended native-clock run PASS: twenty actual12s body periods consumed
  (`SamplesPlayed=11520000`, generated/submitted11521440, queued buffers3), then
  pause/resume/retry/join PASS. This proves consumption and bounded admission,
  not audible seams, Android pacing or the final music/SFX mix.

Exact local Windows executable receipts (unpackaged, not publication artifacts):

| Executable | Bytes | SHA-256 |
| --- | ---: | --- |
| Debug RT app | 8212480 | `75d760a8bdb7316038201864bac080466b57691d879457e37f8af5c74dc825b3` |
| Release RT app | 3075072 | `e785c5db0d8c9e9f70faa3d73354316007ec3d9e795708e132851710d3d8eb6f` |
| ASan sink smoke | 2488320 | `a951fa27576d3f83b3cdc3efbc79be7c87b38191e2c7198b6439993af1cfa079` |

Initial failures are retained separately: `NOMINMAX` and private Pimpl access
compile errors; first device smoke `CO_E_NOTINITIALIZED`; initial app queue→span
conversion error; two command-selection attempts named nonexistent test targets.
The sink now owns a balanced COM apartment on its worker, matching Microsoft's
[XAudio2 initialization requirements](https://learn.microsoft.com/en-us/windows/win32/xaudio2/how-to--initialize-xaudio2).
No failed/not-run command is reported as passing. Independent static review found
no additional demonstrated ownership/clock/order bug; it is not runtime evidence.

## Repeat locally

```text
horde_rt_windows_music_playback_smoke.exe <assets-root>
horde_rt_windows_music_playback_smoke.exe <assets-root> 20
```

The second command uses gain zero for20 actual12s periods, followed by the same
pause/resume/retry/join checks. It is not a headless CI CTest: requires an OS audio
endpoint. MSVC CI compiles both the sink and executable even with Vulkan disabled.
Listening still requires a normal candidate and the A–H cue/loop/transition,
warning/SFX/volume/focus matrix. Audio/haptic manual revalidation required:YES
because playback and gain are now wired; haptic patterns/routes themselves are
unchanged. Phone unavailable; glass/performance/player acceptance unchanged.
