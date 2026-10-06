import os,json,csv,math,textwrap,hashlib
os.environ['MPLCONFIGDIR']='/tmp/dungeon-atlas-mpl'
import numpy as np
import matplotlib;matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle,Polygon,Circle,FancyArrowPatch
from matplotlib.colors import LinearSegmentedColormap,LightSource
from matplotlib.backends.backend_pdf import PdfPages
from scipy.interpolate import RegularGridInterpolator
P=os.path.dirname(__file__); ROOT=os.path.dirname(P)
BASE=P+'/baseline' if os.path.isdir(P+'/baseline') else ROOT+'/connected-reference-maps'
B=json.load(open(BASE+'/shared-coordinate-layout.json'))
BG='#f4f1e8';INK='#233d34';GREEN='#40694e';GOLD='#b78533';BLUE='#477f96';ORANGE='#aa6341';PURPLE='#776581';PALE='#ded6bf'
plt.rcParams.update({'font.family':'DejaVu Sans','font.size':10,'svg.fonttype':'none','pdf.fonttype':42,'axes.edgecolor':'#788174','text.color':INK,'axes.labelcolor':INK,'xtick.color':'#58655b','ytick.color':'#58655b'})
figs=[]; names=[]
def page(title,sub,n):
 f=plt.figure(figsize=(16.54,11.69),facecolor=BG)
 f.text(.05,.958,'THE HORDE  /  THE VEYRLANDS',fontsize=11,color=GREEN,weight='bold')
 f.text(.05,.917,title,fontsize=24,weight='bold')
 f.text(.05,.883,sub,fontsize=10.5)
 f.text(.95,.955,f'{n:02d} / DUNGEON ATLAS',ha='right',fontsize=11,color=GOLD,weight='bold')
 f.text(.05,.027,'R2 / DRAFT METRIC BLOCKOUT  •  06 OCT 2026  •  Approved topology; new dimensions, rooms and encounters proposed.',fontsize=8,color='#737c70')
 f.text(.95,.027,'Approved items / boss use; detailed blockout still draft',ha='right',fontsize=8,color='#737c70')
 return f

def textblock(f,x,y,title,body,width=49,size=10,leading=.019):
 f.text(x,y,title,fontsize=12,weight='bold');y-=.03
 for para in body:
  for line in textwrap.wrap(para,width=width,break_long_words=False):
   f.text(x,y,line,fontsize=size);y-=leading
  y-=.009
 return y

def save(f,name):
 f.savefig(P+'/'+name+'.png',dpi=150,facecolor=BG);f.savefig(P+'/'+name+'.svg',facecolor=BG)
 figs.append(f);names.append(name)

def baseax(f,rect,lims):
 a=f.add_axes(rect,facecolor=BG);a.set_xlim(*lims[0]);a.set_ylim(*lims[1]);a.set_aspect('equal');a.set_xlabel('EAST X (m)',fontsize=8);a.set_ylabel('NORTH Y (m)',fontsize=8);a.tick_params(labelsize=7);a.grid(alpha=.1);return a

def north(a,x,y,L=24):
 a.annotate('N',xy=(x,y+L),xytext=(x,y),ha='center',weight='bold',fontsize=10,arrowprops={'arrowstyle':'-|>','color':INK,'lw':1.5})
def scale(a,x,y,L):
 a.plot([x,x+L/2],[y,y],color=INK,lw=4);a.plot([x+L/2,x+L],[y,y],color='#ffffff',lw=4);a.plot([x,x+L],[y,y],color=INK,lw=.6)
 a.text(x,y-L*.07,'0',fontsize=7,va='top');a.text(x+L,y-L*.07,f'{L:g} m',fontsize=7,va='top',ha='right')
def label(a,xy,txt,off=(6,6),color=INK):
 a.annotate(txt,xy=xy,xytext=off,textcoords='offset points',fontsize=8,color=color,ha='left',va='bottom',bbox={'boxstyle':'round,pad=.18','facecolor':BG,'edgecolor':'none','alpha':.92},arrowprops={'arrowstyle':'-','color':color,'lw':.6},zorder=20)
def route(a,pts,color=GOLD,lw=3,style='-'):
 p=np.array(pts);a.plot(p[:,0],p[:,1],color=BG,lw=lw+1.5);a.plot(p[:,0],p[:,1],color=color,lw=lw,ls=style,zorder=6)
def dist(p):return np.r_[0,np.cumsum(np.linalg.norm(np.diff(np.array(p)[:,:2],axis=0),axis=1))]
def profile(a,p,color=GOLD,label_=None):
 p=np.array(p);d=dist(p);a.plot(d,p[:,2],color=color,lw=2,label=label_);a.scatter(d,p[:,2],s=10,color=color);a.fill_between(d,p[:,2],np.min(p[:,2])-2,color=color,alpha=.1);a.grid(alpha=.18);a.set_xlabel('Route distance (m)',fontsize=8);a.set_ylabel('Z (m)',fontsize=8);a.tick_params(labelsize=7)

# Town baseline is copied verbatim, including every authored footprint and water station.
A=np.array([[47,-70,14.9],[31,-100,13],[15,-140,10.5],[-2,-190,7.5],[-18,-224,5.3],[-20,-248,3.0],[0,-265,1.5]])
F=np.array([[47,-70,14.9],[43,-84,14],[51,-94,13.5],[89,-94,13.5],[119,-76,17],[148,-72,20],[179,-55,24],[213,-47,28],[230,-40,30]])
C=np.array([[119,-76,17],[133,-36,21],[146,8,26],[169,51,31],[193,76,36],[172,104,40],[201,133,45],[230,153,49],[209,183,53],[236,211,59],[272,227,64],[257,258,69],[282,283,75],[306,308,80]])
W=np.array([[0,-265,1.5],[6,-269,0],[11,-272,-2.2],[16,-275,1.2],[21,-278,-2.2],[27,-281,0],[30,-284,2]])
# Two recoverable water legs; W2 is an open-to-air tower ledge above lake.
RIVER=np.array(B['river_surface']); rex=np.array([[135,220,40],[170,290,51],[193,360,65],[214,430,79]])
OUT=np.array([[0,-448,0],[-28,-478,-.4],[-55,-510,-1.2]])
lake=np.array([[30,-240],[68,-246],[104,-262],[148,-300],[166,-349],[148,-400],[103,-444],[50,-459],[0,-448],[-38,-415],[-61,-365],[-59,-310],[-30,-274],[-8,-285],[2,-267],[-5,-252]])
x=np.arange(-150,411,1.);y=np.arange(-510,441,1.);X,Y=np.meshgrid(x,y)
Z=20+.064*Y+45*np.exp(-((X-310)/135)**2-((Y-290)/220)**2)+22*np.exp(-((X-245)/130)**2-((Y+25)/165)**2)+5*np.sin(X/57)*np.cos(Y/76)
# Retain supplied terrain in its full original range, blend an external collar.
bas=np.load(BASE+'/proposed_terrain_heightfield.npz');interp=RegularGridInterpolator((bas['y'],bas['x']),bas['height'],bounds_error=False,fill_value=None)
inside=(X>=bas['x'][0])&(X<=bas['x'][-1])&(Y>=bas['y'][0])&(Y<=bas['y'][-1]);bw=np.clip(np.minimum.reduce([X+145,160-X,Y+260,240-Y])/60,0,1); Z[inside]=Z[inside]*(1-bw[inside])+interp(np.c_[Y[inside],X[inside]])*bw[inside]
def nearest(pts):
 d=np.full(X.shape,1e9);e=np.zeros_like(X)
 for a,b in zip(pts[:-1],pts[1:]):
  v=b[:2]-a[:2];t=np.clip(((X-a[0])*v[0]+(Y-a[1])*v[1])/np.dot(v,v),0,1);dd=np.hypot(X-a[0]-t*v[0],Y-a[1]-t*v[1]);m=dd<d;d[m]=dd[m];e[m]=(a[2]+t*(b[2]-a[2]))[m]
 return d,e
