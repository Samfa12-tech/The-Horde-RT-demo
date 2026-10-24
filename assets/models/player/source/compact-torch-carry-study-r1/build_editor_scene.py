"""Local editable study, not a runtime clip or rendered-game evidence."""
from pathlib import Path
import bpy,hashlib,json,math,sys
from mathutils import Vector
root=Path.cwd();base=root/'reports/compact-torch-carry-20261024'
destination=base/'compact-carry-r1.blend'
assert not destination.exists(), 'Preserve prior derivative'
spec=json.loads((base/'compact-carry.pose.json').read_text())
for name,expected in spec['sourceHashes'].items():
 assert hashlib.sha256((root/name).read_bytes()).hexdigest()==expected,name
bpy.ops.wm.read_factory_settings(use_empty=True)

def collection(name):
 col=bpy.data.collections.new(name);bpy.context.scene.collection.children.link(col);return col
def move(objects,col):
 for obj in objects:
  for old in list(obj.users_collection):old.objects.unlink(obj)
  col.objects.link(obj)

# Preserve an imported original rig as an editable authoring reference. It is
# not presented as a solved Blender action or allowed to replace native solve.
reference=collection('Original rig reference - unchanged bind - hidden')
before=set(bpy.data.objects)
bpy.ops.import_scene.gltf(filepath=str(root/'assets/models/player/runtime/gothic-traveller-lod0.runtime.glb'))
objects=set(bpy.data.objects)-before;move(objects,reference)
rigs=[o for o in objects if o.type=='ARMATURE'];assert len(rigs)==1
rig=rigs[0];rig['source_sha256']=spec['sourceHashes']['assets/models/player/runtime/gothic-traveller-lod0.runtime.glb']
rig['binding_status']='Original reference only; native endpoint solve is the separate measured mesh'
reference.hide_viewport=True;reference.hide_render=True

for mode in ('control','candidate'):
 col=collection('Native solved endpoint - '+mode)
 before=set(bpy.data.objects)
 bpy.ops.wm.obj_import(filepath=str(base/('cpu-'+mode)/'diag13-wide.obj'))
 imported=set(bpy.data.objects)-before;move(imported,col)
 for obj in imported:
  obj['provenance']='Actual native imported-rig solve, no edited vertex positions'
  obj['source_obj']='cpu-'+mode+'/diag13-wide.obj'
  obj.display_type='SOLID'
 if mode=='control':col.hide_viewport=True;col.hide_render=True

targets=collection('Editable endpoint intent - not a transition or authored clip')
for sign in (-1,1):
 target=bpy.data.objects.new('Grip intent '+('left' if sign<0 else 'right'),None)
 target.empty_display_type='ARROWS';target.empty_display_size=.12
 target.location=(sign*spec['gripInView'][0],spec['gripInView'][1],-spec['gripInView'][2])
 target['grip_y_in_view']=[sign*spec['gripYInView'][0],spec['gripYInView'][1],spec['gripYInView'][2]]
 target['status']='Endpoint intent only; read compact-carry.pose.json and failed native oracle'
 targets.objects.link(target)

camera_data=bpy.data.cameras.new('Shader-frustum review camera')
camera=bpy.data.objects.new(camera_data.name,camera_data);bpy.context.scene.collection.objects.link(camera)
camera.location=(0,0,0);camera.rotation_euler=(0,0,0)
camera.data.type='PERSP';camera.data.sensor_fit='VERTICAL'
camera.data.angle_y=2*math.atan(.74/1.22);camera.data.clip_start=.002;camera.data.clip_end=100
scene=bpy.context.scene;scene.camera=camera
scene.render.resolution_x=960;scene.render.resolution_y=540;scene.render.resolution_percentage=100
scene.render.pixel_aspect_x=1/.74;scene.render.pixel_aspect_y=1
scene['evidence_boundary']='Editable native-mesh authoring scene only. Actual RT images are retained separately. No clip/runtime admission.'
scene['candidate_status']='Rejected: complete wall envelope and square framing fail; no transition trial'
text=bpy.data.texts.new('README - evidence and edit boundary')
text.write(json.dumps(spec,indent=2)+'\n\nNo production mesh, bind, clip, camera, collision or light change. Native solved mesh is a reference, not a new rig animation.\n')
bpy.ops.wm.save_as_mainfile(filepath=str(destination))
receipt={'blenderVersion':bpy.app.version_string,'file':destination.name,
 'sha256':hashlib.sha256(destination.read_bytes()).hexdigest(),'bytes':destination.stat().st_size,
 'sourceHashesUnchanged':all(hashlib.sha256((root/p).read_bytes()).hexdigest()==h for p,h in spec['sourceHashes'].items()),
 'referenceRigBones':len(rig.data.bones),'runtimeClipExported':False,'renderedImageClaim':False}
(base/'editor-scene-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(receipt),flush=True)
