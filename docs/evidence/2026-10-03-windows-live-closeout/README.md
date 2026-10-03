# Windows foreground Shipping/High closeout

October 3. No renderer, audio, player, shader or runtime change. The unfinished
foreground/UI check uses the already admitted current-player/current-music ZIP
`d2984666b292862c13cf1b8161ad2c085495bafb5c0d844f39538a7a3df58b12` and executable
`11e0a59b032cee25296d99cbf74d531871d98abbfe784823cc52f9b3b84dba45`.
Source/runtime provenance stays in [package admission](../2026-10-03-windows-music-package/README.md).
Documentation HEAD at execution: `91dacfd075e5509192f824122a6caf095a9774f8`.

## Finite completed checks

Only own process38140, launched with `--require-rayquery-compute`, was controlled
through the native Windows-control skill. Its actual scene was raised/inspected,
not the initially occluding Codex image. Native title is Showcase Alpha1.6.1.

- Interactive Run Benchmark displays readable three-line RT-loop FPS, mean ms,
  last-frame count, lap/waypoint and explicit "not display Hz" qualifier.
  Two actual images show121.1FPS/8.3ms and112.9FPS/8.9ms, each last60frames,
  at different waypoints. Accessibility and image capture can sample different
  frames; their values are not asserted frame-synchronous.
- Escape cancels the first prefix, removes the live counter and returns the
  branded paused menu. Restart starts a fresh run and finishes2/2 laps,
  26/26 waypoints. A sub60-frame restart window was not captured on this fast
  GPU; no claim is made that that separate reset observation occurred.
- The completed report is automatically saved by the application at
  `HordeLanternRT-benchmark-20261003-004125.json/.txt`. Integrity COMPLETE,
  route/workload complete, all1838 measured frames presented/CPU accepted/GPU
  valid; zero rejected, cancelled, outstanding, pending/error/missing rows.
  Every retained row uses opaque-fast, diagnostic status compiled-out, null
  diagnostic counters and matching submitted/completed identities.
- Actual backend **RayQueryCompute**, Shipping/High, RTX5050 Laptop,
  Vulkan1.4.341,100%,1232x803 internal/presentation, MAILBOX. Coarse `rtMode`
  still describes Pipeline capability; it is not the selected launch backend.
- The inspected live opening/restart image shows textured modelled hands,
  held torch/sword and two enemies. This closes bounded live Compute presentation,
  not strict pixel parity, all-scene subjective acceptance or High glass defects.
- Back to Menu removes FPS text; own Quit closes the process. No phone use.

Whole-cycle median7.557ms/p95 9.875ms; GPU median3.213ms/p95 4.748ms. These are
single foreground fixed-step route results with the observer on, not matched
on/off overhead, sustained player pacing, an optimisation or phone evidence.
Observer overhead remains **unmeasured**; the counter remains disabled in matched
performance/frozen runs. No further performance campaign is queued for1.6.1.

## Update discovery

Released1.6.0 source57c81b6 already has delayed Android and Windows startup checks,
both on IncludePrerelease; current shared updater/Windows integration are unchanged.
The new explicit1.6.0→published `v1.6.1` prerelease fixture passes the registered
Release CTest1/1. It tests shared selection, not an actual installed-app dialog.
Anonymous GitHub releases metadata currently contains only `v0.1.3-alpha.1`.
At separately authorised publication, a **non-draft GitHub Release `v1.6.1`**,
with the reviewed artifacts, is required. Itch-only publication cannot trigger
the updater. Update now opens the verified release page; no automatic install.
Offline/API failures can prevent a prompt; no new release/tag was created here.

## Receipt and limits

`archive-evidence.ps1` retains only this owned game's unique report, capability
snapshot, original JPEG/UI observations and focused updater CTest. JSON is
losslessly gzip-compressed; the receipt records both raw and archived hashes.
The first archival assumption that native captures were PNG was rejected before
writing; the original JPEG is retained without transcoding. Initial bare `ctest`
was absent from PATH; the recorded CMake-cache path successfully ran the test.
These tooling attempts are not game failures or extra benchmark runs.

Audio/haptic manual revalidation required:NO: runtime and semantic inputs unchanged.
Owner's separate cuff/footstep/music approval is recorded in the current handoff.
No merge, signing, release or publication.
