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
- Historical next step before C2 completed: finish the already-running C2 blocks for each phone, admit owned
  rows/artifact identities once, then capture candidate live interactions/lifecycle
  and matched S24 discrepancy checkpoints after timed
  matrix. Do not repeat valid completed rows or rebuild unchanged artifacts.
- Subsequent checkpoint: all24 physical/candidate runs complete; all18 P1/P2/C2
  reports pass the extended admission once. Owner requests S24 returned to spouse.
  Matrix invoked its Home cleanup; subsequent scoped stop attempt found S24
  already disconnected, so do not claim that stop succeeded. No data cleared/uninstall.
  **Do not launch further S24 checks without renewed availability.** Missing hands
  and starting-enemy visibility remain open; water comparison is superseded above.
  All24 curated reports are now archived and pushed at6d29d8b. S26 normal Debug
  replay/capture/lifecycle results and the next unfinished step follow below;
  do not rerun the timed matrix.

## Completed matrix: descriptive results, not30FPS acceptance

C1/P1/P2/C2 are respectively physical control, open candidate twice, late physical
control. Every route has1838 admitted rows; the opening anchor below is its160
opening rows. Held-high and live each have600. Values are median/p95 milliseconds.
The derived window view binds hashes of all24 original benchmark and analysis
files; existing receipts are not overwritten or relabelled as a fresh admission.

Native render-entry-through-present cycle (not measured display pacing):

| Exact device / window | C1 | P1 | P2 | C2 |
| --- | --- | --- | --- | --- |
| S26 opening | 82.70/91.51 | 81.44/90.72 | 86.21/96.78 | 89.72/97.12 |
| S26 held-high | 235.87/253.26 | 112.03/115.75 | 114.23/123.83 | 272.80/283.19 |
| S26 live reveal | 236.44/249.49 | 114.88/123.45 | 122.50/130.84 | 278.82/283.60 |
| S24 opening | 58.70/64.95 | 58.69/64.81 | 62.43/66.79 | 64.00/71.47 |
| S24 held-high | 265.33/267.56 | 84.17/85.45 | 84.47/88.91 | 346.32/350.49 |
| S24 live reveal | 279.71/325.54 | 100.45/104.79 | 109.65/121.10 | 354.88/362.07 |

Total RT GPU command-buffer duration (AS/update/trace/copy, not shader-only):

| Exact device / window | C1 | P1 | P2 | C2 |
| --- | --- | --- | --- | --- |
| S26 opening | 72.19/79.99 | 70.90/80.10 | 75.96/85.88 | 78.64/86.39 |
| S26 held-high | 234.09/251.43 | 110.66/113.93 | 112.55/122.38 | 271.09/281.35 |
| S26 live reveal | 225.53/238.39 | 104.12/112.84 | 112.07/120.40 | 268.39/272.14 |
| S24 opening | 45.84/50.70 | 45.86/50.84 | 48.38/53.34 | 50.69/55.85 |
| S24 held-high | 263.27/265.40 | 82.26/83.32 | 82.50/86.79 | 344.33/348.44 |
| S24 live reveal | 264.46/311.71 | 86.89/91.61 | 97.56/106.99 | 343.83/348.67 |

Compared with late C2, S26 held-high cycle medians fall58.1-58.9% and live56.1-58.8%;
S24 held-high75.6-75.7% and live69.1-71.7%. These describe the **whole quality-profile
treatment**, including the intended automatic GenericDielectric-to-OpaqueFast
selection after the last active transmission primitive disappears. No strategy
is forced and no shader changed; this is not an isolated geometry-throughput saving.
Opening uses OpaqueFast in every cohort; its candidate medians differ from C1 by
approximately-1.5/+4.3% on S26 and0/+6.4% on S24. Small opening differences cannot
be called a profile optimisation or used to explain ordinary lighting cost.

Temperature/order qualify the large saving, not erase it: S26 C1 begins29.4C and
ends42.4C across its workloads; later P1/P2/C2 are41.8-44.0C, reaching thermal3.
S24 C1 begins27.5C and ends37.9C; later blocks are37.9-41.2C, reaching thermal2.
Observed GPU thermal power levels reach7 on S26 and10 on S24. Context samples are
not aligned to individual frames. Never pool the phones' different extents/backends
or turn C1's cooler measurements into warm matched evidence.

**Decision:** retain the authorised profile as a development quality choice with
substantial heavy-workload benefit; **NO-GO for a comfortable30FPS claim**. Candidate
heavy native-cycle reciprocals are about8.2-8.9FPS on S26 and9.1-11.9FPS on S24,
not actual display FPS. S26's104-113ms heavy GPU medians are still roughly80-91ms
above the estimated22-24ms GPU allowance. Opening remains71-76ms GPU on S26.
CPU skinning is separately material but cannot account for the large GPU gap.
No further micro-optimisation, quality reduction or architecture promotion follows
automatically. Owner perceived playability and actual display pacing remain open.

RAM boundary drops remain invalid active-memory comparisons because completion
destroys the scene. A separate **active S26 Diagnostic replay** snapshot at waypoint10
(red-torch-bay) records PSS521032KiB/RSS627096KiB, native heap PSS175300KiB,
Graphics283512KiB and system memory PSI avg10=0.00/avg60=0.02. Native resource
inventory is45 buffers/55 allocations/17BLAS/1TLAS/21instances, host-visible9815360B
and device-local76405248B. This one snapshot is not sustained RAM-pressure/peak
memory acceptance and not Shipping A/B. GPU bandwidth/cache/stalls/registers/spills/
occupancy remain unavailable counter gaps; PSI is not GPU stall evidence.

## S26 exact Debug containment and current next step

Frozen Debug APK e10b0203 is reused, not rebuilt at the evidence-only6d29 head.
Run191403 on exactSM-S948B at75%/Diagnostic/Mobile pipeline passes strict ASTC,
13-waypoint replay,7 captures, modelled route/primary ownership and Home/resume
with renewed honest RT presentation; summary has zero warnings/failures.
First run190159 reached waypoint8 before the120s deadline, then harness finally
force-stopped the app. It is retained **incomplete**, not a crash or successful
test. Specific validity problem justified one retry with the existing300s option;
no completed Shipping timing run was repeated. The second replay completes in
about132s from its begin marker, explaining why the earlier deadline was too short.

Lead inspected opening, two-enemy, held-high/low, low parry and mirror images;
forward is byte-identical to opening. Hands/grips, open cage/flame and sharp
cage/player floor shadows are visible; no pane pixels are restored by approximation.
Opening state has1 active skinned enemy, two-enemy state has2. The two-enemy camera
does not provide a clean front-on view of both, so counts are not two distinct
visible silhouettes or a diagnosis of S24. Nested completed-frame evidence records
available/positive primary-player counts; neither that nor frozen parry proves
continuous live transitions. No accepted player tuning was changed.

Current6d29 source push36840516701 and PR36840521652 are green: GCC55/55,
Clang55/55, MSVC56/56 and focused Vulkan CPU-host15/15 each. Actual eight job logs
inspected; compile/host checks are not physical presentation or performance tests.

**Next unfinished step:** S26 unfrozen pickup/high-low/walk-look/attack-parry,
geometric lighting/shadows, pause/resize and display-pacing checks on the normal
candidate; then independent remaining programme work. S24 hands/enemy visuals
await renewed availability, without another timed matrix. High physical transport/
backend-parity and final-candidate gates remain open. Audio/haptic manual:NO for
this geometry/evidence slice; accepted music is unchanged. No merge/publication.
