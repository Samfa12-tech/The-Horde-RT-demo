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
        Require(policy.chosenPreTransform == kSurfaceTransformIdentity,
                "supported identity must be selected for every non-identity current transform");
        Require(policy.identitySupported, "supported identity must be reported");
        Require(!policy.rtPreRotationRequired,
                "identity pre-transform must not claim an RT pre-rotation is required");
    }

    const auto fallback = ChooseSurfacePresentationPolicy(rotate90 | rotate180, rotate90);
    Require(fallback.chosenPreTransform == rotate90,
            "unsupported identity must retain the surface current transform");
    Require(!fallback.identitySupported, "fallback must report identity as unsupported");
    Require(fallback.rtPreRotationRequired,
            "non-identity fallback must expose the unhandled RT pre-rotation requirement");

    const auto identityCurrent = ChooseSurfacePresentationPolicy(rotate180, rotate180);
    Require(identityCurrent.chosenPreTransform == rotate180 && identityCurrent.rtPreRotationRequired,
            "fallback policy must preserve an actual non-identity current transform");

    const auto identitySupportedCurrent = ChooseSurfacePresentationPolicy(kSurfaceTransformIdentity,
                                                                          kSurfaceTransformIdentity);
    Require(identitySupportedCurrent.chosenPreTransform == kSurfaceTransformIdentity &&
                !identitySupportedCurrent.rtPreRotationRequired,
            "identity surfaces must remain identity");
    return 0;
}
