# Keeper two-torch activation — 17 October 2026

**The reported dip is reproduced on both phone RT backends. It begins on the first completed frame uploading both Keeper torches and persists through the reveal. Shader dispatch is the dominant increase; this is not an allocation or AS-update hitch.** No rendering optimization is adopted by this checkpoint.

## Measurement

The new private `keeper-torch-transition` input probe observes the already admitted, live authored `rescue-journey-start` chapter. It follows the collision-resolved route and real automatic torch drench, holds for six simulation seconds immediately before the Keeper arrival plane, then crosses with ordinary movement. The real reveal event activates sources 3 and 4. It holds the view until the normal six-second reveal completes, before staff light or lightning damage. Tutorial timing and transit damage are disabled for this probe; this is not a combat or touch-latency test. No encounter phase/light import, authoritative pose input, delta override, fabricated readiness, shader alteration, or mid-run image readback is used in either timing run. Existing 30-second arming, 120-second run and two-second presentation-tail limits remain unchanged.

The historical `keeper-first-entry` scenario imports the legacy showcase; the frozen `lich` view begins after activation. Neither establishes this transition. The September opening-fire isolation used a different room, one torch, different shaders/settings and thermally confounded effect omissions. It was inspected, not repeated or promoted to a production fix.

The existing completed-frame ledger supplies exact uploaded fire prefixes, simulation states/events, shader identities and resource ownership. The private GPU timestamps bracket dynamic AS update, complete shader dispatch and post-barrier transfer. Every stage row joins its own successful submission, fence and simulation tick, including the final frame. The unchanged strict display analyzer additionally requires a complete one-to-one join to actual Android image-presentation timestamps and clean collector deltas. GPU intervals are not additive work counters or playable FPS.

## Results

Both runs retain **40% scale, 576×1188 internal / 1440×2971 swapchain, CURRENT shadows, cap30, water1/fire0, glass Off, mist On, dust1, Shipping/Mobile shaders**. SM-S948B, API37/Android17, Adreno840, Vulkan1.4.295, driver2150932499; optimized native Debug/O2, not signed Release. The active Opaque shader is identical between the two held views on each backend.

Settled samples discard the first second of each hold by actual display time. Each run has 64 unlit and 50 lit settled samples. The entire ordinary approach is retained in the raw ledger, not silently discarded from the strict timing join.

| Backend / metric | Before activation | During lit reveal |
| --- | ---: | ---: |
| Pipeline dispatch median / p95 | 66.304 / 67.326 ms | 104.590 / 106.625 ms |
| Pipeline actual display interval median / p95 | 75.000 / 83.333 ms | 116.667 / 120.833 ms |
| Pipeline AS interval median | 0.364 ms | 0.434 ms |
| QueryCompute dispatch median / p95 | 67.056 / 67.765 ms | 109.951 / 111.549 ms |
| QueryCompute actual display interval median / p95 | 75.000 / 83.333 ms | 120.833 / 125.000 ms |
| QueryCompute AS interval median | 0.359 ms | 0.429 ms |

Dispatch rises **57.7% / 64.0%** descriptively within the Pipeline / QueryCompute runs. The first lit submissions are557 /553: their dispatch times are104.771 /109.850ms, following65.587 /66.683ms. Their AS intervals are0.348 /0.343ms. There is no first-frame AS spike followed by recovery; dispatch remains expensive. The Keeper's recorded position, tilt and facing are unchanged on that first lit frame, and staff strength/damage pulse remain zero throughout. The unlit screenshot also already shows the Keeper model.

Each held view retains166,119,232 device-local and49,444,864 host-visible accounted bytes,28 TLAS instances and21 BLAS; no static-detail unload, restore or readiness wait occurs. The cumulative player skin-update counter advances once per completed frame in both holds; it is not an actor CPU-duration measurement. Settled unlit frames advance4–5 fixed ticks without catch-up overruns. All50 settled lit frames advance the maximum6 ticks and each adds one overrun. This records lost simulation catch-up headroom without changing the shared60Hz authority or its existing cap.

The simulation sources request strength1. Actual first Pipeline uploaded strengths are approximately0.871253 and0.883601, not a claimed constant1: IDs3/4 at `(-35.5,0.99,-16.325)` / `(-35.5,0.99,-14.025)`. The full per-frame packed colours/strengths and the single `KeeperRevealStarted` event are retained. No light disappears during the measured lit hold.

Framework thermal status is0 before/after both runs. Current-HAL AP/BAT/SKIN readings are Pipeline40.0/27.5/32.5→38.8/29.7/33.8°C and Query41.9/29.3/34.3→40.5/31.1/35.2°C. These are boundary observations, not controlled GPU clock/power evidence or temperatures at every frame. Do not use the two runs to rank backends causally.

