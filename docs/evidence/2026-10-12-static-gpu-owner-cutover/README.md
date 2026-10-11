# Actual static GPU owner cutover

Parent source is `8e37190f4a30d3ef5653ef2cb096eaf688cd4db9` in the existing
PR27 worktree. This milestone exercises actual Vulkan resource replacement;
ordinary gameplay still starts fully resident and does not evict zones.

## Implemented transaction

Loaded assets are packed without reopening files or reloading actors. Six new
GPU buffers, a replacement collapse BLAS, TLAS and update scratch are prepared
offside. Shared texture arrays, material records, player world/viewmodel buffers
and other BLAS remain owned. Reuse checks compare each unchanged owner's local
geometry/index/transform/material mapping at its old and new metadata bases.
Metadata-row relocation is safe because the two metadata buffers are adopted
together and retained BLAS geometry indices are owner-local. An initial review
concern about base relocation was rechecked and disproved; emitter/asset/reserved
instance identities are also verified.

The Windows diagnostic proves real completion, collects completed diagnostics,
consumes outstanding acquisition and resets **all** recorded frame command
buffers before publication. Failed proof or a stale resource generation keeps
both owners. Descriptor preparation failure/exception rolls back all exchanged
owners before cache/generation changes. Publication precedes retirement of the
old TLAS, BLAS and buffers. Mapped metadata/TLAS ownership transfers explicitly.
Kit pose and droplet caches force uploads/refits before the next trace. A failed
queue-idle wait after successful candidate submission retains potentially live
resources and its command buffer for fatal platform retirement; it is not an
ordinary rollback. Scene moves preserve the single candidate owner.

The focused fixture tests exact publication/retirement order, consumed and stale
candidates, invalid/overflow generations, incomplete candidates, failed/stale
proof, pending unsubmitted definitions, missing callbacks, descriptor rejection
and exception, retry, cache/mapping ownership and scene move. Two retained
negative mutations fail the cache/publication assertions. These are deliberate
fault-injection coverage, not owner-reproduced failures. The first descriptor
negative build failed on a test-edit typo; corrected red and green results are
retained. Two invalid shader-check invocations are retained; the final explicit
absolute-path catalog check passed without modifying production artifacts.

## Actual RT evidence and limits

On RTX 5050 Laptop GPU, 960×540 at 100%, Diagnostic High, HIGHER shadows, six
frozen captures each perform forward replacement and restoration. Pipeline and
QueryCompute both cover the forest bend and visible Kit. Pipeline also covers
the reverse portal and planted rope pull-up. All twelve full-frame comparisons
are byte-identical. Actual PNGs were opened: portal stone/underside remains
present, Kit/equipment remain attached, the forest bend remains unchanged, and
the tightly framed dark pull-up matches its accepted source. This is a targeted
regression gallery, not full chapter visual/feel acceptance.

| View | Before | Replacement | Restored |
| --- | --- | --- | --- |
| Forest / Pipeline | [PNG](bend-pipeline-final/cutover-00-before.png) | [PNG](bend-pipeline-final/cutover-01-selected.png) | [PNG](bend-pipeline-final/cutover-02-restored.png) |
| Forest / QueryCompute | [PNG](bend-query-final/cutover-00-before.png) | [PNG](bend-query-final/cutover-01-selected.png) | [PNG](bend-query-final/cutover-02-restored.png) |
| Kit / Pipeline | [PNG](kit-pipeline-final/cutover-00-before.png) | [PNG](kit-pipeline-final/cutover-01-selected.png) | [PNG](kit-pipeline-final/cutover-02-restored.png) |
| Kit / QueryCompute | [PNG](kit-query-final/cutover-00-before.png) | [PNG](kit-query-final/cutover-01-selected.png) | [PNG](kit-query-final/cutover-02-restored.png) |
| Portal / Pipeline | [PNG](portal-pipeline-final/cutover-00-before.png) | [PNG](portal-pipeline-final/cutover-01-selected.png) | [PNG](portal-pipeline-final/cutover-02-restored.png) |
| Pull-up / Pipeline | [PNG](stake-pipeline-final/cutover-00-before.png) | [PNG](stake-pipeline-final/cutover-01-selected.png) | [PNG](stake-pipeline-final/cutover-02-restored.png) |

