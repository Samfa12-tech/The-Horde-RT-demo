#pragma once
#include "scene/TombDressing.h"
#include <algorithm>
#include <vector>

namespace horde::scene {
struct TombWallPanel {
    float minimumZ, maximumZ, minimumY, maximumY;
    float minimumYAtMaximumZ;
    TombWallPanel(float lowZ,float highZ,float lowY,float highY)
        : minimumZ(lowZ),maximumZ(highZ),minimumY(lowY),maximumY(highY),minimumYAtMaximumZ(lowY) {}
    TombWallPanel(float lowZ,float highZ,float lowY,float highY,float farLowY)
        : minimumZ(lowZ),maximumZ(highZ),minimumY(lowY),maximumY(highY),minimumYAtMaximumZ(farLowY) {}
};
// Shared by the real RT builder and source-triangle seam regressions. Order
// faces -X; the +X wall reverses these same corners, without a proxy surface.
inline std::array<std::array<float,3u>,4u> TombWallPanelVertices(
    const TombWallPanel& panel, float x) {
    return {{{{x,panel.minimumY,panel.minimumZ}},{{x,panel.minimumYAtMaximumZ,panel.maximumZ}},
             {{x,panel.maximumY,panel.maximumZ}},{{x,panel.maximumY,panel.minimumZ}}}};
}

struct TombWallCut {
    float minimumZ,maximumZ,minimumY;
    // Piecewise linear top follows the existing module's stone frame, with
    // overlap on the stone side of the visible cavity. No new sealing proxy.
    std::array<std::array<float,2u>,8u> topZY{};
    std::size_t topPointCount;
};
// Fitted to actual world-baked GLB triangle sections, not module bounds. These
// cut loops follow jamb/lintel/shelf solids at both wall planes. The arch loop
// passes through the radial middle of the seven existing voussoirs, preserving
// the complete inner arch and native back. Geometry fitting is not an eviction
// certificate: exact-source radial seams still require a contributor proof.
inline constexpr std::array<TombWallCut,3u> kTombWallCuts{{
    {-.96f,-.44f,kEntryRectNicheShelfTop-.015f,
     {{{{-.96f,.14771484f}},{{-.44f,.14771484f}}}},2u},
    // The wide lower mass owns the complete bottom edge, including both
    // shelf-to-jamb corners; the narrower shelf alone does not seal them.
    {-2.8176686f,-2.3823314f,-.43f,
     {{{{-2.8176686f,-.03812498f}},{{-2.7961126f,.057058407f}},
       {{-2.7357142f,.13338929f}},{{-2.6484359f,.17574981f}},
       {{-2.5515640f,.17574981f}},{{-2.4642856f,.13338929f}},
       {{-2.4038872f,.057058407f}},{{-2.3823314f,-.03812498f}}}},8u},
    {-12.6472f,-12.1528f,kOuterNicheShelfTop-.015f,
     {{{{-12.6472f,.14603513f}},{{-12.1528f,.14603513f}}}},2u},
}};
// Split just the selected planar wall and its backing. The native niche owns
// the sealed rear and returns; the ordinary corridor collision stays solid.
inline std::vector<TombWallPanel> TombWallPanels(float logicalX, float minimumZ,
    float maximumZ, float minimumY, float maximumY, bool dressing) {
    if (!dressing) return {{minimumZ, maximumZ, minimumY, maximumY}};
    std::vector<const TombWallCut*> holes;
    for (std::size_t i=0;i<kTombDressingNicheOpenings.size();++i) {
        const auto& opening=kTombDressingNicheOpenings[i];
        const auto& cut=kTombWallCuts[i];
        if (opening.wallCoordinate == logicalX && cut.minimumZ >= minimumZ &&
            cut.maximumZ <= maximumZ && cut.minimumY > minimumY &&
            std::all_of(cut.topZY.begin(),cut.topZY.begin()+cut.topPointCount,
                [&](const auto& point){return point[1]<maximumY;})) holes.push_back(&cut);
    }
    std::sort(holes.begin(), holes.end(), [](const auto* a, const auto* b) {
        return a->minimumZ < b->minimumZ;
    });
    std::vector<TombWallPanel> panels;
    float cursor = minimumZ;
    for (const auto* cut : holes) {
        if (cut->minimumZ > cursor)
            panels.push_back({cursor, cut->minimumZ, minimumY, maximumY});
        panels.push_back({cut->minimumZ, cut->maximumZ, minimumY, cut->minimumY});
        for(std::size_t i=1;i<cut->topPointCount;++i) {
            const auto& first=cut->topZY[i-1];const auto& last=cut->topZY[i];
            panels.push_back({first[0],last[0],first[1],maximumY,last[1]});
        }
        cursor = cut->maximumZ;
    }
    if (cursor < maximumZ) panels.push_back({cursor, maximumZ, minimumY, maximumY});
    return panels;
}
} // namespace horde::scene
