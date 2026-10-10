import bpy,json,sys,argparse
p=argparse.ArgumentParser();p.add_argument("--package",required=True);a=p.parse_args(sys.argv[sys.argv.index("--")+1:]);root=a.package
bpy.ops.wm.open_mainfile(filepath=root+"/source/fgc_skeleton.original.blend",use_scripts=False,load_ui=False)
from mathutils import Vector
out={}
for o in bpy.data.objects:
 if o.type=='MESH':
  adj=[[] for v in o.data.vertices]
  for e in o.data.edges:
   a,b=e.vertices;adj[a].append(b);adj[b].append(a)
  unseen=set(range(len(adj)));comps=[]
  while unseen:
   todo=[unseen.pop()];ids=[]
   while todo:
    i=todo.pop();ids.append(i)
    for a in adj[i]:
     if a in unseen:unseen.remove(a);todo.append(a)
   vs=[o.matrix_world@o.data.vertices[i].co for i in ids];lo=[min(v[k] for v in vs) for k in range(3)];hi=[max(v[k] for v in vs) for k in range(3)]
   groups={}
   for i in ids:
    for g in o.data.vertices[i].groups:
     if g.weight>0:groups[o.vertex_groups[g.group].name]=groups.get(o.vertex_groups[g.group].name,0)+1
   comps.append({'ids':sorted(ids),'verts':len(ids),'lo':lo,'hi':hi,'groups':groups})
  out[o.name]={'matrix':list(map(list,o.matrix_world)),'components':sorted(comps,key=lambda c:-c['verts'])}
print(json.dumps(out,indent=2));open(root+'/evidence/source-components.json','w').write(json.dumps(out,indent=2))
print('TEXTS',[(t.name,t.as_string()) for t in bpy.data.texts]);print('ARMATURE_BONES',[(b.name,list(b.head_local),list(b.tail_local)) for a in bpy.data.armatures for b in a.bones if b.name in ['upper_arm.L','thigh.L','humerus.L','femur.L']])