# Smooth only extension terrain seam outside preserved baseline.
from scipy.ndimage import gaussian_filter
zs=gaussian_filter(Z,10);edge=np.minimum.reduce([np.abs(X-bas['x'][0]),np.abs(X-bas['x'][-1]),np.abs(Y-bas['y'][0]),np.abs(Y-bas['y'][-1])]);m=(~inside)&(edge<25);blend=np.clip(edge/25,0,1);Z[m]=zs[m]*(1-blend[m])+Z[m]*blend[m]
# Lake bed and containment: shape is a proposed bounded basin, not ocean.
from matplotlib.path import Path
wet=Path(lake).contains_points(np.c_[X.ravel(),Y.ravel()]).reshape(X.shape)
Z[wet]=-7+1.3*np.cos(X[wet]/50)*np.cos(Y[wet]/60)
# Exterior outflow and headwaters descend monotonically; existing river remains untouched.
for rr in [RIVER,rex,OUT]:
 d,e=nearest(rr);w=np.clip((13-d)/8,0,1);Z=Z*(1-w)+(e-1+np.maximum(d-3,0)*.6)*w
# New routes are cut/fill proposals; never alter town or the original river crossing bed.
for rr,width in [(A,3),(F[:3],3),(F[3:],3),(C,3)]:
 d,e=nearest(rr);w=np.clip((7-d)/5,0,1);Z=Z*(1-w)+e*w
# Explicit entrance pads, terrain shelves; interior floors handled separately.
for xx,yy,zz,ww,hh in [(230,-40,30,20,16),(306,308,80,18,14),(-20,-248,3,12,10)]:
 d=np.maximum(np.abs(X-xx)-ww/2,np.abs(Y-yy)-hh/2);w=np.clip((5-d)/5,0,1);Z=Z*(1-w)+zz*w
terrain=RegularGridInterpolator((y,x),Z,bounds_error=False)
cmap=LinearSegmentedColormap.from_list('land',['#d8deca','#a3b596','#819882','#b9b79b','#ded7c0'])
def terrainplot(a):
 rgb=LightSource(315,42).shade(Z,cmap=cmap,vert_exag=1,dx=1,dy=1,blend_mode='soft')
 a.imshow(rgb,origin='lower',extent=[x[0],x[-1],y[0],y[-1]],alpha=.86)
 cs=a.contour(X,Y,Z,levels=np.arange(0,101,5),colors=GREEN,alpha=.22,linewidths=.45);a.clabel(cs,levels=np.arange(0,101,20),fontsize=6,fmt='%d m')
 a.add_patch(Polygon(lake,facecolor='#82a8b1',edgecolor=BLUE,lw=1.2,zorder=3))
 for rr in [RIVER,rex,OUT]:a.plot(rr[:,0],rr[:,1],color=BLUE,lw=6,zorder=3)
 for q in [(70,110),(60,-100),(22,-230),(-18,-464)]:a.annotate('',xy=(q[0]-3,q[1]-28),xytext=q,arrowprops={'arrowstyle':'-|>','color':'#e1f1f3','lw':1.5},zorder=5)

def town(a):
 for key in ['forest','arrival','church_path','shrine_path','mill_path','south_path']:
  route(a,B[key],GREEN if key=='forest' else GOLD,1.8)
 for b in B['buildings']:
  a.add_patch(Rectangle((b['x']-b['width']/2,b['y']-b['depth']/2),b['width'],b['depth'],facecolor='#ddcba5',edgecolor=INK,lw=.6,zorder=7))
 a.scatter([0], [0],s=12,color=INK,zorder=8)

f=page('Bellwether + three dungeon approaches','One preserved hub. Two peer expeditions. A high-ridge destination opened by both seals.',1)
a=baseax(f,[.06,.13,.56,.70],[(-150,410),(-510,440)]);terrainplot(a);town(a)
route(a,A,BLUE);route(a,F,ORANGE);route(a,C,PURPLE);route(a,W,BLUE,2,':')
for xx,yy,ww,hh,c in [(23.5,-328.5,105,107,BLUE),(234.5,-9,85,124,ORANGE),(317.5,354.5,109,117,PURPLE)]:
 a.add_patch(Rectangle((xx-ww/2,yy-hh/2),ww,hh,facecolor=BG,edgecolor=c,lw=1.5,alpha=.82,zorder=5))
label(a,(-65,90),'F04  fixed lookout\nZ30.4',(-55,20));label(a,(0,0),'Bellwether\nwell Z20',(-63,-23));label(a,(25,101),'Starter shrine',(-55,30));label(a,(47,-70),'J0  peer-route split',(-94,-10));label(a,(-18,-224),'A0  tower vista / Z5.3',(-93,-13));label(a,(53,-337),'DROWNED ABBEY\ndry floors +2 / -4 / +6',(0,-8),BLUE);label(a,(264,-15),'ASHEN FOUNDRY\nterrace +30',(10,0),ORANGE);label(a,(315,354),'GLASS COURT\nridge +80',(8,0),PURPLE);label(a,(201,133),'G0  Court gate\nboth peer seals',(13,0),PURPLE);label(a,(70,-94),'X0  broken bridge\n38 m draft span',(12,-25),ORANGE);label(a,(175,310),'NE headwaters',(-68,22));label(a,(-25,-480),'Lake outflow',(-25,0));north(a,365,215,35);scale(a,-125,-465,100)
textblock(f,.66,.82,'FIXED GEOGRAPHY',[
'Bellwether remains west of the incised river. All town buildings, paths and anchor coordinates are carried forward without relocation.',
'Abbey: south, downstream in the enclosed lake. Foundry: east across the gorge. Court: northeast on the high ridge.',
'Treasury access stays concealed from the hub and is revealed after Court. The finale is outside these three floorplans.'
],width=48)
textblock(f,.66,.565,'READABLE LANDMARKS',[
'Abbey: broken towers above still water, then a descending stone processional path.',
'Foundry: tall furnace stack and bridge stumps, framed beyond the gorge. A town sightline is a blockout target, not verified.',
'Court: pale ridge silhouette, uphill switchbacks and a clearly closed two-seal gate. Keep the northern starter shrine a small spur.'
],width=48)
textblock(f,.66,.30,'MAP KEY + SCALE AUTHORITY',[
'Blue route: Abbey. Rust route: Foundry. Violet route: Court. Ochre: retained hub roads. Green: retained forest arrival.',
'Contours every 5 m; lake surface Z0. Coordinates and all new distances are draft, not measurements from the oblique concept.',
'Core town terrain retained; outer paths use proposed cut/fill shelves. No flat-plane regional shortcut.'
],width=48,size=9)
save(f,'01-connected-region')

