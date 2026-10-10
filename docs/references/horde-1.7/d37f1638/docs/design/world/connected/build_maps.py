import os,json,csv,math
os.environ['MPLCONFIGDIR']='/tmp/horde-mpl'
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle,Polygon,Circle,FancyArrowPatch
from matplotlib.colors import LinearSegmentedColormap,LightSource
from matplotlib.backends.backend_pdf import PdfPages
from scipy.interpolate import RegularGridInterpolator
P=os.path.dirname(__file__)
plt.rcParams.update({'font.family':'DejaVu Sans','font.size':10,'svg.fonttype':'none','pdf.fonttype':42,'axes.edgecolor':'#667467','text.color':'#26342e','axes.labelcolor':'#34483c','xtick.color':'#526257','ytick.color':'#526257'})
BG='#f4f1e8';INK='#233d34';GREEN='#40694e';GOLD='#b78533';BLUE='#477f96';ORANGE='#aa6341'
forest=np.array([[-112,139,33.6],[-107,132,33.0],[-94,126,32.1],[-96,111,31.0],[-82,106,31.3],[-65,90,30.4]])
arrival=np.array([[-65,90,30.4],[-56,76,28.8],[-59,58,27.2],[-47,44,25.5],[-30,34,23.6],[-19,18,21.8],[-4,4,20],[0,0,20.]])
churchpath=np.array([[-47,44,25.5],[-36,49,26.6],[-25,59,28.0],[-16,61,29],[-14,51,29],[-2,51,29],[-2,53,29]])
millpath=np.array([[0,0,20],[25,1,18.4],[35,6,17.0],[40,13,16]])
southpath=np.array([[0,0,20],[9,-18,19.1],[18,-37,18.1],[35,-55,16.3],[47,-70,14.9]])
shrinepath=np.array([[-16,61,29],[-17,81,30],[0,90,31],[25,97,31.2]])
river=np.array([[30,-240,0],[44,-170,.8],[60,-110,2.3],[62,-80,3],[58,-25,6],[68,25,10],[68,65,14],[78,105,20],[105,160,29],[135,220,40]])
race=np.array([[68,55,13],[49,48,12.8],[49,25,12.3],[59,25,12.2],[59,20,12.1],[59,12,9.5],[62,-3,7.8]])
# River at Y=-3 is 7.76 m: tailrace reaches matching water level.
race[-1,2]=np.interp(-3,river[:,1],river[:,2])
B=[('B01','Tavern',12,15,22,16,20.4,4,7,'Hero interior'),('B02','Church',-2,65,18,24,29,-2,53,'Exterior; service outside'),('B03','House A',-43,8,16,12,20.8,-35,8,'Optional interior'),('B04','House B',-47,29,16,14,24.6,-39,29,'Optional interior'),('B05','Mill',48,13,16,16,16,40,13,'Exterior / working edge'),('B06','Smith',29,-29,18,14,18.2,20,-29,'Exterior service yard'),('B07','Store',-22,-28,13,11,19.9,-15.5,-28,'Closed shell'),('B08','Outbuilding',1,-44,12,9,18.2,1,-39.5,'Closed shell'),('B09','Starter shrine',25,101,8,8,31.2,25,97,'Nearby spur; provisional')]
localpaths=[np.array([[-5,3,20],[-19,6,20.5],[-35,8,20.8]]),np.array([[-30,34,23.6],[-39,29,24.6]]),np.array([[4,1,20],[4,7,20.4]]),np.array([[-5,-5,20],[-9,-20,20],[-15.5,-28,19.9]]),np.array([[14,-29,18.6],[20,-29,18.2]]),np.array([[18,-37,18.1],[1,-39.5,18.2]])]
bridgepath=np.array([[40,13,16],[34,25,17],[38,37,18],[45.5,37,18],[52.5,37,18],[59,37,18]])
x=np.arange(-145,161,.5);y=np.arange(-260,241,.5);X,Y=np.meshgrid(x,y)
Z=20+.068*Y+7*np.exp(-((X+78)/48)**2-((Y-110)/57)**2)+5*np.exp(-((X+2)/28)**2-((Y-65)/31)**2)+14*np.exp(-((X-135)/50)**2-((Y-50)/150)**2)+1.0*np.sin(X/18)*np.sin(Y/27)+.20*np.maximum(Y-105,0)/(1+np.exp(-(X-40)/20))
def distances(points):return np.r_[0,np.cumsum(np.linalg.norm(np.diff(points[:,:2],axis=0),axis=1))]
def nearest(points):
    d=np.full(X.shape,1e9);e=np.zeros(X.shape)
    for a,b in zip(points[:-1],points[1:]):
        v=b[:2]-a[:2];t=np.clip(((X-a[0])*v[0]+(Y-a[1])*v[1])/np.dot(v,v),0,1)
        dd=np.hypot(X-(a[0]+t*v[0]),Y-(a[1]+t*v[1]));mask=dd<d
        d[mask]=dd[mask];e[mask]=(a[2]+t*(b[2]-a[2]))[mask]
    return d,e
