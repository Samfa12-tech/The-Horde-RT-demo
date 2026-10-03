# Windows startup / refocus observation

**Latest status:** reproduced, repaired and accepted in the owner run below;
the previous uncertainty is retained as historical evidence. Do not repeat the
completed startup smoke, listening check or focus transitions after resumption.

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

## Historical observation candidate / requested owner check

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

## Demonstrated control and targeted foreground-gate repair

Control source `90b3d3a`, Shipping executable `b5f6638d...` above. The owner now
confirms silence through active gameplay and repeated pause/menu returns, with
music beginning only after alt-tab/refocus. SFX balance/independent control are
accepted in that same run. Android does not exhibit this reported defect.

[Actual control log](control-live-audio.log) proves a stale cached activation
gate: `active=0 foreground=1 controls=1 menuPaused=0`, with simulation ticks
advancing through408 and588 across play/menu transitions. The bank/master is
already ready; first device output appears only after `active` becomes1 on
refocus. This demonstrates why music was suspended, not why Windows activation
notification delivery left the cache stale. The [owned RT report](control-live-rt.json)
records actual RTX5050 Laptop RayTracingPipeline swapchain presentation.

Repair removes the latched `musicWindowActive` input. Each existing publication
samples whether the actual game HWND is foreground; controls-ready and menu
pause remain gates. Both app/window deactivation notifications suspend
immediately, including an unsettled foreground handover. This is a small pure
policy plus existing publisher wiring, not a new playback engine. Score, PCM,
Core, cue authority, renderer, haptics and Android playback are unchanged.

Fresh Debug/Shipping game builds PASS. [Debug contracts](focus-fix-debug-contracts.txt)
5/5 PASS,2.70s; [Release](focus-fix-release-contracts.txt)5/5 PASS,1.19s. They cover
all16 focus/ready/menu/loss combinations and publication wiring, SFX settings,
desktop controller, music session and spatial feedback. They do not replace
an actual window or audible owner check.

The distinct development stage is
`C:/Dev/tmp/horde-windows-music-focus-fix-20261002/HordeLanternRT.exe`, Shipping
SHA-256 `243acc6b9b768d7e8e618e9bf98b1e73087f36e43b3b80b58d26fd43505948e2`,
opened normally as PID25812 with unchanged accepted world46a88dac/vieweaa0db3a
assets. The old control stage/settings are preserved. The live candidate log
records first native output16ms after the initial gameplay publication,
before any focus-loss event. [Retained candidate log](focus-fix-live-audio.log)
also records actual play/menu/loss/return transitions. The [owned candidate RT
report](focus-fix-live-rt.json) confirms successful hardware RT swapchain
presentation. This is not sustained performance evidence.

Owner answers the exact-candidate initial-play/menu/refocus question:
"yep - it works as it should now". Windows startup/focus acceptance is CLOSED.
SFX is also accepted; no repeat SFX or instrumentation review.

Implementation5e20f10 is pushed/verified on both engineering branches; fresh
push37001774114 and PR37001780006 are SUCCESS. [Exact CI receipt](CI.md) records
compiler/Android results and the verified integration merge parents.

**Next unfinished step:** continue only remaining 1.6.1 Windows Shipping
live/backend and exact-device final-candidate gates from the existing handoff.
Audio/haptic manual revalidation required: **YES**, Windows music focus behavior
only, completed and owner accepted. Future unrelated changes return to the normal
change-trigger rule. No phone use, publication or main merge.
