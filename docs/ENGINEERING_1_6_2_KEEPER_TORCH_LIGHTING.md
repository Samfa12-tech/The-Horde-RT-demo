# Keeper flank lighting, 5 October 2026

The owner's correction adds two rear flank torches because the Keeper is too
dark under the physically occluded mist. This slice is implementing against the
admitted2bd mist baseline. Its new native appearance and phone cost are not yet
accepted. The owner's earlier moving-mist approval remains scoped to2bd.

## Shared timing and physical ownership

Both WorldObject fire emitters ignite on the same fixed simulation tick as
RevealStarted. They stay lit through first entry, one-second retry recognition,
combat, retreat and the actual Dead animation. They extinguish at
deathAnimationComplete, rather than zero health or the earlier chest unlock.
The existing reward/spotlight timing remains unchanged. Full route reset and
dormant checkpoint imports start dark; authored combat imports are already
revealed and lit. Pause suspends phase/noise progression.

The admitted upright torch bodies use one shared body-only BLAS, excluding the
held torch's tiny emissive core. They remain visible and unlit before ignition
and after extinguishing. Generic stone/iron floor stands supply physical
supports. Meshes, visible fire and light share the authored item transforms and
existing Flame/Light sockets:

| Torch | Stable fire ID | Item origin XYZ | Flame XYZ | Light XYZ |
| --- | ---: | --- | --- | --- |
| First flank | 3 | -35.50,0.255,-16.35 | -35.50,1.02,-16.35 | -35.50,0.99,-16.325 |
| Second flank | 4 | -35.50,0.255,-14.05 | -35.50,1.02,-14.05 | -35.50,0.99,-14.025 |

Actual admitted GLB sockets match the shared transforms within1.43e-8m. The
body has no authored emissive material. Reused assets retain their existing
[licence and attribution](../ASSET_LICENSES.md); no asset generation or new
textures are involved.

## Explicit budgets and evidence

Storage stays four160-byte fire records,640 bytes. Active capacity rises from
two to four; no nearby important light is silently evicted. Opening ID1 and
reward ID0x4c414e54 remain unchanged. An original-torch-only tuning scale retains
the previous carried-light energy/fade while allowing world torches to remain
lit after the player's torch fails. A full append fails without replacing a
light. Global zero strength produces an empty active prefix.

Physical TLAS capacity24 is distinct from22 metadata records: the two additional
instances alias the admitted StaticPbr torch metadata1. The shared body-only
owner raises the maximum live BLAS count to19. There are no added material,
texture or descriptor records. Six stand boxes add72 world triangles. Move,
destroy, resource inventory and output-resize handle ownership include the new
BLAS.

The mist retains density, extinction and2/6/8 integration budgets. Its new
ceiling is six top-level interval-midpoint source visibility calls: sky, staff
and up to four active fire records. Inactive sources skip visibility work.
GenericDielectric may traverse multiple bounded interfaces; six is not a bound
on every hardware query initialization.

Actual temporary Pipeline8 measurements fit every existing finite cap; module
budgets remain SHA256
`0f130f8f1e50623772bf857561adafee23e730e2691c35e31cb6455df4b258ba`.
Shipping modules retain zero diagnostic atomics and no binding22; Opaque stays
fully inlined and Generic retains its bounded functions. Source/compiler review
receipt `1c32e381b30079b92fbbdfbc0e2acc261ff3d64c92e3d320c568688a3e496b03`
records all metrics and socket/alias checks. Static counts establish no dynamic
query cost, frame-time improvement or phone acceptance.

The optional completed-frame fireLighting record carries the actual successful
upload's dense stable IDs, positions/strength and RGB/intensity through the
owning submission/completion. All unused slots must be zero. Historical absent
fields remain absent. Simulation state and completed RT records are separate;
neither a CPU snapshot nor an upload getter establishes presentation.

## Checks and remaining gates

Shared gameplay and development-staging fixtures pass, including exact reveal
tick,15/30/60/120 delivery, pause, retreat, full three-hit defeat, early chest
unlock, actual death completion, retry/reset/import and ordinary lantern claim.
Simulation receipt
`c48fe9f002579851e550ab14e16c24d65475fa578a0a0d21e5adbfa531b0a82b`.
Six affected telemetry/coordinator/benchmark/motion fixtures pass. Generated
ABI/capacity checks pass; Android witness policy passes87 injected assertions
without device actions. Initial fixture compiler/expectation failures remain
preserved; runtime validators were not weakened.

All16 published Pipeline/Compute modules and the two compatibility modules pass generation/freshness, exact catalog word joins and actual Shipping disassembly. Bank receipt `a3850959d6e1b75b330b4b098a31bd590f8a11e684dd708af22094617a9a9f31` retains unchanged budgets. Debug and Release native builds pass; six affected renderer/player tests pass, including resource ownership, physical-instance alias masks and initialization preflight. The initial22/24 mask-type compile failure is preserved and fixed by using physical TLAS capacity in the shared player mask helper. Static captures now retain canonical completed-frame evidence after draining the actual last presentation; a missing/stale owner or fire upload rejects the capture. Updated shader-tooling fixture execution, exact package admission and Android builds remain in progress.
Require actual reveal/fight/death/reward captures joining completed IDs3/4,
including unlit dormant/reset and possible reward-lantern overlap before death
completion. Review the new scene's appearance with the owner when available and
measure matched ordinary Shipping phone cost/thermals. Do not carry earlier
mist cost forward as this scene's cost.

Audio/haptic manual revalidation for this lighting slice: **NO**; no cue asset,
semantic event or feedback timing changes. Earlier Windows listening gates
remain open at their original scope. No release, merge or production signing is
authorized.
