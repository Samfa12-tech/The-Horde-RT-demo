# Sparse static selection and GPU ownership checkpoint

This closes a prerequisite for ordinary two-way chapter streaming: one immutable
static owner can omit selected source triangles while preserving uncertain
contributors and all shared identities. **Normal-play static saving remains zero.**
No selector or visibility certificate has been enabled in ordinary gameplay.

The old copy could remove only complete region tags. The new immutable omission
plan binds sorted, unique global triangle IDs to one exact admitted source and
exclusive static-PBR owner. It rejects Shared provenance, foreign/player owners,
overlapping ownership, empty primitive rows, source substitutions and combination
with whole-region filtering. The scene-owned factory checks the current generation
without exposing the private source. Selection preserves vertices, materials,
transforms and instance identities; only index spans and their provenance compact.

The unchanged per-turn copy/scan budget now charges17 bytes per sparse scan
triangle rather than13 (indices, provenance and plan lookup). Admission/ID-list
allocation is separate CPU work, outside that guarantee. Vertex compaction is
not implemented. A nonempty plan cannot become a complete contributor set merely
because both region bits and every family row remain present. Descriptor rollback,
move and reconstruction preserve/reset that completeness state.

## Actual hardware transaction

`capture-static-sparse-cutover.ps1` invokes a Debug-only explicit diagnostic, never
normal selection. Native Mobile/High CPU fixtures independently identify source
IDs75693 and91585 (one tomb and one exterior triangle). Both hardware backends
exercise partial CPU cancellation, actual GPU-upload cancellation, failed/stale
retirement proof, bounded copy/upload, completed AS work, TLAS-only refresh,
changed-pose first-frame adoption, old-owner retirement and full-source restoration.
The returned owners remain separate until completion; TLAS counts go1→2→1.

All four selected/restored image comparisons have **zero changed pixels** against
their own complete baseline. The actual Pipeline before/selected/restored images
and Query selected image were opened and inspected; the zero-pixel comparisons
bind Query before/restored to that inspected view too. This proves this transaction
and matched frame, not global visibility eligibility or final visual acceptance.

| Backend | Complete | Explicit sparse diagnostic | Restored |
|---|---|---|---|
| Pipeline | ![Complete](pipeline-00-before.png) | ![Sparse](pipeline-01-selected.png) | ![Restored](pipeline-02-restored.png) |
| QueryCompute | ![Complete](query-00-before.png) | ![Sparse](query-01-selected.png) | ![Restored](query-02-restored.png) |

Exact view: chapter-lantern-side-wall-high216, AnatomicalBody, Rest/HeldHigh,
960×540 at100%, High optical, HIGHER shadows, Standard dust, mist enabled,
12 settling frames, NVIDIA GeForce RTX5050 Laptop GPU. Strict primary visibility
passes with43469 player pixels and no arm exception. Each manifest records its
own shader hashes, frame identity, masks and physical held-light state.

In both diagnostics, active static-owner allocation goes69,356,864→69,356,352
bytes and restores exactly; scene device-local405,552,688→405,552,176 also restores.
The two-triangle example therefore retires512 device-local bytes and256 host-visible
bytes. These memory classifications overlap where applicable and must not be
added. This is diagnostic saving only. Upload decreases24 encoded index bytes;
allocator/AS granularity accounts for the different allocation delta.
Preparation overlaps reach474,906,416 scene device-local bytes; exact per-backend
host counts and scratch/owner overlap are in the transaction receipts. Hidden
Debug preparation/cutover times are retained, **not playable FPS or Release pacing**.

## Validation and disposition

Windows Debug/Release builds pass. Final Debug3/3 affected CTests and final
Release5/5 affected/asset-policy/shader-ownership CTests pass.
Android four-ABI Debug build/lint and the exact ASTC/runtime package check pass;
no device was installed or used. Fresh exact-commit CI belongs to the pushed SHA,
not the previous checkpoint's all-six pass.

The focused read-only review found the missing scene-owned factory; that gap is
fixed and exercised against the actual native source. Source/query review also
corrects the aperture count to four and includes dust, mist and terminal receiver
visibility. Native arch seam uncertainty remains retained. The next gate is a
conservative, runtime-bound contributor selection, ordinary two-way scheduling
and reload, then optimized sustained out-and-back memory/pacing verification.
The prior live desktop under60% readout is context, not acquisition-stall attribution.

[Artifact/source ledger](artifact-receipt.json), [pixel comparisons and measurements](image-comparison.json),
the per-backend manifests/transaction receipts and matching source hashes preserve
exact identity. Production rendering, assets, masks, quality, frozen shader budgets,
18 texture layers, actor capacity, torch coverage and180-second policy are unchanged.
Phone Current/Higher and saved Lower→Current migration, full route, owner feel and
physical acceptance remain pending. Mist28 and canopy remain deferred. No merge/release.
