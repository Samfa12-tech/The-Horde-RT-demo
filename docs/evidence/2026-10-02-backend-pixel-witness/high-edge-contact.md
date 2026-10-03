# High glass-edge contact: demonstrated discriminator, not a fix

Engineering renderer/modules/defaults remain unchanged. Isolated observer source
48702729ab7ccc0bddda91517da9b42bc6efa5e2, complete pushed evidence
741c755ebd15fc6ebb560a1f57cbb9e4bff96796 on codex/horde-rtx-corrections.
Exact Debug/High/Diagnostic executable SHA-256
9ed552b897ecfd719cebc223a1ea79f9244eb7071b9350b783f9c821769c05af.

Exactly one glass-edge-fresnel native capture per RTX5050 Laptop backend: exit0,
honest RT. Only one explicitly reserved RGBA8 payload row; no extra rays/samples,
buffers/ABI or budget changes. All nonpayload rows1-539 and41 diagnostic/reason
counters match completed clean controls exactly. Both watched RGBs reproduce:
pipeline22,17,17; compute2,2,3. Backend gate still FAILS max20/32 pixels over3,
unchanged max3/fraction0.001. Controls/full matrices were not repeated.

Primary9/10/107, direction, origin, t and geometric normal agree; actual reflection
direction, origin, hit0/496/2 and radiance agree exactly too. After four internal
reflections, visit5 has t0.3171285/world y=-0.95 on both paths. Pipeline selects
glass bottom9/8/107 (normal down); compute selects floor0/486/1 (normal up).
The selection of coincident glass-exit/opaque-floor surfaces is demonstrated;
the underlying compiler/driver tie mechanism and other failing pixels are not.

Compute absorbs transmission by existing certified reason1 recovery with an open
volume. Pipeline closes the volume, spawns below the same floor (y=-0.9500308),
then misses into sky. Neither is automatically the physical reference; forcing
the brighter baseline could preserve a floor leak. Counts remain566/568 certified
recoveries, TIR13842/13843. No diagnostic suppression or false physical pass.

Two changed modules compile/optimise/disassemble/validate;14 others retain exact
artifacts/provenance, not a new compilation claim. App build PASS, provider1/1
PASS1.91s, generated pipeline adapter PASS. Frozen cost admission remains failed;
no performance/Android/production promotion claim. Negative wrapper repairs and
negative paired-image gate are retained, not hidden. Isolated cache restored to
Mobile/Shipping without rebuilding unchanged artifacts; normal configuration never changed.

Durable isolated [full record](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/741c755ebd15fc6ebb560a1f57cbb9e4bff96796/docs/evidence/2026-10-02-backend-pixel-witness/high-edge-observer.md),
[native fields](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/741c755ebd15fc6ebb560a1f57cbb9e4bff96796/docs/evidence/2026-10-02-backend-pixel-witness/high-edge/decoded.json),
[comparison](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/741c755ebd15fc6ebb560a1f57cbb9e4bff96796/docs/evidence/2026-10-02-backend-pixel-witness/high-edge/comparison.json).
Both PNGs/manifests/build/process logs are retained there. Actual CPU OBJ hashes
are verified; unchanged exports remain in the original raw directories rather
than duplicating195MB. Raw observer root:C:/Dev/tmp/horde-high-edge-witness-20261002.

Next unfinished step at this earlier checkpoint: independent contact reference, then smallest explicit
same-contact terminal/exit policy preserving opaque receiver and physical
transport. Do not repeat completed captures, sweep epsilon/precision, raise
traversal/cost budgets, omit the floor or promote either buggy baseline.
Keep High row43, Mobile finale, other High failures and physical/device gates
separate/open. No phone use while disconnected; no merge/release/publication.
Audio/haptic manual revalidation required:NO (shader-only, semantics unchanged).

## Completed contact reference and candidate availability

Engineering test1f684a8 independently retains the real exit AND opaque receiver.
The authored planes coincide exactly in double precision using float operands;
the exit transmits into the floor and a below-floor spawn loses that receiver.
Exit Fresnel/energy and ordinary open-volume rejection remain checked. Debug/
Release dielectric each1/1 PASS2.45/2.74s. Fresh current-source branch36950835736
and PR36950830217 CI SUCCESS (GCC56/Clang56/MSVC62/Vulkan CPU-host15/Android).

New isolated source6ed11ed4247159ad13d2e753c0b62618a7a69901, pushed complete
evidenceafde28d089b8a21f6b9f05506783e93c39edab96, adds ONE non-confirming hardware
query only at watched pixel after visit4. Shipping is untouched; no new transport
decision, counter, geometry/material, mask, buffer/ABI or traversal budget.
Pipeline/compute two changed modules compile/validate;14 others remain exact.
Real app build PASS, provider1/1 PASS1.80s; both native RTX runs exit0/honest RT.

Each backend enumerates exactly two actual candidates: exit9/8/107 and
receiver0/486/1. All actual-path fields match the earlier observer; all41 numeric
counters and nonpayload rows1-539 match clean controls exactly. Strict backend
gate remains FAIL max20/32 pixels over3. Native raw distances differ, not tie:
pipeline exit0.3171132206916809/floor0.3171133100986481 (+3 float steps);
compute exit0.3171132504940033/floor0.3171132206916809 (-1). Nearest selection is
consistent with each query's reported values. This is NOT an equal-t driver-bug
proof, a universal error allowance or permission for an empirical epsilon.
Exact-distance equality alone is ruled out as the contact qualifier.

