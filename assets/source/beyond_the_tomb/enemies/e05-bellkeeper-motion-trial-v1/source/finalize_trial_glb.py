import sys,pathlib,json,struct,hashlib,numpy as np
from inspect_e05_glb import load_glb,accessor
src=pathlib.Path(sys.argv[1]);out=pathlib.Path(sys.argv[2]);raw,j,b=load_glb(src);changes=[]
for a in j['animations']:
 count=0;maximum=0
 for s in a['samplers']:
  if s.get('interpolation')=='STEP':
   v=accessor(j,b,s['output']);delta=float(np.ptp(v,axis=0).max());assert delta<1e-4,(a['name'],delta);s['interpolation']='LINEAR';count+=1;maximum=max(maximum,delta)
 changes.append({'clip':a['name'],'constant_STEP_to_LINEAR_channels':count,'max_component_range':maximum})
payload=json.dumps(j,separators=(',',':')).encode();payload+=b' '*((-len(payload))%4);b+=b'\x00'*((-len(b))%4);data=struct.pack('<III',0x46546c67,2,28+len(payload)+len(b))+struct.pack('<II',len(payload),0x4e4f534a)+payload+struct.pack('<II',len(b),0x004e4942)+b;out.write_bytes(data)
out.with_suffix('.linear-receipt.json').write_text(json.dumps({'source':str(src),'source_sha256':hashlib.sha256(raw).hexdigest(),'output':str(out),'output_sha256':hashlib.sha256(data).hexdigest(),'changes':changes,'note':'Only constant STEP interpolation tags changed after numeric check; no binary/PBR/geometry/key-value edits.'},indent=2)+'\n')
