"""Add navigation to frozen planning SVG/data; never execute or overwrite archive sources.

Run from any directory: python docs/design/world/atlas/build_atlas.py
Requires the pinned Git object below (fetch retained planning branch if absent).
Python standard library only. overview.webp and visual-review images are browser renders.
"""
from pathlib import Path
import base64
import copy
import hashlib
import html
import json
import re
import subprocess
import xml.etree.ElementTree as ET

OUT = Path(__file__).resolve().parent
REPO = OUT.parents[3]
PIN = "d37f16381311c6e620f0c4a9d01c1cce83260dc8"
PREFIX = "docs/design/world/"
WEB = f"https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/{PIN}/"
RAW = f"https://raw.githubusercontent.com/Samfa12-tech/The-Horde-RT-demo/{PIN}/"
NS = "http://www.w3.org/2000/svg"
XLINK = "http://www.w3.org/1999/xlink"
ET.register_namespace("", NS)
ET.register_namespace("xlink", XLINK)

def source(path):
    return subprocess.check_output(["git", "show", f"{PIN}:{PREFIX}{path}"], cwd=REPO)

def url(path):
    return WEB + PREFIX + path

def element(tag, attrs=None, text=None):
    e = ET.Element(f"{{{NS}}}{tag}", {k: str(v) for k, v in (attrs or {}).items()})
    e.text = text
    return e

def add(parent, tag, attrs=None, text=None):
    e = element(tag, attrs, text)
    parent.append(e)
    return e

def save(path, text):
    (OUT / path).parent.mkdir(parents=True, exist_ok=True)
    (OUT / path).write_text(text, encoding="utf-8", newline="\n")

B = json.loads(source("connected/shared-coordinate-layout.json"))
D = json.loads(source("dungeons/dungeon-layout.json"))
assert B == D["baseline"], "Retained town/forest baseline must match exactly."
original_bytes = source("dungeons/01-connected-region.svg")
original = ET.fromstring(original_bytes)
axes = original.find(f".//{{{NS}}}g[@id='axes_1']")
assert axes is not None
patch = axes.find(f"{{{NS}}}g[@id='patch_2']/{{{NS}}}path").get("d")
nums = list(map(float, re.findall(r"-?\d+(?:\.\d+)?", patch)))
left, bottom, right, _, _, top = nums[:6]
sx = (right-left)/560
sy = (bottom-top)/950
assert abs(sx-sy) < 1e-7, "Original map must use equal metric axes."
MODEL = [-150, 410, -510, 440]
MAP_X, MAP_Y = 210, 120

def xy(x, y):
    return MAP_X+x+150, MAP_Y+440-y

def rect(bounds):
    x0, x1, y0, y1 = bounds
    x, y = xy(x0, y1)
    return x, y, x1-x0, y1-y0

origins = {"DA": [30, -284], "AF": [230, -40], "GC": [306, 308]}
floors = {}
for prefix, origin in origins.items():
    rr = [r for r in D["rooms"] if r["dungeon"] == prefix]
    for r in rr:
        assert r["world_x"] == r["x"]+origin[0]
        assert r["world_y"] == r["y"]+origin[1]
    floors[prefix] = [min(r["world_x"]-r["width"]/2 for r in rr),
                      max(r["world_x"]+r["width"]/2 for r in rr),
                      min(r["world_y"]-r["depth"]/2 for r in rr),
                      max(r["world_y"]+r["depth"]/2 for r in rr)]

