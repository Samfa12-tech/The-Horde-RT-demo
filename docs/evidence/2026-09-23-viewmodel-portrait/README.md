# Native portrait investigation — Phase 3 remains open

These are eight actual RTX `RayTracingPipeline` captures of the corrected Right-source
candidate, not Blender previews, Android runs or accepted art. The added Debug-only
`--capture-portrait` option changes the capture surface from 960x540 to 540x960.
It does not change gameplay, the authored camera/pose, prop placement, render scale,
shaders or normal interactive-window sizing. The existing manifest records actual
dispatch/swapchain dimensions. Portrait and landscape are different pixel workloads;
do not use them as matched performance or glass-counter comparisons.

## Reproduction and checks

Source base: `67a3ea1` plus the portrait-only Windows change. Debug executable SHA:
`23b774c949fcef3977109b43df6b760b39061d6ca44597cdfd795075a879c312`.
Release executable SHA:
`d732349c2e6c857292be570deed49afadbae0eb01ffd94131e77d5874bb71dd6`.
Both build with MSVC. `tools/test-windows-capture-launch.ps1` passes four guards:
portrait requires capture, duplicate flag rejection, no benchmark-only use, and
Release rejection. Each exits before Vulkan initialization. No shader edit occurred.

From a staged Debug executable directory, use:

```text
HordeLanternRT.exe --capture-showcase NEW_OUTPUT --development-checkpoint player-viewmodel-grips --capture-portrait
```

The stage receipt binds the executable and exact assets. Candidate generation uses
Right source, Left/Right Grip rolls 105/145 degrees and continuous elbow weights.
The fitted-sleeve experiment is not used. The paired world/viewmodel receipt is
retained here; neither accepted runtime file changed. Original local roots are
`C:/Dev/tmp/horde-right-portrait-20260923-a` and
`C:/Dev/tmp/horde-right-portrait-matrix-20260923-a`.

All eight manifests report complete RT-storage captures and honest native RT
presentation at 540x960. The unchanged landscape control at
`C:/Dev/tmp/horde-right-landscape-control-20260923-a` is byte-identical to the prior
grips PNG: SHA `9d66c3fbf2592039ccda7bf1ed8507eb9fd5328f65938d9297eaf8af86c9112c`.
That control protects the default capture path; portrait does not replace the old
image gate. Compiled executables and posed OBJs are local-only; the fit report binds
the latter hashes, not archived copies.

## What the evidence does and does not show

Direct review still finds poor sleeve/cuff transitions and incomplete hand framing.
Do not call the anatomy, proportions or surface quality accepted. Exact rigid-skin
fit places the cuff/solved-forearm direction dot in Left 0.731–0.998 and Right
0.431–0.935 across the eight poses (worst Right: upward slice). These are geometric
reference measurements, not a handedness oracle or an art threshold. Source Right
and Left reflection remain explicit; labelled rig chains alone cannot prove anatomy.

The raised-lantern portrait has two primary volume-budget/transport-overflow events;
high/low report 1,041/721 secondary dielectric terminations. Preserve those diagnostics
as open glass findings. Different aspect/pixel workload means these counts cannot
be compared directly with the earlier landscape's one overflow. No physical transport
or tolerance was weakened to produce the images.

Phone/owner acceptance, live motion/retraction and final surface finishing remain
open. ADB was unavailable at the prior exact package check; this Windows evidence
does not change device compatibility status. Audio/haptic manual revalidation:
**NO**, because this capture option does not change gameplay feedback or playback.
