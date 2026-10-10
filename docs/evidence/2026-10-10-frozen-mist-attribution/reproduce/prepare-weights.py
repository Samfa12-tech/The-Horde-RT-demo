import hashlib,json,re,subprocess,sys,time
from pathlib import Path
root=Path.cwd(); original=root/'reports/frozen-mist'
family='owner-midpoint-rope' if '--midpoint-rope-only' in sys.argv else ('owner-weights' if '--owner-view' in sys.argv else 'weights')
out=original/family
out.mkdir(exist_ok=True)
base=(original/'prepare-records.py').read_text()
prefix=base[:base.index('rows=json.loads')]
prefix=prefix.replace("out=root/'reports/frozen-mist/records';", "out=root/'reports/frozen-mist/weights';",1)
prefix=prefix.replace("out=root/'reports/frozen-mist/weights';", f"out=root/'reports/frozen-mist/{family}';",1)
prefix=prefix.replace("baseline=out/'baseline/diagnostic_high_generic_dielectric'", "baseline=root/'reports/frozen-mist/records/baseline/diagnostic_high_generic_dielectric'",1)
prefix=prefix.replace("if not (out/'baseline/diagnostic_high_generic_dielectric/minimal.rgen.resolved').exists():", "if not (root/'reports/frozen-mist/records/baseline/diagnostic_high_generic_dielectric/minimal.rgen.resolved').exists():",1)
exec(prefix)
rows=[]
cases=[('midpoint','rope-loss')] if '--midpoint-rope-only' in sys.argv else [('midpoint','loss'),('sample-local','loss'),('sample-local','rope-loss'),('sample-local','masks')]
for policy, measurement in cases:
 name=policy+'-'+measurement; d=out/'shaders'/name; d.mkdir(parents=True,exist_ok=True)
 s=source
 a,b,f=function(s,'buildMistIncidentSources')
 f=f[:f.index('    float staffStrength =')]+'''    mistProbeActive=false;
    for(uint i=0u;i<kRtActiveFireEmitterCapacity;++i) {sources.firePositions[i]=vec3(0.0);sources.fireVisibleRadiances[i]=vec3(0.0);}
}'''
 f=once(f,'    if (!isnan(skyGain) &&','    mistProbeActive=true;mistProbeInstance=255u;mistProbePrimitive=65535u;\n    mistProbeRaw=skyRadiance*skyGain;\n    if (!isnan(skyGain) &&')
 f=once(f,'    mistProbeActive=false;', '    mistProbeVisible=sources.skyIncident;\n    mistProbeActive=false;')
 s=s[:a]+f+s[b:]
 s=once(s,'vec3 boundedShadowTransmittanceMask(vec3 origin, vec3 direction,', '''bool mistProbeActive=false;
uint mistProbeInstance=255u, mistProbePrimitive=65535u;
vec3 mistProbeRaw=vec3(0.0),mistProbeVisible=vec3(0.0);
vec3 mistProbeEnergy=vec3(0.0);
uint mistProbeCount=0u,mistProbeRopeMask=0u,mistProbeOpaqueMask=0u;
vec3 boundedShadowTransmittanceMask(vec3 origin, vec3 direction,''')
 s=once(s,'    if (rayQueryGetIntersectionTypeEXT(query, true) !=\n        gl_RayQueryCommittedIntersectionNoneEXT)\n        return vec3(0.0);', '''    if (rayQueryGetIntersectionTypeEXT(query, true) !=
        gl_RayQueryCommittedIntersectionNoneEXT) {
        if(mistProbeActive) {mistProbeInstance=uint(rayQueryGetIntersectionInstanceCustomIndexEXT(query,true));mistProbePrimitive=uint(rayQueryGetIntersectionPrimitiveIndexEXT(query,true));}
        return vec3(0.0);
    }''')
 if policy=='sample-local':
  s=once(s,'vec4 lichMistSample(vec3 p, MistIncidentSources sources)\n{','vec4 lichMistSample(vec3 p, MistIncidentSources sources)\n{\n    buildMistIncidentSources(p,sources);')
  s=once(s,'    buildMistIncidentSources(rayOrigin + rayDirection *\n        (0.5 * (marchStart + marchEnd)), sources);','    // Diagnostic reference initializes each density sample locally.')
 s=once(s,'    scattered += transmittance * sampleValue.rgb * stepLength;', '''    float rawY=dot(mistProbeRaw,vec3(0.2126,0.7152,0.0722));
    float visibleY=dot(mistProbeVisible,vec3(0.2126,0.7152,0.0722));
    float weight=transmittance*sampleValue.a*0.82*stepLength;
    bool rope=mistProbeInstance==0u && mistProbePrimitive>=3028u && mistProbePrimitive<=3203u;
    mistProbeEnergy.x+=weight*rawY;
    mistProbeEnergy.y+=weight*(rawY-visibleY);
    if(rope) {mistProbeEnergy.z+=weight*(rawY-visibleY);mistProbeRopeMask|=1u<<mistProbeCount;}
    if(mistProbeInstance!=255u) mistProbeOpaqueMask|=1u<<mistProbeCount;
    mistProbeCount++;
    scattered += transmittance * sampleValue.rgb * stepLength;''')
 if measurement=='masks':
  store='vec4(float(mistProbeRopeMask)/255.0,float(mistProbeOpaqueMask)/255.0,float(mistProbeCount)/255.0,1.0)'
 else:
  field='z' if measurement=='rope-loss' else 'y'
  code=f'''uint mistProbeBits=floatBitsToUint(mistProbeEnergy.x>0.0 ? mistProbeEnergy.{field}/mistProbeEnergy.x : -1.0);
    '''
  s=once(s,'    imageStore(outputImage,',code+'imageStore(outputImage,')
  store='vec4(float(mistProbeBits&255u),float((mistProbeBits>>8u)&255u),float((mistProbeBits>>16u)&255u),float((mistProbeBits>>24u)&255u))/255.0'
 s=once(s,'vec4(clamp(color, 0.0, 1.0), 1.0));',store+');')
 write(d/'source.rgen',s)
 run(name+'-compile',[sdk/'glslangValidator.exe','-V','--target-env','vulkan1.2','-S','rgen','-o',d/'unoptimized.spv',d/'source.rgen'])
 run(name+'-optimize',[sdk/'spirv-opt.exe','--eliminate-dead-functions','--eliminate-dead-code-aggressive','--simplify-instructions','--eliminate-dead-branches','--cfg-cleanup',d/'unoptimized.spv','-o',d/'shader.spv'])
 run(name+'-validate',[sdk/'spirv-val.exe','--target-env','vulkan1.2',d/'shader.spv'])
 run(name+'-disassemble',[sdk/'spirv-dis.exe',d/'shader.spv','-o',d/'shader.spvasm'])
 asm=(d/'shader.spvasm').read_text();data=(d/'shader.spv').read_bytes()
 words=[int.from_bytes(data[i:i+4],'little') for i in range(0,len(data),4)]
 inc=''.join('    '+', '.join(f'0x{w:08x}u' for w in words[i:i+8])+',\n' for i in range(0,len(words),8))
 write(d/'words.inc',inc);write(d/'src/vulkan/raytracing/variants/diagnostic_high_generic_dielectric.inc',inc)
 rows.append(dict(name=name,source_sha256=sha(d/'source.rgen'),spirv_sha256=sha(d/'shader.spv'),include_sha256=sha(d/'words.inc'),words=len(words),atomicInstructions=len(re.findall(r'\bOpAtomic\w+\b',asm))))
