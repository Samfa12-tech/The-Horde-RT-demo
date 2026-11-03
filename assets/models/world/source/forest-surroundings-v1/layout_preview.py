"""Optional Pillow source-layout illustration; not a game/RT render."""
import argparse
import json
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[4]
p=argparse.ArgumentParser();p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
if not out.is_relative_to(ROOT/'reports'):raise ValueError('Keep previews in ignored reports')
out.parent.mkdir(parents=True,exist_ok=True)
c=json.loads((HERE/'composition.json').read_text());mapping=json.loads((ROOT/'docs/references/horde-1.7/d37f1638/docs/design/world/connected/shared-coordinate-layout.json').read_text())
im=Image.new('RGB',(1440,1300),'#eee9dd');d=ImageDraw.Draw(im,'RGBA')
font=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',20);small=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',15);title=ImageFont.truetype('C:/Windows/Fonts/segoeuib.ttf',30)
def pos(x,z):return (70+(x+125)*4,105+(z+125)*4)
for x in range(-120,141,20):d.line([pos(x,-125),pos(x,150)],fill='#d2d0c2');d.text(pos(x,-125),str(x),font=small,fill='#6b7068')
for z in range(-120,151,20):d.line([pos(-125,z),pos(140,z)],fill='#d2d0c2');d.text(pos(-132,z),str(z),font=small,fill='#6b7068')
town=c['townKeepout'];d.rectangle([pos(*town['min']),pos(*town['max'])],fill=(185,149,103,25),outline='#b9a88b',width=2)
river=[pos(x+78.3,126.2-y) for x,y,h in mapping['river_surface']];d.line(river,fill='#8bafb8',width=12)
for b in mapping['buildings']:
 x,z=b['x']+78.3,126.2-b['y'];d.rectangle([pos(x-b['width']/2,z-b['depth']/2),pos(x+b['width']/2,z+b['depth']/2)],fill='#817562',outline='#655747');d.text(pos(x,z),b['id'],font=small,fill='white')
for row in c['placements']:
 x,z=row['x'],row['z'];r=6 if row['band']=='near-original' else 7 if row['band']=='middle' else 6;px,py=pos(x,z)
 color={'near-original':'#294c36','middle':'#518458','far':'#718c85'}[row['band']]
 d.ellipse((px-r,py-r,px+r,py+r),fill=color,outline='#294c3655')
for w in c['windows']:
 a,b=w['from'],w['to'];d.line([pos(*a),pos(*b)],fill=(242,206,81,125),width=round(w['halfWidth']*8))
for edge in c['boundary']:
 color='#695240' if edge['preferredTreatment']!='inspect-existing-bank' else '#a26f52'
 d.line([pos(edge['a'][0],edge['a'][2]),pos(edge['b'][0],edge['b'][2])],fill=color,width=2)
for rock in c['existingRockReferences']:
 x,y=pos(rock['x'],rock['z']);d.polygon([(x,y-4),(x+4,y),(x,y+4),(x-4,y)],fill='#868178',outline='#3d3c37')
d.line([pos(*q) for q in c['route']],fill='#c59535',width=10)
for label,index in [('F01',0),('F02',1),('F03',4),('F04',5)]:
 x,y=pos(*c['route'][index]);d.ellipse((x-7,y-7,x+7,y+7),fill='#f1d69a',outline='#5e4425',width=2);d.text((x+10,y-19),label,font=font,fill='#2f3531')
d.rectangle((1110,0,1440,1300),fill='#eee9dd')
d.text((35,22),'Whole-route woodland / source candidate',font=title,fill='#244538')
for y,text in enumerate(['480 total trees','64 original near','96 middle / 8 stands','320 far / 15 stands','','Brown: fallback fence','Rust: inspect existing bank','Gold: reserved town views','Blue: mapped river','','R2 map transform preserved','Terrain: actual r23 snapshot','No production activation','','BOUNDARIES NOT ADMITTED','No collision removed','No native forest render','No GPU/FPS claim']):d.text((1120,120+y*31),text,font=small,fill='#37433d')
d.text((35,1245),'Planning illustration only. Circle sizes indicate tree roles, not a visibility/coverage acceptance or measured canopy footprint.',font=small,fill='#37433d')
im.save(out)
