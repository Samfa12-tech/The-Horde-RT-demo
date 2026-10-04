#include "audio/AmbiencePcmLoop.h"
#include <algorithm>
#include <limits>

namespace horde::audio
{
pocket_audio::PcmWaveStatus AmbiencePcmLoop::Load(const std::span<const std::uint8_t> bytes) noexcept
{
    core_.reset(); // Release every borrowed span before replacing storage.
    clips_ = {};
    generatedFrames_ = 0u;
    auto status = pocket_audio::DecodePcmWave(bytes, kWaterfallCoreFrames, samples_);
    if (status != pocket_audio::PcmWaveStatus::Ok) return status;
    try
    {
        clips_[1] = {.body = samples_, .tail = {}, .looping = true};
        core_ = std::make_unique<pocket_audio::PcmLoopStream>(clips_, 0u);
    }
    catch (...)
    {
        samples_.clear();
        clips_ = {};
        return pocket_audio::PcmWaveStatus::AllocationFailure;
    }
    if (!IsValid() || Reset(true) != pocket_audio::PcmStatus::Suspended)
    {
        core_.reset();
        samples_.clear();
        clips_ = {};
        return pocket_audio::PcmWaveStatus::InvalidDataAlignment;
    }
    return status;
}

pocket_audio::PcmStatus AmbiencePcmLoop::SetSuspended(const bool suspended) noexcept
{
    if (!IsValid()) return pocket_audio::PcmStatus::InvalidClips;
    return core_->SetSelection({.clipIndex = 1u, .looping = true,
                               .suspended = suspended, .revision = 1u});
}

pocket_audio::PcmStatus AmbiencePcmLoop::Reset(const bool suspended) noexcept
{
    if (!IsValid()) return pocket_audio::PcmStatus::InvalidClips;
    core_->Reset();
    generatedFrames_ = 0u;
    return SetSuspended(suspended);
}

pocket_audio::PcmStatus AmbiencePcmLoop::Render(const std::span<float> output) noexcept
{
    if (!IsValid() || output.empty() || (output.size() & 1u) != 0u ||
        output.size() > kAmbienceChunkFrames * 2u ||
        generatedFrames_ > std::numeric_limits<std::uint64_t>::max() - output.size() / 2u)
    {
        std::fill(output.begin(), output.end(), 0.0f);
        return IsValid() ? pocket_audio::PcmStatus::InvalidOutput : pocket_audio::PcmStatus::InvalidClips;
    }
    const auto status = core_->Render(output);
    if (status == pocket_audio::PcmStatus::Ok) generatedFrames_ += output.size() / 2u;
    return status;
}
}
