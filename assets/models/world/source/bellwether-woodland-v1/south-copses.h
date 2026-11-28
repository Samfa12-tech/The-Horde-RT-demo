#pragma once

#include <array>
#include <cstddef>

namespace horde::scene::bellwether_woodland {
enum class Species { Pine, Alder };

struct Tree {
    Species species;
    float east, north, yaw, scale;
};

// Survey metres. The two copses sit beyond the west garden and store, and
// behind the outbuilding. The smith lane and the southeast road stay open.
inline constexpr std::array<Tree,14> kSouthCopses{{
    {Species::Alder,-42.f,-27.f,.35f,1.02f},
    {Species::Pine,-47.f,-32.f,2.12f,1.08f},
    {Species::Pine,-42.f,-37.f,4.63f,.94f},
    {Species::Pine,-36.f,-31.f,1.58f,1.15f},
    {Species::Alder,-34.f,-39.f,2.70f,1.03f},
    {Species::Pine,-29.f,-44.f,5.41f,1.13f},
    {Species::Alder,-24.f,-42.f,1.44f,.90f},
    {Species::Pine,-21.f,-49.f,.80f,1.02f},
    {Species::Pine,-16.f,-45.f,3.92f,.92f},
    {Species::Alder,-12.f,-51.f,2.18f,1.09f},
    {Species::Pine,-8.f,-48.f,1.14f,1.17f},
    {Species::Pine,-3.f,-54.f,5.27f,1.08f},
    {Species::Alder,4.f,-55.f,3.32f,1.07f},
    {Species::Pine,10.f,-51.f,4.75f,.97f},
}};

inline constexpr std::size_t kTreeTriangles=[] {
    std::size_t total=0;
    for(const auto& tree:kSouthCopses)total+=tree.species==Species::Pine?164:180;
    return total;
}();
inline constexpr std::size_t kTreeContactTriangles=kSouthCopses.size()*10;
}
