import json,hashlib,subprocess,re,xml.etree.ElementTree as ET
from pathlib import Path
root=Path.cwd(); out=root/'reports/wp2a-combat'; base='8bb62ede19b0a06756b89abc9812d4828c61a280'
candidate='262a7fc096876d2417471285c6809f631103cfe6'
def git(*args):return subprocess.check_output(['git',*args],text=True).strip()
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def identity(p):
 path=root/p
 return dict(path=p,bytes=path.stat().st_size,working_sha256=sha(path),git_blob=git('rev-parse',candidate+':'+p),
  url='https://github.com/Samfa12-tech/The-Horde-RT-demo/blob/'+candidate+'/'+p)
changed=git('diff','--name-only',base,candidate).splitlines()
protected=git('ls-files','shaders/*','src/vulkan/raytracing/*Shader.inc',
 'src/vulkan/raytracing/variants/*','src/vulkan/raytracing/Rt*VariantCatalog.generated.h',
 'src/vulkan/raytracing/RtSceneAbi*','tools/raygen-*','tools/rayquery-*',
 'tools/GenerateRt*','tools/GenerateRayQuery*','tools/compile-raygen.ps1').splitlines()
assert not set(changed)&set(protected)
assert not git('diff','--name-only',base,candidate,'--','assets','ASSET_LICENSES.md',
 'tests/FinalHeldTorchClearanceTests.cpp','docs/evidence','docs/superpowers/plans')
assets=['assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb',
 'assets/models/player/runtime/gothic-traveller-lod0.runtime.glb',
 'assets/models/weapons/runtime/gothic-arming-sword-rh-lod0.runtime.glb',
 'assets/models/props/runtime/gothic-hand-torch-lod0.runtime.glb']
inputs=[]
for p in assets:
 item=identity(p); pointer=subprocess.check_output(['git','show',candidate+':'+p])
 match=re.search(rb'oid sha256:([0-9a-f]{64})',pointer)
 item['lfs_oid']=match.group(1).decode() if match else None
 item['hydrated_payload_matches_lfs']=bool(match and item['working_sha256']==item['lfs_oid'])
 assert not match or item['hydrated_payload_matches_lfs']
 inputs.append(item)
recorded=json.loads((root/'docs/evidence/2026-10-09-wp2-shader-budget-admission/recorded-toolchain-verified.json').read_text())
toolchains=[]
for item in recorded['tools']:
 name=Path(item['path'].replace('\\','/')).name; p=Path('C:/VulkanSDK/1.4.350.0/Bin')/name
 measured=sha(p); assert measured==item['sha256']
 toolchains.append(dict(tool=name,sha256=measured,version=subprocess.check_output([str(p),'--version'],text=True)))
psversion=subprocess.check_output(['pwsh','-NoProfile','-Command','$PSVersionTable.PSVersion.ToString()'],text=True).strip()
assert psversion.startswith('7.')
native=[]
for config,label in [('debug','candidate-debug-packages'),('release','final-release-packages')]:
 text=(out/(label+'.log')).read_text()
 objects=[]
 for line in text.splitlines():
  content=re.sub(r'^\d+:\s*','',line)
  if content.startswith('{'):
   try:obj=json.loads(content)
   except ValueError:continue
   if obj.get('targetPlatform')=='Windows':objects.append(obj)
 assert len(objects)==1
 obj=objects[0]; exe=root/f'build/presets/windows-x64-{config}/{config.title()}/HordeLanternRT.exe'
 assert obj['targetSha256']==sha(exe)
 obj['target']=exe.relative_to(root).as_posix(); native.append(obj)
android=json.loads((out/'android-all-abi-identities.json').read_text()); assert android['source_commit']==candidate
arm64=[]
for config in ['debug','release']:
 obj=json.loads((out/f'final-android-{config}-arm64-package.log').read_text().strip())
 obj['apk']='android/app/build/outputs/apk/'+('debug/app-debug.apk' if config=='debug' else 'release/app-release-unsigned.apk')
 obj['packaged']['target']=obj['apk']+'!/'+obj['apkEntry']
 obj['stripped']['target']=obj['stripped']['target'].replace('<repo>\\','').replace('\\','/')
 arm64.append(obj)
tests=[]
for p in sorted((root/'android/app/build/test-results/testDebugUnitTest').glob('TEST-*.xml')):
 suite=ET.parse(p).getroot(); tests.append({k:suite.attrib[k] for k in ['name','tests','failures','errors','skipped','time']})
assert tests and all(int(t['failures'])==int(t['errors'])==int(t['skipped'])==0 for t in tests)
result=dict(start_commit=base,source_commit=candidate,source_milestones=['759fd593bef99746f3701bde5476e3431e5b4204',candidate],
 changed_source_paths=[identity(p) for p in changed],unchanged_shader_policy_abi_and_generated=[identity(p) for p in protected],
 input_payloads=inputs,shader_toolchain=toolchains,powershell_version=psversion,
 windows_packages=native,android_packages=android,android_arm64_shader_inspection=arm64,java_tests=tests,
 java_total=sum(int(t['tests']) for t in tests),
 limits='Host compilation/package inspection only. No GPU presentation, Android installation, touch/lifecycle, listening, vibration or performance acceptance.')
(out/'provenance.json').write_text(json.dumps(result,indent=2)+'\n')
print('Source, immutable shader/input identities, native package identities and Java result counts verified.')
