#include "scene/assets/SkinnedMeshAsset.h"
#include "scene/assets/StaticMeshAsset.h"
#include "scene/assets/SkinnedPbrActor.h"
#include "scene/CompanionPose.h"
#include "gameplay/dialogue/CompanionRouteFixture.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <cstring>
#include <tuple>

namespace {
bool CheckAnalyticTangentFrames() {
    using namespace horde::scene;
    SkinnedClipSet clips{};
    clips.clips[0]={"rotate",false,true};
    clips.clips[1]={"reflect",false,true};
    for(std::size_t i=2;i<clips.clips.size();++i) clips.clips[i]={"",false,false};
    SkinnedMeshAsset model;std::string diagnostic;
    if(!model.LoadClips(HORDE_SKINNED_PBR_FIXTURE,clips,diagnostic)) {
        std::cerr<<"Analytic skin fixture import failed: "<<diagnostic<<'\n';return false;
    }
    std::vector<TexturedSkinnedRtVertex> vertices;
    std::vector<SkinnedPbrTangent> tangents;
    for(const auto clip:{SkinnedClip::Idle,SkinnedClip::Walking}) {
        if(!model.SkinUniquePbr(clip,1,vertices,tangents,diagnostic)||vertices.size()!=3||tangents.size()!=3) return false;
        const bool reflection=clip==SkinnedClip::Walking;
        const std::array<float,4> expected=reflection?std::array<float,4>{-1,0,0,-1}:std::array<float,4>{0,1,0,1};
        for(const auto& frame:tangents) for(std::size_t i=0;i<4;++i)
            if(std::abs(frame.tangent[i]-expected[i])>1e-5f) {
                std::cerr<<"Analytic rotated/reflected tangent mismatch: reflection="<<reflection<<" component="<<i<<'\n';return false;
            }
    }
    // The reflected clip crosses a singular scale: reject it without partial output.
    if(model.SkinUniquePbr(SkinnedClip::Walking,.5f,vertices,tangents,diagnostic)||!vertices.empty()||!tangents.empty()) return false;
    return true;
}

template<class T> bool SameBytes(const std::vector<T>& a,const std::vector<T>& b) {
    return a.size()==b.size() && (a.empty() || std::memcmp(a.data(),b.data(),a.size()*sizeof(T))==0);
}

bool SameActorContract(const horde::scene::assets::StaticMeshAsset& a,
                       const horde::scene::assets::StaticMeshAsset& b) {
    if(a.indices!=b.indices || a.triangleRegions!=b.triangleRegions || a.vertices.size()!=b.vertices.size() ||
       a.primitives.size()!=b.primitives.size() || a.materials.size()!=b.materials.size() ||
       a.nodeTransforms.size()!=b.nodeTransforms.size() || a.sockets.size()!=b.sockets.size())return false;
    for(std::size_t i=0;i<a.vertices.size();++i)if(a.vertices[i].uv0!=b.vertices[i].uv0)return false;
    for(std::size_t i=0;i<a.primitives.size();++i) {
        const auto fields=[](const auto& p){return std::tie(p.vertexOffset,p.indexOffset,p.indexCount,p.materialIndex,p.nodeTransformIndex);};
        if(fields(a.primitives[i])!=fields(b.primitives[i]))return false;
    }
    for(std::size_t i=0;i<a.materials.size();++i) {
        const auto fields=[](const auto& m){return std::tie(m.name,m.baseColorFactor,m.emissiveFactor,m.attenuationColor,
            m.emissiveStrength,m.metallicFactor,m.roughnessFactor,m.occlusionStrength,m.normalScale,m.textureScale,
            m.transmissionFactor,m.ior,m.thicknessFactor,m.attenuationDistance,m.numericalSpawnMinimumWidth,
            m.numericalSpawnGeometryError,m.baseColorTexture,m.normalTexture,m.ormTexture,m.emissiveTexture,
            m.textureGroup,m.deferTextureAllocation,m.flags);};
        if(fields(a.materials[i])!=fields(b.materials[i]))return false;
    }
    for(std::size_t i=0;i<a.nodeTransforms.size();++i)
        if(a.nodeTransforms[i].name!=b.nodeTransforms[i].name || a.nodeTransforms[i].world!=b.nodeTransforms[i].world)return false;
    for(std::size_t i=0;i<a.sockets.size();++i)
        if(a.sockets[i].name!=b.sockets[i].name || a.sockets[i].nodeTransformIndex!=b.sockets[i].nodeTransformIndex ||
           a.sockets[i].world!=b.sockets[i].world)return false;
    return true;
}

bool SameAdmittedActor(const horde::scene::assets::StaticMeshAsset& a,
                       const horde::scene::assets::StaticMeshAsset& b) {
    return SameActorContract(a,b) && SameBytes(a.vertices,b.vertices) &&
        a.bounds.minimum==b.bounds.minimum && a.bounds.maximum==b.bounds.maximum;
}

bool ValidActorFrames(const horde::scene::assets::StaticMeshAsset& actor) {
    std::array<float,3> minimum{},maximum{};
    minimum.fill(std::numeric_limits<float>::infinity());maximum.fill(-std::numeric_limits<float>::infinity());
    for(const auto& v:actor.vertices) {
        for(std::size_t axis=0;axis<4;++axis)
            if(!std::isfinite(v.position[axis]) || !std::isfinite(v.normal[axis]) ||
               !std::isfinite(v.tangent[axis]) || !std::isfinite(v.uv0[axis]))return false;
        float normalLength=0,tangentLength=0,dot=0;
        for(std::size_t axis=0;axis<3;++axis) {
            minimum[axis]=std::min(minimum[axis],v.position[axis]);maximum[axis]=std::max(maximum[axis],v.position[axis]);
            normalLength+=v.normal[axis]*v.normal[axis];tangentLength+=v.tangent[axis]*v.tangent[axis];
            dot+=v.normal[axis]*v.tangent[axis];
        }
        if(std::abs(normalLength-1)>1e-4f || std::abs(tangentLength-1)>1e-4f ||
           std::abs(dot)>1e-4f || std::abs(v.tangent[3])!=1)return false;
    }
    return !actor.vertices.empty() && minimum==actor.bounds.minimum && maximum==actor.bounds.maximum &&
        std::abs(minimum[1])<1e-5f && maximum[1]>=1.4f && maximum[1]<=2.3f;
}

bool CheckCompanionRig(horde::scene::assets::SkinnedPbrActor& actor,
                       const std::filesystem::path& path,const horde::scene::SkinnedClipSet& clips) {
    using namespace horde::scene;
    namespace dialogue=horde::gameplay::dialogue;
    const auto check=[](bool ok,const char* message){if(!ok)std::cerr<<"FAIL: Kit acting rig: "<<message<<'\n';return ok;};
    SkinnedMeshAsset skin;std::string diagnostic;
    if(!check(skin.LoadClips(path.string(),clips,diagnostic),"native indexed Kit rig imports for additive regression"))return false;
    constexpr std::array<std::string_view,6> names{"Head","headfront","Spine","Hips","LeftFoot","RightFoot"};
    std::array<SkinnedNodeTransform,names.size()> nodes{};
    if(!check(skin.NodeTransforms(SkinnedClip::Idle,0,names,nodes,diagnostic),"native idle-zero semantic nodes resolve"))return false;
    std::array<float,3> front{};float frontLength=0;
    for(std::size_t axis=0;axis<3;++axis){front[axis]=nodes[1][12+axis]-nodes[0][12+axis];frontLength+=front[axis]*front[axis];}
    frontLength=std::sqrt(frontLength);
    if(!check(std::isfinite(frontLength) && frontLength>1e-5f,"Head to headfront is a finite nondegenerate native direction"))return false;
    for(auto& value:front)value/=frontLength;
    if(!check(front[2]>.999f,"actual native Kit idle faces imported positive Z"))return false;
    constexpr float pi=3.14159265359f;
    for(float logical:{0.f,pi*.5f,-pi*.5f,pi}) {
        const float model=CompanionModelYaw(logical),c=std::cos(model),s=std::sin(model);
        const std::array<float,3> worldFront{c*front[0]+s*front[2],front[1],-s*front[0]+c*front[2]};
        const float agreement=worldFront[0]*std::sin(logical)-worldFront[2]*std::cos(logical);
        // Columns of the production yaw's proper Y rotation; a reflection or
        // scale cannot repair the historical mirrored X heading.
        const float determinant=c*c+s*s;
        if(!check(agreement>.999f && std::abs(determinant-1)<1e-5f,
            "four cardinal production model yaws preserve handedness and face logical forward"))return false;
    }

    constexpr float phase=0;
    if(!check(actor.Pose(SkinnedClip::Idle,phase,diagnostic),"plain idle phase is admitted"))return false;
    const auto plain=actor.Asset();
    std::vector<TexturedSkinnedRtVertex> rawPlain,rawNeutral;
    std::vector<SkinnedPbrTangent> tangentPlain,tangentNeutral;
    const auto neutral=CompanionUpperBodyRotations({});
    if(!check(skin.SkinUniquePbr(SkinnedClip::Idle,phase,rawPlain,tangentPlain,diagnostic) &&
        skin.SkinUniquePbr(SkinnedClip::Idle,phase,rawNeutral,tangentNeutral,diagnostic,neutral) &&
        SameBytes(rawPlain,rawNeutral) && SameBytes(tangentPlain,tangentNeutral) &&
        actor.Pose(SkinnedClip::Idle,phase,diagnostic,neutral) && SameAdmittedActor(plain,actor.Asset()),
        "neutral production recipe preserves exact native PBR vertices tangents and actor asset"))return false;

    // Select peaks from the real transient controller rather than inventing a
    // golden deformation. Both recipes are applied to the same authored idle.
    std::array<dialogue::CompanionActingPose,2> peaks{};
    for(std::size_t recipe=0;recipe<peaks.size();++recipe) {
        dialogue::CompanionActing acting;dialogue::Snapshot speech{};speech.duration=2;
        speech.line=recipe==0?dialogue::Line::ReunionQuestion:dialogue::Line::None;
        speech.proofAccepted=recipe==1;float maximum=0;
        for(unsigned tick=1;tick<=120;++tick) {
            speech.elapsed=static_cast<float>(tick)/60;
            acting.Step(1.f/60,speech,true,true,true,false,recipe==1 && tick==1);
            const auto pose=acting.State();const float magnitude=std::abs(pose.headPitch)+std::abs(pose.headYaw)+std::abs(pose.chestPitch);
            if(magnitude>maximum){maximum=magnitude;peaks[recipe]=pose;}
        }
        if(!check(maximum>1e-5f,"production speaking and recognition each produce a nonneutral peak"))return false;
    }
    for(std::size_t recipe=0;recipe<peaks.size();++recipe) {
        const auto rotations=CompanionUpperBodyRotations(peaks[recipe]);
        if(!check(actor.Pose(SkinnedClip::Idle,phase,diagnostic,rotations),"production peak recipe skins on the real Kit rig"))return false;
        const auto& posed=actor.Asset();
        if(!check(SameActorContract(plain,posed) && ValidActorFrames(posed) && skin.ValidateStaticVertexLayout(posed,diagnostic),
            "acting preserves topology UV material bindings node palette ground bounds and normalized PBR frames"))return false;
        float maximumDisplacement=0;std::size_t upperChanged=0,headChanged=0,chestChanged=0,lowVertices=0,leftBoot=0,rightBoot=0;
        for(std::size_t i=0;i<posed.vertices.size();++i) {
            const auto& before=plain.vertices[i];const auto& after=posed.vertices[i];float distance=0;
            for(std::size_t axis=0;axis<3;++axis){const float d=after.position[axis]-before.position[axis];distance+=d*d;}
            distance=std::sqrt(distance);maximumDisplacement=std::max(maximumDisplacement,distance);
            if(before.position[1]>plain.bounds.maximum[1]*.65f && distance>1e-5f)++upperChanged;
            if(before.position[1]>plain.bounds.maximum[1]*.85f && distance>1e-5f)++headChanged;
            if(before.position[1]>plain.bounds.maximum[1]*.65f && before.position[1]<plain.bounds.maximum[1]*.8f &&
               distance>1e-5f)++chestChanged;
            // A conservative boot/low-shin band, far below the hip and chest
            // weighting boundary; count both sides so an empty subset cannot pass.
            if(before.position[1]<.25f) {
                ++lowVertices;if(before.position[0]<-.05f)++leftBoot;if(before.position[0]>.05f)++rightBoot;
                if(!check(std::memcmp(&before,&after,sizeof(before))==0,
                    "upper-body acting leaves every low boot/shin vertex normal tangent and UV exactly planted"))return false;
            }
        }
        if(!check(upperChanged>0 && headChanged>0 && chestChanged>0 && lowVertices>0 && leftBoot>0 && rightBoot>0 &&
            maximumDisplacement>1e-5f && maximumDisplacement<.25f,
            "acting moves the upper silhouette within a modest quarter-metre envelope while both boots remain fixed"))return false;
        std::cout<<"Kit native acting recipe="<<(recipe==0?"speaking-peak":"recognition-peak")
            <<" upper_changed="<<upperChanged<<" planted_low_vertices="<<lowVertices
            <<" maximum_displacement_m="<<maximumDisplacement<<'\n';
    }

    const auto admitted=actor.Asset();
    std::array<std::array<SkinnedNodeRotation,2>,3> invalid{};
    for(auto& rotations:invalid)rotations=CompanionUpperBodyRotations(peaks[1]);
    invalid[0][1].nodeName="MissingKitUpperChest";
    invalid[1][1].rotation[0]=std::numeric_limits<float>::quiet_NaN();
    invalid[2][1].rotation[1]=std::numeric_limits<float>::infinity();
    for(const auto& rotations:invalid) {
        rawNeutral=rawPlain;tangentNeutral=tangentPlain;
        if(!check(!skin.SkinUniquePbr(SkinnedClip::Idle,phase,rawNeutral,tangentNeutral,diagnostic,rotations) &&
            rawNeutral.empty() && tangentNeutral.empty() && !diagnostic.empty(),
            "invalid additive node or nonfinite quaternion rejects without partial indexed PBR output"))return false;
        if(!check(!actor.Pose(SkinnedClip::Idle,phase,diagnostic,rotations) && !diagnostic.empty() && SameAdmittedActor(admitted,actor.Asset()),
            "invalid additive pose leaves the last admitted complete actor asset unchanged"))return false;
    }
    std::array<SkinnedNodeTransform,names.size()> afterNodes{};
    return check(skin.NodeTransforms(SkinnedClip::Idle,phase,names,afterNodes,diagnostic) && nodes==afterNodes &&
        actor.Pose(SkinnedClip::Idle,phase,diagnostic) && SameAdmittedActor(plain,actor.Asset()),
        "acting and rejected requests never mutate the imported node palette and neutral posing remains exact");
}

bool CheckCompanionBlend(horde::scene::assets::SkinnedPbrActor& actor,
                         const std::filesystem::path& path,const horde::scene::SkinnedClipSet& clips) {
    using namespace horde::scene;
    namespace dialogue=horde::gameplay::dialogue;
    const auto check=[](bool ok,const char* message){if(!ok)std::cerr<<"FAIL: Kit clip blend: "<<message<<'\n';return ok;};
    SkinnedMeshAsset skin;std::string diagnostic;
    if(!check(skin.LoadClips(path.string(),clips,diagnostic),"actual Kit rig imports"))return false;
    constexpr float idleTime=.37f,walkTime=.23f;
    const std::array<dialogue::CompanionActingPose,2> recipes{{{}, {.08f,.02f,.02f,false}}};
    for(const auto& recipe:recipes) {
        const auto acting=CompanionUpperBodyRotations(recipe);
        std::vector<TexturedSkinnedRtVertex> base,target,blended;
        std::vector<SkinnedPbrTangent> baseTangents,targetTangents,blendTangents;
        if(!check(skin.SkinUniquePbr(SkinnedClip::Idle,idleTime,base,baseTangents,diagnostic,acting) &&
            skin.SkinUniquePbr(SkinnedClip::Walking,walkTime,target,targetTangents,diagnostic,acting) &&
            actor.Pose(SkinnedClip::Idle,idleTime,diagnostic,acting),"both authored endpoint poses admit acting"))return false;
        const auto baseActor=actor.Asset();
        if(!check(actor.Pose(SkinnedClip::Walking,walkTime,diagnostic,acting),"walk endpoint actor admits acting"))return false;
        const auto targetActor=actor.Asset();
        for(float weight:{0.f,1.f}) {
            const SkinnedClipBlend blend{SkinnedClip::Walking,walkTime,weight};
            if(!check(skin.SkinUniquePbr(SkinnedClip::Idle,idleTime,blended,blendTangents,diagnostic,acting,&blend) &&
                SameBytes(blended,weight==0?base:target) && SameBytes(blendTangents,weight==0?baseTangents:targetTangents) &&
                actor.Pose(SkinnedClip::Idle,idleTime,diagnostic,acting,&blend) &&
                SameAdmittedActor(actor.Asset(),weight==0?baseActor:targetActor),
                "blend weights zero and one preserve exact authored vertices tangents and grounded actor including acting"))return false;
        }
        for(float weight:{.25f,.5f,.75f}) {
            const SkinnedClipBlend blend{SkinnedClip::Walking,walkTime,weight};
            if(!check(actor.Pose(SkinnedClip::Idle,idleTime,diagnostic,acting,&blend) &&
                SameActorContract(baseActor,actor.Asset()) && ValidActorFrames(actor.Asset()) &&
                skin.ValidateStaticVertexLayout(actor.Asset(),diagnostic) &&
                !SameBytes(actor.Asset().vertices,baseActor.vertices) && !SameBytes(actor.Asset().vertices,targetActor.vertices),
                "intermediate joint blend changes the pose while retaining normalized PBR frames UV topology material and grounded bounds"))return false;
        }
    }
    const auto admitted=actor.Asset();
    std::array<SkinnedClipBlend,8> invalid{};
    invalid.fill({SkinnedClip::Walking,walkTime,.5f});
    invalid[0].weight=-.01f;invalid[1].weight=1.01f;
    invalid[2].weight=std::numeric_limits<float>::quiet_NaN();invalid[3].weight=std::numeric_limits<float>::infinity();
    invalid[4].clip=SkinnedClip::Attack;invalid[5].clip=static_cast<SkinnedClip>(255);
    invalid[6].timeSeconds=std::numeric_limits<float>::quiet_NaN();invalid[7].timeSeconds=std::numeric_limits<float>::infinity();
    for(const auto& blend:invalid) {
        std::vector<TexturedSkinnedRtVertex> vertices(1);std::vector<SkinnedPbrTangent> tangents(1);
        if(!check(!skin.SkinUniquePbr(SkinnedClip::Idle,idleTime,vertices,tangents,diagnostic,{},&blend) &&
            vertices.empty() && tangents.empty() && !diagnostic.empty() &&
            !actor.Pose(SkinnedClip::Idle,idleTime,diagnostic,{},&blend) && !diagnostic.empty() &&
            SameAdmittedActor(admitted,actor.Asset()),
            "invalid blend weight clip or time rejects without partial output or changing last admitted actor"))return false;
    }

    // Native named-node confirmation of the independently measured boot stance
    // windows. Bone midpoint and boot centroid are different contact proxies;
    // retain native speeds, rather than freezing a golden deformation value.
    const float duration=skin.ClipDuration(SkinnedClip::Walking);
    if(!check(std::isfinite(duration) && duration>0,"native walk duration is finite"))return false;
    std::array<double,2> stanceSpeeds{};
    for(unsigned side=0;side<2;++side) {
        const std::array<std::string_view,2> names=side==0?
            std::array<std::string_view,2>{"LeftFoot","LeftToeBase"}:std::array<std::string_view,2>{"RightFoot","RightToeBase"};
        const float begin=side==0?.28f:.66f,end=side==0?.62f:1.02f;
        std::array<double,25> times{},z{};double meanT=0,meanZ=0;
        for(std::size_t i=0;i<times.size();++i) {
            const float t=begin+(end-begin)*static_cast<float>(i)/(times.size()-1);
            std::array<SkinnedNodeTransform,2> nodes{};
            if(!check(skin.NodeTransforms(SkinnedClip::Walking,t,names,nodes,diagnostic),"native foot and toe stance samples resolve including cycle wrap"))return false;
            times[i]=t;z[i]=(double(nodes[0][14])+nodes[1][14])*.5;meanT+=times[i];meanZ+=z[i];
        }
        meanT/=times.size();meanZ/=times.size();double covariance=0,variance=0;
        for(std::size_t i=0;i<times.size();++i){covariance+=(times[i]-meanT)*(z[i]-meanZ);variance+=(times[i]-meanT)*(times[i]-meanT);}
        stanceSpeeds[side]=-covariance/variance;
        if(!check(std::isfinite(stanceSpeeds[side]) && stanceSpeeds[side]>0,
            "native low-foot stance travels rearward in imported forward space"))return false;
    }
    const double meanSpeed=(stanceSpeeds[0]+stanceSpeeds[1])*.5;
    const double expected=dialogue::CompanionLocomotion::kWalkCycleDistanceMetres/duration;
    if(!check(std::abs(meanSpeed-expected)<expected*.15,
        "independent native foot-node fit supports the measured distance-per-cycle within contact-proxy uncertainty"))return false;
    std::cout<<"Kit native stance duration="<<duration<<" left_mps="<<stanceSpeeds[0]<<" right_mps="<<stanceSpeeds[1]
        <<" inferred_cycle_m="<<meanSpeed*duration<<" pinned_visual_cycle_m="<<dialogue::CompanionLocomotion::kWalkCycleDistanceMetres<<'\n';

    // Generate start/stop snapshots through the actual route owner. This small
    // blockout support fixture is not a native chapter/terrain acceptance run.
    struct Frame {dialogue::CompanionSnapshot pose;float legacySeconds=0;unsigned stage=0;};
    dialogue::CompanionRouteFixture route;std::vector<Frame> frames;
    float legacySeconds=0;bool legacyWalking=false;
    constexpr float dt=1.f/60;
    const auto advance=[&](bool lagging,unsigned stage,bool retain){
        const auto before=route.State();
        route.Step(dt,before.x+(lagging?7.f:0),before.z,true,true,false);
        const auto state=route.State();
        legacySeconds=legacyWalking==state.walking?legacySeconds+dt:0;legacyWalking=state.walking;
        if(retain)frames.push_back({state,legacySeconds,stage});
        return state.grounded && state.walking!=lagging &&
            (lagging || std::abs(std::hypot(state.x-before.x,state.z-before.z)-1.8f*dt)<1e-4f);
    };
    for(unsigned i=0;i<20;++i)if(!check(advance(false,0,i==19),"production route primes unchanged accepted walking distance"))return false;
    for(unsigned i=0;i<12;++i)if(!check(advance(true,1,true),"production player-lagging stop retains grounded route root"))return false;
    for(unsigned i=0;i<12;++i)if(!check(advance(false,2,true),"production route resumes unchanged accepted walking distance"))return false;
    if(!check(frames[12].pose.locomotion.walkWeight==0 && frames.back().pose.locomotion.walkWeight==1,
        "production stop/start snapshots reach both exact blend endpoints"))return false;
    if(!check(actor.Pose(SkinnedClip::Idle,0,diagnostic),"boot subset reference admits"))return false;
    const auto contract=actor.Asset();std::vector<std::size_t> boots;
    for(std::size_t i=0;i<contract.vertices.size();++i)if(contract.vertices[i].position[1]<.25f)boots.push_back(i);
    if(!check(!boots.empty(),"nonempty low-boot subset is compared through route transitions"))return false;
    using Point=std::array<float,3>;
    const auto worldBoots=[&](const dialogue::CompanionSnapshot& pose){
        std::vector<Point> output;output.reserve(boots.size());
        const float yaw=CompanionModelYaw(pose.yaw),c=std::cos(yaw),s=std::sin(yaw);
        for(const auto index:boots){const auto& p=actor.Asset().vertices[index].position;
            output.push_back({c*p[0]+s*p[2]+pose.x,p[1]+pose.y,-s*p[0]+c*p[2]+pose.z});}
        return output;
    };
    const auto jump=[](const std::vector<Point>& previous,const std::vector<Point>& current){
        float maximum=0;
        for(std::size_t i=0;i<current.size();++i){float square=0;for(std::size_t axis=0;axis<3;++axis){const float d=current[i][axis]-previous[i][axis];square+=d*d;}maximum=std::max(maximum,std::sqrt(square));}
        return maximum;
    };
    std::array<float,3> oldPeak{},newPeak{};std::vector<Point> oldPrevious,newPrevious;
    for(const auto& frame:frames) {
        const auto& pose=frame.pose;
        const SkinnedClipBlend blend{SkinnedClip::Walking,pose.locomotion.walkPhase*duration,pose.locomotion.walkWeight};
        if(!check(actor.Pose(SkinnedClip::Idle,pose.animationSeconds,diagnostic,{},&blend) &&
            SameActorContract(contract,actor.Asset()) && ValidActorFrames(actor.Asset()),"production snapshot admits one grounded blended PBR pose"))return false;
        const auto newCurrent=worldBoots(pose);
        if(!check(actor.Pose(pose.walking?SkinnedClip::Walking:SkinnedClip::Idle,frame.legacySeconds,diagnostic),
            "pre-change hard-reset comparison pose admits on the same rig"))return false;
        const auto oldCurrent=worldBoots(pose);
        if(!newPrevious.empty()) {
            newPeak[frame.stage]=std::max(newPeak[frame.stage],jump(newPrevious,newCurrent));
            oldPeak[frame.stage]=std::max(oldPeak[frame.stage],jump(oldPrevious,oldCurrent));
        }
        newPrevious=newCurrent;oldPrevious=oldCurrent;
    }
    for(unsigned stage:{1u,2u}) {
        std::cout<<"Kit native route transition="<<(stage==1?"stop":"start")<<" frames=12 hard_reset_peak_boot_m="
            <<oldPeak[stage]<<" joint_blend_peak_boot_m="<<newPeak[stage]<<'\n';
        if(!check(newPeak[stage]+1e-4f<oldPeak[stage],
            "same-root native start and stop each reduce worst per-frame boot displacement versus hard reset"))return false;
    }
    return true;
}
}

