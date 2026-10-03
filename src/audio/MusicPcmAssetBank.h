#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "audio/MusicPcmStream.h"
#include "audio/MusicPcmWave.h"

namespace horde::audio
{

enum class MusicPcmBankStatus : std::uint8_t
{
    Loaded,
    AlreadyLoaded,
    InvalidReader,
    ReadFailed,
    WrongFileSize,
    DecodeFailed,
    AllocationFailure,
};

struct MusicPcmBankResult
{
    MusicPcmBankStatus status = MusicPcmBankStatus::InvalidReader;
    std::string_view assetPath; // Static admitted path; safe after failure.
    MusicPcmWaveStatus decoderStatus = MusicPcmWaveStatus::Ok;
};

// Platform file/asset access happens only during Load, before audio starts.
// The reader must bound allocation/read to maximumBytes and fully replace output.
// It may return false or throw; neither escapes Load. No filesystem/JNI backend
// is imposed on the shared bank, and no decoder/mixer is duplicated from Core.
using MusicPcmAssetReader = bool (*)(
    void* context, std::string_view assetPath, std::size_t maximumBytes,
    std::vector<std::uint8_t>& outputBytes);

// Owns the exact Horde cue bank's immutable decoded PCM. Load once, off the
// audio/render callback; only publish spans after success. Failed loads release
// all partial storage and may be retried before playback. A successful bank
// cannot reload/move/copy, preventing a live Core stream's borrowed-span eviction.
// Stop/join playback before destroying the bank. This is not a concurrent API.
class MusicPcmAssetBank
{
public:
    MusicPcmAssetBank() = default;
    MusicPcmAssetBank(const MusicPcmAssetBank&) = delete;
    MusicPcmAssetBank& operator=(const MusicPcmAssetBank&) = delete;
    MusicPcmAssetBank(MusicPcmAssetBank&&) = delete;
    MusicPcmAssetBank& operator=(MusicPcmAssetBank&&) = delete;

    [[nodiscard]] MusicPcmBankResult Load(MusicPcmAssetReader reader, void* context) noexcept;
    [[nodiscard]] bool IsReady() const noexcept { return ready_; }
    [[nodiscard]] std::size_t PcmBytes() const noexcept;
    [[nodiscard]] std::array<MusicPcmClip, kMusicPcmCueCount> Clips() const noexcept;

private:
    void ReleasePartial() noexcept;

    std::array<std::vector<std::int16_t>, kMusicPcmCueCount> bodies_;
    std::array<std::vector<std::int16_t>, kMusicPcmCueCount> tails_;
    bool ready_ = false;
};

} // namespace horde::audio
