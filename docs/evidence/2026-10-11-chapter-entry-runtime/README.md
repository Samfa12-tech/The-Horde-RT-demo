# Development chapter: enclosed lower entry in the runtime

The rescue chapter now selects `chapter-entry-return-v1`, a separate derivative
of the accepted collapsed entrance. Ordinary/legacy and preview scenes retain
`collapsed-entry` unchanged. Both Windows and Android closed package rosters
include the pair; Android stages both files before native initialization.
No source masters enter a game package.

The new native asset has 8,254 triangles, 12,131 vertices, two primitives and two
existing material families. The old asset remains 8,422 triangles, 12,467
vertices and a 12.24 m roof. The chapter roof is 6.00 m: the lower twelve stairs
remain, followed by an enclosed upper return. The thirteen-view source audit
and its negative cases remain in the preceding source milestone. No texture,
shader, capacity, quality or collision threshold changed.

## Inspected native views

Same checkpoint 192 (`rescue-journey-start`), camera (0, 0.70, 1.85), yaw pi,
pitch -0.05, frozen simulation time 0; 960x540 at scale 1.0 and compiled High
Diagnostic profile. Frames came from native RT storage images, not Blender.
No foreground input was taken. The source baseline is 69145687; this receipt's
candidate hashes identify the subsequent entry-only integration.

| Original entry, Pipeline | Lower entry, Pipeline | Lower entry, QueryCompute |
| --- | --- | --- |
| ![Before](before-pipeline.png) | ![After Pipeline](after-pipeline.png) | ![After Query](after-query.png) |

All three actual images were opened and inspected. The rubble and lower stair
composition are preserved; the dark upper enclosure changes slightly. This is
one opening view, not a complete exterior/terrain gallery. The Pipeline
before/after has 6,152 changed pixels (maximum channel delta 15). Candidate
Pipeline versus Query differs only by at most one 8-bit channel value. Exact
shader identities and counts are in [receipt.json](receipt.json).

**Strict capture acceptance remains FAIL on all three runs:** checkpoint 192
reports zero primary arm pixels with dedicated ownership enabled. The image is
still written before that assertion. The assertion was not weakened. These
frames establish inspected architecture and selected hardware RT execution;
they do not close held-rig acceptance, sustained performance, or the complete
chapter/phone visual gates. No automatic owner acceptance is inferred.

The recorded static vertex/index buffers shrink by 21,504 / 2,016 bytes and
production-prop BLAS allocation by 7,552 bytes in this matched initialization.
Material and primitive metadata remain 3,840 / 480 bytes. This is a small
measured allocation reduction, not an FPS or outdoor-capacity claim.

## Validation

- Red: integrated scene test rejected the original tall entry when the new
  chapter geometry was required.
- Debug: 4/4 affected tests pass (static collapse, integrated resource intake,
  tomb dressing, forest dressing), 8.63 seconds.
- Release: same 4/4 pass, 3.41 seconds.
- Original legacy collapse checks remain strict, including its 8,422 triangles,
  tall enclosure, material routes, normal strengths and sealed rearward rays.
- Added native chapter bounds/counts/materials and the same sealed-ray checks.
- Asset policy CTest passes, including chapter missing/duplicate/corrupt entries
  and missing/wrong Android native staging paths. The closed current runtime
  roster is 75 Windows / 76 Android entries; original asset hashes stay fixed.
- Android Debug native/Java build, unit tests and lint pass. Exact-byte held-item
  package and closed runtime archive checks pass on the resulting APK.
- Previous source checkpoint 69145687: all six CI jobs pass in run 38104028298.
  This integration's exact-commit CI is recorded after pushing.

Retained local failed-run evidence includes the intended red regression, an
exact-float comparison corrected to a one-micrometre native-coordinate check,
Release's initially unbuilt forest fixture (then built and passed), temporary
policy test mistakes, and the strict capture failures above. Nothing is relabelled
as a pass. Shader source and generated binaries are unchanged.

## Remaining chapter gates

Terrain r18 is still an editable/native-tested source, not live renderer/support
publication. Full woodland dressing, visible Kit, GPU-safe two-way zone
retirement, the graveyard/stake rope anchor, chapter moon attribution, the full
native gallery, owner playtest, Android device/backend/lifecycle and audio/haptic
acceptance remain open. This milestone does not complete the chapter.
