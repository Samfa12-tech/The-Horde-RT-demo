import sys,pathlib,json,struct,hashlib
from inspect_glb import load_glb
source,dest=map(pathlib.Path,sys.argv[1:3]);old,root,b=load_glb(source);renamed=[]
for a in root['animations']:
 if a['name']=='Idle_5':a['name']='Idle';renamed.append('Idle_5 -> Idle')
j=json.dumps(root,separators=(',',':')).encode();j+=b' '*((-len(j))%4);b+=b'\0'*((-len(b))%4);data=struct.pack('<III',0x46546c67,2,28+len(j)+len(b))+struct.pack('<II',len(j),0x4e4f534a)+j+struct.pack('<II',len(b),0x004e4942)+b;dest.write_bytes(data)
print(json.dumps({'source':str(source),'output':str(dest),'names':[a['name']for a in root['animations']],'renamed':renamed,'binary_sha256_unchanged':hashlib.sha256(b).hexdigest(),'sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data)}))
