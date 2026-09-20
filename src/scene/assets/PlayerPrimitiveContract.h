#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace horde::scene::assets
{

// Stable semantic identities, not GLB primitive/material indices or TLAS slots.
enum class PlayerPrimitiveSemantic : std::uint8_t
{
    Body,
    Head,
    NearFace,
    GauntletPrimaryVisible,
};

// Relative texture groups; the renderer owns actual array-layer allocation.
enum class PlayerTextureGroup : std::uint8_t { Body, Gauntlet };

struct PlayerPrimitiveDeclaration
{
    std::string_view material;
    bool firstPersonPrimary = false;
    bool shadow = false;
    bool reflection = false;
};

struct PlayerPrimitiveContract
{
    PlayerPrimitiveSemantic semantic;
    std::string_view material;
    bool firstPersonPrimary;
    bool shadow;
    bool reflection;
    PlayerTextureGroup textureGroup;
};

inline constexpr std::array<PlayerPrimitiveContract, 4> kPlayerPrimitiveContract{{
    {PlayerPrimitiveSemantic::Body, "BodyPrimaryVisible", true, true, true, PlayerTextureGroup::Body},
    {PlayerPrimitiveSemantic::Head, "HeadPrimaryMasked", false, true, true, PlayerTextureGroup::Body},
    {PlayerPrimitiveSemantic::NearFace, "NearFacePrimaryMasked", false, true, true, PlayerTextureGroup::Body},
    {PlayerPrimitiveSemantic::GauntletPrimaryVisible, "GauntletPrimaryVisible", true, true, true, PlayerTextureGroup::Gauntlet},
}};

constexpr const PlayerPrimitiveContract* FindPlayerPrimitiveContract(std::string_view material)
{
    for (const auto& part : kPlayerPrimitiveContract)
        if (part.material == material) return &part;
    return nullptr;
}

constexpr const PlayerPrimitiveContract* FindPlayerPrimitiveContract(PlayerPrimitiveSemantic semantic)
{
    for (const auto& part : kPlayerPrimitiveContract)
        if (part.semantic == semantic) return &part;
    return nullptr;
}

inline bool ValidatePlayerPrimitiveNames(std::span<const std::string_view> names,
                                         std::string& diagnostic)
{
    diagnostic.clear();
    if (names.size() != kPlayerPrimitiveContract.size())
    {
        diagnostic = "Player primitive contract requires exactly four named primitives.";
        return false;
    }
    std::array<bool, kPlayerPrimitiveContract.size()> seen{};
    for (const auto name : names)
    {
        const auto* part = FindPlayerPrimitiveContract(name);
        if (part == nullptr)
        {
            diagnostic = "Player primitive contract contains an unknown material semantic.";
            return false;
        }
        const auto index = static_cast<std::size_t>(part - kPlayerPrimitiveContract.data());
        if (seen[index])
        {
            diagnostic = "Player primitive contract contains a duplicate material semantic.";
            return false;
        }
        seen[index] = true;
    }
    return true;
}

inline bool ValidatePlayerPrimitiveDeclarations(
    std::span<const PlayerPrimitiveDeclaration> declarations, std::string& diagnostic)
{
    diagnostic.clear();
    if (declarations.size() != kPlayerPrimitiveContract.size())
    {
        diagnostic = "Player primitive contract requires exactly four declarations.";
        return false;
    }
    std::array<std::string_view, kPlayerPrimitiveContract.size()> names{};
    for (std::size_t i = 0; i < declarations.size(); ++i) names[i] = declarations[i].material;
    if (!ValidatePlayerPrimitiveNames(names, diagnostic)) return false;
    for (const auto& declaration : declarations)
    {
        const auto* part = FindPlayerPrimitiveContract(declaration.material);
        if (part == nullptr || declaration.firstPersonPrimary != part->firstPersonPrimary ||
            declaration.shadow != part->shadow || declaration.reflection != part->reflection)
        {
            diagnostic = "Player primitive declaration conflicts with its visibility semantic.";
            return false;
        }
    }
    return true;
}

} // namespace horde::scene::assets
