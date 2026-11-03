#pragma once
#include "gameplay/traversal/ChapterTerrainMovement.h"
#include <cmath>

namespace horde::gameplay::healing {
// Furniture and interaction share the same placement. The saved checkpoint
// remains the clear authored landing, not the centre of a solid stone basin.
inline traversal::Vec3 FlaskRestBasinCentre(traversal::Vec3 anchor,bool lookout) {
    anchor.x+=lookout?-.78f:.78f;
    return anchor;
}
inline bool FlaskRestReachClear(const simulation::TriangleSupportSurface& terrain,
                               traversal::Vec3 player,traversal::Vec3 centre) {
    if(std::abs(player.y-centre.y)>.35f ||
       std::hypot(std::hypot(player.x-centre.x,player.z-centre.z),player.y-centre.y)>1.f)return false;
    // The hand-height path is above the basin rim/relief, but still blocked by
    // walls, trees and other physical boundaries. Never interact through them.
    return traversal::ChapterExteriorObstacleClear(terrain,player.x,player.z,centre.x,centre.z,
        std::min(player.y,centre.y)+.85f,std::max(player.y,centre.y)+.95f,.03f);
}
}
