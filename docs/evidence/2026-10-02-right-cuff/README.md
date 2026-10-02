# Right sword glove / sleeve attachment

Owner reopens this specific larger-screen Windows finding on October2, supplying
[rest](owner-rest.png) and [motion](owner-motion.png) crops and explicitly calling
the glove/sleeve misaligned. The complete Shipping218ab102 build was opened for
that interaction. The crops are owner evidence, not exact deterministic poses.
All other accepted player work remains accepted; no broad arm/IK/roll search.

## Demonstrated local facts

Admitted pair remains world `f2c3f62b...` / view `6f06d77e...`. Sleeve cuff follows
ForeArm after the previous Hand-weight removal; the entire separate gauntlet,
including its proximal cuff, follows Hand. These are not a shared mesh boundary.
Retained native opening CPU upload fits both bone maps with less than0.2um
residual. Wrist origins agree to0.0002mm, but the transformed forearm axis differs
34.9384 degrees; full relative rotation is78.7058 degrees. This rules out a
misplaced wrist origin in that pose, not all possible animation defects.

Source-to-runtime UV correspondence fits5,610 right-gauntlet vertices with
less than0.24um maximum residual. The authored terminal +X cuff section centre
is50.5mm radially off the rig forearm axis in bind space. Its cuff axis itself
is about10 degrees from that axis. A grip-mounted hand can therefore be correct
at the hilt while its rear cuff is not fitted to this rig's sleeve.
These measurements support a local cuff fit/skinning defect; they do not by
themselves prove visual acceptance. No sword, Grip, roll, IK target, animation,
sleeve, camera, shader or resource-ownership change.

## Finite contrasts and current disposition

| Contrast | Exact assets / result |
| --- | --- |
| A admitted control | world f2c3f62b / view6f06d77e. Current native rest reproduced. |
| B weights only | world0fcbb41b / viewb925e313. Anatomical wrist-to40mm proximal Hand/ForeArm blend,2,046 changed /3,564 distal vertices protected. Paired C++ admission passes, but CPU nearest-cloud median worsens7.44→19.34mm and native rest looks worse. REJECTED; no production change. This cloud metric is not a surface-gap/coverage gate. |
| C local bind-space fit + cuff articulation | world46a88dac / vieweaa0db3a. Terminal cuff centre fitted onto forearm axis with50.24mm radial correction,58.43mm smooth wrist-to-cuff field.2,023 proximal /3,587 protected distal vertices. Exact source base guard passes. Fourteen pure tests and actual paired C++ addressing/pose/tangent/grip/invalid-pose admission pass. Owner accepts the laptop live join through movement; engineering integration follows. Exact-device acceptance remains open. |

All variants preserve geometry count and gameplay grip authority. Pure tests
protect chirality/UV winding. Independent raw-order-independent artifact checks
pass for the unchanged5,532-triangle view sleeves, all four non-gauntlet world
surfaces and5,610 left-gauntlet vertex records, including skin weights. Both
two-hand gauntlet primitives retain8,838 triangles with exact UV topology and
winding. The right cuff alone changes2,023 positions/2,017 exported weight rows;
3,587 distal Hand-only position/UV/weight records remain exact. Right cuff
normals/tangents change with the fitted surface as expected. These protect scope,
not live visual acceptance. [Comparison](protected-surfaces.json).
C recomputes real cloth/gauntlet surface normals through the
existing Blender export route, not a shader/camera visual substitute. Sleeves
are unchanged. The optional processor flag is not a production default.

Native evidence uses retained Debug exe5c6a7782 (runtime22d6633), RTX5050 Laptop,
Diagnostic/High pipeline,540x960, fixed animation time0, existing development
checkpoints137 and136/141. Nine total captures: A/B/C forward, A/C portrait grips,
A/C portrait look-down, then A/C landscape grips.136 and141 are the same camera/pose in this source, so their images
do not establish independent pose coverage. Near-edge framing remains a limit;
do not call these continuous live-motion acceptance. Candidate staging restores
the exact admitted pair after each contrast. No phone use.

