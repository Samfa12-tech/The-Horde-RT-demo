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
Candidate: `1da483d01124ca673badf6c13939d99736c4a658`, pushed and verified on both
the isolated and engineering remote branches. Original engineering checkout dirty
work remains untouched. Do not repeat completed trials after resumption.

Frozen Shipping/Mobile artifacts (current music, no staged/probe/isolation path):
- Physical control APK SHA-256: `3a4faf72923e24da9da859ef6cd71c221b818b97d7418a75b1c8dd0ca0540378`.
- Open-aperture APK SHA-256: `bac94c45bcfe3f8fcc367f21439735387d188959005b5ee873d02f0f1e9cf142`.
- Normal Debug/Mobile APK SHA-256: `e10b02034e0a0a78828c6ff885d03b79dd2058f77c6f20346a7c977edd794af4`.

Retained artifacts and raw trials are under
`C:/Dev/tmp/horde-mobile-lantern-profile-20261001`; do not rebuild unchanged APKs.
Actual paired APK inspection confirms70/70 asset payloads byte-identical and all
six aligned SPIR-V modules byte-identical. All12 extracted modules pass fresh
Vulkan1.2 validation/disassembly; the four Shipping/Mobile modules have no
diagnostic atomics/image reads/Binding22. The geometry hook is the rendering change.

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
- Exact S24 `SM-S928B`: connected and running the same finite matrix, actual
  RayQueryCompute backend. Its75% extent810x1682 differs from S26's1080x2235;
  report within-device comparisons, not a hardware ranking. Do not infer S25.
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
  retained failure log. Subsequent complete artifacts are frozen above. This was
  artifact setup, not a renderer fix.
- Initial setup had no admitted phone result; subsequent valid trials are listed
  below. Do not reinterpret setup failures as performance evidence.
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
- Repaired C1 route/high/live completed on both phones. S26 route1838 presented
  frames, native-cycle median66.435ms; held-high600 frames235.8745ms. S24 route
  median47.2864ms, held-high265.3316ms. Six C1 reports passed the original
  control-aware row admission; preserved receipts predate the extended source/
  serial/ledger/producer-hash gate. Later parser tests11/11 PASS, including green
  candidate and nine fail-closed mutations. New P1/P2/C2 receipts use the pinned
  extended parser. Paired conclusions remain pending. Native-cycle reciprocal is not measured display
  pacing; completed-report UI pauses rendering and the boundary SurfaceFlinger
  layer query exposed no usable latency layer. Record that as a gap, not a pass.
- First S26 P1 held-high completed600 frames at112.0319ms native-cycle median,
  not a sustained matched conclusion or30FPS acceptance. P1/P2 complete on both
  phones; current C2 blocks continue;
  never restart completed rows. Memory boundary samples do not prove GPU bandwidth,
  cache misses or stalls. Retained S26 source enumeration exposed no counter specs;
  unavailable counters remain gaps, not evidence that memory is the culprit.
- Current1da CI push36834923024 and PR36834930180 both pass: GCC55/55,
  Clang55/55, MSVC56/56, focused Vulkan CPU-host15/15 in each. Actual job logs
  inspected. PR15 is draft/mergeable; no main merge/release/publication.
- Windows Mobile Diagnostic pipeline held-high/held-low/glass-edge and Compute
  held-high/glass-edge all complete and honestly present. Retained primitive
  metadata320 to304 bytes; actual pane geometry omitted. Compute held-high has
  visible hands and46309 diagnostic primary-player pixels. This rejects universal
  desktop compute-path absence, not exact S24 absence or Shipping/Diagnostic parity.
- Windows Debug shell with Shipping/Mobile compute modules now completes held-high;
  visible hands and PNG byte-identical to Diagnostic same-backend counterpart,
  SHA256`915d4945f3870b50955ee75f7be9ca5297416d6c04f63880c10ec0b2938556a8`.
  This is one Shipping/Diagnostic check, not full pipeline/compute parity or phone
  proof. Release capture automation is correctly rejected; no Release performance
  is inferred from this Debug-shell image. Normal High configuration untouched.
- Owner S24 control observations: one rather than two starting skeletons and
  yellow-looking water. Not yet a matched-state reproduction; compare exact
  two-enemy/skylight/water checkpoint images on both devices after timing, record
  state/backend/presentation format. Do not reopen player tuning or change water
  colours/gameplay as a workaround.
- Subsequent owner comparison: water colour appears the same on S24 and S26.
  Do not spend further investigation on a device-colour discrepancy without new
  evidence. The owner prefers the open/no-pane lantern appearance. This is visual
  preference, not a measured-performance or complete-device acceptance verdict.
  Owner also suspects imperfect glass-pane alignment to the lantern model: record
  for **future** geometry/mounting investigation only, explicitly not a fix now.
  Full High dielectric glass and its evidence remain retained for future use.
- Owner also reports missing S24 hands: explicit visual acceptance blocker. Keep
  accepted player geometry unchanged; isolate actual state/geometry/backend path
  with matched captures. Physical-vs-open timing can describe workload cost but
  cannot certify S24 playability while that discrepancy remains unresolved.
- Next: finish the already-running C2 blocks for each phone, admit owned
  rows/artifact identities once, then capture candidate live interactions/lifecycle
  and matched S24 discrepancy checkpoints after timed
  matrix. Do not repeat valid completed rows or rebuild unchanged artifacts.