SHEETS = [
    dict(id="01-forest", number="01", name="Forest / rescue rim to lookout", color="#315b39",
         files=["connected/01-forest-track-1-7"], bounds=[-126,-47,79,153],
         note="Surface plan; exact shared XY registration within the proposed blockout. View X −126…−47, Y 79…153 m; north up; 2 m contours; 20 m scale bar. F01 rescue rim (−112,139,33.6), F04 lookout (−65,90,30.4). This does not align the tomb interior.",
         short=["Forest / rescue rim", "Reunion → fixed lookout", "79 × 74 m view · north up"], point=[-126,139], label=[12,405,190]),
    dict(id="02-bellwether", number="02", name="Bellwether / terrain, use, river and mill", color="#805716",
         files=["connected/02a-bellwether-terrain-1-8", "connected/02b-bellwether-use-1-8", "connected/02c-bellwether-water-height-1-8"], bounds=[-85,92,-78,116],
         note="Terrain and use sheets share X −85…92, Y −78…116 m; north up; terrain contours 2 m; 40 m scale bars. The river/mill sheet is a set of profiles and an unfolded diagram, not a third plan footprint. The true mill section on the use sheet is at world Y=13 m, X=25…90 m. Well (0,0,20); mill floor Z16; lake datum Z0. Optional mill bridge is separate from the Foundry crossing.",
         short=["Bellwether · 3 sheets", "Terrain + use share this view", "Profiles link to river / mill"], point=[-85,0], label=[12,650,190]),
    dict(id="03-abbey", number="03", name="Drowned Abbey / floorplan and section", color="#285c75",
         files=["dungeons/03-drowned-abbey-floorplan"], bounds=floors["DA"],
         note="Projected room-plate bounds X −29…76, Y −382…−275 m (105 × 107 m), not surface terrain. Local (x,y) → world (x+30,y−284), no rotation; north up; 20 m scale bar. Local plan view X −67…56, Y −110…20 m. DA01/A3 origin (30,−284), dry floor Z+2; dry galleries Z−4; hall Z+6. The unfolded section is not a plan-distance axis. Alignment is exact in the draft source, unaccepted in runtime.",
         short=["Drowned Abbey", "Projected interior · 105 × 107 m", "World = local + (30, −284)"], point=[76,-382], label=[792,875,312]),
    dict(id="04-foundry", number="04", name="Ashen Foundry / floorplan and section", color="#934821",
         files=["dungeons/04-ashen-foundry-floorplan"], bounds=floors["AF"],
         note="Projected room-plate bounds X 192…277, Y −71…53 m (85 × 124 m), not a surface terrain sheet. Local (x,y) → world (x+230,y−40), no rotation; north up; 20 m scale bar. Local plan view X −47…56, Y −38…105 m. AF01/F0 origin (230,−40), intake/mint floor Z30; galleries Z34. The section is unfolded. Alignment is exact in the draft source, unaccepted in runtime.",
         short=["Ashen Foundry", "Projected interior · 85 × 124 m", "World = local + (230, −40)"], point=[277,53], label=[792,535,312]),
    dict(id="05-court", number="05", name="Glass Court / floorplan and section", color="#675478",
         files=["dungeons/05-glass-court-floorplan"], bounds=floors["GC"],
         note="Projected room-plate bounds X 263…372, Y 296…413 m (109 × 117 m), not surface terrain. Local (x,y) → world (x+306,y+308), no rotation; north up; 20 m scale bar. Local plan view X −49…75, Y −24…113 m. GC01 origin (306,308), floors Z80/84/88. GC10 is a threshold proposal; it does not locate or size the treasury/finale. Alignment is exact in the draft source, unaccepted in runtime.",
         short=["Glass Court", "Projected interior · 109 × 117 m", "World = local + (306, 308)"], point=[372,413], label=[792,163,312]),
    dict(id="06-approaches", number="06", name="Approaches / three plan insets and profiles", color="#344b43",
         files=["dungeons/02-approach-plans-and-sections"], bounds=None,
         note="6A Abbey: world X −70…115, Y −305…−70 m, 40 m scale bar. 6B gorge/Foundry: X 30…245, Y −125…15 m, 50 m scale bar. 6C ridge/Court: X 110…330, Y −85…330 m, 50 m scale bar. All three plan insets are north up and share regional world XY; their page scales differ. The lower panels are route-distance/Z profiles, not extra map extents. The Abbey inlet/crossing extends from A1 (0,−265) through refuge A2 (16,−275) to A3 (30,−284).", short=[]),
    dict(id="07-transitions", number="07", name="Crossings / complications and transitions", color="#344b43",
         files=["dungeons/06-complications-and-transitions"], bounds=None,
         note="An Abbey crossing distance/Z profile plus text keyed to A1–A3, X0/X1/X2–F0, and G0–GC01. No independent north, plan scale or footprint. Use sheet 06 to place these references; the profile uses metric distance along the authored crossing centreline.", short=[]),
    dict(id="08-progression", number="08", name="Dungeon theme, item and boss progression", color="#344b43",
         files=["dungeons/07-theme-item-boss-progression"], bounds=None,
         note="A room-ID progression diagram for the same three floorplans (03–05). No north, terrain scale or new geographic footprint. Abbey and Foundry remain peer branches; Court follows both seals. This atlas adds navigation only.", short=[]),
]
APPROACH = [
    dict(key="6A", name="Abbey descent", bounds=[-70,115,-305,-70], point=D["routes"]["abbey"][3][:2], label=[792,756,312], color="#285c75"),
    dict(key="6B", name="Foundry gorge", bounds=[30,245,-125,15], point=D["bridge"]["east_xyz"][:2], label=[792,656,312], color="#934821"),
    dict(key="6C", name="Court ridge", bounds=[110,330,-85,330], point=D["routes"]["court"][11][:2], label=[792,354,312], color="#675478"),
]

