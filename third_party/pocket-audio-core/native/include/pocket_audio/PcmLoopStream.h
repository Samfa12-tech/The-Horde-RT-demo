#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "pocket_audio/PcmFormat.h"

namespace pocket_audio
{

inline constexpr std::size_t kPcmMaximumClipCount = 32u;
inline constexpr std::uint64_t kPcmMaximumClipFrames = 576'000u;
inline constexpr std::uint64_t kPcmMaximumCrossfadeFrames = 48'000u;
inline constexpr std::uint64_t kPcmDefaultCrossfadeFrames = 12'000u;

// Immutable, non-owning interleaved stereo PCM16. The caller keeps both spans
// alive and unchanged until the PcmLoopStream is destroyed or no longer uses
// the clip. Empty slot zero is reserved for silence.
struct PcmClip
{
    std::span<const std::int16_t> body;
    std::span<const std::int16_t> tail;
    bool looping = false;
};

struct PcmSelection
{
    std::uint8_t clipIndex = 0u;
    bool looping = false;
    bool suspended = false;
    bool discontinuity = false;
    bool clockValid = true;
    double positionSeconds = 0.0;
    std::uint64_t revision = 0u;
    // Set only by the adapter when a one-shot's authored tail should continue
    // under the next selection. Core still requires the old body to be done.
    bool naturalTailHandoff = false;
};

enum class PcmStatus : std::uint8_t
{
    Ok,
    Suspended,
    ClockInvalid,
    InvalidClips,
    InvalidSelection,
    InvalidOutput,
};

// Fixed-storage PCM cursor/mixer. One owner thread must call every method;
// no concurrent-call guarantee is made. Render performs no allocation, locks,
// or I/O. Clip spans are borrowed and must remain valid for the stream's use.
// Clip zero must be empty; remaining clips have caller-provided loop metadata.
// Body/tail lengths are derived from sample spans, with an optional tail no
// longer than its body. Crossfade length is bounded at construction time.
class PcmLoopStream
{
public:
    explicit PcmLoopStream(std::span<const PcmClip> clips,
                           std::uint64_t crossfadeFrames = kPcmDefaultCrossfadeFrames) noexcept;

    [[nodiscard]] bool IsValid() const noexcept { return clipsValid_; }
    [[nodiscard]] PcmStatus SetSelection(const PcmSelection& selection) noexcept;
    [[nodiscard]] bool SetVolumePercent(float percent) noexcept;
    [[nodiscard]] PcmStatus Render(std::span<float> interleavedStereoOutput) noexcept;
    void Reset() noexcept;

private:
    struct StreamState
    {
        std::uint8_t clipIndex = 0u;
        std::uint64_t cursorFrames = 0u;
        bool completedLoop = false;
        bool active = false;
        bool tailOnly = false;
    };

    [[nodiscard]] bool ValidateClips(std::span<const PcmClip> clips) noexcept;
    [[nodiscard]] bool ValidateSelection(const PcmSelection& selection) const noexcept;
    [[nodiscard]] StreamState MakeState(const PcmSelection& selection) const noexcept;
    [[nodiscard]] bool ReadFrame(StreamState& state, float& left, float& right) const noexcept;
    [[nodiscard]] std::uint64_t BodyFrames(std::uint8_t index) const noexcept;
    [[nodiscard]] std::uint64_t TailFrames(std::uint8_t index) const noexcept;
    void ClearPlayback() noexcept;

    std::array<PcmClip, kPcmMaximumClipCount> clips_{};
    std::size_t clipCount_ = 0u;
    StreamState current_{};
    StreamState outgoing_{};
    PcmSelection selection_{};
    std::uint64_t crossfadeFrames_ = kPcmDefaultCrossfadeFrames;
    std::uint64_t crossfadePosition_ = kPcmDefaultCrossfadeFrames;
    float volume_ = 1.0f;
    bool clipsValid_ = false;
    bool selectionInitialized_ = false;
    bool suspended_ = true;
    bool lastDiscontinuity_ = false;
};

} // namespace pocket_audio
