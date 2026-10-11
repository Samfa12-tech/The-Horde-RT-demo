"""Bounded NumPy-only terrain source derived from the pinned regional atlas.

Original design: d37f1638/docs/design/world/dungeons/build_atlas.py, lines 58-94.
No drawing, file mutation, renderer feature or runtime dependency. The local
chapter author retains its accepted metre-scale interfaces over this macro land.
"""
import hashlib
import json
import numpy as np


def bilinear(x, y, heights, east, north):
    u = np.clip((east-x[0])/(x[1]-x[0]), 0, len(x)-1.0000001)
    v = np.clip((north-y[0])/(y[1]-y[0]), 0, len(y)-1.0000001)
    i, j = u.astype(int), v.astype(int)
    a, b = u-i, v-j
    return (1-b)*((1-a)*heights[j,i]+a*heights[j,i+1])+b*((1-a)*heights[j+1,i]+a*heights[j+1,i+1])


def broad_relief(east, north):
    return (20+.064*north+45*np.exp(-((east-310)/135)**2-((north-290)/220)**2)
            +22*np.exp(-((east-245)/130)**2-((north+25)/165)**2)
            +5*np.sin(east/57)*np.cos(north/76))


def nearest(east, north, points):
    distance=np.full(east.shape,np.inf); elevation=np.zeros_like(east)
    for a,b in zip(points[:-1],points[1:]):
        a,b=np.asarray(a),np.asarray(b);v=b[:2]-a[:2]
        t=np.clip(((east-a[0])*v[0]+(north-a[1])*v[1])/np.dot(v,v),0,1)
        d=np.hypot(east-a[0]-t*v[0],north-a[1]-t*v[1]);closer=d<distance
        elevation=np.where(closer,a[2]+t*(b[2]-a[2]),elevation)
        distance=np.minimum(distance,d)
    return distance,elevation


def gaussian10(values):
    # Same sigma=10, truncate=4 and half-sample reflection as the atlas's
    # scipy.ndimage.gaussian_filter. NumPy is already bundled with Blender.
    kernel=np.exp(-np.arange(-40,41,dtype=float)**2/200);kernel/=kernel.sum()
    result=values.copy()
    for axis in (0,1):
        pads=[(0,0),(0,0)];pads[axis]=(40,40)
        padded=np.pad(result,pads,mode='symmetric')
        result=sum(w*np.take(padded,np.arange(result.shape[axis])+i,axis=axis)
                   for i,w in enumerate(kernel))
    return result


