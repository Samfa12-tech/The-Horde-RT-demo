#pragma once
#include "KitTurnContactData.h"
#include <iomanip>
#include <sstream>

// Optional measurement entry, not an automatic artistic/contact admission.
inline int MeasureKitTurnContact(const std::filesystem::path& candidate,
                                 const std::filesystem::path& output) {
    using namespace horde::scene;
    if(std::filesystem::exists(output) || !std::filesystem::create_directories(output))return 2;
    SkinnedClipSet clips{};
    clips.clips={{{"Warden|idle",true,true},{"Warden|walk",true,true},
                  {"KitProof|turn_left",false,true},{"KitProof|turn_right",false,true}}};
    assets::SkinnedPbrActor actor;std::string error;
    if(!actor.Load(candidate.parent_path(),candidate.filename().string().c_str(),clips,error)) {
        std::cerr<<error<<'\n';return 1;
    }
    const auto idle=actor.Asset().vertices;
    if(idle.size()!=26945 || actor.Duration(SkinnedClip::Attack)!=.75f || actor.Duration(SkinnedClip::Dead)!=.75f)return 1;
    const auto rotate=[](const auto& p,float yaw) {
        const float c=std::cos(yaw),s=std::sin(yaw);
        return std::array<float,3>{c*p[0]+s*p[2],p[1],-s*p[0]+c*p[2]};
    };
    std::ofstream csv(output/"sole-samples.csv");csv<<std::setprecision(9)
        <<"direction,degrees,sample,seconds,phase,foot,authoredPhase,maxReferenceHorizontalOffsetM,meanOffsetXM,meanOffsetZM,maxReferenceHeightDeltaM,soleMinY,soleMaxY,touchVertices1um,contactRunSlip1umM,touchVertices01mm,contactRunSlip01mmM,touchVertices1mm,contactRunSlip1mmM\n";
    std::ostringstream json;json<<std::setprecision(9)<<"{\"schema\":1,\"evidenceClass\":\"120 Hz native grounded PBR sole measurement; not automatic contact admission\",\"runs\":[";
    bool firstRun=true;float endpointMax=0;
    for(unsigned side=0;side<2;++side)for(float degrees:{0.f,2.f,10.f,22.5f,45.f}) {
        const float amplitude=degrees/45.f,finalYaw=kit_turn_contact::Yaw(side!=0,1)*amplitude;
        float worstDrift=0,worstHeight=0,worstMean=0;
        struct Contact {bool touching=false;unsigned ticks=0;float x=0,z=0,lastX=0,lastZ=0,path=0;};
        struct Band {float maxSlip=0,maxPath=0;unsigned longest=0,touchSamples=0,runs=0;};
        constexpr std::array<float,3> heights{.000001f,.0001f,.001f};
        std::array<std::vector<Contact>,6> contacts;
        std::array<Band,3> bands{};
        for(unsigned band=0;band<3;++band) {contacts[band].resize(kit_turn_contact::leftSole.size());
            contacts[3+band].resize(kit_turn_contact::rightSole.size());}
        for(unsigned sample=0;sample<=90;++sample) {
            const float phase=sample/90.f,seconds=phase*.75f,yaw=kit_turn_contact::Yaw(side!=0,phase)*amplitude;
            const SkinnedClipBlend blend{side?SkinnedClip::Dead:SkinnedClip::Attack,seconds,amplitude};
            if(!actor.Pose(SkinnedClip::Idle,0,error,{},&blend)){std::cerr<<error<<'\n';return 1;}
            const auto& vertices=actor.Asset().vertices;
            if(sample==0 || sample==90)for(std::size_t i=0;i<idle.size();++i) {
                float squared=0;for(unsigned axis=0;axis<3;++axis) {
                    const float delta=vertices[i].position[axis]-idle[i].position[axis];squared+=delta*delta;
                }
                endpointMax=std::max(endpointMax,std::sqrt(squared));
            }
            for(unsigned foot=0;foot<2;++foot) {
                const auto indices=foot?std::span<const std::uint32_t>{kit_turn_contact::rightSole}:
                    std::span<const std::uint32_t>{kit_turn_contact::leftSole};
                const bool firstFoot=(foot==0)==(side==0);
                const float begin=firstFoot?.08f:.53f,end=firstFoot?.47f:.92f;
                const int stance=phase<=begin?0:phase>=end?2:1;
                float maxDrift=0,maxHeight=0,meanX=0,meanZ=0,minY=INFINITY,maxY=-INFINITY;
                std::array<unsigned,3> touching{};std::array<float,3> currentSlip{};
                for(std::size_t row=0;row<indices.size();++row) {
                    const auto index=indices[row];
                    if(index>=vertices.size())return 1;
                    const auto value=rotate(vertices[index].position,yaw);
                    const auto ideal=rotate(idle[index].position,stance==2?finalYaw:0);
                    for(unsigned axis=0;axis<3;++axis)if(!std::isfinite(value[axis]))return 1;
                    const float dx=value[0]-ideal[0],dz=value[2]-ideal[2];
                    maxDrift=std::max(maxDrift,std::hypot(dx,dz));meanX+=dx;meanZ+=dz;
                    maxHeight=std::max(maxHeight,std::abs(value[1]-ideal[1]));
                    minY=std::min(minY,value[1]);maxY=std::max(maxY,value[1]);
                    // Track material points during measured floor-contact
                    // runs, including nominal swing. Bands are sensitivity
                    // measurements, not new admission tolerances.
                    for(unsigned band=0;band<heights.size();++band) {
                        auto& track=contacts[foot*3+band][row];auto& stats=bands[band];
                        if(value[1]<=heights[band]) {
                            if(!track.touching) {track={true,1,value[0],value[2],value[0],value[2],0};++stats.runs;}
                            else {++track.ticks;track.path+=std::hypot(value[0]-track.lastX,value[2]-track.lastZ);}
                            track.lastX=value[0];track.lastZ=value[2];
                            const float slip=std::hypot(value[0]-track.x,value[2]-track.z);
                            currentSlip[band]=std::max(currentSlip[band],slip);++touching[band];
                            stats.maxSlip=std::max(stats.maxSlip,slip);stats.maxPath=std::max(stats.maxPath,track.path);
                            stats.longest=std::max(stats.longest,track.ticks);++stats.touchSamples;
                        } else track.touching=false;
                    }
                }
                meanX/=indices.size();meanZ/=indices.size();
                if(stance!=1) {worstDrift=std::max(worstDrift,maxDrift);worstHeight=std::max(worstHeight,maxHeight);
                    worstMean=std::max(worstMean,std::hypot(meanX,meanZ));}
                csv<<(side?"right":"left")<<','<<degrees<<','<<sample<<','<<seconds<<','<<phase<<','
                    <<(foot?"Right":"Left")<<','<<(stance==0?"initial":stance==2?"final":"swing")<<',';
                csv<<maxDrift<<','<<meanX<<','<<meanZ<<','<<maxHeight<<','<<minY<<','<<maxY;
                for(unsigned band=0;band<heights.size();++band)csv<<','<<touching[band]<<','<<currentSlip[band];
                csv<<'\n';
            }
        }
        if(!firstRun)json<<',';firstRun=false;
        json<<"{\"direction\":\""<<(side?"right":"left")<<"\",\"degrees\":"<<degrees
            <<",\"samples\":91,\"soleRows\":[39,38],\"maxDeclaredSupportHorizontalDriftM\":"<<worstDrift
            <<",\"maxDeclaredSupportMeanDriftM\":"<<worstMean<<",\"maxDeclaredSupportHeightErrorM\":"<<worstHeight
            <<",\"measuredContactBands\":[";
        for(unsigned band=0;band<heights.size();++band) {
            if(band)json<<',';const auto& stats=bands[band];
            json<<"{\"maximumPointHeightM\":"<<heights[band]<<",\"maxContactRunSlipM\":"<<stats.maxSlip
                <<",\"maxContactRunPathM\":"<<stats.maxPath<<",\"longestRunSamples\":"<<stats.longest
                <<",\"touchPointSamples\":"<<stats.touchSamples<<",\"contactRuns\":"<<stats.runs<<'}';
        }
        json<<"]}";
    }
    const bool complete=endpointMax<.0001f && csv.good();
    json<<"],\"endpointMaxM\":"<<endpointMax<<",\"measurementComplete\":"<<(complete?"true":"false")
        <<",\"contactAdmitted\":false}\n";
    std::ofstream summary(output/"measurement.json");summary<<json.str();
    std::cout<<json.str();return complete && summary.good()?0:1;
}