# Save core data now so the preview is a usable first deliverable.
meta={'status':'DRAFT FOR BLOCKOUT; NOT ENGINE VALIDATED','source_commit':'48fd8d6e0ee73c480502906104593c4e6d3b8aae','axes':B['axes'],'origin':B['origin'],'baseline':B,'routes':{'abbey':A.tolist(),'foundry':F.tolist(),'court':C.tolist(),'abbey_water':W.tolist()},'lake_polygon_xy':lake.tolist(),'lake_surface_z':0,'outflow_xyz':OUT.tolist(),'headwaters_extension_xyz':rex.tolist(),'bridge':{'west_xyz':F[2].tolist(),'east_xyz':F[3].tolist(),'deck_width_m':3,'proposed_span_m':38},'rooms':[],'connections':[]}
json.dump(meta,open(P+'/dungeon-layout.json','w'),indent=2)
np.savez_compressed(P+'/regional-terrain-draft.npz',x=x,y=y,height=Z,units='metres',datum='lake Z0')
if os.getenv('PREVIEW_ONLY')=='1':quit()

f=page('The approaches / routes, crossings + recovery','Metric proposals keep the river downhill and make the access problem visible before commitment.',2)
# Three plan insets with identical key but independently indicated scale.
for i,(title,lims,pts,color) in enumerate([
 ('A / ABBEY DESCENT',((-70,115),(-305,-70)),A,BLUE),
 ('F / GORGE CROSSING',((30,245),(-125,15)),F,ORANGE),
 ('G / RIDGE ASCENT',((110,330),(-85,330)),C,PURPLE)]):
 left=.06+i*.315;f.text(left,.824,title,fontsize=12,weight='bold',color=color)
 a=baseax(f,[left,.48,.275,.315],lims);terrainplot(a);route(a,pts,color,2.5)
 if i==0:
  route(a,W,BLUE,2,':');a.add_patch(Rectangle((12,-279),8,8,facecolor=BG,edgecolor=INK,zorder=8));label(a,(-18,-224),'A0 vista',(-32,8));label(a,(0,-265),'A1 safe shore',(-25,-18));label(a,(16,-275),'A2 air', (10,7));label(a,(30,-284),'A3 dry', (7,-10));scale(a,-60,-290,40)
 elif i==1:
  # Alternatives stay local to X0 and do not become a second mandatory dungeon.
  climb=np.array([[51,-94,13.5],[46,-110,8],[60,-117,5],[78,-117,5],[96,-104,10],[89,-94,13.5]])
  route(a,climb,GREEN,1.6,'--');a.scatter([51,89],[-94,-94],s=18,color=INK,zorder=10)
  label(a,(66,-94),'X0: bridge / Magic span',(0,17));label(a,(70,-117),'X1: maintenance ledges',(-30,-17));label(a,(119,-76),'to Court',(0,10));label(a,(230,-40),'F0 entry',(-25,10));scale(a,40,-5,50)
 else:
  a.scatter([201],[133],s=60,marker='s',color=PURPLE,zorder=8);label(a,(201,133),'G0 both seals',(14,0));label(a,(272,227),'G1 ridge vista',(-67,6));label(a,(306,308),'Court Z80',(-75,-7));scale(a,125,-40,50)
 if i==0:
  route(a,[[-2,-190],[-29,-205],[-18,-224]],GREEN,1.2,'--');a.add_patch(Rectangle((-36,-210),14,10,facecolor=BG,edgecolor=GREEN,lw=.8,zorder=5));label(a,(-29,-205),'A4 ruined rest',(-36,17),GREEN)
 elif i==1:
  route(a,[[119,-76],[126,-104]],GREEN,1.2,'--');label(a,(126,-104),'F1 crane view',(0,-14),GREEN)
  route(a,[[179,-55],[194,-23],[213,-47]],GREEN,1.2,'--');label(a,(194,-23),'F2 cooling yard',(-40,8),GREEN)
 else:
  a.add_patch(Rectangle((265,221),14,12,facecolor=BG,edgecolor=INK,zorder=5));route(a,[[272,227],[296,232]],GREEN,1.2,'--');a.scatter([245],[238],marker='x',color=ORANGE,s=45,zorder=8);label(a,(245,238),'G2 broken stair',(-100,20),ORANGE);label(a,(296,232),'G4 view shelf',(10,-18),GREEN)
 north(a,lims[0][1]-18,lims[1][0]+25,18)
 ax=f.add_axes([left,.33,.275,.1],facecolor=BG);profile(ax,pts,color)
 length=dist(pts)[-1];maxgrade=np.max(np.abs(np.diff(pts[:,2]))/np.diff(dist(pts)))*100
 f.text(left,.285,f'{length:.0f} m route  •  {maxgrade:.1f}% max surface grade target',fontsize=9,color=color)
 notes=[
 ['J0 → A1: stone path, 3 m clear. Overlook before descent. Shore recovery before any breath commitment.',
  'A1 → A2 → A3: two short wet legs, dry air refuge between. Guided travel remains a prototype candidate.',
  'Magic / Tech / Constitution each suffice. No timer duration is approved; measure the slowest valid input route.'],
 ['X0: 38 m span, 3 m clear, deck Z13.5. River is about Z2.7 here; bridge does not dam the gorge.',
  'X1: lower maintenance ledges, a surviving low arch and an authored climb. X2: magical crossing uses the bridge corridor. These are alternatives.',
  'Each choice must work both ways. Safe wait/recovery shelves sit on both banks; retain landing geometry.'],
 ['Court branches after the gorge, before Foundry. The ridge route is 3 m clear; turning shelves widen to 6 m.',
  'G0 blocks progression until both seals and the access interaction. No loop or climb bypass around it.',
  'Before G0: view the destination and return freely. Beyond: switchbacks, natural rock shoulders, late reveal.']][i]
 textblock(f,left,.246,'TRAVERSAL CONTRACT',notes,width=43,size=9,leading=.017)
save(f,'02-approach-plans-and-sections')

