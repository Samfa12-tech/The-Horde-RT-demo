#pragma once

#include <algorithm>
#include <cmath>

#include "gameplay/ShowcaseRoute.h"

namespace horde::gameplay
{

namespace detail
{

constexpr bool InsideInsetRect(const RouteRect& rect, float x, float z)
{
    const float minX = rect.minX - kPlayerCollisionRadius;
    const float maxX = rect.maxX + kPlayerCollisionRadius;
    const float minZ = rect.minZ - kPlayerCollisionRadius;
    const float maxZ = rect.maxZ + kPlayerCollisionRadius;
    return x > minX && x < maxX && z > minZ && z < maxZ;
}

constexpr bool InsideRadiusInsetRect(const RouteRect& rect, float x, float z)
{
    return x >= rect.minX + kPlayerCollisionRadius &&
           x <= rect.maxX - kPlayerCollisionRadius &&
           z >= rect.minZ + kPlayerCollisionRadius &&
           z <= rect.maxZ - kPlayerCollisionRadius;
}

constexpr bool InsideSharedEdgePortal(const RouteRect& a, const RouteRect& b, float x, float z)
{
    if (a.maxX == b.minX || b.maxX == a.minX)
    {
        const float edgeX = a.maxX == b.minX ? a.maxX : b.maxX;
        const float overlapMin = std::max(a.minZ, b.minZ) + kPlayerCollisionRadius;
        const float overlapMax = std::min(a.maxZ, b.maxZ) - kPlayerCollisionRadius;
        return overlapMin <= overlapMax && x >= edgeX - kPlayerCollisionRadius &&
               x <= edgeX + kPlayerCollisionRadius && z >= overlapMin && z <= overlapMax;
    }
    if (a.maxZ == b.minZ || b.maxZ == a.minZ)
    {
        const float edgeZ = a.maxZ == b.minZ ? a.maxZ : b.maxZ;
        const float overlapMin = std::max(a.minX, b.minX) + kPlayerCollisionRadius;
        const float overlapMax = std::min(a.maxX, b.maxX) - kPlayerCollisionRadius;
        return overlapMin <= overlapMax && z >= edgeZ - kPlayerCollisionRadius &&
               z <= edgeZ + kPlayerCollisionRadius && x >= overlapMin && x <= overlapMax;
    }
    return false;
}

constexpr bool InsideWalkableUnion(float x, float z)
{
    for (const RouteRect& rect : kShowcaseWalkableRects)
    {
        if (InsideRadiusInsetRect(rect, x, z))
        {
            return true;
        }
    }
    for (std::size_t i = 0; i < kShowcaseWalkableRects.size(); ++i)
    {
        for (std::size_t j = i + 1; j < kShowcaseWalkableRects.size(); ++j)
        {
            if (InsideSharedEdgePortal(kShowcaseWalkableRects[i], kShowcaseWalkableRects[j], x, z))
            {
                return true;
            }
        }
    }
    return false;
}

constexpr bool IsPlayerPositionWalkable(float x, float z)
{
    if (!InsideWalkableUnion(x, z))
    {
        return false;
    }
    for (const RouteRect& obstacle : kShowcaseSolidObstacles)
    {
        if (InsideInsetRect(obstacle, x, z))
        {
            return false;
        }
    }
    return true;
}

inline bool IsWalkableSweep(RoutePosition previous, RoutePosition proposed)
{
    const float dx = proposed.x - previous.x;
    const float dz = proposed.z - previous.z;
    const float distance = std::sqrt(dx * dx + dz * dz);
    const int steps = std::max(1, static_cast<int>(std::ceil(distance / (kPlayerCollisionRadius * 0.5f))));
    for (int step = 1; step <= steps; ++step)
    {
        const float t = static_cast<float>(step) / static_cast<float>(steps);
        if (!IsPlayerPositionWalkable(previous.x + dx * t, previous.z + dz * t))
        {
            return false;
        }
    }
    return true;
}

} // namespace detail

inline bool IsShowcasePlayerPositionWalkable(float x, float z)
{
    return detail::IsPlayerPositionWalkable(x, z);
}

inline float ComputeShowcaseHeldPropDepth(float cameraX, float cameraZ, float forwardX, float forwardZ)
{
    float depth = 0.30f;
    for (float distance = 0.30f; distance <= 1.0501f; distance += 0.075f)
    {
        if (!IsShowcasePlayerPositionWalkable(cameraX + forwardX * distance,
                                              cameraZ + forwardZ * distance))
        {
            break;
        }
        depth = distance;
    }
    return std::clamp(depth - 0.075f, 0.30f, 1.05f);
}

inline void ResolveCorridorPlayerCollision(float previousX, float previousZ, float& proposedX, float& proposedZ)
{
    const RoutePosition previous{previousX, previousZ};
    const RoutePosition proposed{proposedX, proposedZ};
    if (detail::IsWalkableSweep(previous, proposed))
    {
        return;
    }

    const RoutePosition xOnly{proposedX, previousZ};
    if (detail::IsWalkableSweep(previous, xOnly))
    {
        proposedZ = previousZ;
        return;
    }

    const RoutePosition zOnly{previousX, proposedZ};
    if (detail::IsWalkableSweep(previous, zOnly))
    {
        proposedX = previousX;
        return;
    }

    proposedX = previousX;
    proposedZ = previousZ;
}

inline constexpr RouteRect kSkeletonEnemyArena{-1.85f, 1.85f, -6.40f, 3.40f};

inline bool IsSkeletonEnemyPositionWalkable(float x, float z)
{
    if (!detail::InsideRadiusInsetRect(kSkeletonEnemyArena, x, z))
    {
        return false;
    }
    for (const RouteRect& obstacle : kShowcaseSolidObstacles)
    {
        if (detail::InsideInsetRect(obstacle, x, z))
        {
            return false;
        }
    }
    return true;
}

inline bool IsSkeletonEnemyWalkableSweep(RoutePosition previous, RoutePosition proposed)
{
    const float dx = proposed.x - previous.x;
    const float dz = proposed.z - previous.z;
    const float distance = std::sqrt(dx * dx + dz * dz);
    const int steps = std::max(1, static_cast<int>(std::ceil(distance / (kPlayerCollisionRadius * 0.5f))));
    for (int step = 1; step <= steps; ++step)
    {
        const float t = static_cast<float>(step) / static_cast<float>(steps);
        if (!IsSkeletonEnemyPositionWalkable(previous.x + dx * t, previous.z + dz * t))
        {
            return false;
        }
    }
    return true;
}

inline void ResolveSkeletonEnemyCollision(float previousX, float previousZ, float& proposedX, float& proposedZ)
{
    const RoutePosition previous{previousX, previousZ};
    const RoutePosition proposed{proposedX, proposedZ};
    if (IsSkeletonEnemyWalkableSweep(previous, proposed))
    {
        return;
    }

    const RoutePosition xOnly{proposedX, previousZ};
    if (IsSkeletonEnemyWalkableSweep(previous, xOnly))
    {
        proposedZ = previousZ;
        return;
    }

    const RoutePosition zOnly{previousX, proposedZ};
    if (IsSkeletonEnemyWalkableSweep(previous, zOnly))
    {
        proposedX = previousX;
        return;
    }

    proposedX = previousX;
    proposedZ = previousZ;
}

} // namespace horde::gameplay
