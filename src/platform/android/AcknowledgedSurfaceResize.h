#pragma once
#include <cstdint>
#include <limits>

namespace horde::platform::android {
// Render-owner only. A UI ACK is not a Vulkan extent proof. Retire all old GPU
// and presentation ownership before refreshing the supported Android VkSurface.
class AcknowledgedSurfaceResize {
public:
    struct Key {
        std::uint64_t request=0,origin=0,generation=0,sequence=0;
        bool restoring=false;
        std::uint32_t width=0,height=0;
        bool operator==(const Key&) const = default;
    };
    struct Geometry {
        std::uint32_t windowWidth=0,windowHeight=0,currentWidth=0,currentHeight=0;
        std::uint32_t minWidth=0,minHeight=0,maxWidth=0,maxHeight=0;
    };
    enum class Result { Waiting, Ready, Failed };
    static constexpr std::uint64_t TimeoutNs=2'000'000'000;
    void Reset() { *this={}; }
    template<class Retire,class Refresh>
    bool Prepare(const Key& key,Retire retire,Refresh refresh) {
        if(!hasKey_ || !(key_==key)) {Reset();key_=key;hasKey_=true;}
        if(prepared_ || failed_)return prepared_ && !failed_;
        if(!key.request || !key.origin || !key.generation || !key.sequence || !key.width || !key.height ||
           !retire() || !refresh()) {failed_=true;return false;}
        prepared_=true;return true;
    }
    Result Observe(const Geometry& actual,std::uint64_t now) {
        if(!prepared_ || failed_)return Result::Failed;
        if(!observing_) {started_=now;observing_=true;}
        // A regressing clock cannot extend the finite no-submission interval.
        if(now<started_ || now-started_>=TimeoutNs) {failed_=true;return Result::Failed;}
        const auto variable=std::numeric_limits<std::uint32_t>::max();
        const bool widthVariable=actual.currentWidth==variable,heightVariable=actual.currentHeight==variable;
        if(widthVariable!=heightVariable || actual.minWidth>actual.maxWidth || actual.minHeight>actual.maxHeight ||
           (widthVariable && (key_.width<actual.minWidth || key_.width>actual.maxWidth ||
            key_.height<actual.minHeight || key_.height>actual.maxHeight))) {
            failed_=true;return Result::Failed;
        }
        if(key_.width<actual.minWidth || key_.width>actual.maxWidth ||
           key_.height<actual.minHeight || key_.height>actual.maxHeight)return Result::Waiting;
        if(actual.windowWidth!=key_.width || actual.windowHeight!=key_.height)return Result::Waiting;
        if(!widthVariable && (actual.currentWidth!=key_.width || actual.currentHeight!=key_.height))return Result::Waiting;
        return Result::Ready;
    }
private:
    Key key_{};std::uint64_t started_=0;
    bool hasKey_=false,prepared_=false,failed_=false,observing_=false;
};

// Called only after successful retirement. Failure never leaves a stale handle
// or a partially-created surface available for a later GPU submission.
template<class Surface,class Destroy,class Create,class Supports>
bool RefreshRetiredPresentationSurface(Surface& surface,Destroy destroy,Create create,Supports supports) {
    if(surface!=Surface{}) {destroy(surface);surface=Surface{};}
    Surface replacement{};
    if(!create(replacement) || replacement==Surface{}) {
        if(replacement!=Surface{})destroy(replacement);
        return false;
    }
    if(!supports(replacement)) {destroy(replacement);return false;}
    surface=replacement;return true;
}
} // namespace horde::platform::android
