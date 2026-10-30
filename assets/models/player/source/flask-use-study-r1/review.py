"""Read saved source actions, validate playback, render finite authoring sequences.

No runtime/native export, game launch, production write or new geometry. The
render output is an animation preview at the source timeline, not measured FPS.
"""

from pathlib import Path
import argparse
import hashlib
import json
import sys

import bpy

ROOT = Path.cwd()
SOURCE = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--output-directory", type=Path, required=True)
parser.add_argument("--motion-receipt", type=Path, required=True)
parser.add_argument("--blend", type=Path, default=SOURCE/"flask-use-study-r1.blend")
args = parser.parse_args(sys.argv[sys.argv.index("--")+1:])
out = args.output_directory.resolve()
if out.exists():
    raise RuntimeError("Preserve prior preview")
out.mkdir(parents=True)
receipt = json.loads(args.motion_receipt.read_text())
source = args.blend.resolve()
assert hashlib.sha256(source.read_bytes()).hexdigest() == receipt["blendSha256"]
bpy.ops.wm.open_mainfile(filepath=str(source))
rig = bpy.data.objects["Armature"]
sword,flask = bpy.data.objects["StudySword"],bpy.data.objects["StudyFlask"]
scene = bpy.context.scene
scene.render.resolution_x,scene.render.resolution_y = 960,540
scene.render.resolution_percentage = 100
results = {}
for label,key,rows in (("FlaskUseStudy_ONLY","complete",receipt["samples"]),
                       ("FlaskCancelStudy_ONLY","cancel",receipt["cancelStudy"]["samples"])):
    for obj in (rig,sword,flask):
        obj.animation_data.action = bpy.data.actions[obj.name+"_"+label]
    frames = out/key
    frames.mkdir()
    max_grip,max_root,max_left = 0.,0.,0.
    first_pose = last_pose = None
    initial_left = None
    for i,row in enumerate(rows):
        scene.frame_set(i+1)
        bpy.context.view_layer.update()
        grip = rig.matrix_world@rig.pose.bones["RightGrip"].matrix
        wanted = row["gripTarget"]
        error = sum((grip.translation[a]-wanted[a][3])**2 for a in range(3))**.5
        max_grip = max(max_grip,error)
        for obj,field in ((sword,"swordRoot"),(flask,"flaskRoot")):
            max_root = max(max_root,max(abs(obj.matrix_world[r][c]-row[field][r][c])
                                       for r in range(4) for c in range(4)))
        left = rig.matrix_world@rig.pose.bones["LeftGrip"].matrix
        if initial_left is None:
            initial_left = left.copy()
        max_left = max(max_left,max(abs(left[r][c]-initial_left[r][c])
                                   for r in range(4) for c in range(4)))
        pose = {n:[list(r) for r in rig.pose.bones[n].matrix_basis]
                for n in ("RightArm","RightForeArm","RightHand")}
        if i == 0:
            first_pose = pose
        last_pose = pose
        scene.render.filepath = str(frames/f"{i:04}.png")
        bpy.ops.render.render(write_still=True)
    endpoint = max(abs(first_pose[n][r][c]-last_pose[n][r][c])
                   for n in first_pose for r in range(4) for c in range(4))
    results[key] = {"renderedFrames":len(rows),"sourceHz":60,
                    "lastSampleSeconds":rows[-1]["time"],
                    "maximumSavedPlaybackGripErrorMetres":max_grip,
                    "maximumPropMatrixDifference":max_root,
                    "maximumLeftMatrixDifference":max_left,
                    "startEndRightBasisDifference":endpoint}
    # These are transform-consistency checks, not visual/contact tolerances.
    assert max_grip < 1e-5 and max_root < 1e-5 and max_left < 1e-6 and endpoint < 1e-5
assert hashlib.sha256(source.read_bytes()).hexdigest() == receipt["blendSha256"]
assert all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==sha for p,sha in receipt["sourceHashes"].items())
result = {"scope":"Source-only Blender action playback; no native, RT or gameplay acceptance",
          "blendSha256":receipt["blendSha256"],"sourceBytesUnchanged":True,
          "engine":"Blender Workbench","resolution":[960,540],"sequences":results,
          "renderingPacedInRealTime":False,"videoMustUseSourceHz":60,
          "leftLightOrCombatChanged":False,"runtimeExported":False}
(out/"review-receipt.json").write_text(json.dumps(result,indent=2)+"\n")
print(json.dumps(result,indent=2))
