# Prepared forest region ownership

This checkpoint fixes a prerequisite for the ordinary two-way static-region
milestone. Prepared trees, understory, rocks and deadwood previously appended
triangles without their CPU region labels, so they implicitly became `Shared`.
They now retain `ExteriorDressing` provenance through material consolidation,
native packing and bounded candidate copying. Existing explicit labels and an
implicit shared prefix remain intact; corrupt incoming provenance fails before
publishing geometry or collision support.

The real chapter fixture contains 365,212 exterior-dressing triangles: 6,184
graveyard, 323,264 trees, 17,712 understory, 6,168 rocks and 11,884 deadwood.
Its 13,516 tomb-dressing triangles remain distinct. Every exterior triangle is
checked against actual authored placement bounds after consolidation. The full
packed owner has eleven world families, 31,957,440 vertex bytes and 5,121,816
index bytes. A byte comparison with the same owner without CPU labels verifies
identical GPU vertices, indices, materials, primitive records, vertex counts,
transforms, instance records and texture-array counts.

No contributor certificate or normal-play eviction is enabled. A partial owner
still reports `Preparing`; labels identify ownership, not ray/light irrelevance.
Coarse terrain, distant landmarks, geometry, collision, all rendering budgets,
texture limits, quality and both hardware backends are unchanged. **Normal
static-region residency saving remains zero.** GPU loading, completed-work
retirement, reload and sustained optimized-route measurements remain required.

Validation on final source:

- The new missing-provenance assertions failed before the implementation.
- Windows Debug and Release app builds pass. Forest dressing, static-geometry
  cutover, scene-resource inventory and rescue integration pass 4/4 in each
  configuration (167.48 s Debug; 15.73 s Release). These are test durations,
  not playable frame times.
- Android Debug builds all four native ABIs; `assembleDebug lintDebug` succeeds.
  Lint analysis is cached/up-to-date, not a fresh analysis claim. No phone was
  connected, installed or driven for this checkpoint.
- The existing closed-roster Android package checker passes against the actual
  APK, including exact admitted asset bytes and strict ASTC payloads. Shader
  sources, modules, catalogs and frozen budgets are unchanged.

[Receipt](receipt.json) binds the source, artifacts, embedded native libraries,
unchanged shader inputs and retained logs. Build outputs are local, uninstalled
development candidates; no release was produced. Fresh RT captures are not
needed for these CPU labels alone because complete GPU payload parity is checked;
visual and physical acceptance of the full chapter remain open.

Audio/haptic manual revalidation required: **NO** for this change. Semantic
events, listener/source data and playback are unchanged.

The preceding Unicode-field checkpoint `b9f2e942` passed all six exact-commit
[CI jobs](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/38471870756),
including the native Windows note-edit test and all seven required Vulkan-host
traversal CTests. Its failed parent remains preserved; particular font-service
causation is not established merely by the successor pass.
