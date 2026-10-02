# Source-plane metadata compatibility slice

Control source: 28b1b5980c2fb446df6f2ecf79414398213628a9 on isolated
`codex/horde-rtx-corrections`; normal engineering/profile remains 3d84e548.
This is NOT contact-policy admission or a performance improvement. Existing
transport, rays, masks, diagnostics, tolerances and authored geometry stay fixed.

The existing binding6 record grows from4 to12 bytes per actual world triangle:
unchanged code, binary32 plane bits, certified axis/sign flags. CPU certification
uses the actual indexed source vertices and conservative exact winding check.
Uncertifiable triangles retain the code and zero certificate. GPU allocation
delta is8 bytes/triangle, with the same buffer/descriptor/lifetime; setup-only
temporary records. No per-frame copy or new query. Actual resident totals and
compiled std430 offsets/stride will be recorded, not inferred from C++ alone.

Finite matrix (next unfinished step, no repeated controls):

- MSVC Debug/Release ABI tests and generator freshness/negative tests.
- Compile/optimise/disassemble/validate all16 affected pipeline/compute modules;
  inspect actual binding6 member offsets0/4/8 and ArrayStride12. Keep frozen
  admission budgets unchanged. Build actual Debug RT application.
- One High Diagnostic glass-edge-fresnel capture/backend, compared to retained
  `C:/Dev/tmp/horde-high-contact-candidates-20261002` controls. All prior payload
  fields/counters and nonpayload rows must remain identical; new marker9876 and
  plane/flags at332-334 must prove floor0/486 carries-.95/2 on both backends.
  Field335 reads the actual descriptor-backed runtime-array triangle count.
- One Mobile Diagnostic opening capture/backend against retained clean
  uninstrumented controls, unchanged image tolerance, to cover OpaqueFast.

No phone tests while disconnected. No new baseline, observer promotion, tolerance
change, or inference that the still-open High divergence is fixed. Audio/haptic
manual revalidation required:NO (no semantic, asset or playback change).

## Completed source/build/layout evidence

Debug/Release ABI and generator each2/2 PASS (2.59/1.61s); focused Debug provider,
ABI/generator, resource-inventory and character-slot checks5/5 PASS11.10s.
Independent focused review found no concrete ordering, flags or ownership bug.
All16 ABI-affected modules freshly compiled/optimised/SPIR-V-validated. After
adding the descriptor-array-length observer field, only the two genuinely
changed High Diagnostic Generic modules were recompiled; the other14 retain
their preceding compilation provenance, not a false fresh-compilation claim.

Actual embedded modules extracted, hash-matched to catalogs, validated and
disassembled: all16 offsets0/4/8, ArrayStride12, set0/binding6 PASS. All8 Shipping
modules have zero atomics, no binding22 and no plane observer. Actual pipeline/
compute High observer280016/280252 bytes,41 atomics; observer remains isolated.
Frozen budget SHA-256 f2ff4be07c08536d140ea395fb24d1e3d502882449a84fc245df121e2ad89f4d
unchanged. This is not cost admission. Required generated16-module artifact
updates are retained only on the isolated branch.

Actual High/Diagnostic MSVC Debug app build PASS, executable SHA-256
91e56a4edd9dddaee743f5dde8d605e866d611eb44cfe761adb78fd8818cb311.
Curated logs/layout receipt: [world-plane-metadata](world-plane-metadata/).
Next unfinished step: the four planned finite native captures/comparisons.
Do not repeat the completed compilation, layout or host matrix without a
specific source/artifact validity problem. No normal-engineering promotion.
