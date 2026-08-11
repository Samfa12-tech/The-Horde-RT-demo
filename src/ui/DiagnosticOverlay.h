#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "vulkan/DeviceCapabilities.h"

namespace horde::ui
{

std::string BuildDiagnosticOverlayText(const vulkan::DeviceCapabilities& capabilities);
std::string BuildUnsupportedDeviceText(const vulkan::DeviceCapabilities& capabilities);

struct DeveloperOverlaySnapshot
{
    std::string buildIdentity;
    std::string shaderIdentity;
    std::string gpuName;
    std::string vulkanApi;
    std::string rtMode;
    std::string routeZone;
    std::string materialEncoding;
    std::string lanternPhase;
    std::string selectedEnemy;
    std::string encounterPhase;
    std::string playerLifePhase = "alive";
    std::uint32_t internalWidth = 0;
    std::uint32_t internalHeight = 0;
    std::uint32_t presentationWidth = 0;
    std::uint32_t presentationHeight = 0;
    std::uint32_t blasCount = 0;
    std::uint32_t tlasCount = 0;
    std::uint32_t tlasInstanceCount = 0;
    std::uint32_t activeSkinnedEnemies = 0;
    std::uint32_t activeEnemyEntityCount = 0;
    std::int32_t attackerEntityId = -1;
    std::uint32_t skeletonPoseBucketCount = 0;
    int enemyHealth = -1;
    int playerVitality = 3;
    int playerMaxVitality = 3;
    bool playerDamageEnabled = true;
    float renderScale = 1.0f;
    float fps = 0.0f;
    float frameTimeMs = 0.0f;
    bool gpuRtTimingValid = false;
    float gpuRtLatestMs = 0.0f;
    float gpuRtAverageMs = 0.0f;
    std::uint64_t gpuRtSampleCount = 0u;
    std::string gpuRtTimingStatus = "Not initialised";
    std::uint32_t simulationTicksThisFrame = 0;
    double fixedStepAccumulatorSeconds = 0.0;
    std::uint64_t catchUpOverrunCount = 0;
    std::size_t queuedEventCount = 0;
    std::size_t eventQueueHighWaterMark = 0;
    std::uint64_t eventQueueOverflowCount = 0;
    std::uint64_t inputPublicationSequence = 0;
    std::uint64_t consumedAttackSequence = 0;
    std::uint64_t consumedRouteResetSequence = 0;
    std::uint64_t consumedRetrySequence = 0;
    bool presented = false;
};

std::string BuildDeveloperOverlayText(const DeveloperOverlaySnapshot& snapshot);

} // namespace horde::ui