The two new960x540 captures address a specific validity gap: the prior portrait
framing did not represent the owner's laptop viewpoint. They use the same136
camera/pose and retained binary, not a new animation or camera contract. Both
native RT manifests complete/exit0; the fitted join replaces the offset open rim
in this pose. [Control](control-landscape.png), [candidate](fitted-landscape.png).
This remains visual evidence for one fixed pose, not an owner verdict.

## Guarded generation and unchanged-artifact reuse

The original palm-contact check was strengthened after independent review:
select protected points from ORIGINAL positions within34mm of the infinite
authored-handle axis, not a post-deformation sphere around its centre. Require
zero ForeArm share and exactly unchanged positions for that source roster;
the distal wrist half-space is also unchanged. Two added tests enforce contact
selection and malformed input rejection. [Fourteen-test result](pure-tests.txt).
Guarded Blender generation exits0 and produces the exact same world46a88dac /
vieweaa0db3a / manifest556f8b4f as the previously captured C. No shader/native
rebuild or repeated capture is required by that guard change. The retained
native admission was executed once against this guarded pair to retain its
previously missing durable result: [receipt](guarded-native-admission.txt).
Retained smoke executable SHA256
`4529673b559089390489f42bae8e03295d7f747f26f00cff0aa4ece35a688310`;
command: `horde_rt_skinned_character_smoke.exe --validate-viewmodel-admission
C:/Dev/tmp/horde-right-cuff-fit-20261002-guarded/gothic-traveller-viewmodel.runtime.glb
assets/models/player/viewmodel/runtime/asset.manifest.json
C:/Dev/tmp/horde-right-cuff-fit-20261002-guarded/world-seam-reconciled.runtime.glb`.

Generated C is retained locally at `C:/Dev/tmp/horde-right-cuff-fit-20261002-guarded`
(byte-identical earlier output `C:/Dev/tmp/horde-right-cuff-fit-20261002-a`).
Reproduce once only if artifact validity is lost, with Blender5.2 single-thread
and this exact paired recipe (not a request to rerun completed generation):

```text
blender --background --threads 1 --python-exit-code 1
  --python tools/process-player-viewmodel-runtime.py -- NEW_OUTPUT_DIRECTORY
  --gauntlet-source-hand Right --grip-roll-degrees 105 145 --blend-elbows
  --gauntlet-scale .099 --reconcile-sleeve-seams --body-remainder
  --retain-upper-torso --reconcile-segmented-seams --fit-right-cuff
```

Source,
receipt and finite capture manifests are retained here; no experimental GLB is
promoted at this checkpoint. A separate live-review stage is prepared at
`C:/Dev/tmp/horde-right-cuff-live-20261002/HordeLanternRT.exe`, using the unchanged
Shipping218ab102 binary and only the paired C geometry. It is explicitly
investigation-only, not the accepted package or a performance comparison.
It was opened normally as PID29916, no benchmark/capture switches. Its report
confirms Shipping/High hardware RT presentation, but the last recorded state is
paused/tick0. Later process inspection finds it closed; Codex did not terminate
it. The paused report alone does not establish a live-motion test. Do not
reopen automatically after resumption or relabel paused timing as gameplay FPS.
The owner subsequently answers the explicit rest/walk/attack/parry review:
"Join looks natural through movement". This closes the requested Windows live
cuff review, not a phone check or all renderer acceptance. A further owner note
confirms "the hand looks good now". The exact paired C assets are now integrated
into the engineering runtime with truthful processing receipts and pinned
manifest tests. Five affected native contracts pass (35.70s): manifests, skinned
smoke, semantic fixtures, paired admission and pose fixtures. The strengthened
cuff receipt test separately passes after its addition. [Result](integrated-native-contracts.txt).
The comparison tool now points to the retained original control and pins both
hashes: rerun once only to check control validity after canonical asset replacement,
not a new contrast. Historical A remains in Git/LFS and that control stage.
Offline pair regeneration can opt into the exact recipe with `-FitRightCuff`;
its default still reproduces the older pair. Parser validation passes; no new
Blender generation. Current next step: prepare the changed phone build while
exact-device owner acceptance remains open. No further
weight/roll search or repeated captures without a specific validity gap.

Audio/haptic manual revalidation required:NO: this candidate alters only offline
cuff geometry/skinning, not gameplay grips, events, listener/source or playback.
No merge, signing, release or publication.
