import bpy,json,sys,argparse,itertools,hashlib
from mathutils.bvhtree import BVHTree
p=argparse.ArgumentParser();p.add_argument('--package',required=True);a=p.parse_args(sys.argv[sys.argv.index('--')+1:]);root=a.package
bpy.ops.wm.open_mainfile(filepath=root+'/source/horde_skeletal_source_kit.blend',use_scripts=False,load_ui=False)
assert not [o for o in bpy.data.objects if o.type=='ARMATURE'];assert not list(bpy.data.actions);assert len(bpy.data.materials)==1
originalhash=hashlib.sha256(open(root+'/source/fgc_skeleton.original.blend','rb').read()).hexdigest();assert originalhash=='0ba13c9f0c2af50aafb317f450a92083f7d08ece67c3ecc9a2c64eac13257926'
rows=[]
for cn in ['04_ARRANGEMENT_A_EXAMPLE','05_ARRANGEMENT_B_EXAMPLE']:
 c=bpy.data.collections[cn];c.hide_viewport=False;c.hide_render=False;bpy.context.view_layer.update();meshes=list(c.objects);trees={};record={'collection':cn,'instances':len(meshes),'linked_to_master':[],'floor_minima_m':{},'triangle_intersections':[]}
 for o in meshes:
  master=bpy.data.objects[o['master_asset']+'_master_lod0'];assert o.data==master.data;record['linked_to_master'].append(o.name);vs=[o.matrix_world@v.co for v in o.data.vertices];record['floor_minima_m'][o.name]=min(v.z for v in vs);assert abs(min(v.z for v in vs))<1e-6;trees[o.name]=BVHTree.FromPolygons(vs,[tuple(f.vertices) for f in o.data.polygons])
 for x,y in itertools.combinations(meshes,2):
  overlaps=trees[x.name].overlap(trees[y.name]);record['triangle_intersections'].append({'a':x.name,'b':y.name,'overlap_pairs':len(overlaps)})
  assert not overlaps,'Arrangement has intersecting bones'
 rows.append(record)
res={'source_sha256':originalhash,'derived_blend_armatures':0,'derived_blend_actions':0,'derived_blend_materials':1,'external_images':len(bpy.data.images),'arrangements':rows,'checks':'PASS: exact original checksum, static derived file, shared editable mesh data, floor support and no inter-object triangle intersections'}
open(root+'/evidence/editable-source-check.json','w').write(json.dumps(res,indent=2));print(json.dumps(res,indent=2))
