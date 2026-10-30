from PIL import Image,ImageDraw,ImageFont
import json,pathlib
R=pathlib.Path(__file__).resolve().parents[1]
rows=json.load(open(R/'validation/blender-reimport-report.json'))
font='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
F=lambda n:ImageFont.truetype(font,n)
W,H=1920,2030;im=Image.new('RGB',(W,H),'#142127');d=ImageDraw.Draw(im)
d.text((38,28),'THE HORDE 1.7  /  FUNERARY UTILITY KIT',font=F(42),fill='#efe8d7')
d.text((40,86),'A17 / T03  •  7 original Blender assets  •  actual GLB reimport renders  •  0 paid credits',font=F(23),fill='#b5c7c5')
names=['Displaced stone lid','Offering bowl','Broken urn base','Curved urn-rim shard','Candle stub / low','Candle stub / tall','Candle stub / spent']
for i,(a,name) in enumerate(zip(rows,names)):
 x=(i%3)*640;y=138+(i//3)*615
 im.paste(Image.open(R/'previews'/f"{a['id']}.png").convert('RGB'),(x,y))
 d.rectangle((x,y+540,x+639,y+614),fill='#1f3035')
 d.text((x+21,y+550),name,font=F(23),fill='#efe8d7')
 sz=a['bounds_blender_m']['size'];d.text((x+21,y+582),f"{a['triangles']:,} triangles  |  {sz[0]:.2f} × {sz[1]:.2f} × {sz[2]:.2f} m",font=F(17),fill='#a9c0bd')
y=138+2*615
for x,title,lines in [(665,'SOURCE-PACK CHECKS',['Meters / base-centered pivots','GLB 2.0 + editable .blend sources','UVs / normals / tangent bases','Closed shells; no degenerate faces','0 textures / opaque PBR materials','No flame, light or baked lighting']), (1305,'BEFORE GAME ADMISSION',['Bind existing Horde stone material','Review exact scene placement','Use simple collision where needed','Check native RT import and all views','Profile on Android and Windows','Owner in-game visual acceptance'])]:
 d.text((x,y+72),title,font=F(25),fill='#e4c98b')
 for j,line in enumerate(lines):d.text((x,y+136+j*51),line,font=F(21),fill='#d0dcd8')
d.text((40,1992),'Geometry/source candidates. Studio lighting is preview-only. Scale is stated per tile; previews are individually framed.',font=F(19),fill='#a9c0bd')
im.save(R/'Horde-1.7-funerary-utility-contact-sheet.png')
