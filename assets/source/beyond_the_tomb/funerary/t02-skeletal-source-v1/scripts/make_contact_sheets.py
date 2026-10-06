from PIL import Image,ImageDraw,ImageFont
import argparse,os
p=argparse.ArgumentParser();p.add_argument('--package',required=True);a=p.parse_args();root=a.package
font='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf';bold='/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'
def ft(size,strong=False):return ImageFont.truetype(bold if strong else font,size)
items=[('01_skull_threequarter','Skull / jaw master','5,564 triangles | 20.8 cm high'),('03_skull_cage','Skull / jaw source cage','2,466 triangles | separate coarse candidate'),('02_skull_side','Skull master: side view','Real openings; inherited open boundaries'),('04_femur','Femur master','396 triangles | 46.4 cm long'),('05_humerus','Humerus master','304 triangles | 31.3 cm long'),('08_core_front','Static skeletal core','15,585 triangles | 1.72 m | no rig / clips'),('06_arrangement_a','Sparse example A','Linked masters; local composition only'),('07_arrangement_b','Sparse example B','No accepted niche or scene dimensions'),('09_core_back','Static core: back view','Source topology still requires review')]
canvas=Image.new('RGB',(1500,1880),(25,30,35));d=ImageDraw.Draw(canvas);d.text((35,28),'THE HORDE  /  SKELETAL SOURCE KIT',fill=(242,235,217),font=ft(34,True));d.text((35,80),'Zero-spend source preparation  |  Gord Goodwin CC0 geometry  |  One matte bone surface',fill=(180,190,200),font=ft(21))
for i,(file,title,sub) in enumerate(items):
 x=30+(i%3)*495;y=130+(i//3)*555
 im=Image.open(root+'/evidence/renders/'+file+'.png').convert('RGB').resize((480,480),Image.Resampling.LANCZOS);canvas.paste(im,(x,y));d.text((x,y+489),title,fill=(242,235,217),font=ft(21,True));d.text((x,y+520),sub,fill=(185,195,205),font=ft(16))
d.text((35,1810),'Fresh GLB imports in Blender Cycles. Static candidates only; native import, RT cost and gameplay fit remain open.',fill=(180,190,200),font=ft(20));canvas.save(root+'/evidence/contact-sheet.jpg',quality=94,subsampling=0)
canvas=Image.new('RGB',(1220,790),(25,30,35));d=ImageDraw.Draw(canvas);d.text((25,22),'INHERITED TOPOLOGY / INSPECTION',fill=(242,235,217),font=ft(28,True));d.text((25,70),'Orange: open boundary edges. Red: more than two adjacent faces. Preview overlays only.',fill=(210,185,150),font=ft(19))
for i,(f,title,sub) in enumerate([('12_skull_open_boundaries','Skull source cage','34 open edges in HEAD; 68 after selective smoothing'),('13_spine_source_topology','Spine source','42 boundary edges + 35 multi-face edges')]):
 x=20+i*610;im=Image.open(root+'/evidence/renders/'+f+'.png').convert('RGB').resize((580,580),Image.Resampling.LANCZOS);canvas.paste(im,(x,112));d.text((x,706),title,fill=(240,235,220),font=ft(23,True));d.text((x,743),sub,fill=(185,195,205),font=ft(18))
canvas.save(root+'/evidence/topology-contact-sheet.jpg',quality=94,subsampling=0)
