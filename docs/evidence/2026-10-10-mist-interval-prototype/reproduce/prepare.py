"""Isolated, over-budget diagnostic. Never freezes/publishes runtime packages.

Run from the existing repository with Python, PowerShell7 and the recorded SDK.
The current production matrix is first reproduced with its supported compiler.
Only copied native sources/temporary generated shaders are changed.
"""
import hashlib,json,re,subprocess,sys,time,struct,math
from pathlib import Path
import numpy as np
root=Path.cwd(); public=root/'docs/evidence/2026-10-10-mist-interval-prototype/reproduce'
out=root/'reports/mist-interval-prototype';sdk=Path('C:/VulkanSDK/1.4.350.0/Bin')
out.mkdir(parents=True,exist_ok=True)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def write(p,s):p=Path(p);p.parent.mkdir(parents=True,exist_ok=True);p.write_text(s,encoding='utf-8',newline='\n')
def once(s,a,b):assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
def run(label,args):
    initial=label;n=2
    while (out/(label+'.private.log')).exists():label=initial+f'-{n}';n+=1
    path=out/(label+'.private.log')
    t=time.perf_counter()
    with path.open('wb') as f:p=subprocess.run([str(x) for x in args],stdout=f,stderr=subprocess.STDOUT)
    text=path.read_text(errors='replace').replace(str(root),'<repo>').replace(root.as_posix(),'<repo>')
    text=re.sub(r'C:[/\\]+Users[/\\]+[^/\\\s"<>]+','<user-home>',text,flags=re.I)
    write(out/(label+'.log'),text)
    print(label,p.returncode,round(time.perf_counter()-t,3),flush=True)
    if p.returncode:print(text[-2500:]);sys.exit(p.returncode)

catalog=json.loads((root/'tools/raygen-variant-catalog.json').read_text())['variants']
budgets={r['key']:r for r in json.loads((root/'tools/raygen-variant-budgets.json').read_text())['budgets']}
if '--native-only' not in sys.argv and '--extras' not in sys.argv:
    if not (out/'baseline-matrix').exists():
        run('baseline-matrix',['pwsh','-NoProfile','-File',root/'tools/compile-raygen.ps1','-Matrix','-OutputDirectory',out/'baseline-matrix'])
    for row in catalog:
        key=row['key'];path=out/'baseline-matrix'/key
        stats=json.loads((path/'raygen-stats.json').read_text())
        assert stats['compiledSpirvSha256']==row['spirvSha256'],key
        for d in stats['dependencies']:
            canonical='\n'.join((root/d['path']).read_text().splitlines())+'\n'
            assert hashlib.sha256(canonical.encode()).hexdigest()==d['sha256'],d['path']

def function(s,name):
    m=re.search(r'(?m)^(?:void|vec[34]|float|bool) '+name+r'\([^;]*?\)\s*\{',s);assert m,name
    a=m.start();b=m.end();n=1
    while n:
        if s[b]=='{':n+=1
        if s[b]=='}':n-=1
        b+=1
    return a,b,s[a:b]

# The actual rope mesh came from the accepted frozen shared simulation, not a
# proxy cylinder. The harness below verifies its entire current mesh exactly.
state=json.loads((root/'docs/evidence/2026-10-10-frozen-mist-attribution/extension/owner-view/current-midpoint-all/frozen-state.json').read_text())
triangles=np.array(state['triangles'],dtype=float).reshape(-1,3,3)
lights=[[-34.35,2.76,-16.02],[-33.05,2.76,-14.38]]
boxmin=np.array([-36.65,-.95,-18.15]);boxmax=np.array([-30.65,.20,-12.25])
def make_cone(a,b,c,light):
    light=np.array(light);normal=np.cross(b-a,c-a)
    if np.linalg.norm(normal)<1e-12:return None
    if np.dot(normal,light-a)<0:normal=-normal
    if abs(np.dot(normal,light-a))<1e-12:return None
    planes=[np.r_[-normal,np.dot(normal,a)]]
    for e,f,other in [(a,b,c),(b,c,a),(c,a,b)]:
        n=np.cross(e-light,f-light)
        if np.dot(n,other-light)<0:n=-n
        planes.append(np.r_[n,-np.dot(n,light)])
    planes=np.array([p/np.linalg.norm(p[:3]) for p in planes])
    # Intersect the convex cone with the actual finite mist box. The extrema
    # of a bounded convex polytope occur at triples of boundary planes.
    pp=list(planes)
    for axis in range(3):
        n=np.eye(3)[axis];pp.extend([np.r_[n,-boxmin[axis]],np.r_[-n,boxmax[axis]]])
    pp=np.array(pp);vertices=[]
    import itertools
    for ids in itertools.combinations(range(10),3):
        m=pp[list(ids),:3]
        if abs(np.linalg.det(m))<1e-10:continue
        p=np.linalg.solve(m,-pp[list(ids),3])
        if np.all(pp[:,:3]@p+pp[:,3]>=-1e-7):vertices.append(p)
    if not vertices:return None
    vertices=np.array(vertices)
    # Conservative float bounds; exact planes still decide interval membership.
    return dict(planes=planes.tolist(),bounds=[(vertices.min(0)-1e-5).tolist(),(vertices.max(0)+1e-5).tolist()])
