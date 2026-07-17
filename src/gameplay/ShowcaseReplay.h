#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "gameplay/CorridorCollision.h"

namespace horde::gameplay
{

struct ShowcaseReplayWaypoint
{
    float x;
    float z;
    ShowcaseZone expectedZone;
};

inline constexpr std::array<ShowcaseReplayWaypoint, 13> kShowcaseReplayPath{{
    {0.0f, -1.0f, ShowcaseZone::Opening},
    {0.0f, -4.8f, ShowcaseZone::SkeletonRoom},
    {0.0f, -9.4f, ShowcaseZone::ShadowCorridor},
    {4.2f, -9.4f, ShowcaseZone::ShadowCorridor},
    {4.2f, -14.6f, ShowcaseZone::ShadowCorridor},
    {-2.0f, -14.6f, ShowcaseZone::ShadowCorridor},
    {-5.5f, -15.2f, ShowcaseZone::SkylightChamber},
    {-11.0f, -15.2f, ShowcaseZone::YellowTorchBay},
    {-16.0f, -15.2f, ShowcaseZone::BlueTorchBay},
    {-21.0f, -15.2f, ShowcaseZone::RedTorchBay},
    {-26.0f, -15.2f, ShowcaseZone::GreenTorchBay},
    {-29.5f, -15.2f, ShowcaseZone::TransmissionThreshold},
    {-33.7f, -15.2f, ShowcaseZone::Finale},
}};

struct ShowcaseReplaySnapshot
{
    float x = kPlayerSpawn.x;
    float z = kPlayerSpawn.z;
    float yaw = 0.0f;
    std::size_t nextWaypoint = 0;
    std::size_t reachedWaypoints = 0;
    ShowcaseZone zone = ShowcaseZone::Opening;
    bool waypointReached = false;
    bool complete = false;
    bool failed = false;
};

class ShowcaseRouteReplay
{
public:
    const ShowcaseReplaySnapshot& Reset()
    {
        snapshot_ = {};
        stalledFrames_ = 0;
        return snapshot_;
    }

    const ShowcaseReplaySnapshot& Update(float distancePerFrame = 0.032f)
    {
        snapshot_.waypointReached = false;
        if (snapshot_.complete || snapshot_.failed)
        {
            return snapshot_;
        }

        const ShowcaseReplayWaypoint& target = kShowcaseReplayPath[snapshot_.nextWaypoint];
        const float dx = target.x - snapshot_.x;
        const float dz = target.z - snapshot_.z;
        const float distance = std::hypot(dx, dz);
        const float step = std::min(std::max(distancePerFrame, 0.001f), distance);
        const float inverseDistance = distance > 0.00001f ? 1.0f / distance : 0.0f;
        float proposedX = snapshot_.x + dx * inverseDistance * step;
        float proposedZ = snapshot_.z + dz * inverseDistance * step;
        snapshot_.yaw = std::atan2(dx, -dz);
        ResolveCorridorPlayerCollision(snapshot_.x, snapshot_.z, proposedX, proposedZ);
        const float travelled = std::hypot(proposedX - snapshot_.x, proposedZ - snapshot_.z);
        snapshot_.x = proposedX;
        snapshot_.z = proposedZ;
        snapshot_.zone = QueryShowcaseZone(snapshot_.x, snapshot_.z);

        if (travelled < 0.0001f && distance > 0.01f)
        {
            ++stalledFrames_;
            snapshot_.failed = stalledFrames_ > 30;
            return snapshot_;
        }
        stalledFrames_ = 0;

        if (std::hypot(target.x - snapshot_.x, target.z - snapshot_.z) <= 0.002f)
        {
            snapshot_.x = target.x;
            snapshot_.z = target.z;
            snapshot_.zone = QueryShowcaseZone(snapshot_.x, snapshot_.z);
            snapshot_.failed = snapshot_.zone != target.expectedZone;
            snapshot_.waypointReached = !snapshot_.failed;
            if (!snapshot_.failed)
            {
                ++snapshot_.reachedWaypoints;
                ++snapshot_.nextWaypoint;
                snapshot_.complete = snapshot_.nextWaypoint >= kShowcaseReplayPath.size();
            }
        }
        return snapshot_;
    }

    const ShowcaseReplaySnapshot& Snapshot() const { return snapshot_; }

private:
    ShowcaseReplaySnapshot snapshot_{};
    int stalledFrames_ = 0;
};

} // namespace horde::gameplay
