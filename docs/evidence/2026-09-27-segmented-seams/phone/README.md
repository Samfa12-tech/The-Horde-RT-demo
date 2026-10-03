# Exact SM-S948B segmented-seam candidate

APK SHA256 `66de46e6a045dacea88864adb153b2d4505bb640c9dbab0111262018e6a31816`;
world `f2c3f62b...`, unchanged view `6f06d77e...`. Installation retained app data;
pulled APK matched exactly. Runtime code/native ELF is unchanged from the accepted
normal-range camera build; only the paired world asset changes. The asset repair
is committed as `6dd9c1f37cf1a9546fc36fd0fd9a657864b656ca`.

## Capture/lifecycle evidence

`reports/android-showcase-runs/run-20260927-134811` passed four captures plus
Home/resume. Source recorded at run launch is `cc9b898...` with a dirty tree:
the then-uncommitted asset-processing work became `6dd9c1f`. Exact APK/asset hashes,
not an inferred clean-source label, identify this evidence.

All four captures report `RayTracingPipeline`, actual RT presentation,
`modelled-viewmodel`, `AnatomicalBody`, dedicated primary ownership and zero
reported socket error. Strict ASTC is retained. Scale is 75%, RT extent1080x2235.

| Checkpoint | Pitch parameter | Transport overflow | Shadow overflow |
| --- | ---: | ---: | ---: |
| 141 look down | -.32 | 0 | 0 |
| 145 lantern low/look down | -.32 | 0 | 0 |
| 146 lantern high/look up | +.28 | 8 | 0 |
| 144 lantern low/parry | -.30 | 1 | 0 |

The glass counts match the previous phone candidate and remain open findings.
The runner used `-Mode Benchmark -Checkpoints @()` with capture selection: no
route replay or timing samples were requested. Do not label this a benchmark,
sustained-performance or full-route pass.

## Continuous live sequence

A fresh non-capture `player-viewmodel-lantern-low` setup completed its normal
three-window/480-frame measurement before Continue dismissed the ending card.
Fresh UI inspection verified the gameplay controls. `live-inputs.json` records
actual touch requests, and `live-input-recipe.ps1` retains the scoped sequence.

Unedited `live-motion.mp4` is 44.937011 seconds, SHA256
`7749d2ac0c7c3b4e22fcc92cfa62db9535aa04f04c87ded90174719a3d5009a7`.
720x1560 is the video encoding resolution, not a changed RT setting.
Reviewed 1 Hz overview and 8 Hz low/high action samples show walking, looking,
parries/sword swings, and high/low lantern transitions. No obvious crossing or
new seam opening was seen in those samples. Dark cloth and touch controls limit
fine surface inspection; this is not every-frame/every-pose or owner acceptance.
The recording has no slow-charging toast obscuring the previous run's view.

The body-presence/mirror gate is not proven by this footage alone. The owner subsequently
checked the inner bicep with lowered lantern and reports it "looks correct now".
Record the armpit/inner-bicep appearance as accepted on this installed candidate.
Prior wrist acceptance remains recorded for the previous APK; its
viewmodel/cuff and mounting are unchanged, without claiming acceptance of all
new world-body geometry. Preserve this accepted arm direction. Do not retire the
production fallback before the remaining player acceptance gates.

The owner then explicitly accepts the current look-down angle for gameplay,
with no noticeable occlusion problems. Visible feet are unnecessary at this
stage; steeper look is deferred until a gameplay reason warrants it. Close
normal look-down/body-presence acceptance on this exact installed candidate;
leave the camera and arm setup unchanged. This is owner evidence, not a claim
that the footage shows feet or that the earlier extreme-camera study passed.
The owner subsequently reports the mirror looks good, closing mirror appearance
acceptance too. Reusing a forward walk for sideways/backward movement is noted
for a later directional-animation pass, explicitly outside this1.6.1 acceptance
gate. See `FUTURE_WORK.md`. These owner reports do not certify a different APK,
glass correctness, sustained performance or unavailable S24/S25 devices.

The phone remains on this APK and Home-backgrounded after recording. No system
settings, render quality or lighting were changed. This does not certify S24/S25.
Audio/haptic manual revalidation required: **NO new check for this topology-only
repair**; the separately recorded anatomical-profile feedback gate is unchanged.

Source `6dd9c1f` passes branch CI36292515083 and PR CI36292517668. This is host
coverage, not physical RT acceptance or a release authorization.