def plot_group():
    # Move only the source drawing's viewport. Terrain/routes/labels stay byte-equivalent
    # as an XML subtree; no geometry authoring or generation is performed here.
    tx = MAP_X - left/sx
    ty = MAP_Y - top/sy
    g = element("g", {"id":"region-content", "transform":f"translate({tx:.10f} {ty:.10f}) scale({1/sx:.10f})"})
    g.append(copy.deepcopy(axes))
    return g

def footprint(parent, bounds, color, projected=False):
    x,y,w,h = rect(bounds)
    add(parent,"rect",dict(x=x,y=y,width=w,height=h,fill="none",stroke="#fffdf7",**{"stroke-width":4}))
    add(parent,"rect",dict(x=x,y=y,width=w,height=h,fill="none",stroke=color,**{"stroke-width":2,"stroke-dasharray":"5 4" if projected else "10 5"}))

def card(parent, key, lines, point, box, color, href):
    x,y,w = box
    px,py = xy(*point)
    endx = x+w if x < MAP_X else x
    mid = endx+7 if x < MAP_X else endx-12
    add(parent,"path",{"d":f"M {px} {py} L {mid} {py} L {mid} {y+26} L {endx} {y+26}","fill":"none","stroke":"#fffdf7","stroke-width":4})
    add(parent,"path",{"d":f"M {px} {py} L {mid} {py} L {mid} {y+26} L {endx} {y+26}","fill":"none","stroke":color,"stroke-width":1.5})
    add(parent,"circle",{"cx":px,"cy":py,"r":4,"fill":color,"stroke":"white","stroke-width":1.5})
    a=add(parent,"a",{"href":href,f"{{{XLINK}}}href":href,"aria-label":f"{key}: {lines[0]}"})
    add(a,"title",text=f"Open local sheet {key}: {lines[0]}")
    add(a,"rect",dict(x=x,y=y,width=w,height=90,rx=8,fill="#fffdf7",stroke=color,**{"stroke-width":1.5}))
    add(a,"text",dict(x=x+12,y=y+25,fill=color,**{"font-size":20,"font-weight":"700"}),key)
    for j,line in enumerate(lines):
        add(a,"text",dict(x=x+12,y=y+46+j*18,fill="#20392f",**{"font-size":14 if j else 16,"font-weight":"700" if j==0 else "400"}),line)

