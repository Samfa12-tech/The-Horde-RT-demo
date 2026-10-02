# Finite High edge observer

Control: clean source b041ea832b0b7fc1882bbf4239c695cc70fbcd45; actual
High Diagnostic executable 265b21c06b201a57116100e3c92716fa1173fa7f56cf04cf202b0d6e44c864d9.
Reuse the completed glass-edge-fresnel controls under
C:/Dev/tmp/horde-clean-rtx-corrections-20261002/native-high-diagnostic.

Question: why does pixel(456,304) remain pipeline RGB(22,17,17), compute
RGB(2,2,3)? This is pre-existing in the normal controls, not a demonstrated
regression from the clean normals/UV corrections. No precision/epsilon sweep.

Smallest missing evidence: the actual primary/reflection hits and ordered
production dielectric loop visits at that one pixel. Reuse the existing RGBA8
float-bit output writer; reserve row0 only. Nine bounded visits match the
unchanged High ceiling, 24 floats each; 64 header floats, maximum560 pixels.
No extra rays, samples, transport decisions, buffers, counters or CPU/GLSL ABI.
Only High/Diagnostic/Generic contains this observer; Shipping remains untouched.

Compile/validate only the two changed modules. Retain the other14 embedded
artifacts and their historical provenance byte-exactly, not falsely relabelled
as fresh compilation. This mixed-provenance catalog is investigation-only;
frozen byte/instruction admission remains failed and budgets remain unchanged.

Finite run matrix: one glass-edge-fresnel image per backend. Record exact source,
executable/module hashes and diagnostics; compare all nonpayload rows1-539 with
their completed clean controls under the unchanged max3/fraction0.001 gate.
If the watched pixel/path no longer reproduces, record observer perturbation
and do not infer a production cause. No more captures without a concrete gap.

Payload header:0 magic;1-2 pixel;3 hit;4-6 instance/primitive/material;7 t;
8-10 geometric normal;11-13 shading normal;14-16 base;17 roughness;
18 transmission;19 flags;20-22 primary direction;23-25 camera origin;
26-28 hit position;29-31 surface radiance;32-34 display RGB;35 Fresnel;
36-38 reflection direction;39-41 reflection query origin;42 reflection hit;
43-45 identity;46 accumulated t;47-49 position;50-52 reflected radiance;
53 visit count;54-56 transmitted radiance;57-59 throughput;60 terminal resolved;
61 overflow;62 volume open;63 TIR since transition.
Each visit starts64+24*n: hit/instance/primitive/material; t/open/roughness/IOR;
position/transmission; geometric normal/direction dot normal; direction/previous
spawn epsilon; actual query origin/minimum. Miss identity fields may be undefined;
never interpret them when hit=0. Default-unvisited reflection fields are likewise
not evidence if primary shading does not enter the production dielectric route.

Two changed modules compiled, optimised, disassembled and passed actual
SPIR-V validation. Pipeline267504 bytes SHA-256
1de465a4ea9e3956d3a85d6511fa79527f388d63460426561b0cad3db683e764;
compute267740 bytes SHA-256
87c59f4a9d1ca86960b94db06ec56dbba7b828728430350b90e83ab2eae3d6ad.
Both retain41 diagnostic atomics. All14 other module hashes are unchanged.
MSVC Debug/High/Diagnostic application build PASS; provider CTest1/1 PASS1.91s;
generated pipeline adapter check PASS. Exact executable SHA-256
9ed552b897ecfd719cebc223a1ea79f9244eb7071b9350b783f9c821769c05af.
The wrapper first lost dynamically loaded function metadata scope, then omitted
the compute manifest argument. Neither was a shader failure. The already
validated pipeline artifact was dependency-checked/reused after wrapper repairs;
only the missing compute module was compiled. Logs remain in C:/Dev/tmp.

## Completed native discriminator — contact ownership, not a fix

Exact source48702729ab7ccc0bddda91517da9b42bc6efa5e2, executable above.
Both finite native RTX5050 Laptop runs exit0 and honestly present RT. The
[comparison receipt](high-edge/comparison.json) checks camera/settings, module
identity, actual CPU geometry, allocations, visibility/grips and PNG hashes.
All nonpayload rows1-539 match their respective retained clean controls
**byte-exactly**, maximum0/fraction0. All41 dielectric/reason counter values
match the corresponding control too. No control or completed matrix was rerun.
The paired backend image gate still FAILS max20 with32 pixels over3; unchanged
tolerances. Both watched RGBs reproduce exactly. Payload images/manifests,
process/build/negative wrapper logs, [actual fields](high-edge/decoded.json) and
[run identity](high-edge/run-receipt.json) are retained alongside this record.
Unchanged CPU OBJ exports remain in the raw path rather than duplicating195MB.

Primary identity9/10/107, t2.2772326, direction, origin, position, geometric
normal and first Fresnel0.40573308 agree exactly. Shading normal differs by one
ULP, but the **actual reflection direction, query origin, hit0/496/2 and radiance
agree exactly**. Reflection is not the demonstrated source of this pixel delta.
Both paths enter the glass top, then visit side5, side6, side5 and back0 with
four internal reflections; tiny direction differences occur along this chain.

At visit5, both report t0.3171285 and world y=-0.95 at approximately
(-9.01598,-0.95,-15.420976). Pipeline commits glass9/8/107, geometric normal
(0,-1,0); compute commits opaque floor0/486/1, normal(0,1,0). Thus the **demonstrated
divergence is the selection of coincident glass-exit/opaque-floor surfaces**,
not a primary-ray, texture identity or reflection-radiance mismatch. This does
not prove which driver/compiler mechanism selected the ties, nor any universal
intersection-error bound. It is not evidence that other31 edge pixels or
held-lantern failures share this cause.

Pipeline refracts/exits, spawns at y=-0.9500308 below the floor, then misses the
scene. It contributes transmitted radiance(0.024184976,0.017057974,0.016500667).
Compute encounters the opaque floor with volume still open and executes existing
certified reason1 recovery: transmission0, terminalResolved1, overflow0,
volumeOpen1. Totals remain pipeline566/compute568 certified recoveries and
TIR13842/13843. No diagnostic suppression or failure-counter change.
Neither the dark recovery nor the brighter escaped-sky result is automatically
the physical reference; forcing compute to reproduce pipeline pixels could
preserve the floor leak instead of fixing transport.

Current traceScene uses forced-opaque hardware commitment for these continuation
calls, without an explicit same-contact medium/opaque ownership rule. Source
fixture transform has Y scale1.25/translation-0.325; its bottom and the route
floor meet at y=-0.95, consistent with the actual native fields. No geometry,
material, mask or budget was altered to remove this contact.

Next unfinished step: make a test-only independent contact reference for this
recorded case, then design the smallest bounded same-contact terminal/exit
policy that preserves physical transport **and the opaque receiver**. Review
the policy before a runtime candidate; do not add an epsilon search, raise
traversal budgets, skip the floor or promote either buggy baseline. This is a
new demonstrated contact case, not a reason to repeat the completed observer
pair or restart row43/Mobile/performance investigations. High/parity/physical
and frozen-cost gates remain open. No performance or Android acceptance claim.

Audio/haptic manual revalidation required:NO; shader-only observer leaves
gameplay/audio semantic inputs unchanged. Normal engineering is untouched;
isolated configuration returns to Mobile/Shipping after the completed capture.
