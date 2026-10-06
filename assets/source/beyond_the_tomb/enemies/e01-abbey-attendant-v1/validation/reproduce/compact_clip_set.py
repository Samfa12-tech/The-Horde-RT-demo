import copy,pathlib,sys,json,struct,hashlib
from inspect_glb import load_glb
source,out=map(pathlib.Path,sys.argv[1:3]);out.mkdir(parents=True,exist_ok=True);_,root,binary=load_glb(source)
def compact(names,leaf):
 g=copy.deepcopy(root);g['animations']=[next(a for a in g['animations'] if a['name']==name)for name in names];used=set()
 for mesh in g['meshes']:
  for p in mesh['primitives']:used.update(p['attributes'].values());used.add(p['indices'])
 for skin in g['skins']:used.add(skin['inverseBindMatrices'])
 for a in g['animations']:
  for s in a['samplers']:used.update([s['input'],s['output']])
 ai={old:new for new,old in enumerate(sorted(used))};g['accessors']=[g['accessors'][old]for old in sorted(used)];views={a['bufferView']for a in g['accessors']}|{i['bufferView']for i in g['images']};vi={old:new for new,old in enumerate(sorted(views))};newbin=bytearray();newviews=[]
 for old in sorted(views):
  v=copy.deepcopy(g['bufferViews'][old]);start=v.get('byteOffset',0);data=binary[start:start+v['byteLength']];newbin.extend(b'\0'*((-len(newbin))%4));v['byteOffset']=len(newbin);newbin.extend(data);newviews.append(v)
 for a in g['accessors']:a['bufferView']=vi[a['bufferView']]
 for i in g['images']:i['bufferView']=vi[i['bufferView']]
 for mesh in g['meshes']:
  for p in mesh['primitives']:p['attributes']={k:ai[v]for k,v in p['attributes'].items()};p['indices']=ai[p['indices']]
 for skin in g['skins']:skin['inverseBindMatrices']=ai[skin['inverseBindMatrices']]
 for a in g['animations']:
  for s in a['samplers']:s['input']=ai[s['input']];s['output']=ai[s['output']]
 g['bufferViews']=newviews;g['buffers']=[{'byteLength':len(newbin)}];j=json.dumps(g,separators=(',',':')).encode();j+=b' '*((-len(j))%4);newbin.extend(b'\0'*((-len(newbin))%4));data=struct.pack('<III',0x46546c67,2,28+len(j)+len(newbin))+struct.pack('<II',len(j),0x4e4f534a)+j+struct.pack('<II',len(newbin),0x004e4942)+newbin;p=out/leaf;p.write_bytes(data)
 return {'file':str(p),'clips':names,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
r={'source':str(source),'core':compact(['Idle_5','Walking','Attack','Dead'],'E01_AbbeyAttendant_core.glb'),'extras':compact(['Hit_Reaction','Idle_Turn_Left','Idle_Turn_Right','Running'],'E01_AbbeyAttendant_source_extras.glb')};(out/'clip-set-receipt.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
