# Rescue graveyard development milestone

Continues parent `fbe33b9f40c9d844702c805b48ffb420533b2a25` in the same PR27
worktree. The chapter remains incomplete.

Six original closed, bevelled Blender markers surround the rope clearing. Two
verified T03 displaced covers remain horizontal; a bowl, broken urn and two
unlit candle stubs reuse verified sources. The new editable `grave-markers-r03`
master, authoring script, native manifests and derivation hashes remain separate
from runtime packages. Earlier r01/r02 authoring and failed intake evidence remain
preserved. No original master/backup, admitted tree, niche or release artifact
was overwritten. The protected existing `.blend1` digest is in the receipt.

Twelve pieces add **6,184 triangles**. Imported world triangles also supply
immutable nonwalkable contact geometry with the terrain's publication/generation;
ground samples and tree rooting remain unchanged. Height-clipped triangle queries
block visible solids for movement and carried equipment. Rope approaches and the
clearing trail retain capsule clearance. Reversible placement defaults are in
`GraveyardDressing.cpp`: 3 cm minimum embedding, 20 cm burial rejection, and
0.75-scale reused covers. The first cover placement failed the burial check;
the failed evidence is retained.

Disconnected same-material ranges consolidate under the existing static owner.
Tests compare every triangle corner's position/normal/tangent/UV bytes and order
within its material, rejecting invalid/unowned indices atomically. **Eleven world
families, 31/32 total native primitive rows, 30 materials, three character slots
and 18 texture layers remain unchanged.** No new texture, light, flame,
transparency, shader branch or budget is introduced.

## Inspected native RT gallery

All candidate images below were opened. RTX 5050 Laptop, 960×540 at 100%,
Diagnostic High optical profile, HIGHER shadows, twelve settling frames.
Exact executable/APK/shader identities, cameras, completed submissions, resource
and timing records are in [candidate-receipt.json](candidate-receipt.json) and
each manifest. These scene readbacks do not capture captions, audio or owner input.

| View | Before | Candidate |
| --- | --- | --- |
| Landing west | [Bare clearing](before/196-chapter-landing-west.png) | [Pipeline](west-pipeline/196-chapter-landing-west.png), [QueryCompute](west-query/196-chapter-landing-west.png) |
| Landing south | [Bare ground](before/195-chapter-landing-south.png) | [Burials and surrounding ground](south-pipeline/195-chapter-landing-south.png) |
| Reverse entry portal | [Parent](before/portal.png) | [Candidate](portal-pipeline/199-chapter-stone-portal-rear.png) |
| Staged repeat ascent | Prior [stake milestone](../2026-10-12-ground-stake-traversal/README.md) | [Grip/shaft](repeat-pipeline/213-chapter-stake-repeat-ascent.png) |
| GPU replacement | [Before](cutover-pipeline/cutover-00-before.png) | [Replacement](cutover-pipeline/cutover-01-selected.png), [restored](cutover-pipeline/cutover-02-restored.png) |

Reverse portal and full-owner replacement/restoration are **pixel-identical**
to their references: zero changed pixels. Pipeline versus QueryCompute west has
3,202 changed pixels (0.618%), mean channel difference 0.00323/255, maximum 33/255.
Camera and uploaded arm/body geometry hashes match. The opened frames are visually
consistent, but **not byte-identical backend output**. One west shadow-pane
diagnostic exists in both parent and candidate; this is not a new graveyard
failure or a clean glass-certification claim.

Repeat ascent is a staged pose, not an owner's fresh round-trip or a normal
Keeper-death test. Its separately seeded Keeper state is not a reproduced
dissolve failure.

## Cost and ownership

Compared with the parent's west camera, actual device-local allocation grows
**1,114,112 bytes** (1.063 MiB), host-visible allocation **731,648 bytes**. These
memory classes may overlap and are not summed. CPU ground/contact retention is
2,765,184 bytes versus 2,337,000 (+428,184). Buffer/allocation/BLAS/TLAS/descriptor/
instance counts and texture payload are unchanged.

The manifests retain finite twelve-frame CPU/GPU observations. West Pipeline
GPU command-buffer average is 8.74 ms; QueryCompute 10.55 ms; the prior west
Pipeline was 4.98 ms. Startup/cache and concurrent host-build conditions differ:
this **does not establish a matched sustained cost delta**, mobile performance or
WP3 budget acceptance. Controlled sustained measurement remains required; scale
and quality were not reduced.

The cutover diagnostic now retains the full owner: burial offerings share tomb
ornament families, so deleting those families would remove exterior contributors.
It reproves completed-work publication/rollback/retirement with zero allocation
savings, 57.5/111.0 ms Debug preparation, 9.0/3.0 ms cutover and 142,887,936-byte
owner overlap including scratch. It is **not normal seamless streaming**. Spatial
eligibility, incremental preparation and owned normal-play readiness generations
remain required; earlier ornament-only savings retain their historical scope.

## Validation and gates

- Windows Debug/Release application builds pass; focused suites **11/11** each
  (130.35 s / 23.07 s). Coverage includes native import/contact, family byte
  preservation, tomb/tree intake, resource lifetimes and rescue/adversarial/
  repeat-terrain traversal.
- Android Debug native builds, Java contracts and lint pass. Unchanged Java/lint
  tasks are UP-TO-DATE, not a fresh phone run. Both runtime/held-item APK roster
  checks pass. The Windows closed runtime asset stage passes; source masters and
  failed revisions are excluded.
- Eight frozen variants match fresh compilation; all structural/query-site/byte
  ceilings and budget digest remain unchanged. Original torch coverage and its
  180-second policy remain intact and selected in CI; that fixture was not
  separately rerun locally for this exterior slice.
- CI explicitly fetches the new GLB payloads for affected lanes and selects the
  new native fixture in Vulkan-host beside all seven required traversal CTests.
  Exact pushed-commit results must be linked to this milestone's SHA.

The first exact-commit run at `e075dc2026a1dc6e8b6e229f203d013187b2db32`
([38181114675](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/38181114675))
failed the closed-roster assertion in MSVC/GCC/Clang: that fixture still expected
86/87 files before the four marker files were admitted. Vulkan, Android and
selected sanitizers passed. The correction retains exact count assertions at
90/91, explicitly requires each of the four final marker paths once on both
platforms, and retains all missing/corrupt/duplicate/source-exclusion checks.
The complete local package fixture passes in 147.59 seconds; this adds evidence
that was missing from the initial focused suite. Fresh exact-commit CI remains
required for the correction. No deadline or assertion was removed.

Normal streaming, rescue-lid finish, further prepared dressing, distant-town
refinement, complete route/gallery and sustained owner/device acceptance remain
open. The two-heart and waterfall-light observations remain separate checks.
Phone CURRENT/HIGHER, saved LOWER migration, touch/orientation/background and
fresh audio/haptic acceptance are physically pending. Mist issue28 and replacement
leafy-tree authoring remain deferred. No owner game launch, phone install/input,
merge, signing or release occurred.

Next owner playtest when requested: defeat/claim, ascend, circle the grave lid
and inspect markers/covers from both sides; bump the solids, follow the clear
trail to Kit, then descend and ascend again. Report floating/buried props,
unexpected item/body retraction or collision snags. Android also needs the
shadow/migration, lifecycle, touch and audio checks above.
