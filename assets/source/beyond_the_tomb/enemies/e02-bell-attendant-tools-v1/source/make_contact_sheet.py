"""Assemble labeled original Blender evidence renders; no geometry/image retouching."""
from PIL import Image,ImageDraw,ImageFont
from pathlib import Path
import argparse
ap=argparse.ArgumentParser();ap.add_argument('--asset-dir',required=True);r=Path(ap.parse_args().asset_dir)
f='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf';fb='/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'
font=lambda n,b=False:ImageFont.truetype(fb if b else f,n)
sheet=Image.new('RGB',(1800,1490),(24,29,31));d=ImageDraw.Draw(sheet)
d.text((30,25),'THE HORDE  /  E02 DUTY TOOL PROPOSALS',font=font(36,True),fill=(225,222,207))
d.text((32,77),'Original hollow iron handbell + short ash staff  |  Shared H1 role; no new body or rig',font=font(23),fill=(171,184,182))
cards=[('02_bell_three_quarter.png','Handbell: 29.5 cm high','1,640 triangles / 2 meshes / 1 material'),('09_handbell_fresh_glb_import.png','Fresh GLB import: handbell','Empty-scene import; underside inspection fill'),('10_staff_fresh_glb_import.png','Fresh GLB import: 88 cm staff','564 triangles / 1 mesh / 1 material'),('03_bell_open_mouth_clapper.png','Hollow cup + separate clapper','Underside close-up; added inspection fill'),('04_bell_grip.png','Bell grip: 27 x 23 mm oval','95 mm provisional hand zone; root at origin'),('07_staff_grip.png','Staff grip: 29 x 26 mm oval','110 mm provisional hand zone; +Z working end')]
for i,(name,title,caption) in enumerate(cards):
 x=30+(i%3)*590;y=125+(i//3)*630
 im=Image.open(r/'previews'/name).convert('RGB');im.thumbnail((560,560),Image.Resampling.LANCZOS);sheet.paste(im,(x+(560-im.width)//2,y+(560-im.height)//2))
 d.text((x,y+568),title,font=font(23,True),fill=(228,225,211));d.text((x,y+601),caption,font=font(17),fill=(161,177,174))
d.text((30,1411),'Views are independently framed, not a same-scale comparison. Opaque PBR: three shared 512px maps.',font=font(21),fill=(200,207,199))
d.text((30,1449),'Source + fresh-import evidence only. H1 fitting, shoulder/yoke variant, alarm animation/audio and engine/device tests remain.',font=font(19),fill=(162,181,176))
sheet.save(r/'previews'/'contact_sheet.jpg',quality=94,subsampling=0)
