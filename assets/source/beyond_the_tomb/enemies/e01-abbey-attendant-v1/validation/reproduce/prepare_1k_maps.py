import pathlib,sys,json,hashlib,numpy as np
from PIL import Image,ImageDraw,ImageFilter
sys.path.insert(0,str(pathlib.Path(__file__).parent));from inspect_glb import load_glb,accessor
src=pathlib.Path(sys.argv[1]);out=pathlib.Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
def read(name):return np.array(Image.open(src/name)).astype(np.float32)/255
def down(a):
 f=a.shape[0]//1024
 return a.reshape(1024,f,1024,f,*a.shape[2:]).mean((1,3))
def save(a,name):Image.fromarray(np.clip(np.round(a*255),0,255).astype('uint8')).save(out/name)
a=read('texture_0_base_color.png');linear=np.where(a<=.04045,a/12.92,((a+.055)/1.055)**2.4);linear=down(linear);base=np.where(linear<=.0031308,linear*12.92,1.055*linear**(1/2.4)-.055);save(base,'E01_base_color_1k.png')
n=down(read('texture_0_normal.png')*2-1);n/=np.maximum(np.linalg.norm(n,axis=2,keepdims=True),1e-10);save(n*.5+.5,'E01_normal_1k.png')
r=down(read('texture_0_roughness.png'));metal=down(read('texture_0_metallic.png'))
_,g,b=load_glb(src/'model.glb');pr=g['meshes'][0]['primitives'][0];pos=accessor(g,b,pr['attributes']['POSITION']);uv=accessor(g,b,pr['attributes']['TEXCOORD_0']);tri=accessor(g,b,pr['indices']).reshape(-1,3)
mask=Image.new('L',(1024,1024));draw=ImageDraw.Draw(mask)
selected=pos[tri,:,].mean(1)[:,1]<-.70
for ids in tri[selected]:draw.polygon([tuple(v*1023) for v in uv[ids]],fill=255)
mask=mask.filter(ImageFilter.GaussianBlur(.65));a=np.asarray(mask,dtype=np.float32)/255;a*=metal<.2
before=r.copy();r=r+(np.maximum(r,.60)-r)*a
orm=np.stack([np.ones_like(r),r,metal],axis=2);save(orm,'E01_orm_1k.png')
mask.save(out/'boot_roughness_mask.png')
report={'source_glb_sha256':hashlib.sha256((src/'model.glb').read_bytes()).hexdigest(),'source_dir':str(src),'method':{'base':'2x area reduction in linear light, encoded sRGB','normal':'2x vector average, renormalized, non-color tangent normal','roughness_metal':'4x linear scalar area average','occlusion':'neutral 1, no AO supplied','boot_roughness':'UV triangle mask for original Y<-0.70 m; nonmetal pixels only; roughness floor 0.60, 0.65px feather; source unchanged'},'boot_triangles':int(selected.sum()),'changed_roughness_texels':int((abs(r-before)>1e-6).sum()),'all_texture_sizes':[1024,1024],'files':{}}
for p in out.glob('*.png'):report['files'][p.name]={'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'bytes':p.stat().st_size}
(out/'texture-processing.json').write_text(json.dumps(report,indent=2)+'\n')
