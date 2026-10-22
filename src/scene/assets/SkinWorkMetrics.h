#pragma once
#include <chrono>
#include <cstdint>

namespace horde::scene
{
// Optional CPU elapsed-work attribution. No GPU work, waits or utilisation.
// Callers publish only after the entire pose succeeds; disabled means no clocks.
struct SkinWorkMetrics
{
    std::uint64_t animationNanoseconds=0, vertexNanoseconds=0, copyNanoseconds=0;
    std::uint64_t vertices=0, joints=0, copyBytes=0, copies=0;
};
class SkinWorkScope
{
public:
    explicit SkinWorkScope(std::uint64_t* destination) : destination_(destination) {
        if(destination_)start_=std::chrono::steady_clock::now();
    }
    ~SkinWorkScope(){Complete();}
    void Complete() {
        if(!destination_)return;
        *destination_+=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now()-start_).count());
        destination_=nullptr;
    }
    SkinWorkScope(const SkinWorkScope&)=delete;
    SkinWorkScope& operator=(const SkinWorkScope&)=delete;
private:
    std::uint64_t* destination_;
    std::chrono::steady_clock::time_point start_{};
};
}
