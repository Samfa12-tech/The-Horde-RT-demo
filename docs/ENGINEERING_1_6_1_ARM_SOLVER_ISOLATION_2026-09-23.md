# Arm solver isolation — 2026-09-23

Base `362fc27ab2770894cb0cb58984978c30e0a6c66d`, not the research brief's
older `6f282bb`. Newer rejected sleeve trials and unrelated scratch are preserved.
This follows the owner's solver/control-first redirection. No mesh, weight,
Grip roll, gameplay authority, ray mask, shader or GPU ownership change.

## Proven defect and narrow correction

For shoulder `(0,0,0)`, target `(.2,0,0)`, pole `(0,1,0)`, lengths `.3/.4`,
the reachable elbow projection is `-.075`, not zero. The former clamp produced
elbow `(0,.3,0)` and forearm length `.360555`. The regression failed before the
change; allowing projection in `[-upper, upper]` yields
`(-.075,.290474,0)` and exact `.3/.4` lengths within 2 micrometres.
Reach clamping, epsilon, pole policy and reachability are unchanged.

The production skinned solver duplicates this calculation; changing the helper
alone would not repair it. A separate minimal bilateral GLB control reproduced
the same `.360555` defect through `SkinnedMeshAsset::EvaluatePlayerPose` and the
actual skinning/upload-vertex path. Its signed-projection correction passes.
The procedural block solver uses equal `.53/.53` lengths and cannot enter this
negative-projection case; it is unchanged.

`tests/SkinnedArmReferenceTests.cpp` constructs a test-only 40-vertex/64-triangle
arm strip with `.3/.4` chains, known inverse translations, named bilateral
mounts, an offset Grip, Idle/Walking clips and a 50/50 elbow ring. It checks bind
identity, nondegenerate outward winding, hand/grip sockets, elbow positions,
rigid upper/lower samples, and independently calculated linear-blend position,
normal and tangent results. Both `.2` folded and `.5` ordinary targets pass at
20-micrometre vector tolerance. It imports no production mesh, uses no Horde IK
helper to calculate expected answers, and is never admitted as a game asset.
This geometric control does **not** certify gauntlet anatomy or production cloth.

The pure helper also covers equal/unequal lengths, near inner/outer reach,
unreachable targets, rigid root transforms and collinear/nearly collinear poles.
No new library or production diagnostic framework was added.

## Independent ozz reference

Reviewed and ran the upstream two-bone solver pinned at
[`744eb9d99f606eda849acb0b1204f7a3dc20bca1`](https://github.com/guillaumeblanc/ozz-animation/blob/744eb9d99f606eda849acb0b1204f7a3dc20bca1/src/animation/runtime/ik_two_bone_job.cc).
The standalone MSVC Release probe uses a downward bind chain, downward pole,
`mid_axis=-Z`, `soften=1`, `weight=1`, and recomputes the hierarchy after applying
local rotation corrections, as in the upstream sample. It reports:

```
reached=1
elbow=(-0.075000063,-0.290473729,-0.000141833)
hand=(0.199999943,0.000000030,0.000000000)
lengths=(0.300000042,0.400000006)
```

The small out-of-plane residual is retained, not concealed; ozz uses approximate
SIMD rotations. This is endpoint/length reference evidence, not quaternion-byte
parity. Its strict inner-boundary `reached` policy differs from Horde's existing
epsilon/inclusive policy; neither that policy nor grip softening was transplanted.
Upstream remains an external MIT reference, not vendored/shipped code.

## Applicability to the current visual defect

Read-only native inspection of both admitted world GLB and frozen corrected-hand
candidate samples each arm at 121 phases in each Idle/Walking clip:

| Chain | Upper range (m) | Lower range (m) | Minimum `upper²-lower²` |
| --- | --- | --- | --- |
| Left | .276332557–.276332766 | .216430396–.216430679 | .029517442 |
| Right | .275409997–.275410324 | .205560163–.205560386 | .033595618 |

Both rigs agree. The actual solver uniformly stretches both segments together,
so these ratios cannot create the negative projection for any target distance.
Gameplay helper lengths are `.42/.40`; its only current production-source caller
is the socket-plan helper, with no render-scene call site. Do not attribute the
upside-down/right-hand appearance to this fixed generic edge case.

Fresh native RTX Debug captures of **all eight existing portrait poses** after
the fix are byte-identical to their preceding candidate images. Same world
`4050e15...`, viewmodel `194aab28...`, pose, extent and shader; no image tolerance
was changed. Local capture root: `C:/Dev/tmp/horde-signed-ik-portrait-20260923-b`.
The failed `-a` staging attempt used a nonexistent override filename and produced
no capture; it is not evidence. Both tracked production/runtime GLBs retain their
previous full hashes. Debug focused Vulkan/CPU-host CTests pass **11/11** after
building four initially missing test binaries; the initial incomplete run remains
in `reports/signed-ik-debug-ctest.log`, final in `...-final.log`. Fresh MSVC
Release focused gates also pass **11/11** (`reports/signed-ik-release-ctest.log`).

## Remaining layers and gates

- Anatomical mounting: source Right is established; current 105/145-degree test
  rolls are frozen and **not accepted**. Establish labelled thumb/palm/cuff/grasp
  frame and hand-to-grip relation, not another roll search or chain-label swap.
- Skinning/topology: independent control passes; production sleeve folds/rims
  remain open. Previous automatic caps remain rejected for intersections.
- Body visibility: current modelled route is arms-only primary, world secondary.
  Visible look-down torso/legs is explicitly required and **not implemented**.
  It needs a nonduplicating body/arm partition; no full-body-primary shortcut.
- Eight frozen captures do not replace live walking, looking, cuts, near-wall
  retraction, lantern transitions or owner anatomical/visual phone acceptance.
- Raised-lantern transport diagnostics remain open. No glass tuning or performance
  improvement is claimed here. The broader 1.6.1 scope remains active.

Audio/haptic manual revalidation required: **NO**; event authority, feedback,
listener inputs and playback are unchanged. No publication or paid generation.