cones=[];t=time.perf_counter()
for source,light in enumerate(lights):
    for primitive,(a,b,c) in enumerate(triangles):
        cone=make_cone(a,b,c,light)
        if cone:cones.append(dict(source=source,primitive=primitive,**cone))
write(out/'cones.json',json.dumps(dict(casters=len(triangles),cone_count=len(cones),cpu_seconds=time.perf_counter()-t,lights=lights,cones=cones),indent=2)+'\n')
def cppfloat(x):return f'{float(x):.9g}f' if '.' in f'{float(x):.9g}' or 'e' in f'{float(x):.9g}' else f'{float(x):.9g}.0f'
header='#pragma once\nstruct ProbeCone {float planes[4][4];unsigned identity[4];};\n'
header+='inline const VkAabbPositionsKHR kProbeBounds[]={\n'+''.join('{'+','.join(cppfloat(x) for x in c['bounds'][0]+c['bounds'][1])+'},\n' for c in cones)+'};\n'
header+='inline const ProbeCone kProbeCones[]={\n'+''.join('{{'+','.join('{'+','.join(cppfloat(x) for x in p)+'}' for p in c['planes'])+'},{'+f"{c['source']}u,{c['primitive']}u,0u,0u"+'}},\n' for c in cones)+'};\n'
header+='inline const std::array<float,3> kProbeRope[]={\n'+''.join('{'+','.join(cppfloat(x) for x in p)+'},\n' for p in state['triangles'])+'};\n'
write(out/'harness/ProbeConeInput.h',header)

snippet=(public/'intervals.glsl').read_text()
def interval_source(s):
    s=once(s,'float visibilityMask(vec3 origin, vec3 direction,',snippet+'\nfloat visibilityMask(vec3 origin, vec3 direction,')
    # Defined before the earlier Generic helper too: move declarations there.
    s=s.replace(snippet+'\n','',1)
    s=once(s,'#if HORDE_GENERIC_TRANSMISSION_VARIANT\nvec3 boundedShadowTransmittanceMask',snippet+'\n#if HORDE_GENERIC_TRANSMISSION_VARIANT\nvec3 boundedShadowTransmittanceMask')
    s=once(s,'uint shadowFlags = shadowSegmentCrossesTransparentWorld(', 'uint shadowFlags = mistSkyExcludeSelected ? gl_RayFlagsNoOpaqueEXT : shadowSegmentCrossesTransparentWorld(')
    # Range is the exact current renderer-owned rope tail. This narrow diagnostic
    # exclusion runs only in sky-to-medium queries; all surface queries stay off.
    for typename in ['uint','int']:
        marker=f'        {typename} instance = '+('uint(\n            rayQueryGetIntersectionInstanceCustomIndexEXT(query, false));' if typename=='uint' else 'int(rayQueryGetIntersectionInstanceCustomIndexEXT(query, false));')
        # Insert after primitive declaration in both shared strategies.
        prim='        int primitive = rayQueryGetIntersectionPrimitiveIndexEXT(query, false);'
        pos=s.index(marker);q=s.index(prim,pos)+len(prim)
        s=s[:q]+'''\n        if(mistSkyExcludeSelected && instance==0 && primitive>=MIST_ROPE_FIRST && primitive<MIST_ROPE_LAST) continue;'''+s[q:]
    s=once(s,'    vec3 skyDirection;\n    float skyDistance;\n    vec3 skyRadiance;\n    float skyGain;\n    activeSkyLight(midpoint,', '    mistSkyExcludeSelected=true;\n    vec3 skyDirection;\n    float skyDistance;\n    vec3 skyRadiance;\n    float skyGain;\n    activeSkyLight(midpoint,')
    s=once(s,'    float staffStrength = controls.staffLightStrength;', '    mistSkyExcludeSelected=false;\n    float staffStrength = controls.staffLightStrength;')
    s=once(s,'    vec3 incidentLight = sources.skyIncident;', '    vec3 incidentLight = sources.skyIncident * (1.0-mistIntervalCoverage(p));')
    s=once(s,'    MistIncidentSources sources;\n', '''    mistCameraOrigin=rayOrigin;mistCameraDirection=rayDirection;
    discoverMistIntervals(rayOrigin,rayDirection,marchStart,marchEnd,areaShadowSampleIndex());
    if(mistIntervalInvalid) return vec4(1.0,0.0,1.0,0.0);
    MistIncidentSources sources;\n''')
    marker='float stepLength = (marchEnd - marchStart) / sampleCount;'
    assert s.count(marker)==3,'Each Lean/Current/High density branch must set its cell width'
    s=s.replace(marker,marker+'''\n        mistCellLength=stepLength;
        if(!(stepLength>0.0) || isnan(stepLength) || isinf(stepLength)) {
            mistIntervalInvalid=true;return vec4(1.0,0.0,1.0,0.0);
        }''')
    assert s.count('mistCellLength=stepLength;')==3
    s=once(s,'    color = color * lichMist.a + lichMist.rgb;',
           '    if(mistIntervalInvalid) lichMist=vec4(1.0,0.0,1.0,0.0);\n    color = color * lichMist.a + lichMist.rgb;')
    return s