def overview(internal=False):
    root=element("svg",{"viewBox":"0 0 1120 1250","width":"1120","height":"1250","role":"img","aria-labelledby":"atlas-title atlas-desc","style":"font-family:Arial,sans-serif"})
    add(root,"title",{"id":"atlas-title"},"The Horde linked world-map atlas")
    add(root,"desc",{"id":"atlas-desc"},"Original draft regional topography with registered local-sheet extents and numbered links. Dungeon boxes project interior plates; they are not surface terrain. Tomb, treasury and finale have unresolved metric alignment.")
    root.append(copy.deepcopy(original.find(f"{{{NS}}}defs")))
    add(root,"style",text="#region-content text{font-family:'DejaVu Sans',Arial,sans-serif !important}")
    # Matplotlib's clipPaths are at the end of the original SVG.
    for defs in original.findall(f"{{{NS}}}defs")[1:]:root.append(copy.deepcopy(defs))
    add(root,"rect",dict(width=1120,height=1250,fill="#f4f1e8"))
    add(root,"text",dict(x=22,y=36,fill="#20392f",**{"font-size":28,"font-weight":"700"}),"THE HORDE / LINKED WORLD ATLAS")
    add(root,"text",dict(x=22,y=64,fill="#315b39",**{"font-size":16}),"Same draft topography. Numbered sheets show where each local map fits.")
    add(root,"text",dict(x=22,y=89,fill="#5c5b4b",**{"font-size":14}),"Exact registration in the proposal ≠ surveyed or accepted runtime geography. X east · Y north · metres · lake Z0.")
    root.append(plot_group())
    nav=add(root,"g",{"id":"atlas-navigation"})
    for s in SHEETS[:5]:
        footprint(nav,s["bounds"],s["color"],s["number"] in ["03","04","05"])
        target="#"+s["id"] if internal else "sheets/"+s["id"]+".md"
        card(nav,s["number"],s["short"],s["point"],s["label"],s["color"],target)
    for a in APPROACH:
        target="#06-approaches" if internal else "sheets/06-approaches.md"
        card(nav,a["key"],[a["name"],"Plan + profile on sheet 06", "World XY · north up"],a["point"],a["label"],a["color"],target)
    add(root,"text",dict(x=22,y=116,fill="#315b39",**{"font-size":13,"font-weight":"700"}),"LOCAL PLAN VIEWS")
    add(root,"text",dict(x=792,y=116,fill="#315b39",**{"font-size":13,"font-weight":"700"}),"INTERIORS + APPROACHES")
    for i,(key,name) in enumerate([("07-transitions","07 / Crossings + transitions"),("08-progression","08 / Item + boss progression")]):
        href="#"+key if internal else "sheets/"+key+".md"
        a=add(root,"a",{"href":href,f"{{{XLINK}}}href":href})
        add(a,"rect",dict(x=12,y=835+i*58,width=190,height=48,rx=6,fill="#fffdf7",stroke="#768273"))
        add(a,"text",dict(x=21,y=864+i*58,fill="#20392f",**{"font-size":12,"font-weight":"700"}),name)
    add(root,"text",dict(x=12,y=974,fill="#5c5b4b",**{"font-size":13}),"07 / 08 are diagrams;")
    add(root,"text",dict(x=12,y=993,fill="#5c5b4b",**{"font-size":13}),"no extra map footprint.")
    for i,line in enumerate(["Solid terrain and routes: unchanged source. Dashed boxes: local surface-sheet views or projected interior room bounds.",
                             "Approach leaders use authored route stations; sheet 06 gives each inset's full coordinate window and independent scale.",
                             "UNPLACED: tomb interior ↔ rescue rim; dry treasury / finale below or behind Court. No coordinates or footprint inferred.",
                             "Regional bounds: X −150…410, Y −510…440 m (560 × 950 m). North up. Original contours every 5 m.",
                             "Use the Markdown index if SVG links are disabled. HTML viewer provides embedded click-through sheets and return links."]):
        add(root,"text",dict(x=22,y=1140+i*20,fill="#20392f",**{"font-size":14,"font-weight":"700" if i==2 else "400"}),line)
    return root

SVG=overview()
save("overview.svg",ET.tostring(SVG,encoding="unicode"))

def locator(s):
    root=element("svg",{"viewBox":"190 105 600 985","width":"120","height":"197","role":"img","aria-label":"Regional locator for "+s["name"]})
    add(root,"use",{"href":"#region-content"})
    if s["bounds"]:
        footprint(root,s["bounds"],s["color"],s["number"] in ["03","04","05"])
    elif s["number"] in ["06","07"]:
        for a in APPROACH:footprint(root,a["bounds"],a["color"])
    else:
        for p in SHEETS[2:5]:footprint(root,p["bounds"],p["color"],True)
    return ET.tostring(root,encoding="unicode")

