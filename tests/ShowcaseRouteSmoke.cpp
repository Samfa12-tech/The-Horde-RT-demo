#include <array>
#include <cmath>
#include <iostream>

#include "gameplay/CorridorCollision.h"

namespace
{

using horde::gameplay::RoutePosition;
using horde::gameplay::ShowcaseZone;

bool NearlyEqual(float a, float b)
{
    return std::abs(a - b) < 0.001f;
}

bool Move(RoutePosition& player, RoutePosition target)
{
    const RoutePosition start = player;
    const float dx = target.x - player.x;
    const float dz = target.z - player.z;
    const float distance = std::sqrt(dx * dx + dz * dz);
    const int steps = std::max(1, static_cast<int>(std::ceil(distance / 0.08f)));
    for (int step = 1; step <= steps; ++step)
    {
        const float t = static_cast<float>(step) / static_cast<float>(steps);
        float proposedX = start.x + dx * t;
        float proposedZ = start.z + dz * t;
        horde::gameplay::ResolveCorridorPlayerCollision(player.x, player.z, proposedX, proposedZ);
        player = {proposedX, proposedZ};
    }
    return NearlyEqual(player.x, target.x) && NearlyEqual(player.z, target.z);
}

} // namespace

int main()
{
    using namespace horde::gameplay;
    bool passed = true;
    const auto check = [&passed](bool condition, const char* message) {
        if (!condition)
        {
            std::cerr << "Route smoke failed: " << message << '\n';
            passed = false;
        }
    };

    check(IsShowcasePlayerPositionWalkable(kPlayerSpawn.x, kPlayerSpawn.z), "spawn must be valid");
    RoutePosition reset = kPlayerSpawn;
    check(NearlyEqual(reset.x, 0.0f) && NearlyEqual(reset.z, 1.85f), "reset must remain deterministic");

    RoutePosition player = kPlayerSpawn;
    const std::array<RoutePosition, 13> route{{
        {0.0f, -1.0f},       // Opening.
        {0.0f, -4.8f},       // Skeleton room.
        {0.0f, -9.4f},       // First south leg.
        {4.2f, -9.4f},       // East leg and first corner.
        {4.2f, -14.6f},      // Second south leg and second corner.
        {-2.0f, -14.6f},     // West leg and third corner.
        {-5.5f, -15.2f},     // Skylight chamber.
        {-11.0f, -15.2f},    // Yellow bay.
        {-16.0f, -15.2f},    // Blue bay.
        {-21.0f, -15.2f},    // Red bay.
        {-26.0f, -15.2f},    // Green bay.
        {-29.5f, -15.2f},    // Transmission threshold.
        {-33.7f, -15.2f},    // Finale.
    }};
    const std::array<ShowcaseZone, 13> expectedZones{{
        ShowcaseZone::Opening,
        ShowcaseZone::SkeletonRoom,
        ShowcaseZone::ShadowCorridor,
        ShowcaseZone::ShadowCorridor,
        ShowcaseZone::ShadowCorridor,
        ShowcaseZone::ShadowCorridor,
        ShowcaseZone::SkylightChamber,
        ShowcaseZone::YellowTorchBay,
        ShowcaseZone::BlueTorchBay,
        ShowcaseZone::RedTorchBay,
        ShowcaseZone::GreenTorchBay,
        ShowcaseZone::TransmissionThreshold,
        ShowcaseZone::Finale,
    }};
    for (std::size_t i = 0; i < route.size(); ++i)
    {
        check(Move(player, route[i]), "continuous route traversal must reach every waypoint");
        check(QueryShowcaseZone(player.x, player.z) == expectedZones[i], "waypoint zone must match route definition");
    }

    // A diagonal endpoint elsewhere in the route must not teleport through solid walls.
    player = {0.0f, -7.0f};
    float proposedX = 4.2f;
    float proposedZ = -9.4f;
    ResolveCorridorPlayerCollision(player.x, player.z, proposedX, proposedZ);
    check(NearlyEqual(proposedX, player.x) && !NearlyEqual(proposedX, 4.2f), "diagonal shortcut must not reach its endpoint");

    // Walking diagonally into a side wall should preserve the unobstructed axis.
    player = {0.0f, -8.0f};
    proposedX = -1.4f;
    proposedZ = -8.4f;
    ResolveCorridorPlayerCollision(player.x, player.z, proposedX, proposedZ);
    check(NearlyEqual(proposedX, player.x) && NearlyEqual(proposedZ, -8.4f), "wall sliding must preserve valid Z movement");

    // The old far wall is open only through the framed 1.8 m doorway.
    player = {0.8f, -6.0f};
    proposedX = 0.8f;
    proposedZ = -6.8f;
    ResolveCorridorPlayerCollision(player.x, player.z, proposedX, proposedZ);
    check(NearlyEqual(proposedX, player.x) && NearlyEqual(proposedZ, player.z), "old far wall must remain solid outside doorway");

    // Existing gallery and arch-post obstacles remain solid.
    check(!IsShowcasePlayerPositionWalkable(-1.0f, 1.0f), "gallery table collision must remain");
    check(!IsShowcasePlayerPositionWalkable(-1.0f, -3.4f), "left arch-post collision must remain");
    check(!IsShowcasePlayerPositionWalkable(1.0f, -3.4f), "right arch-post collision must remain");

    // Camera-held props keep their authored reach in open route space, but tuck
    // behind the camera collision boundary when looking directly into masonry.
    check(ComputeShowcaseHeldPropDepth(-20.0f, -15.2f, -1.0f, 0.0f) > 0.90f,
          "held props must retain reach in the open torch passage");
    check(ComputeShowcaseHeldPropDepth(0.0f, -9.70f, 0.0f, -1.0f) <= 0.38f,
          "held props must retract at the south-leg end wall");
    check(ComputeShowcaseHeldPropDepth(0.90f, -7.5f, 1.0f, 0.0f) <= 0.38f,
          "held props must retract at a corridor side wall");

    if (!passed)
    {
        return 1;
    }
    std::cout << "Route smoke passed: spawn/reset, zones, traversal, corners, sliding, walls, obstacles, and held-prop clearance.\n";
    return 0;
}
