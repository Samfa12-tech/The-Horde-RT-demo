#pragma once
#include <cmath>
#include <cstdint>

namespace horde::gameplay::simulation {
// A renderer-owned spatial readiness lease. It restricts translation only
// while absent detail is irrelevant; it never changes physical collision.
// The renderer must restore a complete owner before releasing the lease.
struct StaticDetailLease {
    std::uint64_t resourceGeneration = 0;
    std::uint64_t worldGeneration = 0;
    float minX = 0, maxX = 0, minZ = 0, maxZ = 0;
    bool restoreRequested = false;
    bool Contains(float x, float z) const noexcept {
        return std::isfinite(x) && std::isfinite(z) && x>=minX && x<=maxX && z>=minZ && z<=maxZ;
    }
    bool Valid() const noexcept {
        return resourceGeneration && worldGeneration && std::isfinite(minX) && std::isfinite(maxX) &&
            std::isfinite(minZ) && std::isfinite(maxZ) && minX<maxX && minZ<maxZ;
    }
};
}
