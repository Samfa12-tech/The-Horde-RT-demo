# Region engine foundation — CPU contracts and private shader feasibility

Continuation of47aaf662/2147e26b on draftPR27; no reset, production page adoption,
world-art replacement, merge or release. The existing normal static path still
owns the actual5.93 MiB saving and45.85 MiB extra peak recorded in the
[ordinary-owner receipt](../2026-10-14-ordinary-static-streaming/README.md).
No new memory saving, frame-time measurement or device presentation is claimed.

## Implemented

`src/world/streaming` supplies a reusable catalog/scheduler with tunable location,
prediction, retention, measured working-set target and owner/preparation limits.
Required traversal/contributions outrank optional detail. Unknown contributors
remain pinned; unknown costs remain unknown. Dependencies, cancellation, stale
callbacks, quarantine, reverse demand and slow-retirement capacity handling are
tested. Only a real backend acknowledgement can publish logical readiness.

The CPU geometry partition keeps every source triangle exactly once across a
compact base/page, preserving all vertex attributes, winding, global material IDs,
base primitive identities and complete outside actor/held layouts (including
unindexed vertices). It refuses shared selection ownership, alpha/transmission,
unbaked transforms, corrupt indices, nonfinite positions and insufficient memory
allowance. Allowance includes returned capacities/provenance and remap scratch.
Tests reproduce the previously underestimated allowance, including a sparse source.

The candidate page-table helper retains the832-byte legacy metadata prefix and
adds128 bytes for four address domains (base plus up to three detail pages).
Current scene free TLAS/AS slots can reduce that count. It rejects invalid flag
bits/page identities, absent owners, actor-role hijacking, misalignment, address
overflow and truncated primitive ranges. It never releases/adopts GPU resources.

Final affected native tests: **3/3 Debug and3/3 optimized Release pass**; logs
are retained here. New fixtures are selected in portable, additive Vulkan-host
and finite Clang ASan/UBSan coverage. CI for this new commit is separate from
the six-job2147e26b pass and must be verified at its exact SHA.

## Private shader result

All production shader sources, generated includes/catalogs and frozen budgets
remain unchanged. `tools/prototype-region-pages.py` creates a new private directory
under `reports` and refuses overwrite. It builds a uniform BDA lookup for retained
base and page material/index/vertex storage, using instance flag bits8–9; existing
actor `reserved1` semantics remain unchanged. CPU admission must reject high flag
bits; the shader does not mask corrupt identities into a valid page.

Final raygen recipe (from the repository root):

```powershell
python tools/prototype-region-pages.py reports/region-page-repeat --uniform-pages --flat-table --compact-symbols
# Use an absolute fresh output directory; never use -Freeze on production.
& ./reports/region-page-repeat/source/tools/compile-raygen.ps1 -Matrix -OutputDirectory "$PWD/reports/region-page-repeat/compiled"
python tools/check-region-page-prototype.py reports/region-page-repeat/compiled reports/region-page-repeat/receipt.json
```

[The final budget receipt](raygen-budget-receipt.json) checks every frozen max
metric and exact invariant against unchanged budget SHA-256
`56936479b2d202dbdb9393ba94b66a1b0bc12d490b700d8af4c63de3460e53bf`.
All eight pass. Generic serialization equals its byte ceiling; no spare headroom.
Diagnostic High Generic:295960 bytes/17618 instructions/83 functions/2 query sites.
Diagnostic High Opaque:586624 bytes/32298 instructions/1 function/29 query sites.
Diagnostic counters, samples, interfaces, capacities and compiler strategies stay
intact. PhysicalStorageBufferAddresses is present; shaderInt64 is not required.

Uniform addressing/reused handles remove executable work; concise new buffer
names remove OpName bytes only. The earlier branch-based and intermediate over-
budget results remain in [the attempt ledger](retained-compile-attempts.json) and
their raw ignored report directories. Compiler argument/admission failures were
also retained; they are not rendering failures or passing shader evidence.

Eight QueryCompute counterparts compile, optimize and validate through an
unmodified copy of `GenerateRayQueryComputeVariants.ps1`, writing **only into the
private prototype**. [Its private catalog](private-querycompute-catalog.json)
records exact source/binary/tool identities and structural checks. This is not a
change to the production compute catalog or a physical QueryCompute pass.

## Remaining integration and acceptance

The new scheduler/partition/page-table APIs do not yet own live scene pages.
Next: independent V/I/primitive/BLAS ownership and compact retained base; bounded
provider/upload jobs; contributor-safe normal scheduling; safe TLAS/current-frame
adoption/fence retirement; real reload/cancellation; then optimized Release
out-and-back measurements and inspected matched frames on both RT backends.
Keep coarse world, collision, distant landmarks, shared assets and uncertain
contributors resident. CPU cached sources must be reported separately.

Phone/device pipeline, lifecycle, physical memory/performance and visual/owner
acceptance remain pending. No device installation or phone input occurred in
this foundation slice. The chapter/loading system is **incomplete** at this point.
