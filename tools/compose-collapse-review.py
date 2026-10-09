"""Compose the gated engineering review from evaluated Blender study meshes."""
import json
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

root = Path(__file__).resolve().parent.parent / "docs/design/1.6.2-collapse"
study = json.loads((root / "images/study-meshes.json").read_text())
root.joinpath("plans").mkdir(exist_ok=True)
font = ImageFont.truetype("C:/Windows/Fonts/arial.ttf", 22)
small = ImageFont.truetype("C:/Windows/Fonts/arial.ttf", 17)

def plan(name, ceiling=False):
    image = Image.new("RGB", (480, 900), "white")
    draw = ImageDraw.Draw(image)
    def p(x, z):
        return (round(240 + x * 96), round(470 - z * 53))
    draw.rectangle([p(-1.92, 6.35), p(1.92, -6.47)], fill="#eeeeee", outline="black", width=2)
    draw.rectangle([p(-1.85, 3.4), p(1.85, -6.4)], fill="white", outline="black", width=2)
    draw.rectangle([p(-1.40, 6.25), p(1.40, 3.4)], fill="white", outline="black", width=2)
    if ceiling:
        draw.rectangle([p(-1.40, 6.25), p(1.40, 3.4)], fill="#b4b4b4", outline="black", width=2)
        draw.rectangle([p(-1.85, 3.4), p(-1.40, 3.05)], fill="#555555")
        draw.rectangle([p(1.40, 3.4), p(1.85, 3.05)], fill="#555555")
    else:
        for i in range(5):
            z = 4.78 + i * 0.28
            draw.rectangle([p(-0.48, z + 0.28), p(0.48, z)], fill="#c0c0c0", outline="#555555")
        for mesh in study["placements"]:
            for face in mesh["faces"]:
                polygon = [p(mesh["vertices"][i][0], mesh["vertices"][i][2]) for i in face]
                draw.polygon(polygon, fill="#888888", outline="#646464")
        # Functional reserve overlays, not proxy furniture/prop silhouettes.
        cx, cy = p(0, 1.85)
        draw.ellipse((cx-82, cy-45, cx+82, cy+45), outline="black", width=3)
        draw.rectangle([p(-1.55, 2.35), p(-0.72, 0.05)], fill="#dddddd", outline="black", width=2)
        for xa, xb in [(-1.20,-.78),(.78,1.20)]:
            draw.rectangle([p(xa,-3.25),p(xb,-3.55)],fill="#666666",outline="black")
        draw.rectangle([p(-.72,-4.2),p(.72,-5.7)],outline="black",width=2)
        draw.line([p(0, 1.0), p(0, -2.7)], fill="black", width=3)
        draw.polygon([p(-0.15,-2.5),p(0,-2.9),p(0.15,-2.5)],fill="black")
    image.save(root / ("plans/" + name + ".png"))
    return image

lower = plan("lower-room")
ceiling = plan("reflected-ceiling", True)
elevation = Image.new("RGB", (960, 650), "white")
d = ImageDraw.Draw(elevation)
def e(x, y): return (int(480+x*210), int(470-y*160))
d.rectangle([e(-1.85,1.35),e(1.85,-.95)],fill="#eeeeee",outline="black",width=3)
d.rectangle([e(-1.4,1.15),e(1.4,-.95)],fill="#dddddd",outline="black",width=3)
for mesh in sorted(study["placements"], key=lambda m:-m["position"][2]):
    for face in mesh["faces"]:
        d.polygon([e(mesh["vertices"][i][0],mesh["vertices"][i][1]) for i in face],fill="#888888",outline="#606060")
elevation.save(root / "plans/rear-elevation.png")
mask = Image.new("RGB", (960,650), "black")
ImageDraw.Draw(mask).rectangle([e(-1.4,1.15),e(1.4,-.95)],fill="white")
mask.save(root / "plans/blocked-opening-mask.png")
sheet = Image.new("RGB", (1600,1320), "#f4f2ed")
d = ImageDraw.Draw(sheet)
d.text((28,18),"Collapse layout candidate - engineering study, approval pending",fill="black",font=font)
for x,image,title in [(20,lower,"Lower room"),(520,ceiling,"Reflected ceiling")]:
    sheet.paste(image,(x,90)); d.text((x,58),title,fill="black",font=font)
sheet.paste(elevation.resize((560,379)),(1020,90))
d.text((1020,58),"Rear elevation / actual mesh projections",fill="black",font=small)
for i,name in enumerate(["boulder-study","broken-dressed-stone-study","broken-lintel-study"]):
    im=Image.open(root/("images/"+name+".png")).convert("RGB").resize((180,180))
    sheet.paste(im,(1020+i*185,510))
d.text((1020,480),"Reduced rock / dressed stone / broken lintel",fill="black",font=small)
d.text((1020,730),"Sealed backing: z=6.35m",fill="black",font=font)
d.text((1020,770),"Stair remnant: z=4.78..6.18m",fill="black",font=font)
d.text((1020,810),"Rubble begins behind old cap z=3.4m",fill="black",font=small)
d.text((1020,850),"Spawn stays x=0,z=1.85m",fill="black",font=font)
d.text((1020,890),"No live return / no extra lights",fill="black",font=font)
d.text((28,1040),"Plan metadata: circle = spawn/held-item reserve; left box = gallery; lower outline = skeleton-space reserve; arrow = forward.",fill="black",font=small)
d.text((28,1080),"The stair remnant is behind the solid blockage. A recessed sealed enclosure prevents sky/light leaks.",fill="black",font=small)
d.text((28,1120),"Asymmetric interlocked pile: two grounded rocks, one smaller upper rock, fallen dressed stone and tilted lintel.",fill="black",font=small)
d.text((28,1160),"Only this rear alcove changes. Spawn, gallery, skeleton encounter, live collision boundary and forward route remain.",fill="black",font=small)
d.text((28,1200),"This study uses neutral Blender shading. It is neither final composition nor evidence of Vulkan RT or mobile cost.",fill="black",font=small)
sheet.save(root / "layout-contact-sheet.png")
maskSheet=Image.new("RGB",(1000,730),"white")
maskSheet.paste(mask,(20,50))
ImageDraw.Draw(maskSheet).text((20,12),"Blocked former entry - 2.8m x 2.1m, 2.95m deep sealed remnant",fill="black",font=small)
maskSheet.save(root/"opening-contact-sheet.png")
print(json.dumps({k:study[k] for k in ["sourceTriangles","sourceBoundsBlenderMeters","referenceRockTriangles"]}))