# Each room is a proposed measured floor plate. Coordinates below are local metres.
def room(id,name,x,y,w,h,z,role):return dict(id=id,name=name,x=x,y=y,width=w,depth=h,floor_z=z,role=role)
DA=[room('DA01','Arrival tower',0,0,12,12,2,'Safe dry arrival; confirm recovery before descent'),room('DA02','Orientation cloister',0,-22,20,16,2,'Foreshadow shutter latch; a reflected path is needed'),room('DA03','Reflector gallery',-26,-22,16,16,2,'ITEM-DA-01: acquire reflector; safe redirect practice'),room('DA04','Submerged viewing gallery',-26,-54,18,20,-4,'Dry underwater view; redirect around a fixed blocker'),room('DA05','Duty hall',-26,-88,22,20,-4,'Dead attendants; first learned-rule encounter'),room('DA06','Shutter chamber',7,-88,18,20,-4,'Mandatory reflector test; resettable shutter chain'),room('DA07','Bell stair',33,-88,12,16,2,'Recovery candidate before final application'),room('DA08','Bellkeeper hall',33,-54,26,28,6,'Reflector REQUIRED to expose Bellkeeper armour'),room('DA09','Seal and relic',33,-20,14,14,6,'Peer seal; plunder evidence and claimant object'),room('DA10','Return cloister',30,3,16,12,2,'Unlock internal return to arrival; same safe crossing'),room('DA11','Flooded side cell',-53,-54,12,12,-2,'Optional, nonessential reward; isolated wet branch')]
AF=[room('AF01','Intake vestibule',0,0,14,14,30,'Safe entry / return confirmation'),room('AF02','Mint floor',0,25,22,22,30,'Foreshadow held-light mechanism and remote work position'),room('AF03','Lantern-stand bay',-29,25,16,18,30,'ITEM-AF-01: acquire stand; safe anchor/shutter practice'),room('AF04','Smoke gallery',-29,57,18,22,34,'Move apart from anchored light; smoke/cover is readable'),room('AF05','Machinery court',0,82,26,22,34,'Workers; cover and exposure combinations'),room('AF06','Shutter test',33,82,18,20,34,'Mandatory stand test: hold light while repositioning'),room('AF07','Recovery antechamber',33,53,14,14,34,'Safe reset / recovery before boss'),room('AF08','Master of Coin hall',33,23,28,28,30,'Stand REQUIRED to hold furnace-shell exposure'),room('AF09','Seal archive',33,-7,16,14,30,'Peer seal; evidence of royal bargain'),room('AF10','Service return',0,-25,14,12,30,'Open return door to AF01; persistent shortcut')]
GC=[room('GC01','Gate vestibule',0,0,16,14,80,'Safe ridge arrival; gate state persists'),room('GC02','Winter garden',0,28,28,24,80,'Roofless landmark court, orient return'),room('GC03','Mirrored gallery',-34,28,18,22,80,'Foreshadow broad-light ambiguity; finite mirror paths'),room('GC04','Aperture room',-34,65,18,20,84,'ITEM-GC-01: acquire aperture; safe receiver isolation'),room('GC05','Reflection test',0,94,26,22,84,'Mandatory aperture isolation test; finite reflection'),room('GC06','Royal records',34,94,18,20,84,'Royal evidence; reinforce selective-light rule'),room('GC07','Keeper ward chamber',34,57,28,28,88,'Aperture REQUIRED to dismantle keeper wards and win'),room('GC08','Final seal chamber',34,23,16,16,88,'Final seal and treasury location reveal'),room('GC09','Return gallery',33,-6,16,12,80,'Descent and unlocked return to GC01'),room('GC10','Concealed threshold',60,23,12,14,88,'Post-reveal future finale connection; scope boundary')]
# Edge records are physical corridors, some with explicit elbow points and stairs.
def edge(a,b,via=None,kind='main',width=3):return dict(a=a,b=b,via=via or [],kind=kind,width_m=width)
DAE=[edge('DA01','DA02'),edge('DA02','DA03'),edge('DA03','DA04'),edge('DA04','DA05'),edge('DA05','DA06'),edge('DA06','DA07'),edge('DA07','DA08'),edge('DA08','DA09'),edge('DA09','DA10'),edge('DA10','DA01',kind='return'),edge('DA03','DA11',[[-53,-22]],'optional',2)]
AFE=[edge('AF01','AF02'),edge('AF02','AF03'),edge('AF03','AF04'),edge('AF04','AF05',[[-29,82]]),edge('AF05','AF06'),edge('AF06','AF07'),edge('AF07','AF08'),edge('AF08','AF09'),edge('AF09','AF10',[[33,-25]]),edge('AF10','AF01',kind='return')]
GCE=[edge('GC01','GC02'),edge('GC02','GC03'),edge('GC03','GC04'),edge('GC04','GC05',[[-34,94]]),edge('GC05','GC06'),edge('GC06','GC07'),edge('GC07','GC08'),edge('GC08','GC09'),edge('GC09','GC01',kind='return'),edge('GC08','GC10',kind='reveal')]

items=[
 {'id':'ITEM-DA-01','name':'Placeable reflector','dungeon':'DA','theme':'Drowned ritual; recover a broken path of light','acquire_room':'DA03','teach_room':'DA03','practice_gate':['DA03','DA04'],'test_gate':['DA06','DA07'],'boss_room':'DA08','seal_room':'DA09','approval_status':'Owner approved 6 October 2026','boss_use':'Redirect lantern light onto shutter receivers to expose armour; then use established attack. Direct light cannot substitute for the reflected approach.','recovery':'Persistent nonconsumable tool; authored stable placements; safe recall/reset when unreachable.','optional_return_use':'DA11 side clue stays optional and recoverable; no underwater essential item.'},
 {'id':'ITEM-AF-01','name':'Shuttered lantern stand','dungeon':'AF','theme':'Bound labour; control when and where the furnace sees light','acquire_room':'AF03','teach_room':'AF03','practice_gate':['AF03','AF04'],'test_gate':['AF06','AF07'],'boss_room':'AF08','seal_room':'AF09','approval_status':'Owner approved 6 October 2026','boss_use':'Anchor and open the lantern stand to keep the shell exposure mechanism lit while moving to the safe strike position; handheld light cannot occupy both positions.','recovery':'Persistent nonconsumable stand; safe recall returns stand/lantern to valid placement or inventory; no lost-light lockout.','optional_return_use':'Optional salvage shutter only; no extra mandatory room or peer-item gate.'},
 {'id':'ITEM-GC-01','name':'Focused lantern aperture','dungeon':'GC','theme':'Truth through appearances; choose the real light path','acquire_room':'GC04','teach_room':'GC04','practice_gate':['GC04','GC05'],'test_gate':['GC05','GC06'],'boss_room':'GC07','seal_room':'GC08','approval_status':'Owner approved 6 October 2026','boss_use':'Use the narrow aperture to isolate small ward receivers through bounded reflected paths; the broad lantern lights adjacent receivers and cannot release the ward. Repeat the already-taught isolation rule until the keeper ward challenge is defeated.','recovery':'Permanent lantern capability after acquisition; focus can reset without consumable loss; required paths remain recoverable.','optional_return_use':'Earlier peer items may aid optional paths only; neither is a hidden Court combat prerequisite.'}
]
all_edges=DAE+AFE+GCE
for item in items:
 for e in all_edges:
  pair=[e['a'],e['b']]
  if pair in [item['practice_gate'],item['test_gate']]:
   e['requires_item']=item['id'];e['requires_demonstrated_use']=True;e['gate_direction']='forward progression only; reverse retreat stays available';e['gate_role']='safe practice' if pair==item['practice_gate'] else 'mandatory combined test'
  if pair==[item['boss_room'],item['seal_room']]:e['requires_boss_defeat']=item['boss_room']
  if e['kind']=='return' and e['a'].startswith(item['dungeon']):e['requires_seal']=item['seal_room'];e['unlock_from']=e['a']
  if e['kind']=='reveal':e['requires_seal']='GC08';e['requires_reveal']=True
