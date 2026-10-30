# Phone ordinary streaming cycle — 16 October 2026

The private independent-page path now has physical phone evidence for **one ordinary unload and reload on both hardware RT backends**: 13.59MiB device-local saving, exact restoration and zero readiness-wait frames. **Smooth gameplay and the complete chapter remain unaccepted.** Both longer full routes failed the unchanged120-second deadline; the focused routes complete but their actual presentation intervals and simulation overruns are poor.

## What changed

Android now exposes the existing snapshot-driven chapter probe through an explicitly opted-in Debug measurement profile. Native code remains optimized (`-O2`); actual selected shaders are **Shipping/Mobile**, with the same saved quality settings. The private page/timing property requires the private source and Shipping/Mobile pair in Gradle and CMake. Existing isolated benchmark admission remains unchanged, and ordinary builds disable this route/timing opt-in.

The chapter scenario adopts an already rendered, ready `chapter-kit-reunion` checkpoint. It exits visual-checkpoint sampling and feeds ordinary axes/context edges to the existing Android wall-delta `GameSimulation.AdvanceFrame`. It does not force traversal poses, readiness, streaming decisions or later phases. Conservative approach inputs accommodate the existing one-to-eight-tick input span; Windows keeps its original default one-tick behavior. A separately named `chapter-streaming-cycle` walks directly from the reunion to the rope, descends, backtracks to the certified tomb passage, returns to the rope and ascends. This shorter test omits the lookout/reunion-raise portion and does not replace full-chapter coverage.

One fixed-size per-slot tag carries the committed serial, route phase/position and scene-owned detail counters. The owning-fence completion joins it to that frame's resource inventory. Successful-present return timestamps are retained separately from actual display timestamps. The existing bounded `VK_GOOGLE_display_timing` collector and strict native/offline joins require exact completed-frame identities, consistent scope/extent/backend and no adverse counter deltas. There are **zero image readbacks or wait-idle captures inside either timed route**; only terminal ownership/timing drain is allowed. Failures preserve ledger, timing and resource records. Chapter exit synchronizes consumed commands and clears generated run intent before returning input ownership.

No game streaming rule, shader, geometry, lighting, quality, capacity or readiness/retirement guard changed. The frozen budget file remains `56936479b2d202dbdb9393ba94b66a1b0bc12d490b700d8af4c63de3460e53bf`. Every packaged asset (169 payloads) matches the preceding candidate. Original torch coverage and its180-second policy are untouched; the separate Android motion deadline remains120seconds and its terminal timing drain2seconds.

## Exact artifacts and settings

| Artifact | SHA-256 | Disposition |
| --- | --- | --- |
| Full-route Debug/O2, Shipping/Mobile APK | `51d7d58b3cbb4d1e9b41297415a0fb33c6515a7f156e235f89e6e205367c1046` | Owner-approved install/pullback; both full routes failed |
| Focused-cycle Debug/O2, Shipping/Mobile APK | `a6b7a00ccd813bc903250079926970f03096ff7436fb4f50c117e86b58b33d76` | Owner-approved install/pullback; both focused cycles passed residency/timing joins; remains installed, stopped |
| Restored default Debug build | `f5af3915ee07479cea15192d752a6ace771da4dc4924e3589c0fd98a6aca0e08` | Four ABI/build/lint/Java verification, not installed |

These are based on1549935d plus the exact source hashes in their candidate receipts, not claimed as pristine later-commit builds. All measured source bytes are archived and verified against those hashes. APKs and preference backups stay private. Both updates use the same development signer/versionCode10 and preserve data; no release app, uninstall, downgrade or data-clearing command was used. The already installed identity was established before replacement rather than repeatedly reinventoried.

Raw device **SM-S948B / Android17 / API37**, Adreno840 / Vulkan1.4.295. Foreground was verified before generated input/captures. Saved settings were identical across runs:40% scale, internal576×1188, swapchain1440×2971, Current shadows, water1/fire0, cap30, glassOff, mistOn, dust1. Raw numeric quality values are retained in the reports. No quality/scale override was applied. Actual Pipeline shader pair:

- Opaque `2b71c86dabdbf5d435ac9c1dd08895db6adf01990ffd9716ce76fe1a25ebd8c2`
- Generic `30ae3c9d21fcb037cd0f2bbf2aa06b3c3c6f7fd61c8a6b81ed2a883a507f3bdc`

Both backend pipeline identities, selected Compute shaders, resource scopes and frame identities are in the raw reports. This changes instrumentation selection from the preceding Diagnostic compatibility check, not optical quality; its timings are not compared as a controlled A/B with that earlier frozen view.

## Full routes — preserved failures

| Observation | Pipeline | QueryCompute |
| --- | ---: | ---: |
| Actual presented observation span | 119.861s | 119.849s |
| Completed frames / matched actual timestamps | 870 /870 | 911 /911 |
| Actual presentation median /max | 133.333 /241.667ms | 129.167 /250.000ms |
| Simulation seconds reached | 80.917 | 83.483 |
| Catch-up overrun count | 607 | 589 |
| Last route phase | lower-sweep | tomb-backtrack |
| Automatic unloads /restores /saving | 0 /0 /0 | 0 /0 /0 |

Both fail with `Motion evidence wall deadline exceeded.` They return from the lookout and descend, but do not reach the certified unload region before the deadline. Their timing collector has no missing matched rows or adverse run counters. These are valid observations of an **incomplete route**, not a pass of the strict complete-route analyzer. Normal frame-delta clamping/overruns remain visible; no deadline or timing assertion was weakened. No cause is assigned to a specific shader, tree family, thermal state or page operation from these observations.

## Focused ordinary residency cycles