style="""
*{box-sizing:border-box}body{margin:0;background:#f4f1e8;color:#20392f;font:17px/1.5 system-ui,sans-serif}
main{max-width:1180px;margin:auto;padding:20px}h1{font-size:clamp(26px,4vw,38px);line-height:1.15}h2{font-size:24px;line-height:1.25}
a{color:#285c75;text-underline-offset:3px}button,.back{font:inherit;display:inline-block;padding:10px 14px;border:1px solid #627164;border-radius:7px;background:#fffdf7;color:#20392f;cursor:pointer}
nav{display:flex;flex-wrap:wrap;gap:9px;margin:20px 0}nav a{background:#fffdf7;border:1px solid #87927b;border-radius:7px;padding:10px 12px;text-decoration:none}
.notice{border-left:5px solid #805716;padding:8px 16px;background:#ede9dc}.map-window{overflow:auto;border:1px solid #87927b;border-radius:10px;background:#f4f1e8}
.map-window>svg{display:block;width:100%;height:auto}.map-window.zoom>svg{width:1120px;max-width:none}.controls{display:flex;flex-wrap:wrap;align-items:center;gap:12px;margin:16px 0}
section{scroll-margin-top:12px;border-top:1px solid #c3c7b7;margin-top:40px;padding-top:20px}.sheet-head{display:flex;gap:24px;align-items:flex-start}.sheet-head>div{flex:1}.sheet-head>.locator{flex:0 0 120px;max-width:25%}.locator svg{width:100%;height:auto}
.sheet-window{overflow:auto;margin:16px 0;background:#e4e5d6;border:1px solid #87927b;border-radius:8px}.sheet-window img{width:100%;height:auto;display:block}.sheet-window.zoom img{width:1600px;max-width:none}
.links{display:flex;gap:15px;flex-wrap:wrap}.small{font-size:14px;color:#586558}footer{margin:40px 0}a:focus-visible,button:focus-visible{outline:3px solid #934821;outline-offset:4px}
@media(max-width:600px){main{padding:12px}.sheet-head{gap:12px}h2{font-size:22px}.sheet-head>.locator{flex-basis:85px}.notice{padding:8px 10px}.map-window{border-radius:5px}}
"""
parts=["<!doctype html><html lang=\"en\"><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><link rel=\"icon\" href=\"data:,\"><title>The Horde linked world atlas</title><style>"+style+"</style><main id=\"overview\">",
       "<h1>The Horde / linked world atlas</h1><p>Start with the whole region, then choose a numbered local sheet. Each locator shows where its view or interior fits.</p>",
       "<p class=\"notice\"><strong>Draft metric blockout.</strong> Registration is exact within the preserved proposal, not accepted runtime geography. Tomb alignment and the treasury/finale remain unresolved. Dungeon boxes project interior rooms; they are not surface topography.</p><nav aria-label=\"Local sheets\">",
       "".join(f'<a href="#{s["id"]}">{s["number"]} / {html.escape(s["name"].split(" / ")[0])}</a>' for s in SHEETS),"</nav>",
       '<div class="controls"><button type="button" data-zoom="regional">Zoom / fit overview</button><span class="small">At phone width, zoom and scroll to read the map. The numbered list stays readable.</span></div>',
       '<div id="regional" class="map-window">'+ET.tostring(overview(True),encoding="unicode")+"</div>"]
for s in SHEETS:
    parts += [f'<section id="{s["id"]}"><div class="sheet-head"><div><h2>{s["number"]} / {html.escape(s["name"])}</h2><p>{html.escape(s["note"])}</p><a class="back" href="#overview">↑ Return to regional overview</a></div><div class="locator">'+locator(s)+'<div class="small">Regional locator</div></div></div>']
    for j,f in enumerate(s["files"]):
        key=f'{s["id"]}-image-{j}'
        data=base64.b64encode(source(f+".webp")).decode()
        parts += [f'<h3>{html.escape(f.split("/")[-1])}</h3><div class="controls"><button type="button" data-zoom="{key}">Zoom / fit local sheet</button></div>',
                  f'<div class="sheet-window" id="{key}"><img src="data:image/webp;base64,{data}" alt="{html.escape(s["name"])}: preserved {html.escape(f.split("/")[-1])} planning sheet"></div>',
                  f'<p class="links"><a href="{url(f+".webp")}">Original image</a><a href="{url(f+".svg")}">Original editable SVG</a><a href="#overview">↑ Regional overview</a></p>']
    parts += ["</section>"]
