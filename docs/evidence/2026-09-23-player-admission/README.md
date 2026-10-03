# Player generated-runtime admission

Analysis/admission: 2026-09-23. Captures were completed on 2026-09-20 and retained;
they were not restarted merely because the session resumed.

## Exact artifacts and controls

- Native source: `adc4579763abe474a08cf8aa9172e0d622579a34`.
- Windows Debug Diagnostic/High executable: 7,725,568 bytes, SHA-256
  `9d26f20dcdfd4af90031b977cf3eb0203d2ae3b221a1c6d7dd5804a399b9368d`.
- Baseline player GLB: `cd64a8447097f59f825620ef43e2425f9286ff075f5bfdb48c25c5d96f2c479d`.
- Candidate/admitted GLB: `e8737f10e7669b284e04109d9c3acdf537df284093a21511656bf191a70450fd`.
- Both isolated executable/asset stages shared every runtime file except this GLB.
  Original sources, textures, licences and normal presentation selection were unchanged.
- RTX 5050 Laptop GPU, RayTracingPipeline, 960x540 dispatch/presentation, 100% scale,
  Diagnostic/High, 12 settling frames, fixed animation time zero, raw RT storage
  images with no overlays. Bundle IDs/hashes are in every native capture manifest.

## Results and scope

All ten native capture processes exited zero with complete one-checkpoint
manifests. All five baseline/candidate PNG pairs are byte-identical:
`player-body-grips`, `player-body-forward`, `player-body-owner-feedback`,
`player-body-downward-cut`, `player-body-upward-slice`.

[comparison.json](comparison.json) verifies matching controls and uses the existing
image tolerances: maximum RGB channel delta 3, fraction of pixels differing by
more than 1 at most 0.001. Actual values are zero throughout. This focused set does
not impersonate the separate canonical 13-checkpoint foundation/performance gate.
No tolerance was relaxed and no timing improvement is claimed.

Lead inspection of grips/downward/upward images confirmed rendered geometry and
held props in the scene. The existing development player still has conspicuous
angular sleeves/hand silhouettes; unchanged pixels do not make it the accepted
final first-person presentation. Dedicated viewmodel geometry/ownership and owner
phone acceptance remain Phase 3 work. Frozen captures are not live-motion evidence.

Static/skinned admission additionally validates actual primitive/vertex/index/UV
addressing, and current/changed-order fixtures pass. The separate regeneration
[receipt](../2026-09-20-player-regeneration/README.md) records exact indexed
position/normal/UV/joint/weight agreement, tiny tangent differences and canonical
single-thread reproducibility. The regenerated derivative is now admitted to the
development runtime; previous bytes remain in Git/LFS history. Original source
assets and licence statements were not replaced.

This is not Shipping/Diagnostic parity, pipeline/compute parity, physical-phone
acceptance, subjective arm acceptance, glass correctness or release certification.

## Recovery

`baseline/` and `candidate/` contain all original PNGs/manifests, copied and
hash-verified. PNGs are Git-LFS-backed; raw manifests are kept without newline
conversion so hashes in the comparison remain valid. `analyse.py` can rerun the
pixel comparison against this directory with a **new** output path (Pillow needed).
It performs image analysis only, not editing. NumPy was unavailable; the final
analysis uses Pillow directly and passed. Pillow emitted a future deprecation
warning for `getdata`, not a failed or weakened image check.

`capture-runner.ps1` is the exact historical invocation/staging script, not a
general release tool: it expects the pre-admission source tree and fixed local
executable paths/hashes. Recreate its baseline from `adc4579` rather than running
it against the now-admitted runtime. Local stage remains
`C:\Dev\tmp\horde-player-admission-20260920-a`; executables/caches are not in Git.

Audio/haptic manual revalidation required: **NO**. No feedback semantics changed.
