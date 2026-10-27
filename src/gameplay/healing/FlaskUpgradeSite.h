#pragma once
#include "gameplay/items/HeldItemKinematics.h"
#include <algorithm>
#include <cmath>
#include <optional>

namespace horde::gameplay::healing {
// Immutable placement, not saved entitlement or a gameplay request.
struct FlaskUpgradeSite {
    items::HeldItemTransform worldFromItem=items::IdentityHeldItemTransform();
    std::array<float,3> supportPoint{};
    bool operator==(const FlaskUpgradeSite&) const = default;
};
struct FlaskUpgradeReadiness {
    std::uint64_t resourceGeneration=0;
    std::optional<FlaskUpgradeSite> site;
};
inline bool ValidateFlaskUpgradeSite(const FlaskUpgradeSite& site,std::string& diagnostic) {
    if(!std::all_of(site.worldFromItem.begin(),site.worldFromItem.end(),[](float v){return std::isfinite(v);}) ||
       !std::all_of(site.supportPoint.begin(),site.supportPoint.end(),[](float v){return std::isfinite(v);}) ||
       std::abs(site.worldFromItem[12]-site.supportPoint[0])>.0001f ||
       std::abs(site.worldFromItem[14]-site.supportPoint[2])>.0001f) {
        diagnostic="Flask upgrade requires one finite coherent world placement.";return false;
    }
    return items::ValidateHeldItemSocketTransform(site.worldFromItem,diagnostic);
}
struct FlaskUpgradeSnapshot {
    std::optional<FlaskUpgradeSite> site;
    std::uint64_t worldGeneration=0,resourceGeneration=0;
    bool interactEligible=false;
};
}
