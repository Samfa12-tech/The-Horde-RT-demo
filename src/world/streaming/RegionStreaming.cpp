#include "world/streaming/RegionStreaming.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace horde::world::streaming {
namespace {
constexpr auto missing = std::numeric_limits<std::size_t>::max();
std::uint64_t Add(std::uint64_t a, std::uint64_t b) {
    return b>UINT64_MAX-a ? UINT64_MAX : a+b;
}
bool Finite(const std::array<float,3>& p) {
    return std::all_of(p.begin(),p.end(),[](float v){return std::isfinite(v);});
}
}
bool Bounds::Valid() const noexcept {
    if(!Finite(min) || !Finite(max))return false;
    for(unsigned a=0;a<3;++a)if(min[a]>max[a])return false;
    return true;
}
double Bounds::DistanceSquared(const std::array<float,3>& p) const noexcept {
    if(!Valid() || !Finite(p))return std::numeric_limits<double>::infinity();
    double result=0;
    for(unsigned a=0;a<3;++a) {
        const double d=std::max({double(min[a])-p[a],0.0,double(p[a])-max[a]});
        result+=d*d;
    }
    return result;
}
std::size_t RegionStreaming::Find(RegionId id) const noexcept {
    const auto found=index_.find(id);return found==index_.end()?missing:found->second;
}
ResidentState* RegionStreaming::Match(Token t) noexcept {
    const auto i=Find(t.region);
    return i!=missing && states_[i].token==t ? &states_[i] : nullptr;
}
bool RegionStreaming::Configure(std::span<const RegionDefinition> definitions,
    StreamingRules rules,std::uint64_t generation,std::string& error) {
    if(!generation || definitions.empty() || !std::isfinite(rules.prepareDistance) ||
       !std::isfinite(rules.retainDistance) || !std::isfinite(rules.predictionSeconds) ||
       !std::isfinite(rules.retireDelaySeconds) || rules.prepareDistance<0 ||
       rules.retainDistance<rules.prepareDistance || rules.predictionSeconds<0 ||
       rules.retireDelaySeconds<0 || !rules.maxConcurrentPreparations || !rules.maxResidentDetailOwners ||
       std::any_of(states_.begin(),states_.end(),[](const auto& s){return s.phase!=Phase::Absent && s.phase!=Phase::Failed;})) {
        error="Region catalog/rules are invalid or still own live/pending resources.";return false;
    }
    std::unordered_map<RegionId,std::size_t> index;
    for(std::size_t i=0;i<definitions.size();++i) {
        const auto& d=definitions[i];
        if(!d.id || !d.bounds.Valid() || !index.emplace(d.id,i).second) {
            error="Region identities/bounds must be finite and unique.";return false;
        }
    }
    std::vector<std::vector<std::size_t>> dependencies(definitions.size()),dependants(definitions.size());
    std::vector<std::size_t> pending(definitions.size()),order;
    for(std::size_t i=0;i<definitions.size();++i) {
        for(auto dep:definitions[i].dependencies) {
            if(dep==definitions[i].id || !index.contains(dep)) {
                error="Every region dependency needs a distinct admitted catalog owner.";return false;
            }
            dependencies[i].push_back(index.at(dep));
        }
        auto& list=dependencies[i];std::sort(list.begin(),list.end());
        list.erase(std::unique(list.begin(),list.end()),list.end());pending[i]=list.size();
        for(auto dep:list)dependants[dep].push_back(i);
        if(list.empty())order.push_back(i);
    }
    for(std::size_t cursor=0;cursor<order.size();++cursor)
        for(auto child:dependants[order[cursor]])if(--pending[child]==0)order.push_back(child);
    if(order.size()!=definitions.size()) {
        error="Region dependency cycle rejected.";return false;
    }
    definitions_.assign(definitions.begin(),definitions.end());
    states_.assign(definitions.size(),{});required_.resize(definitions.size());
    wanted_.resize(definitions.size());absentAllowed_.resize(definitions.size());
    nearPrepare_.resize(definitions.size());
    seen_.resize(definitions.size());dependencyInUse_.resize(definitions.size());
    index_=std::move(index);dependencies_=std::move(dependencies);dependencyOrder_=std::move(order);
    rules_=rules;generation_=generation;lastSeconds_=-1;error.clear();return true;
}
bool RegionStreaming::Evaluate(const Frame& frame,Plan& plan,std::string& error) {
    plan={};
    if(!generation_ || frame.worldGeneration!=generation_ || !Finite(frame.position) || !Finite(frame.velocity) ||
       !std::isfinite(frame.monotonicSeconds) || frame.monotonicSeconds<0 || frame.monotonicSeconds<lastSeconds_) {
        error="Region observation has a stale generation or invalid time/position.";return false;
    }
    std::fill(required_.begin(),required_.end(),0);std::fill(wanted_.begin(),wanted_.end(),0);
    std::fill(absentAllowed_.begin(),absentAllowed_.end(),0);
    std::fill(seen_.begin(),seen_.end(),0);std::fill(dependencyInUse_.begin(),dependencyInUse_.end(),0);
    for(const auto& c:frame.contributions) {
        const auto i=Find(c.region);
        if(i==missing || seen_[i]){error="Contributor certificates must reference unique known regions.";return false;}
        seen_[i]=1;
        absentAllowed_[i]=c.mayBeAbsent;
    }
    for(auto id:frame.requiredForTraversal) {
        const auto i=Find(id);
        if(i==missing){error="Traversal demand references an unknown region.";return false;}
        required_[i]=wanted_[i]=1;
    }
    auto predicted=frame.position;
    for(unsigned a=0;a<3;++a)predicted[a]+=frame.velocity[a]*rules_.predictionSeconds;
    if(!Finite(predicted)){error="Region prediction overflow rejected.";return false;}
    std::uint32_t preparing=0,detailOwners=0;
    std::uint64_t requiredCost=0;
    for(std::size_t i=0;i<states_.size();++i) {
        auto& s=states_[i];const auto& d=definitions_[i];
        if(d.alwaysResident || !absentAllowed_[i])required_[i]=wanted_[i]=1;
        const double distance=d.bounds.DistanceSquared(frame.position);
        if(distance==0)required_[i]=wanted_[i]=1;
        const double radius=s.phase==Phase::Resident ? rules_.retainDistance : rules_.prepareDistance;
        nearPrepare_[i]=distance<=double(rules_.prepareDistance)*rules_.prepareDistance ||
                       d.bounds.DistanceSquared(predicted)<=double(rules_.prepareDistance)*rules_.prepareDistance;
        wanted_[i]|=distance<=radius*radius || nearPrepare_[i];
        if(s.phase==Phase::Resident || s.phase==Phase::Retiring || s.phase==Phase::Quarantined) {
            plan.residentDeviceBytes=Add(plan.residentDeviceBytes,s.deviceBytes);
            if(s.phase==Phase::Quarantined && !s.deviceBytes)plan.costUnknown=true;
        }
        if(s.phase==Phase::Preparing || s.phase==Phase::Ready) {
            ++preparing;plan.preparationReservedDeviceBytes=Add(plan.preparationReservedDeviceBytes,
                s.phase==Phase::Ready?s.deviceBytes:std::max(s.deviceBytes,d.preparationPeakDeviceBytes));
            if(!s.allocationKnown)plan.costUnknown=true;
        }
        if(!d.alwaysResident && s.phase!=Phase::Absent && s.phase!=Phase::Failed)++detailOwners;
    }
    // Demand propagates toward shared owners before their dependants can adopt.
    for(auto item=dependencyOrder_.rbegin();item!=dependencyOrder_.rend();++item)if(wanted_[*item])
        for(auto dep:dependencies_[*item]){wanted_[dep]=1;required_[dep]|=required_[*item];}
    for(std::size_t i=0;i<states_.size();++i)
        if(states_[i].phase!=Phase::Absent && states_[i].phase!=Phase::Failed)
            for(auto dep:dependencies_[i])dependencyInUse_[dep]=1;
    std::uint64_t nextRequiredPeak=0;
    bool requiredNeedsDetailSlot=false;
    for(std::size_t i=0;i<states_.size();++i)if(required_[i] && states_[i].phase==Phase::Absent) {
        nextRequiredPeak=std::max(nextRequiredPeak,definitions_[i].preparationPeakDeviceBytes);
        requiredNeedsDetailSlot|=!definitions_[i].alwaysResident;
    }
    const bool pressure=(requiredNeedsDetailSlot && detailOwners>=rules_.maxResidentDetailOwners) ||
        (rules_.workingSetDeviceBytes && Add(Add(plan.residentDeviceBytes,plan.preparationReservedDeviceBytes),nextRequiredPeak)>rules_.workingSetDeviceBytes);
    if(pressure)
        for(std::size_t i=0;i<states_.size();++i)
            if(!required_[i] && !nearPrepare_[i] && absentAllowed_[i])wanted_[i]=0;
    // Shedding an optional ancestor cannot break a still-wanted child's source
    // ownership. This second linear closure restores every dependency pin.
    for(auto item=dependencyOrder_.rbegin();item!=dependencyOrder_.rend();++item)if(wanted_[*item])
        for(auto dep:dependencies_[*item])wanted_[dep]=1;
    for(std::size_t i=0;i<states_.size();++i) {
        auto& s=states_[i];
        if(required_[i]) {
            requiredCost=Add(requiredCost,(s.phase==Phase::Preparing || !s.allocationKnown)
                ? std::max(s.deviceBytes,definitions_[i].preparationPeakDeviceBytes) : s.deviceBytes);
            if(s.phase!=Phase::Resident)++plan.requiredUnavailable;
        }
        if(!definitions_[i].preparationPeakDeviceBytes && s.phase==Phase::Absent && wanted_[i])plan.costUnknown=true;
        if(wanted_[i])s.undesiredSince=-1;
        else if(s.undesiredSince<0)s.undesiredSince=frame.monotonicSeconds;
    }
    plan.requiredExceedsTarget=rules_.workingSetDeviceBytes && requiredCost>rules_.workingSetDeviceBytes;
    lastSeconds_=frame.monotonicSeconds;
    const auto emit=[&](ActionKind kind,std::size_t i){plan.actions[0]={kind,states_[i].token};plan.count=1;};
    for(std::size_t i=0;i<states_.size();++i)
        if(states_[i].phase==Phase::Retiring && (required_[i] || !absentAllowed_[i] ||
           (wanted_[i] && (!states_[i].retirementForRequired ||
              !requiredNeedsDetailSlot || detailOwners<rules_.maxResidentDetailOwners)))) {
            emit(ActionKind::CancelRetirement,i);error.clear();return true;
        }
    // Cancel obsolete CPU/upload work first. The backend must acknowledge real
    // cancellation/completion before its reservation or dependency pins disappear.
    for(std::size_t i=0;i<states_.size();++i) {
        auto& s=states_[i];
        if((s.phase==Phase::Preparing || s.phase==Phase::Ready) && (s.cancelRequested || !wanted_[i])) {
            s.cancelRequested=true;emit(ActionKind::CancelPreparation,i);error.clear();return true;
        }
    }
    for(std::size_t i=0;i<states_.size();++i)if(states_[i].phase==Phase::Ready) {
        bool dependenciesReady=true;
        for(auto dep:dependencies_[i])dependenciesReady&=states_[dep].phase==Phase::Resident;
        if(dependenciesReady){emit(ActionKind::Adopt,i);error.clear();return true;}
    }
    const auto retire=[&](std::size_t i,bool forRequired=false) {
        auto& s=states_[i];
        if(s.phase!=Phase::Resident || required_[i] || (!forRequired && wanted_[i]) ||
           !absentAllowed_[i] || definitions_[i].alwaysResident || dependencyInUse_[i] ||
           (!forRequired && (s.undesiredSince<0 || frame.monotonicSeconds-s.undesiredSince<rules_.retireDelaySeconds)))return false;
        s.phase=Phase::Retiring;s.retirementForRequired=forRequired;
        emit(ActionKind::Retire,i);return true;
    };
    // Free a proven unnecessary owner before allocating an incoming region when
    // the measured working set or slot pool needs room. Never evict a required
    // contributor just to hit the target.
    if(pressure)
        for(std::size_t i=0;i<states_.size();++i)if(retire(i)){error.clear();return true;}
    if(requiredNeedsDetailSlot && detailOwners>=rules_.maxResidentDetailOwners) {
        // Required return travel outranks optional retention/prediction. Among
        // certified-safe optional owners prefer the farthest; do not impose an
        // extra hysteresis delay at a gate already needing its destination.
        const auto retiring=static_cast<std::uint32_t>(std::count_if(states_.begin(),states_.end(),
            [](const auto& s){return s.phase==Phase::Retiring;}));
        std::size_t victim=missing;double distance=-1;
        for(std::size_t i=0;i<states_.size();++i)
            if(detailOwners-retiring>=rules_.maxResidentDetailOwners &&
               states_[i].phase==Phase::Resident && !required_[i] && absentAllowed_[i] &&
               !definitions_[i].alwaysResident && !dependencyInUse_[i]) {
                const auto d=definitions_[i].bounds.DistanceSquared(frame.position);
                if(d>distance){distance=d;victim=i;}
            }
        if(victim!=missing && retire(victim,true)){error.clear();return true;}
        plan.requiredBlockedByOwnerCapacity=true;
    }
    // Required restoration has priority over optional refinement. A hard target
    // cannot justify hiding required geometry or bypassing a readiness guard.
    for(unsigned priority=0;priority<2;++priority)
        for(auto i:dependencyOrder_) {
            auto& s=states_[i];const auto& d=definitions_[i];
            if(!wanted_[i] || bool(required_[i])!=(priority==0) || s.phase!=Phase::Absent ||
               preparing>=rules_.maxConcurrentPreparations || (!d.alwaysResident && detailOwners>=rules_.maxResidentDetailOwners))continue;
            if(std::any_of(dependencies_[i].begin(),dependencies_[i].end(),
                [&](auto dep){return states_[dep].phase!=Phase::Resident;}))continue;
            const auto total=Add(Add(plan.residentDeviceBytes,plan.preparationReservedDeviceBytes),d.preparationPeakDeviceBytes);
            if(!required_[i] && (!d.preparationPeakDeviceBytes ||
               (rules_.workingSetDeviceBytes && total>rules_.workingSetDeviceBytes)))continue;
            if(requestSequence_==UINT64_MAX){error="Region request identity exhausted.";return false;}
            s.token={generation_,++requestSequence_,d.id};s.phase=Phase::Preparing;
            if(!d.preparationPeakDeviceBytes)plan.costUnknown=true;
            plan.preparationReservedDeviceBytes=Add(plan.preparationReservedDeviceBytes,d.preparationPeakDeviceBytes);
            emit(ActionKind::Prepare,i);error.clear();return true;
        }
    for(std::size_t i=0;i<states_.size();++i)if(states_[i].phase==Phase::Preparing) {
        emit(ActionKind::AdvancePreparation,i);error.clear();return true;
    }
    for(std::size_t i=0;i<states_.size();++i) {
        if(retire(i)){error.clear();return true;}
    }
    error.clear();return true;
}
bool RegionStreaming::MarkPrepared(Token t,std::uint64_t bytes) noexcept {
    auto* s=Match(t);if(!s || t.worldGeneration!=generation_ || s->phase!=Phase::Preparing || s->cancelRequested)return false;
    s->deviceBytes=bytes;s->allocationKnown=true;s->phase=Phase::Ready;return true;
}
bool RegionStreaming::MarkResident(Token t) noexcept {
    auto* s=Match(t);if(!s || t.worldGeneration!=generation_ || s->phase!=Phase::Ready || s->cancelRequested)return false;
    for(auto dep:dependencies_[Find(t.region)])if(states_[dep].phase!=Phase::Resident)return false;
    s->retirementForRequired=false;s->phase=Phase::Resident;return true;
}
bool RegionStreaming::MarkRetired(Token t) noexcept {
    auto* s=Match(t);if(!s || s->phase!=Phase::Retiring)return false;
    *s={};return true;
}
bool RegionStreaming::MarkRetirementCancelled(Token t) noexcept {
    auto* s=Match(t);if(!s || s->phase!=Phase::Retiring)return false;
    s->retirementForRequired=false;s->phase=Phase::Resident;return true;
}
bool RegionStreaming::MarkPreparationCancelled(Token t) noexcept {
    auto* s=Match(t);if(!s || !s->cancelRequested || (s->phase!=Phase::Preparing && s->phase!=Phase::Ready))return false;
    *s={};return true;
}
bool RegionStreaming::MarkFailed(Token t,bool unproved) noexcept {
    auto* s=Match(t);if(!s || (s->phase!=Phase::Preparing && s->phase!=Phase::Ready))return false;
    s->phase=unproved?Phase::Quarantined:Phase::Failed;
    if(!unproved)s->deviceBytes=0;
    return true;
}
bool RegionStreaming::MarkQuarantineReleased(Token t) noexcept {
    auto* s=Match(t);if(!s || s->phase!=Phase::Quarantined)return false;
    *s={};return true;
}
bool RegionStreaming::RetryFailedRegion(RegionId id) noexcept {
    const auto i=Find(id);if(i==missing || states_[i].phase!=Phase::Failed)return false;
    states_[i]={};return true;
}
bool RegionStreaming::ObservePreparationAllocation(Token t,std::uint64_t bytes) noexcept {
    auto* s=Match(t);if(!s || (s->phase!=Phase::Preparing && s->phase!=Phase::Ready && s->phase!=Phase::Quarantined))return false;
    s->deviceBytes=bytes;s->allocationKnown=true;return true;
}
bool RegionStreaming::SeedResident(RegionId id,std::uint64_t bytes) noexcept {
    const auto i=Find(id);if(i==missing || states_[i].phase!=Phase::Absent || requestSequence_==UINT64_MAX)return false;
    for(auto dep:dependencies_[i])if(states_[dep].phase!=Phase::Resident)return false;
    states_[i].token={generation_,++requestSequence_,id};states_[i].deviceBytes=bytes;
    states_[i].allocationKnown=true;states_[i].phase=Phase::Resident;return true;
}
bool RegionStreaming::Invalidate(std::uint64_t next) noexcept {
    if(!next || next<=generation_)return false;
    // Do not destroy resident or in-flight owners on a logical reset. Their
    // tokens remain identifiable until the backend releases the actual owners.
    for(auto& s:states_)if(s.phase==Phase::Preparing || s.phase==Phase::Ready)s.cancelRequested=true;
    generation_=next;lastSeconds_=-1;return true;
}
}
