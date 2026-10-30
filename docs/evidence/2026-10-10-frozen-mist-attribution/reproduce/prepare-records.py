import hashlib,json,re,subprocess,sys,time
from pathlib import Path
root=Path.cwd(); out=root/'reports/frozen-mist/records'; sdk=Path('C:/VulkanSDK/1.4.350.0/Bin')
cache=(root/'build/presets/windows-x64-debug/CMakeCache.txt').read_text()
cmake=next(x.split('=',1)[1] for x in cache.splitlines() if x.startswith('CMAKE_COMMAND:INTERNAL='))
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def write(p,s): p=Path(p);p.parent.mkdir(parents=True,exist_ok=True);p.write_text(s,encoding='utf-8',newline='\n')
def run(label,args):
 if (out/(label+'.private.log')).exists():
  n=2
  while (out/(label+f'-{n}.private.log')).exists():n+=1
  label+=f'-{n}'
 t=time.perf_counter()
 with (out/(label+'.private.log')).open('wb') as f: p=subprocess.run([str(x) for x in args],stdout=f,stderr=subprocess.STDOUT)
 s=(out/(label+'.private.log')).read_text(errors='replace').replace(str(root),'<repo>').replace(root.as_posix(),'<repo>')
 s=re.sub(r'C:[/\\]+Users[/\\]+[^/\\\s"<>]+','<user-home>',s,flags=re.I)
 write(out/(label+'.log'),s)
 print(label,p.returncode,round(time.perf_counter()-t,2),flush=True)
 if p.returncode: print(s[-2500:]);sys.exit(p.returncode)

def once(s,a,b):
 assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)

if not (out/'baseline/diagnostic_high_generic_dielectric/minimal.rgen.resolved').exists():
 run('baseline-compile',['pwsh','-NoProfile','-File',root/'tools/compile-raygen.ps1','-Variant','diagnostic_high_generic_dielectric','-OutputDirectory',out/'baseline'])
baseline=out/'baseline/diagnostic_high_generic_dielectric'
source=(baseline/'minimal.rgen.resolved').read_text()
record=json.loads((baseline/'raygen-stats.json').read_text())
catalog=json.loads((root/'tools/raygen-variant-catalog.json').read_text())
expected=next(x for x in catalog['variants'] if x['key']==record['key'])
assert record['compiledSpirvSha256']==expected['spirvSha256']

def function(s,name):
 m=re.search(r'(?m)^(?:void|vec[34]|float|bool) '+name+r'\([^;]*?\)\s*\{',s);assert m,name
 start=m.start();i=m.end();depth=1
 while depth:
  if s[i]=='{':depth+=1
  if s[i]=='}':depth-=1
  i+=1
 return start,i,s[start:i]