parts += ['<section id="unresolved"><h2>Unresolved geography</h2><p>The tomb interior has no surveyed transform into the rescue rim coordinates. The regional source ends at the three dungeon plans; treasury/finale dimensions are absent. GC10 provides a proposed post-reveal threshold only. No tomb, treasury or finale footprint is invented.</p><p>Historical village perspective and the oblique world concept are context, not metric sheets. The active PR27 implementation has its own acceptance evidence and is untouched.</p></section>',
          f'<footer><p class="links"><a href="{url("connected/README.md")}">Connected source index</a><a href="{url("dungeons/README.md")}">Dungeon source index</a><a href="{WEB}docs/ROADMAP.md">Preserved planning roadmap</a></p><p class="small">Embedded images: pinned planning commit {PIN}. Terrain/routes preserved from original regional SVG. No network needed to navigate the viewer; source links need internet.</p></footer></main>',
          '<script>document.querySelectorAll("[data-zoom]").forEach(b=>b.addEventListener("click",()=>{const p=document.getElementById(b.dataset.zoom);const z=p.classList.toggle("zoom");b.setAttribute("aria-pressed",String(z));}));</script></html>']
save("atlas.html","\n".join(parts))

readme="""# The Horde / linked world-map atlas

For Sam: use the numbered overview to see **where the local maps fit**, then open their matching sheets below. The original terrain, water and routes are retained from the existing vector regional map; this is a navigation derivative, not a new world plan.

![Regional topography with local-sheet footprints and leaders](overview.webp)

[Full vector overview](overview.svg) · [Self-contained HTML viewer](atlas.html) · [Coordinate and source evidence](coordinate-evidence.json) · [Validation](VALIDATION.md)

GitHub image previews may disable SVG links. Use the following Markdown links there. To use the interactive viewer, download `atlas.html` and open it in a browser (including Android); GitHub's HTML source page does not run the viewer. Its overview, local sheets and return links are embedded and work offline; original-source links require internet. Zoom/fit controls and horizontal scrolling keep detail readable on a phone. No site is deployed.

## Local sheet index

| Overview key | Local sheets | Spatial meaning |
|---|---|---|
"""
for s in SHEETS:
    meaning="Surface view; shared world XY" if s["number"] in ["01","02"] else "Interior room-plate projection; explicit draft translation" if s["bounds"] else "World XY plan insets + profiles" if s["number"]=="06" else "Diagram, no new footprint"
    readme+=f'| {s["number"]}'+(" / 6A–6C" if s["number"]=="06" else "")+f' | [{s["name"]}](sheets/{s["id"]}.md) | {meaning} |\n'
