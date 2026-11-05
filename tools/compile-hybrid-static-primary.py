"""Generate opt-in production-shader adapters; original shader files stay unchanged."""
import argparse,pathlib,re,hashlib,json,subprocess,os
p=argparse.ArgumentParser();p.add_argument('repo',type=pathlib.Path);p.add_argument('out',type=pathlib.Path);a=p.parse_args()
a.out.mkdir(parents=True,exist_ok=True);deps={}
def expand(path,active=()):
    path=path.resolve();assert path not in active
    raw=path.read_text(encoding='utf-8');deps[str(path.relative_to(a.repo.resolve()))]=hashlib.sha256(raw.replace('\r\n','\n').encode()).hexdigest()
    return re.sub(r'^\s*#include\s+"([^"]+)"\s*$',lambda m:expand(path.parent/m[1],active+(path,)),raw,flags=re.M)
def block(text,start):
    opening=text.index('{',start);depth=1;i=opening+1
    while depth:
        depth+=(text[i]=='{')-(text[i]=='}');i+=1
    return text[start:i]
abi=(a.repo/'shaders/raytracing/include/rt_scene_abi.glsl').read_text()
controls=block(abi,abi.index('layout(push_constant)'))+' controls;'
# block already ends at } and its source declaration continues with controls;
common='''struct StaticRtVertex {vec4 position;vec4 normal;vec4 tangent;vec4 uv0;};
struct HybridDraw {mat4 objectToWorld;uvec4 ids;};
layout(std430,set=1,binding=2) readonly buffer Draws {HybridDraw values[];} hybridDraws;
'''
vertex='''#version 460
'''+controls+'\n'+common+'''
layout(std430,set=1,binding=3) readonly buffer Vertices {StaticRtVertex values[];} vertices;
layout(std430,set=1,binding=4) readonly buffer Indices {uint values[];} indices;
layout(location=0) out vec3 worldPosition;
layout(location=1) out vec2 bary;
layout(location=2) flat out uint drawIndex;
layout(location=3) flat out uint triangleIndex;
void main(){
    triangleIndex=uint(gl_VertexIndex)/3u;drawIndex=gl_InstanceIndex;HybridDraw draw=hybridDraws.values[drawIndex];
    uint vertex=draw.ids.w+indices.values[draw.ids.z+uint(gl_VertexIndex)];
    worldPosition=(draw.objectToWorld*vertices.values[vertex].position).xyz;
    uint corner=uint(gl_VertexIndex)%3u;bary=corner==0u?vec2(0):corner==1u?vec2(1,0):vec2(0,1);
    float step=controls.time*6.2;
    vec3 origin=vec3(controls.cameraX+sin(step*.5)*.035*controls.walkAmount,
        .70+controls.hybridEyeOffset+abs(sin(step))*.035*controls.walkAmount,controls.cameraZ);
    float pitch=clamp(controls.pitch+sin(step)*.012*controls.walkAmount,-.32,.28);
    vec3 f=normalize(vec3(sin(controls.yaw),-.05+pitch,-cos(controls.yaw)));
    vec3 r=normalize(cross(f,vec3(0,1,0))),u=normalize(cross(r,f));
    vec3 d=worldPosition-origin;float z=dot(d,f);
    uint mode=uint(controls.outputRedBlueSwap+.5),transform=(mode>>1u)&7u,rotation=transform&3u;
    float aspect=controls.hybridWidth/max(controls.hybridHeight,1.0);if(rotation==1u||rotation==3u)aspect=1/aspect;
    vec2 xy=vec2(dot(d,r)*1.22/aspect,-dot(d,u)*1.22/.74);
    if(transform>=4u)xy.x=-xy.x;
    if(rotation==1u)xy=vec2(-xy.y,xy.x);else if(rotation==2u)xy=-xy;else if(rotation==3u)xy=vec2(xy.y,-xy.x);
    gl_Position=vec4(xy,z*10000.0/9999.999-.0010000001,z);
}
'''
# Raster has a separate layout but exactly 128-byte controls; unused fields carry
# extent/eye offset for vertex projection only. Original raygen gets original bytes.
vertex=vertex.replace('float guidanceLightStrength;','float hybridEyeOffset;').replace('float genericTransmissionActive;','float hybridHeight;').replace('float workloadPreset;','float hybridWidth;')
fragment='''#version 460
layout(location=0) in vec3 worldPosition;
layout(location=1) in vec2 bary;
layout(location=2) flat in uint drawIndex;
layout(location=3) flat in uint triangleIndex;
layout(location=0) out uvec4 visibility;
'''+controls.replace('float guidanceLightStrength;','float hybridEyeOffset;')+'''
void main(){float step=controls.time*6.2;vec3 eye=vec3(controls.cameraX+sin(step*.5)*.035*controls.walkAmount,
 .70+controls.hybridEyeOffset+abs(sin(step))*.035*controls.walkAmount,controls.cameraZ);
 visibility=uvec4(floatBitsToUint(length(worldPosition-eye)),drawIndex,triangleIndex,packUnorm2x16(clamp(bary,0,1)));}
'''
hit_path=a.repo/'shaders/raytracing/include/rt_hit_decode.glsl';hit_source=hit_path.read_text()
trace=block(hit_source,hit_source.index('HitInfo traceScene('))
remaining=trace.replace('HitInfo traceScene(', 'HitInfo hybridTraceRemaining(').replace('query, topLevelAS,','query, hybridRemainingAS,')
assert 'query, hybridRemainingAS,' in remaining
initial=trace[trace.index('    HitInfo h;'):trace.index('    rayQueryEXT query;')]
static_part=block(trace,trace.index('        if ((instanceMetadata.flags & kRtInstanceFlagStaticPbr)'))
static_part=static_part.replace('rayQueryGetIntersectionBarycentricsEXT(query, true)','unpackUnorm2x16(record.w)')
static_part=static_part.replace('rayQueryGetIntersectionObjectToWorldEXT(query, true)','mat4x3(draw.objectToWorld)')
static_part=static_part.replace('rayQueryGetIntersectionWorldToObjectEXT(query, true)','mat4x3(inverse(draw.objectToWorld))')
assert 'rayQuery' not in static_part
decode='''HitInfo hybridPrimary(vec3 origin,vec3 direction,float maxDistance,uint mask,float minimumDistance,bool ignoreWater,bool ignorePlayerNearFace){
'''+initial+'''
    uvec4 record=imageLoad(hybridVisibility,ivec2(HORDE_RT_PIXEL_ID.xy));
    if(record.y!=0xffffffffu){
        HybridDraw draw=hybridDraws.values[record.y];
        h.hit=true;h.t=uintBitsToFloat(record.x);h.primitive=int(record.z);h.instance=int(draw.ids.x);
        h.position=origin+direction*h.t;RtInstanceMetadata instanceMetadata=rtInstances.values[h.instance];
        uint geometryIndex=draw.ids.y;
'''+static_part+'''
    }
    HitInfo remaining=hybridTraceRemaining(origin,direction,h.hit?h.t:maxDistance,mask,minimumDistance,ignoreWater,ignorePlayerNearFace);
    return remaining.hit?remaining:h;
}
'''
shared='''layout(set=1,binding=0,rgba32ui) uniform readonly uimage2D hybridVisibility;
layout(set=1,binding=1) uniform accelerationStructureEXT hybridRemainingAS;
struct HybridDraw {mat4 objectToWorld;uvec4 ids;};
layout(std430,set=1,binding=2) readonly buffer HybridDraws {HybridDraw values[];} hybridDraws;
'''
sdk=pathlib.Path(os.environ.get('VULKAN_SDK',r'C:\VulkanSDK\1.4.350.0'))/'Bin'
catalog=json.loads((a.repo/'tools/raygen-variant-catalog.json').read_text())
generic_cleanup=next(v['compiler']['spirvOptArguments'] for v in catalog['variants'] if v['key']=='shipping_mobile_generic_dielectric')
header=['#pragma once','#include <cstdint>','#include <span>','namespace horde::vulkan::raytracing::experimental {','inline constexpr int kHybridInstrumentation=0,kHybridQuality=0;']
records=[]
def compile(name,text):
    path=a.out/name;path.write_text(text,encoding='utf-8');spv=path.with_suffix(path.suffix+'.spv')
    retained=name.startswith('hybrid1.')
    args=[str(sdk/'glslangValidator.exe'),'-V','--target-env','vulkan1.2']
    if not retained:args+=['-Os']
    subprocess.run(args+[str(path),'-o',str(spv)],check=True)
    optimized=path.with_suffix(path.suffix+'.opt.spv')
    opt_args=generic_cleanup if retained else ['-O','<input>','-o','<output>']
    opt_args=[str(spv) if x=='<input>' else str(optimized) if x=='<output>' else x for x in opt_args]
    subprocess.run([str(sdk/'spirv-opt.exe'),*opt_args],check=True)
    spv=optimized
    subprocess.run([str(sdk/'spirv-val.exe'),'--target-env','vulkan1.2',str(spv)],check=True)
    if name=='hybrid.frag':
        dis=subprocess.check_output([str(sdk/'spirv-dis.exe'),str(spv)],text=True)
        assert 'OpCapability Geometry' not in dis,'Raster fragment must not require geometryShader'
    b=spv.read_bytes();sym=name.replace('.','_');digest=hashlib.sha256(b).hexdigest()
    header.append('inline constexpr uint32_t '+sym+'[]={'+','.join(hex(int.from_bytes(b[i:i+4],'little')) for i in range(0,len(b),4))+'};')
    header.append('inline constexpr char '+sym+'_sha[]="'+digest+'";')
    records.append({'name':name,'bytes':len(b),'sha256':digest})
