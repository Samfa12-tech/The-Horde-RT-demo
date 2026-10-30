"""Independent binary glTF checks, embedded decoded texture sizes, lineage hashes."""
from pathlib import Path
import json, struct, hashlib, io, numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
TYPES={5120:('i1',1),5121:('u1',1),5122:('<i2',2),5123:('<u2',2),5125:('<u4',4),5126:('<f4',4)}
SHAPES={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}
rows=[];all_images={}
for p in sorted((ROOT/'assets').glob('*.glb')):
    b=p.read_bytes();magic,ver,total=struct.unpack_from('<4sII',b,0);assert magic==b'glTF' and ver==2 and total==len(b)
    off=12;g=None;binary=None
    while off<len(b):
        n,tag=struct.unpack_from('<I4s',b,off);chunk=b[off+8:off+8+n];off+=8+n
        if tag==b'JSON':g=json.loads(chunk)
        elif tag==b'BIN\x00':binary=chunk
    def arr(index):
        a=g['accessors'][index];assert 'sparse' not in a
        bv=g['bufferViews'][a['bufferView']];dt,size=TYPES[a['componentType']];n=SHAPES[a['type']];stride=bv.get('byteStride',n*size);offset=bv.get('byteOffset',0)+a.get('byteOffset',0)
        return np.ndarray((a['count'],n),dtype=dt,buffer=binary,offset=offset,strides=(stride,size))
    checks=[];tri=0;positions=[]
    for mesh in g['meshes']:
        for primitive in mesh['primitives']:
            assert primitive.get('mode',4)==4
            indices=arr(primitive['indices']).ravel();tri+=len(indices)//3;pos=arr(primitive['attributes']['POSITION']);positions.append(pos)
            assert len(indices)%3==0 and int(indices.max())<len(pos)
            for name,ix in primitive['attributes'].items():
                a=arr(ix);assert np.isfinite(a).all();checks.append(name)
                if name=='NORMAL':assert np.max(np.abs(np.linalg.norm(a,axis=1)-1))<.002
            assert all(x in primitive['attributes'] for x in ['POSITION','NORMAL','TEXCOORD_0'])
    mats=g.get('materials',[])
    assert all(m.get('alphaMode','OPAQUE')=='OPAQUE' for m in mats)
    ims=[]
    for im in g.get('images',[]):
        bv=g['bufferViews'][im['bufferView']];data=binary[bv.get('byteOffset',0):bv.get('byteOffset',0)+bv['byteLength']];sha=hashlib.sha256(data).hexdigest();img=Image.open(io.BytesIO(data));img.load();w,h=img.size
        item={'name':im.get('name'), 'sha256':sha,'width':w,'height':h,'decoded_mode':img.mode,'decoded_bytes':len(img.tobytes()),'conservative_rgba8_bytes_without_mips':w*h*4}
        assert w==512 and h==512
        ims.append(item);all_images[sha]=item
    pos=np.concatenate(positions);mn=pos.min(axis=0);mx=pos.max(axis=0)
    assert mn[1]>-.000001
    rows.append({'asset':p.name,'glb_bytes':len(b),'glb_sha256':hashlib.sha256(b).hexdigest(),'triangles':tri,'mesh_count':len(g['meshes']),'primitive_count':sum(len(m['primitives']) for m in g['meshes']),'material_count':len(mats),'texture_count':len(g.get('textures',[])),'image_count':len(ims),'all_materials_opaque':True,'positions_normals_uvs_finite':True,'normals_unit_within_0_002':True,'gltf_y_up_bounds_min':[round(float(x),6) for x in mn],'gltf_y_up_dimensions':[round(float(x),6) for x in mx-mn],'embedded_images':ims,'extensions_used':g.get('extensionsUsed',[])})
source=[]
for p in sorted((ROOT/'textures').glob('*.png')):
    im=Image.open(p);shared=p.name.startswith(('horde-bark','horde-alder'))
    source.append({'file':p.name,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'dimensions':list(im.size),'mode':im.mode,'decoded_bytes':len(im.tobytes()),'provenance':'Original tree-pair shared maps; same bytes' if shared else 'Original dressing procedural maps','generator':'shared_tree_material_provenance.py' if shared else ('generate_stone_maps.py' if 'stone' in p.name else 'generate_endgrain_maps.py')})
summary={'validator':'Independent Python GLB binary inspection; not game-dev CLI/Khronos/GPU validation','assets':rows,'unique_embedded_images_across_kit':len(all_images),'unique_embedded_decoded_bytes':sum(i['decoded_bytes'] for i in all_images.values()),'conservative_unique_rgba8_bytes_no_mips':sum(i['conservative_rgba8_bytes_without_mips'] for i in all_images.values()),'texture_residency_note':'Per-GLB files embed shared textures for portability. Importer should deduplicate maps/materials by lineage or hash. Reported decoded source bytes and RGBA8 estimates are not measured GPU residency; engine format/mips/compression determine actual use.','source_texture_lineage':source}
(ROOT/'validation'/'static-glb-checks.json').write_text(json.dumps(summary,indent=2));print(json.dumps({k:v for k,v in summary.items() if k not in ['assets','source_texture_lineage']},indent=2))
