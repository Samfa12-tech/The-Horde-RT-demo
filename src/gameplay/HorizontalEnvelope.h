#pragma once

#include "gameplay/ShowcaseRoute.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace horde::gameplay
{
// Exact rectangular cover of an orthogonal footprint union. Bridging aligned
// rooms is essential: independently insetting adjacent rooms creates a false
// gap at a real open portal. This cover never bridges an actual empty slab.
inline std::vector<RouteRect> RectangularUnionCover(std::span<const RouteRect> rooms)
{
    std::vector<float> xs, zs;
    for (const auto& r : rooms)
    {
        if (!std::isfinite(r.minX) || !std::isfinite(r.maxX) ||
            !std::isfinite(r.minZ) || !std::isfinite(r.maxZ) ||
            r.minX >= r.maxX || r.minZ >= r.maxZ) return {};
        xs.insert(xs.end(), {r.minX,r.maxX}); zs.insert(zs.end(), {r.minZ,r.maxZ});
    }
    const auto unique = [](auto& values) {
        std::sort(values.begin(),values.end());
        values.erase(std::unique(values.begin(),values.end()),values.end());
    };
    unique(xs); unique(zs);
    std::vector<RouteRect> cover;
    for (std::size_t a = 0; a < xs.size(); ++a)
    for (std::size_t b = a+1; b < xs.size(); ++b)
    {
        std::optional<std::size_t> first;
        for (std::size_t z = 0; z < zs.size(); ++z)
        {
            bool filled = z+1 < zs.size();
            for (std::size_t x = a; filled && x < b; ++x)
                filled = std::any_of(rooms.begin(),rooms.end(),[&](const auto& r) {
                    return r.minX <= xs[x] && r.maxX >= xs[x+1] &&
                           r.minZ <= zs[z] && r.maxZ >= zs[z+1];
                });
            if (filled && !first) first = z;
            if (!filled && first) { cover.push_back({xs[a],xs[b],zs[*first],zs[z]}); first.reset(); }
        }
    }
    std::vector<RouteRect> maximal;
    for (const auto& a : cover)
        if (!std::any_of(cover.begin(),cover.end(),[&](const auto& b) {
            return b.minX <= a.minX && b.maxX >= a.maxX && b.minZ <= a.minZ && b.maxZ >= a.maxZ &&
                (b.minX != a.minX || b.maxX != a.maxX || b.minZ != a.minZ || b.maxZ != a.maxZ);
        })) maximal.push_back(a);
    return maximal;
}

// Fit the complete horizontal rectangle, rather than just a centre or a few
// perimeter samples. Offsets may be asymmetric for an oriented item; the
// complete rectangle is certified by the union cover above.
inline std::optional<RoutePosition> ClosestHorizontalEnvelopeCentre(
    std::span<const RouteRect> cover, RoutePosition preferred, const RouteRect& offsets)
{
    if (!std::isfinite(preferred.x) || !std::isfinite(preferred.z) ||
        !std::isfinite(offsets.minX) || !std::isfinite(offsets.maxX) ||
        !std::isfinite(offsets.minZ) || !std::isfinite(offsets.maxZ) ||
        offsets.minX >= offsets.maxX || offsets.minZ >= offsets.maxZ) return std::nullopt;
    double distance = std::numeric_limits<double>::infinity();
    std::optional<RoutePosition> result;
    for (const auto& r : cover)
    {
        if (!std::isfinite(r.minX) || !std::isfinite(r.maxX) ||
            !std::isfinite(r.minZ) || !std::isfinite(r.maxZ)) return std::nullopt;
        const float x0=r.minX-offsets.minX,x1=r.maxX-offsets.maxX,
                    z0=r.minZ-offsets.minZ,z1=r.maxZ-offsets.maxZ;
        if (x0 > x1 || z0 > z1) continue;
        const RoutePosition p{std::clamp(preferred.x,x0,x1),std::clamp(preferred.z,z0,z1)};
        const double dx=static_cast<double>(p.x)-preferred.x,dz=static_cast<double>(p.z)-preferred.z;
        const double candidate=dx*dx+dz*dz;
        if (candidate < distance) { distance=candidate; result=p; }
    }
    return result;
}
inline std::optional<RoutePosition> ClosestHorizontalEnvelopeCentre(
    std::span<const RouteRect> cover, RoutePosition preferred, float radius)
{
    if (!std::isfinite(radius) || radius <= 0) return std::nullopt;
    return ClosestHorizontalEnvelopeCentre(cover,preferred,{-radius,radius,-radius,radius});
}
}
