# 1.6.2 shader budget review

Reviewed the released frozen catalog and budgets, compiler strategies, root's failed freeze log, both isolated eight-variant matrices and current shared source. No catalog or repository budget edits, publication, native build, GPU or phone use was performed by this agent. Parent owns budget adoption and final catalog, build and cost validation.

## Original failure and corrective slice

The dated `../shader-pipeline-freeze.log` and `../shader-review-matrix` show Generic 66 functions / 212 calls / two static ray-query sites, compared with released 63 / 199 / two. Opaque stays fully inlined at one function / zero calls, but both qualities retained 27 static query sites and 57 loops versus 23 / 49. Shipping High Opaque grew from 501012 to 530124 bytes; Shipping Mobile Opaque grew from 501012 to 527996. The old Opaque budget was 501424, which already had 412 bytes of slack; that bound differs from released artifact size.

Source and disassembly verified the three added Generic helpers: `scaledTangentNormal`, `areaShadowSampleIndex`, `highMaxAreaShadows`. Original Mobile's high helper returned false, yet its higher-tier loop scaffolding remained in the preprocessed and compiled module. The retained optimizer intentionally preserves helper boundaries rather than doing broad interprocedural inlining; a runtime-false call is weaker than structural removal. Root authorized compile-time exclusion before considering a Mobile query or loop budget increase.

Implemented preprocessor guards around the High-only helper, four-offset array and both additional traversal loops in `rt_lighting.glsl`. Mobile and macro-absent compatibility retain the original two-offset layout/parity and Max two-query average; High+Max retains four genuine primary area queries and fixed phase. Secondary limits, light extent/weights and defaults remain as documented. No traversal/volume budget increase or temporal approximation was introduced.

Temporary Shipping Generic Mobile and High preprocessing confirmed absence and presence of the exact higher-tier blocks, followed by glslang Vulkan 1.2 compilation and spirv-val PASS. Probe hashes: Mobile `3d59cb154baf2ae87674d80a323392fb760c9c8b811597bf2655715b773730e2`, High `60954b272efff89eacc5f40428ce62339b3b3e0ae5ebbdea8417231b662602a3`. These probes establish syntax and structural policy, not the production compiler strategy or device cost. Root's revised eight-variant production-strategy matrix is `../shader-review-bounded-mobile`; old matrices are retained as negative evidence.

## Reviewed finite max proposal

Use exactly the revised measured metric value for every max, with **zero arbitrary headroom**. The complete schema-preserving proposal is outside the repository at `../reviewed-raygen-budget-proposal.json`; repository budget and catalog files remain unchanged until parent adopts it. All existing exact invariants remain unchanged.

Proposal SHA256: `c2b04f7d603fd7ebb3215400118753a414aefdbd6fed6bd59b09b3c2ee8202ae`. The outside-tree `../validate-reviewed-shader-budgets.ps1` extracts the actual unchanged `Assert-RaygenFrozenBudgets` and its three helper functions by PowerShell AST, then checks all eight measured rows with manifest `shippingAllowed` values. PASS: every maximum and exact invariant is accepted. Negative PASS: reducing the first byte bound by one rejects `diagnostic_high_generic_dielectric/bytes`. This invokes no compiler, script body or catalog publication.

| Key | Bytes | Words | Instructions | Branches | Loops | Selections | Functions | Calls | Query sites |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| diagnostic_high_generic_dielectric |261764|65441|15482|856|17|360|66|212|2|
| diagnostic_high_opaque_fast |534360|133590|29205|3841|57|1559|1|0|27|
| diagnostic_mobile_generic_dielectric |258548|64637|15273|834|15|355|65|205|2|
| diagnostic_mobile_opaque_fast |526440|131610|28697|3739|49|1531|1|0|23|
| shipping_high_generic_dielectric |252936|63234|15058|809|17|332|66|212|2|
| shipping_high_opaque_fast |530124|132531|29026|3819|57|1550|1|0|27|
| shipping_mobile_generic_dielectric |249720|62430|14849|787|15|327|65|205|2|
| shipping_mobile_opaque_fast |522204|130551|28518|3717|49|1522|1|0|23|

Atomic max and exact invariants remain Shipping 0 / no binding 22; Diagnostic Generic 41 / Opaque 5 with binding 22. Preserve exact instrumentation, quality, material, strategy, shippingAllowed, diagnostic binding, fully-inlined/retained flags and all toolchain, dependency and byte identities. Opaque remains `LegacyInlined`, one function / zero calls, with reviewed High 27 / Mobile 23 query-site ceilings; the broader compiler emergency guard 29 does not authorize enlargement of these frozen row bounds. Generic remains `GenericRetained`, two query-site ceilings, High 66 functions / 212 calls and Mobile 65 / 205. Literal Mobile interfaces / volumes 4 / 2 and High 8 / 4 remain validated by route checks. Both launchers must retain mandatory environment binding 25 and world/viewmodel 23 / 24, with only the existing unused held-light 20 exception.

## Causal evidence and practical limits

Parsing actual released and current SPIR-V function-call operands accounts for the full Generic call delta. Common Mobile delta 6 is two `scaledTangentNormal` calls, three `areaShadowSampleIndex` calls and one extra `fireHash01` call in root's bounded flame-shape change. High adds seven more: three `highMaxAreaShadows`, two `offsetRayOrigin` and two `sceneShadowTransmittanceMask`. Thus Mobile 63 to 65 functions / 199 to 205 calls and High 63 to 66 / 199 to 212 match specific admitted helpers and the two extra shadow-loop call sites. Environment sampling uses image and extended instructions rather than adding a helper function.

With explicit structural removal, Mobile Opaque query sites 23 and loops 49 exactly return to the released shape, despite common material, normal, AO and world-factor decoding, flame arithmetic and spherical environment sampling still increasing code. Mobile Generic loops 15 likewise remain at release. High retains two additional source loops; fully inlined expansion produces the observed 57 loops / 27 static query sites. The source-removal comparison establishes that the Mobile excess came from higher-tier scaffolding; static site counts do not measure per-frame ray counts.

Revised artifact growth versus the released actual catalog is Generic High +9084 bytes (Shipping 3.725%, Diagnostic 3.595%), Generic Mobile +5868 (2.406% / 2.322%), Opaque High +29112 (5.811% / 5.762%), Opaque Mobile +21192 (4.230% / 4.194%). The guarded slice removes 3160 bytes from each Mobile Generic and 5792 bytes from each Mobile Opaque compared with the first matrix. High size remains unchanged. These are complete-feature static module deltas, not isolated runtime shadow or environment cost. Exact causal byte attribution across material, fire and environment would require separate controlled builds and is unnecessary for the bounded admission review.

Decision: approve this **finite source/strategy budget proposal** for parent's matching freeze, retaining every exact invariant and rejecting any unexplained excess. This does not approve visual/performance admission of the higher PC tier. Lead still needs matching High+Max/Authored movement/contact/occlusion captures, exact allocations, warm GPU/loop/present timing and sustained RTX/device evidence. Whole Max includes other workload changes; its total delta must remain labelled accordingly. Phone is unallocated. Any source/toolchain change that exceeds a proposed metric requires another explicit review, not automatic bound inflation or removed checks.

Next action: parent adopts the exact proposal, freezes/checks both production catalogs and runs affected native/shader artifact checks. Reuse the revised matrix evidence; repeat only for changed source or failed checks. This agent's shared shader source is stable.
