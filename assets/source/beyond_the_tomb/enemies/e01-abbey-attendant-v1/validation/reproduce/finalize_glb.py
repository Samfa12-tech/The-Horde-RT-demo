import pathlib,sys,json,struct,numpy as np,hashlib
from inspect_glb import load_glb,accessor
p=pathlib.Path(sys.argv[1]);out=pathlib.Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
_,g,b=load_glb(p);evidence=[]
for a in g['animations']:
 bound=0.;count=0
 for s in a['samplers']:
  if s.get('interpolation','LINEAR')=='STEP':
   values=accessor(g,b,s['output']);delta=float(np.ptp(values,axis=0).max());assert delta<1e-4,(a['name'],delta)
   s['interpolation']='LINEAR';count+=1;bound=max(bound,delta)
 evidence.append({'clip':a['name'],'constant_or_nearconstant_STEP_samplers_changed_to_LINEAR':count,'max_component_interpolation_difference_bound':bound})
def save(root,path):
 j=json.dumps(root,separators=(',',':')).encode();j+=b' '*((-len(j))%4);bb=b+b'\0'*((-len(b))%4);data=struct.pack('<III',0x46546c67,2,28+len(j)+len(bb))+struct.pack('<II',len(j),0x4e4f534a)+j+struct.pack('<II',len(bb),0x004e4942)+bb;path.write_bytes(data)
 return {'file':str(path),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
all_result=save(g,out/'E01_AbbeyAttendant_all_clips.glb')
g['animations']=[a for a in g['animations'] if a['name'] in ['Idle_5','Walking','Attack','Dead']];assert len(g['animations'])==4
core_result=save(g,out/'E01_AbbeyAttendant_core.glb')
(out/'linear-export-receipt.json').write_text(json.dumps({'source':str(p),'changes':evidence,'all':all_result,'core':core_result,'note':'All animation channels sampled at 60Hz. Blender emits STEP on constant channels; relabel only after a strict <1e-4 component-range check. Extra animation buffers retained unused in core file; no geometry or PBR change.'},indent=2)+'\n')
