#pragma once

#include "vulkan/raytracing/RtPipelineVariants.h"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace horde::vulkan::raytracing {

struct RtPipelineVariantArtifact {
    RtPipelineVariantKey key;
    std::span<const std::uint32_t> words;
    std::string_view canonicalKey;
    std::string_view spirvSha256;
    std::string_view includeSha256;
};

class RtPipelineVariantProvider final {
public:
    [[nodiscard]] static const RtPipelineVariantProvider& Compiled() noexcept;
    [[nodiscard]] const RtPipelineBundleRequest& request() const noexcept;
    [[nodiscard]] std::optional<RtPipelineVariantArtifact> ResolveExact(
        RtPipelineVariantKey requested, std::string* error = nullptr) const;

private:
    explicit constexpr RtPipelineVariantProvider(RtPipelineBundleRequest request) noexcept : request_(request) {}
    RtPipelineBundleRequest request_;
};

} // namespace horde::vulkan::raytracing