rows=json.loads((out/'shader-metrics.json').read_text()) if '--native-only' in sys.argv else []
for policy in ([] if '--native-only' in sys.argv else ['midpoint']):
 for lighting in ['sky','lantern']:
  name=policy+'-'+lighting;d=out/'shaders'/name;d.mkdir(parents=True,exist_ok=True)
  s=source
  a,b,f=function(s,'buildMistIncidentSources')
  if lighting=='sky':
   start=f.index('    float staffStrength =');f=f[:start]+'''    for (uint index=0u;index<kRtActiveFireEmitterCapacity;++index) {
        sources.firePositions[index]=vec3(0.0); sources.fireVisibleRadiances[index]=vec3(0.0);
    }
}'''
  if lighting=='lantern':
   start=f.index('    vec3 skyDirection;');end=f.index('    // Consume every admitted')
   f=f[:start]+f[end:]
   # Stable admitted emitter identity, not array order. Must match frozen snapshot.
   f=once(f,'if (emitter.identity.x == 0u ||','if (emitter.identity.x != 0x4c414e54u ||')
  s=s[:a]+f+s[b:]
  if policy=='sample-local':
   s=once(s,'vec4 lichMistSample(vec3 p, MistIncidentSources sources)\n{','vec4 lichMistSample(vec3 p, MistIncidentSources sources)\n{\n    buildMistIncidentSources(p, sources); // Diagnostic reference: same hardware helper/mask at each existing density sample.')
   s=once(s,'    buildMistIncidentSources(rayOrigin + rayDirection *\n        (0.5 * (marchStart + marchEnd)), sources);','    // Sample-local reference initializes each consumed record inside lichMistSample.')
  s=once(s,'vec3 boundedShadowTransmittanceMask(vec3 origin, vec3 direction,',
    'bool mistProbeActive=false; uint mistProbeInstance=255u; uint mistProbePrimitive=65535u; vec3 mistProbeRaw=vec3(0.0); vec3 mistProbeVisible=vec3(0.0);\nvec3 boundedShadowTransmittanceMask(vec3 origin, vec3 direction,')
  s=once(s,'    if (rayQueryGetIntersectionTypeEXT(query, true) !=\n        gl_RayQueryCommittedIntersectionNoneEXT)\n        return vec3(0.0);',
    '    if (rayQueryGetIntersectionTypeEXT(query, true) !=\n        gl_RayQueryCommittedIntersectionNoneEXT) {\n        if(mistProbeActive) {mistProbeInstance=uint(rayQueryGetIntersectionInstanceCustomIndexEXT(query,true));mistProbePrimitive=uint(rayQueryGetIntersectionPrimitiveIndexEXT(query,true));}\n        return vec3(0.0);\n    }')
  if lighting=='sky':
   s=once(s,'    if (!isnan(skyGain) &&', '    mistProbeActive=true;\n    if (!isnan(skyGain) &&')
   a,b,f=function(s,'buildMistIncidentSources');f=once(f,'    for (uint index=0u;', '    mistProbeActive=false;\n    for (uint index=0u;');s=s[:a]+f+s[b:]
   encoded='vec3(float(mistProbePrimitive & 255u),float((mistProbePrimitive >> 8u) & 255u),float(mistProbeInstance))/255.0'
  else:
   s=once(s,'        sources.fireVisibleRadiances[index] = mistVisiblePointRadiance(', '        mistProbeActive=true;\n        mistProbeRaw=tunedLightColor(emitter.colourIntensity.rgb,kLightTorch)*strength;\n        sources.fireVisibleRadiances[index] = mistVisiblePointRadiance(')
   a,b,f=function(s,'buildMistIncidentSources');f=once(f,'    }\n}', '        mistProbeVisible=sources.fireVisibleRadiances[index];mistProbeActive=false;\n    }\n}');s=s[:a]+f+s[b:]
   encoded='vec3(max(mistProbeRaw.r,max(mistProbeRaw.g,mistProbeRaw.b)),max(mistProbeVisible.r,max(mistProbeVisible.g,mistProbeVisible.b)),float(mistProbeInstance)/255.0)'
  s=once(s,'vec4(clamp(color, 0.0, 1.0), 1.0));', 'vec4('+encoded+',1.0));')
  write(d/'source.rgen',s)
  run(name+'-compile',[sdk/'glslangValidator.exe','-V','--target-env','vulkan1.2','-S','rgen','-o',d/'unoptimized.spv',d/'source.rgen'])
  run(name+'-optimize',[sdk/'spirv-opt.exe','--eliminate-dead-functions','--eliminate-dead-code-aggressive','--simplify-instructions','--eliminate-dead-branches','--cfg-cleanup',d/'unoptimized.spv','-o',d/'shader.spv'])
  run(name+'-validate',[sdk/'spirv-val.exe','--target-env','vulkan1.2',d/'shader.spv'])
  run(name+'-disassemble',[sdk/'spirv-dis.exe',d/'shader.spv','-o',d/'shader.spvasm'])
  asm=(d/'shader.spvasm').read_text();data=(d/'shader.spv').read_bytes()
  words=[int.from_bytes(data[i:i+4],'little') for i in range(0,len(data),4)]
  inc=''.join('    '+', '.join(f'0x{w:08x}u' for w in words[i:i+8])+',\n' for i in range(0,len(words),8))
  write(d/'words.inc',inc)
  write(d/'src/vulkan/raytracing/variants/diagnostic_high_generic_dielectric.inc',inc)
  stats=dict(name=name,source_sha256=sha(d/'source.rgen'),spirv_sha256=sha(d/'shader.spv'),include_sha256=sha(d/'words.inc'),bytes=len(data),words=len(words),
   instructions=len(re.findall(r'^\s*(?:%\S+\s*=\s*)?Op\w+',asm,re.M)),
   branchOperations=len(re.findall(r'\bOp(?:Branch|BranchConditional|Switch)\b',asm)),loops=asm.count('OpLoopMerge'),selectionMerges=asm.count('OpSelectionMerge'),
   functions=len(re.findall(r'\bOpFunction\b',asm)),functionCalls=asm.count('OpFunctionCall'),rayQueryInitializations=asm.count('OpRayQueryInitializeKHR'),
   atomicInstructions=len(re.findall(r'\bOpAtomic\w+\b',asm)),hasDiagnosticsBinding=bool(re.search(r'\bOpDecorate\s+%\S+\s+Binding\s+22\b',asm)))
  rows.append(stats)
