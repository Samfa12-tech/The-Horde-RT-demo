# Exact binary visibility candidate

Base: 3e5bd64a6eff533f6255cd48e18b694a9bba8bfb. One candidate iteration.
Main source allowlist: shaders/raytracing/include/rt_lighting.glsl (visibilityMask only)
and tests/CharacterRenderSlotSmoke.cpp. The pinned compatibility artifact hashes
in tests/RaygenVariantManifestTests.ps1 and tests/RaygenVariantArtifactTests.ps1
must track reviewed regenerated includes;
this is dependent freshness maintenance, not another optimisation iteration.
Generated shader/catalog adapters are
reproducible dependent output, not hand-edited optimisation inputs.

Metric: median opening-room owning GPU command duration in milliseconds at
Shipping/Mobile 75% on exact SM-S948B. Lower is better; the player-facing target
remains 33.333 ms (30 fps), which the existing 66 ms GPU baseline does not meet.
Render entry-to-present return and p95 are separate secondary metrics, not
display pacing. Do not claim dynamic query counts or causality from static sites.

Keep native pipeline, scene, gameplay, masks, materials, transparent filtering,
ray bounds, compiler options, fire volume and glass budgets unchanged. No
nonphysical isolation source belongs in this candidate. Termination is allowed
only after a confirmed opaque blocker in the helper returning a binary answer;
nearest/ordered dielectric helpers retain their complete traversal.

Run focused Debug/Release host contracts, actual shader compilation/validation/
disassembly, affected deterministic native RTX and phone images, then matched
normal Shipping control/candidate trials with exact APK/module receipts.
All measurements after the owner's cooling-removal report are uncooled; warm-up
and thermal/power context must be retained. Do not pool earlier cooled runs.
Stop this candidate if semantics/images regress, comparability fails, or its
single measured evaluation is exhausted; do not widen into a shadow rewrite.

Existing native benchmark/receipt tooling is the admitted adapter. No game-dev
adapter exists for this Vulkan project; do not install a competing framework.
Audio/haptic manual revalidation: NO (no semantic or playback change).
