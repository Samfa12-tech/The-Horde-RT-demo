# Mobile lantern quality-profile decision and finite experiment

## Authority and preservation

Owner authorised open Mobile lantern apertures: omit pane geometry entirely,
without fake transparency/shadows, raster, SSR or a device-model workaround.
Native hardware RT, existing cage/flame/light, gameplay, grips and accepted player
remain unchanged. High/desktop retains full physical dielectric panes and all
transport fixes. Mobile physical panes are **deferred**, not deleted from the engine.
No merge to main, release or publication is authorised.

Base/control: `9f4f439e40a9477efe0bac42ee41b74de64b3b1b`.
Isolated branch: `codex/horde-mobile-lantern-profile`. The engineering worktree's
unfinished staged/probe changes and scratch files are preserved untouched.
Candidate commit/APK hashes: pending. Do not repeat completed trials after resumption.

## Optional optimisation gate: no supported candidate

Retained [feasibility evidence](ENGINEERING_1_6_1_MOBILE_RT_FEASIBILITY_2026-09-30.md)
places physical-lantern GPU cost around223-235ms, versus a22-24ms GPU allowance
for30FPS. The first-blocker trial did not improve matched Shipping results;
opaque-secondary omission did not help; the staged-primary candidate failed its
unchanged image gate. Even investigation-only omission of all direct visibility
left heavy live GPU cost around56ms. These are different workloads, not additive
savings or a measured pane-removal estimate. Read-only independent review agrees
that no retained evidence supports one novel low-risk, quality-preserving change
plausibly closing the physical-glass gap. Do not repeat rejected experiments.
Proceed with the explicitly authorised quality profile and measure its result.

## Bounded implementation

Select by the immutable pipeline bundle's `DielectricQuality`, before static
registration and BLAS construction, independent of instrumentation/backend/model.
Remove only primitive records using the named `LanternGlass` material. Stable
remaining primitive order preserves geometry-index/material routing. Source
vertices/indices/materials/textures/sockets stay unchanged (unused source bytes
may remain in immutable buffers, but panes are absent from the BLAS for all rays).
High selection is an exact no-op. No shader/ABI/transport/budget/material changes.
The independent physical dielectric validation fixture remains intact on Mobile.

## Finite validation matrix

- Host: canonical asset selection, retained geometry/UV/material/socket identity,
  High no-op, material reorder, fail-closed malformed selection, static-slot/BLAS
  metadata agreement; existing affected prop/socket/ABI/quality tests.
- Windows RTX: High production/physical images at unchanged tolerances; Mobile
  open-cage image and actual RT presentation. High is not phone evidence.
- Exact S26 `SM-S948B`: fresh current-music Shipping/Mobile control and candidate,
  unchanged75%, backend RayTracingPipeline. Interleaved C1/P1/P2/C2, each existing
  route, held-high and live reveal workload:12 measured runs. Opening and heavy
  denominators, native cycle/GPU/pacing and thermal context reported separately.
  No recording during timed runs. Retain allocation/RAM evidence, unavailable GPU
  bandwidth/cache/stall counters as gaps. No external cooling assumption.
- Exact S24: currently disconnected, requested reconnection; do not substitute S26
  or infer S25. Prepare the same matrix with its actual supported hardware backend.
- Diagnostic live route: pickup, high/low, walking/looking, attacks/parry, geometric
  cage/player shadows, pause/Home/resume and scale resize. Frozen captures do not
  establish live correctness or perceived playability. Explicit profile image
  changes are expected; no tolerance weakening.

Use existing native harnesses: this repo has no `.game-dev/adapter.json`; do not
introduce a new validation framework merely to use the performance/visual skills.
Restore normal configuration afterward. Audio/haptic manual revalidation required:
**NO**, geometry-only selection preserves audio/event/backend/source identity.

## Completed and next unfinished step

- Verified9f current pushed checkpoint and isolated clean worktree; local LFS
  runtime assets hydrated without regeneration. Initial control build rejected
  unhydrated launcher PNG pointers; hydrated only required resource objects and
  retained failure log. Retry is underway. This is artifact setup, not a source fix.
- No phone result or performance improvement claimed yet; S24 is an explicit gap.
- First controlAPK `d41ec87c...c147777` was invalid: enemy/sword LFS pointers
  packaged instead ofGLBs; owner reported both platform startup failures. No
  benchmark was admitted. Preserve failed artifacts/logs, do not reuse their
  timings. Hydrated required enemies/sword/SFX; rebuilt control once after all
  objects were present. Valid controlAPK `3a4faf72923e24da9da859ef6cd71c221b818b97d7418a75b1c8dd0ca0540378`
  passes strengthened package guard, which rejects the retained badAPK. Renderer
  code matches9f; unreferenced test/helper/docs do not change control rendering.
- Debug affected tests5/6 initially passed; socket test failed on missing sword
  bytes, then passed1/1 after hydration with no code change. New canonical profile
  test passed, no shader/asset mutation. High held-high control presents/captures;
  wall-low fails the existing viewmodel-primary visibility capture gate (player
  fully occluded at that checkpoint), retained rather than weakening the gate.
- S24 reconnected: `SM-S928B`/`R5CXC0G9GBW`. Its exact paired evidence remains pending.
- Final affected MSVC Debug6/6 and Release6/6 PASS. New canonical selector/static
  metadata test is included. Windows High held-high, held-low and independent
  glass-edge-Fresnel control/candidate native images are byte-identical (PNG hashes
  `3faac36e...609d7b`, `613349db...da4a`, `4b561086...04409`); both manifests complete
  with identical actual High shader identities. No pixel tolerance was changed.
- Repaired S26 C1 route completed1838 presented frames, native-cycle median66.435ms;
  row/thermal admission and paired performance conclusion still pending. S24 C1
  route is underway with required hardware RayQueryCompute backend.
- Next: commit/freeze candidate and containment proof, continue C1 heavy/live then
  P1/P2/C2 for each phone; capture candidate live interactions/lifecycle after timed
  matrix. Do not repeat valid completed rows or rebuild unchanged artifacts.