Durable isolated [full record](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/afde28d089b8a21f6b9f05506783e93c39edab96/docs/evidence/2026-10-02-backend-pixel-witness/high-edge-observer.md#completed-candidate-availability--not-an-exact-distance-tie),
[actual candidates](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/afde28d089b8a21f6b9f05506783e93c39edab96/docs/evidence/2026-10-02-backend-pixel-witness/high-contact-candidates/decoded.json),
[stability receipt](https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/afde28d089b8a21f6b9f05506783e93c39edab96/docs/evidence/2026-10-02-backend-pixel-witness/high-contact-candidates/stability.json).
Both PNGs/manifests and process/build/test logs retained there. Unchanged CPU
OBJ exports remain in C:/Dev/tmp/horde-high-contact-candidates-20261002 (195MB
not duplicated). Isolated cache Mobile/Shipping; actual executable still High
observer. Normal runtime/configuration unchanged. No Android/performance claim.

Next unfinished step: qualify contact from both actual triangles, preserving
exit Fresnel/Snell/TIR and the opaque receiver. World surface code/normal alone
has no authored plane position; review the smallest immutable metadata seam,
then prove air-gap/interior-obstruction/TIR rejection before runtime integration.
No more captures of the completed availability pair/controls, epsilon sweep,
budget increase, floor omission or observer promotion. All physical/cost/other
pixel/device gates remain open. Audio/haptic manual revalidation:NO.

## Completed conservative contact qualification (CPU only)

Source441a8f581e191193978325f4feb8e364ed399dcc on engineering/profile is the
reviewed cherry-pick of isolated675a32c65bc4a9c9b13c0655446ba724cd146e48.
`DielectricContactGeometry.h` is consumed only by the dielectric CPU test, not
the production renderer. The recorded source floor486 and box bottom8 qualify
without comparing their native ray distances or choosing an empirical epsilon.

The initial test failed because its nextafter air-gap fixture did NOT retain
the same rounded world coordinate. Splitting its assertions showed the qualifier
already rejected the gap. That erroneous fixture assumption is not evidence of
a transport defect. Separate +/-2^-20 gaps at coordinate64 now demonstrate
rounded-away gaps/interior obstructions and remain rejected.

The provisional FMA/TwoSum implementation was discarded before integration:
ordinary GLSL Fma does not establish the required exact fused-operation proof
([Khronos floating-point rules](https://docs.vulkan.org/spec/latest/appendices/spirvenv.html#spirvenv-floating-point-operations)).
The replacement is deliberately restricted integer binary32 arithmetic: one
power-of-two product operand, same-sign nonzero product/receiver, exponent gap
at most2, exact significand alignment/difference and no nonzero discarded bits.
Unsupported products/transforms/planes remain uncertified; no shader FMA,
float64/int64 feature, rounding-mode assumption, FTZ-sensitive arithmetic, clamp
or spatial tolerance is introduced. This is a mathematical CPU contract, not
evidence of an executed GPU implementation or its cost.

Independent review also identified unproven wide-range double-determinant
winding and degenerate exits. Source winding now requires an in-plane axis edge
and uses only exact coordinate ordering. Duplicate/collinear exits, wrong winding,
nonplanar/slanted triangles, shear/collapsed rows and invalid operands fail closed.
Signed-zero/difference, discarded-bit, cyclic/all-axis and post-Snell TIR cases
are covered. The finite reference grid executes24,576 checks in both multiplier
orders, with6,640 exact contacts admitted; the rest reject. The narrow double
oracle is exact for this grid, NOT for arbitrary admitted operand exponents.
Final focused independent review found no further concrete arithmetic/winding bug.

Fresh source441a8f5 [branch CI36954595930](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/36954595930)
and [PR CI36954600096](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/36954600096)
both SUCCESS. Actual logs inspected: GCC56, Clang56, MSVC62 and Vulkan CPU-host15
pass in each, including the dielectric test in all three compiler lanes. Android
Debug/Java/lint/package jobs pass. These source/run identities, not an old-job
rerun, own this evidence. Receipt-only later commits do not invalidate unchanged
source441 evidence or require repeating these checks.

Fresh isolated MSVC Debug/Release each1/1 PASS3.12/3.02s. Fresh integrated source
Debug/Release each1/1 PASS3.51/2.63s, with identical grid totals. Curated logs are
in [contact-qualification](contact-qualification/). No native captures, shader
rebuilds or phone operations: runtime/shader source and artifacts are unchanged.
The max20 High backend gate, other edge pixels, physical contact handling,
certified recoveries, frozen-cost admission and device gates remain OPEN.
Plane equality alone is NOT complete contact admission: exact matched-volume
candidate identity, actual hardware footprints, opposed geometric normals and
post-refraction outgoing side still require runtime checks. TIR never consumes
the receiver or closes the volume merely because planes coincide.

Next unfinished step: an immutable world-triangle plane certificate through the
existing binding6 buffer. The smallest reviewed representation is three uints
(12-byte std430/CPU record: existing unchanged code, plane-coordinate bits,
axis/winding validity). It adds8 bytes per actual world triangle, no descriptor
or resource-lifetime change. Validate actual source geometry before setting the
certificate; never infer it from the normal code alone. Generate/check the shared
ABI and update every old scalar-buffer consumer together; verify actual SPIR-V
stride/offsets and affected host/native compatibility before any contact policy.
No general framework or prototype promotion. Normal configuration unchanged;
owner audio/haptic manual revalidation:NO (CPU-only contract).
