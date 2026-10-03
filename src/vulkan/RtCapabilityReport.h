#pragma once

#include <string>

#include "vulkan/DeviceCapabilities.h"

namespace horde::telemetry { struct RtLifecyclePublishedState; }

namespace horde::vulkan
{

// Supply an owner-thread copy and explicit observer availability. A non-null
// copy with observerAvailable=false must be an accepted terminal stopped state.
// nullptr produces unavailable evidence (the capability probe has no renderer).
std::string BuildCapabilityTextReport(const DeviceCapabilities& capabilities,
    const horde::telemetry::RtLifecyclePublishedState* evidence = nullptr,
    bool observerAvailable = true);
std::string BuildCapabilityJsonReport(const DeviceCapabilities& capabilities,
    const horde::telemetry::RtLifecyclePublishedState* evidence = nullptr,
    bool observerAvailable = true);

} // namespace horde::vulkan
