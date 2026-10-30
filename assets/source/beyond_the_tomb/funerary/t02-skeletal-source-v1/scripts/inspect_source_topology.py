import bpy,bmesh,json,sys,argparse
p=argparse.ArgumentParser();p.add_argument('--package',required=True);a=p.parse_args(sys.argv[sys.argv.index('--')+1:]);root=a.package
bpy.ops.wm.open_mainfile(filepath=root+'/source/fgc_skeleton.original.blend',use_scripts=False,load_ui=False)
records=[]
for o in bpy.data.objects:
 if o.type!='MESH':continue
 bm=bmesh.new();bm.from_mesh(o.data);bm.verts.ensure_lookup_table();edges=[e for e in bm.edges if not e.is_manifold];bs=[e for e in edges if e.is_boundary];complexes=[e for e in edges if not e.is_boundary];ids=set(v.index for e in edges for v in e.verts);groups={g.name:0 for g in o.vertex_groups}
 for i in ids:
  for g in o.data.vertices[i].groups:
   if g.weight>0:groups[o.vertex_groups[g.group].name]+=1
 details=[]
 for kind,es in [('boundary',bs),('multi_face_or_wire',complexes)]:
  if not es:continue
  vs=[o.matrix_world@v.co for e in es for v in e.verts]
  details.append({'kind':kind,'edges':len(es),'source_world_min':[min(v[k] for v in vs) for k in range(3)],'source_world_max':[max(v[k] for v in vs) for k in range(3)],'face_users_histogram':{str(n):sum(len(e.link_faces)==n for e in es) for n in sorted(set(len(e.link_faces) for e in es))}})
 records.append({'source_mesh':o.name,'boundary_edges':len(bs),'other_nonmanifold_edges':len(complexes),'affected_vertex_groups':{n:k for n,k in groups.items() if k},'details':details});bm.free()
open(root+'/evidence/source-topology.json','w').write(json.dumps(records,indent=2));print(json.dumps(records,indent=2))
