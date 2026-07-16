# Windows Audio Diagnostic - 2026-07-16

Status: Windows listening validation passed; Android device listening remains pending.

## Failure and correction

- Positional Windows cues were multiplied by Android-oriented mix values (`0.11` for footsteps and `0.22` for attacks) before distance attenuation and equal-power panning. The quiet source recordings could consequently fall below an audible level.
- XAudio2 initialization, parsing, matrix, submit, and start failures returned silently. Positional cues had no fallback.
- Centred cues used WinMM while positional cues used XAudio2, making output and lifetime behaviour inconsistent.

Windows cues now share the XAudio2 voice path. Centred/player-local cues use full left and right gains. Skeleton footsteps use a `1.0` emitter gain and skeleton/placeholder-lich attack cues use `0.85` before spatial rolloff. XAudio2 failures are recorded and fall back to the existing WinMM path.

## Runtime evidence

The focused Debug runtime probe recorded:

```text
XAudio2 ready; output channels=8, asset root=<repo>/assets
first voice started: ui_back.wav, format=48000 Hz/16 bit mono
first voice completed: ui_back.wav
```

The runtime diagnostic is written beside the normal reports as `reports/windows_audio.log`. It records asset resolution, device channel count, WAV rejection, and XAudio2 HRESULT failures without changing renderer reports.

The checked FilmCow WAV files are PCM, 48 kHz, 16-bit, mono. The runtime parser accepted the real `ui_back.wav`, created a source voice, routed it to the eight-channel mastering voice, and observed the submitted buffer complete.

## Build evidence

- Windows Debug `horde_rt_diagnostic_window`: passed.
- Windows Release `horde_rt_diagnostic_window`: passed.
- No renderer, shader, asset, or licence file was changed.

Human listening remains the final acceptance check because successful buffer consumption cannot prove the selected Windows endpoint or system mixer is audible to the listener.

## Player-footstep level correction

The later FilmCow dirt contacts initially measured only `0.019-0.024` RMS and `0.245-0.262` peak, compared with `ui_select.wav` at `0.053` RMS / `0.794` peak. Runtime gain alone did not give a dependable authoring baseline.

The importer now peak-normalizes only `player_step_1.wav` and `player_step_2.wav` to `0.78`, retaining 2.16 dB of headroom. The packaged results measure:

| Cue | RMS | Peak |
| --- | ---: | ---: |
| `player_step_1.wav` | 0.0718 | 0.7800 |
| `player_step_2.wav` | 0.0597 | 0.7800 |
| `ui_select.wav` comparison | 0.0526 | 0.7943 |

Windows plays player-local footsteps at unity through XAudio2. Android retains its `0.62` cue mix before the user SFX-volume setting. Windows player contacts are now driven by actual post-collision distance: first contact after 0.24 m, then every 0.72 m, with release/reset when movement stops. Skeleton contacts retain timed cadence but now use the same 0.78 authored peak before positional falloff.

## Final listening verdict

F10 directly proved both centred player-step files and the XAudio output path. The subsequent live route check accepted movement-triggered player footsteps and positional skeleton footsteps. Every accepted lich hit visibly recoils and plays a positional FilmCow body-hit cry; its source is normalized to 0.84 peak and is no longer masked by a duplicate centred fencing impact.
