#include "audio/MusicPcmAssetBank.h"

#include <new>

namespace horde::audio
{

MusicPcmBankResult MusicPcmAssetBank::Load(
    const MusicPcmAssetReader reader, void* const context) noexcept
{
    if (ready_)
    {
        return {MusicPcmBankStatus::AlreadyLoaded};
    }
    if (reader == nullptr)
    {
        return {MusicPcmBankStatus::InvalidReader};
    }

    std::string_view activePath;
    try
    {
        // At most one bounded encoded WAV is resident alongside the immutable
        // PCM. Reuse its temporary storage, then release it before playback.
        std::vector<std::uint8_t> fileBytes;
        for (std::size_t index = 1u; index < kMusicPcmAssets.size(); ++index)
        {
            const auto& asset = kMusicPcmAssets[index];
            for (const bool isTail : {false, true})
            {
                activePath = isTail ? asset.tailPath : asset.bodyPath;
                const std::uint32_t frames = isTail
                    ? static_cast<std::uint32_t>(kMusicPcmTailFrames) : asset.bodyFrames;
                const std::size_t expectedBytes = static_cast<std::size_t>(frames) * 4u + 44u;
                fileBytes.clear();
                if (!reader(context, activePath, expectedBytes, fileBytes))
                {
                    ReleasePartial();
                    return {MusicPcmBankStatus::ReadFailed, activePath};
                }
                if (fileBytes.size() != expectedBytes)
                {
                    ReleasePartial();
                    return {MusicPcmBankStatus::WrongFileSize, activePath};
                }
                auto& samples = isTail ? tails_[index] : bodies_[index];
                const auto decoded = DecodeMusicPcmWave(fileBytes, frames, samples);
                if (decoded != MusicPcmWaveStatus::Ok)
                {
                    ReleasePartial();
                    return {decoded == MusicPcmWaveStatus::AllocationFailure
                                ? MusicPcmBankStatus::AllocationFailure
                                : MusicPcmBankStatus::DecodeFailed,
                            activePath, decoded};
                }
            }
        }
        ready_ = true;
        return {MusicPcmBankStatus::Loaded};
    }
    catch (const std::bad_alloc&)
    {
        ReleasePartial();
        return {MusicPcmBankStatus::AllocationFailure, activePath};
    }
    catch (...)
    {
        ReleasePartial();
        return {MusicPcmBankStatus::ReadFailed, activePath};
    }
}

std::size_t MusicPcmAssetBank::PcmBytes() const noexcept
{
    std::size_t bytes = 0u;
    for (std::size_t index = 1u; index < bodies_.size(); ++index)
    {
        bytes += (bodies_[index].size() + tails_[index].size()) * sizeof(std::int16_t);
    }
    return bytes;
}

std::array<MusicPcmClip, kMusicPcmCueCount> MusicPcmAssetBank::Clips() const noexcept
{
    std::array<MusicPcmClip, kMusicPcmCueCount> result{};
    if (ready_)
    {
        for (std::size_t index = 1u; index < result.size(); ++index)
        {
            result[index] = {bodies_[index], tails_[index]};
        }
    }
    return result;
}

void MusicPcmAssetBank::ReleasePartial() noexcept
{
    for (std::size_t index = 1u; index < bodies_.size(); ++index)
    {
        std::vector<std::int16_t>{}.swap(bodies_[index]);
        std::vector<std::int16_t>{}.swap(tails_[index]);
    }
    ready_ = false;
}

} // namespace horde::audio
