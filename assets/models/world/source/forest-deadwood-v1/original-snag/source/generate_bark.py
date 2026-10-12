"""Original deterministic periodic bark maps; no sampled source photographs."""
import numpy as np
from PIL import Image
from pathlib import Path
p=Path(__file__).resolve().parents[1]/'textures';p.mkdir(exist_ok=True);n=1024
x,y=np.meshgrid(np.arange(n)/n,np.arange(n)/n);rng=np.random.default_rng(47)
phase=x+.009*np.sin(y*2*np.pi*3)+.004*np.sin(y*2*np.pi*11+x*2*np.pi*2);h=np.zeros_like(x)
for f,amp in [(9,.35),(19,.23),(41,.15),(89,.07)]:h+=amp*np.sin(phase*2*np.pi*f+np.sin(y*2*np.pi*(f%7+1))*.8)
fiss=np.maximum(0,(np.sin(phase*2*np.pi*19+np.sin(y*2*np.pi*3)*1.3)-.60)/.40)**2
h=.5+h*.26-fiss*.22;h=np.clip(h+rng.normal(0,.017,(n,n)),0,1);gray=np.clip(.35+h*.30-fiss*.17,0,1);base=np.stack((gray*.85,gray*.79,gray*.69),-1)
Image.fromarray((np.clip(base,0,1)*255).astype('uint8'),'RGB').save(p/'snag_bark_basecolor.png')
dx=(np.roll(h,-1,1)-np.roll(h,1,1))*3;dy=(np.roll(h,-1,0)-np.roll(h,1,0))*3;norm=np.stack((-dx,dy,np.ones_like(h)),-1);norm/=np.linalg.norm(norm,axis=-1,keepdims=True)
Image.fromarray(((norm*.5+.5)*255).astype('uint8'),'RGB').save(p/'snag_bark_normal.png')
rough=np.clip(.80+(.5-h)*.15,0,1);orm=np.stack((np.ones_like(h),rough,np.zeros_like(h)),-1)
Image.fromarray((orm*255).astype('uint8'),'RGB').save(p/'snag_bark_orm.png')
Image.fromarray((rough*255).astype('uint8'),'L').save(p/'snag_bark_roughness.png')
