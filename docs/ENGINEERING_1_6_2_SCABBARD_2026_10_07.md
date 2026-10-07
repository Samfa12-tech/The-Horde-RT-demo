# Sword stow and scabbard integration, 7 October 2026

The shared equipment transition now keeps the sword on Hips while the hand
reaches, changes semantic attachment at full Grip, then moves the held sword.
Sheathing reverses those phases. Reversal, interruption and copied death state
retain their current pose. The torch retains its existing ownership and target.
Production stow and waterfall-relocation flags remain false pending moving
device and owner inspection.

The original authored 304-triangle sheath follows animated Hips through the
ordinary static GLB/PBR route, including secondary visibility. Procedural bodies
without that Hips pose suppress its instance. Existing Keeper indices stay
fixed; scene ABI capacity becomes 24 metadata entries, 12 static assets and
26 TLAS instances. Props atlas layer 13 adds 16,777,212 encoded Windows bytes
and 2,650,704 Android bytes; all earlier layers and emissive data are preserved.
These byte totals are not GPU residency or performance measurements.

The initial authoring axis was wrong: Blender +Y exported as runtime -Z despite
the declared +Y contract. Blender +Z now exports as runtime +Y with identity
mesh-node transform. Source and runtime GLBs are 15,128 bytes, SHA-256
`d23c2b0711f53ce7608e68945ee7c38dd4eb1dc06ad5e33e58acc26c5bc0eaf5`.
The 10,052-byte processing receipt is SHA-256
`3d257b3cd2cfcd4f169ae962ef86e28d8b252351a1fbccd29a06d03d35e56541`.
It records source hashes, topology, bounds, provenance and validation limits.
Both GLBs have zero Khronos errors/warnings and pass the intended open-shell
topology check. No third-party or blanket asset licence is invented.

The corrected offline full-mesh check samples 25 Idle and 25 Walking poses
using runtime glTF Hips interpolation: zero triangle overlap pairs, minimum
floor clearance 0.121889 m and gap to the 0.82 m ceiling plane 0.622555 m.
The closest sampled body-gap witness is 0.020268 m; it is an upper-bound witness,
not an exhaustive continuous or edge-edge minimum. Earlier apparent Walking
intersections used incompatible Blender bone and glTF node bases and are invalid
diagnostics, not confirmed runtime intersections. Moving route inspection is open.

Current local checks pass: final socket regression 1/1 (14.47 s), including the
loaded long-axis guard and logical material routes 14/14/14/0 versus physical
atlas layers 14/14/14/1; transition, ABI, Character and cache/Bundle checks 5/5
in the preceding six-test run; closed source/staging/archive asset admission
108 cases; full Windows Debug default-target build; Android native all four
ABIs, 176/176 JUnit tests and lint zero errors/62 warnings. Shader regeneration
and compute freshness pass; refreshed manifest/artifact checks pass 2/2 with
unchanged frozen budgets. These are build/host checks, not owner or phone passes.

Retained failures include the first transition-fixture typo, a scoped Hips
variable build error, an intermediate missing cache-backend variable, stale
capacity/hash/atlas assertions, and two incorrect shader-tool invocations.
The corrected current checks above supersede their conclusions, not their logs.
Private receipts remain in `task-4`: `scabbard-phased-grip-host-build-20261007-*`,
`scabbard-cache-host-build-20261007-*`, `scabbard-cache-host-tests-20261007-05.log`,
`scabbard-socket-final-test-20261007.log`,
`equipment-scabbard-asset-policy-tests-20261007-03.log`,
`scabbard-shader-freshness-tests-20261007-02.log`,
`integrated-native-build-20261007-06.log`, and
`scabbard-cache-android-native-java-20261007.log`.

Rendered hands/grips, draw/sheath motion and sound, world body, shadows,
reflections, floor/roof/route obstruction, phone memory/cost and owner acceptance
remain separate gates. The earlier completed Rag clearance results are retained
in [their ledger](ENGINEERING_1_6_2_RAG_CLEARANCE_2026_10_07.md).
