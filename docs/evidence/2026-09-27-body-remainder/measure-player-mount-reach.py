"""Geometric upper bound for one fixed native pose; does not tune the solver."""
import json
import math
from pathlib import Path
import sys

nodes = json.loads(Path(sys.argv[1]).read_text())['nodes']
manifest = json.loads(Path(sys.argv[2]).read_text())
capture = manifest['captures'][0]
lines = Path(sys.argv[3]).read_text().splitlines()
matrix = [float(value) for value in lines[1].split()[2:]]
if len(matrix) != 12:
    raise ValueError('Missing native model-to-world transform')
def model_point(world):
    delta = [world[i] - matrix[4*i+3] for i in range(3)]
    return [sum(matrix[4*i+j]*delta[i] for i in range(3)) for j in range(3)]
camera = capture['camera']
eye = model_point([camera['x'], .70, camera['z']])
grip = model_point(capture['visibility']['rewardGrip']['finalGripPosition'])
upper = math.dist(nodes['LeftArm'], nodes['LeftForeArm'])
lower = math.dist(nodes['LeftForeArm'], nodes['LeftHand'])
socket = math.dist(nodes['LeftHand'], nodes['LeftGrip'])
# Existing native SkinnedMeshAsset maximum, not a proposed increase.
maximum = 1.75*(upper+lower)+socket
shift = nodes['Head'][2]-eye[2]
shifted_grip = [grip[0], grip[1], grip[2]+shift]
print(json.dumps(dict(checkpoint=capture['checkpoint'], cameraModel=eye,
    headJointModel=nodes['Head'], headJointIsNotAnEyeLandmark=True,
    upperLengthMetres=upper, lowerLengthMetres=lower, handToGripMetres=socket,
    existingMaximumChainStretch=1.75, maximumShoulderToGripMetres=maximum,
    currentShoulderToGripMetres=math.dist(nodes['LeftArm'], grip),
    bodyBackwardTranslationToHeadJointPlaneMetres=shift,
    translatedShoulderToGripMetres=math.dist(nodes['LeftArm'], shifted_grip),
    translatedReachExcessMetres=math.dist(nodes['LeftArm'], shifted_grip)-maximum,
    qualification='Upper bound with fully straight chain and optimally aligned socket; actual oriented reach can be smaller. No production transform changed.'), indent=2))
