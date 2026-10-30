#pragma once
#include "gameplay/BenchmarkProfile.h"
#include <array>
#include <cstdint>

namespace horde::platform::android {
// A same-owner, nonpersistent output transaction. Neither UI ACKs nor requested
// pixels certify a completed RT frame. The caller drains before publishing any
// command and explicitly recreates the same surface after its matching ACK.
class AndroidBenchmarkOutputTransaction {
public:
    enum class Phase : std::uint8_t { Idle, DrainApply, AwaitApply, RecreateApply,
        AwaitAppliedFrame, Measuring, DrainRestore, AwaitRestore, RecreateRestore,
        AwaitRestoredFrame, Restored, Failed };
    bool Begin(std::uint64_t request,std::uint64_t origin,const graphics::GraphicsSettings& original) {
        if(!request || !origin || Active() || !graphics::ValidGraphicsSettings(original))return false;
        *this={};request_=request;origin_=generation_=origin;original_=original;phase_=Phase::DrainApply;return true;
    }
    bool Drained(bool success) {
        if(phase_!=Phase::DrainApply && phase_!=Phase::DrainRestore)return false;
        if(!success || !generation_) { Fail();return false; }
        ++sequence_;command_=phase_==Phase::DrainApply?1:2;
        phase_=command_==1?Phase::AwaitApply:Phase::AwaitRestore;return true;
    }
    bool Acknowledge(std::uint64_t request,std::uint64_t origin,std::uint64_t generation,
                     std::uint64_t sequence,bool success,std::uint32_t width,std::uint32_t height,bool layoutProof) {
        if(request!=request_ || origin!=origin_ || generation!=generation_ || sequence!=sequence_ ||
           (phase_!=Phase::AwaitApply && phase_!=Phase::AwaitRestore))return false;
        const auto spec=*gameplay::BenchmarkProfileSpecification(gameplay::BenchmarkProfileId::MobileProvisionalV1);
        if(!success || !layoutProof || !width || !height || width>16384 || height>16384 ||
           (phase_==Phase::AwaitApply && (width!=spec.output.width || height!=spec.output.height))) {
            if(phase_==Phase::AwaitApply) { cancelled_=true;command_=0;phase_=Phase::DrainRestore; }
            else Fail();
            return true; // Matching failed ACK is consumed; late success cannot erase it.
        }
        width_=width;height_=height;command_=0;
        phase_=phase_==Phase::AwaitApply?Phase::RecreateApply:Phase::RecreateRestore;return true;
    }
    bool Recreated(std::uint64_t epoch,std::uint64_t measurementGeneration,bool success) {
        if(phase_!=Phase::RecreateApply && phase_!=Phase::RecreateRestore)return false;
        if(!success || !epoch || !measurementGeneration) { Fail();return false; }
        epoch_=epoch;measurement_=measurementGeneration;
        phase_=phase_==Phase::RecreateApply?Phase::AwaitAppliedFrame:Phase::AwaitRestoredFrame;return true;
    }
    // A later scene change (Entry -> Showcase) owns a new evidence epoch too.
    void ExpectCurrentOwner(std::uint64_t epoch,std::uint64_t measurement) {
        if(phase_==Phase::AwaitAppliedFrame || phase_==Phase::AwaitRestoredFrame) {epoch_=epoch;measurement_=measurement;}
    }
    bool Observe(const gameplay::BenchmarkProfileConfiguration& actual,std::uint64_t epoch,
                 std::uint64_t measurement,std::uint64_t submission,std::uint64_t completion) {
        if((phase_!=Phase::AwaitAppliedFrame && phase_!=Phase::AwaitRestoredFrame && phase_!=Phase::Measuring) ||
           !epoch || epoch!=epoch_ || measurement!=measurement_ || !submission || !completion ||
           completion<=completion_ || !gameplay::ValidBenchmarkProfileConfiguration(actual))return false;
        if(phase_==Phase::AwaitRestoredFrame) {
            const bool swapped=graphics::RtPresentationTransformSwapsAxes(
                graphics::RtPresentationTransformFromIndex(actual.presentationTransform));
            const graphics::GraphicsExtent acknowledgedOutput=swapped?
                graphics::GraphicsExtent{height_,width_}:graphics::GraphicsExtent{width_,height_};
            if(actual.graphics!=original_ || actual.output!=acknowledgedOutput)return false;
            phase_=Phase::Restored;
        } else {
            if(!gameplay::MatchesBenchmarkProfile(gameplay::BenchmarkProfileId::MobileProvisionalV1,actual))return false;
            phase_=Phase::Measuring;
        }
        completion_=completion;return true;
    }
    void Restore(bool cancelled=false) {
        if(!Active() || phase_==Phase::DrainRestore || phase_==Phase::AwaitRestore ||
           phase_==Phase::RecreateRestore || phase_==Phase::AwaitRestoredFrame)return;
        cancelled_=cancelled_ || cancelled;command_=0;phase_=Phase::DrainRestore;
    }
    void SurfaceLost(std::uint64_t generation) {
        if(!Active() || generation!=generation_)return;
        Restore(true);generation_=0;command_=0;phase_=Phase::DrainRestore;
    }
    void BindSurface(std::uint64_t generation) {if(Active() && !generation_)generation_=generation;}
    void Fail(){phase_=Phase::Failed;command_=0;cancelled_=true;}
    bool Active()const{return phase_!=Phase::Idle && phase_!=Phase::Restored && phase_!=Phase::Failed;}
    bool Draining()const{return phase_==Phase::DrainApply || phase_==Phase::DrainRestore;}
    bool WaitingUI()const{return phase_==Phase::AwaitApply || phase_==Phase::AwaitRestore;}
    bool Applying()const{return phase_==Phase::RecreateApply;}
    bool Recreating()const{return Applying() || phase_==Phase::RecreateRestore;}
    bool Ready()const{return phase_==Phase::Measuring;}
    bool Restored()const{return phase_==Phase::Restored;}
    bool Cancelled()const{return cancelled_;}
    Phase State()const{return phase_;}
    const graphics::GraphicsSettings& Original()const{return original_;}
    graphics::GraphicsSettings Requested()const {
        auto s=gameplay::BenchmarkProfileSpecification(gameplay::BenchmarkProfileId::MobileProvisionalV1)->graphics;
        s.previewFrameCap=original_.previewFrameCap;return s;
    }
    std::uint32_t Width()const{return width_;}
    std::uint32_t Height()const{return height_;}
    std::array<std::int64_t,8> Snapshot()const{
        const auto spec=*gameplay::BenchmarkProfileSpecification(gameplay::BenchmarkProfileId::MobileProvisionalV1);
        return {static_cast<std::int64_t>(request_),static_cast<std::int64_t>(origin_),
            static_cast<std::int64_t>(generation_),static_cast<std::int64_t>(sequence_),command_,
            command_==1?static_cast<std::int64_t>(spec.output.width):0,
            command_==1?static_cast<std::int64_t>(spec.output.height):0,static_cast<std::int64_t>(phase_)};
    }
private:
    Phase phase_=Phase::Idle;graphics::GraphicsSettings original_{};
    std::uint64_t request_=0,origin_=0,generation_=0,sequence_=0,epoch_=0,measurement_=0,completion_=0;
    std::uint32_t width_=0,height_=0;std::int64_t command_=0;bool cancelled_=false;
};
} // namespace horde::platform::android
