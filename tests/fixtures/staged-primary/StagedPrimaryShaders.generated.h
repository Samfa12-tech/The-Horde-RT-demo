#pragma once

// Tiny deterministic header fixture for CPU ownership tests only. This is not
// SPIR-V and provides no shader-validation or image-correctness evidence.
#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace horde::vulkan::raytracing::experimental {
inline constexpr std::uint32_t kStagedPrimaryDescriptorSet = 1u;
inline constexpr std::uint32_t kStagedPrimaryPageCount = 3u;
inline constexpr std::uint32_t kStagedPrimaryRecordBytes = 128u;
inline constexpr std::uint32_t kInstrumentation = 0u; // Shipping
inline constexpr std::uint32_t kQuality = 0u;          // Mobile
static_assert(kInstrumentation == 0u && kQuality == 0u);
static_assert(kStagedPrimaryDescriptorSet == 1u && kStagedPrimaryPageCount == 3u &&
              kStagedPrimaryRecordBytes == 128u);
inline constexpr std::array<std::uint32_t, 1u> kPrimaryOpaqueWords{1u};
inline constexpr std::array<std::uint32_t, 1u> kShadeOpaqueWords{2u};
inline constexpr std::array<std::uint32_t, 1u> kPrimaryGenericWords{3u};
inline constexpr std::array<std::uint32_t, 1u> kShadeGenericWords{4u};
inline constexpr std::string_view kPrimaryOpaqueSha256 = "fixture-primary-opaque";
inline constexpr std::string_view kShadeOpaqueSha256 = "fixture-shade-opaque";
inline constexpr std::string_view kPrimaryGenericSha256 = "fixture-primary-generic";
inline constexpr std::string_view kShadeGenericSha256 = "fixture-shade-generic";
inline constexpr std::string_view kOpaquePairKey = "fixture-opaque";
inline constexpr std::string_view kGenericPairKey = "fixture-generic";
inline constexpr std::string_view kOpaquePairSha256 = "fixture-opaque-pair";
inline constexpr std::string_view kGenericPairSha256 = "fixture-generic-pair";
} // namespace horde::vulkan::raytracing::experimental
