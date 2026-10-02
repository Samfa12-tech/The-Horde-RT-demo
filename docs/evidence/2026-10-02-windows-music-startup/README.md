# Windows startup / refocus observation

Owner reports music beginning after alt-tab/refocus while checking the accepted
right cuff. They subsequently cannot recall whether the preceding silence was
in active gameplay or a paused menu. This is an observation, **not a reproduced
startup bug or a demonstrated cause**. Menus intentionally suspend music.

## Completed checks and bounded observation

Source already initializes foreground state after scene readiness, publishes
menu transitions synchronously and handles `WM_ACTIVATEAPP`. The native worker
retains the latest input/wakeup across bank/audio initialization. No score,
instrument, render, Core, PCM playback/gate or focus behavior is changed.

The existing native48k smoke now starts externally suspended before backend
initialization: ready/stopped, zero generated/played frames through150ms, then
the first focus-return publication advances actual XAudio2 consumption. Existing
pause (two150ms windows), resume, retry and stop/join checks still pass.
Final observation-enabled Debug and Release builds/smokes PASS at gain0:
[Debug](debug-smoke.txt), [Release](release-smoke.txt). This is real silent device
clock evidence, not a real HWND focus transition or audible owner acceptance.

Existing `reports/windows_audio.log` now receives only gate changes, bank/master
readiness and the first actual device consumption. Gate rows distinguish cached
application-active state, actual game-foreground equality, RT-controls-ready,
menu pause, snapshot pause, accepted publication, volume and simulation tick.
Monotonic wall milliseconds correlate the few transitions. No foreign-window
names/identities, screenshots or per-frame log/telemetry framework. Playback
decisions continue to use the unchanged existing gates, not the observation.

Three affected session/controller/spatial contracts pass in each configuration:
[Debug](debug-contracts.txt)3/3,0.97s; [Release](release-contracts.txt)3/3,0.20s.
Windows game builds PASS. Final Debug executable
`d86315e9bafc1db81fa03d6c45fa765246b6c1ca68f370848c14d753e7a8c704`;
Shipping `b5f6638d757117895621497d80ae7fe9e6c266a6919b7d8388f7caf8bc7c23ef`.
Android audio/candidate hashes are unchanged by this Windows-only observation.

## Next unfinished step / owner review candidate

Prepared Shipping candidate:
`C:/Dev/tmp/horde-sfx-volume-review-20261002/HordeLanternRT.exe`, exact hash above,
with accepted paired world46a88dac/vieweaa0db3a assets. It is a development review
stage, not a release ZIP. The [SFX change](../2026-10-02-sfx-volume/README.md) is
included. Older review stages and unrelated scratch are preserved.

One normal launch: enter gameplay before deliberately switching away; note if
music plays without a refocus. Check independent SFX slider0/low/full and stone
footsteps versus combat, then pause/resume and refocus. If silence recurs, retain
that process's narrow audio log before another run and diagnose its actual gate
or backend evidence. Do not rerender the bank or repeat the prior long-loop run.

Audio/haptic manual revalidation required: **YES** for the included requested
SFX gain/control behavior and still-open startup/refocus observation. This
diagnostic/test-only slice changes no music or haptic behavior. No main merge,
publication, phone use or performance claim.
