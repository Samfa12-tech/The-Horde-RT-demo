#include "vulkan/raytracing/RayTracingRequirements.h"
#include "vulkan/raytracing/RtDeviceEnablePlan.h"

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
    using namespace horde::vulkan::raytracing;
    using horde::vulkan::RtExecutionBackend;
    horde::vulkan::DeviceCapabilities candidate{};
    candidate.identity.vulkanApiVersion = kRtShaderMinimumApiVersion;
    candidate.extensions = fullPipelineExtensions;
    candidate.features = fullPipelineFeatures;
    const auto check = [](const bool passed, const char* message) {
        if (!passed) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };
    check(SelectRtExecutionBackend(candidate, true) == RtExecutionBackend::RayTracingPipeline &&
              SelectRtExecutionBackend(candidate, true, true) == RtExecutionBackend::RayQueryCompute,
          "pipeline is preferred but exact compute validation remains hardware-backed");
    candidate.extensions = rayQueryExtensions;
    candidate.features = rayQueryFeatures;
    check(SelectRtExecutionBackend(candidate, true) == RtExecutionBackend::RayQueryCompute &&
              SelectRtExecutionBackend(candidate, false) == RtExecutionBackend::Unsupported,
          "query-only hardware requires a compute-capable presentation queue");
    for (bool FeatureSupport::* required : {&FeatureSupport::accelerationStructure,
                                            &FeatureSupport::rayQuery,
                                            &FeatureSupport::bufferDeviceAddress})
    {
        auto incomplete = candidate;
        incomplete.features.*required = false;
        check(SelectRtExecutionBackend(incomplete, true) == RtExecutionBackend::Unsupported &&
                  SelectRtExecutionBackend(incomplete, true, true) == RtExecutionBackend::Unsupported,
              "ordinary and forced compute selection must reject every missing required feature");
    }
    for (bool ExtensionSupport::* required : {&ExtensionSupport::accelerationStructure,
                                              &ExtensionSupport::rayQuery,
                                              &ExtensionSupport::deferredHostOperations})
    {
        auto incomplete = candidate;
        incomplete.extensions.*required = false;
        check(SelectRtExecutionBackend(incomplete, true) == RtExecutionBackend::Unsupported &&
                  SelectRtExecutionBackend(incomplete, true, true) == RtExecutionBackend::Unsupported,
              "ordinary and forced compute selection must reject every non-core dependency absence");
    }
    candidate.extensions.bufferDeviceAddress = false;
    check(EvaluateRtMode(candidate.extensions, candidate.features, candidate.identity.vulkanApiVersion) ==
              RtMode::RayQuery && !candidate.extensions.bufferDeviceAddress,
          "capability evaluation must accept core BDA without rewriting raw extension facts");
    check(SelectRtExecutionBackend(candidate, true) == RtExecutionBackend::RayQueryCompute,
          "core Vulkan 1.2 BDA support must not require an advertised promoted extension");
    candidate.identity.vulkanApiVersion = (1u << 22u) | (1u << 12u);
    check(SelectRtExecutionBackend(candidate, true) == RtExecutionBackend::Unsupported,
          "the Vulkan 1.2 shader target must not be selected on an older API");
    candidate.identity.vulkanApiVersion = kRtShaderMinimumApiVersion;
    candidate.extensions.deferredHostOperations = false;
    check(SelectRtExecutionBackend(candidate, true) == RtExecutionBackend::Unsupported,
          "query backend cannot bypass acceleration-structure dependencies");
    const auto queryPlan = MakeRtDeviceEnablePlan(RtExecutionBackend::RayQueryCompute);
    const auto pipelinePlan = MakeRtDeviceEnablePlan(RtExecutionBackend::RayTracingPipeline);
    const horde::vulkan::DeviceIdentity probed{"test GPU", 123u, 456u, 789u, kRtShaderMinimumApiVersion};
    auto otherDevice = probed;
    check(SameRtDeviceIdentity(probed, otherDevice), "same probed physical identity must match");
    ++otherDevice.driverVersion;
    check(!SameRtDeviceIdentity(probed, otherDevice), "a changed driver cannot reuse probed capabilities");
    otherDevice = probed;
    ++otherDevice.vulkanApiVersion;
    check(!SameRtDeviceIdentity(probed, otherDevice), "a changed API cannot reuse probed capabilities");
    otherDevice = probed;
    otherDevice.gpuName = "another GPU";
    check(!SameRtDeviceIdentity(probed, otherDevice), "a different GPU cannot inherit the original report");
    otherDevice = probed;
    ++otherDevice.vendorId;
    check(!SameRtDeviceIdentity(probed, otherDevice), "vendor identity must match the probe");
    otherDevice = probed;
    ++otherDevice.deviceId;
    check(!SameRtDeviceIdentity(probed, otherDevice), "device identity must match the probe");
    check(queryPlan && queryPlan->extensionCount == 3u &&
              queryPlan->features.accelerationStructure && queryPlan->features.rayQuery &&
              queryPlan->features.bufferDeviceAddress && !queryPlan->features.rayTracingPipeline &&
              std::string_view(queryPlan->extensions[0]) == "VK_KHR_acceleration_structure" &&
              std::string_view(queryPlan->extensions[1]) == "VK_KHR_ray_query" &&
              std::string_view(queryPlan->extensions[2]) == "VK_KHR_deferred_host_operations",
          "compute device plan must enable AS/RQ/BDA without pipeline-only extension/features");
    check(pipelinePlan && pipelinePlan->extensionCount == 4u &&
              pipelinePlan->features.rayTracingPipeline &&
              std::string_view(pipelinePlan->extensions[3]) == "VK_KHR_ray_tracing_pipeline" &&
              !MakeRtDeviceEnablePlan(RtExecutionBackend::Unsupported) &&
              !MakeRtDeviceEnablePlan(static_cast<RtExecutionBackend>(99)),
          "pipeline plan adds only its actual extra requirement and rejects invalid backends");
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
