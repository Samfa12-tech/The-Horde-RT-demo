import os,sys,json,argparse,bpy,bmesh
# Reuse only the studio setup helpers, without running the asset renders.
p=argparse.ArgumentParser();p.add_argument('--package',required=True);a=p.parse_args(sys.argv[sys.argv.index('--')+1:]);ROOT=a.package
s=open(ROOT+'/scripts/render_fresh_imports.py').read();exec(s[:s.index('for title,name,delta')])
bpy.ops.wm.open_mainfile(filepath=ROOT+'/source/horde_skeletal_source_kit.blend',use_scripts=False,load_ui=False)
captures=[]
for name,source,group in [('12_skull_open_boundaries','skull_jaw_cage_lod1',None),('13_spine_source_topology','manny_skeletal_core_static','BONES_SPINE')]:
 o=bpy.data.objects[source]
 ids=set(v.index for v in o.data.vertices)
 if group:
  gi=o.vertex_groups[group].index;ids={v.index for v in o.data.vertices if any(g.group==gi and g.weight>0 for g in v.groups)}
 ids=sorted(ids);mp={old:i for i,old in enumerate(ids)};v=[o.data.vertices[i].co.copy() for i in ids];f=[tuple(mp[i] for i in poly.vertices) for poly in o.data.polygons if all(i in mp for i in poly.vertices)]
 captures.append((name,v,f))
for name,v,f in captures:
 sc=start();m=bpy.data.meshes.new(name);m.from_pydata(v,[],f);m.update();o=bpy.data.objects.new(name,m);sc.collection.objects.link(o)
 mat=bpy.data.materials.new('Diagnostic bone');mat.use_nodes=True;bs=mat.node_tree.nodes['Principled BSDF'];bs.inputs['Base Color'].default_value=(.32,.32,.32,1);bs.inputs['Roughness'].default_value=.85;m.materials.append(mat)
 for poly in m.polygons:poly.use_smooth=True
 bm=bmesh.new();bm.from_mesh(m)
 for kind,color,edges in [('Boundary_orange',(1,.35,.025,1),[e for e in bm.edges if e.is_boundary]),('Nonmanifold_red',(1,.025,.01,1),[e for e in bm.edges if not e.is_manifold and not e.is_boundary])]:
  if not edges:continue
  cu=bpy.data.curves.new(kind,'CURVE');cu.dimensions='3D';cu.bevel_depth=.00075 if 'skull' in name else .0011;cu.bevel_resolution=1
  for e in edges:
   sp=cu.splines.new('POLY');sp.points.add(1)
   for p,vertex in zip(sp.points,e.verts):p.co=(*vertex.co,1)
  ob=bpy.data.objects.new(kind,cu);sc.collection.objects.link(ob);ma=bpy.data.materials.new(kind);ma.use_nodes=True;bs=ma.node_tree.nodes['Principled BSDF'];bs.inputs['Base Color'].default_value=color;bs.inputs['Emission Color'].default_value=color;bs.inputs['Emission Strength'].default_value=.3;cu.materials.append(ma)
 bm.free();render(sc,[o],name,(.4,-1.7,.25) if 'skull' in name else (.45,-1.8,.15),False)
for r in results:
 r['render_source']='Derived Blender source subset; topology overlay only, not fresh GLB import'
 r['diagnostic_overlay']='Open edges orange, edges with more than two linked faces red; curves excluded from asset outputs'
open(ROOT+'/evidence/topology-render-record.json','w').write(json.dumps(results,indent=2))
