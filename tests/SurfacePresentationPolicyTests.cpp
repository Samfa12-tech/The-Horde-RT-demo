#include "platform/android/SurfacePresentationPolicy.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Require(const bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}
}

int main()
{
    using horde::platform::android::ChooseSurfacePresentationPolicy;
    using horde::platform::android::kSurfaceTransformIdentity;

    constexpr std::uint32_t rotate90 = 0x00000002u;
    constexpr std::uint32_t rotate180 = 0x00000004u;
    constexpr std::uint32_t rotate270 = 0x00000008u;
    constexpr std::uint32_t mirror = 0x00000010u;
    constexpr std::uint32_t mirrorRotate90 = 0x00000020u;
    constexpr std::uint32_t mirrorRotate180 = 0x00000040u;
    constexpr std::uint32_t mirrorRotate270 = 0x00000080u;

    for (const std::uint32_t current : {rotate90, rotate180, rotate270, mirror,
                                        mirrorRotate90, mirrorRotate180, mirrorRotate270})
    {
        const auto policy = ChooseSurfacePresentationPolicy(kSurfaceTransformIdentity | current, current);
        Require(policy.chosenPreTransform == current,
                "swapchain must match the current surface transform even when identity is supported");
        Require(policy.identitySupported, "supported identity must be reported");
        Require(policy.rtPreRotationRequired,
                "non-identity surfaces require primary-ray pre-rotation");
        Require(policy.transformSupported, "all concrete supported transforms must be admitted");
    }

    const auto fallback = ChooseSurfacePresentationPolicy(rotate90 | rotate180, rotate90);
    Require(fallback.chosenPreTransform == rotate90,
            "unsupported identity must retain the surface current transform");
    Require(!fallback.identitySupported, "fallback must report identity as unsupported");
    Require(fallback.rtPreRotationRequired,
            "non-identity surface must request RT pre-rotation");
    Require(fallback.rtTransform == horde::graphics::RtPresentationTransform::Rotate90,
            "surface flags must map to the shared RT presentation transform");

    const auto identityCurrent = ChooseSurfacePresentationPolicy(rotate180, rotate180);
    Require(identityCurrent.chosenPreTransform == rotate180 && identityCurrent.rtPreRotationRequired,
            "fallback policy must preserve an actual non-identity current transform");

    const auto identitySupportedCurrent = ChooseSurfacePresentationPolicy(kSurfaceTransformIdentity,
                                                                          kSurfaceTransformIdentity);
    Require(identitySupportedCurrent.chosenPreTransform == kSurfaceTransformIdentity &&
                !identitySupportedCurrent.rtPreRotationRequired,
            "identity surfaces must remain identity");
    Require(!ChooseSurfacePresentationPolicy(0x100u, 0x100u).transformSupported &&
                !ChooseSurfacePresentationPolicy(kSurfaceTransformIdentity, rotate90).transformSupported,
            "inherit and unsupported concrete transforms must not claim a valid RT mapping");
    return 0;
}
