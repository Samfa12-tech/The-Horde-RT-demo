#pragma once

#include <algorithm>

namespace horde::gameplay
{

namespace detail
{

constexpr float kPlayerCollisionRadius = 0.24f;

struct CollisionRect
{
    float minX;
    float maxX;
    float minZ;
    float maxZ;
};

constexpr CollisionRect kCorridorObstacles[] = {
    {-1.20f, -0.78f, -3.48f, -3.32f},
    {0.78f, 1.20f, -3.48f, -3.32f},
};

inline void PushPlayerOutOfRect(float& x, float& z, const CollisionRect& rect)
{
    const float minX = rect.minX - kPlayerCollisionRadius;
    const float maxX = rect.maxX + kPlayerCollisionRadius;
    const float minZ = rect.minZ - kPlayerCollisionRadius;
    const float maxZ = rect.maxZ + kPlayerCollisionRadius;
    if (x <= minX || x >= maxX || z <= minZ || z >= maxZ)
    {
        return;
    }

    const float pushLeft = x - minX;
    const float pushRight = maxX - x;
    const float pushBack = z - minZ;
    const float pushForward = maxZ - z;
    const float smallestPush = std::min({pushLeft, pushRight, pushBack, pushForward});
    if (smallestPush == pushLeft)
    {
        x = minX;
    }
    else if (smallestPush == pushRight)
    {
        x = maxX;
    }
    else if (smallestPush == pushBack)
    {
        z = minZ;
    }
    else
    {
        z = maxZ;
    }
}

} // namespace detail

inline void ResolveCorridorPlayerCollision(float& x, float& z)
{
    // The entrance remains open while the corridor walls and arch posts block movement.
    z = std::clamp(z, -4.75f, 4.90f);
    if (z <= 3.20f)
    {
        x = std::clamp(x, -1.58f, 1.58f);
        for (const detail::CollisionRect& obstacle : detail::kCorridorObstacles)
        {
            detail::PushPlayerOutOfRect(x, z, obstacle);
        }
        return;
    }

    x = std::clamp(x, -2.40f, 2.40f);
}

} // namespace horde::gameplay
