#include "audio/MusicPcmWave.h"

namespace horde::audio
{

MusicPcmWaveStatus DecodeMusicPcmWave(
    const std::span<const std::uint8_t> fileBytes,
    const std::uint32_t expectedFrameCount,
    std::vector<std::int16_t>& outputSamples) noexcept
{
    return pocket_audio::DecodePcmWave(fileBytes, expectedFrameCount, outputSamples);
}

MusicPcmWaveStatus DecodeMusicPcmWave(
    const std::span<const std::uint8_t> fileBytes,
    const std::uint32_t expectedFrameCount,
    std::vector<std::int16_t>& outputSamples,
    const pocket_audio::PcmClipLimits limits) noexcept
{
    return pocket_audio::DecodePcmWave(fileBytes, expectedFrameCount, outputSamples, limits);
}

} // namespace horde::audio