class RegionalTerrain:
    def __init__(self, baseline, layout_path):
        if hashlib.sha256(layout_path.read_bytes()).hexdigest()!='0a4da29bf8d988145c0f7b3f1c7afaf415a5da9b14c34bbd53bd1e00e3a86e59':
            raise ValueError('Regional layout differs from the reviewed atlas baseline')
        layout=json.loads(layout_path.read_text())
        self.x=np.arange(-150.,411.);self.y=np.arange(-510.,441.)
        X,Y=np.meshgrid(self.x,self.y);Z=broad_relief(X,Y)
        bx,by=baseline['x'],baseline['y']
        inside=(X>=bx[0])&(X<=bx[-1])&(Y>=by[0])&(Y<=by[-1])
        weight=np.clip(np.minimum.reduce([X+145,160-X,Y+260,240-Y])/60,0,1)
        local=bilinear(bx,by,baseline['height'],X,Y)
        Z=np.where(inside,Z*(1-weight)+local*weight,Z)
        blurred=gaussian10(Z)
        edge=np.minimum.reduce([abs(X-bx[0]),abs(X-bx[-1]),abs(Y-by[0]),abs(Y-by[-1])])
        blend=np.clip(edge/25,0,1)
        Z=np.where((~inside)&(edge<25),blurred*(1-blend)+Z*blend,Z)
        lake=np.array([[30,-240],[68,-246],[104,-262],[148,-300],[166,-349],[148,-400],
            [103,-444],[50,-459],[0,-448],[-38,-415],[-61,-365],[-59,-310],
            [-30,-274],[-8,-285],[2,-267],[-5,-252]])
        wet=np.zeros(X.shape,dtype=bool)
        for a,b in zip(lake,np.roll(lake,-1,axis=0)):
            if a[1]==b[1]:continue
            wet^=((a[1]>Y)!=(b[1]>Y))&(X<(b[0]-a[0])*(Y-a[1])/(b[1]-a[1])+a[0])
        Z=np.where(wet,-7+1.3*np.cos(X/50)*np.cos(Y/60),Z)
        for river in [layout['river_surface'],[[135,220,40],[170,290,51],[193,360,65],[214,430,79]],
                      [[0,-448,0],[-28,-478,-.4],[-55,-510,-1.2]]]:
            d,e=nearest(X,Y,river);w=np.clip((13-d)/8,0,1)
            Z=Z*(1-w)+(e-1+np.maximum(d-3,0)*.6)*w
        routes=[[[47,-70,14.9],[31,-100,13],[15,-140,10.5],[-2,-190,7.5],[-18,-224,5.3],[-20,-248,3],[0,-265,1.5]],
            [[47,-70,14.9],[43,-84,14],[51,-94,13.5]],
            [[89,-94,13.5],[119,-76,17],[148,-72,20],[179,-55,24],[213,-47,28],[230,-40,30]],
            [[119,-76,17],[133,-36,21],[146,8,26],[169,51,31],[193,76,36],[172,104,40],
             [201,133,45],[230,153,49],[209,183,53],[236,211,59],[272,227,64],[257,258,69],[282,283,75],[306,308,80]]]
        for route in routes:
            d,e=nearest(X,Y,route);w=np.clip((7-d)/5,0,1);Z=Z*(1-w)+e*w
        for x,y,z,w,h in [(230,-40,30,20,16),(306,308,80,18,14),(-20,-248,3,12,10)]:
            d=np.maximum(abs(X-x)-w/2,abs(Y-y)-h/2);blend=np.clip((5-d)/5,0,1)
            Z=Z*(1-blend)+z*blend
        self.height=Z

    def sample(self,east,north):
        value=bilinear(self.x,self.y,self.height,east,north)
        # Outside the reviewed map: explicitly provisional coarse continuation,
        # not a clamp of the local sheet. At the boundary this is continuous;
        # the outer closed envelope is beyond the unchanged 10km primary ray.
        distance=np.hypot(np.maximum(np.maximum(self.x[0]-east,east-self.x[-1]),0),
                          np.maximum(np.maximum(self.y[0]-north,north-self.y[-1]),0))
        t=np.clip(distance/300,0,1);t=t*t*(3-2*t)
        surrounding=broad_relief(east,np.clip(north,-700,700))
        return value*(1-t)+surrounding*t


def ring_coordinates(bounds, spacing):
    """Four CCW sides in engine X/Z; each includes both shared corner points."""
    xmin,xmax,zmin,zmax=bounds
    nx=max(1,int(np.ceil((xmax-xmin)/spacing)));nz=max(1,int(np.ceil((zmax-zmin)/spacing)))
    return [list(zip(np.linspace(xmin,xmax,nx+1),np.full(nx+1,zmin))),
            list(zip(np.full(nz+1,xmax),np.linspace(zmin,zmax,nz+1))),
            list(zip(np.linspace(xmax,xmin,nx+1),np.full(nx+1,zmax))),
            list(zip(np.full(nz+1,xmin),np.linspace(zmax,zmin,nz+1)))]


def stitch_sides(inner,outer,vertices):
    """Join matching rectangle sides without cracks or thin global grid stripes."""
    triangles=[]
    for a,b in zip(inner,outer):
        i=j=0
        while i<len(a)-1 or j<len(b)-1:
            if i<len(a)-1 and (j==len(b)-1 or (i+1)/(len(a)-1)<=(j+1)/(len(b)-1)):
                tri=(a[i],a[i+1],b[j]);i+=1
            else:
                tri=(a[i],b[j+1],b[j]);j+=1
            p,q,r=(vertices[k] for k in tri)
            # Blender X,-engineZ orientation; outward terrain normal is +Z.
            area=(q[0]-p[0])*(r[1]-p[1])-(q[1]-p[1])*(r[0]-p[0])
            if abs(area)<1e-9:raise ValueError('Degenerate regional stitch')
            triangles.append(tri if area>0 else (tri[0],tri[2],tri[1]))
    return triangles