### Attribution limits and next step

The view is held within each sampled phase, but ordinary arrival moves the player origin **6.33cm**, from `(-30.4475,-0.95,-15.2)` to `(-30.5108,-0.95,-15.2)`, with rendered yaw−1.5708/pitch0 in both. These triples use **player support Y**, not optical eye Y: the standing eye is0.70m,1.65m above the−0.95m support. CSV X/Z identify the player origin; the view yaw/pitch come from the submitted frame. This crosses the existing zone/arrival boundary and co-activates reveal presentation. It is not an identical-camera light-only counterfactual. The results strongly support investigating the added two-light shader workload; they do not apportion its cost between surface visibility, fire-volume transport, mist/dust scattering, or every other shader dependency.

The recommended next bounded step is to attribute those added shader paths at this actual reveal workload, then choose a reusable, output-preserving correction. Reuse the existing experiments and avoid repeating broad unoccluded-light omissions as a proposed fix. Baseline dispatch was already about66–67ms before the torches, so optimizing their increment alone cannot establish smooth gameplay. No lower quality, removed legitimate shadows/mist, inverse-transform adoption, increased budget, AS/descriptor redesign or scene-specific light exception is justified by this result. Full fight, sustained warm gameplay, lifecycle/owner acceptance and the wider chapter remain open; issue28 stays deferred.

## Inspected visual witness

These are unmodified foreground phone screenshots from a **separate** Pipeline run on the same APK. Both were opened and inspected: Keeper and torch bodies exist before activation; the second image shows warm room/floor illumination and shadows with the same heading after the small arrival displacement. They are OS screenshots, not exact-token RT image readbacks. Completed-phase CSV witnesses before/after each screenshot are archived. All screenshot runs are excluded from the timing table.

| Before arrival | Lit reveal |
| --- | --- |
| <img src="torch-baseline.png" width="240" alt="Unlit Keeper chamber before arrival"> | <img src="torch-lit.png" width="240" alt="Keeper chamber after both torches activate"> |

The first visual attempt missed the baseline witness because its milestone log watched only `ObserveAdvance`, while baseline entry occurs in `BuildInput`. Its logs remain archived; it is not a successful before/after capture. The follow-up observes completed CSV phases for screenshots and supplies both images. This did not change native/rendering code or the timing runs.

## Artifact custody and checks

APK **`6d43d5dc3fa9325a7be4b85ffc3521f536e5ad41d63db7b7ac71ae5ff3d625e8`**,183,656,522 bytes. Source is4cb9145747e5441d787294dd99d57d3f8298ede6 plus the archived source patch/native-file hashes in [build receipt](build-receipt.json). The analyzer was subsequently strengthened to reject a pipeline switch and report catch-up headroom; no APK-native code changed after the measured build. Private stage profiling uses the already-working r10 region-page shader profile, not the inverse-transform treatment or sparse-clock shaders. Its historical source pin is2147e26b; all four module hashes match the original installed control and are retained in the reports. This is not a pristine production APK or a reset to that historical source.

Installed SHA256 matches the candidate. All169 APK asset payloads match the backed-up original. Development signer `2bc40608…f5566736`, package/versionCode10 match; no uninstall/data clearing. The exact original `f13dda30…fa527` is restored, preferences byte-identical and Debug stopped; release app untouched. [Install/restore receipt](install-restore.json). Frozen budget file SHA256 remains `56936479b2d202dbdb9393ba94b66a1b0bc12d490b700d8af4c63de3460e53bf`; production shaders, materials, tomb/niches and loading rules are unchanged.

Passed locally: native Debug and Release focused scenario/policy CTests; live probe at15/30/60/120 update rates, rejection of legacy seeding, ordinary collision traversal, real two-source activation and pause/readiness freeze; eight attribution analyzer tests,15 strict display tests and five existing stage-join tests; four-ABI Android Debug build and lint. Early host failures at the inner corridor turn are preserved: the waypoint lay on a collision boundary; moving the input target inside the existing walkable corridor fixed it. No collider, assertion or deadline was weakened.

Pipeline608 and Query604 completed frames all pass strict display/stage joins (1,212total). [Pipeline analysis](pipeline-timing-analysis.json), [Query analysis](query-timing-analysis.json), [raw reports, CSV, thermal/prefs, runner, failure evidence and source patch](raw-evidence.zip), [file hashes](files.json). Exact-commit CI is recorded on PR27 after push; it is distinct from the private measured APK. No merge or release.
