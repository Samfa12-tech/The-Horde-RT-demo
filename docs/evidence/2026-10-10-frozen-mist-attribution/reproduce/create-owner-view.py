from pathlib import Path
import shutil
root=Path.cwd(); original=root/'reports/frozen-mist'; out=original/'owner-view'
out.mkdir(exist_ok=True)
assert not (out/'prepare.py').exists(), 'Preserve prior attempts'
for directory in ['shaders','baseline']: shutil.copytree(original/directory,out/directory)
shutil.copy2(original/'shader-metrics.json',out/'shader-metrics.json')
s=(original/'quality/prepare.py').read_text().replace("out=root/'reports/frozen-mist/quality';", "out=root/'reports/frozen-mist/owner-view';",1)
# Approximate the supplied view from the other side of the rope. This is not an exact owner camera recovery.
s=s.replace('.cameraX = -33.0f, .cameraZ = -17.5f, .yaw = -2.805f, .pitch = -0.24f,', '.cameraX = -34.4f, .cameraZ = -13.5f, .yaw = 0.10f, .pitch = -0.24f,',1)
s=s.replace('.rewardPose=DevelopmentRewardPose::HeldHigh}', '.rewardPose=DevelopmentRewardPose::HeldLow}',1)
(out/'prepare.py').write_text(s,encoding='utf-8',newline='\n')
s=(original/'quality/capture.py').read_text().replace("out=root/'reports/frozen-mist/quality';", "out=root/'reports/frozen-mist/owner-view';",1)
(out/'capture.py').write_text(s,encoding='utf-8',newline='\n')
print('Prepared explicitly approximate owner-facing view; pose and frozen state recorded independently.')
