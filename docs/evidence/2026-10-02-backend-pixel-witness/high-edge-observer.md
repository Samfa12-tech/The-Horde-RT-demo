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
1de465a4ea9e3956d3a85d6511fa79527f388d63460426561b0cad3db681e0ad67;
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

Next unfinished step: execute the finite pair, decode fields and compare all
nonpayload rows with the retained clean controls. Audio/haptic manual revalidation required:NO;
shader-only observer leaves gameplay/audio semantic inputs unchanged.