def grade(points,width):
    global Z
    d,e=nearest(points);t=np.clip((d-width/2)/2.5,0,1);w=(1+np.cos(np.pi*t))/2;Z=Z*(1-w)+e*w
for pts,w in [(forest,2.4),(arrival,3.5),(churchpath,2.2),(millpath,3),(southpath,3.5),(shrinepath,2.0)]:grade(pts,w)
for pts in localpaths:grade(pts,2.0)
grade(bridgepath[:4],2.0)
grade(bridgepath[4:],2.0)
# Incised channel and banks: river target is continuous, not pasted over terrain.
rx=np.interp(Y,river[:,1],river[:,0]);rw=np.interp(Y,river[:,1],river[:,2]);rd=np.abs(X-rx)
half=3.0+np.clip(-Y/150,0,2)
channel=rw-1.1+np.minimum(rd/half,1)*.25
bank=rw+1.8+(rd-half-2)*.55
carve=np.where(rd<half,channel,np.maximum(rw+1.8,bank))
t=np.clip((rd-half-2)/11,0,1);w=(1+np.cos(np.pi*t))/2
Z=Z*(1-w)+carve*w
# Engineered mill race; channel bed is below its explicit water profile.
d,e=nearest(race);t=np.clip((d-1.15)/2.0,0,1);w=(1+np.cos(np.pi*t))/2
Z=Z*(1-w)+(e-.65)*w
# Building platforms and square. Cut/fill shoulders are visible terrain features.
for _,_,bx,by,bw,bh,bz,*_ in B:
    d=np.maximum(np.maximum(np.abs(X-bx)-bw/2-.6,np.abs(Y-by)-bh/2-.6),0)
    t=np.clip(d/2.5,0,1);w=(1+np.cos(np.pi*t))/2;Z=Z*(1-w)+bz*w
r=np.hypot(X,Y);t=np.clip((r-3)/2,0,1);w=(1+np.cos(np.pi*t))/2;Z=Z*(1-w)+20*w
# Reconcile walkable route surfaces after terrace and water cuts.
# The bridge span is deliberately excluded; it sits above the race.
for pts,wid in [(forest,2.4),(arrival,3.5),(churchpath,2.2),(millpath,3),(southpath,3.5),(shrinepath,2)]:grade(pts,wid)
for pts in localpaths:grade(pts,2.0)
grade(bridgepath[:4],2.0);grade(bridgepath[4:],2.0)
# One nearest-route solve avoids overlapping sequential cuts at junctions.
roads=[(forest,2.4),(arrival,3.5),(churchpath,2.2),(millpath,3),(southpath,3.5),(shrinepath,2)]+[(p,2) for p in localpaths]+[(bridgepath[:4],2),(bridgepath[4:],2)]
D=np.full_like(Z,1e9);E=np.zeros_like(Z);W=np.zeros_like(Z)
for pts,wid in roads:
    dd,ee=nearest(pts);sel=dd<D;D[sel]=dd[sel];E[sel]=ee[sel];W[sel]=wid
t=np.clip((D-W/2)/2.5,0,1);blend=(1+np.cos(np.pi*t))/2;Z=Z*(1-blend)+E*blend
# Last operation reserves the open race beside the mill retaining edge.
# Footbridge span is a deck, not terrain, and remains over this cut.
d,e=nearest(race);t=np.clip((d-1.15)/1.0,0,1);w=(1+np.cos(np.pi*t))/2;Z=Z*(1-w)+(e-.65)*w
terrain=RegularGridInterpolator((y,x),Z,bounds_error=False)
np.savez_compressed(P+'/proposed_terrain_heightfield.npz',x=x,y=y,height=Z,units='metres',datum='proposed Abbey lake = 0')
meta={'status':'PROPOSED BLOCKOUT, NOT ENGINE-VALIDATED','date':'2026-10-05','source_commit':'6aef4cde32188fd8a1ced2bef30360451f80aaae','axes':{'x':'east metres','y':'north metres','z':'up metres above proposed lake datum'},'origin':'Bellwether well, X=0 Y=0 Z=20','forest':forest.tolist(),'arrival':arrival.tolist(),'river_surface':river.tolist(),'millrace_surface':race.tolist(),'local_paths':[p.tolist() for p in localpaths],'church_path':churchpath.tolist(),'shrine_path':shrinepath.tolist(),'mill_path':millpath.tolist(),'south_path':southpath.tolist(),'optional_bridge_path':bridgepath.tolist(),'buildings':[dict(zip(['id','label','x','y','width','depth','floor_z','door_x','door_y','scope'],a)) for a in B],'path_widths_m':{'forest':2.4,'arrival':3.5,'side_path':2.2},'warning':'Concept map topology is approved but not metric. All dimensions and elevations here are proposals. No full-world dimensions are fixed. Terrain is a deterministic 0.5 m planning grid, not a shipping mesh.'}
meta['landmarks']={'F01':{'label':'Rescue rim','xyz':forest[0].tolist()},'F02':{'label':'Reunion','xyz':forest[1].tolist()},'F03':{'label':'Optional lantern clue','xyz':forest[4].tolist()},'F04':{'label':'Lookout and shared join','xyz':forest[-1].tolist()},'T01':{'label':'Well','xyz':[0,0,20]},'T02':{'label':'Gardens','xy':[-43,-10]},'T03':{'label':'Active graveyard','bounds':[11,31,54,78]},'T04':{'label':'Optional service bridge','deck_bounds':[45.5,52.5,35.5,38.5],'deck_z':18,'underside_z':17.4},'T05':{'label':'Outbound junction','xyz':southpath[-1].tolist()}}
meta['services']=[{'id':i,'role':r,'xy':[a,b]} for i,r,a,b in [('S1','Kit',-8,-1),('S2','Tavern keeper',3,7),('S3','Record/shrine service',-7,51),('S4','Smith/artificer',19,-23),('S5','River salvager',36,0),('S6','Relic dealer',-18,-3)]]
json.dump(meta,open(P+'/shared-coordinate-layout.json','w'),indent=2)
with open(P+'/building-feature-register.csv','w') as f:
    writer=csv.writer(f);writer.writerow(['ID','Feature','East_m','North_m','Width_m','Depth_m','Floor_m','Door_east_m','Door_north_m','Scope']);writer.writerows(B)
