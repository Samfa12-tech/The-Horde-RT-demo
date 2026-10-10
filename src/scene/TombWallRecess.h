#pragma once
#include "scene/TombDressing.h"
#include <algorithm>
#include <vector>

namespace horde::scene {
struct TombWallPanel { float minimumZ, maximumZ, minimumY, maximumY; };
// Split just the selected planar wall and its backing. The native niche owns
// the sealed rear and returns; the ordinary corridor collision stays solid.
inline std::vector<TombWallPanel> TombWallPanels(float logicalX, float minimumZ,
    float maximumZ, float minimumY, float maximumY, bool dressing) {
    if (!dressing) return {{minimumZ, maximumZ, minimumY, maximumY}};
    std::vector<TombDressingNicheOpening> holes;
    for (const auto& opening : kTombDressingNicheOpenings)
        if (opening.wallCoordinate == logicalX && opening.minimumZ >= minimumZ &&
            opening.maximumZ <= maximumZ && opening.minimumY > minimumY &&
            opening.maximumY < maximumY) holes.push_back(opening);
    std::sort(holes.begin(), holes.end(), [](const auto& a, const auto& b) {
        return a.minimumZ < b.minimumZ;
    });
    std::vector<TombWallPanel> panels;
    float cursor = minimumZ;
    for (const auto& opening : holes) {
        if (opening.minimumZ > cursor)
            panels.push_back({cursor, opening.minimumZ, minimumY, maximumY});
        panels.push_back({opening.minimumZ, opening.maximumZ, minimumY, opening.minimumY});
        panels.push_back({opening.minimumZ, opening.maximumZ, opening.maximumY, maximumY});
        cursor = opening.maximumZ;
    }
    if (cursor < maximumZ) panels.push_back({cursor, maximumZ, minimumY, maximumY});
    return panels;
}
} // namespace horde::scene
