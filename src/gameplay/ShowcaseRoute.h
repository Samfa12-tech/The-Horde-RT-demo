#pragma once

#include <array>
#include <cstddef>

namespace horde::gameplay
{

struct RoutePosition
{
    float x;
    float z;
};

struct RouteRect
{
    float minX;
    float maxX;
    float minZ;
    float maxZ;
};

enum class ShowcaseZone
{
    Outside,
    Opening,
    SkeletonRoom,
    ShadowCorridor,
    SkylightChamber,
    YellowTorchBay,
    BlueTorchBay,
    RedTorchBay,
    GreenTorchBay,
    TransmissionThreshold,
    Finale,
};

inline constexpr float kPlayerCollisionRadius = 0.24f;
inline constexpr RoutePosition kPlayerSpawn{0.0f, 1.85f};
inline constexpr RoutePosition kSkeletonRoomCenter{0.0f, -4.8f};
inline constexpr RoutePosition kSkylightChamberCenter{-5.5f, -15.2f};
inline constexpr std::array<RoutePosition, 4> kTorchBayCenters{{
    {-11.0f, -15.2f},
    {-16.0f, -15.2f},
    {-21.0f, -15.2f},
    {-26.0f, -15.2f},
}};
inline constexpr RoutePosition kTransmissionThresholdCenter{-29.5f, -15.2f};
inline constexpr RoutePosition kFinaleCenter{-33.7f, -15.2f};

// Geometry-space bounds. Collision applies the player radius to their union,
// including radius-safe portals where two rectangles meet edge-to-edge.
inline constexpr std::array<RouteRect, 9> kShowcaseWalkableRects{{
    {-1.85f, 1.85f, -6.4f, 3.4f},       // Existing opening and skeleton chamber.
    {-1.2f, 1.2f, -10.0f, -6.4f},       // Shadow corridor: south.
    {0.0f, 4.8f, -11.2f, -8.8f},        // Shadow corridor: east.
    {3.6f, 6.0f, -15.2f, -10.0f},       // Shadow corridor: south.
    {-2.5f, 4.8f, -16.4f, -14.0f},      // Shadow corridor: west.
    {-8.5f, -2.5f, -18.0f, -12.4f},     // Skylight chamber.
    {-28.5f, -8.5f, -16.8f, -13.6f},    // Four five-metre torch bays.
    {-30.5f, -28.5f, -16.8f, -13.6f},   // Transmission threshold.
    {-36.9f, -30.5f, -18.4f, -12.0f},   // Finale room.
}};

// The gallery and arch posts are retained from the original chamber. The two
// far-wall returns enforce the framed 1.8 m doorway at z=-6.4.
inline constexpr std::array<RouteRect, 5> kShowcaseSolidObstacles{{
    {-10.0f, -0.72f, 0.05f, 2.35f},
    {-1.20f, -0.78f, -3.55f, -3.25f},
    {0.78f, 1.20f, -3.55f, -3.25f},
    {-1.85f, -0.90f, -6.50f, -6.30f},
    {0.90f, 1.85f, -6.50f, -6.30f},
}};

constexpr bool Contains(const RouteRect& rect, float x, float z)
{
    return x >= rect.minX && x <= rect.maxX && z >= rect.minZ && z <= rect.maxZ;
}

constexpr ShowcaseZone QueryShowcaseZone(float x, float z)
{
    if (Contains({-1.85f, 1.85f, -3.25f, 3.4f}, x, z))
    {
        return ShowcaseZone::Opening;
    }
    if (Contains({-1.85f, 1.85f, -6.4f, -3.25f}, x, z))
    {
        return ShowcaseZone::SkeletonRoom;
    }
    if (Contains(kShowcaseWalkableRects[1], x, z) ||
        Contains(kShowcaseWalkableRects[2], x, z) ||
        Contains(kShowcaseWalkableRects[3], x, z) ||
        Contains(kShowcaseWalkableRects[4], x, z))
    {
        return ShowcaseZone::ShadowCorridor;
    }
    if (Contains(kShowcaseWalkableRects[5], x, z))
    {
        return ShowcaseZone::SkylightChamber;
    }
    if (Contains({-13.5f, -8.5f, -16.8f, -13.6f}, x, z))
    {
        return ShowcaseZone::YellowTorchBay;
    }
    if (Contains({-18.5f, -13.5f, -16.8f, -13.6f}, x, z))
    {
        return ShowcaseZone::BlueTorchBay;
    }
    if (Contains({-23.5f, -18.5f, -16.8f, -13.6f}, x, z))
    {
        return ShowcaseZone::RedTorchBay;
    }
    if (Contains({-28.5f, -23.5f, -16.8f, -13.6f}, x, z))
    {
        return ShowcaseZone::GreenTorchBay;
    }
    if (Contains(kShowcaseWalkableRects[7], x, z))
    {
        return ShowcaseZone::TransmissionThreshold;
    }
    if (Contains(kShowcaseWalkableRects[8], x, z))
    {
        return ShowcaseZone::Finale;
    }
    return ShowcaseZone::Outside;
}

constexpr const char* ShowcaseZoneName(ShowcaseZone zone)
{
    switch (zone)
    {
    case ShowcaseZone::Opening: return "opening";
    case ShowcaseZone::SkeletonRoom: return "skeleton-room";
    case ShowcaseZone::ShadowCorridor: return "shadow-corridor";
    case ShowcaseZone::SkylightChamber: return "skylight-chamber";
    case ShowcaseZone::YellowTorchBay: return "yellow-torch-bay";
    case ShowcaseZone::BlueTorchBay: return "blue-torch-bay";
    case ShowcaseZone::RedTorchBay: return "red-torch-bay";
    case ShowcaseZone::GreenTorchBay: return "green-torch-bay";
    case ShowcaseZone::TransmissionThreshold: return "transmission-threshold";
    case ShowcaseZone::Finale: return "finale";
    default: return "outside";
    }
}

} // namespace horde::gameplay