forestS=distances(forest);arrivalS=distances(arrival)
print('FOREST',forestS[-1],'ARRIVAL',arrivalS[-1]);print('FOREST grades',np.diff(forest[:,2])/np.diff(forestS));print('ARRIVAL grades',np.diff(arrival[:,2])/np.diff(arrivalS))

def base(title,subtitle,index):
    fig=plt.figure(figsize=(16.54,11.69),facecolor=BG)
    fig.text(.05,.951,'THE HORDE  /  THE VEYRLANDS',fontsize=11,weight='bold',color=GREEN)
    fig.text(.05,.912,title,fontsize=27,weight='bold',color=INK)
    fig.text(.05,.883,subtitle,fontsize=11,color='#5b6b5e')
    fig.text(.955,.95,index,fontsize=13,ha='right',color=GOLD,weight='bold')
    fig.text(.05,.018,'PROPOSED METRIC BLOCKOUT  •  05 OCT 2026  •  Topology from approved world map; dimensions and elevations require playable review.',fontsize=8.5,color='#626e64')
    fig.text(.955,.018,'No runtime / visibility / performance validation',ha='right',fontsize=8,color='#626e64')
    return fig

def mapax(fig,rect,limits,grid,terrain_on=True):
    ax=fig.add_axes(rect,facecolor='#e0e4d4');ax.set_xlim(limits[:2]);ax.set_ylim(limits[2:]);ax.set_aspect('equal');
    if terrain_on:
        cmap=LinearSegmentedColormap.from_list('land',['#d2dcc4','#cad5ba','#bfccab','#a4b895','#88a184','#708c79','#5a796c'])
        ax.imshow(Z,extent=[x[0],x[-1],y[0],y[-1]],origin='lower',cmap=cmap,vmin=5,vmax=45,alpha=.88,zorder=0)
        ls=LightSource(315,40).hillshade(Z,vert_exag=1,dx=.5,dy=.5);ax.imshow(ls,extent=[x[0],x[-1],y[0],y[-1]],origin='lower',cmap='gray',alpha=.16,zorder=1)
        co=ax.contour(X,Y,Z,levels=np.arange(-10,65,2),colors='#4f6950',linewidths=.45,alpha=.46,zorder=2)
        ax.clabel(co,levels=np.arange(0,65,4),fontsize=6,inline=True,fmt='%d m')
    ax.set_xticks(np.arange(math.ceil(limits[0]/grid)*grid,limits[1]+1,grid));ax.set_yticks(np.arange(math.ceil(limits[2]/grid)*grid,limits[3]+1,grid));ax.grid(color='#536c58',alpha=.18,lw=.6)
    ax.tick_params(labelsize=8);ax.set_xlabel('EAST  X (m)',fontsize=8);ax.set_ylabel('NORTH  Y (m)',fontsize=8)
    ax.annotate('N',xy=(.945,.955),xytext=(.945,.875),xycoords='axes fraction',ha='center',fontsize=12,weight='bold',arrowprops=dict(arrowstyle='-|>',color=INK,lw=1.8),zorder=30)
    return ax

def line(ax,pts,width=2.4,color=GOLD,style='-',alpha=1):
    if style=='-':
        for a,b in zip(pts[:-1],pts[1:]):
            v=b[:2]-a[:2];n=np.array([-v[1],v[0]])/np.linalg.norm(v)*width/2
            ax.add_patch(Polygon([a[:2]+n,b[:2]+n,b[:2]-n,a[:2]-n],color=color,alpha=alpha,zorder=7,lw=0))
        for p in pts:ax.add_patch(Circle(p[:2],width/2,color=color,alpha=alpha,zorder=7,lw=0))
    else:ax.plot(pts[:,0],pts[:,1],style,color=color,lw=2,alpha=alpha,zorder=8)