# These are diagnostic output records, never catalogue packages.
write(out/'shader-metrics.json',json.dumps(rows,indent=2)+'\n')

h=out/'harness';h.mkdir(exist_ok=True)
header=(root/'src/gameplay/DevelopmentCheckpoints.h').read_text()
header=once(header,'.cameraX = 0.0f, .cameraZ = 1.85f, .yaw = 3.14159265359f, .pitch = -0.05f},',
 '.cameraX = -33.0f, .cameraZ = -17.5f, .yaw = -2.805f, .pitch = -0.24f,\n     .usesProductionRewardProps=true, .rewardPose=DevelopmentRewardPose::HeldHigh},')
header=once(header,'.id = 192, .name = "rescue-journey-start", .baseShowcaseCheckpointId = 0,','.id = 192, .name = "rescue-journey-start", .baseShowcaseCheckpointId = 11,')
write(h/'gameplay/DevelopmentCheckpoints.h',header)
stage='''
static bool StageFrozenMist(horde::gameplay::simulation::GameSimulation& sim,
 const horde::gameplay::DevelopmentCheckpoint& checkpoint) {
 if(!horde::gameplay::StageDevelopmentCheckpointSimulation(sim,checkpoint)) return false;
 sim.SetDevelopmentRescueJourney(true);
 horde::gameplay::simulation::InputSnapshot input;
 input.damageEnabled=false;input.hasAuthoritativePlayerPose=true;
 input.authoritativePlayerX=checkpoint.cameraX;input.authoritativePlayerZ=checkpoint.cameraZ;
 input.yawRadians=checkpoint.yaw;input.pitchRadians=checkpoint.pitch;input.torchLightStrength=0;
 for(unsigned tick=0;tick<600;++tick) sim.StepFixed(input);
 sim.ClearEvents();
 const auto& s=sim.Snapshot();
 return s.rescue.ropeReady && s.chestReward.phase==horde::gameplay::interactions::ChestRewardPhase::LanternClaimed &&
        s.lich.staffLightStrength==0 && s.lich.finaleDawnRevealProgress==0;
}
static void WriteFrozenMistState(const horde::gameplay::simulation::SimulationSnapshot& s,const std::filesystem::path& path) {
 std::ofstream f(path/"frozen-state.json"); f<<std::setprecision(9);
 f<<"{\\"tick\\":"<<s.tickIndex<<",\\"walkTime\\":"<<s.walkTime<<",\\"camera\\":["<<s.playerX<<","<<s.playerSupportWorldY+1.65f<<","<<s.playerZ<<","<<s.playerYawRadians<<","<<s.playerPitchRadians<<"],\\"ropePhase\\":"<<unsigned(s.rescue.phase)<<",\\"deploymentPhase\\":"<<unsigned(s.rescue.deploymentPhase)<<",\\"tension\\":"<<s.rescue.ropeTension<<",\\"nodes\\":[";
 for(unsigned i=0;i<s.rescue.ropeNodes.size();++i) { const auto p=s.rescue.ropeNodes[i];if(i)f<<",";f<<"["<<p.x<<","<<p.y<<","<<p.z<<"]"; }
 f<<"],\\"triangles\\":["; const auto v=horde::scene::RescueRopeTriangleVertices(s.rescue);
 for(unsigned i=0;i<v.size();++i) {if(i)f<<",";f<<"["<<v[i][0]<<","<<v[i][1]<<","<<v[i][2]<<"]";}
 f<<"],\\"emitters\\":[";
 for(unsigned i=0;i<s.fireEmitterCount;++i) {if(i)f<<",";const auto& e=s.fireEmitters[i];f<<"{\\"id\\":"<<e.stableId<<",\\"parent\\":"<<unsigned(e.parentObject)<<",\\"strength\\":"<<e.strength<<",\\"light\\":["<<e.worldFromLight[12]<<","<<e.worldFromLight[13]<<","<<e.worldFromLight[14]<<"],\\"phase\\":"<<e.phase<<"}";}
 f<<"]}\\n";
}
'''
window=(root/'src/platform/windows/DiagnosticWindow.cpp').read_text()
window='#include "scene/RescueJourneyGeometry.h"\n#include <fstream>\n'+window
window=once(window,'namespace\n{','#define MessageBoxA(...) IDOK // Capture-only harness: report failures without a modal UI.\nnamespace\n{')
window=once(window,'int RunShowcaseCapture(VulkanSurfaceContext& context,',stage+'\nint RunShowcaseCapture(VulkanSurfaceContext& context,')
window=once(window,'!horde::gameplay::StageDevelopmentCheckpointSimulation(\n                    context.simulation, *development)','!StageFrozenMist(context.simulation, *development)')
window=once(window,'            context.simulation.ResetTiming();\n            context.simulation.ClearEvents();','            WriteFrozenMistState(context.simulation.Snapshot(), outputDirectory);\n            context.simulation.ResetTiming();\n            context.simulation.ClearEvents();')
window=once(window,'        context.stagedWorldPreparation = proof != nullptr && proof->stagedWorldPreparation;',
 '        context.stagedWorldPreparation = proof != nullptr && proof->stagedWorldPreparation;\n        context.developmentRescueJourney=true;context.developmentWorldRoute=true;context.simulation.SetDevelopmentRescueJourney(true);')
