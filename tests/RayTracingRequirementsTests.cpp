#include "vulkan/raytracing/RayTracingRequirements.h"

#include <iostream>
#include <string_view>

namespace
{

using horde::vulkan::ExtensionSupport;
using horde::vulkan::FeatureSupport;
using horde::vulkan::RtMode;
using horde::vulkan::raytracing::EvaluateRtMode;

int failures = 0;

void ExpectMode(const std::string_view label,
                const ExtensionSupport& extensions,
                const FeatureSupport& features,
                const RtMode expected)
{
    const RtMode actual = EvaluateRtMode(extensions, features);
    if (actual != expected)
    {
        std::cerr << "FAIL: " << label << " expected mode "
                  << static_cast<int>(expected) << " but received "
                  << static_cast<int>(actual) << '\n';
        ++failures;
    }
}

} // namespace

int main()
{
    const ExtensionSupport fullPipelineExtensions{
        true, true, true, true, true};
    const FeatureSupport fullPipelineFeatures{
        true, true, true, true};
    ExpectMode("full pipeline is preferred", fullPipelineExtensions,
               fullPipelineFeatures, RtMode::RayTracingPipeline);

    const ExtensionSupport rayQueryExtensions{
        true, false, true, true, true};
    const FeatureSupport rayQueryFeatures{
        true, false, true, true};
    ExpectMode("complete RayQuery-only capability", rayQueryExtensions,
               rayQueryFeatures, RtMode::RayQuery);

    FeatureSupport noPipelineFeature = fullPipelineFeatures;
    noPipelineFeature.rayTracingPipeline = false;
    ExpectMode("full pipeline feature absence falls back to RayQuery",
               fullPipelineExtensions, noPipelineFeature, RtMode::RayQuery);

    ExtensionSupport missingExtension = rayQueryExtensions;
    missingExtension.accelerationStructure = false;
    ExpectMode("RayQuery rejects missing acceleration-structure extension",
               missingExtension, rayQueryFeatures, RtMode::Unsupported);

    missingExtension = rayQueryExtensions;
    missingExtension.rayQuery = false;
    ExpectMode("RayQuery rejects missing ray-query extension",
               missingExtension, rayQueryFeatures, RtMode::Unsupported);

    missingExtension = rayQueryExtensions;
    missingExtension.bufferDeviceAddress = false;
    ExpectMode("RayQuery rejects missing buffer-device-address extension",
               missingExtension, rayQueryFeatures, RtMode::Unsupported);

    missingExtension = rayQueryExtensions;
    missingExtension.deferredHostOperations = false;
    ExpectMode("RayQuery rejects missing deferred-host-operations dependency",
               missingExtension, rayQueryFeatures, RtMode::Unsupported);

    FeatureSupport missingFeature = rayQueryFeatures;
    missingFeature.accelerationStructure = false;
    ExpectMode("RayQuery rejects missing acceleration-structure feature",
               rayQueryExtensions, missingFeature, RtMode::Unsupported);

    missingFeature = rayQueryFeatures;
    missingFeature.rayQuery = false;
    ExpectMode("RayQuery rejects missing ray-query feature",
               rayQueryExtensions, missingFeature, RtMode::Unsupported);

    missingFeature = rayQueryFeatures;
    missingFeature.bufferDeviceAddress = false;
    ExpectMode("RayQuery rejects missing buffer-device-address feature",
               rayQueryExtensions, missingFeature, RtMode::Unsupported);

    if (failures != 0)
    {
        std::cerr << failures << " ray-tracing requirement check(s) failed.\n";
        return 1;
    }

    std::cout << "Ray-tracing requirement policy checks passed.\n";
    return 0;
}
