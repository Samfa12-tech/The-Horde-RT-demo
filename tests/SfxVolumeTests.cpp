#include "audio/SfxVolume.h"

#include <cmath>
#include <iostream>

int main()
{
    using namespace horde::audio;
    const auto expect = [](const bool condition, const char* message)
    {
        if (!condition)
        {
            std::cerr << message << '\n';
            return false;
        }
        return true;
    };

    bool passed = true;
    passed &= expect(ClampSfxVolumePercent(-20) == 0, "negative SFX volume must clamp to mute");
    passed &= expect(ClampSfxVolumePercent(70) == 70, "SFX volume must preserve in-range values");
    passed &= expect(ClampSfxVolumePercent(140) == 100, "SFX volume must clamp to 100 percent");
    passed &= expect(ResolveSfxVolumePercent(kSfxVolumeSettingMissing, true) == 100,
                     "legacy enabled SFX setting must migrate to full volume");
    passed &= expect(ResolveSfxVolumePercent(kSfxVolumeSettingMissing, false) == 0,
                     "legacy disabled SFX setting must migrate to mute");
    passed &= expect(ResolveSfxVolumePercent(37, false) == 37,
                     "new SFX volume setting must take precedence over legacy state");
    passed &= expect(std::abs(SfxVolumeLinearGain(0) - 0.0f) < 0.0001f &&
                     std::abs(SfxVolumeLinearGain(70) - 0.7f) < 0.0001f &&
                     std::abs(SfxVolumeLinearGain(100) - 1.0f) < 0.0001f,
                     "mastering gain must follow the bounded percentage");
    passed &= expect(std::abs(kPlayerFootstepCueGain - 0.45f) < 0.0001f,
                     "player footsteps must retain their authored quieting gain");
    return passed ? 0 : 1;
}
