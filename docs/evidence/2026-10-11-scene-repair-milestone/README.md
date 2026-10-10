# Scene repair milestone — 11 October 2026

This checkpoint preserves the ongoing tomb-to-woodland goal. It is **not a
completed chapter** or a new owner/device acceptance. Exact artifacts, current
shader metrics and test outcomes are in [receipt.json](receipt.json).

## Implemented repair batch

- Close the entry portal underside with geometry derived from the existing
  overhead collision solid. Exclude exterior terrain from all nine occupied
  tomb volumes, including thin diagonal crossings. The owner's later report
  confirms the underside/interior intrusion improvements; the rear dark outline
  remains an open visual observation.
- Retain both waterfall skeleton/corpse instances while independently rendering
  the Keeper in the approved third lane. Reuse the existing two skeleton pose
  BLAS resources and Lich BLAS; preserve viewmodel and Keeper torch ownership.
  Seed a finite, masked third owner at initial TLAS construction as well as in
  dynamic frames. Keeper disappearance and subsequent Kit ownership are **not**
  implemented by this batch.
- Admit the original pine/alder LOD1 derivatives with preserved masters and
  tangent/provenance receipts. Append only the four approved texture families
  within 18 layers. Remove the duplicate demonstration-tree boxes from the
  rescue scene and its collision roster. Move imported trunks off the actual
  route bends; the complete simulated rope/route/backtracking test passes.
- Coalesce adjacent world-baked geometry of identical material without changing
  vertex bytes or expanded triangle order, and share byte-identical authored
  world material records. This resolves the reproduced 32-primitive/material
  startup admission failures without increasing either ceiling.
- Use compact per-pixel Windows captions and input-aware, per-line Android Auto
  placement. Preserve explicit positions and font sizing. Scoped owner approval
  of captions/niches belongs to the earlier executable in the owner receipt.
- Fix Windows dialogue paths joining `assets` twice. All 13 Kit and both Keeper
  speech assets now resolve from the actual asset root. The independent saved
  slider is labelled **Dialogue / Speech** on both platforms; fresh listening
  remains pending.
- Fix the three-heart HUD width at fractional DPI: at 125%, separate inset
  rounding previously wrapped the third heart outside a one-row control. Keep
  gameplay vitality unchanged. Actual clean-start HUD inspection remains open.

## Inspected RT images

Both images use the same executable, frozen checkpoint
`waterfall-guards-walk-early`, camera `(-3.45, -15.2)`, yaw `-1.570796`, pitch
`-0.06`, and 960 × 540 capture output at 100% scale. These are the established
capture dimensions, not a performance-driven quality reduction. Captures are
scene-only, exclude UI, and were launched without foreground activation or input.

| Pipeline Diagnostic/High | QueryCompute Diagnostic/High |
|---|---|
| ![Pipeline guards](pipeline-waterfall/154-waterfall-guards-walk-early.png) | ![Query guards](query-waterfall/154-waterfall-guards-walk-early.png) |

The actual selected backends and shader identities are recorded in each
sanitized manifest. Both completed RT storage readback/presentation. Images differ
at 25 of 518,400 pixels by at most one 8-bit channel value. Inspection confirms
both guards render in this view. A single sampled walking pose does **not** prove
the owner's live idle-motion observation resolved, three-actor simultaneous
visibility in every transition, or either phone backend.

Short capture GPU timings are not sustained performance evidence. Full matched
portal/gallery/Keeper views, outdoor panorama, visible Kit, return/second ascent,
and physical lifecycle/audio/haptic acceptance remain required.

## Validation and retained failures

Windows Debug build and 18 affected tests completed without assertion failures.
The original torch test took **185.93 seconds in Debug**, outside its 180-second
policy; this does not qualify as a torch gate. Its unchanged Release test then
passed in **17.16 seconds with `--timeout 180` enforced**. Five other focused
Release tests passed. Android Debug built all four configured ABIs; 283 Java
tests in 43 suites and lint passed. Exact APK world/audio payload policy passed.

The first shader tests rejected old compatibility hash witnesses. Fresh
compilation reproduced all eight current Pipeline artifacts within every
unchanged frozen limit; the QueryCompute shader test also passed. Only exact
hash witnesses were updated, then manifest/artifact negative suites passed.
Their original failures remain recorded in the receipt and ignored local logs.

The PR remains draft with base conflicts. This milestone does not merge main,
rewrite history, change released 1.6.2 artifacts or relax shader budgets.

### Exact-commit CI follow-up

[Run 38094375607](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/38094375607)
on `2dd45a50b16a0d5bc2746c31b39d4f52c28099dd` passed Android Debug,
Vulkan host and selected sanitizers. GCC and Clang each passed 114/115 tests;
MSVC passed 123/126. The retained failures exposed two integration mistakes:

- The subtitle fixture configured the Windows manifest a second time, violating
  the existing single-template version contract. It now shares the one generated
  manifest with the game, including in non-Vulkan Windows test builds.
- Windows auto-CRLF checkout changed hash-pinned forest package text and its
  runtime manifest. Scoped attributes now preserve original source bytes and
  canonical LF runtime JSON. No expected hashes or asset assertions changed.

Local correction checks: unchanged version contract PASS (0.55 s), full asset
policy PASS (126.65 s), native subtitle bitmap fixture PASS, forest source/texel
checks PASS. A separate auto-CRLF index export verified exact hashes for all 27
archived/runtime text payloads in that selection. The first export also included
new derivative receipts with local CRLF, which are not original archive bytes;
their explicit canonical-LF rule avoids changing the source package policy.
Remote correction CI must be verified on the subsequent commit; these local
results do not retroactively turn the failed run green.

## Next chapter work

The new topo-derived Blender masters remain separate source candidates. Native
import and surface correspondence have passed for r03, but runtime admission,
final support/collision, woodland composition and actual RT inspection are
unfinished. The current world geometry uses shared resident GPU buffers;
CPU zone readiness is not streaming. Safe two-way zone resource handoff, visible
Kit with full admitted materials, Keeper disappearance, and the fresh lantern
raise reunion remain required. No placeholder or missing physical evidence is
treated as completion. Mist issue28 stays deferred.
