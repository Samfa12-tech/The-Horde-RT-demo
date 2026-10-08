#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace horde::vulkan
{
// A bounded, process-local seed for the next VkPipelineCache on the same
// physical device. This stores only opaque bytes returned by the Vulkan
// driver; it does not create Vulkan objects or perform disk I/O.
class PipelineCacheSeed
{
public:
    static constexpr std::size_t kHeaderSize = 32u;
    static constexpr std::size_t kMaximumSize = 16u * 1024u * 1024u;
    static constexpr std::uint32_t kHeaderVersionOne = 1u;

    struct DeviceIdentity
    {
        std::uint32_t vendorId = 0u;
        std::uint32_t deviceId = 0u;
        std::array<std::uint8_t, 16u> pipelineCacheUuid{};

        bool operator==(const DeviceIdentity&) const = default;
    };

    // Admit only a complete successful vkGetPipelineCacheData result. Vulkan
    // returns VK_INCOMPLETE when the supplied buffer was too small; that data
    // must not replace a complete seed. Validation and allocation happen in a
    // temporary so every rejected candidate leaves the current seed intact.
    bool ReplaceFromDriverData(const DeviceIdentity& device,
                               const std::span<const std::byte> bytes,
                               const bool retrievalSucceeded,
                               const bool retrievalIncomplete) noexcept
    {
        if (!retrievalSucceeded || retrievalIncomplete || !ValidHeader(device, bytes))
            return false;

        try
        {
            std::vector<std::byte> replacement(bytes.begin(), bytes.end());
            bytes_.swap(replacement);
            device_ = device;
            valid_ = true;
            return true;
        }
        catch (...)
        {
            // Cache seeding is an optimization. Allocation failure must not
            // escape into renderer recovery or discard the previous seed.
            return false;
        }
    }

    // The returned span borrows this object's storage. Access is owner-thread
    // only; callers must not mutate or clear the seed while using the span.
    std::span<const std::byte> ForDevice(const DeviceIdentity& device) const noexcept
    {
        if (!valid_ || device_ != device) return {};
        return bytes_;
    }

    void Clear() noexcept
    {
        std::vector<std::byte>{}.swap(bytes_);
        device_ = {};
        valid_ = false;
    }

    bool Empty() const noexcept { return !valid_; }
    std::size_t Size() const noexcept { return valid_ ? bytes_.size() : 0u; }

private:
    static std::uint32_t ReadLittleEndianU32(const std::span<const std::byte> bytes,
                                           const std::size_t offset) noexcept
    {
        return static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset])) |
               (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset + 1u])) << 8u) |
               (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset + 2u])) << 16u) |
               (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset + 3u])) << 24u);
    }

    static bool ValidHeader(const DeviceIdentity& device,
                            const std::span<const std::byte> bytes) noexcept
    {
        if (bytes.size() < kHeaderSize || bytes.size() > kMaximumSize) return false;
        if (ReadLittleEndianU32(bytes, 0u) != kHeaderSize ||
            ReadLittleEndianU32(bytes, 4u) != kHeaderVersionOne ||
            ReadLittleEndianU32(bytes, 8u) != device.vendorId ||
            ReadLittleEndianU32(bytes, 12u) != device.deviceId)
            return false;
        for (std::size_t index = 0u; index < device.pipelineCacheUuid.size(); ++index)
            if (std::to_integer<std::uint8_t>(bytes[16u + index]) != device.pipelineCacheUuid[index])
                return false;
        return true;
    }

    std::vector<std::byte> bytes_;
    DeviceIdentity device_{};
    bool valid_ = false;
};
} // namespace horde::vulkan
