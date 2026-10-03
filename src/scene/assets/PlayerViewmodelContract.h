#pragma once

#include "scene/assets/PlayerPrimitiveContract.h"

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace horde::scene::assets
{

enum class PlayerAssetRole : std::uint8_t
{
    Unspecified,
    WorldBody,
    Viewmodel,
};

struct PlayerViewmodelPrimitiveDeclaration
{
    std::string_view material;
    bool firstPersonPrimary = false;
    bool shadow = false;
    bool reflection = false;
};

struct PlayerViewmodelPrimitiveContract
{
    std::string_view material;
    bool firstPersonPrimary;
    bool shadow;
    bool reflection;
    PlayerTextureGroup textureGroup;
};

inline constexpr std::array<PlayerViewmodelPrimitiveContract, 2u>
    kPlayerViewmodelPrimitiveContract{{
        {"ViewmodelSleeves", true, false, false, PlayerTextureGroup::Body},
        {"ViewmodelGauntlets", true, false, false, PlayerTextureGroup::Gauntlet},
    }};

constexpr const PlayerViewmodelPrimitiveContract*
FindPlayerViewmodelPrimitiveContract(std::string_view material)
{
    for (const auto& part : kPlayerViewmodelPrimitiveContract)
        if (part.material == material) return &part;
    return nullptr;
}

inline bool ValidatePlayerViewmodelPrimitiveNames(
    std::span<const std::string_view> names, std::string& diagnostic)
{
    diagnostic.clear();
    if (names.size() != kPlayerViewmodelPrimitiveContract.size())
    {
        diagnostic = "Player viewmodel contract requires exactly two named primitives.";
        return false;
    }
    std::array<bool, kPlayerViewmodelPrimitiveContract.size()> seen{};
    for (const auto name : names)
    {
        const auto* part = FindPlayerViewmodelPrimitiveContract(name);
        if (part == nullptr)
        {
            diagnostic = "Player viewmodel contract contains an unknown material semantic.";
            return false;
        }
        const auto index = static_cast<std::size_t>(
            part - kPlayerViewmodelPrimitiveContract.data());
        if (seen[index])
        {
            diagnostic = "Player viewmodel contract contains a duplicate material semantic.";
            return false;
        }
        seen[index] = true;
    }
    return true;
}

inline bool ValidatePlayerViewmodelPrimitiveDeclarations(
    std::span<const PlayerViewmodelPrimitiveDeclaration> declarations,
    std::string& diagnostic)
{
    diagnostic.clear();
    if (declarations.size() != kPlayerViewmodelPrimitiveContract.size())
    {
        diagnostic = "Player viewmodel contract requires exactly two declarations.";
        return false;
    }
    std::array<std::string_view, kPlayerViewmodelPrimitiveContract.size()> names{};
    for (std::size_t i = 0u; i < declarations.size(); ++i)
        names[i] = declarations[i].material;
    if (!ValidatePlayerViewmodelPrimitiveNames(names, diagnostic)) return false;
    for (const auto& declaration : declarations)
    {
        const auto* part = FindPlayerViewmodelPrimitiveContract(declaration.material);
        if (part == nullptr || declaration.firstPersonPrimary != part->firstPersonPrimary ||
            declaration.shadow != part->shadow || declaration.reflection != part->reflection)
        {
            diagnostic = "Player viewmodel declaration conflicts with its visibility semantic.";
            return false;
        }
    }
    return true;
}

} // namespace horde::scene::assets
