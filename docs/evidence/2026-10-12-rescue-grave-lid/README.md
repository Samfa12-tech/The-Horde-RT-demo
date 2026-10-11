# Rescue grave lid finish

Continues `ee808c02136d62c7d6657f1153e48e68629d9d04` in the existing PR27
worktree. The tomb-to-woodland chapter remains incomplete.

The grave lid now has a shallow stone field, four raised rim bands and a central
panel. Six closed original pieces replace the plain proof box: 72 triangles,
60 more than the old slab. The accepted outer bounds, shaft seal, collision,
slide distance and deployment timing remain unchanged. No inscription, loot,
light, transparency, actor or texture family was added.

The new versioned Blender master and source-only native GLB round trip use the
same metre-scale recipe as runtime. Native tests verify all six closed pieces,
their positions, original seal and admitted bounds. The authoring tool refuses
to overwrite existing revisions and disables backup writes. Existing masters,
untracked backups and failed authoring evidence remain preserved.

The lid uses the existing packed procedural surface-code buffer and stone
material. An explicit bounded metadata range replaces its former flat-colour
shader exception. Local-coordinate projection carries the texture with the
moving lid. Static PBR ownership, ABI size, descriptors, material/primitive rows,
three actor slots and 18 layers remain unchanged. RayGen's frozen budgets are
unchanged; all eight variants fit. Compiled High diagnostic Generic/Opaque are
596/2,312 bytes smaller than the parent, with unchanged static query counts.
These static reductions do not establish an FPS improvement.

## Inspected native RT evidence

RTX 5050 Laptop, 960×540 at 100%, Diagnostic High, HIGHER shadows, twelve
settling frames. Exact cameras, completed submissions, shaders, counters,
timings, memory observations and artifact hashes are in
[the receipt](candidate-receipt.json) and capture manifests.

| View | Before | Candidate |
| --- | --- | --- |
| Grave / Pipeline | [Plain lid](before/196-chapter-landing-west.png) | [Stone finish](west-pipeline/196-chapter-landing-west.png) |
| Grave / QueryCompute | Same frozen viewpoint | [Stone finish](west-query/196-chapter-landing-west.png) |
| Reverse portal / Pipeline | [Parent](before/199-chapter-stone-portal-rear.png) | [Candidate](portal-pipeline/199-chapter-stone-portal-rear.png) |
| Repeated ascent / Pipeline | Existing planted traversal | [Candidate](repeat-pipeline/213-chapter-stake-repeat-ascent.png) |
| Repeated ascent / QueryCompute | Same traversal stage | [Candidate](repeat-query/213-chapter-stake-repeat-ascent.png) |

These candidate PNGs were opened. The material, relief and grave placement read
consistently on both backends. The reverse portal retains lit stone and its
underside. The planted repeat-ascent grip remains attached; its tight/dark view
still needs owner acceptance. Captures are scene-only and do not verify HUD,
subtitles, voice or a fresh interactive three-heart start.

Actual complete-owner replacement and restoration have **zero changed pixels**
on both backends. The portal differs from its parent by 73 pixels at a maximum
one-channel step, rather than being byte-identical. The backend comparison has
3,197 changed pixels, mean channel difference 0.00323/255, maximum 33; do not
claim cross-backend byte parity. [Pixel results](image-comparisons.json) retain
the exact pairs. The deferred mist correction is untouched.

Preparation still takes synchronous work and retains the complete owner. The
cutover manifests report actual overlap and timings; finite frozen samples,
some taken alongside host checks, are not sustained performance evidence or
ordinary streaming. No eligible zone retirement or residency saving is claimed.

## Validation and retained failures

- Windows Debug/Release application builds passed. Focused CTests passed 12/12
  in 139.22/19.34 seconds, including native import, range bounds, readiness,
  resource lifetime, terrain journey, attachment and adversarial regressions.
- Five other backend/containment/torch CTests passed in each configuration.
  The sixth, shader recompilation, initially failed because the sandbox compiler
  could not write its temporary SPIR-V file. Its unchanged CTest passed outside
  that sandbox in Debug/Release, 38.89/45.83 seconds. Failed suite logs remain.
- Fresh eight-key RayGen compilation/budget checks and eight-mode QueryCompute
  provenance/structural checks passed. Windows containment controls and exact
  Android stripped/packaged shader identities passed.
- Android all four Debug native ABIs, assemble, lint and unit-test tasks passed;
  284 Java tests have zero failures/errors. Runtime roster/exact-byte, strict
  ASTC and held-item attribution contracts passed. No device install or input.

The first capture failed because readiness still assumed a world-only surface
code buffer. The guard and focused missing/mismatched-range tests now account
for the lid's owned range. Failed captures and their state trace are retained.
An intermediate QueryCompute capture exposed stale embedded shaders and is
retained separately as failed evidence; fresh artifacts and recaptures correct
it. Initial invalid freeze/stale-adapter build failures also remain in logs.
No assertion, shader limit or original torch coverage/deadline was relaxed.

## Remaining acceptance

Inspect the lid from both sides after claim, approach the stake, ascend, return,
descend and repeat ascent. Check equipment/light attachment and the reverse
portal while backtracking. Phone CURRENT/HIGHER shadows, saved LOWER migration,
touch/lifecycle/orientation and fresh audio/haptic review remain pending.
Incremental two-way GPU readiness/retirement, remaining prepared woodland
dressing, the distant shell and the full chapter gallery still prevent chapter
completion. Exact pushed-commit CI must be checked independently of the parent.
