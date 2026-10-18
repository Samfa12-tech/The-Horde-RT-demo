#pragma once
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include "scene/ChapterRegionalWater.h"

namespace horde::platform::android
{
inline constexpr unsigned kEnvironmentSurveyFrames=120,kEnvironmentSurveyImages=12;
inline constexpr float EnvironmentSurveyTime(unsigned frame){return float(frame)/60.f;}
inline bool EnvironmentSurveyIdValid(std::string_view id) {
    if(id.empty() || id.size()>64)return false;
    for(const auto c:id)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return false;
    return true;
}
// CPU policy only. Callers must prove native RT completion before AcceptFrame;
// neither a phase nor a row count claims GPU readiness or presentation.
class AndroidEnvironmentSurveyPolicy
{
public:
    bool Configure(std::string_view id,bool disabled) {
        if(!EnvironmentSurveyIdValid(id) || phase_==1 || id==id_ || serial_==UINT64_MAX)return false;
        id_=id;disabled_=disabled;++serial_;phase_=1;frames_=0;surface_=scene_=measurement_=submission_=0;return true;
    }
    bool Bind(std::uint64_t serial,std::uint64_t surface) {
        if(phase_!=1 || serial!=serial_ || !surface || surface_)return false;
        surface_=surface;return true;
    }
    bool AcceptFrame(std::uint64_t surface,std::uint64_t scene,std::uint64_t measurement,std::uint64_t submission) {
        if(phase_!=1 || !surface_ || surface!=surface_ || !scene || !measurement || !submission ||
           frames_>=kEnvironmentSurveyFrames || submission<=submission_ ||
           (frames_ && (scene!=scene_ || measurement!=measurement_)))return false;
        scene_=scene;measurement_=measurement;submission_=submission;++frames_;return true;
    }
    bool Complete() {if(phase_!=1 || frames_!=kEnvironmentSurveyFrames)return false;phase_=2;return true;}
    void Cancel() {if(phase_==1)phase_=3;}
    unsigned Phase()const{return phase_;} unsigned Frames()const{return frames_;}
    std::uint64_t Serial()const{return serial_;} std::uint64_t Surface()const{return surface_;}
    const std::string& Id()const{return id_;} bool WaterDisabled()const{return disabled_;}
private:
    std::string id_;bool disabled_=false;unsigned phase_=0,frames_=0;
    std::uint64_t serial_=0,surface_=0,scene_=0,measurement_=0,submission_=0;
};
struct EnvironmentSurveyCamera {float x=0,z=0,supportY=0,yaw=0,pitch=0;};
struct EnvironmentSurveyWaterFacts {
    float footDistance=0,eyeDistance=0,waterY=0;
    bool waterAboveOrBelowCamera=false;
};
inline EnvironmentSurveyWaterFacts SurveyWaterFacts(const EnvironmentSurveyCamera& camera,float eyeY) {
    EnvironmentSurveyWaterFacts facts;
    facts.footDistance=std::sqrt(horde::scene::ChapterRegionalWaterDistanceSquared(camera.x,camera.supportY,camera.z));
    facts.eyeDistance=std::sqrt(horde::scene::ChapterRegionalWaterDistanceSquared(camera.x,eyeY,camera.z));
    for(const auto& t:horde::scene::ChapterRegionalWaterTriangles()) {
        const double ax=t[1][0]-t[0][0],az=t[1][2]-t[0][2],bx=t[2][0]-t[0][0],bz=t[2][2]-t[0][2];
        const double x=camera.x-t[0][0],z=camera.z-t[0][2],det=ax*bz-az*bx;
        if(std::abs(det)<1e-10)continue;
        const double u=(x*bz-z*bx)/det,v=(ax*z-az*x)/det;
        if(u>=0 && v>=0 && u+v<=1) {
            facts.waterAboveOrBelowCamera=true;
            facts.waterY=float(t[0][1]+u*(t[1][1]-t[0][1])+v*(t[2][1]-t[0][1]));
            break;
        }
    }
    return facts;
}
inline EnvironmentSurveyCamera RiverBendSurveyCamera() {
    const auto shore=horde::scene::ChapterWaterFromAtlas(56,-5,0);
    const auto target=horde::scene::ChapterWaterFromAtlas(58,-25,6);
    return {shore[0],shore[2],0,std::atan2(target[0]-shore[0],shore[2]-target[2]),0};
}
}