readme+="""
## Alignment, scale and authority

All plans use **X east, Y north, Z up, metres**; north is up. Regional origin is Bellwether well XY(0,0), Z20; Z0 is the proposed Abbey lake, not sea level. Regional view bounds are X −150…410, Y −510…440 m: **560 × 950 m**, with 5 m contours and a 100 m scale bar. This is the plotted draft window, not a fixed total world size.

**Exact within the source model:** forest, arrival, town buildings and water coordinates equal the dungeon generator's retained baseline. Their local plan sheets use these world XY axes directly. The floorplan JSON gives local and translated world coordinates for every room. Dungeon translations have zero rotation: Abbey +(30,−284), Foundry +(230,−40), Court +(306,308). The dashed interior bounds match the source region boxes and union of room plates; they are not terrain, collision, roofs or a survey. Leaf pages record each view's own scale, bounds, elevation and a return link.

**Proposed blockout:** all metric geometry, route grades, water engineering and floor elevations remain proposals. Exact mathematical registration does not promote these to approved runtime geography. Surface-sheet rectangles show the plotted camera window, not a playable boundary. Approach leaders point to existing authored route stations; their full plan windows are recorded on sheet 06 and the coordinate evidence. Sections/profiles are not extra surface maps.

**Unknown / unplaced:** the original tomb interior is not surveyed into the rescue rim coordinates. No tomb footprint is added. Treasury/finale lie below or behind the Court ridge in the established topology, but no precise layout/extent is supplied; GC10 does not prove their coordinates. The historical village perspective and oblique whole-world concept are not metric local sheets. Future region directions remain directions.

The [shared roadmap](../../../ROADMAP.md) and [source inventory](../../../ROADMAP_SOURCES.md) retain scope/status authority. Active PR27 world development, source geometry and game assets are untouched.

## Source custody and rebuild

The source is pinned to planning commit `d37f16381311c6e620f0c4a9d01c1cce83260dc8`, retained on `docs/horde-1.6.2-roadmap`. The overview copies the original regional SVG's map subtree and definitions, adds extents/leaders outside critical routes and changes only its display transform. It never invokes the archived generators or overwrites the originals. Source hashes, exact windows and transforms are in [coordinate evidence](coordinate-evidence.json).

Run `python docs/design/world/atlas/build_atlas.py` from a checkout with that planning Git object. Python's standard library generates the SVG, Markdown sheets, evidence and offline HTML. The WebP overview is a browser render of the vector derivative, not an independently redrawn map. The source generators and all eleven original map images were inspected before placement.

"""
readme+=f'[Original regional SVG]({url("dungeons/01-connected-region.svg")}) · [Original regional image]({url("dungeons/01-connected-region.webp")}) · [Connected source index]({url("connected/README.md")}) · [Dungeon source index]({url("dungeons/README.md")})\n'
save("README.md",readme)
for s in SHEETS:
    md=f'# {s["number"]} / {s["name"]}\n\n[← Regional overview and local sheet index](../README.md) · [Offline viewer](../atlas.html#{s["id"]})\n\n**Overview key {s["number"]}**'+(" (6A Abbey, 6B Foundry, 6C Court)" if s["number"]=="06" else "")+f'. {s["note"]}\n\n'
    md+='Exact registration refers to the draft coordinate model. Metric geometry remains proposed and unaccepted in runtime. '+("The dashed box projects interior room plates; it is not a surface topo map. " if s["number"] in ["03","04","05"] else "")+'[Coordinate evidence](../coordinate-evidence.json).\n\n'
    for f in s["files"]:
        md+=f'## {f.split("/")[-1]}\n\n![{s["name"]}: original preserved planning sheet]({RAW}{PREFIX}{f}.webp)\n\n[Full image]({url(f+".webp")}) · [Editable source SVG]({url(f+".svg")})\n\n'
    md+='[↑ Back to the regional overview](../README.md)\n'
    save(f'sheets/{s["id"]}.md',md)

evidence=dict(source_commit=PIN,base_main_commit="8e1a41ee4d04b3c45e63c872e3ef216b0d5e2b0d",
              status="Navigation derivative; all metric geography draft, no runtime acceptance",
              axes=B["axes"],origin=B["origin"],regional_window_xy=MODEL,regional_contour_interval_m=5,
              original_svg_plot_box=[left,right,top,bottom],original_svg_units_per_m=sx,
              retained_baseline_equal=True,room_translations_xy=origins,room_rotation_degrees=0,
              room_plate_world_bounds=floors,local_sheets=SHEETS,approach_inset_windows=APPROACH,
              unresolved=["No tomb-interior to rescue-rim transform", "No precise treasury/finale floorplan or extent"],
              retained_axes_xml_sha256=hashlib.sha256(ET.tostring(axes)).hexdigest(),sources={})
paths=["connected/shared-coordinate-layout.json","connected/build_maps.py","connected/building-feature-register.csv",
       "connected/README.md","dungeons/01-connected-region.svg","dungeons/01-connected-region.webp",
       "dungeons/build_atlas.py","dungeons/dungeon-layout.json","dungeons/baseline/shared-coordinate-layout.json",
       "dungeons/README.md","dungeons/room-register.csv","dungeons/route-register.csv"]
paths+= [f+ext for s in SHEETS for f in s["files"] for ext in [".webp",".svg"]]
for path in paths:
    data=source(path)
    assert not data.startswith(b"version https://git-lfs.github.com"),f"Need actual image bytes: {path}"
    evidence["sources"][path]=dict(sha256=hashlib.sha256(data).hexdigest(),bytes=len(data),url=url(path))
save("coordinate-evidence.json",json.dumps(evidence,indent=2,ensure_ascii=False)+"\n")
print(f"Generated {len(SHEETS)} linked local pages, preserved SVG overlay and offline viewer in {OUT}")
