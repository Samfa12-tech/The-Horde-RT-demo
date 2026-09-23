# Opt-in native RT viewmodel ownership evidence

Captured 2026-09-23 from the GPU-integration working tree based on `051fcde`.
The containing implementation commit identifies the reviewed source. This is a
development checkpoint, not accepted final arms or release certification.

Windows Debug Diagnostic/High executable: 7,957,504 bytes, SHA-256
`ee37411d240beeb06e7bdf10bcd5437ff1cbd85ae0550791ee2b7db227a41fcc`.
The exact staged runtime hashes are in [stage-receipt.json](stage-receipt.json).
Local executable/stage: `C:\Dev\tmp\horde-viewmodel-rt-20260923-f\stage`.
The executable/build cache is not backed up in Git; the source, shaders, runtime
assets, PNGs, manifests and analysis are.

## Frozen checkpoint results

All nine native processes exited zero with complete manifests: the existing
`player-body-grips` control and eight `player-viewmodel-*` poses. Each used the
RTX 5050 Laptop GPU, RayTracingPipeline, 960x540 at 100%, twelve settling frames,
fixed animation time zero and raw RT storage pixels without overlays.

The modelled route has world-body index 4 masked `0x10`, dedicated viewmodel index
20 masked `0x40`, and all legacy arm/head indices 10-16 disabled. Nonzero primary
player pixels are recorded in each pose. Sword/torch/lantern geometry retains its
separate ownership. The raised/lowered reward grip position and orientation
errors are zero at the reported precision. This does not prove continuous motion.

The new `camera` record contains actual simulation state; `requestedCamera`
preserves the authored request separately. All nine agree. Legal pitch extremes
are -0.32 and +0.28 radians. Earlier exploratory A/B captures requested +/-1.25
but gameplay clamped them; those requests are not extreme-angle evidence.

[comparison.json](comparison.json) records every image/manifest hash and checks
the old world-body image against the admitted Phase 2 control. Maximum RGB delta
is 2, with one pixel differing by more than 1 (fraction 0.0000019290123456790124).
The unchanged gate is maximum 3 / fraction at most 0.001: PASS. The shader
implementation changed, so bundle hashes intentionally differ from Phase 2;
scene, camera, device, resolution and capture controls match.

`analyse.py` reruns ownership/camera/hash checks and the comparison with Pillow:

```powershell
python docs/evidence/2026-09-23-viewmodel-rt/analyse.py docs/evidence/2026-09-23-viewmodel-rt C:/Dev/tmp/viewmodel-comparison-new.json
```

The output must be new. This script analyses images; it does not edit them.
To recapture, stage the foundation runtime roster plus the two viewmodel runtime
files, then invoke the exact Debug executable once per checkpoint with
`--capture-showcase <new-output-directory> --development-checkpoint <name>`.
Keep all stages/artifact hashes and inspect complete manifests, not exit codes alone.

An additional isolated stage with the same executable and runtime roster except
the two optional viewmodel files verifies absence behavior. `player-body-grips`
still presents RT and exits zero. `player-viewmodel-grips` exits one with an
incomplete manifest and the explicit missing-validated-runtime error; it does not
silently switch to full-body primary rendering. Original manifests are retained
under `optional-asset-absent/`; local stage is
`C:\Dev\tmp\horde-viewmodel-absent-20260923\stage`.

## Explicitly open

- Lead image inspection found severe angular/open-looking sleeve and shoulder
  surfaces and poor near-camera composition. Native ownership is proven; the
  model is **not visually accepted**. Refine geometry, not screen-space masking.
- `player-viewmodel-lantern-high` records one transport overflow. Other eight
  captures record zero; all nine record zero unclosed volumes. The overflow
  remains visible and requires the existing glass investigation; capture success
  does not certify glass correctness or waive that diagnostic.
- Live movement, swing transitions, wall retraction, exact Android presentation,
  owner phone acceptance, Shipping/Diagnostic and pipeline/compute image parity,
  matched performance, and S24/S25 validation are not provided by this evidence.
- Normal gameplay still uses the accepted block-arm route. No publication occurred.

Audio/haptic manual revalidation required: **NO**; no feedback semantics changed.