| Observation | Pipeline | QueryCompute |
| --- | ---: | ---: |
| Actual presented span | 45.462s | 45.387s |
| Completed frames /exactly matched display timestamps | 521 /521 | 530 /530 |
| Device-local full | 160,831,232B | 160,830,720B |
| Device-local retired | 146,581,568B | 146,581,056B |
| Device-local restored | 160,831,232B | 160,830,720B |
| Saving | **14,249,664B (13.59MiB)** | **14,249,664B (13.59MiB)** |
| Observed device-local peak | 174,307,392B | 174,306,880B |
| Observed peak above starting full | **13,476,160B (12.85MiB)** | **13,476,160B (12.85MiB)** |
| Host-visible full /retired /restored | 48,121,792 /40,880,640 /48,121,792B | 48,121,280 /40,880,128 /48,121,280B |
| Automatic unload /restore /readiness-wait frames | 1 /1 /0 | 1 /1 /0 |
| Actual presentation median /p95 /max | 83.333 /141.667 /166.597ms | 83.333 /141.667 /162.500ms |
| Simulation overrun delta | 139 | 141 |

The scene retires the existing certified **84,219-triangle forest detail owner** (V/I/private primitive storage and BLAS), then reloads it before rope return/ascent requires it. Coarse terrain/world, distant landmarks, shared textures/materials, collision, actors/held items and uncertain secondary-ray contributors remain loaded. CPU sources are cached; this does not prove package I/O or CPU-cache eviction. It is not an entire-tomb unload or a Keeper-only saving.

Pipeline cutovers occur at23.187s (tomb backtrack) and33.600s (rope return); ±1second display-interval windows peak83.333/95.833ms. Query cutovers occur at23.150/33.629s; corresponding maxima83.333/100.000ms. These windows include ordinary scene/camera work; their cost is not isolated traversal/AS/upload cost and the larger peaks elsewhere are not discarded. The full/retired views differ, so no rendering-speed improvement is inferred from their frame times. Peaks are observed owning-frame inventories; the cumulative owner-overlap witness is also retained, not a claim of OS-wide or total GPU-heap high-water measurement.

Terminal checks require one real unload, matching restore, zero saved-owner bytes, complete contributors, no preparation failure and cleared traversal lease. The native probe reaches physical final grounding with equipment released. Resource CSV rows are checked one-to-one against completed-frame inventories and display identities by the archived analysis script. Both strict timing analyzers pass; no missing display rows, scope discontinuities or adverse counter deltas occur.

## Appearance, thermal and acceptance boundaries

The pre-run reunion and post-restoration view were opened. The post-run menu initially obscured the game; after timing ended, its visually identified Resume button was used and the restored graveyard/trees/ground shadows/rope prompt were inspected. This is **not** a matched primary/reflection/transmission gallery or owner visual acceptance. No mid-run screenshot/readback is presented as free instrumentation. Full collision/secondary-ray visual regression remains pending beyond the traversal and retained contributor guards.

Current-HAL post-run samples: Pipeline framework status0, AP40.5°C /BAT34.4°C /SKIN37.3°C; Query status1, AP44.8°C /BAT37.1°C /SKIN39.7°C. Starting states differ (Pipeline AP33.7°C, Query39.7°C; both framework0), so this is not a thermally matched backend comparison. Post-run samples do not establish clocks, power or thermal causality. There is no long-duration smoothness or thermal pass. Actual display intervals are not GPU reciprocal FPS, scanout, photons or touch latency.

## Checks and reproducibility

- Affected native Release2/2 and Debug2/2 pass, including original motion cases and full chapter paths at1/3/4/6 ticks per frame plus the separately named focused5-tick cycle. Failed readiness cannot be fabricated by the scenario; admission leaves the rendered seed unchanged.
- Timing analyzer14/14 pass, including chapter zero-readback data, missing timing owners and adverse counters. Existing nonchapter capture requirements stay unchanged.
- Opted-in Android build/lint: all four ABIs pass. Default build/lint and286 Java tests (45 suites, no skips/failures) pass with the opt-in removed. Default artifact is not installed and differs from its predecessor because native code changed.
- Existing build-policy CTest passes1/1 after the sandbox denied temporary fixture creation; that failed environment run is retained. Initial configuration-argument and forward-declaration build failures are retained and fixed before the approved candidates.
- [Archive and source receipts](receipt.json), [focused results](focused-cycle-results.json), [full failures](full-route-failures.json) and [raw/source packet](phone-streaming-receipts.zip). The packet retains both exact measured source versions, ledgers, timing/CSV observations, strict analyses and relevant build/test/native logs. APKs, preference XML, full UI dumps and unfiltered process logs are excluded.

Original preference bytes match before/after both approved updates. The Debug app is stopped; the release app remains untouched. Default production rendering remains unchanged and the independent page profile is **not adopted**. Exact-commit CI is reported in PR27 after push, without an extra evidence-only commit.

## Next gate

The focused phone residency gate is closed for this one eligible page. The smooth-play/full-route gate is open. Next perform a matched resident-versus-independent-page mobile comparison from the same representative views at these same settings, separating baseline rendering from transition preparation/upload/AS cost. The12.85MiB phone transition overlap needs a measured memory policy; do not transfer the Windows peak or invent a phone budget. Keep the short-cycle and failed long-route evidence distinct. Broader useful coverage, source I/O/cache lifetime, lifecycle/pressure during real transitions, Current/Higher and saved Lower migration, full chapter visuals/audio/haptics/touch and longer sustained play remain pending. No quality/capacity reduction, merge or release.

![Pre-run reunion outside timing](reunion-before-full-route.png)

![Inspected Query restored view after timing and Resume](query-restored-view.png)