meta['revision']='R2';meta['items']=items;meta['main_route_retreat']='Always available before boss victory; only reward shortcut requires seal';meta['item_concepts_status']='Owner approved 6 October 2026'
meta['design_requirement']='Owner approved 6 October 2026: each theme, named local item and required boss-use concept; detailed implementation and dimensions remain draft'

# Floorplan room origins: map-local X/Y translated to common world X/Y without rotation.
origins={'DA':[30,-284],'AF':[230,-40],'GC':[306,308]}
# Region footprints match measured plan bounds, rather than decorative icons.
for prefix,rooms,edges in [('DA',DA,DAE),('AF',AF,AFE),('GC',GC,GCE)]:
 for r in rooms:r['dungeon']=prefix;r['world_x']=r['x']+origins[prefix][0];r['world_y']=r['y']+origins[prefix][1]
 for r in rooms:
  item=next(it for it in items if it['dungeon']==prefix)
  r['acquired_item_id']=item['id'] if r['id']==item['acquire_room'] else ''
  r['boss_required_item_id']=item['id'] if r['id']==item['boss_room'] else ''
  r['approval_status']='Room layout draft; item and boss-use concept approved'
 meta['rooms']+=rooms;meta['connections']+=edges

def floorplan(a,rooms,edges,color):
 rr={r['id']:r for r in rooms}
 # Build corridors beneath floor plates; actual passage width is metric.
 for e in edges:
  ra,rb=rr[e['a']],rr[e['b']];pts=np.array([[ra['x'],ra['y']]]+e['via']+[[rb['x'],rb['y']]])
  for p,q in zip(pts[:-1],pts[1:]):
   v=q-p;n=np.array([-v[1],v[0]])/np.linalg.norm(v)*e['width_m']/2;a.add_patch(Polygon([p+n,q+n,q-n,p-n],facecolor=PALE,edgecolor=INK,lw=.7,zorder=2))
  cc=GREEN if e['kind']=='return' else BLUE if e['kind']=='optional' else PURPLE if e['kind']=='reveal' else color
  a.plot(pts[:,0],pts[:,1],color=cc,lw=1.5,ls='--' if e['kind']!='main' else '-',zorder=4)
  if ra['floor_z']!=rb['floor_z']:
   m=(pts[-2]+pts[-1])/2
   v=pts[-1]-pts[-2];v=v/np.linalg.norm(v);n=np.array([-v[1],v[0]])*1.5
   for k in np.linspace(-2,2,5):
    c=m+v*k;a.plot([c[0]-n[0],c[0]+n[0]],[c[1]-n[1],c[1]+n[1]],color=INK,lw=.7,zorder=5)
 for r in rooms:
  face='#d3e0df' if r['floor_z']<0 else '#e2d9c1'
  if r['id']=='DA11':face='#82a8b1'
  a.add_patch(Rectangle((r['x']-r['width']/2,r['y']-r['depth']/2),r['width'],r['depth'],facecolor=face,edgecolor=INK,lw=1.4,zorder=6))
  a.text(r['x'],r['y']+1.7,r['id'],ha='center',va='center',fontsize=9,weight='bold',zorder=8)
  a.text(r['x'],r['y']-2.5,f"Z{r['floor_z']:+g}",ha='center',va='center',fontsize=8,zorder=8)
 # Function symbols are reference marks, not in-game HUD markers.
 roles={'DA03':'T','DA05':'E','DA06':'P','DA07':'C','DA08':'B','DA09':'S','DA01':'C','AF01':'C','AF03':'T','AF04':'E','AF05':'E','AF06':'P','AF07':'C','AF08':'B','AF09':'S','GC01':'C','GC04':'T','GC05':'P','GC06':'L','GC07':'B','GC08':'S'}
 for r in rooms:
  if r['id'] in roles:
   a.text(r['x']+r['width']/2-2.3,r['y']+r['depth']/2-2.5,roles[r['id']],ha='center',fontsize=6.5,color=color,zorder=10,bbox={'boxstyle':'circle,pad=.18','facecolor':BG,'edgecolor':color,'lw':.6})
 # Doorways explicitly mask perimeter on connecting centreline, without slicing room corners.
 for e in edges:
  ra,rb=rr[e['a']],rr[e['b']];pts=[[ra['x'],ra['y']]]+e['via']+[[rb['x'],rb['y']]]
  for r,q in [(ra,pts[1]),(rb,pts[-2])]:
   d=np.array(q)-[r['x'],r['y']];tx=(r['width']/2)/abs(d[0]) if d[0] else 1e9;ty=(r['depth']/2)/abs(d[1]) if d[1] else 1e9;t=min(tx,ty);p=np.array([r['x'],r['y']])+d*t
   if tx<ty:a.plot([p[0],p[0]],[p[1]-1.35,p[1]+1.35],color=PALE,lw=3,zorder=9)
   else:a.plot([p[0]-1.35,p[0]+1.35],[p[1],p[1]],color=PALE,lw=3,zorder=9)

 for e in edges:
  if e.get('requires_item') or e.get('requires_boss_defeat'):
   ra,rb=rr[e['a']],rr[e['b']];pts=np.array([[ra['x'],ra['y']]]+e['via']+[[rb['x'],rb['y']]])
   m=(pts[-2]+pts[-1])/2
   gate='I' if e.get('requires_item') else 'B'
   a.text(m[0],m[1],gate,ha='center',va='center',fontsize=7,weight='bold',color=INK,zorder=15,bbox={'boxstyle':'square,pad=.18','facecolor':'#fff4d5','edgecolor':GOLD,'lw':1})
 # Main traversal direction arrowheads over corridors, not rooms.
 for e in edges[:2]:
  r,s=rr[e['a']],rr[e['b']];mid=np.array([r['x']+s['x'],r['y']+s['y']])/2;v=np.array([s['x']-r['x'],s['y']-r['y']]);v=v/np.linalg.norm(v)*3
  a.annotate('',xy=mid+v,xytext=mid-v,arrowprops={'arrowstyle':'-|>','color':color,'lw':1.4},zorder=12)

def roomlist(f,rooms,color):
 y=.822
 f.text(.645,y,'ROOM PURPOSE / PROPOSED PLATES',fontsize=12,weight='bold');y-=.032
 for r in rooms:
  f.text(.645,y,f"{r['id']}  {r['name']}  |  {r['width']} × {r['depth']} m",fontsize=9.3,color=color,weight='bold');y-=.018
  for line in textwrap.wrap(r['role'],width=65):f.text(.645,y,line,fontsize=8.8);y-=.016
  y-=.012
 return y

f=page('Drowned Abbey / dry ruins beneath the lake','A short wet threshold; an enclosed, walkable under-lake loop; water remains outside intact walls and glazing.',3)
a=baseax(f,[.055,.33,.54,.51],[(-67,56),(-110,20)]);a.set_facecolor('#bad0d2');floorplan(a,DA,DAE,BLUE)
# water-view glazing, double lines on west walls of enclosed dry galleries.
for r in DA[3:5]:
 xx=r['x']-r['width']/2;a.plot([xx-.5,xx-.5],[r['y']-5,r['y']+5],color=BLUE,lw=3,zorder=10)
