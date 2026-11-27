"""Author B01-B08 rooflines into a fresh source/runtime candidate directory.

Run from the repository root:
 blender --background --python-exit-code 1 --python assets/models/world/source/bellwether-roofline-v1/author-bellwether-roofline.py -- . <fresh-output-dir> <ec850e56-town.glb> <sibling-manifest.json>
The exact parent GLB and manifest are SHA-pinned; this recipe never overwrites
its input or the installed runtime.
"""
from pathlib import Path
import hashlib, json, math, struct, sys, tempfile
import bpy
import bmesh
from mathutils import Vector

args = sys.argv[sys.argv.index('--') + 1:]
if len(args) != 4:
    raise RuntimeError('Expected ROOT OUTPUT_DIR PARENT_TOWN_GLB PARENT_MANIFEST_JSON')
ROOT, OUTPUT_DIR, PARENT_TOWN, PARENT_MANIFEST = map(lambda p: Path(p).resolve(), args)
if OUTPUT_DIR.exists():
    raise RuntimeError('Output directory must be fresh; preserve any prior generation.')
OUTPUT_DIR.mkdir(parents=True)
WORLD = ROOT / 'assets/models/world'
SOURCE = OUTPUT_DIR / 'source/bellwether-roofline-v1'
RUNTIME = OUTPUT_DIR / 'runtime/bellwether-tavern-v1'
SOURCE.mkdir(parents=True); RUNTIME.mkdir(parents=True)
MASTER = SOURCE / 'bellwether-roofline-v1.blend'
RECEIPT = SOURCE / 'authoring-receipt.json'
GENERATED = SOURCE / 'geometry.generated.h'
README = SOURCE / 'README.md'
RECIPE = SOURCE / 'author-bellwether-roofline.py'
TOWN = RUNTIME / 'bellwether-tavern-v1.glb'
MANIFEST = RUNTIME / 'asset.manifest.json'
SHELL = WORLD / 'source/bellwether-shell-v4'
SHELL_BLEND = SHELL / 'bellwether-shell-v4.blend'
SHELL_RECEIPT = json.loads((SHELL / 'authoring-receipt.json').read_text())
if not PARENT_TOWN.is_file() or not PARENT_MANIFEST.is_file():
    raise RuntimeError('Explicit parent town GLB and sibling manifest are required.')
if PARENT_MANIFEST.parent != PARENT_TOWN.parent:
    raise RuntimeError('Parent GLB and sibling manifest must be in the same directory.')
original_town_sha = hashlib.sha256(PARENT_TOWN.read_bytes()).hexdigest()
original_manifest_sha = hashlib.sha256(PARENT_MANIFEST.read_bytes()).hexdigest()
EXPECTED_PARENT_TOWN_SHA256='5c0b850b1159e5417f0d58800e8d38dce9c1a12b80f1ddd4c9e32a229b82e6de'
EXPECTED_PARENT_MANIFEST_SHA256='ac31c5dba4d4e4018fda77662201c2108f6ef42b0f22449770a685a7b826a3f6'
if original_town_sha!=EXPECTED_PARENT_TOWN_SHA256 or original_manifest_sha!=EXPECTED_PARENT_MANIFEST_SHA256:
    raise RuntimeError('Parent town inputs do not match the pinned ec850e56 content hashes.')

BUILDINGS = [b for b in SHELL_RECEIPT['buildings'] if b['id'] in {f'B{i:02d}' for i in range(1,9)}]
assert [b['id'] for b in BUILDINGS] == [f'B{i:02d}' for i in range(1,9)]

def pbr(name, rough=0.9):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    m.diffuse_color = (1,1,1,1); m.roughness = rough; m.metallic = 0
    return m