write(out/'shader-metrics.json',json.dumps(rows,indent=2)+'\n')
tail=base[base.index("h=out/'harness'"):]
tail=tail.replace('RtPipelineVariantArtifact,2',f'RtPipelineVariantArtifact,{len(cases)}').replace("text[0]>'1'", f"text[0]>'{len(cases)-1}'")
tail=tail.replace('context.shadowQuality = horde::graphics::ShadowQuality::Higher;', 'context.shadowQuality = std::getenv("HORDE_MIST_PROBE_SHADOW") != nullptr && std::string_view(std::getenv("HORDE_MIST_PROBE_SHADOW")) == "CURRENT" ? horde::graphics::ShadowQuality::Current : horde::graphics::ShadowQuality::Higher;',1)
tail=tail.replace("window='#include", "window='#include <cstdlib>\\n#include",1)
if '--owner-view' in sys.argv:
 tail=tail.replace('.cameraX = -33.0f, .cameraZ = -17.5f, .yaw = -2.805f, .pitch = -0.24f,', '.cameraX = -34.4f, .cameraZ = -13.5f, .yaw = 0.10f, .pitch = -0.24f,',1)
 tail=tail.replace('.rewardPose=DevelopmentRewardPose::HeldHigh}', '.rewardPose=DevelopmentRewardPose::HeldLow}',1)
exec(tail)
