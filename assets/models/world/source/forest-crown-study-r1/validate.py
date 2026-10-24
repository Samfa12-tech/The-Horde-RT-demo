import bpy,json,hashlib,struct
from pathlib import Path
r=Path.cwd();o=r/'assets/models/world/source/forest-crown-study-r1';records=[]
def mesh(ob):
 uv=hashlib.sha256();top=hashlib.sha256();pos=hashlib.sha256()
 for v in ob.data.vertices:pos.update(struct.pack('<3f',*v.co))
 for face in ob.data.polygons:top.update(struct.pack('<'+'I'*len(face.vertices),*face.vertices));top.update(struct.pack('<I',face.material_index))
 for layer in ob.data.uv_layers:
  for point in layer.data:uv.update(struct.pack('<2f',*point.uv))
 for row in ob.matrix_world:pos.update(struct.pack('<4f',*row))
 return {'positionTransform':pos.hexdigest(),'topologyMaterial':top.hexdigest(),'uv':uv.hexdigest(),'vertices':len(ob.data.vertices),'anchors':[list(ob.data.vertices[i].co) for i in range(0,len(ob.data.vertices),6)] if 'needle-spray' in ob.name or 'opaque leaves' in ob.name else None}
def load(path):
 bpy.ops.wm.open_mainfile(filepath=str(path));obs=[x for x in bpy.context.scene.objects if x.type=='MESH' and 'QA-only' not in x.name]
 used={node.image.name for ob in obs for mat in ob.data.materials if mat and mat.use_nodes for node in mat.node_tree.nodes if node.type=='TEX_IMAGE' and node.image}
 return {x.name:mesh(x) for x in obs},{im.name:hashlib.sha256(im.packed_file.data).hexdigest() for im in bpy.data.images if im.packed_file},used
for species,stem in [('pine','horde-irregular-pine-v1'),('alder','horde-upright-alder-v1')]:
 a,images,used=load(r/'assets/models/world/source/forest-tree-pair-v1/source'/(stem+'-lod1.blend'));b,newimages,newused=load(o/'source'/(species+'-fuller-crown-r1.blend'));assert a.keys()==b.keys();print(species,'PACKED_IMAGE_COMPARISON',json.dumps({'control':images,'candidate':newimages}),flush=True);assert used==newused;assert all(images[k]==newimages[k] for k in used);assert all(images[k]==v for k,v in newimages.items());assert not (set(images)-set(newimages))&used
 for name in a:
  assert a[name]['topologyMaterial']==b[name]['topologyMaterial'];assert a[name]['uv']==b[name]['uv'];assert a[name]['vertices']==b[name]['vertices']
  if a[name]['anchors'] is None:assert a[name]['positionTransform']==b[name]['positionTransform']
  else:assert a[name]['anchors']==b[name]['anchors']
 records.append({'species':species,'savedSourceReopened':True,'allTopologyMaterialUVUnchanged':True,'woodMossExact':True,'foliageBaseAnchorsExact':True,'referencedPackedImagesExact':{k:images[k] for k in used},'unreferencedImagesOmittedBySave':list(set(images)-set(newimages))})
(o/'validation.json').write_text(json.dumps(records,indent=2)+'\n');print('CROWN_SOURCE_VALIDATION_PASS',flush=True)
