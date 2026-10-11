"""Check atlas source registration, bounds, local links and pinned remote paths.

Use --remote for an HTTP HEAD check of every emitted GitHub/raw source URL.
The large data URI images are parsed as HTML, not searched as free text.
"""
from pathlib import Path
from html.parser import HTMLParser
from urllib.parse import urlsplit, unquote
from urllib.request import Request, urlopen
import argparse
import concurrent.futures
import hashlib
import json
import re
import subprocess
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[3]
SVG = "{http://www.w3.org/2000/svg}"
ET.register_namespace("", "http://www.w3.org/2000/svg")
ET.register_namespace("xlink", "http://www.w3.org/1999/xlink")
parser=argparse.ArgumentParser()
parser.add_argument("--remote",action="store_true")
args=parser.parse_args()
e=json.loads((ROOT/"coordinate-evidence.json").read_text(encoding="utf-8"))
pin=e["source_commit"]
paths=set(subprocess.check_output(["git","ls-tree","-r","--name-only",pin],cwd=REPO,text=True).splitlines())
remote=set()
links=[]
failures=[]

class Page(HTMLParser):
    def __init__(self):
        super().__init__();self.ids=set();self.links=[];self.images=0
    def handle_starttag(self,tag,attrs):
        a=dict(attrs)
        if "id" in a:
            if a["id"] in self.ids:failures.append("Duplicate HTML id: "+a["id"])
            self.ids.add(a["id"])
        for key in ["href","src"]:
            if key in a and not a[key].startswith("data:"):self.links.append(a[key])
        if tag=="img" and a.get("src","").startswith("data:image/webp;base64,"):self.images+=1

page=Page();page.feed((ROOT/"atlas.html").read_text(encoding="utf-8"))
assert page.images==10
links += [(ROOT/"atlas.html",u) for u in page.links]
for p in ROOT.rglob("*.md"):
    text=p.read_text(encoding="utf-8")
    links += [(p,u) for u in re.findall(r"\]\(([^)]+)\)",text)]
root=ET.parse(ROOT/"overview.svg").getroot()
links += [(ROOT/"overview.svg",a.get("href")) for a in root.iter(SVG+"a")]
for record in e["sources"].values():remote.add(record["url"])
for parent,u in links:
    v=urlsplit(u)
    if v.scheme in ["https","http"]:remote.add(u);continue
    if not v.path:
        if v.fragment not in page.ids and parent.suffix==".html":failures.append("Missing fragment "+u)
        continue
    target=(parent.parent/unquote(v.path)).resolve()
    if not target.exists():failures.append(f"Missing local file {parent.name}: {u}")
    if target.name=="atlas.html" and v.fragment and v.fragment not in page.ids:failures.append("Missing viewer target "+u)

for u in remote:
    v=urlsplit(u)
    if v.netloc=="github.com":match=re.fullmatch(r"/Samfa12-tech/The-Horde-RT-demo/blob/([0-9a-f]{40})/(.+)",v.path)
    elif v.netloc=="raw.githubusercontent.com":match=re.fullmatch(r"/Samfa12-tech/The-Horde-RT-demo/([0-9a-f]{40})/(.+)",v.path)
    else:failures.append("Unexpected remote source "+u);continue
    if not match or match[1]!=pin or match[2] not in paths:failures.append("Unresolved pinned source "+u)

actual=root.find(f".//{SVG}g[@id='axes_1']")
assert hashlib.sha256(ET.tostring(actual)).hexdigest()==e["retained_axes_xml_sha256"],"Terrain/map XML subtree altered"
for s in e["local_sheets"]:
    if s["bounds"]:
        x0,x1,y0,y1=s["bounds"]
        assert -150<=x0<x1<=410 and -510<=y0<y1<=440
for a in e["approach_inset_windows"]:
    x0,x1,y0,y1=a["bounds"]
    assert -150<=x0<x1<=410 and -510<=y0<y1<=440
for path,record in e["sources"].items():
    data=subprocess.check_output(["git","show",f"{pin}:docs/design/world/{path}"],cwd=REPO)
    assert hashlib.sha256(data).hexdigest()==record["sha256"]

results={}
if args.remote:
    def check(u):
        try:
            with urlopen(Request(u,method="HEAD",headers={"User-Agent":"HordeAtlas-LinkCheck"}),timeout=25) as response:return u,response.status
        except Exception as ex:return u,str(ex)
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        for u,status in pool.map(check,sorted(remote)):
            results[u]=status
            if status!=200:failures.append(f"Remote {status}: {u}")
    (ROOT/"remote-link-check.json").write_text(json.dumps(results,indent=2)+"\n",encoding="utf-8",newline="\n")
print(json.dumps(dict(local_links=len(links),unique_remote_links=len(remote),embedded_local_images=page.images,
                     source_files=len(e["sources"]),map_subtree_preserved=True,within_regional_bounds=True,
                     http_checked=args.remote,failures=failures),indent=2))
if failures:raise SystemExit(1)
