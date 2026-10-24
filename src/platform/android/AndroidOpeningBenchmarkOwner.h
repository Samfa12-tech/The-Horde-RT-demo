#pragma once
#include "graphics/OpeningBenchmarkOwner.h"
#include <array>
#include <cstdint>

namespace horde::platform::android {
// Owner-thread policy; the bridge serializes immutable JNI commands separately.
// Neither these state changes nor UI polling manufacture GPU retirement/presentation.
class AndroidOpeningBenchmarkOwner {
public:
    enum class Phase : std::uint8_t { Idle, Queued, Starting, Running, Returning, Complete, Blocked };
    bool Queue(std::uint64_t serial,std::uint64_t generation,const graphics::OpeningBenchmarkEligibility& eligibility) {
        if(!serial || !generation || Busy() || phase_==Phase::Blocked)return false;
        graphics::OpeningBenchmarkOwnerTransfer next;
        if(!next.Queue(graphics::OpeningBenchmarkChoice::Custom,eligibility))return false;
        transfer_=next;serial_=serial;origin_=generation;generation_=generation;
        phase_=Phase::Queued;result_=0;cancelled_=started_=false;return true;
    }
    bool Prepare(const graphics::OpeningBenchmarkEligibility& eligibility) {
        if(phase_!=Phase::Queued)return false;
        if(cancelled_ || !eligibility.Eligible()) { BeginReturn(3);return false; }
        eligibility_=eligibility;phase_=Phase::Starting;return true;
    }
    bool EntryRetired(bool retired) {
        if(phase_!=Phase::Starting || !transfer_.RecheckAndRetireEntry(eligibility_,retired)) { Block();return false; }
        return true;
    }
    bool ShowcasePresented(std::uint64_t generation) {
        if(phase_!=Phase::Starting || generation!=generation_ || cancelled_ ||
           transfer_.Phase()!=graphics::OpeningBenchmarkOwnerPhase::Benchmark)return false;
        phase_=Phase::Running;return true;
    }
    bool TakeStart() {
        if(phase_!=Phase::Running || started_ || cancelled_)return false;
        started_=true;return true;
    }
    bool Cancel(std::uint64_t serial,std::uint64_t origin) {
        if(!Busy() || serial!=serial_ || origin!=origin_)return false;
        cancelled_=true;result_=3;return true;
    }
    void BeginReturn(unsigned result) {
        if(!Busy())return;
        if(transfer_.Phase()==graphics::OpeningBenchmarkOwnerPhase::Queued)transfer_.CancelQueued();
        phase_=Phase::Returning;result_=cancelled_?3:result;
    }
    bool ReturnRetired(bool settingsUnchanged,bool retired) {
        if(phase_!=Phase::Returning || !settingsUnchanged || !retired) { Block();return false; }
        if(transfer_.Phase()==graphics::OpeningBenchmarkOwnerPhase::Benchmark &&
           !transfer_.BenchmarkRetired(true,true)) { Block();return false; }
        return true;
    }
    bool EntryPresented(std::uint64_t generation,bool settingsUnchanged) {
        if(phase_!=Phase::Returning || generation!=generation_ || !settingsUnchanged)return false;
        if(transfer_.Phase()==graphics::OpeningBenchmarkOwnerPhase::ReturningEntry && !transfer_.EntryPresented(true)) {
            Block();return false;
        }
        if(transfer_.Phase()!=graphics::OpeningBenchmarkOwnerPhase::Entry) { Block();return false; }
        phase_=Phase::Complete;if(!result_)result_=3;return true;
    }
    void SurfaceRetired(std::uint64_t generation,bool retired) {
        if(!Busy() || generation!=generation_)return;
        if(!retired) { Block();return; }
        cancelled_=true;BeginReturn(3);ReturnRetired(true,true);generation_=0;
    }
    void BindSurface(std::uint64_t generation) {
        if((phase_==Phase::Returning && generation_==0) || phase_==Phase::Blocked)generation_=generation;
    }
    void Block() {phase_=Phase::Blocked;result_=3;}
    bool Busy() const {return phase_>=Phase::Queued && phase_<=Phase::Returning;}
    bool WantsShowcase() const {return !cancelled_ && (phase_==Phase::Starting || phase_==Phase::Running);}
    bool NeedsEntryRetirement() const {return phase_==Phase::Starting &&
        transfer_.Phase()==graphics::OpeningBenchmarkOwnerPhase::Queued;}
    bool Returning() const {return phase_==Phase::Returning;}
    bool Cancelled() const {return cancelled_;}
    Phase State() const {return phase_;}
    std::uint64_t Generation() const {return generation_;}
    std::array<std::int64_t,6> Snapshot() const {
        return {static_cast<std::int64_t>(serial_),static_cast<std::int64_t>(generation_),
            static_cast<std::int64_t>(phase_),result_,phase_==Phase::Complete?1:0,static_cast<std::int64_t>(origin_)};
    }
private:
    graphics::OpeningBenchmarkOwnerTransfer transfer_;
    graphics::OpeningBenchmarkEligibility eligibility_{};
    Phase phase_=Phase::Idle;unsigned result_=0;
    std::uint64_t serial_=0,origin_=0,generation_=0;
    bool cancelled_=false,started_=false;
};
} // namespace horde::platform::android
