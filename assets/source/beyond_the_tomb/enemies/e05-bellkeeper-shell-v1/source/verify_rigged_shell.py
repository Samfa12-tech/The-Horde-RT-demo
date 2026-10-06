import bpy,json,sys,argparse,math
from pathlib import Path
from mathutils import Vector,Matrix
import numpy as np
p=argparse.ArgumentParser();p.add_argument('--blend',required=True);p.add_argument('--output-dir',required=True);a=p.parse_args(sys.argv[sys.argv.index('--')+1:]);out=Path(a.output_dir);out.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=a.blend);scene=bpy.context.scene;rig=bpy.data.objects['Armature'];shell=[o for o in bpy.data.collections['ORIGINAL_MODULAR_SHELL'].objects if o.type=='MESH'];scene.frame_set(1);bpy.context.view_layer.update()
def points(o,eval=False):
    ob=o.evaluated_get(bpy.context.evaluated_depsgraph_get()) if eval else o
    return np.array([tuple(ob.matrix_world@v.co) for v in ob.data.vertices])
closed={o.name:points(o) for o in shell};report={'closed_rest_errors':{},'open_hinge_errors':{},'bone_count':len(rig.data.bones),'rigid_weight_checks':{},'status':'Static closed/open helper-bone binding proof; no dynamic body action certification'}
for o in shell:
    report['closed_rest_errors'][o.name]=float(np.linalg.norm(points(o,True)-closed[o.name],axis=1).max())
    report['rigid_weight_checks'][o.name]=all(len(v.groups)==1 and abs(v.groups[0].weight-1)<1e-6 for v in o.data.vertices)
cam=scene.camera;cam.location=(2.8,-5,2.2);cam.rotation_euler=(Vector((0,0,1.06))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=2.42;scene.render.image_settings.file_format='JPEG';scene.render.image_settings.quality=93
scene.frame_set(1);scene.render.filepath=str(out/'07_rigged_closed.jpg');bpy.ops.render.render(write_still=True)
scene.frame_set(60);bpy.context.view_layer.update()
for side,sign in [('L',1),('R',-1)]:
    o=bpy.data.objects['BreastShutter.'+side];pivot=Vector((sign*.25,-.123,1.38));R=Matrix.Rotation(math.radians(sign*95),4,'Z');expected=np.array([tuple(pivot+R.to_3x3()@(Vector(p)-pivot)) for p in closed[o.name]])
    report['open_hinge_errors'][side]=float(np.linalg.norm(points(o,True)-expected,axis=1).max())
scene.render.filepath=str(out/'08_rigged_open.jpg');bpy.ops.render.render(write_still=True)
report['pass']=max(report['closed_rest_errors'].values())<1e-4 and max(report['open_hinge_errors'].values())<1e-4 and all(report['rigid_weight_checks'].values());(out/'rigged-shell-validation.json').write_text(json.dumps(report,indent=2));print('RIGGED_SHELL_VALIDATION',json.dumps(report))
