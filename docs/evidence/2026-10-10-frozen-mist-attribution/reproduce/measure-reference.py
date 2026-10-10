import hashlib,json,re,subprocess,time
from pathlib import Path
root=Path.cwd();out=root/'reports/frozen-mist/matrix-reference';out.mkdir(parents=True,exist_ok=True)
sdk=Path('C:/VulkanSDK/1.4.350.0/Bin')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def run(name,args):
 t=time.perf_counter()
 with (out/(name+'.private.log')).open('wb') as log:p=subprocess.run([str(a) for a in args],stdout=log,stderr=subprocess.STDOUT)
 assert p.returncode==0,(name,p.returncode)
 return time.perf_counter()-t
def function(s,name):
 m=re.search(r'(?m)^(?:void|vec[34]|float|bool) '+name+r'\([^;]*?\)\s*\{',s);assert m,name
 start=m.start();i=m.end();depth=1
 while depth:
  if s[i]=='{':depth+=1
  if s[i]=='}':depth-=1
  i+=1
 return start,i,s[start:i]


budgets={b['key']:b for b in json.loads((root/'tools/raygen-variant-budgets.json').read_text())['budgets']}
catalog={b['key']:b for b in json.loads((root/'tools/raygen-variant-catalog.json').read_text())['variants']}
rows=[]
for path in sorted((root/'reports/frozen-mist/matrix-baseline').glob('*/raygen-stats.json')):
 baseline=json.loads(path.read_text());key=baseline['key'];assert baseline['compiledSpirvSha256']==catalog[key]['spirvSha256']
 d=out/key;d.mkdir(exist_ok=True);s=(path.parent/'minimal.rgen.resolved').read_text()
 a='vec4 lichMistSample(vec3 p, MistIncidentSources sources)\n{';assert s.count(a)==1;s=s.replace(a,a+'\n    buildMistIncidentSources(p,sources);')
 a='    buildMistIncidentSources(rayOrigin + rayDirection *\n        (0.5 * (marchStart + marchEnd)), sources);';assert s.count(a)==1;s=s.replace(a,'    // Sample-local diagnostic reference, no production admission.')
 (d/'source.rgen').write_text(s,encoding='utf-8',newline='\n')
 args=[sdk/'glslangValidator.exe','-V','--target-env','vulkan1.2']
 if baseline['strategy']=='LegacyInlined':args+=['-Os']
 run(key+'-compile',args+['-S','rgen','-o',d/'unoptimized.spv',d/'source.rgen'])
 args=[sdk/'spirv-opt.exe']+(['-O'] if baseline['strategy']=='LegacyInlined' else ['--eliminate-dead-functions','--eliminate-dead-code-aggressive','--simplify-instructions','--eliminate-dead-branches','--cfg-cleanup'])
 run(key+'-optimize',args+[d/'unoptimized.spv','-o',d/'shader.spv'])
 run(key+'-validate',[sdk/'spirv-val.exe','--target-env','vulkan1.2',d/'shader.spv'])
 run(key+'-disassemble',[sdk/'spirv-dis.exe',d/'shader.spv','-o',d/'shader.spvasm'])
 asm=(d/'shader.spvasm').read_text();data=(d/'shader.spv').read_bytes()
 candidate=dict(bytes=len(data),words=len(data)//4,instructions=len(re.findall(r'^\s*(?:%\S+\s*=\s*)?Op\w+',asm,re.M)),branchOperations=len(re.findall(r'\bOp(?:Branch|BranchConditional|Switch)\b',asm)),loops=asm.count('OpLoopMerge'),selectionMerges=asm.count('OpSelectionMerge'),functions=len(re.findall(r'\bOpFunction\b',asm)),functionCalls=asm.count('OpFunctionCall'),rayQueryInitializations=asm.count('OpRayQueryInitializeKHR'),atomicInstructions=len(re.findall(r'\bOpAtomic\w+\b',asm)),hasDiagnosticsBinding=bool(re.search(r'\bOpDecorate\s+%\S+\s+Binding\s+22\b',asm)),strategy=baseline['strategy'],instrumentation=baseline['instrumentation'],quality=baseline['quality'],material=baseline['material'],shippingAllowed=baseline['instrumentation']=='Shipping')
 candidate['driverSafeFullyInlined']=candidate['functions']==1 and candidate['functionCalls']==0 and candidate['rayQueryInitializations']<=29
 candidate['boundedGenericFunctionsRetained']=candidate['functions']>1 and candidate['functionCalls']>0 and candidate['rayQueryInitializations']<=3
 failures=[dict(metric=m,measured=candidate[m],maximum=v) for m,v in budgets[key]['max'].items() if candidate[m]>v]
 exactFailures=[dict(field=m,measured=candidate[m],expected=v) for m,v in budgets[key]['exact'].items() if candidate[m]!=v]
 r=dict(key=key,baseline=baseline,candidate=candidate,source_sha256=sha(d/'source.rgen'),spirv_sha256=sha(d/'shader.spv'),disassembly_sha256=sha(d/'shader.spvasm'),violations=failures,exact_violations=exactFailures)
 rows.append(r);print(key,candidate['bytes'],candidate['rayQueryInitializations'],len(failures),len(exactFailures),flush=True)
(out/'full-metrics.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8',newline='\n')
assert len(rows)==8