def compile_source(name,s,legacy=False):
    d=out/'shaders'/name;write(d/'source.rgen',s)
    run(name+'-compile',[sdk/'glslangValidator.exe','-V','--target-env','vulkan1.2',*(['-Os'] if legacy else []),'-S','rgen','-o',d/'raw.spv',d/'source.rgen'])
    passes=['-O'] if legacy else ['--eliminate-dead-functions','--eliminate-dead-code-aggressive','--simplify-instructions','--eliminate-dead-branches','--cfg-cleanup']
    run(name+'-optimize',[sdk/'spirv-opt.exe',*passes,d/'raw.spv','-o',d/'shader.spv'])
    run(name+'-validate',[sdk/'spirv-val.exe','--target-env','vulkan1.2',d/'shader.spv'])
    run(name+'-disassemble',[sdk/'spirv-dis.exe',d/'shader.spv','-o',d/'shader.spvasm'])
    asm=(d/'shader.spvasm').read_text();data=(d/'shader.spv').read_bytes();words=struct.unpack('<'+'I'*(len(data)//4),data)
    inc=''.join('    '+', '.join(f'0x{w:08x}u' for w in words[i:i+8])+',\n' for i in range(0,len(words),8))
    write(d/'words.inc',inc);write(d/'src/vulkan/raytracing/variants/diagnostic_high_generic_dielectric.inc',inc)
    row=dict(name=name,source_sha256=sha(d/'source.rgen'),spirv_sha256=sha(d/'shader.spv'),include_sha256=sha(d/'words.inc'),bytes=len(data),words=len(words))
    for key,pattern in [('instructions',r'(?m)^\s*(?:%\S+\s*=\s*)?Op\w+'),('branchOperations',r'\bOp(?:Branch|BranchConditional|Switch)\b'),('loops',r'\bOpLoopMerge\b'),('selectionMerges',r'\bOpSelectionMerge\b'),('functions',r'\bOpFunction\b'),('functionCalls',r'\bOpFunctionCall\b'),('rayQueryInitializations',r'\bOpRayQueryInitializeKHR\b'),('atomicInstructions',r'\bOpAtomic\w+\b')]:row[key]=len(re.findall(pattern,asm))
    return row

# Determine the current rope range from the complete authored renderer output;
# it is checked in the copied renderer before any dispatch. Never guess a stale
# triangle index from the old tomb.
cpp=(root/'src/vulkan/raytracing/PresentableTinyRtScene.cpp').read_text()
# Current renderer reports first3022/last3198, independently checked before
# dispatch. The historical 3028..3203 range was rejected and retained as failure.
first=3022;last=first+len(triangles)
rows=[];matrix=[]
if '--native-only' not in sys.argv and '--extras' not in sys.argv:
    for entry in catalog:
        key=entry['key'];s=(out/'baseline-matrix'/key/'minimal.rgen.resolved').read_text()
        s=interval_source(s);s=s.replace('#version 460',f'#version 460\n#define MIST_ROPE_FIRST {first}\n#define MIST_ROPE_LAST {last}',1)
        row=compile_source(key+'-interval',s,'opaque_fast' in key)
        row['key']=key;row['excess']={k:row[k]-v for k,v in budgets[key]['max'].items() if row[k]>v}
        matrix.append(row)
    write(out/'full-metrics.json',json.dumps(matrix,indent=2)+'\n')
    base=(out/'baseline-matrix/diagnostic_high_generic_dielectric/minimal.rgen.resolved').read_text()
    corrected=(out/'shaders/diagnostic_high_generic_dielectric-interval/source.rgen').read_text()
    rows=[dict(matrix[next(i for i,r in enumerate(matrix) if r['key']=='diagnostic_high_generic_dielectric')])]
    rows[0]['name']='diagnostic_high_generic_dielectric-interval'
    # Baseline package itself is used directly and checked against its catalogue.
    rows.append(compile_source('baseline',base))
    assert rows[-1]['spirv_sha256']==next(r for r in catalog if r['key']=='diagnostic_high_generic_dielectric')['spirvSha256']
    for label,s in [('interval-surface',corrected),('baseline-surface',base),('depth',base)]:
        a=s.index('    vec4 lichMist = lichGroundMist(')
        b=s.index('    imageStore(outputImage,',a)
        s=s[:a]+s[b:]
        if label=='depth':
            s=once(s,'    imageStore(outputImage,','    uint bits=floatBitsToUint(primary.t);\n    imageStore(outputImage,')
            s=once(s,'vec4(clamp(color, 0.0, 1.0), 1.0));','vec4(float(bits&255u),float((bits>>8u)&255u),float((bits>>16u)&255u),float((bits>>24u)&255u))/255.0);')
        rows.append(compile_source(label,s))
    # Exact interval coverage output has actual hardware discovery and actual
    # primary depth. It is evidence, not a replacement screen-space renderer.
    s=corrected
    s=once(s,'    imageStore(outputImage,','    float coverage=0.0;for(int i=0;i<mistIntervalCount;++i) coverage+=mistIntervals[i].y-mistIntervals[i].x;\n    uint bits=floatBitsToUint(mistIntervalInvalid ? -2.0 : coverage);\n    imageStore(outputImage,')
    s=once(s,'vec4(clamp(color, 0.0, 1.0), 1.0));','vec4(float(bits&255u),float((bits>>8u)&255u),float((bits>>16u)&255u),float((bits>>24u)&255u))/255.0);')
    rows.append(compile_source('interval-coverage',s))
    write(out/'shader-metrics.json',json.dumps(rows,indent=2)+'\n')
else:rows=json.loads((out/'shader-metrics.json').read_text())
if '--extras' in sys.argv:
    assert len(rows)==6,'Extras are generated once; preserve previous attempts'
    corrected=(out/'shaders/diagnostic_high_generic_dielectric-interval/source.rgen').read_text()
    s=once(corrected,'float mistCellLength=0.0;','float mistCellLength=0.0;\nfloat mistLastCoverage=0.0;vec2 mistProbeWeights=vec2(0.0);')
    s=once(s,'    vec3 incidentLight = sources.skyIncident * (1.0-mistIntervalCoverage(p));','    mistLastCoverage=mistIntervalCoverage(p);\n    vec3 incidentLight = sources.skyIncident * (1.0-mistLastCoverage);')
    s=once(s,'    scattered += transmittance * sampleValue.rgb * stepLength;','    mistProbeWeights+=vec2(1.0,mistLastCoverage)*(transmittance*sampleValue.a*.82*stepLength);\n    scattered += transmittance * sampleValue.rgb * stepLength;')
    s=once(s,'    imageStore(outputImage,','    uint bits=floatBitsToUint(mistProbeWeights.x>0.0 ? mistProbeWeights.y/mistProbeWeights.x : -1.0);\n    imageStore(outputImage,')
    s=once(s,'vec4(clamp(color, 0.0, 1.0), 1.0));','vec4(float(bits&255u),float((bits>>8u)&255u),float((bits>>16u)&255u),float((bits>>24u)&255u))/255.0);')
    rows.append(compile_source('weighted-coverage',s))
    s=(out/'shaders/interval-coverage/source.rgen').read_text().replace('kMistCandidateCapacity=512','kMistCandidateCapacity=1',1)
    rows.append(compile_source('overflow-negative',s))
    write(out/'shader-metrics.json',json.dumps(rows,indent=2)+'\n')

# Reuse the game-owned frozen capture recipe, copied into a new isolated output.
# All edits below affect the diagnostic copies only.
old=(root/'docs/evidence/2026-10-10-frozen-mist-attribution/reproduce/prepare.py').read_text()
tail=old[old.index("h=out/'harness'"):]
tail=tail.replace(".cameraX = -33.0f, .cameraZ = -17.5f, .yaw = -2.805f, .pitch = -0.24f,", ".cameraX = -34.4f, .cameraZ = -13.5f, .yaw = 0.10f, .pitch = -0.24f,",1)
tail=tail.replace('.rewardPose=DevelopmentRewardPose::HeldHigh}', '.rewardPose=DevelopmentRewardPose::HeldLow}',1)
tail=tail.replace('input.yawRadians=checkpoint.yaw;input.pitchRadians=checkpoint.pitch;input.torchLightStrength=0;', 'input.yawRadians=checkpoint.yaw;input.pitchRadians=checkpoint.pitch;input.torchLightStrength=0;\n if(const char* yaw=std::getenv("HORDE_MIST_VIEW_YAW")) input.yawRadians+=std::strtof(yaw,nullptr);')
tail=tail.replace('RtPipelineVariantArtifact,6',f'RtPipelineVariantArtifact,{len(rows)}').replace("text[0]>'5'",f"text[0]>'{len(rows)-1}'")
tail=tail.replace('context.shadowQuality = horde::graphics::ShadowQuality::Higher;', 'context.shadowQuality = std::getenv("HORDE_MIST_PROBE_SHADOW") != nullptr && std::string_view(std::getenv("HORDE_MIST_PROBE_SHADOW")) == "CURRENT" ? horde::graphics::ShadowQuality::Current : horde::graphics::ShadowQuality::Higher;',1)
tail=tail.replace("window='#include", "window='#include <cstdlib>\\n#include",1)
tail=tail.replace("write(h/'CMakeLists.txt',c)","c+='target_sources(frozen_mist PRIVATE ProbeScene.cpp ProbeContracts.cpp)\\n'\nwrite(h/'CMakeLists.txt',c)")
tail=tail.replace("write(h/'DiagnosticWindow.cpp',window)","window=window.replace('static bool StageFrozenMist(', 'bool StageFrozenMist(',1)\nwrite(h/'DiagnosticWindow.cpp',window)")
tail=tail.replace("run('configure',", "prepare_native(h)\nrun('configure',")
cache=(root/'build/presets/windows-x64-debug/CMakeCache.txt').read_text()
cmake=next(x.split('=',1)[1] for x in cache.splitlines() if x.startswith('CMAKE_COMMAND:INTERNAL='))
record=json.loads((out/'baseline-matrix/diagnostic_high_generic_dielectric/raygen-stats.json').read_text())
def prepare_native(h):
    # Native clone implements extra diagnostic AS ownership only. Production
    # world geometry, masks, surface shadow helpers and ABI remain unchanged.
    header=(root/'src/vulkan/raytracing/PresentableTinyRtScene.h').read_text()
    header=once(header,'    bool BuildAccelerationStructures(std::string& diagnostic);','    bool BuildAccelerationStructures(std::string& diagnostic);\n    bool BuildMistConeDiagnostic(std::string& diagnostic);\n    Buffer probeAabbs_,probePlanes_,probeInstances_;\n    AccelerationStructure probeBlas_,probeTlas_;')
    write(h/'vulkan/raytracing/PresentableTinyRtScene.h',header)
    s=cpp
    # Vulkan/types precede generated input data.
    s=once(s,'#include "vulkan/raytracing/PresentableTinyRtScene.h"','#include "vulkan/raytracing/PresentableTinyRtScene.h"\n#include "ProbeConeInput.h"\n#include "gameplay/simulation/GameSimulation.h"\n#include "gameplay/DevelopmentCheckpoints.h"\n#include "gameplay/DevelopmentCheckpointSimulation.h"\n#include <fstream>')
    s=once(s,'    physicalDevice_ = std::exchange(other.physicalDevice_, nullptr);','    probeAabbs_=std::exchange(other.probeAabbs_,{});probePlanes_=std::exchange(other.probePlanes_,{});probeInstances_=std::exchange(other.probeInstances_,{});\n    probeBlas_=std::exchange(other.probeBlas_,{});probeTlas_=std::exchange(other.probeTlas_,{});\n    physicalDevice_ = std::exchange(other.physicalDevice_, nullptr);')
    s=once(s,'    DestroyAccelerationStructure(tlas_);','    DestroyAccelerationStructure(probeTlas_);DestroyAccelerationStructure(probeBlas_);\n    DestroyBuffer(probeInstances_);DestroyBuffer(probeAabbs_);DestroyBuffer(probePlanes_);\n    DestroyAccelerationStructure(tlas_);')
    s=once(s,'            return BuildAccelerationStructures(diagnostic);','            return BuildAccelerationStructures(diagnostic) && BuildMistConeDiagnostic(diagnostic);')
    # Layout/pool/write extensions are explicit diagnostic resources; preserve
    # validation of the production write roster before adding them.
    s=once(s,'    const VkDescriptorSetLayoutCreateInfo layoutInfo{','    std::vector<VkDescriptorSetLayoutBinding> probeBindings(bindings->values.begin(),bindings->values.begin()+bindings->count);\n    probeBindings.push_back({28u,VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,1u,executionPolicy_.shaderStage,nullptr});\n    probeBindings.push_back({29u,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1u,executionPolicy_.shaderStage,nullptr});\n    const VkDescriptorSetLayoutCreateInfo layoutInfo{')
    s=once(s,'        bindings->count, bindings->values.data()};','        static_cast<unsigned>(probeBindings.size()), probeBindings.data()};')
    s=once(s,'{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1u},','{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 2u},')
    s=once(s,'{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, contract.storageBufferDescriptorCount},','{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, contract.storageBufferDescriptorCount+1u},')
    s=once(s,'    vkUpdateDescriptorSets(device_, static_cast<std::uint32_t>(writes.size()), writes.data(), 0u, nullptr);', '''    VkWriteDescriptorSetAccelerationStructureKHR coneAs{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
    coneAs.accelerationStructureCount=1;coneAs.pAccelerationStructures=&probeTlas_.handle;
    VkWriteDescriptorSet coneWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};coneWrite.pNext=&coneAs;
    coneWrite.dstSet=descriptorSet;coneWrite.dstBinding=28;coneWrite.descriptorCount=1;coneWrite.descriptorType=VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    VkDescriptorBufferInfo coneInfo{probePlanes_.buffer,0,probePlanes_.size};
    writes.push_back(coneWrite);writes.push_back(bufferWrite(29u,&coneInfo));
    vkUpdateDescriptorSets(device_, static_cast<std::uint32_t>(writes.size()), writes.data(), 0u, nullptr);''')
    s+= '\n'+(public/'native-cones.cpp').read_text()
    write(h/'ProbeScene.cpp',s)
    # A copied, explicitly extended diagnostic roster. Validate the original
    # production roster in full, plus an all-or-nothing28/29 pair. Never remove
    # required bindings or bypass module hashes/atomic/entry-point checks.
    contract=(root/'src/vulkan/raytracing/RtPipelineBundleContracts.cpp').read_text()
    contract=once(contract,'    if (descriptorIo.bindingCount > descriptorIo.bindings.size() ||', '''    const auto end=reflected.descriptorBindings.begin()+reflected.descriptorBindingCount;
    const bool coneAs=std::find(reflected.descriptorBindings.begin(),end,28u)!=end;
    const bool coneData=std::find(reflected.descriptorBindings.begin(),end,29u)!=end;
    if(coneAs!=coneData) return false;
    const std::size_t extra=coneAs ? 2u : 0u;
    if (descriptorIo.bindingCount > descriptorIo.bindings.size() ||''')
    contract=once(contract,'reflected.descriptorBindingCount > descriptorIo.bindingCount)', 'reflected.descriptorBindingCount > descriptorIo.bindingCount+extra)')
    contract=once(contract,'std::array<std::uint32_t, std::tuple_size_v<decltype(RtDescriptorIoContract::bindings)>> expected{};', 'std::array<std::uint32_t,32u> expected{};')
    contract=once(contract,'    if (reflected.descriptorBindingCount != expectedCount) return false;', '    if(extra) {expected[expectedCount++]=28u;expected[expectedCount++]=29u;}\n    if (reflected.descriptorBindingCount != expectedCount) return false;')
    write(h/'ProbeContracts.cpp',contract)
exec(tail)
