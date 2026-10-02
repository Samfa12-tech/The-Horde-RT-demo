#pragma once

#include <algorithm>

namespace horde::audio
{

inline constexpr int kSfxVolumePercentMinimum = 0;
inline constexpr int kSfxVolumePercentMaximum = 100;
inline constexpr int kSfxVolumeSettingMissing = 101;
inline constexpr float kPlayerFootstepCueGain = 0.45f;

[[nodiscard]] constexpr int ClampSfxVolumePercent(const int percent) noexcept
{
    return std::clamp(percent, kSfxVolumePercentMinimum, kSfxVolumePercentMaximum);
}

[[nodiscard]] constexpr int ResolveSfxVolumePercent(
    const int configuredPercent, const bool legacySfxEnabled) noexcept
{
    return configuredPercent == kSfxVolumeSettingMissing
        ? (legacySfxEnabled ? kSfxVolumePercentMaximum : kSfxVolumePercentMinimum)
        : ClampSfxVolumePercent(configuredPercent);
}

[[nodiscard]] constexpr float SfxVolumeLinearGain(const int percent) noexcept
{
    return static_cast<float>(ClampSfxVolumePercent(percent)) / 100.0f;
}

} // namespace horde::audio
