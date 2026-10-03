#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <optional>

namespace horde::vulkan::raytracing::experimental {

// Must agree with staged_primary_record.glsl. Scalars are bit-preserved,
// not a quantised PBR ABI. The two omitted bools are losslessly recoverable.
inline constexpr std::uint32_t kStagedRecordBytes = 128u;
inline constexpr std::uint32_t kStagedPageCount = 3u;
inline constexpr std::uint32_t kStagedGuardBit = 0x80000000u;
inline constexpr std::uint32_t kStagedInstanceMask = 0x00ffffffu;
struct StagedPrimaryRecord { std::array<std::uint32_t, 32u> words{}; };
static_assert(sizeof(StagedPrimaryRecord) == kStagedRecordBytes);

struct StagedPrimaryExtent {
    std::uint32_t pixelCount = 0u;
    std::uint32_t pageCapacity = 0u;
    std::uint64_t pageBytes = 0u;
    std::uint64_t logicalBytes = 0u;
    std::uint64_t paddedBufferBytes = 0u;
    std::uint64_t logicalReadWriteBytes = 0u;
};

[[nodiscard]] constexpr std::optional<StagedPrimaryExtent> TryMakeStagedPrimaryExtent(
    std::uint32_t width, std::uint32_t height, std::uint64_t maxStorageBufferRange) noexcept
{
    const std::uint64_t pixels = static_cast<std::uint64_t>(width) * height;
    // GLSL pixel addressing is uint, not an implicitly truncated uint64.
    if (width == 0u || height == 0u || pixels > std::numeric_limits<std::uint32_t>::max())
        return std::nullopt;
    const auto capacity = pixels / kStagedPageCount + (pixels % kStagedPageCount != 0u ? 1u : 0u);
    const auto pageBytes = capacity * kStagedRecordBytes;
    if (pageBytes > maxStorageBufferRange) return std::nullopt;
    return StagedPrimaryExtent{static_cast<std::uint32_t>(pixels),
                              static_cast<std::uint32_t>(capacity), pageBytes,
                              pixels * kStagedRecordBytes, pageBytes * kStagedPageCount,
                              pixels * kStagedRecordBytes * 2u};
}

[[nodiscard]] constexpr std::optional<std::uint32_t> PackStagedInstance(
    std::uint32_t instance, bool spawnGuarded) noexcept
{
    if ((instance & ~kStagedInstanceMask) != 0u) return std::nullopt;
    return instance | (spawnGuarded ? kStagedGuardBit : 0u);
}

} // namespace horde::vulkan::raytracing::experimental
