#include "pocket_audio/PcmWave.h"

#include <cstddef>
#include <new>
#include <stdexcept>

namespace pocket_audio
{
namespace
{

constexpr std::uint16_t kChannels = 2u;
constexpr std::uint16_t kBitsPerSample = 16u;
constexpr std::uint16_t kBlockAlignment = 4u;
constexpr std::uint32_t kByteRate = 192'000u;
constexpr std::size_t kMaximumContainerOverheadBytes = 65'536u;

bool HasFourCc(const std::span<const std::uint8_t> bytes,
               const std::size_t offset,
               const char (&fourCc)[5]) noexcept
{
    return offset <= bytes.size() && bytes.size() - offset >= 4u &&
           bytes[offset] == static_cast<std::uint8_t>(fourCc[0]) &&
           bytes[offset + 1u] == static_cast<std::uint8_t>(fourCc[1]) &&
           bytes[offset + 2u] == static_cast<std::uint8_t>(fourCc[2]) &&
           bytes[offset + 3u] == static_cast<std::uint8_t>(fourCc[3]);
}

std::uint16_t ReadU16Le(const std::span<const std::uint8_t> bytes,
                        const std::size_t offset) noexcept
{
    return static_cast<std::uint16_t>(bytes[offset]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[offset + 1u]) << 8u);
}

std::uint32_t ReadU32Le(const std::span<const std::uint8_t> bytes,
                        const std::size_t offset) noexcept
{
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u) |
           (static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u) |
           (static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u);
}

std::int16_t DecodePcm16Le(const std::span<const std::uint8_t> bytes,
                           const std::size_t offset) noexcept
{
    const std::uint16_t encoded = ReadU16Le(bytes, offset);
    const std::int32_t signedValue = encoded < 0x8000u
        ? static_cast<std::int32_t>(encoded)
        : static_cast<std::int32_t>(encoded) - 65'536;
    return static_cast<std::int16_t>(signedValue);
}

} // namespace

PcmWaveStatus DecodePcmWave(const std::span<const std::uint8_t> fileBytes,
                            const std::uint32_t expectedFrameCount,
                            std::vector<std::int16_t>& outputSamples) noexcept
{
    outputSamples.clear();
    if (expectedFrameCount == 0u || expectedFrameCount > kPcmWaveMaximumFrameCount)
    {
        return PcmWaveStatus::InvalidExpectedFrameCount;
    }
    const std::size_t expectedDataBytes = static_cast<std::size_t>(expectedFrameCount) *
                                          kBlockAlignment;
    if (fileBytes.size() > expectedDataBytes + kMaximumContainerOverheadBytes)
    {
        return PcmWaveStatus::FileTooLarge;
    }
    if (fileBytes.size() < 12u || !HasFourCc(fileBytes, 0u, "RIFF") ||
        !HasFourCc(fileBytes, 8u, "WAVE"))
    {
        return PcmWaveStatus::InvalidRiffHeader;
    }

    const std::uint32_t riffPayloadBytes = ReadU32Le(fileBytes, 4u);
    if (riffPayloadBytes < 4u ||
        static_cast<std::uint64_t>(riffPayloadBytes) + 8u != fileBytes.size())
    {
        return PcmWaveStatus::InvalidRiffLength;
    }
    const std::size_t riffEnd = fileBytes.size();

    bool hasFormat = false;
    bool hasData = false;
    std::uint16_t formatTag = 0u;
    std::uint16_t channels = 0u;
    std::uint32_t sampleRate = 0u;
    std::uint32_t byteRate = 0u;
    std::uint16_t blockAlignment = 0u;
    std::uint16_t bitsPerSample = 0u;
    std::size_t dataOffset = 0u;
    std::size_t dataBytes = 0u;

    for (std::size_t chunkOffset = 12u; chunkOffset < riffEnd;)
    {
        if (riffEnd - chunkOffset < 8u)
        {
            return PcmWaveStatus::InvalidChunk;
        }

        const std::uint32_t chunkByteCount = ReadU32Le(fileBytes, chunkOffset + 4u);
        const std::size_t payloadOffset = chunkOffset + 8u;
        if (static_cast<std::uint64_t>(chunkByteCount) > riffEnd - payloadOffset)
        {
            return PcmWaveStatus::InvalidChunk;
        }
        const std::size_t payloadBytes = static_cast<std::size_t>(chunkByteCount);
        const std::size_t paddedPayloadBytes = payloadBytes + (payloadBytes & 1u);
        if (paddedPayloadBytes > riffEnd - payloadOffset)
        {
            return PcmWaveStatus::InvalidChunkPadding;
        }

        if (HasFourCc(fileBytes, chunkOffset, "fmt "))
        {
            if (hasFormat)
            {
                return PcmWaveStatus::DuplicateFormatChunk;
            }
            if (payloadBytes < 16u)
            {
                return PcmWaveStatus::InvalidFormatChunk;
            }
            hasFormat = true;
            formatTag = ReadU16Le(fileBytes, payloadOffset);
            channels = ReadU16Le(fileBytes, payloadOffset + 2u);
            sampleRate = ReadU32Le(fileBytes, payloadOffset + 4u);
            byteRate = ReadU32Le(fileBytes, payloadOffset + 8u);
            blockAlignment = ReadU16Le(fileBytes, payloadOffset + 12u);
            bitsPerSample = ReadU16Le(fileBytes, payloadOffset + 14u);
        }
        else if (HasFourCc(fileBytes, chunkOffset, "data"))
        {
            if (hasData)
            {
                return PcmWaveStatus::DuplicateDataChunk;
            }
            hasData = true;
            dataOffset = payloadOffset;
            dataBytes = payloadBytes;
        }

        chunkOffset = payloadOffset + paddedPayloadBytes;
    }

    if (!hasFormat)
    {
        return PcmWaveStatus::MissingFormatChunk;
    }
    if (!hasData)
    {
        return PcmWaveStatus::MissingDataChunk;
    }
    if (formatTag != 1u)
    {
        return PcmWaveStatus::UnsupportedEncoding;
    }
    if (channels != kChannels)
    {
        return PcmWaveStatus::WrongChannelCount;
    }
    if (sampleRate != kPcmSampleRate)
    {
        return PcmWaveStatus::WrongSampleRate;
    }
    if (bitsPerSample != kBitsPerSample)
    {
        return PcmWaveStatus::WrongBitDepth;
    }
    if (blockAlignment != kBlockAlignment)
    {
        return PcmWaveStatus::WrongBlockAlignment;
    }
    if (byteRate != kByteRate)
    {
        return PcmWaveStatus::WrongByteRate;
    }
    if ((dataBytes & 1u) != 0u)
    {
        return PcmWaveStatus::OddDataLength;
    }
    if (dataBytes % kBlockAlignment != 0u)
    {
        return PcmWaveStatus::InvalidDataAlignment;
    }
    if (dataBytes != expectedDataBytes)
    {
        return PcmWaveStatus::WrongFrameCount;
    }

    try
    {
        std::vector<std::int16_t> decoded;
        decoded.resize(static_cast<std::size_t>(expectedFrameCount) * kChannels);
        for (std::size_t sample = 0u; sample < decoded.size(); ++sample)
        {
            decoded[sample] = DecodePcm16Le(fileBytes, dataOffset + sample * 2u);
        }
        outputSamples.swap(decoded);
    }
    catch (const std::bad_alloc&)
    {
        return PcmWaveStatus::AllocationFailure;
    }
    catch (const std::length_error&)
    {
        return PcmWaveStatus::AllocationFailure;
    }

    return PcmWaveStatus::Ok;
}

} // namespace pocket_audio
