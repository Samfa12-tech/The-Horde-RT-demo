import bpy,pathlib,json,math,argparse,sys
from mathutils import Vector,Matrix
parser=argparse.ArgumentParser();parser.add_argument('--output',type=pathlib.Path,required=True)
args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
OUT=args.output.resolve();OUT.mkdir(parents=True,exist_ok=False)
results=[]
for scene in bpy.data.scenes:
 if 'authored 45 degree proof' not in scene.name:continue
 bpy.context.window.scene=scene
 arm=next(o for o in scene.objects if o.type=='ARMATURE')
 mesh=next(o for o in scene.objects if o.type=='MESH' and o.name.startswith('briarhold-warden'))
 root=next(o for o in scene.objects if o.name.startswith('PROOF ONLY root yaw'))
 label='left' if scene.name.startswith('LEFT') else 'right';sign=1 if label=='left' else -1
 directory=OUT/('preview-'+label);directory.mkdir(exist_ok=False)
 def points():
  ev=mesh.evaluated_get(bpy.context.evaluated_depsgraph_get());m=ev.to_mesh()
  p=[ev.matrix_world@v.co for v in m.vertices];ev.to_mesh_clear();return p
 scene.frame_set(1);initial=points();subsets={}
 for side in ['Left','Right']:
  groups={mesh.vertex_groups[side+n].index for n in ['UpLeg','Leg','Foot','ToeBase']}
  boot=[v.index for v in mesh.data.vertices if v.groups and all(g.group in groups for g in v.groups if g.weight>1e-5) and initial[v.index].z<.25]
  floor=min(initial[i].z for i in boot);subsets[side]=[i for i in boot if initial[i].z<=floor+.006]
 rows=[]
 for half in range(91):
  frame=1+half*.5;scene.frame_set(int(frame),subframe=frame-int(frame));p=points();u=(frame-1)/45
  row={'frame':frame,'feet':{}}
  for side in ['Left','Right']:
   first=(side=='Left')==(sign==1);begin,end=(.08,.47) if first else (.53,.92)
   stance='initial' if u<=begin else 'final' if u>=end else 'swing'
   indices=subsets[side];ideal=[initial[i] if stance=='initial' else Matrix.Rotation(sign*math.pi/4,4,'Z')@initial[i] for i in indices]
   errors=[Vector((p[i].x-q.x,p[i].y-q.y)) for i,q in zip(indices,ideal)]
   mean=sum(errors,Vector((0.,0.)))/len(errors)
   row['feet'][side]={'stance':stance,'maxHorizontalDriftM':max(e.length for e in errors) if stance!='swing' else None,
    'meanDriftXYM':list(mean) if stance!='swing' else None,'shapeResidualAfterMeanTranslationM':max((e-mean).length for e in errors) if stance!='swing' else None,
    'soleMinimumZ':min(p[i].z for i in indices),'soleMaximumZ':max(p[i].z for i in indices)}
  rows.append(row)
 result={'direction':label,'samples':91,'sampleHz':120,'rows':rows,
  'maxSupportDriftM':max(f['maxHorizontalDriftM'] for r in rows for f in r['feet'].values() if f['maxHorizontalDriftM'] is not None)}
 results.append(result);print('REOPENED_METRICS',label,result['maxSupportDriftM'],flush=True)
 for frame in range(1,47):
  scene.frame_set(frame);scene.render.filepath=str(directory/f'{frame:04d}.png')
  bpy.ops.render.render(write_still=True,scene=scene.name)
(OUT/'reopened-validation.json').write_text(json.dumps({'evidenceClass':'Reopened baked .blend geometry at 120Hz; offline non-RT authoring studio renders','directions':results},indent=2)+'\n')
print('PREVIEWS_FINISHED',flush=True)