int main() {
    using namespace horde::scene;
    constexpr std::array<const char*,7> clips{{"Warden|idle","Warden|walk","Warden|run",
        "Warden|jump","Warden|fall","Warden|slide","Warden|mantle"}};
    std::size_t poses=0;int failures=CheckAnalyticTangentFrames()?0:1;
    const std::filesystem::path runtimeDirectory=HORDE_KIT_RUNTIME_DIRECTORY;
    const auto runtimePath=runtimeDirectory/"kit-warden-lod0.runtime.glb";
    assets::AssetManifest manifest;assets::StaticMeshAsset staticAsset;std::string admission;
    if(!assets::AssetManifest::Load(runtimeDirectory/"asset.manifest.json",manifest,admission)||
       !assets::StaticMeshAsset::Load(runtimePath,manifest,staticAsset,admission)||
       staticAsset.vertices.size()!=26945u||staticAsset.indices.size()!=77904u||
       staticAsset.materials.size()!=1u) {
        std::cerr<<"Native Kit PBR admission failed: "<<admission<<'\n';return 1;
    }
    const auto& material=staticAsset.materials[0];
    assets::SkinnedPbrActor actor;
    SkinnedClipSet actorClips{};
    actorClips.clips[0]={"Warden|idle",true,true};
    actorClips.clips[1]={"Warden|walk",true,true};
    for(std::size_t i=2;i<actorClips.clips.size();++i) actorClips.clips[i]={"",false,false};
    if(!actor.Load(runtimeDirectory,"kit-warden-lod0.runtime.glb",actorClips,admission)) {
        std::cerr<<"Scene actor import failed: "<<admission<<'\n';return 1;
    }
    if(!CheckCompanionRig(actor,runtimePath,actorClips))++failures;
    if(!CheckCompanionBlend(actor,runtimePath,actorClips))++failures;
    for(const auto clip:{SkinnedClip::Idle,SkinnedClip::Walking}) {
        for(float fraction:{0.f,.25f,.5f,.75f}) {
            if(!actor.Pose(clip,fraction*actor.Duration(clip),admission)) return 1;
            const auto& pose=actor.Asset();
            if(std::abs(pose.bounds.minimum[1])>1e-5f ||
               pose.bounds.maximum[1]<1.4f || pose.bounds.maximum[1]>2.3f ||
               pose.indices!=staticAsset.indices) {
                std::cerr<<"Actor must retain indexed topology and human-scale ground contact\n";return 1;
            }
            for(const auto& node:pose.nodeTransforms)
                if(node.world!=std::array<float,16>{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}) return 1;
            for(std::size_t i=0;i<pose.vertices.size();++i)
                if(pose.vertices[i].uv0!=staticAsset.vertices[i].uv0) return 1;
        }
    }
    if(material.baseColorTexture<0||material.normalTexture<0||material.ormTexture<0||
       material.emissiveTexture<0||std::abs(material.normalScale-.7f)>1e-5f||
       std::abs(material.emissiveFactor[0]-.55f)>1e-5f) {
        std::cerr<<"Kit's four authored PBR maps/factors must survive native import\n";return 1;
    }
    for(const auto name:clips) {
        SkinnedMeshAsset model;SkinnedClipSet bindings{};
        bindings.clips[0]={name,false,true};
        for(std::size_t i=1;i<bindings.clips.size();++i) bindings.clips[i]={"",false,false};
        std::string diagnostic;
        if(!model.LoadClips(HORDE_KIT_SOURCE_CANDIDATE,bindings,diagnostic)) {
            std::cerr<<"Native clip import rejected "<<name<<": "<<diagnostic<<'\n';++failures;continue;
        }
        SkinnedMeshAsset runtimeModel;
        if(!runtimeModel.LoadClips(runtimePath.string(),bindings,diagnostic)||
           !runtimeModel.ValidateStaticVertexLayout(staticAsset,diagnostic)) {
            std::cerr<<"Runtime Kit skin/static layout rejected: "<<diagnostic<<'\n';return 1;
        }
        const float duration=model.ClipDuration(SkinnedClip::Idle);
        if(!std::isfinite(duration)||duration<=0||!model.HasTexcoords()||!model.HasTangents()) {
            std::cerr<<"Missing finite clip duration/UV/tangent streams "<<name<<'\n';++failures;continue;
        }
        for(float fraction:{0.f,.25f,.5f,.75f,1.f}) {
            std::vector<TexturedSkinnedRtVertex> unique,reference;
            std::vector<SkinnedPbrTangent> tangents;
            if (!model.SkinUniqueTextured(SkinnedClip::Idle,fraction*duration,reference,diagnostic) ||
                !model.SkinUniquePbr(SkinnedClip::Idle,fraction*duration,unique,tangents,diagnostic) ||
                unique.size()!=reference.size() || tangents.size()!=unique.size() ||
                std::memcmp(unique.data(),reference.data(),unique.size()*sizeof(unique[0]))!=0) {
                std::cerr<<"PBR skin must preserve native unique positions/normals/UVs and supply tangents "<<name<<'\n';
                ++failures;
            } else {
                for(std::size_t v=0;v<unique.size();++v) {
                    const auto& t=tangents[v].tangent;const auto& n=unique[v].normal;
                    const float length=std::sqrt(t[0]*t[0]+t[1]*t[1]+t[2]*t[2]);
                    const float dot=t[0]*n[0]+t[1]*n[1]+t[2]*n[2];
                    if(!std::isfinite(length)||std::abs(length-1)>1e-4f||
                       !std::isfinite(dot)||std::abs(dot)>1e-4f||std::abs(t[3])!=1) {
                        std::cerr<<"PBR skin tangent frame is invalid "<<name<<'\n';++failures;break;
                    }
                }
                std::vector<TexturedSkinnedRtVertex> repeated;
                std::vector<SkinnedPbrTangent> repeatedTangents;
                if(!model.SkinUniquePbr(SkinnedClip::Idle,fraction*duration,repeated,repeatedTangents,diagnostic)||
                   repeated.size()!=unique.size()||repeatedTangents.size()!=tangents.size()||
                   std::memcmp(repeated.data(),unique.data(),unique.size()*sizeof(unique[0]))||
                   std::memcmp(repeatedTangents.data(),tangents.data(),tangents.size()*sizeof(tangents[0]))) ++failures;
                if(!runtimeModel.SkinUniquePbr(SkinnedClip::Idle,fraction*duration,repeated,repeatedTangents,diagnostic)||
                   repeated.size()!=unique.size()||repeatedTangents.size()!=tangents.size()||
                   std::memcmp(repeated.data(),unique.data(),unique.size()*sizeof(unique[0]))||
                   std::memcmp(repeatedTangents.data(),tangents.data(),tangents.size()*sizeof(tangents[0]))) {
                    std::cerr<<"Runtime derivative changed a native source skin pose/tangent\n";++failures;
                }
            }
            std::vector<TexturedSkinnedRtVertex> vertices;
            if(!model.SkinTextured(SkinnedClip::Idle,fraction*duration,vertices,diagnostic)||vertices.empty()) {
                std::cerr<<"Native skin failed "<<name<<": "<<diagnostic<<'\n';++failures;continue;
            }
            if(vertices.size()!=staticAsset.indices.size()) ++failures;
            else for(std::size_t v=0;v<vertices.size();++v) {
                const auto index=staticAsset.indices[v];
                if(index>=unique.size() || std::memcmp(&vertices[v],&unique[index],sizeof(vertices[v]))) {
                    std::cerr<<"Expanded skin differs from indexed PBR skin\n";++failures;break;
                }
            }
            std::array<float,3> minimum{},maximum{};
            minimum.fill(std::numeric_limits<float>::infinity());
            maximum.fill(-std::numeric_limits<float>::infinity());
            bool finite=true;
            for(const auto& vertex:vertices) for(std::size_t axis=0;axis<3;++axis) {
                finite&=std::isfinite(vertex.position[axis])&&std::isfinite(vertex.normal[axis])&&
                    std::isfinite(vertex.texcoord[axis<2?axis:0]);
                minimum[axis]=std::min(minimum[axis],vertex.position[axis]);
                maximum[axis]=std::max(maximum[axis],vertex.position[axis]);
            }
            if(!finite||maximum[1]-minimum[1]<=.01f) ++failures;
            std::cout<<name<<" phase="<<fraction<<" duration="<<duration<<" vertices="<<vertices.size()
                <<" height="<<maximum[1]-minimum[1]<<" minY="<<minimum[1]<<" maxY="<<maximum[1]<<'\n';++poses;
        }
        std::vector<TexturedSkinnedRtVertex> rejected(1);
        std::vector<SkinnedPbrTangent> rejectedTangents(1);
        for(float badTime:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()})
            if(model.SkinUniquePbr(SkinnedClip::Idle,badTime,rejected,rejectedTangents,diagnostic)||
               !rejected.empty()||!rejectedTangents.empty()) ++failures;
        if(model.SkinUniquePbr(static_cast<SkinnedClip>(255),0,rejected,rejectedTangents,diagnostic)||
           !rejected.empty()||!rejectedTangents.empty()) ++failures;
    }
    std::cout<<"Kit source intake clips=7 sampled_poses="<<poses<<" failures="<<failures
        <<" runtime_admission=NO\n";
    return failures?1:0;
}
