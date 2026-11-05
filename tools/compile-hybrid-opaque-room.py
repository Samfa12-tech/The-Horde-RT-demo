"""Complete opaque primary visibility: forward raster with selected genuine RT lights/shadows."""
import argparse,pathlib,re,hashlib,json,subprocess,os
p=argparse.ArgumentParser();p.add_argument('repo',type=pathlib.Path);p.add_argument('out',type=pathlib.Path);a=p.parse_args()
a.out.mkdir(parents=True,exist_ok=True);deps={}
def expand(path):
    raw=path.read_text(encoding='utf-8');deps[str(path.relative_to(a.repo))]=hashlib.sha256(raw.encode()).hexdigest()
    return re.sub(r'^\s*#include\s+"([^"]+)"\s*$',lambda m:expand(path.parent/m[1]),raw,flags=re.M)
def block(s,start):
    b=s.index('{',start);depth=1;i=b+1
    while depth:depth+=(s[i]=='{')-(s[i]=='}');i+=1
    return s[start:i]
abi=(a.repo/'shaders/raytracing/include/rt_scene_abi.glsl').read_text()
controls=(block(abi,abi.index('layout(push_constant)'))+' controls;\n').replace('float guidanceLightStrength;','float roomEyeOffset;')
draws='''struct HybridDraw {mat4 objectToWorld;uvec4 ids;};
layout(std430,set=1,binding=2) readonly buffer Draws {HybridDraw values[];} roomDraws;
'''
vertex='''#version 460
layout(set=0,binding=1,rgba8) uniform readonly image2D outputImage;
struct StaticRtVertex {vec4 position;vec4 normal;vec4 tangent;vec4 uv0;};
layout(std430,set=1,binding=3) readonly buffer Vertices {StaticRtVertex values[];} vertices;
layout(std430,set=1,binding=4) readonly buffer Indices {uint values[];} indices;
layout(std430,set=1,binding=5) readonly buffer WorldVertices {float values[];} worldVertices;
layout(std430,set=1,binding=6) readonly buffer WorldIndices {uint values[];} worldIndices;
layout(std430,set=1,binding=7) readonly buffer WorldMap {uint values[];} worldMap;
'''+controls+draws+'''
layout(location=0) out vec3 worldPosition;
layout(location=1) out vec2 bary;
layout(location=2) flat out uint drawIndex;
layout(location=3) flat out uint triangleIndex;
void main(){
 uint triangle=uint(gl_VertexIndex)/3u;drawIndex=gl_InstanceIndex;HybridDraw draw=roomDraws.values[drawIndex];
 triangleIndex=draw.ids.x==0u?worldMap.values[triangle]:triangle;
 if(draw.ids.x==0u){uint v=worldIndices.values[uint(gl_VertexIndex)]*3u;
  worldPosition=(draw.objectToWorld*vec4(worldVertices.values[v],worldVertices.values[v+1],worldVertices.values[v+2],1)).xyz;
 }else{uint v=draw.ids.w+indices.values[draw.ids.z+uint(gl_VertexIndex)];
  worldPosition=(draw.objectToWorld*vertices.values[v].position).xyz;}
 uint corner=uint(gl_VertexIndex)%3u;bary=corner==0u?vec2(0):corner==1u?vec2(1,0):vec2(0,1);
 float step=controls.time*6.2;
 vec3 origin=vec3(controls.cameraX+sin(step*.5)*.035*controls.walkAmount,
 .70+controls.roomEyeOffset+abs(sin(step))*.035*controls.walkAmount,controls.cameraZ);
 float pitch=clamp(controls.pitch+sin(step)*.012*controls.walkAmount,-.32,.28);
 vec3 f=normalize(vec3(sin(controls.yaw),-.05+pitch,-cos(controls.yaw)));
 vec3 r=normalize(cross(f,vec3(0,1,0))),u=normalize(cross(r,f));
 vec3 d=worldPosition-origin;float z=dot(d,f);
 uint mode=uint(controls.outputRedBlueSwap+.5),transform=(mode>>1u)&7u,rotation=transform&3u;
 float aspect=float(imageSize(outputImage).x)/float(imageSize(outputImage).y);
 if(rotation==1u||rotation==3u)aspect=1/aspect;
 vec2 xy=vec2(dot(d,r)*1.22/aspect,-dot(d,u)*1.22/.74);
 if(transform>=4u)xy.x=-xy.x;
 if(rotation==1u)xy=vec2(-xy.y,xy.x);else if(rotation==2u)xy=-xy;else if(rotation==3u)xy=vec2(xy.y,-xy.x);
 gl_Position=vec4(xy,z*10000.0/9999.998-.0020000004,z);
}
'''
trace_source=(a.repo/'shaders/raytracing/include/rt_hit_decode.glsl').read_text()
trace=block(trace_source,trace_source.index('HitInfo traceScene('))
initial=trace[trace.index('    HitInfo h;'):trace.index('    rayQueryEXT query;')]
static_part=block(trace,trace.index('        if ((instanceMetadata.flags & kRtInstanceFlagStaticPbr)'))
static_part=static_part.replace('rayQueryGetIntersectionBarycentricsEXT(query, true)','rasterBary')
static_part=static_part.replace('rayQueryGetIntersectionObjectToWorldEXT(query, true)','mat4x3(draw.objectToWorld)')
static_part=static_part.replace('rayQueryGetIntersectionWorldToObjectEXT(query, true)','mat4x3(inverse(draw.objectToWorld))')
assert 'rayQuery' not in static_part
shared='''layout(set=1,binding=1) uniform accelerationStructureEXT roomAS;
layout(std430,set=1,binding=7) readonly buffer WorldMap {uint values[];} roomWorldMap;
'''+draws
shade='''vec3 roomFinish(HitInfo h,vec3 direction){
 if(!h.hit)return vec3(0.012,0.018,0.030); // Shared display-space background.
 if(dot(h.normal,-direction)<0)h.normal=-h.normal;
 if(dot(h.geometricNormal,-direction)<0)h.geometricNormal=-h.geometricNormal;
 float localVisibility,skyVisibility,skyDiffuse,localStrength;vec3 localColor;
 vec3 color=shadeOpaqueDirect(h,direction,true,false,localVisibility,skyVisibility,skyDiffuse,localColor,localStrength);
 color+=h.base*h.emissive;
 color=linearToSrgb(toneMapAces(max(color,vec3(0))*controls.outputExposure));
 if((uint(controls.outputRedBlueSwap+.5)&1u)!=0u)color=color.bgr;
 return clamp(color,0,1);
}
'''
decode='''HitInfo roomRasterHit(vec3 origin,vec3 direction){
 float maxDistance=10000;
'''+initial+'''
 HybridDraw draw=roomDraws.values[rasterDraw];h.hit=true;h.t=length(rasterWorld-origin);
 h.position=rasterWorld;h.primitive=int(rasterPrimitive);h.instance=int(draw.ids.x);
 RtInstanceMetadata instanceMetadata=rtInstances.values[h.instance];uint geometryIndex=draw.ids.y;
'''+static_part+'''
 else{materialForPrimitive(h.primitive,h.instance,h.position,h.material,h.normal,h.geometricNormal,h.base,
      h.metallic,h.reflectivity,h.occlusion,h.emissive);}
 return h;
}
'''
fragment_inputs='''layout(early_fragment_tests) in;
layout(location=0) in vec3 rasterWorld;
layout(location=1) in vec2 rasterBary;
layout(location=2) flat in uint rasterDraw;
layout(location=3) flat in uint rasterPrimitive;
layout(location=0) out vec4 rasterColor;
'''
def fixed_lod(s):
    # Explicit LOD0 in both paths: fragment derivatives must not change the
    # raygen/compute reference's material filtering policy.
    start=0
    while True:
        m=re.search(r'\btexture\s*\(',s[start:])
        if not m:return s
        opening=start+m.end()-1;begin=start+m.start();depth=1;i=opening+1
        while depth:depth+=(s[i]=='(')-(s[i]==')');i+=1
        s=s[:begin]+'textureLod'+s[opening:i-1]+',0.0)'+s[i:];start=i+8
