#pragma once
#include "vulkan/raytracing/RtRegionGeometryPreparation.h"
#include "world/streaming/RegionStreaming.h"
#include <algorithm>

namespace horde::vulkan::raytracing {
// Coordinate the logical request and offside GPU owner after a proved,
// unsubmitted failure. One retry per loading episode, on a later application
// turn; repeated failure is explicit. Unproved work can never enter retry.
class RtRegionPreparationRecovery {
public:
    bool Recover(horde::world::streaming::RegionStreaming& scheduler,
        RtRegionGeometryPreparation& preparation,horde::world::streaming::Token token,
        std::string& error) {
        using horde::world::streaming::Phase;
        const auto states=scheduler.States();
        if(std::none_of(states.begin(),states.end(),[&](const auto& state){
            return state.token==token && (state.phase==Phase::Preparing || state.phase==Phase::Ready);}))return false;
        const auto phase=preparation.Status();
        if(phase==RtRegionGeometryPreparation::Phase::Quarantined) {
            scheduler.MarkFailed(token,true);return false;
        }
        if(phase!=RtRegionGeometryPreparation::Phase::Idle && phase!=RtRegionGeometryPreparation::Phase::Failed)return false;
        if(!scheduler.MarkFailed(token,false))return false;
        if(retries_>=1) {error="Region preparation retry exhausted: "+error;return false;}
        if(phase==RtRegionGeometryPreparation::Phase::Failed && !preparation.ResetFailed(token))return false;
        if(!scheduler.RetryFailedRegion(token.region))return false;
        ++retries_;return true;
    }
    void ResetAfterAdoption() noexcept {retries_=0;}
    unsigned Retries() const noexcept {return retries_;}
private:
    unsigned retries_=0;
};
}