def box_faces(name, center, axis, cross, depth, mat):
    c=Vector(center); a=Vector(axis).normalized(); q=Vector(cross).normalized(); n=Vector(depth).normalized()
    # The dimensions in axis/cross/depth are encoded by the scaled input vectors.
    a=Vector(axis); q=Vector(cross); n=Vector(depth)
    pts=[c+a*s+q*t+n*u for s,t,u in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
    faces=[(0,1,2,3),(4,7,6,5),(0,4,5,1),(1,5,6,2),(2,6,7,3),(3,7,4,0)]
    mesh=bpy.data.meshes.new(name); mesh.from_pydata(pts,[],faces); mesh.materials.append(mat)
    bm=bmesh.new(); bm.from_mesh(mesh); bmesh.ops.recalc_face_normals(bm,faces=bm.faces); bm.to_mesh(mesh); bm.free(); mesh.update()
    uv=mesh.uv_layers.new(name='metre_projection')
    for poly in mesh.polygons:
        for li in poly.loop_indices:
            v=mesh.vertices[mesh.loops[li].vertex_index].co
            uv.data[li].uv=(v.x/2.4, -v.y/2.4)
    obj=bpy.data.objects.new(name,mesh); bpy.context.collection.objects.link(obj); return obj

def roofline_geometry(b, timber, clay):
    x=b['engineX']; z=b['engineZ']; floor=b['actualFloorY']; w=b['width']; d=b['depth']; eave=floor+b['wallHeight']-.07; rise=b['roofRise']
    half=w/2+.25; z0=z-d/2-.25; z1=z+d/2+.25; ridge=eave+rise
    added=[]
    # Solid raking verge boards at both gable ends; the boards sit just proud of the roof.
    for gable_z in (z0,z1):
        north= gable_z<z
        zn=gable_z+(-.085 if north else .085)
        for side in (-1,1):
            p0=Vector((x+side*half,-zn,eave)); p1=Vector((x,-zn,ridge))
            axis=(p1-p0)/2; perp=Vector((-axis.z,0,axis.x)).normalized()*.12
            normal=Vector((0,.085 if north else -.085,0))
            added.append(box_faces(f"{b['id']}_oak_verge_{'north' if north else 'south'}_{'west' if side<0 else 'east'}",(p0+p1)/2,axis,perp,normal,timber))
        # A restrained lower collar and king post frame the masonry gable without an opening.
        collar_c=Vector((x,-zn,eave+.18)); suffix='north' if north else 'south'; added.append(box_faces(f"{b['id']}_oak_gable_collar_{suffix}",collar_c,(half-.02,0,0),(0,0,.105),(0,.075,0),timber))
        post_len=rise*.43
        post_c=Vector((x,-zn,eave+.35+post_len/2)); added.append(box_faces(f"{b['id']}_oak_gable_kingpost_{suffix}",post_c,(0,0,post_len/2),(.065,0,0),(0,.075,0),timber))
    # One closed half-round clay ridge cap, segmented longitudinally by its profile.
    r=.17; segments=8; ring0=[]; ring1=[]
    for i in range(segments+1):
        a=math.pi*i/segments
        ring0.append((x+math.cos(a)*r,-z0,ridge+.025+math.sin(a)*r))
        ring1.append((x+math.cos(a)*r,-z1,ridge+.025+math.sin(a)*r))
    # Blender authoring coordinates are (east,-north,up); ridge parallels source Y.
    verts=[Vector(v) for v in ring0+ring1]
    faces=[(i,segments+1+i,segments+2+i,i+1) for i in range(segments)]
    faces.append((0,segments,2*segments+1,segments+1))
    faces.append(tuple(range(segments+1)))
    faces.append(tuple(range(2*segments+1,segments,-1)))
    mesh=bpy.data.meshes.new(f"{b['id']}_clay_ridge_cap_mesh"); mesh.from_pydata(verts,[],faces); mesh.materials.append(clay)
    bm=bmesh.new(); bm.from_mesh(mesh); bmesh.ops.recalc_face_normals(bm,faces=bm.faces); bm.to_mesh(mesh); bm.free(); mesh.update()
    obj=bpy.data.objects.new(f"{b['id']}_half_round_clay_ridge_cap",mesh); bpy.context.collection.objects.link(obj); added.append(obj)
    return added

# Open the nine-building source master and make only the eight mapped primary roofs editable.
bpy.ops.wm.open_mainfile(filepath=str(SHELL_BLEND)); bpy.context.preferences.filepaths.save_version=0
# Hidden terrain is authoring context, not part of this editable deliverable.
for o in list(bpy.context.scene.objects):
    if o.type=='MESH' and o.hide_get(): bpy.data.objects.remove(o,do_unlink=True)
stone=pbr('HordeTownStone'); clay=pbr('HordeTownRoofClay'); timber=pbr('HordeTavernTimber',.82); tiles=pbr('HordeVillageRoofTiles',1.0)
for b in BUILDINGS:
    obj=bpy.data.objects[f"{b['id']}_closed_gable"]
    b['roofRise']=max(v.co.z for v in obj.data.vertices)-(b['actualFloorY']+b['wallHeight']-.07)
    if stone not in obj.data.materials[:]: obj.data.materials.append(stone)
    if tiles not in obj.data.materials[:]: obj.data.materials.append(tiles)
    for poly in obj.data.polygons:
        n=poly.normal
        if abs(n.y)<.01 and abs(n.x)>.2 and n.z>.2:
            poly.material_index=obj.data.materials[:].index(tiles)
        elif abs(n.y)>.99 and len(poly.vertices)==3:
            poly.material_index=obj.data.materials[:].index(stone)
trim=[]
for b in BUILDINGS: trim.extend(roofline_geometry(b,timber,clay))
for o in trim:
    bm=bmesh.new(); bm.from_mesh(o.data); bm.normal_update()
    if not all(e.is_manifold for e in bm.edges) or bm.calc_volume(signed=True)<=0:
        bm.free(); raise RuntimeError(f'Roofline addition is not outward closed solid: {o.name}')
    bm.free()
# The source-shell terrain is intentionally absent; drop only its now-unused
# materials and the packed images referenced exclusively by those materials.
for name in ('ChapterForestGround','ChapterTrailDampSand'):
    mat=bpy.data.materials.get(name)
    if mat and mat.users==0: bpy.data.materials.remove(mat,do_unlink=True)
for mesh_data in list(bpy.data.meshes):
    if mesh_data.users==0: bpy.data.meshes.remove(mesh_data,do_unlink=True)
for image in list(bpy.data.images):
    if image.users==0: bpy.data.images.remove(image,do_unlink=True)
# Removing the unused source terrain materials makes their six packed terrain
# maps orphaned only after Blender resolves the dependency graph.
bpy.data.orphans_purge(do_local_ids=True,do_linked_ids=True,do_recursive=True)
assert not any(image.users==0 for image in bpy.data.images)
assert len(bpy.data.images)==0
assert len(bpy.data.meshes)==sum(1 for o in bpy.data.objects if o.type=='MESH' for _ in (o.data,))
source_mesh_count=sum(1 for o in bpy.data.objects if o.type=='MESH')
source_material_count=len(bpy.data.materials)
source_packed_image_count=sum(bool(i.packed_file) for i in bpy.data.images)
bpy.ops.wm.save_as_mainfile(filepath=str(MASTER),compress=True)

# Export additions as a temporary glTF, keeping source/runtime separation intact.
with tempfile.TemporaryDirectory(prefix='horde-roofline-') as td:
    extra=Path(td)/'roofline-additions.glb'
    bpy.ops.object.select_all(action='DESELECT')
    for o in trim: o.select_set(True)
    bpy.context.view_layer.objects.active=trim[0]
    bpy.ops.export_scene.gltf(filepath=str(extra),export_format='GLB',use_selection=True,
        export_apply=True,export_texcoords=True,export_normals=True,export_tangents=True,
        export_animations=False,export_extras=True)

    # GLB helpers: preserve existing buffers and append only the selected/reclassified triangles.
    def read_glb(path):
        raw=path.read_bytes(); assert raw[:4]==b'glTF'
        jlen=struct.unpack_from('<I',raw,12)[0]; doc=json.loads(raw[20:20+jlen]); p=20+jlen
        assert struct.unpack_from('<I',raw,p+4)[0]==0x004E4942
        blen=struct.unpack_from('<I',raw,p)[0]; return doc,bytearray(raw[p+8:p+8+blen])
    doc,blob=read_glb(PARENT_TOWN); adoc,abin=read_glb(extra)
    addition_triangle_count=sum(adoc['accessors'][prim['indices']]['count']//3
        for m in adoc['meshes'] for prim in m['primitives'])
    def values(d,binbuf,ai):
        a=d['accessors'][ai]; v=d['bufferViews'][a['bufferView']]; fmt={5121:'B',5123:'H',5125:'I',5126:'f'}[a['componentType']]
        cnt={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']]; sz=struct.calcsize(fmt)*cnt
        off=v.get('byteOffset',0)+a.get('byteOffset',0); stride=v.get('byteStride',sz)
        return [struct.unpack_from('<'+fmt*cnt,binbuf,off+i*stride) for i in range(a['count'])]
    def norm3(v):
        length=math.sqrt(sum(float(x)*float(x) for x in v))
        if length<1e-10: raise RuntimeError('Cannot author a tangent from a zero normal.')
        return tuple(float(x)/length for x in v)
    def dot3(a,b): return sum(a[i]*b[i] for i in range(3))
    def cross3(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
    def unit_tangent(normal, direction):
        projection=dot3(normal,direction); v=tuple(direction[i]-normal[i]*projection for i in range(3))
        return norm3(v)
    def material_uv_and_tangents(positions,normals,indices):
        # Texture projection follows the dominant face normal and keeps a real
        # two-dimensional metre basis on side faces as well as the top planes.
        uvs=[]
        for p,n0 in zip(positions,normals):
            n=norm3(n0); axis=max(range(3),key=lambda i:abs(n[i]))
            pair=((1,2),(0,2),(0,1))[axis]
            uvs.append((p[pair[0]]/2.4,p[pair[1]]/2.4))
        tangent_sum=[[0.0,0.0,0.0] for _ in positions]; bitangent_sum=[[0.0,0.0,0.0] for _ in positions]
        for start in range(0,len(indices),3):
            ids=indices[start:start+3]; p0,p1,p2=(positions[i] for i in ids); uv0,uv1,uv2=(uvs[i] for i in ids)
            e1=tuple(p1[a]-p0[a] for a in range(3)); e2=tuple(p2[a]-p0[a] for a in range(3))
            du1,dv1=uv1[0]-uv0[0],uv1[1]-uv0[1]; du2,dv2=uv2[0]-uv0[0],uv2[1]-uv0[1]
            det=du1*dv2-du2*dv1
            if abs(det)>1e-10:
                tangent=tuple((e1[a]*dv2-e2[a]*dv1)/det for a in range(3))
                bitangent=tuple((e2[a]*du1-e1[a]*du2)/det for a in range(3))
            else:
                n=norm3(tuple(sum(normals[i][a] for i in ids) for a in range(3)))
                ref=(0.0,0.0,1.0) if abs(n[2])<.9 else (1.0,0.0,0.0)
                tangent=unit_tangent(n,ref); bitangent=cross3(n,tangent)
            for idx in ids:
                for a in range(3):
                    tangent_sum[idx][a]+=tangent[a]; bitangent_sum[idx][a]+=bitangent[a]
        tangents=[]
        for i,n0 in enumerate(normals):
            n=norm3(n0); t=unit_tangent(n,tangent_sum[i]); b=tuple(bitangent_sum[i])
            sign=-1.0 if dot3(cross3(n,t),b)<0 else 1.0
            tangents.append((t[0],t[1],t[2],sign))
        return uvs,tangents
    def append_accessor(data,typ,component,vals):
        formats={5121:'B',5123:'H',5125:'I',5126:'f'}; cnt={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[typ]; fmt=formats[component]
        while len(blob)%4: blob.append(0)
        off=len(blob); flat=[v for row in vals for v in row] if cnt>1 else [r[0] for r in vals]
        payload=struct.pack('<'+fmt*len(flat),*flat); blob.extend(payload)
        vi=len(doc['bufferViews']); doc['bufferViews'].append({'buffer':0,'byteOffset':off,'byteLength':len(payload)})
        acc={'bufferView':vi,'componentType':component,'count':len(vals),'type':typ}
        if typ in ('VEC2','VEC3','VEC4') and vals:
            acc['min']=[min(row[i] for row in vals) for i in range(cnt)]; acc['max']=[max(row[i] for row in vals) for i in range(cnt)]
        ai=len(doc['accessors']); doc['accessors'].append(acc); return ai
    solid=next(n for n in doc['nodes'] if n.get('name')=='BellwetherSolid'); mesh=doc['meshes'][solid['mesh']]
    prims=mesh['primitives']; assert [doc['materials'][p['material']]['name'] for p in prims]==['HordeTownRoofClay','HordeTownStone','HordeTavernTimber','HordeHearthChar']
    clayp,stonep,timberp,charp=prims; cp=clayp['attributes']; all_roof_indices=values(doc,blob,clayp['indices']); tris=[tuple(v[0] for v in all_roof_indices[i:i+3]) for i in range(0,len(all_roof_indices),3)]
    srcattrs={k:values(doc,blob,ai) for k,ai in cp.items()}; pvals=srcattrs['POSITION']
    selected_roof=[]; selected_gable=[]; remaining=[]; by_build={b['id']:{'roof':0,'gable':0} for b in BUILDINGS}
    for tri in tris:
        pts=[pvals[i] for i in tri]; cx=sum(p[0] for p in pts)/3; cy=sum(p[1] for p in pts)/3; cz=sum(p[2] for p in pts)/3
        found=None; kind=None
        for b in BUILDINGS:
            x,z,w,d,floor,wh=b['engineX'],b['engineZ'],b['width'],b['depth'],b['actualFloorY'],b['wallHeight']
            half=w/2+.25; zh=d/2+.25; eave=floor+wh-.07; rise=b['roofRise']
            if not (x-half-.01<=cx<=x+half+.01 and z-zh-.01<=cz<=z+zh+.01 and eave-.01<=cy<=eave+rise+.01): continue
            on_profile=all(abs(p[1]-(eave+rise*(1-abs(p[0]-x)/half)))<.003 for p in pts)
            if not on_profile: continue
            zspan=max(p[2] for p in pts)-min(p[2] for p in pts); xspan=max(p[0] for p in pts)-min(p[0] for p in pts)
            if zspan>1 and xspan>1:
                nx=abs((pts[1][1]-pts[0][1])*(pts[2][2]-pts[0][2])-(pts[1][2]-pts[0][2])*(pts[2][1]-pts[0][1]))
                ny=abs((pts[1][2]-pts[0][2])*(pts[2][0]-pts[0][0])-(pts[1][0]-pts[0][0])*(pts[2][2]-pts[0][2]))
                nz=abs((pts[1][0]-pts[0][0])*(pts[2][1]-pts[0][1])-(pts[1][1]-pts[0][1])*(pts[2][0]-pts[0][0]))
                length=math.sqrt(nx*nx+ny*ny+nz*nz)
                if length and nx/length>.2 and ny/length>.2 and nz/length<.003: found=b; kind='roof'
            elif zspan<.002 and abs(abs(cz-z)-zh)<.003 and xspan>1: found=b; kind='gable'
            if found: break
        if found and kind=='roof': selected_roof.append(tri); by_build[found['id']]['roof']+=1
        elif found and kind=='gable': selected_gable.append(tri); by_build[found['id']]['gable']+=1
        else: remaining.append(tri)
    assert all(v=={'roof':4,'gable':2} for v in by_build.values()),by_build
    # Remove selected roof and gable indices from the clay group while retaining its original accessors.
    kept=[i for tri in remaining for i in tri]
    clayp['indices']=append_accessor(doc,'SCALAR',5123,[(i,) for i in kept])
    def semhash(attrs, indices, pos):
        seq=[]
        for idx in indices:
            seq.append(tuple(tuple(round(float(v),6) for v in attrs[k][idx]) for k in sorted(attrs)))
        return hashlib.sha256(json.dumps(seq,separators=(',',':')).encode()).hexdigest()
    stone_old_index_count=len(values(doc,blob,stonep['indices']))
    timber_old_index_count=len(values(doc,blob,timberp['indices']))
    char_old_index_count=len(values(doc,blob,charp['indices']))
    base_hashes={
        'roof_clay_unselected':semhash(srcattrs,[i for t in remaining for i in t],pvals),
        'stone':semhash({k:values(doc,blob,ai) for k,ai in stonep['attributes'].items()},[v[0] for v in values(doc,blob,stonep['indices'])],None),
        'timber':semhash({k:values(doc,blob,ai) for k,ai in timberp['attributes'].items()},[v[0] for v in values(doc,blob,timberp['indices'])],None),
        'hearth_char':semhash({k:values(doc,blob,ai) for k,ai in charp['attributes'].items()},[v[0] for v in values(doc,blob,charp['indices'])],None),
    }
    # Append the exact gable-end triangles to the existing stone primitive's streams.
    def merge_triangles(target, source_values, triangles):
        attrs=target['attributes']; old={k:values(doc,blob,ai) for k,ai in attrs.items()}; base=len(old['POSITION']); add={k:[] for k in attrs}
        for tri in triangles:
            for idx in tri:
                for k in attrs: add[k].append(source_values[k][idx])
        newattrs={}
        for k,ai in attrs.items():
            a=doc['accessors'][ai]; vals=old[k]+add[k]; newattrs[k]=append_accessor(doc,a['type'],a['componentType'],vals)
        oldix=[v[0] for v in values(doc,blob,target['indices'])]
        appended=list(range(base,base+len(add['POSITION'])))
        target['attributes']=newattrs; target['indices']=append_accessor(doc,'SCALAR',5123,[(i,) for i in oldix+appended])
        return base,len(add['POSITION'])
    merge_triangles(stonep,srcattrs,selected_gable)
    # Add the fifth material primitive for exactly the 32 sloping roof triangles.
    roofpos=[]; roofnorm=[]; roofuv=[]; rooftan=[]; roofix=[]
    for tri in selected_roof:
        for idx in tri:
            x,y,z=pvals[idx]; b=next(b for b in BUILDINGS if b['engineX']-b['width']/2-.26<=x<=b['engineX']+b['width']/2+.26 and b['engineZ']-b['depth']/2-.26<=z<=b['engineZ']+b['depth']/2+.26 and b['actualFloorY']+b['wallHeight']-.1<=y<=b['actualFloorY']+b['wallHeight']+4.3)
            n=srcattrs['NORMAL'][idx]; roofpos.append((x,y,z)); roofnorm.append(n)
            eave=b['actualFloorY']+b['wallHeight']-.07
            slope_run=(b['width']+.5)/2-abs(x-b['engineX'])
            roofuv.append(((z-(b['engineZ']-b['depth']/2-.25))/2.4, math.hypot(slope_run,y-eave)/2.4))
            rooftan.append((0.,0.,1.,1. if n[0]<0 else -1.)); roofix.append(len(roofix))
    roofprim={'attributes':{'POSITION':append_accessor(doc,'VEC3',5126,roofpos),'NORMAL':append_accessor(doc,'VEC3',5126,roofnorm),'TEXCOORD_0':append_accessor(doc,'VEC2',5126,roofuv),'TANGENT':append_accessor(doc,'VEC4',5126,rooftan)},'indices':append_accessor(doc,'SCALAR',5123,[(i,) for i in roofix]),'material':len(doc['materials'])}
    uv_witnesses=[]
    for tri in selected_roof:
        tri_uv=[]
        for idx in tri:
            x,y,z=pvals[idx]; b=next(b for b in BUILDINGS if b['engineX']-b['width']/2-.26<=x<=b['engineX']+b['width']/2+.26 and b['engineZ']-b['depth']/2-.26<=z<=b['engineZ']+b['depth']/2+.26 and b['actualFloorY']+b['wallHeight']-.1<=y<=b['actualFloorY']+b['wallHeight']+4.3)
            run=(b['width']+.5)/2-abs(x-b['engineX']); eave=b['actualFloorY']+b['wallHeight']-.07
            u=(z-(b['engineZ']-b['depth']/2-.25))/2.4; v=math.hypot(run,y-eave)/2.4; tri_uv.append((u,v))
            if abs(run)<.003 and abs(y-eave)<.003: assert abs(v)<.002, (b['id'],x,y,v)
            if abs(x-b['engineX'])<.003 and abs(y-(eave+b['roofRise']))<.003:
                assert abs(v-math.hypot((b['width']+.5)/2,b['roofRise'])/2.4)<.002, (b['id'],v)
        uv_witnesses.append({'uDeltas':[round(tri_uv[i][0]-tri_uv[0][0],6) for i in (1,2)],'vDeltas':[round(tri_uv[i][1]-tri_uv[0][1],6) for i in (1,2)]})
    doc['materials'].append({'name':'HordeVillageRoofTiles','pbrMetallicRoughness':{'baseColorFactor':[1,1,1,1],'metallicFactor':0,'roughnessFactor':1.0},'doubleSided':False})
    mesh['primitives'].append(roofprim)
    # Merge authored trim into the existing material groups without reordering them.
    target_by_name={doc['materials'][p['material']]['name']:p for p in prims}
    additions={name:{k:[] for k in ('POSITION','NORMAL','TEXCOORD_0','TANGENT')}|{'indices':[]} for name in target_by_name}
    for amesh in adoc['meshes']:
        for ap in amesh['primitives']:
            name=adoc['materials'][ap['material']]['name']
            assert name in additions, f'Unexpected roofline material {name}'
            vals={k:values(adoc,abin,ai) for k,ai in ap['attributes'].items()}
            positions=vals['POSITION']; normals=vals['NORMAL']
            idx=[v[0] for v in values(adoc,abin,ap['indices'])]
            vals['TEXCOORD_0'],vals['TANGENT']=material_uv_and_tangents(positions,normals,idx)
            base=len(additions[name]['POSITION'])
            for k in ('POSITION','NORMAL','TEXCOORD_0','TANGENT'): additions[name][k].extend(vals[k])
            additions[name]['indices'].extend(base+i for i in idx)
    trim_vertex_ranges={}
    for name,target in target_by_name.items():
        if additions[name]['indices']:
            idx=additions[name].pop('indices'); addtri=[tuple(idx[i:i+3]) for i in range(0,len(idx),3)]
            # The indices address one concatenated stream of per-material additions.
            trim_vertex_ranges[name]=merge_triangles(target,additions[name],addtri)
    # Drop obsolete accessors/views and compact the BIN chunk while preserving all mesh semantics.
    used=set()
    for m in doc['meshes']:
        for prim in m.get('primitives',[]): used.add(prim['indices']); used.update(prim['attributes'].values())
    old_acc=doc['accessors']; acc_map={old:new for new,old in enumerate(sorted(used))}
    view_ids=sorted({old_acc[i]['bufferView'] for i in used}); view_map={old:new for new,old in enumerate(view_ids)}
    compact=bytearray(); new_views=[]
    for vi in view_ids:
        v=dict(doc['bufferViews'][vi]); start=v.get('byteOffset',0); size=v['byteLength']
        while len(compact)%4: compact.append(0)
        v['byteOffset']=len(compact); compact.extend(blob[start:start+size]); new_views.append(v)
    for m in doc['meshes']:
        for prim in m.get('primitives',[]):
            prim['indices']=acc_map[prim['indices']]; prim['attributes']={k:acc_map[v] for k,v in prim['attributes'].items()}
    doc['accessors']=[dict(old_acc[i],bufferView=view_map[old_acc[i]['bufferView']]) for i in sorted(used)]
    doc['bufferViews']=new_views; blob=compact
    roof_attrs={k:values(doc,blob,ai) for k,ai in roofprim['attributes'].items()}
    assert len(roof_attrs['POSITION'])==len(roofpos)==len(roof_attrs['TEXCOORD_0'])==len(roof_attrs['TANGENT'])
    for i,(p,uv,tan) in enumerate(zip(roof_attrs['POSITION'],roof_attrs['TEXCOORD_0'],roof_attrs['TANGENT'])):
        x,y,z=p; b=next(b for b in BUILDINGS if b['engineX']-b['width']/2-.26<=x<=b['engineX']+b['width']/2+.26 and b['engineZ']-b['depth']/2-.26<=z<=b['engineZ']+b['depth']/2+.26 and b['actualFloorY']+b['wallHeight']-.1<=y<=b['actualFloorY']+b['wallHeight']+4.3)
        run=(b['width']+.5)/2-abs(x-b['engineX']); eave=b['actualFloorY']+b['wallHeight']-.07
        expected=((z-(b['engineZ']-b['depth']/2-.25))/2.4,math.hypot(run,y-eave)/2.4)
        assert abs(uv[0]-expected[0])<1e-5 and abs(uv[1]-expected[1])<1e-5
        assert abs(tan[0])<1e-6 and abs(tan[1])<1e-6 and abs(tan[2]-1)<1e-6
        assert tan[3] == (1. if srcattrs['NORMAL'][selected_roof[i//3][i%3]][0]<0 else -1.)
    def output_semhash(primitive,count=None):
        aa={k:values(doc,blob,ai) for k,ai in primitive['attributes'].items()}
        ix=[v[0] for v in values(doc,blob,primitive['indices'])]
        return semhash(aa,ix[:count] if count is not None else ix,None)
    assert output_semhash(clayp,len(kept))==base_hashes['roof_clay_unselected']
    assert output_semhash(stonep,stone_old_index_count)==base_hashes['stone']
    assert output_semhash(timberp,timber_old_index_count)==base_hashes['timber']
    assert output_semhash(charp,char_old_index_count)==base_hashes['hearth_char']
    full_tangent_validation={}
    for primitive in mesh['primitives']:
        matname=doc['materials'][primitive['material']]['name']; aa={k:values(doc,blob,ai) for k,ai in primitive['attributes'].items()}
        if 'TANGENT' not in aa: continue
        max_dot=0.0; min_length=1e9
        for normal,tangent in zip(aa['NORMAL'],aa['TANGENT']):
            n=norm3(normal); length=math.sqrt(sum(tangent[k]*tangent[k] for k in range(3))); min_length=min(min_length,length); max_dot=max(max_dot,abs(dot3(n,tangent[:3])))
            projected_length=math.sqrt(max(0.0,length*length-dot3(n,tangent[:3])**2))
            assert length>.999 and abs(length-1.0)<2e-3 and projected_length>1e-6 and tangent[3] in (-1.0,1.0), (matname,tangent,normal)
        full_tangent_validation[matname]={'vertices':len(aa['TANGENT']),'minimumTangentLength':min_length,'maximumAbsNormalDot':max_dot}
    tangent_validation={}
    for name,(start,count) in trim_vertex_ranges.items():
        primitive=target_by_name[name]; aa={k:values(doc,blob,ai) for k,ai in primitive['attributes'].items()}
        max_dot=0.0; min_length=1e9
        for i in range(start,start+count):
            n=norm3(aa['NORMAL'][i]); t=aa['TANGENT'][i]; length=math.sqrt(sum(t[k]*t[k] for k in range(3))); min_length=min(min_length,length)
            max_dot=max(max_dot,abs(dot3(n,t[:3])))
            assert abs(length-1.0)<2e-5 and abs(dot3(n,t[:3]))<2e-5 and t[3] in (-1.0,1.0), (name,i,t,n)
        tangent_validation[name]={'vertices':count,'minimumTangentLength':min_length,'maximumAbsNormalDot':max_dot}
    doc['buffers'][0]['byteLength']=len(blob)
    while len(blob)%4: blob.append(0)
    jbytes=json.dumps(doc,separators=(',',':')).encode('utf-8')
    while len(jbytes)%4:jbytes+=b' '
    total=12+8+len(jbytes)+8+len(blob)
    TOWN.write_bytes(b'glTF'+struct.pack('<II',2,total)+struct.pack('<I',len(jbytes))+struct.pack('<I',0x4E4F534A)+jbytes+struct.pack('<I',len(blob))+struct.pack('<I',0x004E4942)+blob)

# The texture-free material primitive is bound by the runtime shader.
candidate_doc,_=read_glb(TOWN)
all_primitives=[p for m in candidate_doc['meshes'] for p in m['primitives']]
vertex_total=sum(candidate_doc['accessors'][p['attributes']['POSITION']]['count'] for p in all_primitives)
index_total=sum(candidate_doc['accessors'][p['indices']]['count'] for p in all_primitives)
manifest=json.loads(PARENT_MANIFEST.read_text()); manifest['budgets']['maxPrimitives']=6; manifest['budgets']['maxMaterials']=5
manifest['budgets']['maxVertices']=vertex_total; manifest['budgets']['maxIndices']=index_total
manifest['lods'][0]['maxTriangles']=index_total//3
MANIFEST.write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8',newline='\n')

# Deterministic deliverables and counts.
runtime_bytes=TOWN.read_bytes(); source_bytes=MASTER.read_bytes()
doc2,bin2=read_glb(TOWN); mesh2=doc2['meshes'][next(n['mesh'] for n in doc2['nodes'] if n.get('name')=='BellwetherSolid')]
solid_tri=sum(doc2['accessors'][p['indices']]['count']//3 for p in mesh2['primitives'])
added_tri=addition_triangle_count
geom_header=f'''#pragma once\n#include <cstdint>\n\nnamespace horde::gameplay::bellwether {{\ninline constexpr std::uint32_t kVillageRoofAddedSolidTriangles = {added_tri}u;\ninline constexpr std::uint32_t kVillageRoofSlopeTriangles = {len(selected_roof)}u;\ninline constexpr std::uint32_t kVillageRoofGableTriangles = {len(selected_gable)}u;\ninline constexpr std::uint32_t kVillageRoofSolidTriangles = {solid_tri}u;\n}}  // namespace horde::gameplay::bellwether\n'''
GENERATED.write_text(geom_header,encoding='utf-8',newline='\n')
receipt={'feature':'Mapped Bellwether B01-B08 roof tiles, masonry gables and oak roofline','parentSourceMasterSha256':hashlib.sha256(SHELL_BLEND.read_bytes()).hexdigest(),'originalTownSha256':original_town_sha,'originalTownManifestSha256':original_manifest_sha,'runtimeTownSha256':hashlib.sha256(runtime_bytes).hexdigest(),'sourceMasterSha256':hashlib.sha256(source_bytes).hexdigest(),'sourceInventory':{'meshObjects':source_mesh_count,'hiddenTerrainMeshes':0,'unusedMeshDatablocks':0,'packedImages':source_packed_image_count,'materials':source_material_count,'compressedBlend':True},'untouchedGeometrySemanticSha256':base_hashes,'runtimeMaterialOrder':['HordeTownRoofClay','HordeTownStone','HordeTavernTimber','HordeHearthChar','HordeVillageRoofTiles'],'roofFaceCounts':by_build,'slopeTrianglesMoved':len(selected_roof),'gableTrianglesMovedToStone':len(selected_gable),'addedSolidTriangles':added_tri,'uvValidation':uv_witnesses,'trimTangentValidation':tangent_validation,'allSolidTangentValidation':full_tangent_validation,'primitiveCountIncludingWalkable':sum(len(m['primitives']) for m in doc2['meshes']),'materialCount':len(doc2['materials']),'runtimeCounts':{'vertices':vertex_total,'indices':index_total,'triangles':index_total//3},'affectedViewTitles':['B01 Tavern - Square South Gable','B07 Store - Street Gable','B08 Outbuilding - Street Gable'],'roofMaterialPbr':{'baseColorFactor':[1,1,1,1],'metallicFactor':0,'roughnessFactor':1.0},'uvConvention':{'U':'along ridge Z, 2.4 metres per repeat','V':'eave V=0 to ridge, 2.4 metres per repeat','tangent':'U along +Z; handedness is derived from the outward slope normal on each face'},'collisionNote':f'{len(selected_roof)} existing slope triangles are relabelled with unchanged positions; {len(selected_gable)} existing gable triangles are relabelled as masonry with unchanged positions; {added_tri} new closed solid triangles are in the BellwetherSolid collision source and remain non-walkable. No underside, ancillary roof, pot or church-spire geometry changed.'}
RECEIPT.write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8',newline='\n')
import shutil
shutil.copy2(__file__,RECIPE)
README.write_text('''# Bellwether roofline v1\n\nOriginal editable roof and gable refinement for mapped village buildings B01-B08. The main roof slopes use the textureless `HordeVillageRoofTiles` material; triangular end faces are masonry in `HordeTownStone`. Closed oak verge boards, gable collars and king posts, plus closed half-round clay ridge caps, are authored at metre scale.\n\n`bellwether-roofline-v1.blend` is the editable derivative of the nine-building shell source, without dormant terrain meshes. `author-bellwether-roofline.py` regenerates this source bundle and a runtime candidate in a fresh output directory. The delivered runtime uses the original four material groups in their existing order, with the appended roof primitive last in `BellwetherSolid`; `BellwetherWalkable` remains separate.\n\nThe pinned parent content is the town GLB at `ec850e56` (SHA-256 `5c0b850b1159e5417f0d58800e8d38dce9c1a12b80f1ddd4c9e32a229b82e6de`) and its sibling manifest (SHA-256 `ac31c5dba4d4e4018fda77662201c2108f6ef42b0f22449770a685a7b826a3f6`). The repository stores the large GLB via LFS. On a fresh checkout, run from repo root:\n```powershell\nblender --background --python-exit-code 1 --python assets/models/world/source/bellwether-roofline-v1/author-bellwether-roofline.py -- . <fresh-output-directory> <ec850e56-town.glb> <same-directory-asset.manifest.json>\n```\nThe recipe verifies both parent hashes, takes explicit parent inputs, and does not depend on reverting the installed runtime. The output directory must not already exist.\n\nUV contract: exported glTF U runs along the ridge (Z) at 2.4 m per repeat; V starts at 0 on the eave and increases uphill at 2.4 m per repeat. Tangent handedness is derived from each outward roof slope normal.\n''',encoding='utf-8',newline='\n')
print(json.dumps({'receipt':str(RECEIPT),'sourceSha256':receipt['sourceMasterSha256'],'originalTownSha256':original_town_sha,'runtimeTownSha256':receipt['runtimeTownSha256'],'addedTriangles':added_tri,'roofTriangles':len(selected_roof),'gableTriangles':len(selected_gable),'solidTriangles':solid_tri,'perBuilding':by_build},indent=2))
