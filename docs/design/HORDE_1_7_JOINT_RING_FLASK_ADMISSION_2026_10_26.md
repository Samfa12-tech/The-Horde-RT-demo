# Joint warning/flask admission proposal — pending owner approval

This supersedes the two standalone +1 requests for a combined1.7 decision.
No runtime, ABI, shader, default or asset is changed. The machine-readable
[proposal](data/horde-1.7-ring-flask-owner-proposal.json) and
[offline validation](../evidence/2026-10-26-joint-owner-proposal/README.md) pin one
internally consistent request, not an admitted resource layout.

## One roster, both roles present

Keep ordinary owners0–27 unchanged, including forest master metadata26/27.
Append warning owner/physical slot28 with unique asset13; append right-hand flask
owner/physical slot29 with unique asset14. These are planned append indices,
requiring packing tests to preserve them; no other owner is borrowed or aliased.
The existing sword/scabbard, left light, actors and effects remain independently
owned. The new roles may coexist during an incoming attack while drinking.

Each new asset has one primitive and one separately owned material. The warning
material alone changes its colour/emissive factors; the opaque flask material is
immutable. Use the existing generic StaticPbr/emissive route and per-frame
instance transforms, not a new object shader branch or light. Today's singular
attack coordinator needs one warning owner. A future simultaneous-warning pool
is outside this request and must not silently drop extra sources.

| Bound | Current | Requested ceiling |
| --- | ---: | ---: |
| Ordinary definitions / metadata | 28 / 28 | 30 / 30 |
| Unique static assets | 13 | 15 |
| Primitive rows | 36 | 38 |
| Material rows, including intake peak | 32 | 34 |
| TLAS definitions | 104 | 106 |
| Ordinary / shared BLAS maxima | 21 / 23 | 23 / 25 |

| Preserved Showcase profile | Planned physical definitions | Primitive rows | Unique assets | Final materials | BLAS maximum |
| --- | ---: | ---: | ---: | ---: | ---: |
| Baked world | 30 | 34 | 15 | 34 | 23 |
| Shared64 | 94 = 30+64 | 38 | 15 | 34 | 25 |
| Shared76 diagnostic | 106 = 30+64+12 | 38 | 15 | 34 | 25 |

The forest physical block follows the new ordinary base30, while original logical
placement IDs, transforms and metadata providers26/27 remain unchanged. All
dependent counts must derive from that base; keeping the old28 offset would
overwrite the new roles. Do not truncate the twelve-tree diagnostic to fit104.

## Why34 materials rather than a hoped-for33

The retained native ordinary source has30 packed materials after the existing two
exact ChestWood/BlackIron aliases; the Showcase world palette adds2 for32 final.
`BuildStaticMeshSlot` first calls `InitializeSelectedGeometry`, whose per-asset
intake enforces capacity, and only then applies the two chest aliases.

With two new one-material assets the sequence is **34 before aliases →32 after
the existing aliases →34 with the world palette**. Therefore34 is the minimum
ceiling for this unchanged order and selected no-new-alias plan. Existing
`ShareImmutableMaterialRows` is retained. A later proven flask alias could lower
the final row count, but would not erase the earlier34-row intake peak. A33-cap
plan would need a separately scoped pre-pack change/proof; it is not selected.

No new texture layers (current18), lights, fire emitters (current4), descriptor
bindings, record layouts or frames in flight (current1) are requested. Array
payload growth is not a driver allocation, shader-cost or timing measurement.
BLAS/TLAS backing/scratch, startup overlap, generated bank size and frame costs
still need actual queries and paired observations. Keep all current gates.

## Admission sequence, after specific approval

1. Produce/validate original bounded ring/flask assets, grip and source provenance.
   Each joint candidate role must own a valid imported mesh, BLAS and material
   before any visibility. Both remain bound while mask-hidden; no borrowed/null
   owner or incomplete asset becomes active. Current production stays unchanged
   until candidate prerequisites and visual gates pass.
2. Apply the reviewed bounds in canonical ABI source and regenerate C++/GLSL/bank
   through the established workflow. Preserve existing field offsets/bindings,
   owners0–27, light/effect budgets, RT presentation and one-frame synchronization.
   Loading/move/destroy/failure/recreation must account for both roles atomically.
3. Publish only immutable authoritative presentation. Warning source/phase and
   closing animation follow real attack timing/cancellation. Flask action and
   right-hand ownership follow simulation; renderer grants no healing or timing.
   Preserve accepted guard→attack→guard and left-light ownership.
4. Revalidate ordinary/shared retention with the new query sources/owners.
   Existing frozen full-source certificates are not transferable. Until proved,
   active warning/utility presentation must use complete resident contributors;
   restoration must finish before any incompatible reduced-owner frame presents.
   Do not weaken closure/lease gates to make the new props fit.
5. Run relevant CPU packing/ABI/lifecycle/asset tests, unchanged shader gates,
   actual RT simultaneous ring+flask and original full-actor controls, visibility/
   occlusion/reflection/shadow/contact and hand joins, dark/light and both aspects.
   Then use a separately allocated phone for correctness/interaction. Performance
   remains measured optimisation, not an FPS veto on approved scope. Normal
   enablement and release require their separate decisions/evidence.

The [Application Control evidence](../evidence/2026-10-26-windows-app-control/README.md)
retains the blocked automated launch and later successful owner launch of the
same forest executable. Its existing-window visual inspection then stalled at
Computer Use approval, without a capture. Neither observation admits this
joint layout or proves automated recreation. This proposal does not authorize
a blocked-launch retry or security change, nor does it admit the private
revival audio/stand assets.

**Bounded decision requested:** approve implementation/evaluation of this joint
two-owner layout and the exact ceilings above, preserving all stated validation
gates. This is not approval to enable unfinished features, ship assets, merge,
publish, spend, change security or start a phone session.
