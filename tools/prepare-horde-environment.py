"""Hash-admit an existing owned Briarhold panorama to bounded native KTX tiers."""
import argparse
import hashlib
import json
import shutil
import subprocess
from pathlib import Path
from PIL import Image

parser=argparse.ArgumentParser()
parser.add_argument("--briarhold",type=Path,required=True)
parser.add_argument("--output",type=Path,required=True)
args=parser.parse_args()
provenance=json.loads((args.briarhold/"assets/world/briarhold-storm-sky-provenance.json").read_text())
for kind in ["source","runtime"]:
    original=args.briarhold/provenance[kind]["path"]
    if hashlib.sha256(original.read_bytes()).hexdigest()!=provenance[kind]["sha256"]:
        raise ValueError("Owned upstream panorama hash mismatch")
if provenance["rights"]!="Project-owned generated asset.":
    raise ValueError("Upstream rights record does not match inspected owner inventory")
source=args.output/"source"; runtime=args.output/"runtime"
source.mkdir(parents=True,exist_ok=True); runtime.mkdir(parents=True,exist_ok=True)
shutil.copyfile(args.briarhold/provenance["source"]["path"],source/"briarhold-storm-sky-imagegen-v1.png")
shutil.copyfile(args.briarhold/"assets/world/briarhold-storm-sky-provenance.json",source/"briarhold-provenance.json")
image=Image.open(args.briarhold/provenance["runtime"]["path"]).convert("RGBA")
image.resize((512,256),Image.Resampling.LANCZOS).save(source/"night-storm-512.png")
records=[]
for platform,format in [("windows","R8G8B8A8_SRGB"),("android","ASTC_6x6_SRGB_BLOCK")]:
    path=runtime/("night-storm."+platform+".ktx2")
    command=["ktx","create","--testrun","--format",format,"--width","512","--height","256","--generate-mipmap","--assign-tf","srgb",str(source/"night-storm-512.png"),str(path)]
    subprocess.run(command,check=True)
    subprocess.run(["ktx","validate",str(path)],check=True)
    records.append({"platform":platform,"path":path.name,"format":format,"dimensions":[512,256],"mipLevels":10,"sha256":hashlib.sha256(path.read_bytes()).hexdigest(),"bytes":path.stat().st_size})
manifest={"schema":1,"status":"admitted native-miss-radiance input; visual/device acceptance pending","rights":provenance["rights"],"sourceProject":"Samfa12 Briarhold","sourceSha256":provenance["source"]["sha256"],"intermediateSha256":provenance["runtime"]["sha256"],"processing":"Pillow Lanczos512x256 from owned1024x512WebP; KTX-Software4.4.2 generated10mips; RGBA8SRGB Windows and strict ASTC6x6SRGBAndroid; no supercompression.","colorSpace":"sRGB; sampled linear by Vulkan","sampler":"U repeat, V clamp, mipmapped; same direction-based native miss/glass/reflection shading","lighting":"Environment appearance/radiance, no baked scene illumination or added light; accepted moon and finale dawn policy retained","runtime":records}
(runtime/"asset.manifest.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
print(json.dumps(records,indent=2))
