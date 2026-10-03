# Suppress repeated transparent triangle candidates

Runtime change: only the shared static BLAS geometry flag. Baseline is corner-fix
checkpoint `3b9232963f966a5429d22d0c7de71d3a893b55e5`; the candidate adds
`VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR` for transmissive primitives,
leaving opaque primitives `VK_GEOMETRY_OPAQUE_BIT_KHR`.

## Proven contract defect and targeted result

`boundedShadowTransmittanceMask` multiplies transmission and counts each
non-opaque candidate. This is not idempotent. The previous BLAS flags were zero
for glass, which permits repeated evaluation of the same triangle candidate.
The Vulkan specification explicitly applies the duplicate-suppression guarantee
to ray-query triangle candidates, not just pipeline any-hit shaders:
[Ray traversal, triangle/generated candidates](https://docs.vulkan.org/spec/latest/chapters/raytraversal.html).

Matched RTX5050 Laptop captures,540x960, same shader hashes/settings/assets:

| Shader policy / view | Shadow overflows before→after | Primary transport before→after |
| --- | --- | --- |
| Mobile isolated lantern | 4759→0 | 0→0 |
| Mobile high/look-up | 1110→0 | 0→0 |
| Mobile low/parry | 8166→0 | 0→0 |
| High isolated lantern | 0→0 | 0→0 |
| High high/look-up | 46→0 | 0→0 |
| High low/parry | 0→0 | 0→0 |

The flag-only A/B eliminates the reproduced RTX shadow-count failures without
changing their limits or silencing diagnostics. Four additional High glass
fixtures (millimetre, edge Fresnel, tinted, fire) complete with transport/shadow0.
This does not certify every shadow ray or validate the existing material-thickness
attenuation approximation. That separate physical-shadow work remains open.
No performance gain is inferred from capture timings or build-time variation.

Windows candidate EXEs: Mobile
`f8588dc8b2b53d4c37593aa70681e203ef5e96300cc9572e057fb146603f3940`, High
`f2820d62bd099e02219d39e8e31222df5d7c0ebef0a567e2de46fe43c0998658`.
Baselines are the exact candidate EXEs in the [corner-fix report](../2026-09-30-glass-corner/README.md).
Reported static/production BLAS allocation sizes are unchanged (1032704/648064
bytes in the isolated Mobile pair). These are recorded allocation sizes, not
an assumption about driver memory, all resources or speed.

The standard13 pipeline capture run completes;12 images are byte-identical to
the preceding corner-fix build. Finale-roof changes368 pixels by more than1,
maximum delta10, so the existing max3/fraction.001 image-equivalence comparison
**fails** (fraction.00070988). Its timing check passes but is not a performance
claim. No old reference image or threshold is replaced. The altered glass
shadow result is expected from removing repeated attenuation; final changed-image
reconciliation remains distinct from merely obtaining zero failure counters.

Targeted High pipeline/compute captures all complete with zero shadow/transport
failures, but pixel parity is still **failed**: isolated lantern3 pixels over1,
max5; high/look-up no pixels over1,max1 (passes); low/parry9 pixels over1,max4.
The unchanged tolerance is max3/fraction.001. No tolerance was widened, and this
comparison is not Shipping/Diagnostic parity or acceptance of another GPU.

The separate standard13 backend comparison also fails5 images: worst-bend
max4, blue116, red120, finale-roof139, two-enemy-combat92. Other8 pass the
unchanged pixel tolerance; all13 captures complete. Changed-pixel counts are
6/1/1/137/4 respectively. Those sparse but sometimes large discrepancies remain
an independent backend investigation, not a duplicate-candidate success claim.
An initial attempt to request `opening` as a development-checkpoint name exited2
without capture; the ordinary standard13 route supplies the valid opening view.

## Phone and build evidence

Exact **SM-S948B**, Android16/Adreno840/driver2150932499,75%,1080x2235,
strict ASTC and RayTracingPipeline. Candidate ordinary Debug APK
`ea1c6265e49713cc3f5bd3061e38ddd64054b10a614d3ff24dbe949e016503e3`
installed/pulled back byte-identically without clearing data. Source is3b92329
plus the flag change. Seven-view run064746 and Home/resume pass. **All seven
PNGs are byte-identical** to exact previous APKfd11d9db/run062611; all recorded
glass counters are unchanged. Baseline phone shadow counts were already zero.
The old PNGs remain hash-bound in the preceding run; this bundle avoids copying
identical images and retains the new manifest/state/summary records instead.

ARM64 Debug and unsigned Release builds pass. Unsigned Release APK
`e7ca5e6a99f8121048ea2ee1e73974cf6d803754b776cd76879ec3ab0a7194d4`
is not installed/published. Actual packaged Shipping library
`ac613b6509aebfa44c3541092206cb5d2738e4e8f7682be64defd092829a08e8`
matches its stripped output; SPIR-V validation/disassembly confirms unchanged
Shipping modules with zero atomics/no binding22.

Fresh MSVC builds and3/3 selected CTests pass: static GLTF, production props,
and character/scene smoke. The existing source-smoke test now pins the shared
transmissive/non-opaque duplicate-suppression classification. Native before/after
captures, not that source assertion alone, supply the behavioral regression.

Review checked all callers of `appendStaticGeometries`, resource sizing and
unchanged ownership. The existing player refit uses opaque geometry flags;
both actual world-body and viewmodel GLBs have zero transmission on every
material and empty transmission overrides. Thus their flags are unchanged.
If transmissive skinned geometry is admitted later, initial/refit flags must
agree; this is a pre-existing conditional limitation, not a current asset issue.

## Remaining boundaries

This is a second bounded correctness fix, not Phase4 completion. Primary phone
interface-budget1 and certified recoveries remain; live visual glass, geometric
shadow attenuation, matched Shipping performance, changed-image reconciliation
and backend parity remain separate. S26 results do not certify S24/S25.
No shader/math/material/asset/animation/mask/budget changed in this fix.
Audio/haptic manual revalidation required: **NO**; feedback inputs and playback
are unchanged. Stable and owner-accepted candidate apps remain untouched.
