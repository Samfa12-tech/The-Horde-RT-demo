import json,struct,pathlib,sys,math,hashlib,numpy as np
from inspect_e05_glb import load_glb,accessor
p=pathlib.Path(sys.argv[1]);raw,j,b=load_glb(p);b=bytearray(b);node=next(i for i,n in enumerate(j['nodes'])if n.get('name')=='RightGrip');fix=np.array([math.sqrt(.5),0,0,math.sqrt(.5)])
def mul(a,c):
 av=np.array(a[:3]);cv=np.array(c[:3]);return np.r_[a[3]*cv+c[3]*av+np.cross(av,cv),a[3]*c[3]-av.dot(cv)]
def put(idx,values):
 a=j['accessors'][idx];v=j['bufferViews'][a['bufferView']];arr=np.asarray(values,dtype='<f4').reshape(a['count'],-1);stride=v.get('byteStride',arr.shape[1]*4);start=v.get('byteOffset',0)+a.get('byteOffset',0)
 for i,row in enumerate(arr):b[start+i*stride:start+i*stride+row.nbytes]=row.tobytes()
 if 'min'in a:a['min']=arr.min(0).tolist()
 if 'max'in a:a['max']=arr.max(0).tolist()
j['nodes'][node]['rotation']=mul(j['nodes'][node].get('rotation',[0,0,0,1]),fix).tolist();j['nodes'][node].setdefault('extras',{})['socket_axes']='Static prop-compatible glTF basis: handle +Y, head +X; unit world scale; original Blender tool frame uses handle +Z.'
changed=[]
for a in j['animations']:
 for c in a['channels']:
  if c['target']['node']==node and c['target']['path']=='rotation':
   idx=a['samplers'][c['sampler']]['output'];vals=accessor(j,b,idx);put(idx,[mul(q,fix)for q in vals]);changed.append(a['name'])
sk=j['skins'][0];ib=accessor(j,b,sk['inverseBindMatrices']).reshape(-1,4,4).transpose(0,2,1);ji=sk['joints'].index(node);C=np.array([[1,0,0,0],[0,0,1,0],[0,-1,0,0],[0,0,0,1.]]);ib[ji]=C@ib[ji];put(sk['inverseBindMatrices'],ib.transpose(0,2,1).reshape(-1,16))
payload=json.dumps(j,separators=(',',':')).encode();payload+=b' '*((-len(payload))%4);b+=b'\0'*((-len(b))%4);out=struct.pack('<III',0x46546c67,2,28+len(payload)+len(b))+struct.pack('<II',len(payload),0x4e4f534a)+payload+struct.pack('<II',len(b),0x004e4942)+b;p.write_bytes(out)
p.with_suffix('.socket-axes.json').write_text(json.dumps({'source_before_axis_correction_sha256':hashlib.sha256(raw).hexdigest(),'final_sha256':hashlib.sha256(out).hexdigest(),'changed_node':'RightGrip','clips':changed,'change':'Postmultiply RightGrip local rotation by +90deg X, and premultiply its unused inverse-bind by -90deg X; adapts Blender +Z tool shaft to standalone glTF +Y shaft. Body/shell weights to this socket are zero. No other geometry, rig keys or PBR changes.'},indent=2)+'\n')
