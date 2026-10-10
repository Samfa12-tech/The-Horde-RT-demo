#pragma once

#include <array>
#include <cstddef>

namespace horde::gameplay::simulation
{

enum class ForestTreeSpecies : unsigned char { Pine, Alder };

// Authored route-bank positions shared by runtime rendering and simulation
// collision. Trunk bounds are measured from the admitted LOD1 GLB bark
// POSITION accessors, retaining the source's native Y-up coordinates. Bounds
// include bark vertices through 2 m above the bark root; foliage stays visual.
struct ForestTreePlacementContract
{
    ForestTreeSpecies species;
    float x;
    float z;
    float yawRadians;
    float trunkMinX;
    float trunkMinZ;
    float trunkMaxX;
    float trunkMaxZ;
};

inline constexpr std::array<ForestTreePlacementContract, 10u> kForestTreePlacementContract{{
    {ForestTreeSpecies::Pine, -29.794079338f, -15.589943330f, 0.000000000f, -30.863443886f, -16.487786452f, -28.786482845f, -14.622606377f},
    {ForestTreeSpecies::Alder, -37.605920662f, -10.010056670f, 0.314159265f, -38.826573080f, -10.880634711f, -36.522706943f, -8.819153571f},
    {ForestTreeSpecies::Alder, -25.639155895f, -9.497463098f, 0.628318531f, -26.697655388f, -10.411633529f, -24.687246733f, -8.256177950f},
    {ForestTreeSpecies::Pine, -31.760844105f, -2.102536902f, 0.942477796f, -32.660476016f, -2.992535184f, -30.858224416f, -1.222520488f},
    {ForestTreeSpecies::Pine, -11.494337832f, -2.113526686f, 1.256637061f, -12.349338743f, -3.093665310f, -10.638433329f, -1.108073099f},
    {ForestTreeSpecies::Alder, -19.905662168f, 2.513526686f, 0.000000000f, -21.170116056f, 1.570182093f, -18.796517242f, 3.538620718f},
    {ForestTreeSpecies::Alder, -13.601898786f, 12.700886869f, 0.314159265f, -14.822551204f, 11.830308828f, -12.518685067f, 13.891789968f},
    {ForestTreeSpecies::Pine, -21.798101214f, 17.699113131f, 0.628318531f, -22.690757936f, 16.710861766f, -20.935324933f, 18.706506628f},
    {ForestTreeSpecies::Pine, -1.194931918f, 16.105536189f, 0.942477796f, -2.094563829f, 15.215537907f, -0.292312229f, 16.985552603f},
    {ForestTreeSpecies::Alder, -6.205068082f, 24.294463811f, 1.256637061f, -7.229667702f, 23.265079796f, -5.163768764f, 25.481583153f},
}};

} // namespace horde::gameplay::simulation
