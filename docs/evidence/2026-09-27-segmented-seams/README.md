# Bounded segmented-cloth seam repair

The owner accepts the raised-lantern wrist as a normal cuff on APK `c68fa94e...`.
This change does not add cuff geometry, adjust hand mounting, change the camera
range, or retune IK/animation. It addresses a separate measured cloth boundary.

## Proven cause and change

The unchanged view sleeves have 278 boundary edges. Earlier weight reconciliation
matched 198; the other 80 were two segments opposite 40 coarse retained-body
edges. Bind-space coverage was complete, but different subdivision/weights caused
up to **32.172846 mm** separation in the normal low-lantern/look-down native pose.
The retained before report measures exact CPU uploads, not image-space gaps.

Opt-in `--reconcile-segmented-seams` splits only those existing cloth triangles
at canonical sleeve vertices, then applies the existing exact boundary weight
transfer. This adds 40 vertices/triangles, no panels or caps. All 278 edges now
agree; the source viewmodel bytes, UVs, grip calibration and gameplay authority
remain unchanged. World/view GPU ownership and primary/secondary masks are not
changed. Production asset selection is not changed.

Two implementation checks caught and corrected candidate-only defects:

- Authoring coordinates use centimetres. Plan in world metres with the existing
  2-micrometre collinearity tolerance, then copy the exact raw canonical vertex.
  No numerical tolerance was enlarged.
- Blender re-encodes custom normals when setting the whole array. Preserve packed
  normals for untouched normal fans; only split-face fans need re-encoding.
  A disconnected-triangle regression verifies exact packed and evaluated normals.

## Exact final offline candidate

Directory: `C:/Dev/tmp/horde-segmented-seams-20260927-e`.

- World SHA256: `f2c3f62b2696c4630309b1d0b0ecb366151054fb48f7c0c6bcc6956980ff81eb`.
- View SHA256: `6f06d77e754d7e2d9017b84c1204879aba2be302b69c077c5f84578408d5166b`
  (byte-identical to the owner-tested viewmodel).
- Paired manifest SHA256: `556f8b4f4f7509ee9cb6d8350d5bb79dd430a7544d923d88051794f18ab61141`.
- World triangle count: 27,775 -> 27,815, below the unchanged 28,000 budget.
- View triangle count: unchanged 14,370, below the unchanged 15,000 budget.

Reproduce with Blender 5.2, `--background --threads 1 --python-exit-code 1`,
`--python tools/process-player-viewmodel-runtime.py -- NEW_DIRECTORY`, and:

```text
--gauntlet-source-hand Right --grip-roll-degrees 105 145
--blend-elbows --gauntlet-scale .099 --reconcile-sleeve-seams
--body-remainder --retain-upper-torso --reconcile-segmented-seams
```

The new directory must not exist. Source assets and earlier candidates are never
overwritten. Candidate D and its APK were not installed; they retain superseded
normal re-encoding evidence locally. Candidate E GLBs/receipt are retained here.

## Current verification

- Pure player tests: 28/28. Blender fixtures: 8/8, including unit conversion,
  one/two incident faces, ordered midpoints, UV/weight/normal preservation and
  protected-surface/malformed-plan rejection.
- Actual C++ world admission and paired viewmodel/pose/tangent/grip admission pass.
- glTF validation: zero errors, one existing `NODE_SKINNED_MESH_NON_ROOT` warning.
- Expanded Gauntlet (8,838), Head (1,345) and NearFace (468) triangles retain exact
  positions, normals, tangents, UVs, joints, weights and winding versus baseline.
- [Attribute comparison](artifact-report.json) accounts for all 40 inserted
  canonical sleeve positions/weights and all 40 old-edge splits. Common triangle
  positions/UVs are exact. The remaining normal/tangent changes affect 4/5
  BodyPrimary and 21/21 BodyRemainder triangles, with maximum vector differences
  below 0.000100/0.000142. These are bounded topology-export differences, not
  changes to image tolerances; the report does not independently localize every
  tangent change to a split fan. Changed weights at 7 BodyPrimary and 5 retained
  existing positions exactly match the canonical view sleeves. No animation,
  node, skin, material, texture or image metadata changed.
- Fresh RTX 5050 Laptop Diagnostic/High portrait captures: 141, 145, 146, 144.
  All 278 boundaries have zero measured endpoint separation in each exact native
  CPU upload. Captures use the existing normal camera range and RT storage image.
  Dark surfaces limit subjective seam inspection; this is not owner acceptance.
- The pre-existing high/look-up Windows shadow overflow count remains 47;
  transport overflow is zero in these four Windows captures. Do not declare
  glass correctness or extrapolate Windows counts to the phone.
- ARM64 Debug build succeeds. APK `66de46e6a045dacea88864adb153b2d4505bb640c9dbab0111262018e6a31816`
  is archived in `C:/Dev/tmp/horde-segmented-seams-android-20260927-b` with matching
  embedded world/view assets. Native ELF remains exactly
  `36eb0be2aab7201105e5e071242ac1d48b529dcef4a4c0281af642ec67288fbb`.

The isolated APK is now installed and pulled back hash-exact on SM-S948B.
Affected phone captures, live transitions and owner seam/body acceptance are
still pending for this new asset pair. Frozen poses alone do not
close Phase 3; mirror/body visibility, glass, performance and the full 1.6.1
programme remain open. No production promotion or publication is authorized.

Audio/haptic manual revalidation required: **NO for this offline topology-only
repair**; no feedback, event-time source/listener or playback semantics change.
This does not waive the separate earlier anatomical-profile feedback gate.
