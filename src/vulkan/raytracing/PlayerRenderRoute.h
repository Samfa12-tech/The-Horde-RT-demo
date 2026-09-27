#pragma once

#include <cstdint>
#include <string_view>

namespace horde::vulkan::raytracing
{

enum class PlayerRenderRoute : std::uint8_t
{
    Procedural,
    Skinned,
    // Explicit diagnostic comparison only; never a silent gameplay fallback.
    HybridBlockPrimary,
    ModelledViewmodel,
};

inline constexpr PlayerRenderRoute kProductionPlayerRenderRoute =
    PlayerRenderRoute::ModelledViewmodel;

// A/B checkpoints remain available without changing ordinary gameplay,
// showcase/replay or glass workloads back to the retired block-arm route.
constexpr PlayerRenderRoute PlayerRenderRouteForCheckpoint(std::string_view name)
{
    if (name.starts_with("player-fallback-")) return PlayerRenderRoute::Procedural;
    if (name.starts_with("player-body-")) return PlayerRenderRoute::Skinned;
    return kProductionPlayerRenderRoute;
}

} // namespace horde::vulkan::raytracing
