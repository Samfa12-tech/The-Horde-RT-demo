# Explicit body remainder: native evidence, not visual acceptance

2026-09-27, development work on parent `363bc72`. The phone remains on the
previous `b5d486b2...` candidate. No production asset was replaced.

## Implemented contract

An opt-in fifth `WorldBody` semantic, `BodyRemainderPrimaryVisible`, separates
connecting cloth/torso/pelvis/legs from the old catch-all NearFace material.
The exact four-region profile and separate two-region viewmodel remain valid.
Five-region admission requires the explicit WorldBody role and budgets; missing,
duplicate, unknown or visibility-conflicting declarations remain failures.

The existing world-body buffer/BLAS owns the remainder. The modelled route adds
primary bit `0x80` to its secondary bit `0x10`, while the independent viewmodel
keeps primary-only `0x40`. Instance flag `BodyRemainderOnlyPrimary` admits only
the remainder from world geometry on primary rays, excluding duplicate world
sleeves/gauntlets as well as head/collar. Secondary geometry is unchanged.
No new BLAS, descriptor, instance, buffer role or CPU/GLSL record size; no
screen-window cutoff on this new body region. Normal production stays unchanged.

Capture ownership now checks the actual recorded world instance flags and the
corresponding mask together, not just the old literal `0x10`. Negative tests
reject unfiltered remainder visibility, full-body primary and procedural arms.
Existing grip and image-pixel requirements were not relaxed.

## Asset evidence

Recipe: Blender 5.2.0 LTS `fbe6228777e7`, single thread, existing processor,
`--gauntlet-source-hand Right --grip-roll-degrees 105 145 --blend-elbows
--gauntlet-scale .099 --reconcile-sleeve-seams --body-remainder`.
Use a new output directory; no input/source assets overwritten.

- World SHA-256: `e8e0488809ab350ddcca29127fb6827eeecc3ba90c99516643d0b6c431496583`.
- View remains `6f06d77e754d7e2d9017b84c1204879aba2be302b69c077c5f84578408d5166b`.
- Paired manifest: `556f8b4f4f7509ee9cb6d8350d5bb79dd430a7544d923d88051794f18ab61141`.
- Triangles: sleeve 5,532; gauntlet 8,838; head 1,813; collar 1,116;
  remainder 10,476. Total remains 27,775. Expanded bind position, normal,
  tangent, UV and winding are exactly unchanged after rejoining the material
  partition. View bytes are unchanged.
- C++ world and paired view admission pass. glTF validation: zero errors,
  one existing `NODE_SKINNED_MESH_NON_ROOT` warning.
- First export had the old embedded identity-texture budget of four and was
  rejected. The explicit five-material manifest admits five 4x4 identities;
  actual production atlas allocation remains **173,364,524 bytes**, identical
  to the four-region control. This is not a new full-size atlas layer.
- Fresh no-option export still reproduces production world `e8737f10...` and
  view `1e3b041e...` exactly.

## Native results and rejection

RTX 5050 Laptop, RayTracingPipeline, Diagnostic High, 540x960 portrait,
fixed checkpoints. Capture executable SHA-256
`4c6a497f2e907807d2d516081ab74e4ef2ebe3c65894d329fdbdeac666c9fa93`.

- High lantern/look up 146 passes native ownership/grip/pixel gates:
  world mask 144/flags 9, view mask 64, no procedural arms.
- Low lantern/look down 145 and low parry 144 fail the unchanged lantern-body
  pixel requirement: zero body pixels, with 123/189 ring pixels respectively.
  These failed captures are retained, not admitted or installed.
- Zero pixels alone is **not** proof of a rendering defect. Owner clarification:
  physically natural view occlusion and light occlusion are valid and must not
  be removed to satisfy a capture. No cosmetic shadow suppression is allowed.
- The exact CPU-upload sightline gives camera model coordinates
  `(-0.002400, 1.649749, -0.300911)` in low/down. Source +Z is forward; the
  camera is behind the model. A retained torso surface intersects the ray
  toward the lantern at 0.409888 m, before the 1.085544 m target. This is evidence
  for a camera/body mounting mismatch, **not** proof of the original wrist cause
  or permission to hide that surface. The existing view-relative shoulder anchor
  is at forward 0.40 m. Anatomical mounting/reach must be resolved before this
  optional route can be accepted; no blind root/IK/weight search was applied.