compile('hybrid.vert',vertex);compile('hybrid.frag',fragment)
for ext in ['rgen','comp']:
    source=expand(a.repo/f'shaders/raytracing/minimal.{ext}')
    for material in range(2):
        config=f'\n#define HORDE_RT_VARIANT_INSTRUMENTATION 0\n#define HORDE_RT_VARIANT_QUALITY 0\n#define HORDE_RT_VARIANT_MATERIAL {material}\n'
        if ext=='rgen':
            config+=expand(a.repo/'shaders/raytracing/include/rt_variant_config.glsl')
            config+=f'\n#define HORDE_GENERIC_TRANSMISSION_VARIANT {material}\n'
        insert=source.index('\n',source.index('#version'))+1
        # Defines must precede configuration but extensions remain ahead of code.
        while source[insert:].startswith('#extension'):
            insert=source.index('\n',insert)+1
        candidate=source[:insert]+config+source[insert:]
        marker='void main()'
        pos=candidate.index(marker)
        candidate=candidate[:pos]+shared+remaining+decode+candidate[pos:]
        candidate=candidate.replace('HitInfo primary = traceScene(origin, rayDirection,','HitInfo primary = hybridPrimary(origin, rayDirection,')
        assert 'HitInfo primary = hybridPrimary' in candidate
        compile(f'hybrid{material}.{ext}',candidate)
header.append('}')
(a.out/'HybridStaticPrimaryShaders.h').write_text('\n'.join(header)+'\n')
(a.out/'manifest.json').write_text(json.dumps({'scope':'production shader adapter; immutable instance21 primary only','instrumentation':'Shipping','quality':'Mobile','dependencies':deps,'modules':records,'optimizationPolicy':{'opaqueFast':'production LegacyInlined: glslang -Os + spirv-opt -O','genericDielectric':'production GenericRetained cleanup list from frozen raygen catalog','raster':'glslang -Os + spirv-opt -O','geometryShaderRequired':False}},indent=2)+'\n')
print(json.dumps(records,indent=2))
