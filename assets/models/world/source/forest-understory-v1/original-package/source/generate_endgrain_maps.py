"""Original deterministic decayed end-grain PBR; no external images."""
from pathlib import Path
import numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'textures'
y,x=np.mgrid[0:512,0:512]/511-.5
r=np.sqrt(x*x+y*y);a=np.arctan2(y,x)
warp=.008*np.sin(a*3+r*19)+.004*np.cos(a*7-r*16)
ring=np.sin((r+warp)*175);ring2=np.clip((ring-.30)*1.8,0,1)
rng=np.random.default_rng(27413);grain=rng.random(r.shape)-.5
crack=np.zeros_like(r)
for angle,start in [(.4,.15),(1.95,.23),(-1.4,.10),(-2.65,.30)]:
    da=np.abs(np.arctan2(np.sin(a-angle-.024*np.sin(r*45)),np.cos(a-angle-.024*np.sin(r*45))))
    crack=np.maximum(crack,np.clip((.016-da)/.010,0,1)*np.clip((r-start)*18,0,1))
v=ring2*-.035+grain*.025+.022*np.sin(r*10)-crack*.09
base=np.stack([.245+v,.213+v*.93,.164+v*.83],axis=2)
height=.012*ring+.02*grain-.09*crack
gy,gx=np.gradient(height);nx=-gx*7;ny=-gy*7;nz=np.ones_like(nx);norm=np.sqrt(nx*nx+ny*ny+nz*nz)
normal=np.stack([nx/norm*.5+.5,ny/norm*.5+.5,nz/norm*.5+.5],2)
rough=np.clip(.88+.06*grain+crack*.08,0,1)
for suffix,v in [('basecolor',base),('normal',normal),('roughness',rough)]:
    Image.fromarray(np.uint8(np.clip(v,0,1)*255)).save(OUT/f'horde-endgrain-original-{suffix}-512.png')
print('Original endgrain maps generated.')