def water(ax):
    ys=np.linspace(-260,240,600);xs=np.interp(ys,river[:,1],river[:,0]);h=3+np.clip(-ys/150,0,2)
    ax.fill_betweenx(ys,xs-h-2,xs+h+2,color='#8db0b7',alpha=.35,zorder=3)
    ax.plot(xs-h-2,ys,color=BLUE,ls=':',lw=.8,zorder=4);ax.plot(xs+h+2,ys,color=BLUE,ls=':',lw=.8,zorder=4)
    ax.fill_betweenx(ys,xs-h,xs+h,color=BLUE,alpha=.92,zorder=5)
    for yy in [-50,0,75,110]:
        xx=np.interp(yy,river[:,1],river[:,0]);ax.annotate('',xy=(xx,yy-12),xytext=(xx,yy),arrowprops=dict(arrowstyle='-|>',color='#e3f1ec',lw=1.3),zorder=10)
    line(ax,race,2.3,BLUE)

def buildings(ax,labels=True):
    for bid,name,bx,by,bw,bh,bz,dx,dy,scope in B:
        ax.add_patch(Rectangle((bx-bw/2-1,by-bh/2-1),bw+2,bh+2,fill=False,ec='#6e6954',lw=.7,zorder=8))
        ax.add_patch(Rectangle((bx-bw/2,by-bh/2),bw,bh,fc='#dcc9a6',ec='#574f3e',lw=1.2,zorder=9))
        ax.plot([bx-bw/2+1,bx+bw/2-1],[by,by],color='#9e8967',lw=.8,zorder=10)
        ax.plot(dx,dy,'s',ms=4,color='#fff3d0',mec='#574f3e',zorder=12)
        if labels:ax.text(bx,by,bid,ha='center',va='center',fontsize=8,weight='bold',zorder=13,bbox=dict(fc=BG,ec='none',alpha=.84,pad=1.5))
    ax.add_patch(Circle((0,0),2.1,fc='#d8c6a4',ec='#574f3e',lw=1.3,zorder=13))
    ax.add_patch(Rectangle((-55,-17),23,13,fc='#a5b377',ec=GREEN,lw=1,zorder=8))
    for gx in range(-53,-32,4):ax.plot([gx,gx],[-16,-5],color=GREEN,lw=.7,zorder=9)
    ax.add_patch(Rectangle((11,54),20,24,fc='#d1d0b3',ec='#6b745f',lw=.8,zorder=8))
    for xx in [14,20,26]:
        for yy in [58,65,72]:ax.plot(xx,yy,marker='+',color='#64735d',ms=4,zorder=9)
    ax.add_patch(Rectangle((8,-48),15,11,fill=False,ec='#8d7b59',lw=1,zorder=8))

def scale(ax,start,length,step,label=True):
    xx,yy=start
    for j in range(int(length/step)):ax.add_patch(Rectangle((xx+j*step,yy),step,1.0,fc=INK if j%2==0 else BG,ec=INK,lw=.8,zorder=30))
    for v in [0,length/2,length]:ax.text(xx+v,yy-2,str(int(v)),ha='center',va='top',fontsize=7,zorder=30)
    if label:ax.text(xx+length+2,yy,'m',fontsize=7,zorder=30)

def label(ax,xy,text,offset=(5,5),fs=8,color=INK):
    ax.annotate(text,xy=xy,xytext=offset,textcoords='offset points',fontsize=fs,color=color,zorder=25,bbox=dict(boxstyle='round,pad=.22',fc=BG,ec='none',alpha=.93),arrowprops=dict(arrowstyle='-',lw=.6,color=color))
def block(fig,x0,y0,title,lines,fs=10,space=.024):
    fig.text(x0,y0,title,fontsize=12,weight='bold',color=INK);y0-=.026
    for ln in lines:
        fig.text(x0,y0,ln,fontsize=fs,color='#405044');y0-=space
    return y0

def overview(fig,rect):
    ax=fig.add_axes(rect,facecolor='#e0e4d4');ax.set_xlim(-125,105);ax.set_ylim(-80,150);ax.set_aspect('equal');ax.set_xticks([]);ax.set_yticks([])
    water(ax);line(ax,forest,4,GREEN);line(ax,arrival,4,GOLD);line(ax,southpath,3,GOLD)
    for bb in B:ax.add_patch(Rectangle((bb[2]-bb[4]/2,bb[3]-bb[5]/2),bb[4],bb[5],fc='#8c8066',ec='none',zorder=10))
    ax.plot(-65,90,'o',ms=5,color=GOLD,zorder=20);ax.text(-60,95,'F04 / join',fontsize=7,zorder=25)
    ax.text(-121,139,'Tomb',fontsize=7,zorder=25);ax.text(-35,-8,'Bellwether',fontsize=8,zorder=25,weight='bold')
    ax.text(91,131,'NE uplands',fontsize=7,ha='right');ax.text(92,-73,'S: Abbey lake',fontsize=7,ha='right')
    ax.set_title('SAME GRID / FIXED CONNECTION',fontsize=9,loc='left',pad=5)
    return ax

