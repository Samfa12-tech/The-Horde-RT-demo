# Torch wall blackout diagnosis — 22 October 2026

**Confirmed desktop defect; proposed fix withdrawn.** At legal opening-room
positions, the carried Rag torch's final uploaded Light can cross the physical
wall while remaining active. A whole-hand envelope fit removes the blackout,
but also removes a previously visible torch from an already-lit close-wall view.
The experiment is retained for review; production code and compiled tests return
to `9136ff0be99b315d565231d2f672d2c90c27c6e6`.

## Evidence and scope

The owner's original landscape clip contains textured floor/wall pixels during
the dark interval, with HUD still visible. The original file is retained locally,
not committed. Its SHA-256, encoded-frame analysis and limitations are in
[the receipt](receipt.json). The clip's exact source/build/settings and camera
pose are unknown. **This desktop reproduction does not establish its exact cause.**

Windows Debug uses the NVIDIA RTX 5050 Laptop GPU, real RayTracingPipeline,
960×540 output/internal dimensions, High shader/fire, Higher shadows, Mist On
and Dust High. This is a geometry/lighting investigation, not a phone-matched
performance comparison. No phone operation occurred.

The temporary diagnostic starts current chapter rules and moves with ordinary
simulation axes, without importing historical progression/completion flags.
It renders the approach and captures frozen look poses with owning completed
submission/presentation identities, uploaded fire lights and hashed RT readbacks.
It is hidden diagnostic evidence, not physical input or full natural-route
acceptance. [Retained method](probe-method.cpp.txt).

All 48 control/candidate pairs have verified matching camera, simulation tick,
pipeline/quality, Mist/Dust and emitter strength. Image hashes and display-crop
means are in [matched-witnesses.csv](matched-witnesses.csv). The metric is a
BT709-weighted display-RGB crop mean, not linear radiance or engine FPS.

| Witness | Control | Candidate | Interpretation |
| --- | --- | --- | --- |
| Right wall, diag13; camera X 1.59908, Z −4.9, yaw 1.9635 | Light X 1.92248; active power 1.47749; crop mean 0.841 | Light X 1.51957; same power; mean 50.739 | Light was beyond the X 1.85 wall and X 1.92 rear shell. Warm stone becomes visible. |
| Left wall, diag21 | Light X −1.91860; power 1.51898; mean 1.209 | Light X −1.51957; same power; mean 63.555 | Mirrored blackout removed. |
| Already-lit right-wall diagonal, diag11; tick 540 | Visible torch; Light X 1.76852, power 1.50104952 | Torch disappears; Light X 1.50748, identical power/tick | **New framing regression: reject the candidate.** |

Local raw images and ledgers remain under `reports/dark-world-20261022/`:
`side-wall-probe/`, `side-wall-candidate/` and `current-route-probe/`.
They are not added to Git. Centre-room inspection did not reproduce the blackout.
The original checkpoint200 capture remains rejected for zero primary arm pixels;
its warm image is not an admission pass.

After correcting an unnecessary second roof-lowering query, three fresh native
wall witnesses confirmed the light positions. Their sparse look sequence uses
different ticks, so they carry **no matched pixel/timing claim**. The final
unhooked candidate binary also passes the existing checkpoint198 portal fixture
with 2,151 primary player pixels; its [manifest](portal-manifest.json) does not
admit close-wall framing. All executable/source identities are in the receipt.

## Validation and disposition

The independent new test loads actual Rag vertices/Grip/Flame/Light sockets and
checks complete bounds and shadow endpoints against a separate raw room-union
oracle. It covers 339 poses, joins and both turning sweeps. Existing overhead
and visibility gates were preserved.

- [Original runtime red](original-runtime-red.txt): wall containment and turns
  fail. The earlier [incorrect nominal-centre fixture assumption](initial-fixture-failure.txt)
  is also retained; it was replaced by measured original resolved coordinates.
- [First candidate](candidate-centre-regression.txt): eight checks pass; the
  centre-pose regression fails and was corrected in the experiment.
- [Corrected candidate](corrected-candidate-batch.txt): all eight executed checks
  pass, including actual final rig, sockets, fire and gameplay. Transition did
  not launch (`BAD_COMMAND`, unspecified process error); its
  [single focused retry passes](transition-retry.txt). This is not a green
  nine-test batch. No assertion or deadline was weakened.
- Restored production app and focused contracts are verified separately in
  `restored-tests.txt`. Candidate validation does not admit the visual regression.

The [compressed withdrawn patch](withdrawn-candidate.patch.gz) preserves both
runtime and test changes; decompress it only for review/reproduction. It is not
applied. The diagnostic hook was removed byte-for-byte. No source/asset,
renderer quality, settings, campaign data or save-schema change is adopted.
Raw clip, PNGs, OBJ dumps, binaries and owner-untracked assets are excluded.

Next bounded refinement: jointly preserve physical mesh/Flame/light-sample
clearance **and usable close-wall torch presentation**, retaining diag11 alongside
the two dark witnesses. Keep the complete flame gate and ordinary centre pose.
Do not relocate only the light or compensate with extra illumination.

The dark-world issue remains open. Phone cause/acceptance, natural Keeper-to-route
recovery, actual audio listening, warmed-phone performance, the Debug campaign
30-second gate (prior direct completion 39.264 seconds), and PR27 conflicts remain
separate open items. All base integration/merges remain unapplied.

Audio/haptic manual revalidation required: **NO** for this evidence-only checkpoint;
no runtime change is adopted. Existing listening acceptance remains pending.