label(a,(0,9),'A3 from crossing',(-40,4));label(a,(15,3),'return unlock',(0,15),GREEN)
label(a,(-53,-45),'isolated wet spur',(-30,12),BLUE);north(a,47,-102,13);scale(a,-61,-106,20)
f.text(.069,.286,'Local XY in metres. Origin = world (30, -284). Main corridors 3 m; optional spur 2 m.',fontsize=8.5)
roomlist(f,DA,BLUE)
f.text(.07,.265,'T item • E encounter • P puzzle • B boss • S seal • C safe state • Box I = item gate / B = boss gate',fontsize=8)
# Orthographic section with continuous lake line and an explicit dry connection above water.
s=f.add_axes([.07,.105,.53,.15],facecolor=BG);s.set_xlim(-4,100);s.set_ylim(-8,11);s.axhspan(-8,0,color='#a7c6ce',alpha=.75);s.axhline(0,color=BLUE,lw=1.8)
s.text(98,.5,'LAKE Z0',ha='right',fontsize=8,color=BLUE)
# dry tower entry + stairs down to intact submarine basement + stair up to bell hall
poly=[(4,2),(18,2),(30,-4),(61,-4),(73,2),(80,6),(96,6),(96,10),(79,10),(70,6),(59,-.5),(32,-.5),(20,6),(4,6)]
s.add_patch(Polygon(poly,facecolor=BG,edgecolor=INK,lw=1.5));s.plot([4,18,30,61,73,80,96],[2,2,-4,-4,2,6,6],color=GOLD,lw=2)
s.plot([35,56],[-3.2,-3.2],color=BLUE,lw=2);s.text(45,-6.3,'SEALED DRY GALLERIES Z-4',ha='center',fontsize=8)
s.text(10,7,'A3 / Z+2',ha='center',fontsize=8);s.text(89,10.3,'BELL HALL Z+6',ha='center',fontsize=8)
s.set_xlabel('Unfolded section through route (horizontal diagram, not plan distance)',fontsize=8);s.set_ylabel('Z (m)',fontsize=8);s.tick_params(labelsize=7);s.set_xticks([])
textblock(f,.645,.22,'FLOODLINE + RETURN SAFETY',[
'Proposed intact retaining shell and sealed glazing below Z0; ventilation/entry above lake. No open submerged window into dry rooms.',
'DA11 is separated from the dry basement; its wet stair returns to DA03 above water. Never put essential loot in that cell.',
'Reflector use is mandatory at the boss. Defeat opens DA09; seal opens the shortcut. Main-route retreat remains available before victory.'
],width=57,size=8.5,leading=.016)
save(f,'03-drowned-abbey-floorplan')

f=page('Ashen Foundry / the mint + furnace loop','A practical royal worksite: production floor, worker passages, elevated galleries and the mintmaster’s furnace hall.',4)
a=baseax(f,[.055,.33,.54,.51],[(-47,56),(-38,105)]);floorplan(a,AF,AFE,ORANGE)
label(a,(0,-7),'F0 from gorge',(-65,0));label(a,(0,-18),'service return unlock',(-85,0),GREEN);label(a,(-29,45),'stair +4 m',(-68,0));north(a,47,89,12);scale(a,-41,-32,20)
f.text(.069,.286,'Local XY in metres. Origin = world (230, -40). Main corridors 3 m; doors 2.7 m target.',fontsize=8.5)
roomlist(f,AF,ORANGE)
f.text(.07,.265,'T item • E encounter • P puzzle • B boss • S seal • C safe state • Box I/B = item/boss gate',fontsize=8)
s=f.add_axes([.07,.11,.53,.145],facecolor=BG);s.set_xlim(0,115);s.set_ylim(27,43)
for xx,ww,zz,title in [(0,24,30,'INTAKE / MINT'),(35,42,34,'GALLERY + MACHINERY'),(89,25,30,'FURNACE / SEAL')]:
 s.add_patch(Rectangle((xx,zz),ww,5,facecolor=PALE,edgecolor=INK,lw=1));s.text(xx+ww/2,zz+5.8,title,ha='center',fontsize=7.8)
s.plot([0,24,35,77,89,114],[30,30,34,34,30,30],color=ORANGE,lw=2.5);s.axhline(30,color=INK,lw=.6,ls=':');s.text(52,28,'Ramps/stairs occupy links; service return stays at Z30.',ha='center',fontsize=8)
s.set_ylabel('Z (m)',fontsize=8);s.set_xticks([]);s.tick_params(labelsize=7);s.set_xlabel('Unfolded section; gallery is a distinct +4 m level, not a floating floor',fontsize=8)
textblock(f,.645,.22,'ENCOUNTERS, HAZARDS + EXIT',[
'Worker encounters belong in work areas AF04–AF06, with visible machinery cover and readable escape pockets. Exact counts remain open.',
'Fire/smoke appearance must match authoritative hazards. Magic, filters and endurance are complementary; no infinite tolerance.',
'AF08 requires the stand’s learned anchored-light use. Boss defeat opens AF09; seal recovery opens AF10 → AF01. No Abbey item is required.'
],width=57,size=8.5,leading=.016)
save(f,'04-ashen-foundry-floorplan')

f=page('Glass Court / garden, mirrors + royal witness','A palace worth preserving. The final dungeon tests prior understanding and reveals where the treasury lies.',5)
a=baseax(f,[.055,.33,.54,.51],[(-49,75),(-24,113)]);floorplan(a,GC,GCE,PURPLE)
a.add_patch(Rectangle((-10,19),20,18,fill=False,edgecolor=GREEN,lw=1.0,ls=':',zorder=10))
label(a,(0,7),'roofless garden',(-84,5),GREEN);label(a,(0,-7),'from G0 ridge gate',(-70,-6));label(a,(60,23),'concealed until reveal',(0,20),PURPLE);label(a,(17,-6),'return unlock',(0,-35),GREEN);north(a,65,94,12);scale(a,-42,-19,20)
f.text(.069,.286,'Local XY in metres. Origin = world (306, 308). Main corridors 3 m; doors 2.7 m target.',fontsize=8.5)
roomlist(f,GC,PURPLE)
f.text(.07,.265,'T item • P puzzle • B boss • S seal • C safe state • L lore • Box I/B = item/boss gate',fontsize=8)
s=f.add_axes([.07,.11,.53,.145],facecolor=BG);s.set_xlim(0,115);s.set_ylim(77,99)
for xx,ww,zz,title in [(0,24,80,'GARDEN'),(36,30,84,'GALLERIES / RECORDS'),(78,27,88,'KEEPER / SEAL')]:
 s.add_patch(Rectangle((xx,zz),ww,6,facecolor=PALE,edgecolor=INK,lw=1));s.text(xx+ww/2,zz+7,title,ha='center',fontsize=8)
