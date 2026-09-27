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
staging evidence, **not** an Android build, installation or physical-device pass.

New body visibility is investigation-only and **not accepted**. Required next:
anatomical camera/body mounting and reach, nonduplicating visible torso/legs,
inner sleeve and cuff continuity, mirror motion, live phone transitions and
owner acceptance. Keep the installed accepted-improvement candidate intact
until a replacement satisfies those gates. Do not reopen the fixed signed IK
projection or tune weights/roll to conceal this placement issue.

Audio/haptic manual revalidation required: **NO**. This slice changes geometry
semantics/visibility and candidate packaging, not gameplay feedback inputs.