def profile(fig,rect,pts,title,color=GOLD):
    ax=fig.add_axes(rect,facecolor=BG);s=distances(pts)
    ax.fill_between(s,pts[:,2],min(pts[:,2])-1,color=color,alpha=.12);ax.plot(s,pts[:,2],color=color,lw=2);ax.scatter(s,pts[:,2],s=15,color=color,zorder=4)
    ax.grid(alpha=.2);ax.tick_params(labelsize=7);ax.set_xlim(0,s[-1]);ax.set_ylim(min(pts[:,2])-1,max(pts[:,2])+1)
    ax.set_ylabel('Z (m)',fontsize=7);ax.set_xlabel('Distance along route (m)',fontsize=7,labelpad=1);ax.set_title(title,fontsize=9,loc='left',pad=4)
    return ax

def save(fig,name,pdf=None):
    fig.savefig(P+'/'+name+'.png',dpi=160,facecolor=BG);fig.savefig(P+'/'+name+'.svg',facecolor=BG)
    if pdf:pdf.savefig(fig,facecolor=BG)
    else:fig.savefig(P+'/'+name+'.pdf',facecolor=BG)

# SHEET 1.7
fig=base('1.7  /  Moonlit forest track','Measured proposal: rescue, reunion, short woodland route and a fixed lookout over future Bellwether.','01 / FOREST')
ax=mapax(fig,[.05,.23,.57,.62],[-126,-47,79,153],10)
# Woodland symbols do not imply actual asset placement or counts.
rng=np.random.default_rng(12)
for tx,ty in zip(rng.uniform(-127,-46,145),rng.uniform(78,154,145)):
    pt=np.array([tx,ty]);dist=1e9
    for a,b in zip(forest[:-1],forest[1:]):
        v=b[:2]-a[:2];t=np.clip(np.dot(pt-a[:2],v)/np.dot(v,v),0,1);dist=min(dist,np.linalg.norm(pt-a[:2]-t*v))
    if dist>5:ax.add_patch(Circle((tx,ty),rng.uniform(1.1,2.0),fc='#466951',ec='#385942',alpha=.17,lw=.5,zorder=3))
ax.text(-124,151,'Tree circles: illustrative massing; exact placement TBD',fontsize=7.5,zorder=26,bbox=dict(fc=BG,ec='none',alpha=.92,pad=3))
line(ax,forest,2.4,GOLD);line(ax,arrival,3.5,GOLD,'--',.85)
for p,rad in [(forest[1],3.2),(forest[-1],3.5)]:ax.add_patch(Circle(p[:2],rad,fc='#c8b684',ec=GOLD,lw=1,zorder=8))
ax.add_patch(Rectangle((-115,138),6,4,fc='#aea994',ec=INK,lw=1,zorder=9));ax.add_patch(Circle((-112,140),1.2,fc='#4b5147',zorder=10))
for xy,txt,off in [((-112,139),'F01  Rescue rim\nZ 33.6 m',(-30,20)),((-107,132),'F02  Reunion\nZ 33.0 m',(15,0)),((-96,111),'Shallow trail dip\nZ 31.0 m',(-93,-7)),((-82,106),'F03  Lantern clue\nZ 31.3 m',(16,14)),((-65,90),'F04  Lookout / join\n(-65, 90, 30.4)',(-112,-25))]:label(ax,xy,txt,off,9)
ax.plot([-69,-61],[94,86],color=ORANGE,lw=2.0,ls=(0,(3,2)),zorder=15)
label(ax,(-53,79),'1.8 continues on\nthis same road',(-3,29),8)
label(ax,(-117,120),'Woodland / root banks\nselective off-trail pockets',(-5,0),8)
scale(ax,(-123,83),20,5)
block(fig,.655,.833,'A SHORT CHAPTER, NOT A BIGGER FOREST',[
'76.3 m proposed trail; 2.4 m clear walking width.',
'F02: 6.4 m reunion pocket. F04: 7 m lookout.',
'Undulation: 33.6 → 31.0 → 31.3 → 30.4 m.',
'Centreline grades: max 7.3%; test walk + run.',
'2 m contours derive from the shared terrain grid.',
'No forest bridge, new fight or forced camera.'],fs=10,space=.026)
block(fig,.655,.627,'STAGING / STORY CONTRACT',[
'F01  Separate finale opening and fixed rope anchor.',
'       Tomb interior alignment is still to be surveyed.',
'F02  Safe grounded reunion; fresh lantern raise.',
'       Kit reacts only once the lantern is presented.',
'F03  Optional inscription pause; content remains draft.',
'F04  Kit waits; player retains control and backtracking.',
'       Honest 1.7 endpoint, no oversized road wall.'],fs=9.5,space=.024)
overview(fig,[.675,.165,.24,.23])
fig.text(.655,.125,'1.7: town = low-detail exterior shell only.\n1.8 activates the same footprint; no relocation.\nView corridor toward church/tavern is a target, not a verified sightline.',fontsize=9,color=INK,linespacing=1.45)
profile(fig,[.085,.08,.50,.09],forest,'FOREST LONGITUDINAL PROFILE  /  actual proposed centreline',GREEN)
fig.text(.655,.413,'Gold: path  /  Dashes: future road  /  Rust: chapter limit',fontsize=8.5)
save(fig,'01-forest-track-1-7');plt.close(fig)