def base(ext):
    s=expand(a.repo/f'shaders/raytracing/minimal.{ext}')
    config='\n#define HORDE_RT_VARIANT_INSTRUMENTATION 0\n#define HORDE_RT_VARIANT_QUALITY 0\n#define HORDE_RT_VARIANT_MATERIAL 0\n'
    insert=s.index('\n',s.index('#version'))+1
    while s[insert:].startswith('#extension'):insert=s.index('\n',insert)+1
    if ext=='rgen':
        config+=expand(a.repo/'shaders/raytracing/include/rt_variant_config.glsl')+'\n#define HORDE_GENERIC_TRANSMISSION_VARIANT 0\n'
    s=s[:insert]+config+shared+s[insert:]
    old=block(s,s.index('void main()'))
    s=s.replace(old,'',1)
    s=s.replace('uniform accelerationStructureEXT topLevelAS;','uniform accelerationStructureEXT unusedOriginalAS;')
    s=re.sub(r'\btopLevelAS\b','roomAS',s)
    needle='h.instance = int(rayQueryGetIntersectionInstanceCustomIndexEXT(query, true));'
    assert s.count(needle)==1
    s=s.replace(needle,needle+'\n        if(h.instance==0)h.primitive=int(roomWorldMap.values[uint(h.primitive)]);')
    s=s.replace('controls.guidanceLightStrength','0.0').replace('float guidanceLightStrength;','float roomEyeOffset;')
    s=s.replace('rtHeldLight.value.playerTransform.x','controls.roomEyeOffset')
    return fixed_lod(s)