s.plot([0,24,36,66,78,105],[80,80,84,84,88,88],color=PURPLE,lw=2.5);s.plot([105,114],[88,80],color=GREEN,lw=2,ls='--');s.text(64,78,'Return descent uses switchback stair within GC09 link.',ha='center',fontsize=8)
s.set_ylabel('Z (m)',fontsize=8);s.set_xticks([]);s.tick_params(labelsize=7);s.set_xlabel('Unfolded section; +4 m terraces; return stair geometry must be blocked out',fontsize=8)
textblock(f,.645,.22,'GATES + NARRATIVE RESTRAINT',[
'G0 requires Abbey AND Foundry seals plus the authored access interaction. It persists for ordinary return travel.',
'GC07 is now an item-required boss encounter. Defeat the ward challenge; keeper survival, dialogue and lethal/nonlethal resolution remain open.',
'GC10 is a concealed post-reveal finale threshold, not a visible hub landmark or a fourth dungeon in this atlas. No infinite mirror paths.'
],width=57,size=8.5,leading=.016)
save(f,'05-glass-court-floorplan')

f=page('Approach complications + safe transitions','Distinct expedition areas, then compact reusable boundaries. Every threshold has a viable return side.',6)
# Exact crossing profile, independent of the unfolded interior section.
a=f.add_axes([.07,.57,.52,.25],facecolor=BG);dd=dist(W);a.axhspan(-4,0,color='#b0cbd0');a.axhline(0,color=BLUE,lw=1.4);a.plot(dd,W[:,2],color=GOLD,lw=2.6);a.scatter(dd,W[:,2],s=28,color=GOLD)
a.set_ylim(-4,4);a.set_xlim(-1,dd[-1]+1);a.grid(alpha=.16);a.set_xlabel('Distance along proposed crossing centreline (m)',fontsize=9);a.set_ylabel('Route elevation Z (m)',fontsize=9)
for idx,txt in [(0,'A1 / safe shore'),(3,'A2 / air refuge'),(6,'A3 / dry arrival')]:a.annotate(txt,(dd[idx],W[idx,2]),xytext=(0,18),textcoords='offset points',ha='center',fontsize=8)
a.text(dd[1],-3.4,'WET LEG 1',fontsize=8,color=BLUE);a.text(dd[4],-3.4,'WET LEG 2',fontsize=8,color=BLUE)
f.text(.07,.84,'ABBEY: TWO WET LEGS WITH A REAL DRY REFUGE',fontsize=12,weight='bold',color=BLUE)
textblock(f,.645,.82,'AREAS WITH A PURPOSE',[
'A4 ruined processional rest: an optional loop between the descent and the tower vista. Read the drowned destination; return to the main path without spending access resources.',
'F1 crane lookout: inspect the broken bridge and lower ledges before choosing a solution. F2 cooling yard: a small exploration pocket beside the main ascent, with industrial history rather than compulsory combat.',
'G2 broken direct stair: collapsed masonry makes the direct uphill line unusable. The ordinary walking route bends through G3 roofless gatehouse at G1, then rejoins above; G4 is an optional view shelf. Neither bypasses G0.'
],width=57,size=9,leading=.017)
textblock(f,.07,.48,'BOUNDARY OWNERSHIP / PROPOSED OCCLUSION',[
'T-A: shore/rest → wet threshold → arrival tower. Prepare the destination before intentional entry, while the player can wait safely on land. Keep water interfaces, refuge, tower and all required optical geometry resident.',
'T-F: bank shelters → bridge/maintenance route → eastern rock turn → intake. Readiness before crossing. Keep the whole chosen crossing and both landing shelves valid; do not strand the player halfway.',
'T-G: gatehouse turns → garden vestibule. Ridge rock and building turns hide detail changes naturally. Keep visible palace silhouette and shadow/reflection-relevant geometry even when distant rooms unload.'
],width=80,size=9.5,leading=.018)
textblock(f,.645,.48,'SAVE / RETURN / SCOPE',[
'Checkpoint candidates: A1/A3/DA07; F0/AF07; G0/GC01. These are proposed safe states, not implemented checkpoints.',
'Persist tool, seals, opened internal returns and gate state. Death/retry must retain a viable access solution; do not consume the only essential preparation.',
'Seal recovery unlocks the short return, not all retreat. The main route remains backtrackable before the boss; exterior access stays viable in both directions.',
'The approved tomb rope is the precedent: prepare before commitment, share the boundary set, no normal loading screen in either direction. Never rely on a traversal timer to guarantee I/O.'
],width=57,size=9,leading=.017)
textblock(f,.07,.18,'ACCEPTANCE BEFORE ART OR ENGINE IMPLEMENTATION',[
'1. Walk the draft grades, stairs and turns with supported controls; verify room dimensions and camera clearance. 2. Test each peer dungeon first, with each access build. 3. Prove Abbey flood volumes and safe recovery. 4. Review gate bypasses, repeated entry/exit and saves. 5. Measure combined real-geometry/RT residency on target devices before expansion.',
'Each approved dungeon item is required at its boss. Detailed attacks, timings and numerical budgets remain draft; sheet 7 owns the teaching chain. Main-route retreat is always available before boss victory.'
],width=142,size=9,leading=.018)
save(f,'06-complications-and-transitions')


f=page('Theme → item → mastery → boss','R2 design requirement: each local dungeon item is necessary to win, with the rule taught before the boss.',7)
chains=[('DROWNED ABBEY',BLUE,items[0],['DA02  FORESHADOW','DA03  ITEM + SAFE PRACTICE','DA04–05  COMBINE','DA06  REQUIRED ITEM TEST','DA08  ITEM-REQUIRED BOSS','DA09  SEAL → RETURN']),('ASHEN FOUNDRY',ORANGE,items[1],['AF02  FORESHADOW','AF03  ITEM + SAFE PRACTICE','AF04–05  COMBINE','AF06  REQUIRED ITEM TEST','AF08  ITEM-REQUIRED BOSS','AF09  SEAL → RETURN']),('GLASS COURT',PURPLE,items[2],['GC03  FORESHADOW','GC04  ITEM + SAFE PRACTICE','GC05–06  COMBINE + TEST','GC07  ITEM-REQUIRED BOSS','GC08  SEAL + REVELATION','GC09  RETURN / GC10 LATER'])]
for i,(title,col,item,chain) in enumerate(chains):
 left=.055+i*.318;f.text(left,.831,title,fontsize=12,color=col,weight='bold')
 f.text(left,.801,item['id']+' / '+item['name'],fontsize=9,weight='bold')
 y=.77
 for line in textwrap.wrap(item['theme'],width=47):f.text(left,y,line,fontsize=9);y-=.018
 ax=f.add_axes([left,.447,.274,.27]);ax.set_xlim(0,1);ax.set_ylim(0,6);ax.axis('off')
 for j,stage in enumerate(chain):
  yy=5.7-j;ax.add_patch(Rectangle((.035,yy-.43),.93,.68,facecolor='#e3dece',edgecolor=col,lw=1.1));ax.text(.5,yy-.07,stage,ha='center',va='center',fontsize=8.5,weight='bold',color=col)
  if j<5:ax.annotate('',xy=(.5,yy-.71),xytext=(.5,yy-.45),arrowprops={'arrowstyle':'-|>','color':col,'lw':1.2})
 textblock(f,left,.412,'REQUIRED BOSS USE', [item['boss_use']],width=48,size=9,leading=.017)
 textblock(f,left,.253,'FAILURE / RETURN', [item['recovery'],item['optional_return_use']],width=48,size=8.8,leading=.016)