- All three four-region control PNGs are byte-identical to the preceding seam
  candidate's native PNGs. Existing route and atlas use are preserved.

## Verification and remaining work

Fresh Debug builds and six selected Vulkan-enabled CTests pass (32.33 s):
primitive contract, player animation, ABI, static GLTF, actual skinned smoke,
character slot smoke. Python seam/gauntlet tests pass 18/18. All shader variants
compile under unchanged frozen budgets. A first conditional implementation
exceeded the branch budget; an equivalent branch-free integer predicate passes.
Shipping modules still have zero diagnostic atomics/binding 22. Actual Debug
executable containment finds exactly the four Diagnostic High pipeline/compute
modules; `spirv-val` and `spirv-dis` pass. This is not backend image parity or
performance acceptance.

Android candidate staging separately accepts an optional paired manifest only
with its receipt hash and a canonical file inside the candidate directory.
The real Gradle Sync task passes valid and legacy staging cases; missing name,
missing hash, wrong hash and traversal cases each reject with the expected
diagnostic. The focused test restores the previous generated overlay. This is
staging evidence; the separate subsequent build/package result follows below.

ARM64 Debug `:app:assembleDebug` subsequently succeeds (20 s), with the exact
five-region candidate manifest overlay. Archived APK SHA-256:
`a19fc1cfde52150461dd4c28debd348db0cfe54231a9301db00779f08ed149d4`, at
`C:/Dev/tmp/horde-body-remainder-android-20260927-a/HordeLanternRT-1.6.1-viewmodel-debug-arm64.apk`.
Runtime source is unchanged between `53c786a` and the test-probe-only `c339b6b`.
ZIP-entry hashes match the world, view and manifest above. The extracted actual
ARM64 library is `29f621a799227dd42f1faa8d121b14534b242c831ad8cae97f74b55f14296ce3`;
containment confirms the four Diagnostic Mobile pipeline/compute modules and
passes `spirv-val`/`spirv-dis`. Initial inspection incorrectly supplied the APK
to the ELF inspector and was rejected; the successful check used its extracted
library. No install, phone claim, release signing or publication.

New body visibility is investigation-only and **not accepted**. Required next:
anatomical camera/body mounting and reach, nonduplicating visible torso/legs,
inner sleeve and cuff continuity, mirror motion, live phone transitions and
owner acceptance. Keep the installed accepted-improvement candidate intact
until a replacement satisfies those gates. Do not reopen the fixed signed IK
projection or tune weights/roll to conceal this placement issue.

### Mount/reach constraint isolated after the checkpoint

The existing native smoke tool now exposes read-only `--inspect-player-mount
WORLD_GLB`, reporting actual Idle/0 node positions. This avoids treating Blender
imported display-bone tails as real arm lengths. Native upper/lower left arm
lengths are 0.276334/0.216431 m; Hand-to-Grip length is 0.068529 m. With the
existing maximum 1.75 chain stretch, the most generous shoulder-to-grip reach
is 0.930868 m (straight arm, optimally aligned socket).

For the exact low/down capture, translating the body backward just 0.202621 m
would bring the camera to the Head **joint** depth plane; this is deliberately
not claimed as an eye landmark or a final anatomical mount. Keeping the current
gameplay grip fixed would then require 0.964928 m reach, exceeding even that
generous bound by 34.060 mm. Actual oriented/bent reach can be smaller. Thus a
blind body-only translation is not a complete fix and cannot preserve the 15 mm
grip gate. No body transform, stretch bound, grip, pole, roll or weight was changed
for this probe. Next implementation must establish an anatomical body/camera
mount and compatible shared gameplay-held pose together, without faking arm
length or concealing body surfaces. Retain the owner's accepted swing separation.

Branch/PR CI at `53c786a` passes push36283401416 / PR36283405115, including the
portable and Vulkan-enabled player host lanes. This remains host evidence,
not final Android/RT-device acceptance.

Audio/haptic manual revalidation required: **NO**. This slice changes geometry
semantics/visibility and candidate packaging, not gameplay feedback inputs.
