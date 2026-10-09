#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>
#include "pocket_audio/PcmLoopStream.h"
#include "pocket_audio/PcmWave.h"

namespace horde::audio
{
inline constexpr std::uint32_t kWaterfallCoreFrames = 552'000u;
inline constexpr std::size_t kAmbienceChunkFrames = 480u;
inline constexpr std::size_t kWaterfallCoreMaximumFileBytes = kWaterfallCoreFrames * 4u + 65'536u;
inline constexpr const char* kWaterfallCoreAsset = "audio/pixabay/waterfall_core_loop.wav";

// Asset admission and platform lifecycle only. Canonical Core owns decoding
// and every sample/loop cursor; no render-time allocation, resampling or mixer.
// All methods and borrowed PCM storage are confined to one audio worker.
class AmbiencePcmLoop
{
public:
    [[nodiscard]] pocket_audio::PcmWaveStatus Load(std::span<const std::uint8_t> bytes) noexcept;
    [[nodiscard]] bool IsValid() const noexcept { return core_ && core_->IsValid(); }
    [[nodiscard]] pocket_audio::PcmStatus SetSuspended(bool suspended) noexcept;
    [[nodiscard]] pocket_audio::PcmStatus Reset(bool suspended) noexcept;
    [[nodiscard]] pocket_audio::PcmStatus Render(std::span<float> output) noexcept;
    [[nodiscard]] std::uint64_t GeneratedFrames() const noexcept { return generatedFrames_; }
    [[nodiscard]] std::size_t DecodedBytes() const noexcept { return samples_.size() * sizeof(std::int16_t); }
    AmbiencePcmLoop() = default;
    AmbiencePcmLoop(const AmbiencePcmLoop&) = delete;
    AmbiencePcmLoop& operator=(const AmbiencePcmLoop&) = delete;
private:
    std::vector<std::int16_t> samples_;
    std::array<pocket_audio::PcmClip, 2u> clips_{};
    std::unique_ptr<pocket_audio::PcmLoopStream> core_;
    std::uint64_t generatedFrames_ = 0u;
};
}