The forest diagnostic retains common families 0/1 and forest families 5..10,
omitting only three tomb ornament families. The other views clone the complete
owner. This is explicitly **not** ordinary eviction eligibility: common masonry
still includes tomb surfaces and every shared texture remains resident. The
forest step reduces actual device-local allocation from 396,691,232 to
395,027,440 bytes (1,663,792 bytes, 1.587 MiB), then restores it exactly. The
TLAS owner count goes 1 → 2 during preparation → 1 after commit. At the prepared
boundary device-local residency reaches 455,511,024 bytes; the complete-owner
clones reach 457,174,816. Replaced-owner peak overlap includes temporary collapse
build scratch, 138,000,720–140,347,264 bytes. Host-visible/device-local categories
can overlap and must not be added. These are actual allocation sizes, not asset
payload estimates. Other scene owners are reported separately in each manifest.

Debug synchronous preparation took approximately 57–110 ms across the final
views; cutover including retirement proof took approximately 3–10 ms. Some host
checks ran concurrently. These finite frozen captures are not sustained frame
or phone-performance acceptance. This synchronous method is too coarse to claim
seamless ordinary travel. Next implementation must prepare incrementally, bind
zone readiness to actual owned generations/availability, and establish retained
shaft/light/secondary-ray contributors and eligible per-region geometry before
activating two-way gameplay cutovers. The current generic readiness adapter is
not suitable for evicted zones and is **not** presented as solved here.

## Validation and exact artifacts

- Windows Debug/Release application builds passed; final focused CTests passed
  **9/9** in 117.67 s / 15.84 s.
- Android all four Debug native ABIs built and assemble passed. Unchanged local
  Java and lint tasks were `UP-TO-DATE`; fresh CI execution remains distinct.
  Both runtime package contracts passed. No installation or device input.
- Fresh eight-key shader catalog compilation/structural budget checks passed.
  The budget SHA remains `56936479b2d202dbdb9393ba94b66a1b0bc12d490b700d8af4c63de3460e53bf`.
- Three actor slots, 18 texture layers, all shader ceilings and original torch
  coverage/180-second deadline are unchanged. The original torch test was not
  repeated for this input-independent diagnostic; retained prior evidence is
  distinct from the nine current focused tests.
- The protected untracked Blender backup hash remains
  `0dae1e8fb803336cef2d045f48c5f4c9195a6463f45161cb0b6c7434350769fb`.
  This does not claim recovery of the historical overwritten bytes.
- Exact source-file/binary/shader/frame identities are in
  [validation-receipt.json](validation-receipt.json) and adjacent manifests.

| Artifact | SHA-256 |
| --- | --- |
| Windows Debug | `25ef2b51246f309095b5f3e8ff37dfdbeca5bf717878e2bd184972bf2628ef4c` |
| Windows Release | `67ec5dd1ea0617e5d353700c3e4871c0e42a1eacd7f9c4e757696c7c1a74a28e` |
| Android Debug APK | `60ff7d907f9a41d323946df12d538aa9ecd8f16392b4f63597fed578d16219fe` |

The new fixture is built/selected explicitly in Vulkan-host CI alongside all
seven requested traversal CTests. Parent `8e37190f` passed all six jobs, including
Unicode insertion in 281 ms. Fresh pushed-commit results belong to that exact
commit's run, retained current receipt and PR summary. PR27 remains draft with
base conflicts; no main merge/history rewrite was performed.

## Continuing chapter and owner checks

Chapter remains incomplete: ordinary two-way loading/retirement, graveyard/lid
composition, remaining prepared woodland dressing, full route gallery and
sustained/physical acceptance are still required. Mist28 and replacement canopy
remain deferred. No limits or required effects were reduced.

For the next owner candidate: defeat Keeper, claim lantern, ascend, freshly raise
it for visible/speaking Kit, follow both bends to lookout, return/descent and
repeat; check attached equipment/light, corpses/backtracking and pause/resume.
Phone checks still need authorized installation of exact new bytes, both RT
backends where supported, CURRENT/HIGHER and saved LOWER→CURRENT migration,
touch/orientation/background recovery. Owner voice/haptic and final terrain/
traversal appearance/feel remain pending. Do not infer acceptance from these
frozen frames.