reference_main='''void main(){
 #ifdef HORDE_RT_COMPUTE_ENTRY
 if(any(greaterThanEqual(HORDE_RT_PIXEL_ID.xy,uvec2(HORDE_RT_PIXEL_EXTENT.xy))))return;
 #endif
'''+'''
 vec2 uv=(vec2(HORDE_RT_PIXEL_ID.xy)+.5)/vec2(HORDE_RT_PIXEL_EXTENT.xy);
 uint transform=(uint(controls.outputRedBlueSwap+.5)>>1u)&7u,rotation=transform&3u;
 if(rotation==1u)uv=vec2(uv.y,1-uv.x);else if(rotation==2u)uv=1-uv;else if(rotation==3u)uv=vec2(1-uv.y,uv.x);
 if(transform>=4u)uv.x=1-uv.x;
 float aspect=float(HORDE_RT_PIXEL_EXTENT.x)/float(HORDE_RT_PIXEL_EXTENT.y);
 if(rotation==1u||rotation==3u)aspect=1/aspect;
 float step=controls.time*6.2;
 vec3 origin=vec3(controls.cameraX+sin(step*.5)*.035*controls.walkAmount,.70+controls.roomEyeOffset+abs(sin(step))*.035*controls.walkAmount,controls.cameraZ);
 float pitch=clamp(controls.pitch+sin(step)*.012*controls.walkAmount,-.32,.28);
 vec3 f=normalize(vec3(sin(controls.yaw),-.05+pitch,-cos(controls.yaw)));
 vec3 r=normalize(cross(f,vec3(0,1,0))),u=normalize(cross(r,f));vec2 screen=uv*2-1;screen.x*=aspect;screen.y*=-.74;
 vec3 direction=normalize(f*1.22+r*screen.x+u*screen.y);
 HitInfo h=traceScene(origin,direction,10000.0,0xffu,.002,false,false);
 imageStore(outputImage,ivec2(HORDE_RT_PIXEL_ID.xy),vec4(roomFinish(h,direction),1));
}
'''
fragment_main='''void main(){
 float step=controls.time*6.2;
 vec3 origin=vec3(controls.cameraX+sin(step*.5)*.035*controls.walkAmount,.70+controls.roomEyeOffset+abs(sin(step))*.035*controls.walkAmount,controls.cameraZ);
 vec3 direction=normalize(rasterWorld-origin);
 rasterColor=vec4(roomFinish(roomRasterHit(origin,direction),direction),1);
}
'''
sdk=pathlib.Path(os.environ.get('VULKAN_SDK',r'C:\VulkanSDK\1.4.350.0'))/'Bin'
header=['#pragma once','#include <cstdint>','#include <span>','namespace horde::vulkan::raytracing::experimental {'];records=[]
def compile(name,s):
    path=a.out/name;path.write_text(s,encoding='utf-8');raw=path.with_suffix(path.suffix+'.raw.spv');opt=path.with_suffix(path.suffix+'.spv')
    subprocess.run([str(sdk/'glslangValidator.exe'),'-V','--target-env','vulkan1.2','-Os',str(path),'-o',str(raw)],check=True)
    subprocess.run([str(sdk/'spirv-opt.exe'),'-O',str(raw),'-o',str(opt)],check=True)
    subprocess.run([str(sdk/'spirv-val.exe'),'--target-env','vulkan1.2',str(opt)],check=True)
    dis=subprocess.check_output([str(sdk/'spirv-dis.exe'),str(opt)],text=True)
    assert 'OpCapability Geometry' not in dis
    b=opt.read_bytes();symbol=name.replace('.','_');digest=hashlib.sha256(b).hexdigest()
    header.append('inline constexpr uint32_t '+symbol+'[]={'+','.join(hex(int.from_bytes(b[i:i+4],'little')) for i in range(0,len(b),4))+'};')
    header.append('inline constexpr char '+symbol+'_sha[]="'+digest+'";')
    records.append({'name':name,'bytes':len(b),'sha256':digest,'staticQueryInitSites':dis.count('OpRayQueryInitializeKHR')})
compile('room.vert',vertex)
fs=base('comp').replace('layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;','')
fs=fs.replace('#define HORDE_RT_PIXEL_ID gl_GlobalInvocationID','#define HORDE_RT_PIXEL_ID uvec3(gl_FragCoord.xy,0)')
compile('room.frag',fixed_lod(fs+fragment_inputs+decode+shade+fragment_main))
for ext in ['rgen','comp']:compile('room_reference.'+ext,base(ext)+shade+reference_main)
header.append('}')
(a.out/'HybridOpaqueRoomShaders.h').write_text('\n'.join(header)+'\n')
(a.out/'manifest.json').write_text(json.dumps({'scope':'complete static opaque architecture world0 + StaticPbr21','primaryQueryInForwardMain':False,'selectedEffects':'production direct lighting and genuine hardware ray-query shadows only','textureLod':0,'dependencies':deps,'modules':records},indent=2)+'\n')
print(json.dumps(records,indent=2))