window=once(window,'int showMode=captureDirectory != nullptr ? SW_SHOWNOACTIVATE : SW_SHOW;','int showMode=captureDirectory != nullptr ? SW_HIDE : SW_SHOW; // Diagnostic capture never takes focus or exposes private desktop.')
window=once(window,'        const bool completedRtDispatch = context.useRtPath &&', '        { std::ofstream evidence(outputDirectory / "completed-frame.json"); evidence << completedFrameJson; }\n        const bool completedRtDispatch = context.useRtPath &&')
window=once(window,'constexpr std::uint32_t kCaptureWidth = 960u;','constexpr std::uint32_t kCaptureWidth = 1232u;')
window=once(window,'constexpr std::uint32_t kCaptureHeight = 540u;','constexpr std::uint32_t kCaptureHeight = 803u;')
window=once(window,'        context.renderScale = 1.0f;','        context.shadowQuality = horde::graphics::ShadowQuality::Higher;\n        context.requestedMistEnabled = true;\n        context.rtSceneTuning.workloadPreset = horde::vulkan::raytracing::RtWorkloadPreset::Authored;\n        context.renderScale = 1.0f;')
sceneHeader=(root/'src/vulkan/raytracing/PresentableTinyRtScene.h').read_text()
sceneHeader=once(sceneHeader,'    const horde::scene::DevelopmentWorldGeometry& WorldRouteGeometry() const { return worldRouteGeometry_; }', '    const horde::scene::DevelopmentWorldGeometry& WorldRouteGeometry() const { return worldRouteGeometry_; }\n    std::uint32_t FrozenMistWorldPrimitiveCount() const { return rescueWorldPrimitiveCount_; }')
write(h/'vulkan/raytracing/PresentableTinyRtScene.h',sceneHeader)
window=once(window,'        const bool completedRtDispatch = context.useRtPath &&', '        { std::ofstream geometry(outputDirectory / "geometry-state.json"); geometry << "{\\\"worldPrimitiveCount\\\":" << context.rtScene.FrozenMistWorldPrimitiveCount() << ",\\\"ropeTriangleCount\\\":" << horde::scene::RescueRopeTriangleVertices(context.simulation.Snapshot().rescue).size()/3u << ",\\\"masks\\\":[";const auto masks=context.rtScene.LastInstanceMasks();for(unsigned i=0;i<masks.size();++i) {if(i)geometry << ",";geometry << unsigned(masks[i]);}geometry << "]}"; }\n        const bool completedRtDispatch = context.useRtPath &&')
write(h/'DiagnosticWindow.cpp',window)