# TOWN PHYSICAL / SERVICE VIEWS
with PdfPages(P+'/02-bellwether-town-1-8.pdf') as pdf:
    fig=base('1.8  /  Bellwether terrain + footprints','An unwalled village on stepped western banks. The arrival road and northern church rise out of the river ravine.','02 / TERRAIN')
    ax=mapax(fig,[.05,.21,.56,.64],[-85,92,-78,116],25);water(ax)
    for pts,w in [(arrival,3.5),(churchpath,2.2),(millpath,3),(southpath,3.5),(shrinepath,2)]:line(ax,pts,w,GOLD)
    
    for pts in localpaths:line(ax,pts,2,GOLD)
    line(ax,bridgepath,2,'#998969','--')
    buildings(ax)
    ax.add_patch(Rectangle((45.5,35.5),7,3,fc='#bfab83',ec='#514d40',lw=1,zorder=14))
    ax.plot(-65,90,'o',color=ORANGE,ms=5,zorder=20);label(ax,(-65,90),'F04  same lookout\nZ 30.4 m',(10,9),8)
    label(ax,(0,0),'Well / square\nZ 20.0 m',(-73,-17),8)
    label(ax,(-44,-10),'Gardens',(-39,-10),8);label(ax,(21,76),'Active graveyard',(-6,15),8)
    label(ax,(49,37),'Optional service\nbridge, deck Z18',(30,5),7.5)
    label(ax,(62,-3),'Tailrace returns\nto main river',(22,0),7.5)
    label(ax,(77,98),'NE inflow\nZ20 at Y105',(0,-34),8)
    label(ax,(60,-57),'Downstream to\nsouthern lake',(16,0),8)
    scale(ax,(-78,-67),40,10)
    block(fig,.64,.835,'BUILDING REGISTER  /  X, Y; PAD Z',[
'B01  Tavern                 (12, 15); 20.4 m',
'B02  Church                (-2, 65); 29.0 m',
'B03  House A              (-43, 8); 20.8 m',
'B04  House B             (-47, 29); 24.6 m',
'B05  Mill                      (48, 13); 16.0 m',
'B06  Smith                 (29, -29); 18.2 m',
'B07  Store                 (-22, -28); 19.9 m',
'B08  Outbuilding         (1, -44); 18.2 m',
'B09  Starter shrine      (25, 101); 31.2 m'],fs=10,space=.024)
    block(fig,.64,.575,'FOUNDED ON TERRAIN',[
'Building rectangles = true proposed footprints.',
'Pale squares mark exterior door positions.',
'Level pads use short cut/fill and retaining shoulders;',
'between them the land continues to rise and fall.',
'Church: +9 m above well. Mill: -4 m below well.',
'Arrival: 118.6 m long, 3.5 m wide; max 9.7% target.',
'Contours: 2 m. Side paths: 2–2.2 m clear.'],fs=9.5,space=.024)
    block(fig,.64,.359,'RESERVE THE WATER GEOMETRY NOW',[
'Main channel: 6–8 m wet width in this local area.',
'Dotted wet margin = +1.8 m design high-water target,',
'not a calculated flood map. Banks reach dry terraces.',
'Mill headrace: 2.3 m; 13.0 → 12.1 m headwater.',
'Wheel drop: 2.6 m; tailrace reaches river at 7.76 m.',
'Optional 7 × 3 m service bridge: deck 18 m,',
'underside 17.4 m; high-water clearance ≈3.0 m.',
'Bridge/race are working-layout proposals, not quests.'],fs=9.3,space=.023)
    profile(fig,[.085,.078,.50,.09],arrival,'SAME ARRIVAL ROAD  /  F04 → square (no flat-plane shortcut)')
    fig.text(.645,.075,'Later river rendering: authored bed/surface, bank contact,\nshallow edges and bounded cascades. No fluid-simulation promise.\nWater must follow these heights; materials cannot fix bad geometry.',fontsize=9,linespacing=1.45)
    save(fig,'02a-bellwether-terrain-1-8',pdf);plt.close(fig)
    fig=base('1.8  /  Services, legibility + future routes','A compact return hub. Landmarks and inhabited thresholds guide the player; no compass waypoint dependency.','03 / USE + VIEWS')
    ax=mapax(fig,[.05,.21,.56,.64],[-85,92,-78,116],25,False);water(ax)
    for pts,w in [(arrival,3.5),(churchpath,2.2),(millpath,3),(southpath,3.5),(shrinepath,2)]:line(ax,pts,w,GOLD)
    for pts in localpaths:line(ax,pts,2,GOLD)
    line(ax,bridgepath,2,'#998969','--')
    ax.add_patch(Rectangle((45.5,35.5),7,3,fc='#bfab83',ec='#514d40',lw=1,zorder=14))
    buildings(ax,False)
    for bb in B:ax.text(bb[2],bb[3],bb[1].replace('Starter ','').replace('Outbuilding','Shed'),ha='center',va='center',fontsize=7.8,zorder=20,bbox=dict(fc=BG,ec='none',alpha=.8,pad=1))
    ax.plot([-65,-2],[90,65],ls='--',lw=1,color=ORANGE,zorder=12);ax.plot([-65,12],[90,15],ls='--',lw=1,color=ORANGE,zorder=12)
    label(ax,(-65,90),'Arrival frame\nchurch bell + warm roofs',(-5,17),8)
    services=[(-8,-1,'S1'),(3,7,'S2'),(-7,51,'S3'),(19,-23,'S4'),(36,0,'S5'),(-18,-3,'S6')]
    for sx,sy,txt in services:
        ax.add_patch(Circle((sx,sy),3.2,fc=GREEN,ec=BG,lw=1,zorder=24));ax.text(sx,sy,txt,color='white',fontsize=7,ha='center',va='center',weight='bold',zorder=25)
    label(ax,(25,101),'Small first-expedition spur\nexact quest / access TBD',(-143,26),8)
    label(ax,(29,-45),'Pen / yard\nscenery proposal',(8,-13),7.5)
    label(ax,(47,-70),'Outbound junction\npeer branches later',(-128,13),8)
    ax.annotate('S: Drowned Abbey',xy=(47,-77),xytext=(10,-75),fontsize=8,color=BLUE,arrowprops=dict(arrowstyle='->',color=BLUE),zorder=30)
    ax.annotate('E: Foundry',xy=(90,-66),xytext=(59,-54),fontsize=8,color=ORANGE,arrowprops=dict(arrowstyle='->',color=ORANGE),zorder=30)
    ax.text(89,106,'NE: Court ridge\nTreasury below / behind',ha='right',fontsize=8,color=ORANGE,zorder=30,bbox=dict(fc=BG,ec='none',alpha=.9,pad=3))
    scale(ax,(-78,-67),40,10)
    block(fig,.64,.835,'SERVICE POINTS  /  ROLES, NOT NEW NAMES',[
'S1  Kit: square-side waiting / reunion point.',
'S2  Tavern keeper: story, information, return anchor.',
'S3  Record / shrine service: history and Magic.',
'S4  Smith / artificer: mechanisms, repairs and Tech.',
'S5  River salvager: breath / climbing / endurance.',
'S6  Relic dealer: valuables and authored object choices.',
'Roles may combine; target 4–7 cast, not all at once.',
'Markers are staging suggestions, not locked triggers.'],fs=9.5,space=.024)
    block(fig,.64,.595,'CONTENT ENVELOPE / READABILITY',[
'8 village masses + small separate shrine.',
'Tavern hero interior; House A / B optional candidates.',
'Choose 2–3 interiors total; other doors read as closed.',
'North church and west gardens stay unmistakable.',
'Light pools and roof gaps lead into the central well.',
'Kit waits off the road; service points keep doors clear.',
'Woodland banks frame arrival; do not hide the hub',
'behind arbitrary maze walls or oversized fences.'],fs=9.5,space=.024)
    block(fig,.64,.355,'LATER CONNECTIONS / HONEST LIMITS',[
'Abbey south and Foundry east are peer branches.',
'Foundry broken-bridge problem is farther east,',
'not this optional local millrace service bridge.',
'Court gate requires both seals; no ridge bypass.',
'Treasury stays dry beneath / behind the NE ridge.',
'Exact future route lengths and distant visibility TBD.',
'Dashed rust = desired arrival sightlines only;',
'Foundry / Court silhouettes require a real 3D test.'],fs=9.5,space=.023)
    # True terrain cross-section through the mill edge at Y=13.
    ax2=fig.add_axes([.085,.078,.50,.09],facecolor=BG)
    xx=np.linspace(25,90,600);zz=terrain(np.c_[np.full_like(xx,13),xx]);ax2.fill_between(xx,zz,0,color='#a7b38b',alpha=.55);ax2.plot(xx,zz,color=GREEN,lw=1.2)
    rx13=np.interp(13,river[:,1],river[:,0]);w13=np.interp(13,river[:,1],river[:,2]);ax2.plot([rx13-3,rx13+3],[w13,w13],color=BLUE,lw=2)
    ax2.plot([40,56],[16,16],color=ORANGE,lw=3);ax2.text(47,17,'mill pad 16 m',fontsize=7,ha='center');ax2.text(rx13,11,'river 9.04 m',fontsize=7,ha='center',color=BLUE)
    ax2.set_xlim(25,90);ax2.set_ylim(0,27);ax2.grid(alpha=.2);ax2.tick_params(labelsize=7);ax2.set_xlabel('East X (m), section at Y = 13 m',fontsize=7,labelpad=1);ax2.set_ylabel('Z (m)',fontsize=7);ax2.set_title('TRUE TERRAIN SECTION / western mill terrace → incised river → east bank',fontsize=9,loc='left',pad=4)
    fig.text(.645,.072,'Continuity test: enter from F04, find the tavern, return to Kit,\nreach every admitted service, inspect the river and later gates.\nThen test from player-eye height with lantern, moon and foliage.',fontsize=9,linespacing=1.45)
    save(fig,'02b-bellwether-use-1-8',pdf);plt.close(fig)
    fig=base('Terrain continuity  /  River + mill section','One shared height datum ties the upland inflow, western terraces, incised channel and future southern lake.','04 / HEIGHTS')
    ax=fig.add_axes([.075,.47,.52,.34],facecolor=BG)
    rp=river[::-1];rs=distances(rp)
    ax.fill_between(rs,rp[:,2],0,color=BLUE,alpha=.10);ax.plot(rs,rp[:,2],color=BLUE,lw=3);ax.scatter(rs,rp[:,2],color=BLUE,s=25)
    ax.set_xlim(0,rs[-1]);ax.set_ylim(-2,44);ax.grid(alpha=.2);ax.set_xlabel('Distance along proposed river centreline (m)');ax.set_ylabel('Water surface Z (m above lake)');ax.set_title('NE UPLAND → SOUTHERN LAKE / strictly downhill',fontsize=12,loc='left')
    for i,txt,ofs in [(0,'Upland inflow\n40 m',(8,-15)),(2,'Y105: 20 m',(-20,15)),(4,'Y25: 10 m',(-20,15)),(6,'Y-80: 3 m',(-25,14)),(9,'Lake interface\n0 m',(-65,16))]:
        ax.annotate(txt,(rs[i],rp[i,2]),xytext=ofs,textcoords='offset points',fontsize=8,color=INK)
    ax=fig.add_axes([.68,.47,.265,.34],facecolor=BG);ss=distances(race)
    ax.plot(ss,race[:,2],color=BLUE,lw=2);ax.scatter(ss,race[:,2],s=20,color=BLUE);ax.axhline(16,color=ORANGE,lw=1.5,ls='--');ax.text(2,16.15,'Mill floor 16 m',fontsize=8,color=ORANGE)
    ax.set_xlim(0,ss[-1]);ax.set_ylim(6,18);ax.grid(alpha=.2);ax.set_xlabel('Millrace distance (m)');ax.set_ylabel('Z (m)');ax.set_title('MILL HEAD / DROP / RETURN',fontsize=11,loc='left')
    ax.annotate('Wheel: 12.1 → 9.5 m',(ss[4],12.1),xytext=(4,14.4),fontsize=8,arrowprops=dict(arrowstyle='-',color=INK))
    ax.annotate('Return: 7.76 m',(ss[-1],race[-1,2]),xytext=(15,8.5),fontsize=8,arrowprops=dict(arrowstyle='-',color=INK))
    block(fig,.075,.365,'VERTICAL CONTRACT / PROPOSED',[
'Well Z20.0  |  lookout Z30.4  |  church pad Z29.0  |  mill pad Z16.0.',
'River runs NE → S. No uphill water segments or level river painted over a slope.',
'Headwater intakes, wheel drop and tailrace are modelled as different heights.',
'River high-water target: +1.8 m; millrace working freeboard: +1.8 m.',
'Mill floor is 3.9 m above wheel headwater; +2.1 m above its high-water target.',
'Local bridge: underside Z17.4; race near crossing ≈12.56 m;',
'clearance above proposed high water ≈3.0 m. Abutments still need blockout.'],fs=10,space=.026)
    block(fig,.68,.365,'LIMITS OF THIS REFERENCE',[
'All numbers are authoring proposals.',
'Upstream / downstream reach lengths are',
'working context, not a full-world scale lock.',
'Lake basin outline, outflow and Abbey floors',
'are not designed by this sheet.',
'No flood simulation or bridge engineering claim.',
'No river crossing required for town arrival.'],fs=10,space=.026)
    fig.text(.075,.10,'NEXT PROOF: rebuild these curves as terrain and collision, smooth junctions, retain dry pads, then walk every route with Kit.\nInspect the fixed lookout view in real 3D before detailed forest art or activating the hub. Verify phone/Windows RT cost separately.',fontsize=11,linespacing=1.7,color=INK)
    save(fig,'02c-bellwether-water-height-1-8',pdf);plt.close(fig)
print('DONE')
