#pragma once

#include "scene/ShowcaseOverheadGeometry.h"

#include <array>

namespace horde::scene
{

struct EntryPortalCapFace
{
    std::array<std::array<float, 3u>, 4u> vertices{};
    unsigned int normalCode = 0u;
    unsigned int materialCode = 0u;
};

// The existing route ceiling is the cap's top face. These five remaining
// faces derive their footprint and lower bound from the shared overhead solid;
// the top meets, but does not duplicate, the ceiling plane.
inline constexpr std::array<EntryPortalCapFace, 5u> EntryPortalCapFaces()
{
    const auto& volume = kShowcaseLowOverheadVolumes[1u];
    const float minX = volume.footprint[1u][0];
    const float maxX = volume.footprint[2u][0];
    const float frontZ = volume.footprint[0u][1];
    const float backZ = volume.footprint[1u][1];
    const float bottomY = volume.bottomY;
    const float topY = kShowcaseRouteCeilingWorldY;
    return {{
        {{{{{minX, bottomY, frontZ}}, {{maxX, bottomY, frontZ}},
           {{maxX, topY, frontZ}}, {{minX, topY, frontZ}}}}, 4u, 2u},
        {{{{{maxX, bottomY, backZ}}, {{minX, bottomY, backZ}},
           {{minX, topY, backZ}}, {{maxX, topY, backZ}}}}, 5u, 2u},
        {{{{{minX, bottomY, backZ}}, {{minX, bottomY, frontZ}},
           {{minX, topY, frontZ}}, {{minX, topY, backZ}}}}, 3u, 2u},
        {{{{{maxX, bottomY, frontZ}}, {{maxX, bottomY, backZ}},
           {{maxX, topY, backZ}}, {{maxX, topY, frontZ}}}}, 2u, 2u},
        {{{{{minX, bottomY, frontZ}}, {{minX, bottomY, backZ}},
           {{maxX, bottomY, backZ}}, {{maxX, bottomY, frontZ}}}}, 1u, 2u},
    }};
}

} // namespace horde::scene
