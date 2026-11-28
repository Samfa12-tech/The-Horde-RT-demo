#pragma once

#include <array>
#include <cstddef>

namespace horde::scene::bellwether_woodland {
enum class Species { Pine, Alder };

struct Tree {
    Species species;
    float east, north, yaw, scale;
    bool middleCrown;
};

// East and north metres from the Bellwether terrain survey.
inline constexpr std::array<Tree,32> kBellwetherCopses{{
    // South woodland.
    {Species::Alder,-42.f,-27.f,.35f,1.02f,true},
    {Species::Pine,-47.f,-32.f,2.12f,1.08f,false},
    {Species::Pine,-42.f,-37.f,4.63f,.94f,false},
    {Species::Pine,-36.f,-31.f,1.58f,1.15f,true},
    {Species::Alder,-34.f,-39.f,2.70f,1.03f,true},
    {Species::Pine,-29.f,-44.f,5.41f,1.13f,false},
    {Species::Alder,-24.f,-42.f,1.44f,.90f,true},
    {Species::Pine,-21.f,-49.f,.80f,1.02f,false},
    {Species::Pine,-16.f,-45.f,3.92f,.92f,true},
    {Species::Alder,-12.f,-51.f,2.18f,1.09f,true},
    {Species::Pine,-8.f,-48.f,1.14f,1.17f,true},
    {Species::Pine,-3.f,-54.f,5.27f,1.08f,false},
    {Species::Alder,4.f,-55.f,3.32f,1.07f,true},
    {Species::Pine,10.f,-51.f,4.75f,.97f,true},

    // West slope below the gardens. Stagger the taller crowns and keep
    // the garden terraces and House A lane on the village side of the grove.
    {Species::Pine,-55.f,-24.f,1.18f,1.04f,true},
    {Species::Alder,-58.f,-12.f,4.03f,1.15f,true},
    {Species::Pine,-60.f,-33.f,2.71f,1.12f,true},
    {Species::Alder,-62.f,-21.f,1.20f,0.89f,false},
    {Species::Pine,-64.f,-15.f,5.33f,1.18f,false},
    {Species::Pine,-63.f,-27.f,3.17f,0.86f,false},
    {Species::Alder,-64.f,-41.f,3.42f,1.10f,true},
    {Species::Pine,-67.f,-35.f,5.62f,1.06f,false},
    {Species::Pine,-69.f,-23.f,0.28f,1.20f,false},
    {Species::Alder,-54.f,-47.f,2.88f,1.04f,false},
    {Species::Pine,-71.f,-45.f,4.66f,1.17f,false},
    {Species::Alder,-56.f,-52.f,0.91f,1.12f,false},
    {Species::Pine,-74.f,-28.f,4.32f,1.09f,false},
    {Species::Pine,-74.f,-37.f,2.74f,0.95f,false},
    {Species::Alder,-58.f,-57.f,1.44f,1.08f,false},
    {Species::Pine,-60.f,-52.f,5.24f,1.12f,false},
    {Species::Pine,-79.f,-34.f,3.71f,1.19f,false},
    {Species::Pine,-65.f,-50.f,0.83f,1.08f,false},
}};

inline constexpr std::size_t kTreeTriangles=[] {
    std::size_t total=0;
    for(const auto& tree:kBellwetherCopses) {
        const bool pine=tree.species==Species::Pine;
        const std::size_t bark=pine?20:84;
        const std::size_t crown=tree.middleCrown?1120:(pine?144:96);
        total+=bark+crown;
    }
    return total;
}();
inline constexpr std::size_t kTreeContactTriangles=kBellwetherCopses.size()*10;

struct Understory {
    float x, z, yaw, scale;
};

// Registered engine X/Z metres. The small pockets follow plot edges and
// glimpses of the woodland from the lanes. Keep the surveyed paths open.
inline constexpr std::array<Understory,39> kSouthUnderstory{{
    // West entrance and the back of the market wall.
    {68.10f,134.43f,.18f,.78f}, {67.95f,136.75f,3.18f,1.03f},
    {68.82f,138.22f,5.57f,.99f}, {70.02f,138.91f,1.98f,.87f},
    {72.48f,139.84f,2.49f,.78f}, {73.62f,138.67f,3.82f,1.13f},
    {74.95f,140.04f,.91f,.89f}, {76.54f,138.84f,4.22f,1.04f},
    // Shoulders on the bend and around the store yard.
    {75.81f,144.49f,4.31f,.96f}, {72.39f,146.39f,.52f,1.15f},
    {71.24f,147.68f,1.58f,1.02f}, {70.69f,149.40f,5.80f,.79f},
    {67.85f,151.70f,4.43f,.91f}, {66.88f,154.68f,2.86f,.85f},
    {65.50f,156.33f,.77f,.91f}, {61.96f,147.87f,1.12f,.71f},
    {63.96f,148.49f,4.01f,.95f}, {60.76f,161.68f,.21f,1.13f},
    // Uneven groups at the foot of the two copses.
    {38.89f,155.86f,.59f,.98f}, {36.08f,157.80f,3.04f,1.23f},
    {41.23f,156.60f,4.68f,.83f}, {43.45f,161.43f,5.55f,1.12f},
    {47.50f,162.70f,1.29f,1.18f}, {49.36f,165.65f,2.68f,.97f},
    {54.72f,163.76f,3.72f,1.19f}, {58.37f,169.53f,4.92f,.87f},
    {61.98f,171.34f,1.65f,1.21f}, {66.59f,173.45f,3.62f,1.06f},
    {69.38f,169.30f,5.22f,.96f}, {72.16f,171.33f,2.19f,1.09f},
    {78.66f,177.63f,1.02f,1.12f}, {81.11f,176.16f,3.93f,.93f},
    {89.92f,172.03f,5.72f,1.18f}, {90.17f,179.19f,.58f,.93f},
    // Tavern delivery wall and the far side of the common yard.
    {89.36f,125.65f,4.36f,.79f}, {92.14f,125.46f,1.04f,.87f},
    {94.64f,125.17f,2.07f,1.02f}, {97.03f,142.72f,5.01f,.95f},
    {100.39f,141.29f,3.32f,.97f},
}};

inline constexpr std::size_t kUnderstoryTriangles=kSouthUnderstory.size()*588;
}
