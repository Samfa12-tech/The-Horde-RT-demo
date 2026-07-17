#pragma once

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
    std::uint32_t internalWidth = 0;
    std::uint32_t internalHeight = 0;
    std::uint32_t presentationWidth = 0;
    std::uint32_t presentationHeight = 0;
    std::uint32_t blasCount = 0;
    std::uint32_t tlasCount = 0;
    std::uint32_t tlasInstanceCount = 0;
    std::uint32_t activeSkinnedEnemies = 0;
    int enemyHealth = -1;
    float renderScale = 1.0f;
    float fps = 0.0f;
    float frameTimeMs = 0.0f;
    bool presented = false;
};

std::string BuildDeveloperOverlayText(const DeveloperOverlaySnapshot& snapshot);

} // namespace horde::ui