provider='''#include "vulkan/raytracing/RtPipelineVariantProvider.h"
#include "vulkan/raytracing/RtPipelineVariantCatalog.generated.h"
#include <array>
#include <cstdlib>
namespace horde::vulkan::raytracing {
namespace {
constexpr std::uint32_t opaqueWords[]={
#include "vulkan/raytracing/variants/diagnostic_high_opaque_fast.inc"
};
'''
for i,r in enumerate(rows):
 provider+=f'constexpr std::uint32_t words{i}[]={{\n#include "{(out/"shaders"/r["name"]/"words.inc").as_posix()}"\n}};\n'
provider+='const std::array<RtPipelineVariantArtifact,2> probes{{\n'
for i,r in enumerate(rows):
 provider+=f'RtPipelineVariantArtifact{{{{RtInstrumentation::Diagnostic,DielectricQuality::High,RtMaterialStrategy::GenericDielectric}},words{i},"diagnostic_high_generic_dielectric","src/vulkan/raytracing/variants/diagnostic_high_generic_dielectric.inc","{r["spirv_sha256"]}","{r["include_sha256"]}",{r["words"]},{r["atomicInstructions"]},true}},\n'
provider+='''}};
}
const RtPipelineVariantProvider& RtPipelineVariantProvider::Compiled(RtExecutionBackend backend) noexcept {
 static constexpr RtPipelineVariantProvider pipeline{{RtInstrumentation::Diagnostic,DielectricQuality::High}};
 static constexpr RtPipelineVariantProvider unavailable{{RtInstrumentation::Diagnostic,DielectricQuality::High,RtExecutionBackend::Unsupported}};
 return backend==RtExecutionBackend::RayTracingPipeline?pipeline:unavailable;
}
const RtPipelineBundleRequest& RtPipelineVariantProvider::request() const noexcept {return request_;}
std::optional<RtPipelineVariantArtifact> RtPipelineVariantProvider::ResolveExact(RtPipelineVariantKey requested,std::string* error) const {
 if(requested.instrumentation!=RtInstrumentation::Diagnostic || requested.quality!=DielectricQuality::High || requested.executionBackend!=RtExecutionBackend::RayTracingPipeline) {if(error)*error="Diagnostic probe supports only Pipeline/Diagnostic/High";return std::nullopt;}
 if(requested.material==RtMaterialStrategy::OpaqueFast) {const auto& r=detail::kSelectedRtPipelineCatalog[0];return RtPipelineVariantArtifact{r.key,opaqueWords,r.canonicalKey,r.artifactPath,r.spirvSha256,r.includeSha256,r.words,r.atomicInstructions,r.hasDiagnosticsBinding};}
 const char* text=std::getenv("HORDE_MIST_PROBE_MODE");
 if(!text || text[0]<'0' || text[0]>'1' || text[1]!=0) {if(error)*error="Explicit probe mode 0..5 required";return std::nullopt;}
 if(error)error->clear();return probes[text[0]-'0'];
}
}
'''
write(h/'Provider.cpp',provider)
native=root/'build/presets/windows-x64-debug'; src=root/'src'
extra=['platform/windows/DiagnosticWindowMain.cpp','platform/windows/WindowsGitHubReleaseUpdate.cpp','platform/windows/WindowsPlaytestReport.cpp','platform/windows/WindowsRemotePlaytestReport.cpp','platform/windows/WindowsPlaytestScreenshot.cpp','platform/windows/WindowsPlaytestSubmission.cpp','platform/windows/WindowsPlaytestVerification.cpp','ui/DiagnosticOverlay.cpp','gameplay/validation/MotionEvidenceScenario.cpp','telemetry/MotionEvidenceLedger.cpp']
c='cmake_minimum_required(VERSION 3.22)\nset(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")\nproject(FrozenMistAttribution LANGUAGES CXX)\nfind_package(Vulkan REQUIRED)\n'
c+=f'include("{(root/"cmake/HordeRtWebView2.cmake").as_posix()}")\nhorde_rt_add_report_webview2("{root.as_posix()}")\n'
c+='add_executable(frozen_mist WIN32 DiagnosticWindow.cpp Provider.cpp\n'+''.join(f'"{(src/x).as_posix()}"\n' for x in extra)+')\n'
c+=f'target_sources(frozen_mist PRIVATE "{(native/"generated/windows/HordeLanternRT.manifest").as_posix()}")\n'
c+=f'target_include_directories(frozen_mist PRIVATE "{h.as_posix()}" "{src.as_posix()}" "{(root/"third_party/pocket-audio-core/native/include").as_posix()}")\ntarget_compile_features(frozen_mist PRIVATE cxx_std_20)\n'
c+=f'target_compile_definitions(frozen_mist PRIVATE HORDE_RT_SELECTED_INSTRUMENTATION=1 HORDE_RT_SELECTED_DIELECTRIC_QUALITY=1 HORDE_RT_DISPLAY_VERSION="1.6.2" HORDE_RT_PACKAGE_VERSION="1.6.2" HORDE_RT_BUILD_ID="frozen-mist-attribution" HORDE_RT_SOURCE_DIR="{root.as_posix()}")\n'
libs=[native/'Debug/horde_rt_probe_core.lib',native/'Debug/horde_windows_music.lib',native/'Debug/horde_gameplay_simulation.lib',native/'pocket-audio-core-native/Debug/pocket_audio_native_pcm.lib']
for lib in libs:assert lib.is_file()
c+='target_link_libraries(frozen_mist PRIVATE '+''.join(f'"{p.as_posix()}" ' for p in libs)+'Vulkan::Vulkan horde_rt_report_webview2 comdlg32 comctl32 bcrypt ole32 windowscodecs oleaut32 shell32 winhttp winmm xaudio2)\n'
write(h/'CMakeLists.txt',c)
run('configure',[cmake,'-S',h,'-B',out/'build','-G','Visual Studio 17 2022','-A','x64'])
run('build',[cmake,'--build',out/'build','--config','Debug','--target','frozen_mist','--parallel','2'])
write(out/'input-identities.json',json.dumps(dict(source_head=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),baseline=record,diagnostic_sources={str(p.relative_to(out)):sha(p) for p in h.rglob('*') if p.is_file()},native_libraries={str(p.relative_to(root)):sha(p) for p in libs}),indent=2)+'\n')
