#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "audio/MusicDirector.h"

namespace horde::audio
{

inline constexpr std::uint32_t kMusicPcmSampleRate = 48'000u;
inline constexpr std::uint64_t kMusicPcmLoopFrames = 576'000u;
inline constexpr std::uint64_t kMusicPcmTailFrames = 144'000u;
inline constexpr std::uint64_t kMusicPcmCrossfadeFrames = 12'000u;
inline constexpr std::size_t kMusicPcmCueCount = 9u;

// Non-owning interleaved stereo PCM16. The caller retains both spans for the
// complete lifetime of MusicPcmStream. No decoding or file access occurs here.
struct MusicPcmClip
{
    std::span<const std::int16_t> body;
    std::span<const std::int16_t> tail;
};

enum class MusicPcmStatus : std::uint8_t
{
    Ok,
    Suspended,
    ClockInvalid,
    InvalidClips,
    InvalidSelection,
    InvalidOutput,
};

// Audio-thread-owned PCM cursor/mixer for the nine MusicCue rows. SetSelection,
// SetVolumePercent, Render, and Reset must be called by one owner thread; this
// class makes no concurrent-call guarantee. Render performs no allocation,
// locks, or I/O and reads only the caller-owned immutable clip spans.
// Normal cue changes retain at most one 250 ms outgoing stream; a newer cue
// revision replaces that outgoing stream rather than accumulating old audio.
// C->D/G->H preserve the remaining exact tail only after the one-shot body has
// actually ended. An earlier interruption follows the normal bounded fade.
class MusicPcmStream
{
public:
    explicit MusicPcmStream(std::span<const MusicPcmClip> clips) noexcept;

    [[nodiscard]] bool IsValid() const noexcept { return clipsValid_; }
    [[nodiscard]] MusicPcmStatus SetSelection(const MusicSelection& selection) noexcept;
    [[nodiscard]] bool SetVolumePercent(float percent) noexcept;
    [[nodiscard]] MusicPcmStatus Render(std::span<float> interleavedStereoOutput) noexcept;
    void Reset() noexcept;

private:
    struct StreamState
    {
        MusicCue cue = MusicCue::None;
        std::uint64_t cursorFrames = 0u;
        bool completedLoop = false;
        bool active = false;
        bool tailOnly = false;
    };

    [[nodiscard]] bool ValidateClips(std::span<const MusicPcmClip> clips) noexcept;
    [[nodiscard]] bool ValidateSelection(const MusicSelection& selection) const noexcept;
    [[nodiscard]] StreamState MakeState(const MusicSelection& selection) const noexcept;
    [[nodiscard]] bool ReadFrame(StreamState& state, float& left, float& right) const noexcept;
    void ClearPlayback() noexcept;

    std::array<MusicPcmClip, kMusicPcmCueCount> clips_{};
    StreamState current_{};
    StreamState outgoing_{};
    MusicSelection selection_{};
    std::uint64_t crossfadePosition_ = kMusicPcmCrossfadeFrames;
    float volume_ = 1.0f;
    bool clipsValid_ = false;
    bool selectionInitialized_ = false;
    bool suspended_ = true;
    bool lastDiscontinuity_ = false;
};

} // namespace horde::audio
