# Bounded Android RT render-scale resize

Local October 1 evidence; not a release, glass correctness, FPS or owner-feel pass.
Implementation `54057689872102f1bd2c2735f8b127a1e3686d29`; focused CI addition
`b74559cf77248b8bcc9c46c9c002d898d920a91c`. Accepted players, lighting, materials,
ray work, traversal budgets, gameplay and feedback authority are unchanged.

## Proven cause and change

The old Android Settings path waited for device idle, destroyed the full RT scene
and initialized it again. Actual Settings changes reproduce a 13.28–14.24-second
request-to-first-RT-presentation pause without changing process or surface thread.
The replacement retains resolution-independent geometry, textures, BLAS/TLAS,
pipelines, SBT, descriptors and simulation state. It allocates/transitions a new
output image, updates storage-image binding 1, publishes the new extent/compute
groups and retires the old output. Whole-device idle remains intentional; this
does not increase frames in flight. The next acquired command buffer is reset
and re-recorded before submission.

Allocation/preflight failure preserves old output ownership. Android reports a
runtime error and stops on failure; this is fail-closed, not a seamless fallback
to the old scale. Temporary peak memory includes both output images. Genuine
surface/swapchain recreation and Home/resume still use the existing full path.
Windows uses the common allocation helper, but its scale-change integration is
not converted to output-only resize in this slice.

Capture readiness restarts at the new extent. A newly allocated output cannot be
read back before a frame is recorded; stale output colour-swap metadata is cleared.
Capture callers must submit a successfully recorded frame before readback.

## Exact artifacts and method

SM-S948B/R5GL219SZGK, Android16, Adreno840/driver2150932499; no external cooling.
Default test scale75%/1080x2235, swapchain1440x2980. Actual observed Settings slider
sequence is 75→100→75→50→75, ending at the owner's normal75%. SFX/look/quality
settings, app data and the stable installed app were not changed. Optional
Play Protect upload was declined without changing global security settings.

| Artifact | SHA-256 | Role |
| --- | --- | --- |
| Normal Debug | `389d6954f7a3fddde1ced690470029fa4b14cc6f825fa34bf7269dc617f946e0` | Old full-scene resize control |
| Prototype Debug | `8da43d5c635802479af2ad2b87f8d0f8971f9d3127cf499461e94868295f23d0` | Initial output-only comparison, before capture-validity follow-up |
| Final Debug | `c11ff703794d74dbfb75dc7a0186af0f6c5c416e8383755a5cd8f84310daf0fe` | Reviewed implementation and capture-validity fixes |

Final APK was built from113200e plus the retained source patch, before committing
5405768. The source receipt independently checks all five affected Git blobs
against5405768. Harness `sourceCommit` identifies the checkout at collection time,
not a claim that a reused APK was rebuilt there. No APK is committed here.
All four embedded SPIR-V modules exactly match the normal Debug APK. Of53 assets,
only ASSET_LICENSES.md changes (owner-authorized music-prototype provenance); no
render payload changes. Prototype lacks a separately frozen source patch; final
artifact provenance is authoritative for the accepted code.

Version1 measurement records device-shell time immediately before the real touch
and the first successful RT swapchain log afterwards. It includes input transport,
350ms debounce, idle/allocation and presentation; not compositor/touchscreen latency.
Each run rejects a changed process or surface restart. Version2 additionally joins
before/after completed-frame packets to newer epochs, the selected backend and
requested extent; that report wait is outside the timed interval. These are Debug
UI-pause measurements, not Shipping FPS or frame-pacing measurements.

## Results

All rows use the same four ordered settings changes; medians span all four, not
only returns to75%. Raw battery/thermal snapshots accompany each transition.

| Run | Minimum / median / maximum request→present ms |
| --- | --- |
| Control A1 | 13282.45 / 13795.82 / 14063.72 |
| Prototype B1 | 431.68 / 490.26 / 539.24 |
| Prototype B2, same process | 414.29 / 452.28 / 530.78 |
| Restored control A2 | 13326.31 / 13828.14 / 14238.26 |
| Final pipeline C1 | 430.16 / 489.19 / 585.64 |
| Final required-compute | 427.61 / 477.69 / 554.12 |
| Final pipeline, owning-packet checks | 414.11 / 462.36 / 562.26 |
| Final compute, owning-packet checks | 436.33 / 480.65 / 500.11 |

The eight version2 transitions have successful new-epoch, same-backend presented
packets with frame-owned41-counter diagnostics still available. Reported buffer,
allocation, pipeline/descriptor/SBT and instance counts,17BLAS/1TLAS and host-visible
bytes remain stable. Only output-dependent device-local bytes vary. These aggregate
counts do not prove individual Vulkan handle identity; transaction tests verify
that ownership bookkeeping separately.

Separate `control-389d6954-a1/a2` runs under `lifecycle-contaminated/` used an
Activity intent and restarted the surface. They are retained failures of the
measurement method and excluded. **Do not confuse them with `settings-control-*`:**
the latter keep one process/thread and exactly one surface-start marker; their
full RT-scene rebuild is the intended control treatment, not contamination.

## Current validation and limits

- MSVC Debug4/4 and Release4/4 focused tests PASS: frame evidence, initialization
  preflight, scene resource inventory/resize transaction and character/render smoke.
  Failure injection checks partial allocation cleanup, old-output retention, true
  no-op and descriptor→publish→retire ordering, including non-null texture views
  and sampler ownership. Unwritten output capture is rejected before Vulkan work.
- ARM64 Debug build PASS. Exact APK pullback, strict ASTC and native presentation
  PASS. Opening/Home-resume020500 and full13-waypoint replay/opening/Home-resume022603
  PASS. Both final opening PNGs match control SHA
  `d3ec98a838e61194c340343ae4e293aea0ce8b39697962dbac17e134c62dbbcf` exactly.
  These opening captures exercise initialization/readback, not in-place resize.
- Actual final Diagnostic APK containment: all four normal Mobile modules pass
  SPIR-V validation/disassembly; Diagnostic atomics are expected. No shader edit
  or experimental compiler treatment is included.
- ARM64 unsigned Shipping build PASS; exact APK SHA
  `d501c394b8bcbf7c8da91870104905356633a7f0a9a1b38b09b4134097660baa`.
  Actual packaged four Mobile modules pass val/dis and contain zero atomics and
  no diagnostic binding22. Not installed, signed or published; not Shipping
  device-resize/performance acceptance.
- Windows RTX5050 Laptop GPU: native Debug High pipeline13/13 and required compute
  13/13 standard captures/manifest validation PASS. Nonblack sampling and zero
  recorded transport-overflow/rejection diagnostics are not visual acceptance,
  dedicated glass validation, backend pixel parity or Windows resize coverage.
- Fresh b74559c push36743947124 and PR36743955283 both PASS:46portable plus13
  Vulkan CPU-host tests,10APK resolver cases and existing offline player/Android
  contracts. Actual logs inspected. CPU-host CI does not create a physical RT device.

Remaining: Shipping/device final-candidate matrix, lantern-heavy/live-gameplay
resize stress and resource-failure/lifecycle stress where warranted. This fixes
the demonstrated opening/settings pause; it does not improve ordinary GPU frame
time or close glass budget/recovery, pixel attribution, backend parity or30FPS.
S24/S25 remain unverified. Audio/haptic manual revalidation required: **NO**;
listener, events, transport, cues, gain and playback are unchanged.
