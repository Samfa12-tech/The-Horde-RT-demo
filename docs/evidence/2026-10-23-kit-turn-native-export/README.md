# Kit step-turn: bounded contact cleanup and native export

Base `0cf8829c847a375eedaa985f1fec58f71d137e65`. **Production motion, bindings,
accepted GLBs, saves, dialogue and renderer are unchanged.** The separate source
study remains unadmitted for gameplay.

## Reviewed previews

[Left motion](left-cleanup-preview.mp4) · [right motion](right-cleanup-preview.mp4)
· [left contact exchange](foot-left-0024.png) · [right support view](foot-right-0035.png).

The full-body sequences and close foot frames were inspected against the
[preserved first proof](../2026-10-23-kit-step-turn-proof/README.md). The two steps
and restrained pelvis transfer read clearly. Small toe creep is visible around
the support exchange. One cleanup bakes exactly two bounded rigid sole-pose fits
per foot through the existing trajectory, including swing, avoiding a correction
jump at landing. Geometry, weights and imported bind transforms are unchanged.
No engine solver or second skin pass is added.

The saved take is reopened and sampled at 120 Hz (91 samples/direction): maximum
support-sole drift falls **12.125 -> 6.856 mm**; no geometric leg stretch is
required. Idle-zero endpoints remain within 0.001 mm. The close views show less
toe creep with the original weight transfer retained. The remaining boot
deformation is recorded, not declared a failure solely for being nonzero or
silently granted a new tolerance. **No further contact tuning loop is run.**

These are Blender Workbench **offline authoring studio previews**, not native RT
screenshots, owner artistic acceptance, physics balance or phone evidence. The
0.75-second / 45-degree takes remain candidate authoring choices.

## Native export and loader proof

The [source folder](../../../assets/models/characters/source/kit-step-turn-proof-v1/README.md)
keeps the original proof and adds `kit-step-turn-cleanup.blend`, the append-only
export recipe and a `native/` proof. The exporter preserves the pinned runtime
GLB's entire old binary prefix and original nodes, skins/inverse binds, meshes,
materials, images, accessors, buffer views and seven animations. Only new joint
tracks/buffer payloads are appended. No Blender geometry/bind round-trip occurs;
the original pinned-master intake recipe is unchanged.

The static Blender-to-glTF joint basis is calibrated against original Idle zero.
The preview wrapper's yaw is removed from joint poses and recorded separately in
`root-motion.json`. Its axis is **asset-space +Y model yaw**. The existing
`CompanionModelYaw` conversion requires the negative delta in logical facing.
It must not also be baked into the armature or applied twice by an integrator.

The existing native `SkinnedMeshAsset` loader and PBR skinning are exercised through
an optional `--turn-proof` entry on the existing Kit intake executable. It uses
an isolated caller-selected clip set; the game gains no turn bindings. The normal
CTest entry and its assertions are unchanged; no additional full-suite execution,
CI exclusion or deadline change is introduced.

| Native check | Result |
| --- | --- |
| Exported takes at 38 sampled times, 142 sole/body reference vertices each | **5,396 references**, max error **0.00380 mm** after one yaw application |
| All-vertex start/end against original Idle zero, both directions | Max **0.000616 mm** |
| Missing/doubled yaw against the same references | About **330 mm** max mismatch |
| Deliberately doubled external yaw fixture | Validator returns **1**, expected rejection retained |
| Original seven clips, 85 raw/grounded PBR poses | **2,290,325 vertex-poses byte-identical** |
| Static export / reference / root curve / manifest reproduction | Byte-identical in a fresh output directory |

The optional checker retains a 0.1 mm coordinate limit for these float32 export
comparisons, finite/unit/orthogonal tangent-frame checks, all-vertex endpoints
and explicit wrong-yaw controls. It does not reduce any existing acceptance gate.
The first original-clip comparison stopped because the candidate directory lacked
its manifest. The unchanged original manifest was staged and the comparison then
passed; the original failed log remains retained. It was an asset-staging issue,
not a gameplay defect or permission denial.

This is **CPU native import/pose correspondence**, not live animation state,
in-game RT visual quality or performance acceptance. Only 38 times / 142 reference
vertices receive cross-tool intermediate position comparison; endpoints and PBR
frame validity inspect the full unique-vertex mesh. Arbitrary Idle phases,
speech overlays, repeated/interrupted/partial turns, slopes and joins remain open.

## Explicit remaining integration gate

Review the previews and whether the remaining contact motion is acceptable at
game distance. Then, in a separately scoped integration slice, add explicit turn
selection and transient phase to the shared simulation/immutable actor snapshot,
including both serial and optional-worker cache identities. Apply the declared
model-yaw curve once, complete supported foot exchanges, and evaluate turn-then-
walk/short acceleration while retaining current cruising speed and distance phase.
Do not alias production Attack/Dead bindings or enable these files as a shortcut.
Use the retained native RT turn/tether witnesses for actual joins, terrain,
pause/restore and dialogue-source checks before gameplay admission.

## Custody, packaging and CI

All 151 parked owner files and the original master/runtime hashes are preserved.
Android `build.gradle` and Windows `package-alpha.ps1` still select the literal
original runtime Kit path; neither packages these source-only proof files.
Provenance remains the original owner-confirmed Meshy source plus offline motion
authoring; no paid generation or new asset entitlement is claimed.

The preceding exact [0cf8829c CI40038985047](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/40038985047)
is terminal **all six jobs successful**. A subsequent backup needs its own exact
CI result; this statement does not certify an unrun newer head. Compact hashes,
metrics, negative control and raw-log references are in [receipt.json](receipt.json).
Ignored working evidence stays in `reports/kit-turn-native-export-20261023/`.

Audio/haptic manual revalidation required: **NO** for this source-only proof.
No phone, foreground activation, production integration, merge or release.
