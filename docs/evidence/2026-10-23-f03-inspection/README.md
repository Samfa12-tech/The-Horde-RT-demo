# Direct F03 story-cue inspection - 23 October 2026

The approved treasury-road/waystone composition gap is now inspected directly
in native RT, after the backed-up outdoor music checkpoint. This change adds
two shared **development-only viewpoints**, not scenery or gameplay rules.
F03 has a readable dirt trail and natural rocks, but no visible marked road
stone or masonry remnant in either direct view. The scene assembly likewise
has no dedicated F03 clue/waystone. The village shell is visible forward;
chimney smoke remains unimplemented. No placement was changed merely to create
a visual difference.

![F03 toward the next trail segment](forward.png)

![F03 back toward the preceding trail segment](reverse.png)

## What changed and what was checked

`DevelopmentCheckpoints.h` adds 226 `chapter-forest-clue-forward` and 227
`chapter-forest-clue-reverse`. Both use accepted route point 6, X -3.7/Z 20.2,
and the existing station-six staging: diagnostic accepted-death seed, actual
ascent, fixed-step movement through stations 3–6, 120 settling ticks, then
zero-delta finalisation. This is not natural Keeper victory or earned campaign
acceptance. No exterior teleport or new input/JNI operation was added.
Forward yaw follows point 7; reverse yaw follows the actual incoming point 5.
Both pitch values are -0.30 radians inside the existing gameplay look limit.
Android retains matching name/ID and numeric view poses.

The registry test retains the 13 release checkpoints, checks unique names/IDs,
development-only lookup, actual route coordinates, existing equipment/arm
requirements and direction along the real trail. The existing forest dressing
fixture now stages both new views through the same actual terrain and root
contact checks as the three earlier exterior captures. It preserves the same
grounding, support ID/height/surface, camera and 20 STEP/root assertions.

| Evidence actually obtained | Result |
| --- | --- |
| Windows Debug registry and forest dressing registrations | 2/2 pass, 34.88 s total; forest 30.77 s |
| Android automatic chapter checkpoint name/ID/pose parity | 1/1 pass, no skips |
| Windows Debug app | Builds successfully |
| Forward / reverse native RT captures | Both exit 0 and report complete, honestly presented RT frames |
| User settings / owner campaign | Settings hash unchanged; existing absent owner campaign directory remains absent |

The unrelated complete Debug route deadline is still open and was not rerun or
relaxed for this inspection. Raw OBJ dumps, build trees and test APKs stay in
ignored reports. [Native test details](final-test-details.log),
[Android test run](android-final-tests.log),
[source receipt](source-receipt.json) and
[bounded capture script](capture.ps1) make the narrower evidence reproducible.
The source receipt identifies music base `e9541169` plus the four files committed
with this checkpoint; the capture manifest's configured buildId is not an exact
source-commit substitute.

The fresh captures use the **NVIDIA GeForce RTX 5050 Laptop GPU**, preferred
`RayTracingPipeline`, Diagnostic/High pair, 960×540 internal and output at 100%.
The immutable static payload is unchanged in both: vertex 39,956,288 bytes,
index 6,377,292, materials 3,840, primitive metadata 512, textures 274,027,796,
9 descriptors and 28 instance masks. These are resource/framing checks, not
matched frame-time or Android performance evidence.
Both retain strict primary-arm visibility: 22,087/22,088 player pixels and
11,322/11,314 lantern-body pixels, plus 493 ring pixels each. Actual images were
opened and inspected. The normal held reward lantern is visible; zero ordinary
torch/sword pixels are expected for that held equipment. See the complete
[forward](forward.capture-manifest.json) and
[reverse](reverse.capture-manifest.json) manifests and per-process custody receipts.
Capture mode skips both campaign `PlaySession` creation and audio playback.

## Retained failures

The first native command used a CTest registration as a build-target name;
that MSBuild command error remains in `native-wrong-target-retained.log`.
The actual target is `horde_rt_rescue_journey_integration_tests`; it builds.
No source or gate change was needed for that command correction.

The initial F03 pitch was -0.35. Both views staged, but the unchanged strict
camera/support gate failed. Operand diagnostics show actual pitch -0.32,
matching `GameSimulation.cpp`'s existing lower pitch clamp; support IDs, heights,
surface and grounded state match exactly. The setup was corrected to -0.30,
with the assertions retained. [First failure](native-first-failure.log) and
[operand diagnosis](pitch-clamp-failure.log) stay recorded. The temporary
diagnostic print also initially failed to compile because `PlayerSupportId` is
an enum class; its print now uses an explicit integer cast. No gameplay limit
was changed and no failing visual gate was waived.

## Next bounded content candidate

The next useful existing-scope change is a small marked masonry focus with
3–5 metres of broken edging at F03, aligned with this trail rather than creating
a fork. Use original closed slab geometry and the already licensed
`MedievalWall02` family; do not repurpose funerary lids as a road. Ground every
footprint on the actual immutable terrain, bury bases 2–3 cm, preserve at least
1.44 m from centerline to solid geometry and inspect Kit's offset path.
Preserve native terrain/support IDs and the existing STEP triangles; any solid
contact must use the same triangles and existing exterior ownership mechanism.

The full packed scene currently fills 32/32 primitive rows and 28/28 instances.
The chapter owner has 21 transient primitives and 11 materials before material
consolidation; one masonry append would make 22 transient and still consolidate
to the same 11 families. Reuse that owner before
`ConsolidateWorldBakedMaterialFamilies`; a surviving new primitive, instance,
material, texture layer, descriptor or BLAS owner is not this candidate.
A provisional twelve simple closed-slab ceiling is 144 triangles; actual mesh
bytes, grounding/clearance, native before/after views, AS allocation and timing
must be measured before adoption. Triangle counts alone are not a performance win.

Bellwether has three cold chimneys but no plume. Existing fire smoke is coupled
to flame strength/height, so it is not an independent cheap chimney effect.
Do not substitute opaque smoke meshes or enable expensive participating mist.
That separate story cue stays open. No town expansion, river crossing, save,
route-boundary, lighting-setting, phone, merge or release change.

Audio/haptic manual revalidation required: **NO for this diagnostic-view change**.
The separate outdoor music listening/phone gate remains open.
