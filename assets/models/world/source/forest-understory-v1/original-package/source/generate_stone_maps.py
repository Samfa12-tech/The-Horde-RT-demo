"""Original deterministic procedural stone PBR authoring; no external image input.
All maps 512 px; authored for The Horde by OpenAI on 2026-10-06.
"""
from pathlib import Path
import numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'textures'; OUT.mkdir(exist_ok=True)
N=512
rng=np.random.default_rng(107604)
def field(cells):
    a=rng.random((cells,cells)); t=np.arange(N)*cells/N
    i=np.floor(t).astype(int); q=t-i; q=q*q*(3-2*q)
    x=i[None,:]; y=i[:,None]; u=q[None,:]; v=q[:,None]
    return (a[y,x]*(1-u)*(1-v)+a[y,(x+1)%cells]*u*(1-v)+a[(y+1)%cells,x]*(1-u)*v+a[(y+1)%cells,(x+1)%cells]*u*v)
y,x=np.mgrid[0:N,0:N]/N
macro=field(5); mid=field(16); fine=field(54); grain=field(128)
strata=np.sin((y*13+macro*2.3+mid*.4)*2*np.pi)
fracture=.65*np.power(np.maximum(0,1-np.abs(field(14)-.5)/.026),2)+.25*np.power(np.maximum(0,1-np.abs(strata)/.13),2)*np.clip((field(9)-.4)*2,0,1)
height=.46*macro+.23*mid+.1*fine+.04*grain-.11*fracture
base=np.empty((N,N,3)); value=(macro-.5)*.065+(mid-.5)*.048+(fine-.5)*.028+(grain-.5)*.025+np.maximum(0,grain-.67)*.075
for k,c in enumerate([.205,.219,.208]): base[:,:,k]=c+value
base-=fracture[:,:,None]*np.array([.028,.029,.028])
# Restricted olive weathering rather than bright moss camouflage.
weather=np.clip((field(7)-.69)*2,0,.2)
base+=weather[:,:,None]*np.array([.005,.036,-.015])
rough=np.clip(.89+.07*(mid-.5)+.04*(grain-.5)+fracture*.055,0,1)
gy,gx=np.gradient(height); nx=-gx*11; ny=-gy*11; nz=np.ones_like(nx)
d=np.sqrt(nx*nx+ny*ny+nz*nz); normal=np.stack([nx/d*.5+.5,ny/d*.5+.5,nz/d*.5+.5],2)
for suffix,a in [('basecolor',base),('normal',normal),('roughness',rough)]:
    p=OUT/f'horde-stone-original-{suffix}-512.png'
    Image.fromarray(np.uint8(np.clip(a,0,1)*255)).save(p)
print('Original 512px stone maps generated:',OUT)