textblock(f,.055,.139,'NO HIDDEN SECOND-DUNGEON REQUIREMENT',[
'Abbey and Foundry each work with starting equipment, their own access solution and their own local item. Court access requires both seals; its required boss action uses the aperture, not a hidden requirement to carry both peer items. Boss defeat does not automatically mean keeper death.',
'Light outcomes must use bounded supported world-space receiver/occlusion logic. No screen-brightness test, fake marker-only solution, infinite reflection or assumed full-GI simulation. Exact mechanics are proposed for playable review; no boss-only rule appears without teaching.'
],width=163,size=8.2,leading=.014)
save(f,'07-theme-item-boss-progression')

meta['approach_pockets']=[{'id':'J0','xyz':[47,-70,14.9],'role':'Outbound peer-route junction'},{'id':'A0','xyz':[-18,-224,5.3],'role':'Abbey tower vista'},{'id':'A1','xyz':W[0].tolist(),'role':'Shore preparation and safe wait'},{'id':'A2','xyz':W[3].tolist(),'role':'Dry mid-crossing tower refuge'},{'id':'A3','xyz':W[-1].tolist(),'role':'Dry arrival'},{'id':'A4','xyz':[-29,-205,6.3],'role':'Optional ruined processional rest','loop_xyz':[[-2,-190,7.5],[-29,-205,6.3],[-18,-224,5.3]]},{'id':'F0','xyz':F[-1].tolist(),'role':'Foundry intake arrival'},{'id':'F1','xyz':[126,-104,17],'role':'Optional crane outlook','spur_xyz':[[119,-76,17],[126,-104,17]]},{'id':'F2','xyz':[194,-23,27],'role':'Optional cooling yard','loop_xyz':[[179,-55,24],[194,-23,27],[213,-47,28]]},{'id':'G0','xyz':[201,133,45],'role':'Two-seal gate'},{'id':'G1','xyz':[272,227,64],'role':'Ridge vista'},{'id':'G2','xyz':[245,238,65],'role':'Collapsed direct stair; nonwalkable'},{'id':'G3','xyz':[272,227,64],'role':'Roofless gatehouse; walking detour'},{'id':'G4','xyz':[296,232,64],'role':'Optional view shelf','spur_xyz':[[272,227,64],[296,232,64]]}]
meta['crossing_alternatives']={'X0':{'role':'repair bridge','xyz':F[2:4].tolist()},'X1':{'role':'maintenance ledges/climb; authored traversal','xyz':climb.tolist()},'X2':{'role':'Magic span on X0 corridor','xyz':F[2:4].tolist()}}
meta['transitions']=[{'id':'T-A','boundary':'A1-A2-A3','safe_prepare':'A1','resident':'crossing, refuge, tower, relevant water/RT geometry'},{'id':'T-F','boundary':'X0/X1/X2 to F0','safe_prepare':'west/east bank shelves','resident':'chosen crossing, both landing sides, relevant gorge/RT geometry'},{'id':'T-G','boundary':'G0 to GC01','safe_prepare':'gatehouse shelf','resident':'local turns, garden arrival, relevant palace silhouette/RT geometry'}]
meta['legend']={'main_corridor_width_m':3,'door_clear_width_target_m':2.7,'optional_wet_spur_width_m':2,'stair_symbol':'parallel bars denote proposed vertical link; detailed stair treads unresolved','return_dashed_green':'unlocks from inside','optional_dashed_blue':'isolated wet side route','reveal_dashed_violet':'post-reveal finale boundary','north':'up; design reference only, no HUD compass assumption'}
meta['validation_limits']=['Metric proposals, not playable or engine validated','Sections explicitly unfolded where stated; crossing distance profile is metric','Terrain is planning data, not a final collision/render mesh','Tool mechanics, room functions and exact enemy staging need author review','Watertight basement is proposed architecture, not approved flood-history canon']
json.dump(meta,open(P+'/dungeon-layout.json','w'),indent=2)
with open(P+'/room-register.csv','w') as fp:
 writer=csv.DictWriter(fp,fieldnames=list(meta['rooms'][0]));writer.writeheader();writer.writerows(meta['rooms'])
with open(P+'/route-register.csv','w') as fp:
 writer=csv.writer(fp);writer.writerow(['route','station','x_m','y_m','z_m','cumulative_horizontal_m'])
 for name,pts in meta['routes'].items():
  for i,(p,d) in enumerate(zip(pts,dist(pts))):writer.writerow([name,i,*p,round(float(d),3)])
with PdfPages(P+'/The-Horde-Dungeon-Atlas-Draft.pdf') as pdf:
 for f in figs:pdf.savefig(f,facecolor=BG)
# Final output metadata contains no claims beyond demonstrated checks.
checks={'baseline_all_coordinates_unchanged':meta['baseline']==B,'existing_river_monotonic_upstream':bool(np.all(np.diff(RIVER[:,2])>0)),'extended_headwaters_monotonic_upstream':bool(np.all(np.diff(rex[:,2])>0)),'outflow_monotonic_downstream':bool(np.all(np.diff(OUT[:,2])<0)),'unique_room_ids':len(set(r['id'] for r in meta['rooms']))==len(meta['rooms']),'room_count':len(meta['rooms']),'required_item_count':len(items),'page_count':len(figs),'route_lengths_m':{k:round(float(dist(v)[-1]),2) for k,v in meta['routes'].items()}}
for prefix,rs,es in [('DA',DA,DAE),('AF',AF,AFE),('GC',GC,GCE)]:
 ids={r['id'] for r in rs};seen={rs[0]['id']}
 while True:
  old=len(seen)
  for e in es:
   if e['a'] in seen:seen.add(e['b'])
   if e['b'] in seen:seen.add(e['a'])
  if len(seen)==old:break
 checks[prefix+'_connected']=seen==ids
 checks[prefix+'_no_room_overlap']=all(not (abs(a['x']-b['x'])<(a['width']+b['width'])/2 and abs(a['y']-b['y'])<(a['depth']+b['depth'])/2) for i,a in enumerate(rs) for b in rs[i+1:])
json.dump(checks,open(P+'/validation.json','w'),indent=2)
print(json.dumps(checks,indent=2))

for item,rs,es in zip(items,[DA,AF,GC],[DAE,AFE,GCE]):
 def reachable(with_item):
  seen={rs[0]['id']}
  while True:
   old=len(seen)
   for e in es:
    if e.get('requires_boss_defeat') or e.get('requires_seal'):continue
    if e.get('requires_item') and not with_item:continue
    if e['a'] in seen:seen.add(e['b'])
    if e['b'] in seen:seen.add(e['a'])
   if len(seen)==old:return seen
 checks[item['dungeon']+'_boss_unreachable_without_item']=item['boss_room'] not in reachable(False)
 checks[item['dungeon']+'_boss_reachable_with_own_item']=item['boss_room'] in reachable(True)
 checks[item['dungeon']+'_item_acquisition_reachable_without_item']=item['acquire_room'] in reachable(False)
 checks[item['dungeon']+'_no_peer_item_dependency']=all(e.get('requires_item',item['id'])==item['id'] for e in es)
json.dump(checks,open(P+'/validation.json','w'),indent=2)
print('R2 gate checks:',json.dumps({k:v for k,v in checks.items() if 'item' in k or 'boss' in k},indent=2))
