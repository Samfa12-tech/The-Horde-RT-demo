#include "gameplay/effects/KeeperCastPresentation.h"
#include "scene/EntryPortalCapGeometry.h"
#include "scene/ForestDressing.h"
#include "scene/assets/StaticPrimitiveGrouping.h"
#include "vulkan/raytracing/RtWorldMaterialPalette.h"
#include "scene/TombDressing.h"
#include "scene/TombWallRecess.h"
#include "vulkan/raytracing/RescuePlayerRig.h"
#include "scene/RescueJourneyGeometry.h"
#include "scene/ShowcaseIndoorDust.h"
#include "vulkan/raytracing/PresentableTinyRtScene.h"
#include "vulkan/raytracing/SimulationFrameAdapter.h"
#include "graphics/EntryMenuScene.h"
#include "graphics/GraphicsPreviewSession.h"
#include "vulkan/raytracing/ChestGuidanceLight.h"
#include "vulkan/raytracing/DynamicBlasSynchronization.h"
#include "vulkan/raytracing/RtDescriptorSetLayoutBindings.h"
#include "vulkan/raytracing/RtDeviceAddressLayout.h"
#include "vulkan/raytracing/RtTextureLayerSubset.h"

#include "gameplay/items/HeldItemKinematics.h"
#include "gameplay/items/HeldLightState.h"
#include "gameplay/effects/KeeperTorchLighting.h"
#include "vulkan/raytracing/HeldItemRenderSlot.h"
#include "vulkan/raytracing/RtFrameEvidenceCoordinator.h"
#include "vulkan/raytracing/RtSceneRecordObservation.h"
#include "vulkan/raytracing/RtSceneRouteConstants.h"
#include "scene/ShowcaseOverheadGeometry.h"
#include "scene/ShaftDressingGeometry.h"
#include "vulkan/raytracing/RtLanternGeometryProfile.h"
#include "vulkan/raytracing/TlasInstanceRefresh.h"
#include "vulkan/raytracing/WaterContactRenderGeometry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <limits>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "gameplay/CorridorCollision.h"
#include "scene/assets/AssetManifest.h"

namespace horde::vulkan::raytracing
{
namespace {
struct alignas(16) QualityDustUpload {
    RtQualityControlsGpu quality{};
    horde::scene::atmosphere::DustFrame dust{};
};
static_assert(offsetof(QualityDustUpload,dust)==16 && sizeof(QualityDustUpload)==11280);
static_assert(sizeof(horde::scene::atmosphere::DustMote)==sizeof(RtDustMoteGpu));
static_assert(offsetof(horde::scene::atmosphere::DustMote,response)==offsetof(RtDustMoteGpu,response));
}


using horde::gameplay::kRouteFloorWorldY;
using horde::gameplay::kShowcaseEyeWorldY;

namespace
{

using InitialiseClock = std::chrono::steady_clock;

template <typename Action>
bool MeasureInitialisationStage(
    PresentableTinyRtScene::InitialisationMeasurements& measurements,
    const std::string_view name,
    Action&& action)
{
    measurements.stages.push_back({name, 0u, true, false});
    auto& stage = measurements.stages.back();
    const auto start = InitialiseClock::now();
    const bool succeeded = std::forward<Action>(action)();
    stage.cpuNanoseconds = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            InitialiseClock::now() - start).count());
    stage.succeeded = succeeded;
    return succeeded;
}

class InitialiseAttemptTimer
{
public:
    explicit InitialiseAttemptTimer(
        PresentableTinyRtScene::InitialisationMeasurements& measurements) noexcept
        : measurements_(measurements), start_(InitialiseClock::now()) {}

    ~InitialiseAttemptTimer()
    {
        measurements_.totalCpuNanoseconds = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                InitialiseClock::now() - start_).count());
    }

private:
    PresentableTinyRtScene::InitialisationMeasurements& measurements_;
    InitialiseClock::time_point start_;
};

class CpuElapsedTimer
{
public:
    explicit CpuElapsedTimer(std::uint64_t& elapsedNanoseconds) noexcept
        : elapsedNanoseconds_(elapsedNanoseconds), start_(InitialiseClock::now()) {}

    ~CpuElapsedTimer()
    {
        elapsedNanoseconds_ = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                InitialiseClock::now() - start_).count());
    }

private:
    std::uint64_t& elapsedNanoseconds_;
    InitialiseClock::time_point start_;
};

constexpr VkFormat kStorageImageFormat = VK_FORMAT_R8G8B8A8_UNORM;

bool ValidEvidenceHash(const std::string_view hash) noexcept
{
    return hash.size() == 64u &&
           std::all_of(hash.begin(), hash.end(), [](const char value) {
               return (value >= '0' && value <= '9') ||
                      (value >= 'a' && value <= 'f');
           });
}

bool CheckedMetresToMicrometres(const float metres,
                                std::uint64_t& micrometres) noexcept
{
    if (!std::isfinite(metres) || metres < 0.0f)
    {
        return false;
    }
    const long double converted = static_cast<long double>(metres) * 1'000'000.0L;
    const long double upper = std::ldexp(
        1.0L, std::numeric_limits<std::uint64_t>::digits);
    if (converted >= upper)
    {
        return false;
    }
    micrometres = static_cast<std::uint64_t>(std::round(converted));
    return true;
}

std::string_view EvidenceBundleKey(
    const RtPipelineBundleRequest& request) noexcept
{
    if (request.executionBackend == RtExecutionBackend::RayQueryCompute)
    {
        if (request.instrumentation == RtInstrumentation::Shipping)
        {
            return request.quality == DielectricQuality::Mobile
                ? "rayquery_compute_shipping_mobile_pair" : "rayquery_compute_shipping_high_pair";
        }
        return request.quality == DielectricQuality::Mobile
            ? "rayquery_compute_diagnostic_mobile_pair" : "rayquery_compute_diagnostic_high_pair";
    }
    if (request.instrumentation == RtInstrumentation::Shipping)
    {
        return request.quality == DielectricQuality::Mobile
            ? "shipping_mobile_pair" : "shipping_high_pair";
    }
    return request.quality == DielectricQuality::Mobile
        ? "diagnostic_mobile_pair" : "diagnostic_high_pair";
}

struct ScenePushConstants
{
    float yaw = 0.0f;
    float pitch = 0.0f;
    float torchLight = 1.0f;
    float time = 0.0f;
    float cameraX = 0.0f;
    float cameraZ = 1.85f;
    float walkAmount = 0.0f;
    // Integer-valued output mode: bit 0 swaps R/B, bits 1-3 select the
    // primary-ray presentation transform. Identity preserves released 0/1.
    float outputRedBlueSwap = 0.0f;
    float outputExposure = 0.92f;
    float damageFlash = 0.0f;
    float enemyKind = 0.0f;
    float staffLightStrength = 0.0f;
    float staffX = -32.2f;
    float staffY = 0.55f;
    float staffZ = -13.1f;
    float finaleSkylightOpen = 0.0f;
    float finaleDawnReveal = 0.0f;
    float heldPropDepth = 1.05f;
    float waterQuality = 2.0f;
    float waterfallWidthScale = 1.0f;
    float fogDensityScale = 1.0f;
    float torchHueDegrees = 0.0f;
    float torchIntensityScale = 1.0f;
    float skylightHueDegrees = 0.0f;
    float skylightIntensityScale = 1.0f;
    float passageHueDegrees = 0.0f;
    float passageIntensityScale = 1.0f;
    float staffHueDegrees = 0.0f;
    float staffIntensityScale = 1.0f;
    float workloadPreset = 1.0f;
    float genericTransmissionActive = 0.0f;
    float guidanceLightStrength = 0.0f;
};

static_assert(sizeof(ScenePushConstants) == 128u,
              "CPU and raygen push-constant ABI must remain 32 packed floats");
static_assert(sizeof(ScenePushConstants) <= 128u,
              "push constants must fit Vulkan's required minimum device limit");
static_assert(offsetof(ScenePushConstants, waterQuality) == 72u,
              "water quality must stay appended after every released push field");
static_assert(offsetof(ScenePushConstants, waterfallWidthScale) == 76u,
              "RT lab tuning must append after the released water-quality field");
static_assert(offsetof(ScenePushConstants, genericTransmissionActive) == 120u,
              "generic transmission activity must remain append-only");
static_assert(offsetof(ScenePushConstants, guidanceLightStrength) == 124u,
              "chest guidance light strength must remain append-only");

bool HasActiveGenericTransmission(
    std::span<const RtInstanceMetadata> instances,
    std::span<const RtPrimitiveMetadata> primitives,
    std::span<const RtMaterialGpu> materials)
{
    constexpr std::uint32_t kTransmissiveInstance =
        static_cast<std::uint32_t>(RtInstanceFlag::Transmissive);
    constexpr std::uint32_t kTransmissionMaterial =
        static_cast<std::uint32_t>(RtMaterialFlag::Transmission);
    for (const RtInstanceMetadata& instance : instances)
    {
        if ((instance.flags & kTransmissiveInstance) == 0u)
            continue;
        const std::size_t primitiveBegin = instance.primitiveBase;
        const std::size_t primitiveEnd = primitiveBegin + instance.primitiveCount;
        if (primitiveEnd > primitives.size())
            continue;
        for (std::size_t primitiveIndex = primitiveBegin;
             primitiveIndex < primitiveEnd; ++primitiveIndex)
        {
            const std::uint32_t materialIndex = primitives[primitiveIndex].materialIndex;
            if (materialIndex >= materials.size())
                continue;
            const RtMaterialGpu& material = materials[materialIndex];
            if ((material.materialFlags[0] & kTransmissionMaterial) != 0u &&
                material.metallicRoughnessOcclusionTransmission[3] > 0.001f &&
                material.metallicRoughnessOcclusionTransmission[0] <= 0.5f)
                return true;
        }
    }
    return false;
}

constexpr std::uint32_t kMinimalMissShader[] = {
    0x07230203u, 0x00010500u, 0x0008000bu, 0x00000069u, 0x00000000u, 0x00020011u, 0x0000117fu, 0x0006000au,
    0x5f565053u, 0x5f52484bu, 0x5f796172u, 0x63617274u, 0x00676e69u, 0x0006000bu, 0x00000001u, 0x4c534c47u,
    0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu, 0x00000000u, 0x00000001u, 0x0007000fu, 0x000014c5u,
    0x00000004u, 0x6e69616du, 0x00000000u, 0x0000000bu, 0x00000045u, 0x00030003u, 0x00000002u, 0x000001ccu,
    0x00060004u, 0x455f4c47u, 0x725f5458u, 0x745f7961u, 0x69636172u, 0x0000676eu, 0x00040005u, 0x00000004u,
    0x6e69616du, 0x00000000u, 0x00030005u, 0x00000009u, 0x00000064u, 0x00080005u, 0x0000000bu, 0x575f6c67u,
    0x646c726fu, 0x44796152u, 0x63657269u, 0x6e6f6974u, 0x00545845u, 0x00040005u, 0x0000000fu, 0x6e6f6f6du,
    0x00000000u, 0x00040005u, 0x0000001au, 0x69726f68u, 0x006e6f7au, 0x00030005u, 0x00000022u, 0x00796b73u,
    0x00040005u, 0x00000045u, 0x6c796170u, 0x0064616fu, 0x00040047u, 0x0000000bu, 0x0000000bu, 0x000014cau,
    0x00020013u, 0x00000002u, 0x00030021u, 0x00000003u, 0x00000002u, 0x00030016u, 0x00000006u, 0x00000020u,
    0x00040017u, 0x00000007u, 0x00000006u, 0x00000003u, 0x00040020u, 0x00000008u, 0x00000007u, 0x00000007u,
    0x00040020u, 0x0000000au, 0x00000001u, 0x00000007u, 0x0004003bu, 0x0000000au, 0x0000000bu, 0x00000001u,
    0x00040020u, 0x0000000eu, 0x00000007u, 0x00000006u, 0x0004002bu, 0x00000006u, 0x00000011u, 0xbe8f4d7du,
    0x0004002bu, 0x00000006u, 0x00000012u, 0x3effe5cdu, 0x0004002bu, 0x00000006u, 0x00000013u, 0xbf51d609u,
    0x0006002cu, 0x00000007u, 0x00000014u, 0x00000011u, 0x00000012u, 0x00000013u, 0x0004002bu, 0x00000006u,
    0x00000016u, 0x00000000u, 0x0004002bu, 0x00000006u, 0x00000018u, 0x42800000u, 0x0004002bu, 0x00000006u,
    0x0000001bu, 0xbe6147aeu, 0x0004002bu, 0x00000006u, 0x0000001cu, 0x3ee66666u, 0x00040015u, 0x0000001du,
    0x00000020u, 0x00000000u, 0x0004002bu, 0x0000001du, 0x0000001eu, 0x00000001u, 0x0004002bu, 0x00000006u,
    0x00000023u, 0x3c75c28fu, 0x0004002bu, 0x00000006u, 0x00000024u, 0x3c9374bcu, 0x0004002bu, 0x00000006u,
    0x00000025u, 0x3cc49ba6u, 0x0006002cu, 0x00000007u, 0x00000026u, 0x00000023u, 0x00000024u, 0x00000025u,
    0x0004002bu, 0x00000006u, 0x00000027u, 0x3d23d70au, 0x0004002bu, 0x00000006u, 0x00000028u, 0x3d851eb8u,
    0x0004002bu, 0x00000006u, 0x00000029u, 0x3dd70a3du, 0x0006002cu, 0x00000007u, 0x0000002au, 0x00000027u,
    0x00000028u, 0x00000029u, 0x0004002bu, 0x00000006u, 0x0000002eu, 0x3e800000u, 0x0004002bu, 0x00000006u,
    0x0000002fu, 0x3ea8f5c3u, 0x0004002bu, 0x00000006u, 0x00000030u, 0x3ef5c28fu, 0x0006002cu, 0x00000007u,
    0x00000031u, 0x0000002eu, 0x0000002fu, 0x00000030u, 0x0004002bu, 0x00000006u, 0x00000036u, 0x3db851ecu,
    0x0004002bu, 0x00000006u, 0x00000037u, 0x3d0f5c29u, 0x0004002bu, 0x00000006u, 0x00000038u, 0x3c449ba6u,
    0x0006002cu, 0x00000007u, 0x00000039u, 0x00000036u, 0x00000037u, 0x00000038u, 0x0004002bu, 0x00000006u,
    0x0000003au, 0xbeb33333u, 0x0004002bu, 0x00000006u, 0x0000003bu, 0x3e19999au, 0x00040017u, 0x00000043u,
    0x00000006u, 0x00000004u, 0x00040020u, 0x00000044u, 0x000014deu, 0x00000043u, 0x0004003bu, 0x00000044u,
    0x00000045u, 0x000014deu, 0x0004002bu, 0x0000001du, 0x00000046u, 0x00000003u, 0x00040020u, 0x00000047u,
    0x000014deu, 0x00000006u, 0x0004002bu, 0x00000006u, 0x0000004au, 0xbfc00000u, 0x00020014u, 0x0000004bu,
    0x0004002bu, 0x00000006u, 0x00000050u, 0x3f400000u, 0x0004002bu, 0x00000006u, 0x00000052u, 0x3ca3d70au,
    0x0004002bu, 0x00000006u, 0x00000053u, 0x3cf5c28fu, 0x0006002cu, 0x00000007u, 0x00000054u, 0x00000052u,
    0x00000025u, 0x00000053u, 0x0004002bu, 0x00000006u, 0x00000056u, 0x3f800000u, 0x0004002bu, 0x00000006u,
    0x0000005eu, 0xbf000000u, 0x0007002cu, 0x00000043u, 0x00000062u, 0x00000056u, 0x00000056u, 0x00000056u,
    0x00000016u, 0x00050036u, 0x00000002u, 0x00000004u, 0x00000000u, 0x00000003u, 0x000200f8u, 0x00000005u,
    0x0004003bu, 0x00000008u, 0x00000009u, 0x00000007u, 0x0004003bu, 0x0000000eu, 0x0000000fu, 0x00000007u,
    0x0004003bu, 0x0000000eu, 0x0000001au, 0x00000007u, 0x0004003bu, 0x00000008u, 0x00000022u, 0x00000007u,
    0x0004003du, 0x00000007u, 0x0000000cu, 0x0000000bu, 0x0006000cu, 0x00000007u, 0x0000000du, 0x00000001u,
    0x00000045u, 0x0000000cu, 0x0003003eu, 0x00000009u, 0x0000000du, 0x0004003du, 0x00000007u, 0x00000010u,
    0x00000009u, 0x00050094u, 0x00000006u, 0x00000015u, 0x00000010u, 0x00000014u, 0x0007000cu, 0x00000006u,
    0x00000017u, 0x00000001u, 0x00000028u, 0x00000015u, 0x00000016u, 0x0007000cu, 0x00000006u, 0x00000019u,
    0x00000001u, 0x0000001au, 0x00000017u, 0x00000018u, 0x0003003eu, 0x0000000fu, 0x00000019u, 0x00050041u,
    0x0000000eu, 0x0000001fu, 0x00000009u, 0x0000001eu, 0x0004003du, 0x00000006u, 0x00000020u, 0x0000001fu,
    0x0008000cu, 0x00000006u, 0x00000021u, 0x00000001u, 0x00000031u, 0x0000001bu, 0x0000001cu, 0x00000020u,
    0x0003003eu, 0x0000001au, 0x00000021u, 0x0004003du, 0x00000006u, 0x0000002bu, 0x0000001au, 0x00060050u,
    0x00000007u, 0x0000002cu, 0x0000002bu, 0x0000002bu, 0x0000002bu, 0x0008000cu, 0x00000007u, 0x0000002du,
    0x00000001u, 0x0000002eu, 0x00000026u, 0x0000002au, 0x0000002cu, 0x0003003eu, 0x00000022u, 0x0000002du,
    0x0004003du, 0x00000006u, 0x00000032u, 0x0000000fu, 0x0005008eu, 0x00000007u, 0x00000033u, 0x00000031u,
    0x00000032u, 0x0004003du, 0x00000007u, 0x00000034u, 0x00000022u, 0x00050081u, 0x00000007u, 0x00000035u,
    0x00000034u, 0x00000033u, 0x0003003eu, 0x00000022u, 0x00000035u, 0x00050041u, 0x0000000eu, 0x0000003cu,
    0x00000009u, 0x0000001eu, 0x0004003du, 0x00000006u, 0x0000003du, 0x0000003cu, 0x0004007fu, 0x00000006u,
    0x0000003eu, 0x0000003du, 0x0008000cu, 0x00000006u, 0x0000003fu, 0x00000001u, 0x00000031u, 0x0000003au,
    0x0000003bu, 0x0000003eu, 0x0005008eu, 0x00000007u, 0x00000040u, 0x00000039u, 0x0000003fu, 0x0004003du,
    0x00000007u, 0x00000041u, 0x00000022u, 0x00050081u, 0x00000007u, 0x00000042u, 0x00000041u, 0x00000040u,
    0x0003003eu, 0x00000022u, 0x00000042u, 0x00050041u, 0x00000047u, 0x00000048u, 0x00000045u, 0x00000046u,
    0x0004003du, 0x00000006u, 0x00000049u, 0x00000048u, 0x000500b8u, 0x0000004bu, 0x0000004cu, 0x00000049u,
    0x0000004au, 0x000300f7u, 0x0000004eu, 0x00000000u, 0x000400fau, 0x0000004cu, 0x0000004du, 0x0000004eu,
    0x000200f8u, 0x0000004du, 0x0004003du, 0x00000007u, 0x0000004fu, 0x00000022u, 0x0005008eu, 0x00000007u,
    0x00000051u, 0x0000004fu, 0x00000050u, 0x00050081u, 0x00000007u, 0x00000055u, 0x00000051u, 0x00000054u,
    0x00050051u, 0x00000006u, 0x00000057u, 0x00000055u, 0x00000000u, 0x00050051u, 0x00000006u, 0x00000058u,
    0x00000055u, 0x00000001u, 0x00050051u, 0x00000006u, 0x00000059u, 0x00000055u, 0x00000002u, 0x00070050u,
    0x00000043u, 0x0000005au, 0x00000057u, 0x00000058u, 0x00000059u, 0x00000056u, 0x0003003eu, 0x00000045u,
    0x0000005au, 0x000100fdu, 0x000200f8u, 0x0000004eu, 0x00050041u, 0x00000047u, 0x0000005cu, 0x00000045u,
    0x00000046u, 0x0004003du, 0x00000006u, 0x0000005du, 0x0000005cu, 0x000500b8u, 0x0000004bu, 0x0000005fu,
    0x0000005du, 0x0000005eu, 0x000300f7u, 0x00000061u, 0x00000000u, 0x000400fau, 0x0000005fu, 0x00000060u,
    0x00000061u, 0x000200f8u, 0x00000060u, 0x0003003eu, 0x00000045u, 0x00000062u, 0x000100fdu, 0x000200f8u,
    0x00000061u, 0x0004003du, 0x00000007u, 0x00000064u, 0x00000022u, 0x00050051u, 0x00000006u, 0x00000065u,
    0x00000064u, 0x00000000u, 0x00050051u, 0x00000006u, 0x00000066u, 0x00000064u, 0x00000001u, 0x00050051u,
    0x00000006u, 0x00000067u, 0x00000064u, 0x00000002u, 0x00070050u, 0x00000043u, 0x00000068u, 0x00000065u,
    0x00000066u, 0x00000067u, 0x00000056u, 0x0003003eu, 0x00000045u, 0x00000068u, 0x000100fdu, 0x00010038u
};

constexpr std::uint32_t kMinimalClosestHitShader[] = {
    0x07230203u, 0x00010500u, 0x0008000bu, 0x0000000fu, 0x00000000u, 0x00020011u, 0x0000117fu, 0x0006000au,
    0x5f565053u, 0x5f52484bu, 0x5f796172u, 0x63617274u, 0x00676e69u, 0x0006000bu, 0x00000001u, 0x4c534c47u,
    0x6474732eu, 0x3035342eu, 0x00000000u, 0x0003000eu, 0x00000000u, 0x00000001u, 0x0006000fu, 0x000014c4u,
    0x00000004u, 0x6e69616du, 0x00000000u, 0x00000009u, 0x00030003u, 0x00000002u, 0x000001ccu, 0x00060004u,
    0x455f4c47u, 0x725f5458u, 0x745f7961u, 0x69636172u, 0x0000676eu, 0x00040005u, 0x00000004u, 0x6e69616du,
    0x00000000u, 0x00040005u, 0x00000009u, 0x6c796170u, 0x0064616fu, 0x00020013u, 0x00000002u, 0x00030021u,
    0x00000003u, 0x00000002u, 0x00030016u, 0x00000006u, 0x00000020u, 0x00040017u, 0x00000007u, 0x00000006u,
    0x00000004u, 0x00040020u, 0x00000008u, 0x000014deu, 0x00000007u, 0x0004003bu, 0x00000008u, 0x00000009u,
    0x000014deu, 0x0004002bu, 0x00000006u, 0x0000000au, 0x3ca3d70au, 0x0004002bu, 0x00000006u, 0x0000000bu,
    0x3d75c28fu, 0x0004002bu, 0x00000006u, 0x0000000cu, 0x3df5c28fu, 0x0004002bu, 0x00000006u, 0x0000000du,
    0x3f800000u, 0x0007002cu, 0x00000007u, 0x0000000eu, 0x0000000au, 0x0000000bu, 0x0000000cu, 0x0000000du,
    0x00050036u, 0x00000002u, 0x00000004u, 0x00000000u, 0x00000003u, 0x000200f8u, 0x00000005u, 0x0003003eu,
    0x00000009u, 0x0000000eu, 0x000100fdu, 0x00010038u
};

void SetImageBarrier(VkCommandBuffer commandBuffer,
                     VkImage image,
                     VkImageLayout oldLayout,
                     VkImageLayout newLayout,
                     VkPipelineStageFlags srcStage,
                     VkPipelineStageFlags dstStage,
                     VkAccessFlags srcAccess,
                     VkAccessFlags dstAccess)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1u;
    barrier.subresourceRange.layerCount = 1u;
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;
    vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0u, 0u, nullptr, 0u, nullptr, 1u, &barrier);
}

bool CreateShaderModule(VkDevice device, const std::uint32_t* code, std::size_t byteSize, VkShaderModule& shaderModule)
{
    const VkShaderModuleCreateInfo createInfo{
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        nullptr,
        0u,
        byteSize,
        code};
    return vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) == VK_SUCCESS;
}

} // namespace

bool TryMakeRtPipelineEvidenceIdentity(
    const RtPipelineBundleRequest& request,
    const RtPipelineVariantArtifact& opaqueFast,
    const RtPipelineVariantArtifact& genericDielectric,
    horde::telemetry::RtPipelineEvidenceIdentity& identity) noexcept
{
    identity = {};
    if ((request.instrumentation != RtInstrumentation::Shipping &&
         request.instrumentation != RtInstrumentation::Diagnostic) ||
        (request.quality != DielectricQuality::Mobile &&
         request.quality != DielectricQuality::High) ||
        (request.executionBackend != RtExecutionBackend::RayTracingPipeline &&
         request.executionBackend != RtExecutionBackend::RayQueryCompute) ||
        opaqueFast.key.executionBackend != request.executionBackend ||
        genericDielectric.key.executionBackend != request.executionBackend ||
        opaqueFast.key.instrumentation != request.instrumentation ||
        genericDielectric.key.instrumentation != request.instrumentation ||
        opaqueFast.key.quality != request.quality ||
        genericDielectric.key.quality != request.quality ||
        opaqueFast.key.material != RtMaterialStrategy::OpaqueFast ||
        genericDielectric.key.material != RtMaterialStrategy::GenericDielectric ||
        opaqueFast.canonicalKey.empty() || genericDielectric.canonicalKey.empty() ||
        !ValidEvidenceHash(opaqueFast.spirvSha256) ||
        !ValidEvidenceHash(genericDielectric.spirvSha256))
    {
        return false;
    }

    horde::telemetry::RtPipelineEvidenceIdentity candidate{};
    candidate.executionMode = request.executionBackend == RtExecutionBackend::RayQueryCompute
        ? horde::telemetry::RtExecutionMode::RayQueryCompute
        : horde::telemetry::RtExecutionMode::RayTracingPipeline;
    candidate.instrumentation = request.instrumentation == RtInstrumentation::Shipping
        ? horde::telemetry::RtInstrumentationMode::Shipping
        : horde::telemetry::RtInstrumentationMode::Diagnostic;
    candidate.dielectricQuality = request.quality == DielectricQuality::Mobile
        ? horde::telemetry::RtDielectricQuality::Mobile
        : horde::telemetry::RtDielectricQuality::High;
    candidate.activeStrategy = horde::telemetry::RtMaterialStrategy::OpaqueFast;
    candidate.waterQuality = horde::telemetry::RtWaterQuality::Off;
    if (!horde::telemetry::AssignRtFixedText(
            candidate.bundleKey, EvidenceBundleKey(request)) ||
        !horde::telemetry::AssignRtFixedText(
            candidate.opaqueFast.key, opaqueFast.canonicalKey) ||
        !horde::telemetry::AssignRtFixedText(
            candidate.opaqueFast.sha256, opaqueFast.spirvSha256) ||
        !horde::telemetry::AssignRtFixedText(
            candidate.genericDielectric.key, genericDielectric.canonicalKey) ||
        !horde::telemetry::AssignRtFixedText(
            candidate.genericDielectric.sha256,
            genericDielectric.spirvSha256))
    {
        return false;
    }
    candidate.active = candidate.opaqueFast;
    identity = candidate;
    return true;
}

PlayerWeaponRenderPose EvaluatePlayerWeaponRenderPose(
    const horde::gameplay::PlayerCombatSnapshot& playerCombat,
    const float swordSwingRadians,
    const float heldPropDepth)
{
    const horde::gameplay::items::HeldSwordPose sharedPose =
        horde::gameplay::items::EvaluateHeldSwordPose(
            playerCombat, swordSwingRadians, heldPropDepth);
    return {sharedPose.rightHandLocal,
            sharedPose.swordRadians,
            sharedPose.parryBlend,
            sharedPose.successJolt};
}

PresentableTinyRtScene::~PresentableTinyRtScene()
{
    Destroy();
}

PresentableTinyRtScene::PresentableTinyRtScene(PresentableTinyRtScene&& other) noexcept
{
    *this = std::move(other);
}

PresentableTinyRtScene& PresentableTinyRtScene::operator=(PresentableTinyRtScene&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    Destroy();

    physicalDevice_ = std::exchange(other.physicalDevice_, nullptr);
    instance_ = std::exchange(other.instance_, nullptr);
    device_ = std::exchange(other.device_, nullptr);
    pipelineCache_ = std::exchange(other.pipelineCache_, VK_NULL_HANDLE);
    compiledPipelineCache_ = std::exchange(other.compiledPipelineCache_, nullptr);
    queue_ = std::exchange(other.queue_, nullptr);
    commandPool_ = std::exchange(other.commandPool_, VK_NULL_HANDLE);
    dispatchExtent_ = std::exchange(other.dispatchExtent_, VkExtent2D{});
    presentationUsesBgra_ = std::exchange(other.presentationUsesBgra_, false);
    scaledBlitSupported_ = std::exchange(other.scaledBlitSupported_, false);
    storageImage_ = std::exchange(other.storageImage_, VK_NULL_HANDLE);
    storageImageMemory_ = std::exchange(other.storageImageMemory_, VK_NULL_HANDLE);
    storageImageView_ = std::exchange(other.storageImageView_, VK_NULL_HANDLE);
    storageImageAllocationSize_ = std::exchange(other.storageImageAllocationSize_, 0u);
    storageImageMemoryPropertyFlags_ =
        std::exchange(other.storageImageMemoryPropertyFlags_, 0u);
    storageImageLayout_ = std::exchange(other.storageImageLayout_, VK_IMAGE_LAYOUT_UNDEFINED);
    storageImageFrameRecorded_ = std::exchange(other.storageImageFrameRecorded_, false);
    lastOutputRedBlueSwapApplied_ = std::exchange(other.lastOutputRedBlueSwapApplied_, false);
    materialDiffuse_ = std::exchange(other.materialDiffuse_, TextureArray{});
    materialNormal_ = std::exchange(other.materialNormal_, TextureArray{});
    materialArm_ = std::exchange(other.materialArm_, TextureArray{});
    environmentTexture_ = std::exchange(other.environmentTexture_, TextureArray{});
    environmentSampler_ = std::exchange(other.environmentSampler_, VK_NULL_HANDLE);
    lichBaseColor_ = std::exchange(other.lichBaseColor_, TextureArray{});
    lichEmissive_ = std::exchange(other.lichEmissive_, TextureArray{});
    staticBaseColor_ = std::exchange(other.staticBaseColor_, TextureArray{});
    staticNormal_ = std::exchange(other.staticNormal_, TextureArray{});
    staticOrm_ = std::exchange(other.staticOrm_, TextureArray{});
    staticEmissive_ = std::exchange(other.staticEmissive_, TextureArray{});
    materialSampler_ = std::exchange(other.materialSampler_, VK_NULL_HANDLE);
    materialEncoding_ = std::move(other.materialEncoding_);
    vertexBuffer_ = std::exchange(other.vertexBuffer_, Buffer{});
    indexBuffer_ = std::exchange(other.indexBuffer_, Buffer{});
    transformBuffer_ = std::exchange(other.transformBuffer_, Buffer{});
    instanceBuffer_ = std::exchange(other.instanceBuffer_, Buffer{});
    tlasBuiltInstances_ = std::exchange(other.tlasBuiltInstances_, {});
    tlasInstanceDefinitionsValid_ = std::exchange(other.tlasInstanceDefinitionsValid_, false);
    tlasPendingInstances_ = std::exchange(other.tlasPendingInstances_, {});
    tlasPendingDefinitionsValid_ = std::exchange(other.tlasPendingDefinitionsValid_, false);
    heldLightBuffer_ = std::exchange(other.heldLightBuffer_, Buffer{});
    fireEmitterBuffer_ = std::exchange(other.fireEmitterBuffer_, Buffer{});
    qualityControlsBuffer_ = std::exchange(other.qualityControlsBuffer_, {});
    waterContactRippleBuffer_ = std::exchange(other.waterContactRippleBuffer_, {});
    uploadedQualityControls_ = std::exchange(other.uploadedQualityControls_, {});
    uploadedFireQuality_ = std::exchange(other.uploadedFireQuality_, FireEmitterQuality::Mobile);
    uploadedFireEmitters_ = std::exchange(other.uploadedFireEmitters_, FireEmitterUpload{});
    uploadedFireEmittersValid_ = std::exchange(other.uploadedFireEmittersValid_, false);
    uploadedQualityControlsValid_ = std::exchange(other.uploadedQualityControlsValid_, false);
    worldSurfaceBuffer_ = std::exchange(other.worldSurfaceBuffer_, Buffer{});
    staticVertexBuffer_ = std::exchange(other.staticVertexBuffer_, Buffer{});
    worldPlayerVertexBuffer_ = std::exchange(other.worldPlayerVertexBuffer_, Buffer{});
    viewmodelVertexBuffer_ = std::exchange(other.viewmodelVertexBuffer_, Buffer{});
    staticIndexBuffer_ = std::exchange(other.staticIndexBuffer_, Buffer{});
    staticGeometryTransformBuffer_ = std::exchange(other.staticGeometryTransformBuffer_, Buffer{});
    instanceMetadataBuffer_ = std::exchange(other.instanceMetadataBuffer_, Buffer{});
    primitiveMetadataBuffer_ = std::exchange(other.primitiveMetadataBuffer_, Buffer{});
    materialMetadataBuffer_ = std::exchange(other.materialMetadataBuffer_, Buffer{});
    blas_ = std::exchange(other.blas_, AccelerationStructure{});
    waterfallBlas_ = std::exchange(other.waterfallBlas_, AccelerationStructure{});
    finaleRoofBlas_ = std::exchange(other.finaleRoofBlas_, AccelerationStructure{});
    torchBlas_ = std::exchange(other.torchBlas_, AccelerationStructure{});
    worldTorchBodyBlas_ = std::exchange(other.worldTorchBodyBlas_, AccelerationStructure{});
    swordBlas_ = std::exchange(other.swordBlas_, AccelerationStructure{});
    playerSwordScabbardBlas_ =
        std::exchange(other.playerSwordScabbardBlas_, AccelerationStructure{});
    gothicChestBaseBlas_ =
        std::exchange(other.gothicChestBaseBlas_, AccelerationStructure{});
    gothicChestLidBlas_ =
        std::exchange(other.gothicChestLidBlas_, AccelerationStructure{});
    rewardLanternRingBlas_ =
        std::exchange(other.rewardLanternRingBlas_, AccelerationStructure{});
    rewardLanternBodyBlas_ =
        std::exchange(other.rewardLanternBodyBlas_, AccelerationStructure{});
    dielectricFixtureBlas_ =
        std::exchange(other.dielectricFixtureBlas_, AccelerationStructure{});
    playerBodyBlas_ = std::exchange(other.playerBodyBlas_, AccelerationStructure{});
    playerLimbBlas_ = std::exchange(other.playerLimbBlas_, AccelerationStructure{});
    skinnedPlayerBlas_ = std::exchange(other.skinnedPlayerBlas_, AccelerationStructure{});
    viewmodelBlas_ = std::exchange(other.viewmodelBlas_, AccelerationStructure{});
    collapseBlas_ = std::exchange(other.collapseBlas_, AccelerationStructure{});
    waterDropletBlas_ = std::exchange(other.waterDropletBlas_, AccelerationStructure{});
    waterDropletBlasUpdateScratch_ = std::exchange(other.waterDropletBlasUpdateScratch_, Buffer{});
    viewmodelBlasUpdateScratch_ = std::exchange(other.viewmodelBlasUpdateScratch_, Buffer{});
    skinnedPlayerBlasUpdateScratch_ =
        std::exchange(other.skinnedPlayerBlasUpdateScratch_, Buffer{});
    tlas_ = std::exchange(other.tlas_, AccelerationStructure{});
    tlasUpdateScratch_ = std::exchange(other.tlasUpdateScratch_, Buffer{});
    characterSlot_ = std::move(other.characterSlot_);
    other.characterSlot_ = {};
    playerRenderSlot_ = std::move(other.playerRenderSlot_);
    other.playerRenderSlot_ = {};
    developmentStaticAsset_ = std::move(other.developmentStaticAsset_);
    collapseStaticAsset_ = std::move(other.collapseStaticAsset_);
    productionTorchAsset_ = std::move(other.productionTorchAsset_);
    playerTorchAsset_ = std::move(other.playerTorchAsset_);
    playerSwordScabbardAsset_ = std::move(other.playerSwordScabbardAsset_);
    productionPlayerAsset_ = std::move(other.productionPlayerAsset_);
    gothicChestBaseAsset_ = std::move(other.gothicChestBaseAsset_);
    gothicChestLidAsset_ = std::move(other.gothicChestLidAsset_);
    rewardLanternRingAsset_ = std::move(other.rewardLanternRingAsset_);
    rewardLanternBodyAsset_ = std::move(other.rewardLanternBodyAsset_);
    productionDielectricFixtureAsset_ =
        std::move(other.productionDielectricFixtureAsset_);
    waterDropletAsset_ = std::move(other.waterDropletAsset_);
    waterDropletGeometryVisible_ = std::exchange(other.waterDropletGeometryVisible_, false);
    skinnedPlayerUpload_ = std::move(other.skinnedPlayerUpload_);
    viewmodelAsset_ = std::move(other.viewmodelAsset_);
    viewmodelSkin_ = std::move(other.viewmodelSkin_);
    viewmodelPoseVertices_ = std::move(other.viewmodelPoseVertices_);
    viewmodelPoseTangents_ = std::move(other.viewmodelPoseTangents_);
    viewmodelUpload_ = std::move(other.viewmodelUpload_);
    viewmodelAvailable_ = std::exchange(other.viewmodelAvailable_, false);
    playerBodyRemainderAvailable_ = std::exchange(other.playerBodyRemainderAvailable_, false);
    viewmodelPoseCurrent_ = std::exchange(other.viewmodelPoseCurrent_, false);
#ifndef NDEBUG
    viewmodelCaptureTransform_ = std::exchange(other.viewmodelCaptureTransform_, {});
    playerWorldBodyCaptureTransform_ =
        std::exchange(other.playerWorldBodyCaptureTransform_, {});
    playerWorldBodyPoseCurrent_ =
        std::exchange(other.playerWorldBodyPoseCurrent_, false);
#endif
    playerStaticVertexBase_ = std::exchange(other.playerStaticVertexBase_, 0u);
    dielectricFixtureMaterialIndex_ =
        std::exchange(other.dielectricFixtureMaterialIndex_, 0u);
    playerCpuSkinCadence_ = std::exchange(other.playerCpuSkinCadence_, PlayerCpuSkinCadence::Hz60);
    measuredPlayerRoute_ = std::exchange(other.measuredPlayerRoute_, PlayerRenderRoute::Procedural);
    playerSkinUpdateCount_ = std::exchange(other.playerSkinUpdateCount_, 0u);
    playerSkinTotalMilliseconds_ = std::exchange(other.playerSkinTotalMilliseconds_, 0.0);
    playerMaxSocketErrorMetres_ = std::exchange(other.playerMaxSocketErrorMetres_, 0.0f);
    lastInstanceMasks_ = std::exchange(other.lastInstanceMasks_, {});
    lastPlayerPrimaryVisible_ = std::exchange(other.lastPlayerPrimaryVisible_, false);
    lastPlayerWorldBodyInstanceFlags_ = std::exchange(other.lastPlayerWorldBodyInstanceFlags_, 0u);
    rewardLanternGripAgreement_ = std::exchange(other.rewardLanternGripAgreement_, {});
    rewardLanternAuthorityAgreement_ = std::exchange(other.rewardLanternAuthorityAgreement_, {});
    rewardLanternFinalGripPosition_ = std::exchange(other.rewardLanternFinalGripPosition_, {});
    rewardLanternRingGripPosition_ = std::exchange(other.rewardLanternRingGripPosition_, {});
    rewardLanternBodyPosition_ = std::exchange(other.rewardLanternBodyPosition_, {});
    dielectricTransportOverflowCount_ =
        std::exchange(other.dielectricTransportOverflowCount_, 0u);
    dielectricShadowOverflowCount_ =
        std::exchange(other.dielectricShadowOverflowCount_, 0u);
    dielectricSecondaryRejectCount_ =
        std::exchange(other.dielectricSecondaryRejectCount_, 0u);
    dielectricUnclosedVolumeCount_ =
        std::exchange(other.dielectricUnclosedVolumeCount_, 0u);
    dielectricPrimaryUnclosedVolumeCount_ =
        std::exchange(other.dielectricPrimaryUnclosedVolumeCount_, 0u);
    dielectricShadowUnclosedVolumeCount_ =
        std::exchange(other.dielectricShadowUnclosedVolumeCount_, 0u);
    productionPaneStackFailureCount_ =
        std::exchange(other.productionPaneStackFailureCount_, 0u);
    productionPaneSecondaryOriginCount_ =
        std::exchange(other.productionPaneSecondaryOriginCount_, 0u);
    productionPaneSecondaryTerminalCount_ =
        std::exchange(other.productionPaneSecondaryTerminalCount_, 0u);
    productionPaneSecondarySameMediumCount_ =
        std::exchange(other.productionPaneSecondarySameMediumCount_, 0u);
    productionPaneSecondaryDifferentMediumCount_ =
        std::exchange(other.productionPaneSecondaryDifferentMediumCount_, 0u);
    secondaryNearSelfHitCount_ = std::exchange(other.secondaryNearSelfHitCount_, 0u);
    primaryOpenMissCount_ = std::exchange(other.primaryOpenMissCount_, 0u);
    primaryOpenOpaqueCount_ = std::exchange(other.primaryOpenOpaqueCount_, 0u);
    primaryMismatchedExitCount_ = std::exchange(other.primaryMismatchedExitCount_, 0u);
    primaryInterfaceBudgetCount_ = std::exchange(other.primaryInterfaceBudgetCount_, 0u);
    primaryVolumeBudgetCount_ = std::exchange(other.primaryVolumeBudgetCount_, 0u);
    shadowOpenMissCount_ = std::exchange(other.shadowOpenMissCount_, 0u);
    shadowMismatchedExitCount_ = std::exchange(other.shadowMismatchedExitCount_, 0u);
    primaryTirCount_ = std::exchange(other.primaryTirCount_, 0u);
    primaryInterfaceBudgetOpenVolumeCount_ =
        std::exchange(other.primaryInterfaceBudgetOpenVolumeCount_, 0u);
    primaryInterfaceBudgetClosedVolumeCount_ =
        std::exchange(other.primaryInterfaceBudgetClosedVolumeCount_, 0u);
    shadowMismatchEmptyCount_ = std::exchange(other.shadowMismatchEmptyCount_, 0u);
    shadowImplicitOriginExitCount_ =
        std::exchange(other.shadowImplicitOriginExitCount_, 0u);
    secondaryDielectricTerminalCount_ =
        std::exchange(other.secondaryDielectricTerminalCount_, 0u);
    primaryTirTerminationCount_ =
        std::exchange(other.primaryTirTerminationCount_, 0u);
    shadowFiniteEndpointVolumeCount_ =
        std::exchange(other.shadowFiniteEndpointVolumeCount_, 0u);
    primaryOpenOpaqueSameInstanceDifferentMaterialCount_ =
        std::exchange(other.primaryOpenOpaqueSameInstanceDifferentMaterialCount_, 0u);
    primaryOpenOpaqueAfterTirCount_ =
        std::exchange(other.primaryOpenOpaqueAfterTirCount_, 0u);
    primaryOpenOpaqueTerminalInstanceMask_ =
        std::exchange(other.primaryOpenOpaqueTerminalInstanceMask_, 0u);
    primaryOpenOpaqueVolumeInstanceMask_ =
        std::exchange(other.primaryOpenOpaqueVolumeInstanceMask_, 0u);
    primaryOpenOpaqueTerminalMaterialMask_ =
        std::exchange(other.primaryOpenOpaqueTerminalMaterialMask_, 0u);
    primaryClosedVolumeAbsorptionCount_ =
        std::exchange(other.primaryClosedVolumeAbsorptionCount_, 0u);
    primaryCertifiedClosedVolumeRecoveryCount_ =
        std::exchange(other.primaryCertifiedClosedVolumeRecoveryCount_, 0u);
    shadowCertifiedClosedVolumeRecoveryCount_ =
        std::exchange(other.shadowCertifiedClosedVolumeRecoveryCount_, 0u);
    certifiedClosedVolumeRecoveryReasonMask_ =
        std::exchange(other.certifiedClosedVolumeRecoveryReasonMask_, 0u);
    primaryTorchPixelCount_ = std::exchange(other.primaryTorchPixelCount_, 0u);
    primarySwordPixelCount_ = std::exchange(other.primarySwordPixelCount_, 0u);
    primaryPlayerPixelCount_ = std::exchange(other.primaryPlayerPixelCount_, 0u);
    primaryRewardRingPixelCount_ =
        std::exchange(other.primaryRewardRingPixelCount_, 0u);
    primaryRewardBodyPixelCount_ =
        std::exchange(other.primaryRewardBodyPixelCount_, 0u);
    developmentStaticAssetDirectory_ = std::move(other.developmentStaticAssetDirectory_);
    staticTextureDirectory_ = std::move(other.staticTextureDirectory_);
    staticMeshSlot_ = std::move(other.staticMeshSlot_);
    genericStaticAssetEnabled_ = std::exchange(other.genericStaticAssetEnabled_, false);
    genericTransmissionActive_ = std::exchange(other.genericTransmissionActive_, false);
    productionHeldItemAssetsEnabled_ =
        std::exchange(other.productionHeldItemAssetsEnabled_, false);
    staticMeshBlasBytes_ = std::exchange(other.staticMeshBlasBytes_, 0u);
    staticTextureBytes_ = std::exchange(other.staticTextureBytes_, 0u);
    staticMeshBlasBuildMilliseconds_ = std::exchange(other.staticMeshBlasBuildMilliseconds_, 0.0);
    productionPropBlasBytes_ = std::exchange(other.productionPropBlasBytes_, 0u);
    productionPropBlasBuildMilliseconds_ =
        std::exchange(other.productionPropBlasBuildMilliseconds_, 0.0);
    heldItemBlasMeasurements_ = std::exchange(
        other.heldItemBlasMeasurements_, HeldItemBlasMeasurements{});
    pipelineBundle_ = std::move(other.pipelineBundle_);
#ifdef HORDE_RT_STAGED_PRIMARY_EXPERIMENT
    stagedPrimary_ = std::move(other.stagedPrimary_);
#endif
    executionPolicy_ = std::exchange(other.executionPolicy_, RtExecutionPolicy{});
    computeDispatchGroups_ = std::exchange(other.computeDispatchGroups_, {});
    pipelineEvidenceIdentity_ = std::exchange(
        other.pipelineEvidenceIdentity_,
        horde::telemetry::RtPipelineEvidenceIdentity{});
    pipelineEvidenceIdentityValid_ = std::exchange(
        other.pipelineEvidenceIdentityValid_, false);
    initialiseMeasurements_ = std::move(other.initialiseMeasurements_);
    other.initialiseMeasurements_ = {};
    framePipelineEvidence_ = std::exchange(
        other.framePipelineEvidence_,
        horde::telemetry::RtPipelineEvidenceIdentity{});
    framePipelineEvidenceValid_ = std::exchange(
        other.framePipelineEvidenceValid_, false);
    vkCreateAccelerationStructureKHR_ = other.vkCreateAccelerationStructureKHR_;
    vkDestroyAccelerationStructureKHR_ = other.vkDestroyAccelerationStructureKHR_;
    vkGetAccelerationStructureBuildSizesKHR_ = other.vkGetAccelerationStructureBuildSizesKHR_;
    vkGetAccelerationStructureDeviceAddressKHR_ = other.vkGetAccelerationStructureDeviceAddressKHR_;
    vkCmdBuildAccelerationStructuresKHR_ = other.vkCmdBuildAccelerationStructuresKHR_;
    vkCreateRayTracingPipelinesKHR_ = other.vkCreateRayTracingPipelinesKHR_;
    vkGetRayTracingShaderGroupHandlesKHR_ = other.vkGetRayTracingShaderGroupHandlesKHR_;
    vkCmdTraceRaysKHR_ = other.vkCmdTraceRaysKHR_;
    vkGetBufferDeviceAddressKHR_ = other.vkGetBufferDeviceAddressKHR_;
    gpuResources_.Bind(physicalDevice_, device_, vkDestroyAccelerationStructureKHR_, vkGetBufferDeviceAddressKHR_);
    pipelineBundle_.RebindDestroyContext(this, &gpuResources_);
    scratchAddressAlignment_ = std::exchange(other.scratchAddressAlignment_, 0u);
    developmentSupportFixture_ = std::exchange(other.developmentSupportFixture_, false);
    developmentWorldRoute_=std::exchange(other.developmentWorldRoute_,false);
    developmentRescueJourney_=std::exchange(other.developmentRescueJourney_,false);
    rescueWorldUpdateScratch_=std::exchange(other.rescueWorldUpdateScratch_,Buffer{});
    rescueWorldVertices_=std::move(other.rescueWorldVertices_);
    rescueWorldSurfaceCodes_=std::move(other.rescueWorldSurfaceCodes_);
    rescueRopeVertexOffset_=std::exchange(other.rescueRopeVertexOffset_,0);
    rescueWorldPrimitiveCount_=std::exchange(other.rescueWorldPrimitiveCount_,0);
    rescueWorldMaxVertex_=std::exchange(other.rescueWorldMaxVertex_,0);
    stagedWorldPreparation_=std::exchange(other.stagedWorldPreparation_,false);
    worldRouteGeometry_=std::move(other.worldRouteGeometry_);
    sceneProfile_ = std::exchange(other.sceneProfile_, RtSceneProfile::Showcase);
    glassEnabled_ = std::exchange(other.glassEnabled_, true);
    mistEnabled_ = std::exchange(other.mistEnabled_, true);
    dustQuality_ = std::exchange(other.dustQuality_, horde::graphics::DustQuality::Off);
    dustWork_ = std::exchange(other.dustWork_, {});
    dustCache_ = std::exchange(other.dustCache_, {});
    sceneMaterials_ = std::move(other.sceneMaterials_);
    worldMaterialBase_ = std::exchange(other.worldMaterialBase_, 0u);
    tlasInstanceCount_ = std::exchange(other.tlasInstanceCount_, kTlasInstanceCount);
    previewTransforms_ = std::exchange(other.previewTransforms_, {});
    previewFireInputs_ = std::exchange(other.previewFireInputs_, {});
    other.gpuResources_.Reset();
    ready_ = std::exchange(other.ready_, false);

    return *this;
}

bool PresentableTinyRtScene::Initialise(VkInstance instance,
                                        VkPhysicalDevice physicalDevice,
                                        VkDevice device,
                                        VkQueue queue,
                                        VkCommandPool commandPool,
                                        VkExtent2D dispatchExtent,
                                        VkFormat presentationFormat,
                                        const std::string& skeletonAssetPath,
                                        const std::string& lichAssetPath,
                                        const std::string& materialAssetDirectory,
                                        const std::string& lichTextureDirectory,
                                        std::string& diagnostic,
                                        const std::string& developmentStaticAssetDirectory,
                                        const std::string& productionAssetRoot,
                                        RtExecutionBackend executionBackend,
                                        RtSceneProfile sceneProfile,
                                        bool glassEnabled,
                                        VkPipelineCache pipelineCache,
                                        RtBundleCompiledPipelineCache* compiledPipelineCache)
{
    InitialiseOrchestrationApi api{};
    api.user = &executionBackend;
    api.resolvePreflight = [](void* user, RtPipelineBundlePreflight& preflight,
                              std::string& failureKey) {
        return ResolveCompiledRtPipelineBundlePreflight(
            preflight, failureKey, *static_cast<RtExecutionBackend*>(user));
    };
    api.continueAfterPreflight = [](
        void*, PresentableTinyRtScene& scene, VkFormat format,
        const std::string& skeletonPath, const std::string& lichPath,
        const std::string& materialDirectory, const std::string& lichDirectory,
        const std::string& developmentDirectory, const std::string& productionRoot,
        std::string& error) {
        return scene.ContinueInitialiseAfterPreflight(
            format, skeletonPath, lichPath, materialDirectory, lichDirectory,
            developmentDirectory, productionRoot, error);
    };
    return InitialiseWithOrchestration(
        instance, physicalDevice, device, queue, commandPool, dispatchExtent,
        presentationFormat, skeletonAssetPath, lichAssetPath,
        materialAssetDirectory, lichTextureDirectory, diagnostic,
        developmentStaticAssetDirectory, productionAssetRoot, api, sceneProfile, glassEnabled,
        pipelineCache, compiledPipelineCache);
}

bool PresentableTinyRtScene::InitialiseWithOrchestration(
    VkInstance instance,
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkQueue queue,
    VkCommandPool commandPool,
    VkExtent2D dispatchExtent,
    VkFormat presentationFormat,
    const std::string& skeletonAssetPath,
    const std::string& lichAssetPath,
    const std::string& materialAssetDirectory,
    const std::string& lichTextureDirectory,
    std::string& diagnostic,
    const std::string& developmentStaticAssetDirectory,
    const std::string& productionAssetRoot,
    const InitialiseOrchestrationApi& api,
    const RtSceneProfile sceneProfile,
    const bool glassEnabled,
    const VkPipelineCache pipelineCache,
    RtBundleCompiledPipelineCache* compiledPipelineCache)
{
    Destroy();
    initialiseMeasurements_ = {};
    InitialiseAttemptTimer totalAttemptTimer(initialiseMeasurements_);
    sceneProfile_ = sceneProfile;
    glassEnabled_ = glassEnabled;
    instance_ = instance;
    physicalDevice_ = physicalDevice;
    device_ = device;
    queue_ = queue;
    commandPool_ = commandPool;
    pipelineCache_ = pipelineCache;
    dispatchExtent_ = dispatchExtent;
    presentationUsesBgra_ = presentationFormat == VK_FORMAT_B8G8R8A8_UNORM || presentationFormat == VK_FORMAT_B8G8R8A8_SRGB;
    if (instance_ == VK_NULL_HANDLE || physicalDevice_ == VK_NULL_HANDLE || device_ == VK_NULL_HANDLE || queue_ == VK_NULL_HANDLE || commandPool_ == VK_NULL_HANDLE)
    {
        diagnostic = "Invalid Vulkan handles supplied to presentable RT scene.";
        return false;
    }
    if (dispatchExtent_.width == 0u || dispatchExtent_.height == 0u)
    {
        diagnostic = "RT dispatch extent is zero.";
        return false;
    }
    if (api.resolvePreflight == nullptr || api.continueAfterPreflight == nullptr)
    {
        diagnostic = "Invalid RT scene initialisation orchestration.";
        Destroy();
        return false;
    }
    RtPipelineBundlePreflight selectedPreflight{};
    if (!MeasureInitialisationStage(initialiseMeasurements_, "preflight", [&] {
            return api.resolvePreflight(api.user, selectedPreflight, diagnostic);
        }))
    {
        Destroy();
        return false;
    }
    RtPipelineBundleDestroyApi destroyApi{};
    compiledPipelineCache_ = selectedPreflight.request.executionBackend ==
            RtExecutionBackend::RayTracingPipeline ? compiledPipelineCache : nullptr;
    destroyApi.user = this;
    destroyApi.gpuResources = &gpuResources_;
    destroyApi.destroyBuffer = [](void*, RtGpuResources* resources, RtGpuBuffer& buffer,
                                  RtPipelineOwnedBuffer) noexcept {
        if (resources != nullptr) resources->DestroyBuffer(buffer);
        else buffer = {};
    };
    destroyApi.destroyPipeline = [](void* user, VkPipeline& pipeline,
                                    RtMaterialStrategy) noexcept {
        auto& scene = *static_cast<PresentableTinyRtScene*>(user);
        if (scene.device_ != VK_NULL_HANDLE && pipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(scene.device_, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    };
    destroyApi.destroyPipelineLayout = [](void* user, VkPipelineLayout& layout) noexcept {
        auto& scene = *static_cast<PresentableTinyRtScene*>(user);
        if (scene.device_ != VK_NULL_HANDLE && layout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(scene.device_, layout, nullptr);
        layout = VK_NULL_HANDLE;
    };
    destroyApi.destroyDescriptorPool = [](void* user, VkDescriptorPool& pool) noexcept {
        auto& scene = *static_cast<PresentableTinyRtScene*>(user);
        if (scene.device_ != VK_NULL_HANDLE && pool != VK_NULL_HANDLE)
            vkDestroyDescriptorPool(scene.device_, pool, nullptr);
        pool = VK_NULL_HANDLE;
    };
    destroyApi.destroyDescriptorSetLayout = [](void* user,
                                               VkDescriptorSetLayout& layout) noexcept {
        auto& scene = *static_cast<PresentableTinyRtScene*>(user);
        if (scene.device_ != VK_NULL_HANDLE && layout != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(scene.device_, layout, nullptr);
        layout = VK_NULL_HANDLE;
    };
    if (!pipelineBundle_.AdoptPreflight(
            std::move(selectedPreflight), destroyApi, diagnostic))
    {
        Destroy();
        return false;
    }
    const auto executionPolicy = TryMakeRtExecutionPolicy(ExecutionBackend());
    if (!executionPolicy)
    {
        diagnostic = "Selected RT execution backend has no valid Vulkan policy.";
        Destroy();
        return false;
    }
    executionPolicy_ = *executionPolicy;
    const bool initialised = api.continueAfterPreflight(
        api.user, *this, presentationFormat, skeletonAssetPath, lichAssetPath,
        materialAssetDirectory, lichTextureDirectory,
        developmentStaticAssetDirectory, productionAssetRoot, diagnostic);
    initialiseMeasurements_.succeeded = initialised;
    return initialised;
}

bool PresentableTinyRtScene::ContinueInitialiseAfterPreflight(
    VkFormat presentationFormat,
    const std::string& skeletonAssetPath,
    const std::string& lichAssetPath,
    const std::string& materialAssetDirectory,
    const std::string& lichTextureDirectory,
    const std::string& developmentStaticAssetDirectory,
    const std::string& productionAssetRoot,
    std::string& diagnostic)
{
    if (!MeasureInitialisationStage(initialiseMeasurements_, "device-capability-preflight", [&] {
            if (ExecutionBackend() == RtExecutionBackend::RayQueryCompute)
            {
                VkPhysicalDeviceProperties properties{};
                vkGetPhysicalDeviceProperties(physicalDevice_, &properties);
                const auto groups = TryMakeRtComputeDispatch(dispatchExtent_, properties.limits);
                if (!groups)
                {
                    diagnostic = "Device compute limits cannot execute the fixed 8x8 hardware RT workload.";
                    return false;
                }
                computeDispatchGroups_ = *groups;
            }
            VkFormatProperties storageFormatProperties{};
            VkFormatProperties presentationFormatProperties{};
            vkGetPhysicalDeviceFormatProperties(physicalDevice_, kStorageImageFormat, &storageFormatProperties);
            vkGetPhysicalDeviceFormatProperties(physicalDevice_, presentationFormat, &presentationFormatProperties);
            const VkFormatFeatureFlags storageBlitFeatures = VK_FORMAT_FEATURE_BLIT_SRC_BIT |
                                                             VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
            scaledBlitSupported_ = (storageFormatProperties.optimalTilingFeatures & storageBlitFeatures) == storageBlitFeatures &&
                                   (presentationFormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) != 0u;
            return true;
        }))
    {
        Destroy();
        return false;
    }
    if (sceneProfile_ != RtSceneProfile::EntryMenu &&
        !MeasureInitialisationStage(initialiseMeasurements_, "character-assets", [&] {
            return characterSlot_.LoadAssets(skeletonAssetPath, lichAssetPath, diagnostic,
                                             sceneProfile_ == RtSceneProfile::GraphicsPreview);
        }))
    {
        Destroy();
        return false;
    }
    if (!MeasureInitialisationStage(initialiseMeasurements_, "vulkan-entry-points", [&] {
            return LoadEntryPoints(diagnostic);
        }))
    {
        Destroy();
        return false;
    }
    gpuResources_.Bind(physicalDevice_, device_, vkDestroyAccelerationStructureKHR_, vkGetBufferDeviceAddressKHR_);
    if (!MeasureInitialisationStage(initialiseMeasurements_, "static-held-item-assets", [&] {
            return LoadStaticHeldItemAssets(
                developmentStaticAssetDirectory, productionAssetRoot, diagnostic);
        }) ||
        !MeasureInitialisationStage(initialiseMeasurements_, "storage-image", [&] {
            return CreateStorageImage(diagnostic);
        }) ||
        !MeasureInitialisationStage(initialiseMeasurements_, "material-textures", [&] {
            return CreateMaterialTextures(materialAssetDirectory, diagnostic);
        }) ||
        !MeasureInitialisationStage(initialiseMeasurements_, "environment-texture", [&] {
            return CreateEnvironmentTexture(productionAssetRoot, diagnostic);
        }) ||
        (sceneProfile_ == RtSceneProfile::Showcase &&
         !MeasureInitialisationStage(initialiseMeasurements_, "lich-textures", [&] {
            return CreateLichTextures(lichTextureDirectory, diagnostic);
         })) ||
        !MeasureInitialisationStage(initialiseMeasurements_, "static-meshes", [&] {
            return CreateStaticMeshResources(diagnostic);
        }) ||
        !MeasureInitialisationStage(initialiseMeasurements_, "acceleration-structures", [&] {
            return BuildAccelerationStructures(diagnostic);
        }) ||
        !MeasureInitialisationStage(initialiseMeasurements_, "pipeline-bundle", [&] {
            return CreateSelectedPipelineBundle(diagnostic);
        }))
    {
        Destroy();
        return false;
    }

#if defined(HORDE_RT_STAGED_PRIMARY_EXPERIMENT) && HORDE_RT_STAGED_PRIMARY_DEFAULT
    if (ExecutionBackend() != RtExecutionBackend::RayTracingPipeline)
    {
        diagnostic = "StagedPrimaryV1Investigation requires RayTracingPipeline; no alternate-backend substitution.";
        Destroy();
        return false;
    }
    RtPipelineBundleBuildApi stagedApi{};
    stagedApi.user = this;
    stagedApi.createSharedShaderModules = [](void* user, VkShaderModule& miss, VkShaderModule& hit, std::string& error) {
        return static_cast<PresentableTinyRtScene*>(user)->CreateBundleSharedShaderModules(miss, hit, error);
    };
    stagedApi.createEntryShaderModule = [](void* user, const RtPipelineVariantArtifact& artifact, VkShaderModule& out, std::string& error) {
        return static_cast<PresentableTinyRtScene*>(user)->CreateBundleEntryShaderModule(artifact, out, error);
    };
    stagedApi.createStrategyPipeline = [](void* user, RtMaterialStrategy strategy, VkShaderModule entry, VkShaderModule miss,
                                         VkShaderModule hit, VkPipelineLayout layout, VkPipeline& out, std::string& error) {
        return static_cast<PresentableTinyRtScene*>(user)->CreateBundleStrategyPipeline(strategy, entry, miss, hit, layout, out, error);
    };
    stagedApi.createStrategySbt = [](void* user, RtMaterialStrategy strategy, VkPipeline pipeline, Buffer& out,
                                   std::array<VkStridedDeviceAddressRegionKHR, 4u>& regions, std::string& error) {
        return static_cast<PresentableTinyRtScene*>(user)->CreateBundleStrategySbt(strategy, pipeline, out, regions, error);
    };
    stagedApi.destroyShaderModule = [](void* user, VkShaderModule& module) noexcept {
        auto& scene = *static_cast<PresentableTinyRtScene*>(user);
        if (module != VK_NULL_HANDLE) vkDestroyShaderModule(scene.device_, module, nullptr);
        module = VK_NULL_HANDLE;
    };
    stagedPrimary_ = experimental::StagedPrimaryPass::Create(physicalDevice_, device_, gpuResources_,
        pipelineBundle_.descriptorSetLayout, pipelineBundle_.descriptorSet, dispatchExtent_, sizeof(ScenePushConstants),
        vkCmdTraceRaysKHR_, stagedApi, diagnostic);
    if (!stagedPrimary_) { Destroy(); return false; }
#endif
    pipelineEvidenceIdentityValid_ = CapturePipelineEvidenceIdentity();
    ready_ = true;
    diagnostic.clear();
    return true;
}

horde::gameplay::simulation::ZoneReadiness PresentableTinyRtScene::WorldZoneReadiness(
    horde::gameplay::simulation::WorldZoneToken token) const
{
    using namespace horde::gameplay::simulation;
    if(token.generation==0 || static_cast<std::size_t>(token.zone)>=kWorldZones.size() || !developmentWorldRoute_)
        return ZoneReadiness::Unprepared;
    // ready_ is set only after real uploads, completed one-time BLAS/TLAS builds
    // and the selected pipeline bundle succeed. No platform declares GPU readiness.
    if(!ready_ || !worldRouteGeometry_.valid || vertexBuffer_.memory==VK_NULL_HANDLE ||
        indexBuffer_.memory==VK_NULL_HANDLE || blas_.handle==VK_NULL_HANDLE || tlas_.handle==VK_NULL_HANDLE)
        return ZoneReadiness::Preparing;
    if(developmentRescueJourney_ && (rescueWorldUpdateScratch_.buffer==VK_NULL_HANDLE ||
        rescueWorldVertices_.empty() || rescueWorldPrimitiveCount_==0 ||
        rescueWorldSurfaceCodes_.size()!=rescueWorldPrimitiveCount_)) return ZoneReadiness::Preparing;
    return ZoneReadiness::Ready;
}
void PresentableTinyRtScene::Destroy()
{
    worldRouteGeometry_ = {}; // Release CPU batches with the renderer-owned scene.
    pipelineCache_ = VK_NULL_HANDLE; // Borrowed device cache remains owned by the platform context.
    uploadedQualityControls_ = {};
    uploadedFireQuality_ = FireEmitterQuality::Mobile;
    uploadedQualityControlsValid_ = false;
    uploadedFireEmitters_ = {};
    uploadedFireEmittersValid_ = false;
    glassEnabled_ = true;
    mistEnabled_ = true;
    dustQuality_ = horde::graphics::DustQuality::Off;
    dustWork_ = {};
    dustCache_.Invalidate();
    tlasBuiltInstances_ = {};
    tlasInstanceDefinitionsValid_ = false;
    tlasPendingInstances_ = {};
    tlasPendingDefinitionsValid_ = false;
    executionPolicy_ = {};
    computeDispatchGroups_ = {};
    if (device_ == VK_NULL_HANDLE)
    {
        pipelineBundle_.Reset();
        compiledPipelineCache_ = nullptr;
        worldTorchBodyBlas_ = {};
        waterDropletBlas_ = {};
        waterDropletBlasUpdateScratch_ = {};
        waterDropletAsset_ = {};
        waterDropletGeometryVisible_ = false;
        pipelineEvidenceIdentity_ = {};
        pipelineEvidenceIdentityValid_ = false;
        framePipelineEvidence_ = {};
        framePipelineEvidenceValid_ = false;
        ready_ = false;
        return;
    }

#ifdef HORDE_RT_STAGED_PRIMARY_EXPERIMENT
    stagedPrimary_.reset();
#endif
    pipelineBundle_.Reset();
    compiledPipelineCache_ = nullptr;
    DestroyAccelerationStructure(tlas_);
    DestroyBuffer(tlasUpdateScratch_);
    DestroyBuffer(rescueWorldUpdateScratch_);
    DestroyBuffer(waterDropletBlasUpdateScratch_);
    rescueWorldVertices_.clear();
    rescueWorldSurfaceCodes_.clear();
    characterSlot_.DestroyGpuResources(gpuResources_);
    DestroyBuffer(skinnedPlayerBlasUpdateScratch_);
    DestroyBuffer(viewmodelBlasUpdateScratch_);
    DestroyAccelerationStructure(viewmodelBlas_);
    DestroyAccelerationStructure(collapseBlas_);
    DestroyAccelerationStructure(waterDropletBlas_);
    DestroyAccelerationStructure(skinnedPlayerBlas_);
    DestroyAccelerationStructure(playerLimbBlas_);
    DestroyAccelerationStructure(playerBodyBlas_);
    DestroyAccelerationStructure(dielectricFixtureBlas_);
    DestroyAccelerationStructure(rewardLanternBodyBlas_);
    DestroyAccelerationStructure(rewardLanternRingBlas_);
    DestroyAccelerationStructure(gothicChestLidBlas_);
    DestroyAccelerationStructure(gothicChestBaseBlas_);
    DestroyAccelerationStructure(swordBlas_);
    DestroyAccelerationStructure(playerSwordScabbardBlas_);
    DestroyAccelerationStructure(torchBlas_);
    DestroyAccelerationStructure(worldTorchBodyBlas_);
    DestroyAccelerationStructure(finaleRoofBlas_);
    DestroyAccelerationStructure(waterfallBlas_);
    DestroyAccelerationStructure(blas_);
    DestroyBuffer(worldSurfaceBuffer_);
    DestroyBuffer(materialMetadataBuffer_);
    DestroyBuffer(primitiveMetadataBuffer_);
    DestroyBuffer(instanceMetadataBuffer_);
    DestroyBuffer(staticIndexBuffer_);
    DestroyBuffer(staticGeometryTransformBuffer_);
    DestroyBuffer(staticVertexBuffer_);
    DestroyBuffer(worldPlayerVertexBuffer_);
    DestroyBuffer(viewmodelVertexBuffer_);
    DestroyBuffer(heldLightBuffer_);
    DestroyBuffer(fireEmitterBuffer_);
    DestroyBuffer(qualityControlsBuffer_);
    DestroyBuffer(waterContactRippleBuffer_);
    DestroyBuffer(instanceBuffer_);
    DestroyBuffer(transformBuffer_);
    DestroyBuffer(indexBuffer_);
    DestroyBuffer(vertexBuffer_);

    if (materialSampler_ != VK_NULL_HANDLE)
    {
        vkDestroySampler(device_, materialSampler_, nullptr);
        materialSampler_ = VK_NULL_HANDLE;
    }
    if (environmentSampler_ != VK_NULL_HANDLE)
    {
        vkDestroySampler(device_, environmentSampler_, nullptr);
        environmentSampler_ = VK_NULL_HANDLE;
    }
    DestroyTextureArray(environmentTexture_);
    DestroyTextureArray(materialArm_);
    DestroyTextureArray(materialNormal_);
    DestroyTextureArray(materialDiffuse_);
    DestroyTextureArray(lichEmissive_);
    DestroyTextureArray(lichBaseColor_);
    DestroyTextureArray(staticEmissive_);
    DestroyTextureArray(staticOrm_);
    DestroyTextureArray(staticNormal_);
    DestroyTextureArray(staticBaseColor_);
    materialEncoding_.clear();
    developmentStaticAsset_ = {};
    collapseStaticAsset_ = {};
    productionTorchAsset_ = {};
    playerTorchAsset_ = {};
    playerSwordScabbardAsset_ = {};
    productionPlayerAsset_ = {};
    gothicChestBaseAsset_ = {};
    gothicChestLidAsset_ = {};
    rewardLanternRingAsset_ = {};
    rewardLanternBodyAsset_ = {};
    productionDielectricFixtureAsset_ = {};
    waterDropletAsset_ = {};
    waterDropletGeometryVisible_ = false;
    playerRenderSlot_ = {};
    skinnedPlayerUpload_.clear();
    viewmodelAsset_ = {};
    viewmodelSkin_ = {};
    viewmodelPoseVertices_.clear();
    viewmodelPoseTangents_.clear();
    viewmodelUpload_.clear();
    viewmodelAvailable_ = false;
    playerBodyRemainderAvailable_ = false;
    lastPlayerWorldBodyInstanceFlags_ = 0u;
    viewmodelPoseCurrent_ = false;
#ifndef NDEBUG
    viewmodelCaptureTransform_ = {};
    playerWorldBodyCaptureTransform_ = {};
    playerWorldBodyPoseCurrent_ = false;
#endif
    playerStaticVertexBase_ = 0u;
    dielectricFixtureMaterialIndex_ = 0u;
    dielectricTransportOverflowCount_ = 0u;
    dielectricShadowOverflowCount_ = 0u;
    dielectricSecondaryRejectCount_ = 0u;
    dielectricUnclosedVolumeCount_ = 0u;
    dielectricPrimaryUnclosedVolumeCount_ = 0u;
    dielectricShadowUnclosedVolumeCount_ = 0u;
    productionPaneStackFailureCount_ = 0u;
    productionPaneSecondaryOriginCount_ = 0u;
    productionPaneSecondaryTerminalCount_ = 0u;
    productionPaneSecondarySameMediumCount_ = 0u;
    productionPaneSecondaryDifferentMediumCount_ = 0u;
    secondaryNearSelfHitCount_ = 0u;
    primaryOpenMissCount_ = 0u;
    primaryOpenOpaqueCount_ = 0u;
    primaryMismatchedExitCount_ = 0u;
    primaryInterfaceBudgetCount_ = 0u;
    primaryVolumeBudgetCount_ = 0u;
    shadowOpenMissCount_ = 0u;
    shadowMismatchedExitCount_ = 0u;
    primaryTirCount_ = 0u;
    primaryInterfaceBudgetOpenVolumeCount_ = 0u;
    primaryInterfaceBudgetClosedVolumeCount_ = 0u;
    shadowMismatchEmptyCount_ = 0u;
    shadowImplicitOriginExitCount_ = 0u;
    secondaryDielectricTerminalCount_ = 0u;
    primaryTirTerminationCount_ = 0u;
    shadowFiniteEndpointVolumeCount_ = 0u;
    primaryOpenOpaqueSameInstanceDifferentMaterialCount_ = 0u;
    primaryOpenOpaqueAfterTirCount_ = 0u;
    primaryOpenOpaqueTerminalInstanceMask_ = 0u;
    primaryOpenOpaqueVolumeInstanceMask_ = 0u;
    primaryOpenOpaqueTerminalMaterialMask_ = 0u;
    primaryClosedVolumeAbsorptionCount_ = 0u;
    primaryCertifiedClosedVolumeRecoveryCount_ = 0u;
    shadowCertifiedClosedVolumeRecoveryCount_ = 0u;
    certifiedClosedVolumeRecoveryReasonMask_ = 0u;
    primaryTorchPixelCount_ = 0u;
    primarySwordPixelCount_ = 0u;
    primaryPlayerPixelCount_ = 0u;
    primaryRewardRingPixelCount_ = 0u;
    primaryRewardBodyPixelCount_ = 0u;
    developmentStaticAssetDirectory_.clear();
    staticTextureDirectory_.clear();
    staticMeshSlot_ = {};
    genericStaticAssetEnabled_ = false;
    genericTransmissionActive_ = false;
    productionHeldItemAssetsEnabled_ = false;
    staticMeshBlasBytes_ = 0u;
    staticTextureBytes_ = 0u;
    staticMeshBlasBuildMilliseconds_ = 0.0;
    productionPropBlasBytes_ = 0u;
    productionPropBlasBuildMilliseconds_ = 0.0;
    heldItemBlasMeasurements_ = {};
    pipelineEvidenceIdentity_ = {};
    pipelineEvidenceIdentityValid_ = false;
    framePipelineEvidence_ = {};
    framePipelineEvidenceValid_ = false;

    auto output = OutputImage();
    DestroyOutputImage(output);
    AdoptOutputImage(output);
    storageImageFrameRecorded_ = false;
    lastOutputRedBlueSwapApplied_ = false;

    scaledBlitSupported_ = false;
    ready_ = false;
    gpuResources_.Reset();
    scratchAddressAlignment_ = 0u;
    sceneProfile_ = RtSceneProfile::Showcase;
    sceneMaterials_.clear();
    worldMaterialBase_ = 0u;
    tlasInstanceCount_ = kTlasInstanceCount;
    previewTransforms_ = {};
    previewFireInputs_ = {};
}

#ifndef NDEBUG
PresentableTinyRtScene::ResourceHandleSnapshot
PresentableTinyRtScene::CaptureResourceHandles() const
{
    ResourceHandleSnapshot result;
    result.ready = ready_;
    const auto bits = [](const auto handle) {
        std::uint64_t value = 0u;
        static_assert(sizeof(handle) <= sizeof(value));
        std::memcpy(&value, &handle, sizeof(handle));
        return value;
    };
    const auto append = [&bits](auto& values, const auto handle) {
        if (handle != VK_NULL_HANDLE) values.push_back(bits(handle));
    };
    for (const AccelerationStructure* blas : std::array{
             &blas_, &waterfallBlas_, &finaleRoofBlas_, &torchBlas_, &worldTorchBodyBlas_, &swordBlas_,
             &playerSwordScabbardBlas_,
             &gothicChestBaseBlas_, &gothicChestLidBlas_, &rewardLanternRingBlas_,
             &rewardLanternBodyBlas_, &dielectricFixtureBlas_, &playerBodyBlas_,
             &playerLimbBlas_, &skinnedPlayerBlas_, &viewmodelBlas_, &collapseBlas_,
             &waterDropletBlas_})
        append(result.bottomLevelAccelerationStructures, blas->handle);
    for (std::size_t bucket = 0u;
         bucket < CharacterRenderSlot::kMaximumSkeletonPoseBuckets; ++bucket)
        append(result.bottomLevelAccelerationStructures,
               characterSlot_.SkeletonGpu(bucket).accelerationStructure.handle);
    append(result.bottomLevelAccelerationStructures,
           characterSlot_.LichGpu().accelerationStructure.handle);
    append(result.topLevelAccelerationStructures, tlas_.handle);
    for (const auto strategy : {RtMaterialStrategy::OpaqueFast,
                               RtMaterialStrategy::GenericDielectric})
    {
        const auto& resources = pipelineBundle_.Strategy(strategy);
        append(result.pipelines, resources.pipeline);
        append(result.shaderBindingTableBuffers, resources.shaderBindingTable.buffer);
    }
    append(result.descriptorSets, pipelineBundle_.descriptorSet);
    for (const TextureArray* texture : std::array<const TextureArray*, 10u>{
             &environmentTexture_, &materialDiffuse_, &materialNormal_, &materialArm_,
             &lichBaseColor_, &lichEmissive_, &staticBaseColor_, &staticNormal_,
             &staticOrm_, &staticEmissive_})
        append(result.textureImages, texture->image);
    result.outputImage = bits(storageImage_);
    result.outputMemory = bits(storageImageMemory_);
    result.outputView = bits(storageImageView_);
    return result;
}
#endif

horde::telemetry::RtResourceInventory PresentableTinyRtScene::ResourceInventory() const noexcept
{
    horde::telemetry::RtResourceInventory inventory{};
    for (const Buffer* buffer : std::array{
             &vertexBuffer_, &indexBuffer_, &transformBuffer_, &instanceBuffer_,
             &heldLightBuffer_, &fireEmitterBuffer_, &qualityControlsBuffer_, &waterContactRippleBuffer_, &worldSurfaceBuffer_,
             &staticVertexBuffer_, &worldPlayerVertexBuffer_, &viewmodelVertexBuffer_,
             &staticIndexBuffer_, &staticGeometryTransformBuffer_,
             &instanceMetadataBuffer_, &primitiveMetadataBuffer_, &materialMetadataBuffer_,
             &skinnedPlayerBlasUpdateScratch_, &viewmodelBlasUpdateScratch_,
             &rescueWorldUpdateScratch_, &waterDropletBlasUpdateScratch_,
             &tlas_.backing, &tlasUpdateScratch_})
    {
        AccumulateRtGpuBuffer(inventory, *buffer);
    }
    const auto accumulateBlas = [&inventory](const AccelerationStructure& blas) {
        AccumulateRtGpuBuffer(inventory, blas.backing);
        if (blas.handle != VK_NULL_HANDLE)
        {
            AccumulateRtResourceCount(inventory.bottomLevelAccelerationStructureCount);
        }
    };
    for (const AccelerationStructure* blas : std::array{
             &blas_, &waterfallBlas_, &finaleRoofBlas_, &torchBlas_, &worldTorchBodyBlas_, &swordBlas_,
             &playerSwordScabbardBlas_,
             &gothicChestBaseBlas_, &gothicChestLidBlas_, &rewardLanternRingBlas_,
             &rewardLanternBodyBlas_, &dielectricFixtureBlas_, &playerBodyBlas_,
             &playerLimbBlas_, &skinnedPlayerBlas_, &viewmodelBlas_, &collapseBlas_,
             &waterDropletBlas_})
    {
        accumulateBlas(*blas);
    }
    if (tlas_.handle != VK_NULL_HANDLE)
    {
        AccumulateRtResourceCount(inventory.topLevelAccelerationStructureCount);
        inventory.tlasInstanceCount = tlasInstanceCount_;
    }
    characterSlot_.AccumulateResourceInventory(inventory);
    pipelineBundle_.AccumulateResourceInventory(inventory);
#ifdef HORDE_RT_STAGED_PRIMARY_EXPERIMENT
    if (stagedPrimary_) stagedPrimary_->AccumulateResourceInventory(inventory);
#endif

    AccumulateRtMemoryAllocation(
        inventory, storageImageMemory_, storageImageAllocationSize_,
        storageImageMemoryPropertyFlags_);
    for (const TextureArray* texture : std::array<const TextureArray*, 10u>{
             &environmentTexture_, &materialDiffuse_, &materialNormal_, &materialArm_, &lichBaseColor_,
             &lichEmissive_, &staticBaseColor_, &staticNormal_, &staticOrm_,
             &staticEmissive_})
    {
        AccumulateRtMemoryAllocation(
            inventory, texture->memory, texture->allocationSize,
            texture->memoryPropertyFlags);
    }
    return inventory;
}

bool PresentableTinyRtScene::CollectCompletedDiagnostic(
    RtDiagnosticCounterPayload& payload,
    std::string& diagnostic)
{
    payload = {};
    static_assert(kRtDielectricDiagnosticsSchema1FieldCount ==
                  horde::telemetry::kRtDielectricCounterCount);
    static_assert(offsetof(RtDielectricDiagnostics, primaryRewardBodyPixelCount) +
                      sizeof(std::uint32_t) ==
                  horde::telemetry::kRtDielectricCounterCount *
                      sizeof(std::uint32_t));
    static_assert(offsetof(RtDielectricDiagnostics, primaryPlayerPixelCount) /
                      sizeof(std::uint32_t) ==
                  horde::telemetry::kRtPrimaryPlayerPixelCounterIndex);
    if (pipelineBundle_.DiagnosticAvailability() !=
            RtDiagnosticAvailability::Available ||
        pipelineBundle_.diagnosticBuffer.memory == VK_NULL_HANDLE)
    {
        diagnostic = "The completed RT Diagnostic buffer is unavailable.";
        return false;
    }
    RtDielectricDiagnostics completed{};
    if (!ReadBuffer(pipelineBundle_.diagnosticBuffer, 0u,
                    &completed, sizeof(completed),
                    "dielectric diagnostics", diagnostic))
    {
        return false;
    }
    std::memcpy(payload.counters.data(), &completed,
                payload.counters.size() * sizeof(payload.counters[0]));
    diagnostic.clear();
    return true;
}

void PresentableTinyRtScene::PublishCompletedDiagnostic(
    const RtDiagnosticCounterPayload& payload) noexcept
{
    using CounterMember = std::uint32_t PresentableTinyRtScene::*;
    static constexpr std::array<CounterMember,
                                horde::telemetry::kRtDielectricCounterCount>
        members{{
            &PresentableTinyRtScene::dielectricTransportOverflowCount_,
            &PresentableTinyRtScene::dielectricShadowOverflowCount_,
            &PresentableTinyRtScene::dielectricSecondaryRejectCount_,
            &PresentableTinyRtScene::dielectricUnclosedVolumeCount_,
            &PresentableTinyRtScene::dielectricPrimaryUnclosedVolumeCount_,
            &PresentableTinyRtScene::dielectricShadowUnclosedVolumeCount_,
            &PresentableTinyRtScene::productionPaneStackFailureCount_,
            &PresentableTinyRtScene::productionPaneSecondaryOriginCount_,
            &PresentableTinyRtScene::productionPaneSecondaryTerminalCount_,
            &PresentableTinyRtScene::productionPaneSecondarySameMediumCount_,
            &PresentableTinyRtScene::productionPaneSecondaryDifferentMediumCount_,
            &PresentableTinyRtScene::secondaryNearSelfHitCount_,
            &PresentableTinyRtScene::primaryOpenMissCount_,
            &PresentableTinyRtScene::primaryOpenOpaqueCount_,
            &PresentableTinyRtScene::primaryMismatchedExitCount_,
            &PresentableTinyRtScene::primaryInterfaceBudgetCount_,
            &PresentableTinyRtScene::primaryVolumeBudgetCount_,
            &PresentableTinyRtScene::shadowOpenMissCount_,
            &PresentableTinyRtScene::shadowMismatchedExitCount_,
            &PresentableTinyRtScene::primaryTirCount_,
            &PresentableTinyRtScene::primaryInterfaceBudgetOpenVolumeCount_,
            &PresentableTinyRtScene::primaryInterfaceBudgetClosedVolumeCount_,
            &PresentableTinyRtScene::shadowMismatchEmptyCount_,
            &PresentableTinyRtScene::shadowImplicitOriginExitCount_,
            &PresentableTinyRtScene::secondaryDielectricTerminalCount_,
            &PresentableTinyRtScene::primaryTirTerminationCount_,
            &PresentableTinyRtScene::shadowFiniteEndpointVolumeCount_,
            &PresentableTinyRtScene::primaryOpenOpaqueSameInstanceDifferentMaterialCount_,
            &PresentableTinyRtScene::primaryOpenOpaqueAfterTirCount_,
            &PresentableTinyRtScene::primaryOpenOpaqueTerminalInstanceMask_,
            &PresentableTinyRtScene::primaryOpenOpaqueVolumeInstanceMask_,
            &PresentableTinyRtScene::primaryOpenOpaqueTerminalMaterialMask_,
            &PresentableTinyRtScene::primaryClosedVolumeAbsorptionCount_,
            &PresentableTinyRtScene::primaryCertifiedClosedVolumeRecoveryCount_,
            &PresentableTinyRtScene::shadowCertifiedClosedVolumeRecoveryCount_,
            &PresentableTinyRtScene::certifiedClosedVolumeRecoveryReasonMask_,
            &PresentableTinyRtScene::primaryTorchPixelCount_,
            &PresentableTinyRtScene::primarySwordPixelCount_,
            &PresentableTinyRtScene::primaryPlayerPixelCount_,
            &PresentableTinyRtScene::primaryRewardRingPixelCount_,
            &PresentableTinyRtScene::primaryRewardBodyPixelCount_,
        }};
    for (std::size_t index = 0u; index < members.size(); ++index)
    {
        this->*members[index] = payload.counters[index];
    }
}

RtDiagnosticFrameIo MakeRtDiagnosticFrameIo(
    PresentableTinyRtScene& scene) noexcept
{
    return {
        &scene,
        [](void* user, RtDiagnosticCounterPayload& output,
           std::string& diagnostic) {
            return static_cast<PresentableTinyRtScene*>(user)
                ->CollectCompletedDiagnostic(output, diagnostic);
        },
        [](void* user, const RtDiagnosticCounterPayload& value) noexcept {
            static_cast<PresentableTinyRtScene*>(user)
                ->PublishCompletedDiagnostic(value);
        }};
}

bool PresentableTinyRtScene::LoadEntryPoints(std::string& diagnostic)
{
    vkCreateAccelerationStructureKHR_ = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device_, "vkCreateAccelerationStructureKHR"));
    vkDestroyAccelerationStructureKHR_ = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device_, "vkDestroyAccelerationStructureKHR"));
    vkGetAccelerationStructureBuildSizesKHR_ = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device_, "vkGetAccelerationStructureBuildSizesKHR"));
    vkGetAccelerationStructureDeviceAddressKHR_ = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device_, "vkGetAccelerationStructureDeviceAddressKHR"));
    vkCmdBuildAccelerationStructuresKHR_ = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device_, "vkCmdBuildAccelerationStructuresKHR"));
    if (executionPolicy_.requiresShaderBindingTable)
    {
        vkCreateRayTracingPipelinesKHR_ = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device_, "vkCreateRayTracingPipelinesKHR"));
        vkGetRayTracingShaderGroupHandlesKHR_ = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device_, "vkGetRayTracingShaderGroupHandlesKHR"));
        vkCmdTraceRaysKHR_ = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device_, "vkCmdTraceRaysKHR"));
    }
    else
    {
        vkCreateRayTracingPipelinesKHR_ = nullptr;
        vkGetRayTracingShaderGroupHandlesKHR_ = nullptr;
        vkCmdTraceRaysKHR_ = nullptr;
    }
    vkGetBufferDeviceAddressKHR_ = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device_, "vkGetBufferDeviceAddressKHR"));
    if (!vkGetBufferDeviceAddressKHR_)
    {
        vkGetBufferDeviceAddressKHR_ = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(
            vkGetDeviceProcAddr(device_, "vkGetBufferDeviceAddress"));
    }

    if (!vkCreateAccelerationStructureKHR_ || !vkDestroyAccelerationStructureKHR_ ||
        !vkGetAccelerationStructureBuildSizesKHR_ || !vkGetAccelerationStructureDeviceAddressKHR_ ||
        !vkCmdBuildAccelerationStructuresKHR_ || !vkGetBufferDeviceAddressKHR_ ||
        (executionPolicy_.requiresShaderBindingTable &&
         (!vkCreateRayTracingPipelinesKHR_ || !vkGetRayTracingShaderGroupHandlesKHR_ ||
          !vkCmdTraceRaysKHR_)))
    {
        diagnostic = "Required Vulkan RT entry points are unavailable.";
        return false;
    }

    diagnostic.clear();
    return true;
}

bool PresentableTinyRtScene::CreateBuffer(VkDeviceSize size,
                                          VkBufferUsageFlags usage,
                                          VkMemoryPropertyFlags memoryFlags,
                                          bool deviceAddress,
                                          Buffer& out,
                                          std::string& diagnostic,
                                          VkMemoryPropertyFlags preferredMemoryFlags) const
{
    return gpuResources_.CreateBuffer(size, usage, memoryFlags, deviceAddress, out,
                                      diagnostic, preferredMemoryFlags);
}

void PresentableTinyRtScene::DestroyBuffer(Buffer& buffer) const
{
    gpuResources_.DestroyBuffer(buffer);
}

bool PresentableTinyRtScene::WriteBuffer(const Buffer& buffer,
                                         const void* data,
                                         const VkDeviceSize size,
                                         const char* label,
                                         std::string& diagnostic,
                                         RtSceneRecordObservation* observation) const
{
    return gpuResources_.WriteBuffer(
        buffer, data, size, label, diagnostic, observation);
}

bool PresentableTinyRtScene::ReadBuffer(const Buffer& buffer,
                                        const VkDeviceSize offset,
                                        void* data,
                                        const VkDeviceSize size,
                                        const char* label,
                                        std::string& diagnostic) const
{
    if (buffer.memory == VK_NULL_HANDLE || data == nullptr || size == 0u ||
        offset > buffer.size || size > buffer.size - offset ||
        offset > std::numeric_limits<std::size_t>::max() ||
        size > std::numeric_limits<std::size_t>::max())
    {
        diagnostic = std::string("Invalid ") + label + " readback.";
        return false;
    }
    // Persistent mappings belong to the resource owner. Reading coherent
    // uploaded inputs must neither remap nor release that owner's mapping.
    if (buffer.mappedWriteData != nullptr)
    {
        constexpr auto required = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if ((buffer.memoryPropertyFlags & required) != required)
        {
            diagnostic = std::string("Invalid coherent ") + label + " mapping for readback.";
            return false;
        }
        std::memcpy(data, static_cast<const std::uint8_t*>(buffer.mappedWriteData) +
                            static_cast<std::size_t>(offset),
                    static_cast<std::size_t>(size));
        diagnostic.clear();
        return true;
    }
    void* mapped = nullptr;
    if (vkMapMemory(device_, buffer.memory, 0u, buffer.size, 0u, &mapped) != VK_SUCCESS ||
        mapped == nullptr)
    {
        diagnostic = std::string("Failed to map ") + label + " memory for readback.";
        return false;
    }
    std::memcpy(data, static_cast<const std::uint8_t*>(mapped) + offset,
                static_cast<std::size_t>(size));
    vkUnmapMemory(device_, buffer.memory);
    diagnostic.clear();
    return true;
}

VkDeviceAddress PresentableTinyRtScene::BufferAddress(VkBuffer buffer) const
{
    return gpuResources_.BufferAddress(buffer);
}

bool PresentableTinyRtScene::RunOneTimeCommands(void (*record)(VkCommandBuffer, void*), void* userData, std::string& diagnostic) const
{
    VkCommandBufferAllocateInfo allocateInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        commandPool_,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1u};
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(device_, &allocateInfo, &commandBuffer) != VK_SUCCESS)
    {
        diagnostic = "Failed to allocate one-time RT command buffer.";
        return false;
    }

    VkCommandBufferBeginInfo beginInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        nullptr};
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        vkFreeCommandBuffers(device_, commandPool_, 1u, &commandBuffer);
        diagnostic = "Failed to begin one-time RT command buffer.";
        return false;
    }

    record(commandBuffer, userData);

    VkResult result = vkEndCommandBuffer(commandBuffer);
    if (result == VK_SUCCESS)
    {
        const VkSubmitInfo submitInfo{
            VK_STRUCTURE_TYPE_SUBMIT_INFO,
            nullptr,
            0u,
            nullptr,
            nullptr,
            1u,
            &commandBuffer,
            0u,
            nullptr};
        result = vkQueueSubmit(queue_, 1u, &submitInfo, VK_NULL_HANDLE);
    }
    if (result == VK_SUCCESS)
    {
        result = vkQueueWaitIdle(queue_);
    }

    vkFreeCommandBuffers(device_, commandPool_, 1u, &commandBuffer);
    if (result != VK_SUCCESS)
    {
        diagnostic = "Failed to submit one-time RT command buffer.";
        return false;
    }

    diagnostic.clear();
    return true;
}

bool PresentableTinyRtScene::CreateStorageImage(std::string& diagnostic)
{
    OutputImageResources output{};
    if (!CreateOutputImage(dispatchExtent_, output, diagnostic))
    {
        DestroyOutputImage(output);
        return false;
    }
    AdoptOutputImage(output);
    storageImageFrameRecorded_ = false;
    return true;
}

PresentableTinyRtScene::OutputImageResources PresentableTinyRtScene::OutputImage() const noexcept
{
    return {storageImage_, storageImageMemory_, storageImageView_,
            storageImageAllocationSize_, storageImageMemoryPropertyFlags_, storageImageLayout_};
}

void PresentableTinyRtScene::AdoptOutputImage(const OutputImageResources& image) noexcept
{
    storageImage_ = image.image;
    storageImageMemory_ = image.memory;
    storageImageView_ = image.view;
    storageImageAllocationSize_ = image.allocationSize;
    storageImageMemoryPropertyFlags_ = image.memoryFlags;
    storageImageLayout_ = image.layout;
}

void PresentableTinyRtScene::DestroyOutputImage(OutputImageResources& image) const noexcept
{
    if (image.view != VK_NULL_HANDLE) vkDestroyImageView(device_, image.view, nullptr);
    if (image.image != VK_NULL_HANDLE) vkDestroyImage(device_, image.image, nullptr);
    if (image.memory != VK_NULL_HANDLE) vkFreeMemory(device_, image.memory, nullptr);
    image = {};
}

bool PresentableTinyRtScene::ResizeOutputAfterDeviceIdle(VkExtent2D extent, std::string& diagnostic)
{
    if (!ready_ || device_ == VK_NULL_HANDLE || extent.width == 0u || extent.height == 0u)
    {
        diagnostic = "Cannot resize an unready RT output or use a zero extent.";
        return false;
    }
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayProperties{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
    VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    if (ExecutionBackend() == RtExecutionBackend::RayTracingPipeline)
        properties.pNext = &rayProperties;
    // Android minSdk24 cannot link the Vulkan1.1 symbol directly. Resolve it
    // through the same loader route used by the existing pipeline/SBT setup.
    auto getProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
        vkGetInstanceProcAddr(instance_, "vkGetPhysicalDeviceProperties2"));
    if (getProperties == nullptr)
        getProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
            vkGetInstanceProcAddr(instance_, "vkGetPhysicalDeviceProperties2KHR"));
    if (getProperties == nullptr)
    {
        diagnostic = "Vulkan properties2 entry point is unavailable for RT output resize.";
        return false;
    }
    getProperties(physicalDevice_, &properties);
    if (extent.width > properties.properties.limits.maxImageDimension2D ||
        extent.height > properties.properties.limits.maxImageDimension2D ||
        (ExecutionBackend() == RtExecutionBackend::RayTracingPipeline &&
         static_cast<std::uint64_t>(extent.width) * extent.height >
             rayProperties.maxRayDispatchInvocationCount))
    {
        diagnostic = "Requested RT output exceeds the device image/dispatch limits.";
        return false;
    }
    auto groups = computeDispatchGroups_;
    if (ExecutionBackend() == RtExecutionBackend::RayQueryCompute)
    {
        const auto selected = TryMakeRtComputeDispatch(extent, properties.properties.limits);
        if (!selected)
        {
            diagnostic = "Device compute limits cannot execute the requested RT output.";
            return false;
        }
        groups = *selected;
    }
    OutputResizeApi api{};
    api.user = this;
    api.create = [](void* user, VkExtent2D size, OutputImageResources& image, std::string& failure) {
        return static_cast<PresentableTinyRtScene*>(user)->CreateOutputImage(size, image, failure);
    };
    api.writeDescriptor = [](void* user, VkImageView view) noexcept {
        auto& scene = *static_cast<PresentableTinyRtScene*>(user);
        const VkDescriptorImageInfo info{VK_NULL_HANDLE, view, VK_IMAGE_LAYOUT_GENERAL};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = scene.pipelineBundle_.descriptorSet;
        write.dstBinding = 1u;
        write.descriptorCount = 1u;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        write.pImageInfo = &info;
        vkUpdateDescriptorSets(scene.device_, 1u, &write, 0u, nullptr);
    };
    api.destroy = [](void* user, OutputImageResources& image) noexcept {
        static_cast<PresentableTinyRtScene*>(user)->DestroyOutputImage(image);
    };
#ifdef HORDE_RT_STAGED_PRIMARY_EXPERIMENT
    if (stagedPrimary_ && !stagedPrimary_->PrepareResizeAfterDeviceIdle(extent, diagnostic)) return false;
#endif
    const bool resized = ResizeOutputWithApi(extent, groups, api, diagnostic);
#ifdef HORDE_RT_STAGED_PRIMARY_EXPERIMENT
    if (stagedPrimary_) {
        if (resized) stagedPrimary_->CommitResizeAfterDeviceIdle();
        else stagedPrimary_->CancelResizeAfterDeviceIdle();
    }
#endif
    return resized;
}

bool PresentableTinyRtScene::ResizeOutputWithApi(VkExtent2D extent, std::array<std::uint32_t, 3u> groups,
                                               const OutputResizeApi& api, std::string& diagnostic)
{
    if (!ready_ || device_ == VK_NULL_HANDLE || storageImage_ == VK_NULL_HANDLE ||
        pipelineBundle_.descriptorSet == VK_NULL_HANDLE || extent.width == 0u || extent.height == 0u ||
        api.create == nullptr || api.writeDescriptor == nullptr || api.destroy == nullptr)
    {
        diagnostic = "Invalid RT output resize transaction.";
        return false;
    }
    if (extent.width == dispatchExtent_.width && extent.height == dispatchExtent_.height)
    {
        diagnostic.clear();
        return true;
    }
    OutputImageResources replacement{};
    if (!api.create(api.user, extent, replacement, diagnostic))
    {
        api.destroy(api.user, replacement);
        return false;
    }
    auto old = OutputImage();
    api.writeDescriptor(api.user, replacement.view);
    AdoptOutputImage(replacement);
    dispatchExtent_ = extent;
    computeDispatchGroups_ = groups;
    // No recorded frame identifies the newly allocated, as-yet unpresented output.
    framePipelineEvidence_ = {};
    framePipelineEvidenceValid_ = false;
    storageImageFrameRecorded_ = false;
    lastOutputRedBlueSwapApplied_ = false;
    api.destroy(api.user, old);
    diagnostic.clear();
    return true;
}

bool PresentableTinyRtScene::CreateOutputImage(VkExtent2D extent, OutputImageResources& out,
                                              std::string& diagnostic)
{
    const VkImageCreateInfo imageInfo{
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr,
        0u,
        VK_IMAGE_TYPE_2D,
        kStorageImageFormat,
        {extent.width, extent.height, 1u},
        1u,
        1u,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0u,
        nullptr,
        VK_IMAGE_LAYOUT_UNDEFINED};
    if (vkCreateImage(device_, &imageInfo, nullptr, &out.image) != VK_SUCCESS)
    {
        diagnostic = "Failed to create RT storage image.";
        return false;
    }

    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device_, out.image, &requirements);
    VkMemoryPropertyFlags selectedMemoryFlags = 0u;
    const std::uint32_t memoryType = gpuResources_.FindMemoryType(
        requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &selectedMemoryFlags);
    if (memoryType == UINT32_MAX)
    {
        diagnostic = "No compatible memory type for RT storage image.";
        return false;
    }

    const VkMemoryAllocateInfo allocateInfo{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        requirements.size,
        memoryType};
    if (vkAllocateMemory(device_, &allocateInfo, nullptr, &out.memory) != VK_SUCCESS ||
        vkBindImageMemory(device_, out.image, out.memory, 0u) != VK_SUCCESS)
    {
        diagnostic = "Failed to allocate RT storage image memory.";
        return false;
    }
    out.allocationSize = requirements.size;
    out.memoryFlags = selectedMemoryFlags;

    const VkImageViewCreateInfo viewInfo{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0u,
        out.image,
        VK_IMAGE_VIEW_TYPE_2D,
        kStorageImageFormat,
        {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u}};
    if (vkCreateImageView(device_, &viewInfo, nullptr, &out.view) != VK_SUCCESS)
    {
        diagnostic = "Failed to create RT storage image view.";
        return false;
    }

    struct TransitionData
    {
        VkImage image;
        VkPipelineStageFlags shaderStage;
    } data{out.image, executionPolicy_.shaderPipelineStage};
    const auto record = [](VkCommandBuffer commandBuffer, void* userData) {
        const auto* transition = static_cast<const TransitionData*>(userData);
        SetImageBarrier(commandBuffer,
                        transition->image,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_GENERAL,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        transition->shaderStage,
                        0u,
                        VK_ACCESS_SHADER_WRITE_BIT);
    };
    if (!RunOneTimeCommands(record, &data, diagnostic))
    {
        return false;
    }

    out.layout = VK_IMAGE_LAYOUT_GENERAL;
    diagnostic.clear();
    return true;
}

void PresentableTinyRtScene::DestroyTextureArray(TextureArray& texture)
{
    if (device_ != VK_NULL_HANDLE && texture.view != VK_NULL_HANDLE) vkDestroyImageView(device_, texture.view, nullptr);
    if (device_ != VK_NULL_HANDLE && texture.image != VK_NULL_HANDLE) vkDestroyImage(device_, texture.image, nullptr);
    if (device_ != VK_NULL_HANDLE && texture.memory != VK_NULL_HANDLE) vkFreeMemory(device_, texture.memory, nullptr);
    texture = {};
}

bool PresentableTinyRtScene::CreateTextureArray(const std::string& path, VkFormat format, TextureArray& texture, std::string& diagnostic)
{
    return CreateTexture(path, format, 512u, 512u, 5u, texture, diagnostic);
}

bool PresentableTinyRtScene::CreateTexture(const std::string& path,
                                           VkFormat format,
                                           std::uint32_t width,
                                           std::uint32_t height,
                                           std::uint32_t layers,
                                           TextureArray& texture,
                                           std::string& diagnostic,
                                           const std::span<const std::uint32_t> sourceLayers,
                                           const VkImageViewType viewType)
{
    if ((viewType != VK_IMAGE_VIEW_TYPE_2D_ARRAY && viewType != VK_IMAGE_VIEW_TYPE_2D) ||
        (viewType == VK_IMAGE_VIEW_TYPE_2D && layers != 1u))
    {
        diagnostic = "Sampled texture view type disagrees with its layer count.";
        return false;
    }
    const bool ktx2 = path.ends_with(".ktx2");
    const bool astc4 = format == VK_FORMAT_ASTC_4x4_UNORM_BLOCK || format == VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
    const bool astc6 = format == VK_FORMAT_ASTC_6x6_UNORM_BLOCK || format == VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
    const VkDeviceSize layerByteSize = astc4
        ? static_cast<VkDeviceSize>((width + 3u) / 4u) * ((height + 3u) / 4u) * 16u
        : (astc6
               ? static_cast<VkDeviceSize>((width + 5u) / 6u) * ((height + 5u) / 6u) * 16u
               : static_cast<VkDeviceSize>(width) * height * 4u);
    const VkDeviceSize byteSize = layerByteSize * layers;
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
    {
        diagnostic = "PBR texture array is missing: " + path;
        return false;
    }
    const std::size_t fileSize = static_cast<std::size_t>(stream.tellg());
    stream.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> fileBytes(fileSize);
    if (!stream.read(reinterpret_cast<char*>(fileBytes.data()), static_cast<std::streamsize>(fileBytes.size())))
    {
        diagnostic = "Failed to read PBR texture array: " + path;
        return false;
    }
    struct MipPayload
    {
        VkDeviceSize bufferOffset = 0u;
        VkDeviceSize layerByteSize = 0u;
        std::uint32_t width = 0u;
        std::uint32_t height = 0u;
    };
    std::vector<MipPayload> mipPayloads;
    std::vector<std::uint8_t> pixels;
    std::uint32_t mipLevels = 1u;
    if (ktx2)
    {
        constexpr std::array<std::uint8_t, 12u> identifier{{0xABu, 0x4Bu, 0x54u, 0x58u, 0x20u, 0x32u, 0x30u, 0xBBu, 0x0Du, 0x0Au, 0x1Au, 0x0Au}};
        const auto read32 = [&fileBytes](std::size_t offset) {
            std::uint32_t value = 0u;
            if (offset + sizeof(value) <= fileBytes.size()) std::memcpy(&value, fileBytes.data() + offset, sizeof(value));
            return value;
        };
        const auto read64 = [&fileBytes](std::size_t offset) {
            std::uint64_t value = 0u;
            if (offset + sizeof(value) <= fileBytes.size()) std::memcpy(&value, fileBytes.data() + offset, sizeof(value));
            return value;
        };
        mipLevels = read32(40u);
        const auto sourceLayerCount = std::max(read32(32u), 1u);
        const bool subset = !sourceLayers.empty();
        const bool validHeader = mipLevels > 0u && mipLevels <= 16u &&
                                 fileBytes.size() >= 80u + static_cast<std::size_t>(mipLevels) * 24u &&
                                 std::equal(identifier.begin(), identifier.end(), fileBytes.begin()) &&
                                 read32(12u) == static_cast<std::uint32_t>(format) && read32(16u) == 1u &&
                                 read32(20u) == width && read32(24u) == height && read32(28u) == 0u &&
                                 (subset ? sourceLayers.size() == layers && sourceLayerCount <= kRtTextureLayerCapacity
                                         : read32(32u) == layers || (layers == 1u && read32(32u) == 0u)) &&
                                 read32(36u) == 1u &&
                                 read32(44u) == 0u;
        if (!validHeader)
        {
            diagnostic = "PBR KTX2 array has an unsupported header, format, or payload size: " + path;
            return false;
        }
        for (std::uint32_t level = 0u; level < mipLevels; ++level)
        {
            const std::size_t entry = 80u + static_cast<std::size_t>(level) * 24u;
            const std::uint64_t levelOffset = read64(entry);
            const std::uint64_t levelLength = read64(entry + 8u);
            const std::uint64_t levelUncompressedLength = read64(entry + 16u);
            const std::uint32_t mipWidth = std::max(width >> level, 1u);
            const std::uint32_t mipHeight = std::max(height >> level, 1u);
            const VkDeviceSize mipLayerBytes = astc4
                ? static_cast<VkDeviceSize>((mipWidth + 3u) / 4u) * ((mipHeight + 3u) / 4u) * 16u
                : (astc6
                       ? static_cast<VkDeviceSize>((mipWidth + 5u) / 6u) * ((mipHeight + 5u) / 6u) * 16u
                       : static_cast<VkDeviceSize>(mipWidth) * mipHeight * 4u);
            const VkDeviceSize expectedLevelBytes = mipLayerBytes * sourceLayerCount;
            if (levelLength != expectedLevelBytes || levelUncompressedLength != expectedLevelBytes ||
                levelOffset > fileBytes.size() || levelLength > fileBytes.size() - static_cast<std::size_t>(levelOffset))
            {
                diagnostic = "PBR KTX2 array has an unsupported mip payload size: " + path;
                return false;
            }
            const VkDeviceSize destinationOffset = pixels.size();
            if (subset)
            {
                if (!AppendRtTextureLayerSubset(
                        std::span<const std::uint8_t>(fileBytes).subspan(
                            static_cast<std::size_t>(levelOffset), static_cast<std::size_t>(levelLength)),
                        sourceLayerCount, mipLayerBytes, sourceLayers, pixels))
                {
                    diagnostic = "PBR KTX2 selected layers exceed the validated source mip range: " + path;
                    return false;
                }
            }
            else pixels.insert(pixels.end(),
                          fileBytes.begin() + static_cast<std::ptrdiff_t>(levelOffset),
                          fileBytes.begin() + static_cast<std::ptrdiff_t>(levelOffset + levelLength));
            mipPayloads.push_back({destinationOffset, mipLayerBytes, mipWidth, mipHeight});
        }
    }
    else
    {
        if (!sourceLayers.empty() || fileBytes.size() != static_cast<std::size_t>(byteSize))
        {
            diagnostic = "Raw PBR texture array has the wrong size: " + path;
            return false;
        }
        pixels = std::move(fileBytes);
        mipPayloads.push_back({0u, layerByteSize, width, height});
    }

    Buffer staging;
    const VkDeviceSize uploadByteSize = pixels.size();
    if (!CreateBuffer(uploadByteSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false, staging, diagnostic)) return false;
    if (!WriteBuffer(staging, pixels.data(), uploadByteSize, "PBR texture staging", diagnostic))
    {
        DestroyBuffer(staging);
        return false;
    }

    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent = {width, height, 1u};
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = layers;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device_, &imageInfo, nullptr, &texture.image) != VK_SUCCESS)
    {
        DestroyBuffer(staging);
        diagnostic = "Failed to create PBR texture array image.";
        return false;
    }
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device_, texture.image, &requirements);
    VkMemoryPropertyFlags selectedMemoryFlags = 0u;
    const std::uint32_t memoryType = gpuResources_.FindMemoryType(
        requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &selectedMemoryFlags);
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = memoryType;
    if (memoryType == UINT32_MAX || vkAllocateMemory(device_, &allocation, nullptr, &texture.memory) != VK_SUCCESS || vkBindImageMemory(device_, texture.image, texture.memory, 0u) != VK_SUCCESS)
    {
        DestroyBuffer(staging);
        DestroyTextureArray(texture);
        diagnostic = "Failed to allocate PBR texture array memory.";
        return false;
    }
    texture.allocationSize = requirements.size;
    texture.memoryPropertyFlags = selectedMemoryFlags;
    struct UploadData
    {
        PresentableTinyRtScene* scene;
        Buffer* staging;
        TextureArray* texture;
        const std::vector<MipPayload>* mipPayloads;
        std::uint32_t mipLevels;
        std::uint32_t layers;
    } upload{this, &staging, &texture, &mipPayloads, mipLevels, layers};
    const auto recordUpload = [](VkCommandBuffer commandBuffer, void* userData) {
        auto* data = static_cast<UploadData*>(userData);
        VkImageMemoryBarrier toTransfer{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer.image = data->texture->image;
        toTransfer.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, data->mipLevels, 0u, data->layers};
        toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, nullptr, 0u, nullptr, 1u, &toTransfer);
        std::vector<VkBufferImageCopy> copies;
        copies.reserve(static_cast<std::size_t>(data->mipLevels) * data->layers);
        for (std::uint32_t level = 0u; level < data->mipLevels; ++level)
        {
            const MipPayload& mip = (*data->mipPayloads)[level];
            for (std::uint32_t layer = 0u; layer < data->layers; ++layer)
            {
                VkBufferImageCopy copy{};
                copy.bufferOffset = mip.bufferOffset + static_cast<VkDeviceSize>(layer) * mip.layerByteSize;
                copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, layer, 1u};
                copy.imageExtent = {mip.width, mip.height, 1u};
                copies.push_back(copy);
            }
        }
        vkCmdCopyBufferToImage(commandBuffer, data->staging->buffer, data->texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<std::uint32_t>(copies.size()), copies.data());
        VkImageMemoryBarrier toShader = toTransfer;
        toShader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toShader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        toShader.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toShader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, data->scene->executionPolicy_.shaderPipelineStage, 0u, 0u, nullptr, 0u, nullptr, 1u, &toShader);
    };
    const bool uploaded = RunOneTimeCommands(recordUpload, &upload, diagnostic);
    DestroyBuffer(staging);
    if (!uploaded) { DestroyTextureArray(texture); return false; }

    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = texture.image;
    viewInfo.viewType = viewType;
    viewInfo.format = format;
    viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, mipLevels, 0u, layers};
    if (vkCreateImageView(device_, &viewInfo, nullptr, &texture.view) != VK_SUCCESS)
    {
        DestroyTextureArray(texture);
        diagnostic = "Failed to create PBR texture array view.";
        return false;
    }
    texture.mipLevels = mipLevels;
    return true;
}

bool PresentableTinyRtScene::SupportsTextureArrayFormat(VkFormat format) const
{
    VkFormatProperties formatProperties{};
    vkGetPhysicalDeviceFormatProperties(physicalDevice_, format, &formatProperties);
    constexpr VkFormatFeatureFlags required = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
                                               VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
                                               VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    if ((formatProperties.optimalTilingFeatures & required) != required)
    {
        return false;
    }
    VkImageFormatProperties imageProperties{};
    const VkResult result = vkGetPhysicalDeviceImageFormatProperties(physicalDevice_,
                                                                      format,
                                                                      VK_IMAGE_TYPE_2D,
                                                                      VK_IMAGE_TILING_OPTIMAL,
                                                                      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                                      0u,
                                                                      &imageProperties);
    return result == VK_SUCCESS && imageProperties.maxExtent.width >= 512u &&
           imageProperties.maxExtent.height >= 512u && imageProperties.maxArrayLayers >= 5u;
}

bool PresentableTinyRtScene::CreateMaterialTextures(const std::string& directory, std::string& diagnostic)
{
    const auto path = [&directory](const char* name) { return directory + "/" + name; };
    const bool astcSupported = SupportsTextureArrayFormat(VK_FORMAT_ASTC_6x6_SRGB_BLOCK) &&
                               SupportsTextureArrayFormat(VK_FORMAT_ASTC_4x4_UNORM_BLOCK) &&
                               SupportsTextureArrayFormat(VK_FORMAT_ASTC_6x6_UNORM_BLOCK);
    const std::string compressedDiffuse = path("diff-array-512-astc6x6.ktx2");
    const std::string compressedNormal = path("normal-array-512-astc4x4.ktx2");
    const std::string compressedArm = path("arm-array-512-astc6x6.ktx2");
    const bool compressedAssetsPresent = std::ifstream(compressedDiffuse, std::ios::binary).good() &&
                                         std::ifstream(compressedNormal, std::ios::binary).good() &&
                                         std::ifstream(compressedArm, std::ios::binary).good();
    if (astcSupported && compressedAssetsPresent)
    {
        if (!CreateTextureArray(compressedDiffuse, VK_FORMAT_ASTC_6x6_SRGB_BLOCK, materialDiffuse_, diagnostic) ||
            !CreateTextureArray(compressedNormal, VK_FORMAT_ASTC_4x4_UNORM_BLOCK, materialNormal_, diagnostic) ||
            !CreateTextureArray(compressedArm, VK_FORMAT_ASTC_6x6_UNORM_BLOCK, materialArm_, diagnostic)) return false;
        materialEncoding_ = "ASTC 6x6 diffuse/ARM + ASTC 4x4 normal (KTX2)";
    }
    else
    {
        const std::string rawDiffuse = path("diff-array-512.rgba");
        const std::string rawNormal = path("normal-array-512.rgba");
        const std::string rawArm = path("arm-array-512.rgba");
        const bool rawAssetsPresent = std::ifstream(rawDiffuse, std::ios::binary).good() &&
                                      std::ifstream(rawNormal, std::ios::binary).good() &&
                                      std::ifstream(rawArm, std::ios::binary).good();
        if (!rawAssetsPresent)
        {
            diagnostic = astcSupported
                ? "ASTC texture arrays are missing and no raw fallback is packaged."
                : "ASTC LDR sampled-array linear filtering is unsupported and no raw mobile fallback is packaged.";
            return false;
        }
        if (!CreateTextureArray(rawDiffuse, VK_FORMAT_R8G8B8A8_SRGB, materialDiffuse_, diagnostic) ||
            !CreateTextureArray(rawNormal, VK_FORMAT_R8G8B8A8_UNORM, materialNormal_, diagnostic) ||
            !CreateTextureArray(rawArm, VK_FORMAT_R8G8B8A8_UNORM, materialArm_, diagnostic)) return false;
        materialEncoding_ = "RGBA8 raw fallback";
    }
    VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler.addressModeU = sampler.addressModeV = sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.maxAnisotropy = 1.0f;
    sampler.maxLod = VK_LOD_CLAMP_NONE;
    if (vkCreateSampler(device_, &sampler, nullptr, &materialSampler_) != VK_SUCCESS)
    {
        diagnostic = "Failed to create PBR material sampler.";
        return false;
    }
    return true;
}

bool PresentableTinyRtScene::CreateEnvironmentTexture(const std::string& productionRoot,
                                                        std::string& diagnostic)
{
#if defined(__ANDROID__)
    const auto path = (std::filesystem::path(productionRoot) / "night-storm.android.ktx2").string();
    constexpr VkFormat format = VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
#else
    const auto path = (std::filesystem::path(productionRoot) /
                       "textures/environment/runtime/night-storm.windows.ktx2").string();
    constexpr VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
#endif
    VkFormatProperties properties{};
    vkGetPhysicalDeviceFormatProperties(physicalDevice_, format, &properties);
    constexpr VkFormatFeatureFlags required = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    if ((properties.optimalTilingFeatures & required) != required)
    {
        diagnostic = "The packaged environment texture format lacks sampled/linear/transfer support.";
        return false;
    }
    if (!CreateTexture(path, format, 512u, 256u, 1u, environmentTexture_, diagnostic,
                       {}, VK_IMAGE_VIEW_TYPE_2D)) return false;
    if (environmentTexture_.mipLevels != 10u)
    {
        diagnostic = "The bounded 512x256 environment requires its complete ten-level mip chain.";
        return false;
    }
    VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler.magFilter = sampler.minFilter = VK_FILTER_LINEAR;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.addressModeV = sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.maxAnisotropy = 1.0f;
    sampler.maxLod = static_cast<float>(environmentTexture_.mipLevels - 1u);
    if (vkCreateSampler(device_, &sampler, nullptr, &environmentSampler_) != VK_SUCCESS)
    {
        diagnostic = "Failed to create the shared environment panorama sampler.";
        return false;
    }
    return true;
}

bool PresentableTinyRtScene::CreateLichTextures(const std::string& directory, std::string& diagnostic)
{
    const auto path = [&directory](const char* name) { return directory + "/" + name; };
#if defined(__ANDROID__)
    if (!SupportsTextureArrayFormat(VK_FORMAT_ASTC_6x6_SRGB_BLOCK))
    {
        diagnostic = "The Android lich path requires sampled ASTC 6x6 support; no uncompressed mobile fallback is allowed.";
        return false;
    }
    const std::string basePath = path("base-color-2048-astc6x6.ktx2");
    const std::string emissivePath = path("emissive-2048-astc6x6.ktx2");
    if (!CreateTexture(basePath, VK_FORMAT_ASTC_6x6_SRGB_BLOCK, 2048u, 2048u, 1u, lichBaseColor_, diagnostic) ||
        !CreateTexture(emissivePath, VK_FORMAT_ASTC_6x6_SRGB_BLOCK, 2048u, 2048u, 1u, lichEmissive_, diagnostic))
    {
        return false;
    }
    materialEncoding_ += " + strict ASTC 6x6 lich";
#else
    const std::string basePath = path("base-color-2048-rgba8.ktx2");
    const std::string emissivePath = path("emissive-2048-rgba8.ktx2");
    if (!CreateTexture(basePath, VK_FORMAT_R8G8B8A8_SRGB, 2048u, 2048u, 1u, lichBaseColor_, diagnostic) ||
        !CreateTexture(emissivePath, VK_FORMAT_R8G8B8A8_SRGB, 2048u, 2048u, 1u, lichEmissive_, diagnostic))
    {
        return false;
    }
    materialEncoding_ += " + raw RGBA8 KTX2 lich";
#endif
    return true;
}

bool PresentableTinyRtScene::LoadStaticHeldItemAssets(
    const std::string& developmentDirectory,
    const std::string& productionAssetRoot,
    std::string& diagnostic)
{
    (void)developmentDirectory;
    genericStaticAssetEnabled_ = !productionAssetRoot.empty();
    productionHeldItemAssetsEnabled_ = !productionAssetRoot.empty();
    developmentStaticAssetDirectory_.clear();
    staticTextureDirectory_.clear();
    developmentStaticAsset_ = {};
    collapseStaticAsset_ = {};
    productionTorchAsset_ = {};
    playerTorchAsset_ = {};
    playerSwordScabbardAsset_ = {};
    productionPlayerAsset_ = {};
    gothicChestBaseAsset_ = {};
    gothicChestLidAsset_ = {};
    rewardLanternRingAsset_ = {};
    rewardLanternBodyAsset_ = {};
    productionDielectricFixtureAsset_ = {};
    if (!productionHeldItemAssetsEnabled_)
    {
        diagnostic = "Production held-item asset root is required; procedural sword/torch bodies are retired.";
        return false;
    }

    if (sceneProfile_ != RtSceneProfile::Showcase)
        return LoadPreviewStaticAssets(productionAssetRoot, diagnostic);

    const std::filesystem::path root(productionAssetRoot);
    const auto swordDirectory = root / "models/weapons/runtime";
    const auto torchDirectory = root / "models/props/runtime";
    const auto playerTorchDirectory = root / "models/props/runtime/player-rag-torch";
    const auto playerSwordScabbardDirectory =
        root / "models/props/runtime/player-sword-scabbard";
    const auto playerDirectory = root / "models/player/runtime";
    const auto dielectricDirectory =
        root / "models/props/runtime/dielectric-fixture";
    const auto chestBaseDirectory =
        root / "models/props/runtime/gothic-chest-base";
    const auto chestLidDirectory =
        root / "models/props/runtime/gothic-chest-lid";
    const auto lanternRingDirectory =
        root / "models/props/runtime/reward-lantern-ring";
    const auto lanternBodyDirectory =
        root / "models/props/runtime/reward-lantern-body";
    const auto collapseDirectory = root / "models/world/runtime/collapsed-entry";
    horde::scene::assets::AssetManifest swordManifest;
    horde::scene::assets::AssetManifest torchManifest;
    horde::scene::assets::AssetManifest playerTorchManifest;
    horde::scene::assets::AssetManifest playerSwordScabbardManifest;
    horde::scene::assets::AssetManifest playerManifest;
    horde::scene::assets::AssetManifest dielectricManifest;
    horde::scene::assets::AssetManifest chestBaseManifest;
    horde::scene::assets::AssetManifest chestLidManifest;
    horde::scene::assets::AssetManifest lanternRingManifest;
    horde::scene::assets::AssetManifest lanternBodyManifest;
    horde::scene::assets::AssetManifest collapseManifest;
    if (!horde::scene::assets::AssetManifest::Load(
            swordDirectory / "asset.manifest.json", swordManifest, diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(
            swordDirectory / "gothic-arming-sword-rh-lod0.runtime.glb",
            swordManifest,
            developmentStaticAsset_,
            diagnostic) ||
        !horde::scene::assets::AssetManifest::Load(
            torchDirectory / "asset.manifest.json", torchManifest, diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(
            torchDirectory / "gothic-hand-torch-lod0.runtime.glb",
            torchManifest,
            productionTorchAsset_,
            diagnostic) ||
        !horde::scene::assets::AssetManifest::Load(
            playerTorchDirectory / "asset.manifest.json",
            playerTorchManifest, diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(
            playerTorchDirectory / "rag-torch-player-lod0.runtime.glb",
            playerTorchManifest, playerTorchAsset_, diagnostic) ||
        !horde::scene::assets::AssetManifest::Load(
            playerSwordScabbardDirectory / "asset.manifest.json",
            playerSwordScabbardManifest, diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(
            playerSwordScabbardDirectory / "player-sword-scabbard-lod0.runtime.glb",
            playerSwordScabbardManifest, playerSwordScabbardAsset_, diagnostic) ||
        !horde::scene::assets::AssetManifest::Load(
            playerDirectory / "asset.manifest.json", playerManifest, diagnostic) ||
        !playerManifest.ValidatePlayerSemantics(diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(
            playerDirectory / "gothic-traveller-lod0.runtime.glb",
            playerManifest, productionPlayerAsset_, diagnostic) ||
        !playerRenderSlot_.LoadAsset(
            (playerDirectory / "gothic-traveller-lod0.runtime.glb").string(), diagnostic) ||
        !horde::scene::assets::AssetManifest::Load(
            dielectricDirectory / "asset.manifest.json",
            dielectricManifest, diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(
            dielectricDirectory / "closed-glass-lod0.runtime.glb",
            dielectricManifest, productionDielectricFixtureAsset_, diagnostic) ||
        !horde::scene::assets::AssetManifest::Load(
            chestBaseDirectory / "asset.manifest.json", chestBaseManifest, diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(
            chestBaseDirectory / "gothic-chest-base-lod0.runtime.glb",
            chestBaseManifest, gothicChestBaseAsset_, diagnostic) ||
        !horde::scene::assets::AssetManifest::Load(
            chestLidDirectory / "asset.manifest.json", chestLidManifest, diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(
            chestLidDirectory / "gothic-chest-lid-lod0.runtime.glb",
            chestLidManifest, gothicChestLidAsset_, diagnostic) ||
        !horde::scene::assets::AssetManifest::Load(
            lanternRingDirectory / "asset.manifest.json", lanternRingManifest, diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(
            lanternRingDirectory / "reward-lantern-ring-lod0.runtime.glb",
            lanternRingManifest, rewardLanternRingAsset_, diagnostic) ||
        !horde::scene::assets::AssetManifest::Load(
            lanternBodyDirectory / "asset.manifest.json", lanternBodyManifest, diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(
            lanternBodyDirectory / "reward-lantern-body-lod0.runtime.glb",
            lanternBodyManifest, rewardLanternBodyAsset_, diagnostic) ||
        !horde::scene::assets::AssetManifest::Load(
            collapseDirectory / "asset.manifest.json", collapseManifest, diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(
            collapseDirectory / "collapsed-entry-lod0.runtime.glb",
            collapseManifest, collapseStaticAsset_, diagnostic))
        return false;
    if (sceneProfile_ == RtSceneProfile::Showcase && developmentRescueJourney_)
    {
        horde::scene::TombDressingBuildReport dressing;
        if (!horde::scene::AppendPreparedTombDressing(root, collapseStaticAsset_, dressing, diagnostic))
            return false;
        horde::scene::ForestDressingBuildReport forest;
        if (!horde::scene::AppendPreparedForestDressing(root, collapseStaticAsset_, forest, diagnostic))
            return false;
        // The collapse's final masonry range and the first dressing range use
        // the same material. Keep every triangle while sharing one metadata row;
        // High's physical lantern panes must fit alongside this resident scene.
        if (!horde::scene::assets::CoalesceAdjacentWorldBakedPrimitives(collapseStaticAsset_, diagnostic))
            return false;
    }
    // The selected immutable quality bundle owns the geometry profile too.
    // Mobile panes are absent from the BLAS, not hidden/skipped in a shader.
    if (!SelectLanternGeometryForQuality(
            rewardLanternBodyAsset_, pipelineBundle_.Request().quality, diagnostic, glassEnabled_))
        return false;
    if (sceneProfile_ == RtSceneProfile::Showcase)
        waterDropletAsset_ = MakeWaterDropletStaticAsset();
    staticTextureDirectory_ = (root / "textures/props/runtime").string();
    const auto viewmodelDirectory = root / "models/player/viewmodel/runtime";
    const auto viewmodelPath = viewmodelDirectory / "gothic-traveller-viewmodel.runtime.glb";
    viewmodelAvailable_ = std::filesystem::exists(viewmodelPath);
    if (viewmodelAvailable_)
    {
        horde::scene::assets::AssetManifest manifest;
        if (!horde::scene::assets::AssetManifest::Load(viewmodelDirectory / "asset.manifest.json", manifest, diagnostic) ||
            !manifest.ValidatePlayerViewmodelSemantics(diagnostic) ||
            !horde::scene::assets::StaticMeshAsset::Load(viewmodelPath, manifest, viewmodelAsset_, diagnostic) ||
            !viewmodelSkin_.LoadClips(viewmodelPath.string(), horde::scene::PlayerLocomotionClipSet(), diagnostic) ||
            !viewmodelSkin_.ValidateStaticVertexLayout(viewmodelAsset_, diagnostic)) return false;
    }
    std::vector<StaticRtAssetRegistration> registrations{
        {3u, 0x53574f52u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr),
         0u, &developmentStaticAsset_},
        {1u, 0x544f5243u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr),
         1u, &productionTorchAsset_},
        {kPlayerWorldBodyInstanceIndex, 0x504c4159u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr),
         0u, &productionPlayerAsset_, nullptr, RtGeometryRole::PlayerWorldBody},
        {9u, 0x474c4153u,
         static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr) |
             static_cast<std::uint32_t>(RtInstanceFlag::Transmissive),
         0u, &productionDielectricFixtureAsset_},
        {5u, 0x43484241u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr),
         0u, &gothicChestBaseAsset_},
        {6u, 0x43484c44u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr),
         0u, &gothicChestLidAsset_},
        {7u, 0x4c4e5247u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr),
         0u, &rewardLanternRingAsset_},
        {8u, 0x4c4e4244u,
         static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr) |
             static_cast<std::uint32_t>(RtInstanceFlag::Transmissive),
         0u, &rewardLanternBodyAsset_},
    };
    if (viewmodelAvailable_)
        registrations.push_back({kPlayerViewmodelInstanceIndex, 0x56494557u,
            static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr), 0u,
            &viewmodelAsset_, &productionPlayerAsset_, RtGeometryRole::PlayerViewmodel});
    registrations.push_back({kCollapseInstanceIndex, 0x434f4c4cu,
        static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr), 0u,
        &collapseStaticAsset_});
    // Player Rag torch gets its own metadata/material route. The original
    // production torch remains layer1 for permanent Keeper/world instances.
    registrations.push_back({kPlayerTorchInstanceIndex, 0x544f5243u,
        static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr), 0u,
        &playerTorchAsset_});
    registrations.push_back({kPlayerSwordScabbardMetadataIndex, 0x53434142u,
        static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr), 0u,
        &playerSwordScabbardAsset_});
    if (sceneProfile_ == RtSceneProfile::Showcase)
        registrations.push_back({kWaterDropletMetadataIndex, 0x57415452u,
            static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr) |
                static_cast<std::uint32_t>(RtInstanceFlag::Transmissive), 0u,
            &waterDropletAsset_});
    if (!staticMeshSlot_.Initialize(registrations, diagnostic)) return false;
    const RtInstanceMetadata playerMetadata = staticMeshSlot_.InstanceMetadata()[kPlayerWorldBodyInstanceIndex];
    if (playerMetadata.primitiveCount == 0u ||
        playerMetadata.primitiveCount != productionPlayerAsset_.primitives.size() ||
        playerMetadata.primitiveBase >= staticMeshSlot_.PrimitiveMetadata().size() ||
        playerMetadata.primitiveCount > staticMeshSlot_.PrimitiveMetadata().size() - playerMetadata.primitiveBase)
    {
        diagnostic = "Runtime player static-PBR primitive metadata is incomplete.";
        return false;
    }
    playerStaticVertexBase_ =
        staticMeshSlot_.PrimitiveMetadata()[playerMetadata.primitiveBase].vertexOffset;
    playerBodyRemainderAvailable_ = std::any_of(
        productionPlayerAsset_.primitives.begin(), productionPlayerAsset_.primitives.end(),
        [this](const auto& primitive) {
            return (productionPlayerAsset_.materials[primitive.materialIndex].flags &
                static_cast<std::uint32_t>(RtMaterialFlag::BodyRemainderPrimaryVisible)) != 0u;
        });
    const RtInstanceMetadata dielectricMetadata =
        staticMeshSlot_.InstanceMetadata()[9u];
    if (dielectricMetadata.primitiveCount != 1u ||
        dielectricMetadata.primitiveBase >= staticMeshSlot_.PrimitiveMetadata().size())
    {
        diagnostic = "Runtime dielectric fixture static-PBR primitive metadata is incomplete.";
        return false;
    }
    dielectricFixtureMaterialIndex_ = staticMeshSlot_.PrimitiveMetadata()[
        dielectricMetadata.primitiveBase].materialIndex;
    if (!playerRenderSlot_.ValidateStaticVertexLayout(productionPlayerAsset_, diagnostic)) return false;
    return true;
}

const PresentableTinyRtScene::Buffer& PresentableTinyRtScene::VertexBufferForRole(RtGeometryRole role) const
{
    const std::array<const Buffer*, 3u> buffers{{
        &staticVertexBuffer_, &worldPlayerVertexBuffer_, &viewmodelVertexBuffer_}};
    return *buffers.at(static_cast<std::size_t>(role));
}

bool PresentableTinyRtScene::LoadPreviewStaticAssets(const std::string& productionAssetRoot,
                                                   std::string& diagnostic)
{
    const std::filesystem::path root(productionAssetRoot);
    const auto load = [&diagnostic, &root](const char* directory, const char* filename,
                                          horde::scene::assets::StaticMeshAsset& asset) {
        const auto path = root / directory;
        horde::scene::assets::AssetManifest manifest;
        return horde::scene::assets::AssetManifest::Load(path / "asset.manifest.json", manifest, diagnostic) &&
            horde::scene::assets::StaticMeshAsset::Load(path / filename, manifest, asset, diagnostic);
    };
    const bool entry = sceneProfile_ == RtSceneProfile::EntryMenu;
    if ((!entry && !load("models/props/runtime", "gothic-hand-torch-lod0.runtime.glb",
                         productionTorchAsset_)) ||
        (!entry && !load("models/props/runtime/dielectric-fixture", "closed-glass-lod0.runtime.glb",
                         productionDielectricFixtureAsset_)) ||
        !load("models/props/runtime/reward-lantern-ring", "reward-lantern-ring-lod0.runtime.glb",
              rewardLanternRingAsset_) ||
        !load("models/props/runtime/reward-lantern-body", "reward-lantern-body-lod0.runtime.glb",
              rewardLanternBodyAsset_) ||
        !SelectLanternGeometryForQuality(rewardLanternBodyAsset_, pipelineBundle_.Request().quality,
                                         diagnostic, glassEnabled_))
        return false;
    viewmodelAvailable_ = false;
    playerBodyRemainderAvailable_ = false;
    staticTextureDirectory_ = (root / "textures/props/runtime").string();
    const std::array<StaticRtAssetRegistration, 4u> registrations{{
        {1u, 0x544f5243u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr), 0u, &productionTorchAsset_},
        {9u, 0x474c4153u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr) |
            static_cast<std::uint32_t>(RtInstanceFlag::Transmissive), 0u, &productionDielectricFixtureAsset_},
        {7u, 0x4c4e5247u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr), 0u, &rewardLanternRingAsset_},
        {8u, 0x4c4e4244u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr) |
            static_cast<std::uint32_t>(RtInstanceFlag::Transmissive), 1u, &rewardLanternBodyAsset_},
    }};
    const std::array<StaticRtAssetRegistration, 2u> entryRegistrations{
        {registrations[2], registrations[3]}};
    if (!staticMeshSlot_.Initialize(
            entry ? std::span<const StaticRtAssetRegistration>(entryRegistrations)
                  : std::span<const StaticRtAssetRegistration>(registrations),
            diagnostic))
        return false;
    if (entry)
        return ResolvePreviewPropTransforms(diagnostic);
    const auto fixture = staticMeshSlot_.InstanceMetadata()[9u];
    if (fixture.primitiveCount != 1u || fixture.primitiveBase >= staticMeshSlot_.PrimitiveMetadata().size())
    {
        diagnostic = "Preview dielectric fixture requires its admitted single closed primitive.";
        return false;
    }
    dielectricFixtureMaterialIndex_ = staticMeshSlot_.PrimitiveMetadata()[fixture.primitiveBase].materialIndex;
    return ResolvePreviewPropTransforms(diagnostic);
}

bool PresentableTinyRtScene::ResolvePreviewPropTransforms(std::string& diagnostic)
{
    using namespace horde::gameplay::items;
    const bool entry = sceneProfile_ == RtSceneProfile::EntryMenu;
    const auto description = entry ? horde::graphics::MakeEntryMenuDescription()
                                   : horde::graphics::MakeGraphicsPreviewDescription();
    const auto place = [](const horde::graphics::PreviewPoint& position) {
        auto transform = IdentityHeldItemTransform();
        transform[12] = position[0]; transform[13] = position[1]; transform[14] = position[2];
        return transform;
    };
    previewTransforms_[0] = place(description.torchPosition);
    auto hinge = place(description.lanternPosition);
    const auto* ringHinge = FindHeldItemSocket(rewardLanternRingAsset_.sockets, "Hinge");
    const auto* torchFlame = FindHeldItemSocket(productionTorchAsset_.sockets, "Flame");
    const auto* torchLight = FindHeldItemSocket(productionTorchAsset_.sockets, "Light");
    const auto* lanternFlame = FindHeldItemSocket(rewardLanternBodyAsset_.sockets, "Flame");
    const auto* lanternLight = FindHeldItemSocket(rewardLanternBodyAsset_.sockets, "Light");
    if (ringHinge == nullptr || (!entry && (torchFlame == nullptr || torchLight == nullptr)) ||
        lanternFlame == nullptr || lanternLight == nullptr)
    {
        diagnostic = "Preview requires admitted torch and lantern pivot/flame/light sockets.";
        return false;
    }
    auto ringSocket = ringHinge->world;
    const float lanternScale = entry ? horde::graphics::kEntryMenuLanternScale
                                     : kClaimedRewardLanternScale;
    if (entry)
        for (std::size_t axis = 12; axis < 15; ++axis)
            ringSocket[axis] *= lanternScale;
    if (!ComposeWorldFromItem(hinge, ringSocket, previewTransforms_[1], diagnostic))
        return false;
    auto scale = IdentityHeldItemTransform();
    scale[0] = scale[5] = scale[10] = lanternScale;
    previewTransforms_[1] = MultiplyHeldItemTransforms(previewTransforms_[1], scale);
    previewTransforms_[2] = MultiplyHeldItemTransforms(hinge, scale);
    previewTransforms_[3] = place(description.panePosition);
    previewTransforms_[3][0] = 0.06f;
    previewTransforms_[3][5] = 1.25f;
    previewTransforms_[3][10] = 0.75f;
    previewTransforms_[4] = place(description.waterOrigin);
    if (!entry)
    {
        previewFireInputs_[0].worldFromFlame =
            MultiplyHeldItemTransforms(previewTransforms_[0], torchFlame->world);
        previewFireInputs_[0].worldFromLight =
            MultiplyHeldItemTransforms(previewTransforms_[0], torchLight->world);
    }
    previewFireInputs_[0].strength = 1.8f;
    previewFireInputs_[1].worldFromFlame = MultiplyHeldItemTransforms(previewTransforms_[2], lanternFlame->world);
    previewFireInputs_[1].worldFromLight = MultiplyHeldItemTransforms(previewTransforms_[2], lanternLight->world);
    previewFireInputs_[1].strength = 0.78f;
    diagnostic.clear();
    return true;
}

bool PresentableTinyRtScene::ConfigurePreviewFireSockets(horde::graphics::GraphicsPreviewSession& session,
                                                       std::string& diagnostic) const
{
    if (!ready_ || sceneProfile_ != RtSceneProfile::GraphicsPreview)
    {
        diagnostic = "Preview fire sockets require an initialized GraphicsPreview profile.";
        return false;
    }
    for (std::size_t index = 0u; index < previewFireInputs_.size(); ++index)
        session.SetFireSockets(index, previewFireInputs_[index]);
    session.Reset();
    diagnostic.clear();
    return true;
}

bool PresentableTinyRtScene::ConfigureEntryMenu(horde::graphics::EntryMenuSession &session,
                                                std::string &diagnostic) const
{
    const auto *flame =
        horde::gameplay::items::FindHeldItemSocket(rewardLanternBodyAsset_.sockets, "Flame");
    const auto *light =
        horde::gameplay::items::FindHeldItemSocket(rewardLanternBodyAsset_.sockets, "Light");
    if (!ready_ || sceneProfile_ != RtSceneProfile::EntryMenu || flame == nullptr ||
        light == nullptr)
    {
        diagnostic = "Entry menu requires its initialized lantern and authored fire sockets.";
        return false;
    }
    session.ConfigureSockets(flame->world, light->world,
                             horde::graphics::kEntryMenuLanternScale);
    diagnostic.clear();
    return true;
}

bool PresentableTinyRtScene::CreateStaticMeshResources(std::string& diagnostic)
{
    const VkMemoryPropertyFlags uploadMemory =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    const auto& instances = staticMeshSlot_.InstanceMetadata();
    const auto& primitives = staticMeshSlot_.PrimitiveMetadata();
    sceneMaterials_ = staticMeshSlot_.Materials();
    const auto worldPalette=MakeRtWorldMaterialPalette(sceneProfile_ == RtSceneProfile::Showcase);
    const std::uint32_t worldMaterialCount = worldPalette.count;
    if (sceneMaterials_.size() > kRtMaterialCapacity - worldMaterialCount)
    {
        diagnostic = "Imported and authored world materials exceed the fixed RT material capacity.";
        return false;
    }

    worldMaterialBase_ = static_cast<std::uint32_t>(sceneMaterials_.size());
    for (std::uint32_t material = 0u; material < worldMaterialCount; ++material)
    {
        sceneMaterials_.push_back(worldPalette.records[material]);
    }
    const auto& materials = sceneMaterials_;
    const auto& vertices = staticMeshSlot_.Vertices();
    const auto& worldVertices = staticMeshSlot_.Vertices(RtGeometryRole::PlayerWorldBody);
    const auto& viewVertices = staticMeshSlot_.Vertices(RtGeometryRole::PlayerViewmodel);
    const horde::scene::assets::StaticRtVertex unusedViewVertex{};
    const auto& indices = staticMeshSlot_.Indices();
    const auto& geometryTransforms = staticMeshSlot_.GeometryTransforms();
    const auto createAndWrite = [this, uploadMemory, &diagnostic](
        const void* data,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        bool address,
        const char* label,
        Buffer& buffer, const bool dynamic = false) {
        const std::array<std::uint8_t, 16u> zeros{};
        const VkDeviceSize actualSize = std::max<VkDeviceSize>(size, zeros.size());
        if (!CreateBuffer(actualSize, usage, uploadMemory, address, buffer, diagnostic,
                          dynamic ? 0u : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) return false;
        if (dynamic && !gpuResources_.MapBufferForHostWrites(buffer, diagnostic)) return false;
        return WriteBuffer(buffer, size == 0u ? zeros.data() : data, actualSize, label, diagnostic);
    };

    const VkBufferUsageFlags storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    const VkBufferUsageFlags geometry = storage |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    if (!createAndWrite(instances.data(), sizeof(instances), storage, false, "RT instance metadata",
                        instanceMetadataBuffer_, true) ||
        !createAndWrite(primitives.data(), primitives.size() * sizeof(RtPrimitiveMetadata), storage,
                        false, "RT primitive metadata", primitiveMetadataBuffer_) ||
        !createAndWrite(materials.data(), materials.size() * sizeof(RtMaterialGpu), storage, false,
                        "RT material metadata", materialMetadataBuffer_, true) ||
        !createAndWrite(vertices.data(),
                        vertices.size() * sizeof(horde::scene::assets::StaticRtVertex), geometry,
                        true, "static RT vertices", staticVertexBuffer_) ||
        (sceneProfile_ == RtSceneProfile::Showcase &&
         !createAndWrite(worldVertices.data(), worldVertices.size() * sizeof(worldVertices.front()),
                         geometry, true, "world player RT vertices", worldPlayerVertexBuffer_,
                         true)) ||
        (sceneProfile_ == RtSceneProfile::Showcase &&
         !createAndWrite(viewVertices.empty() ? &unusedViewVertex : viewVertices.data(),
                         std::max<std::size_t>(viewVertices.size(), 1u) * sizeof(unusedViewVertex),
                         geometry, true, "viewmodel RT vertices", viewmodelVertexBuffer_, true)) ||
        !createAndWrite(indices.data(), indices.size() * sizeof(std::uint32_t), geometry, true,
                        "static RT indices", staticIndexBuffer_) ||
        !createAndWrite(geometryTransforms.data(),
                        geometryTransforms.size() * sizeof(geometryTransforms.front()),
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, true,
                        "static RT geometry transforms", staticGeometryTransformBuffer_))
    {
        return false;
    }

    if (!genericStaticAssetEnabled_) return true;
    const RtTextureArrayCounts textureCounts = staticMeshSlot_.TextureArrayCounts();
    // Canonical packaged atlas order is recorded in textures/props/runtime/
    // asset.manifest.json. Narrow admission preserves torch/ring/body order.
    constexpr std::array<std::uint32_t, 3u> previewTextureLayers{{1u, 8u, 9u}};
    constexpr std::array<std::uint32_t, 2u> entryTextureLayers{{8u, 9u}};
    const std::span<const std::uint32_t> sourceLayers =
        sceneProfile_ == RtSceneProfile::GraphicsPreview
            ? std::span<const std::uint32_t>(previewTextureLayers)
        : sceneProfile_ == RtSceneProfile::EntryMenu
            ? std::span<const std::uint32_t>(entryTextureLayers)
            : std::span<const std::uint32_t>{};
    if (!sourceLayers.empty() &&
        (textureCounts.baseColor != sourceLayers.size() ||
         textureCounts.normal != sourceLayers.size() || textureCounts.orm != sourceLayers.size()))
    {
        diagnostic = "Preview prop texture groups no longer match the admitted canonical layer selection.";
        return false;
    }
    const std::array<std::uint32_t, 4u> actualTextureLayers{{
        std::max(textureCounts.baseColor, 1u),
        std::max(textureCounts.normal, 1u),
        std::max(textureCounts.orm, 1u),
        std::max(textureCounts.emissive, 1u)}};
    const std::uint32_t textureDimension = productionHeldItemAssetsEnabled_ ? 1024u : 512u;
    for (std::uint32_t dimension = textureDimension; dimension > 0u; dimension >>= 1u)
        staticTextureBytes_ += static_cast<VkDeviceSize>(dimension) * dimension * 4u *
            (actualTextureLayers[0] + actualTextureLayers[1] +
             actualTextureLayers[2] + actualTextureLayers[3]);
    const std::filesystem::path assetDirectory(staticTextureDirectory_);
    const auto path = [&assetDirectory](const char* name) {
        return (assetDirectory / name).string();
    };
#if defined(__ANDROID__)
    if (!productionHeldItemAssetsEnabled_)
    {
        diagnostic = "Development static assets cannot be enabled in an Android package.";
        return false;
    }
    if (!SupportsTextureArrayFormat(VK_FORMAT_ASTC_6x6_SRGB_BLOCK) ||
        !SupportsTextureArrayFormat(VK_FORMAT_ASTC_4x4_UNORM_BLOCK) ||
        !SupportsTextureArrayFormat(VK_FORMAT_ASTC_6x6_UNORM_BLOCK))
    {
        diagnostic = "Production held-item PBR requires sampled ASTC 4x4/6x6 support; no uncompressed Android fallback is allowed.";
        return false;
    }
    if (!CreateTexture(path("base-color.android.ktx2"), VK_FORMAT_ASTC_6x6_SRGB_BLOCK,
                       textureDimension, textureDimension, actualTextureLayers[0], staticBaseColor_, diagnostic, sourceLayers) ||
        !CreateTexture(path("normal.android.ktx2"), VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
                       textureDimension, textureDimension, actualTextureLayers[1], staticNormal_, diagnostic, sourceLayers) ||
        !CreateTexture(path("orm.android.ktx2"), VK_FORMAT_ASTC_6x6_UNORM_BLOCK,
                       textureDimension, textureDimension, actualTextureLayers[2], staticOrm_, diagnostic, sourceLayers) ||
        !CreateTexture(path("emissive.android.ktx2"), VK_FORMAT_ASTC_6x6_SRGB_BLOCK,
                       textureDimension, textureDimension, actualTextureLayers[3], staticEmissive_, diagnostic))
        return false;
#else
    if (!CreateTexture(path("base-color.windows.ktx2"), VK_FORMAT_R8G8B8A8_SRGB,
                       textureDimension, textureDimension, actualTextureLayers[0], staticBaseColor_, diagnostic, sourceLayers) ||
        !CreateTexture(path("normal.windows.ktx2"), VK_FORMAT_R8G8B8A8_UNORM,
                       textureDimension, textureDimension, actualTextureLayers[1], staticNormal_, diagnostic, sourceLayers) ||
        !CreateTexture(path("orm.windows.ktx2"), VK_FORMAT_R8G8B8A8_UNORM,
                       textureDimension, textureDimension, actualTextureLayers[2], staticOrm_, diagnostic, sourceLayers) ||
        !CreateTexture(path("emissive.windows.ktx2"), VK_FORMAT_R8G8B8A8_SRGB,
                       textureDimension, textureDimension, actualTextureLayers[3], staticEmissive_, diagnostic))
    {
        return false;
    }
#endif
    return true;
}

void PresentableTinyRtScene::DestroyAccelerationStructure(AccelerationStructure& accelerationStructure)
{
    gpuResources_.DestroyAccelerationStructure(accelerationStructure);
}

bool PresentableTinyRtScene::CreateScratchBuffer(const VkDeviceSize usableSize,
                                                Buffer& out,
                                                std::string& diagnostic) const
{
    return gpuResources_.CreateAlignedBuffer(
        usableSize, scratchAddressAlignment_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, out, diagnostic);
}

bool PresentableTinyRtScene::BuildProfileAccelerationStructure(
    const std::span<const VkAccelerationStructureGeometryKHR> geometries,
    const std::span<const VkAccelerationStructureBuildRangeInfoKHR> ranges,
    const VkAccelerationStructureTypeKHR type, const VkBuildAccelerationStructureFlagsKHR flags,
    AccelerationStructure& out, Buffer* retainedScratch, std::string& diagnostic)
{
    if (geometries.empty() || geometries.size() != ranges.size() || geometries.size() > UINT32_MAX)
    {
        diagnostic = "RT content profile supplied an empty or mismatched AS geometry/range set.";
        return false;
    }
    std::vector<std::uint32_t> counts;
    std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> pointers;
    for (const auto& range : ranges)
    {
        if (range.primitiveCount == 0u)
        {
            diagnostic = "RT content profile supplied an empty AS primitive range.";
            return false;
        }
        counts.push_back(range.primitiveCount);
        pointers.push_back(&range);
    }
    VkAccelerationStructureBuildGeometryInfoKHR build{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    build.type = type; build.flags = flags; build.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build.geometryCount = static_cast<std::uint32_t>(geometries.size());
    build.pGeometries = geometries.data();
    VkAccelerationStructureBuildSizesInfoKHR sizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &build, counts.data(), &sizes);
    if (!CreateBuffer(sizes.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, out.backing, diagnostic)) return false;
    VkAccelerationStructureCreateInfoKHR create{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    create.buffer = out.backing.buffer; create.size = sizes.accelerationStructureSize; create.type = type;
    if (vkCreateAccelerationStructureKHR_(device_, &create, nullptr, &out.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create an admitted RT content profile acceleration structure.";
        return false;
    }
    Buffer temporaryScratch;
    Buffer& scratch = retainedScratch != nullptr ? *retainedScratch : temporaryScratch;
    if (!CreateScratchBuffer(retainedScratch != nullptr ? std::max(sizes.buildScratchSize, sizes.updateScratchSize)
                                                        : sizes.buildScratchSize, scratch, diagnostic)) return false;
    build.dstAccelerationStructure = out.handle;
    build.scratchData.deviceAddress = scratch.AlignedAddress();
    struct BuildData
    {
        PresentableTinyRtScene* scene;
        const VkAccelerationStructureBuildGeometryInfoKHR* build;
        const VkAccelerationStructureBuildRangeInfoKHR* const* ranges;
    } data{this, &build, pointers.data()};
    const bool built = RunOneTimeCommands([](VkCommandBuffer commandBuffer, void* user) {
        const auto& data = *static_cast<BuildData*>(user);
        data.scene->vkCmdBuildAccelerationStructuresKHR_(commandBuffer, 1u, data.build, data.ranges);
        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR |
                                 data.scene->executionPolicy_.shaderPipelineStage,
                             0u, 1u, &barrier, 0u, nullptr, 0u, nullptr);
    }, &data, diagnostic);
    if (retainedScratch == nullptr) DestroyBuffer(temporaryScratch);
    if (!built) return false;
    VkAccelerationStructureDeviceAddressInfoKHR addressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    addressInfo.accelerationStructure = out.handle;
    out.address = vkGetAccelerationStructureDeviceAddressKHR_(device_, &addressInfo);
    if (out.address == 0u)
    {
        diagnostic = "RT content profile acceleration structure returned a zero device address.";
        return false;
    }
    return true;
}

bool PresentableTinyRtScene::BuildPreviewAccelerationStructures(std::string& diagnostic)
{
    const bool entry = sceneProfile_ == RtSceneProfile::EntryMenu;
    const auto content = entry ? horde::graphics::MakeEntryMenuDescription()
                               : horde::graphics::MakeGraphicsPreviewDescription();
    using Vertex = horde::graphics::PreviewPoint;
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<std::uint32_t> surfaceCodes;
    const auto quad = [&vertices, &indices](const std::array<Vertex, 4u>& points) {
        const auto base = static_cast<std::uint32_t>(vertices.size());
        vertices.insert(vertices.end(), points.begin(), points.end());
        indices.insert(indices.end(), {base, base + 1u, base + 2u, base, base + 2u, base + 3u});
    };
    const auto worldQuad = [this, &quad, &surfaceCodes](const horde::graphics::PreviewQuad& plane) {
        quad(plane.vertices);
        const auto authored = AuthoredWorldMaterialIndexPlusOne(worldMaterialBase_,plane.material);
        const auto code = plane.SurfaceCode() | (authored << 16u);
        surfaceCodes.insert(surfaceCodes.end(), {code, code});
    };
    for (const auto& plane : content.worldQuads) worldQuad(plane);
    for (const auto& plane : content.waterQuads) worldQuad(plane);
    const auto worldIndexCount = static_cast<std::uint32_t>(indices.size());
    // Reuse the production analytic water profile exactly, including taper and
    // world anchor. Each stream has physical depth and open air between streams.
    constexpr std::array<float, 8u> ringCos{{1.0f, .70710678f, 0.0f, -.70710678f, -1.0f, -.70710678f, 0.0f, .70710678f}};
    constexpr std::array<float, 8u> ringSin{{0.0f, .70710678f, 1.0f, .70710678f, 0.0f, -.70710678f, -1.0f, -.70710678f}};
    if (!entry)
        for (const auto &stream : content.waterStreams)
        {
            std::array<std::array<Vertex, 8u>, 6u> rings{};
            for (std::size_t height = 0u; height < rings.size(); ++height)
            {
                const float y = content.waterRingHeights[height];
                const float scale = 0.70f + std::clamp((y + .91f) / 3.07f, 0.0f, 1.0f) * .30f;
                for (std::size_t facet = 0u; facet < ringCos.size(); ++facet)
                    rings[height][facet] = {
                        {ringCos[facet] * stream.radiusX * scale, y,
                         stream.centreZ + ringSin[facet] * stream.radiusZ * scale}};
            }
            for (std::size_t segment = 0u; segment + 1u < rings.size(); ++segment)
                for (std::size_t facet = 0u; facet < ringCos.size(); ++facet)
                {
                    const auto next = (facet + 1u) % ringCos.size();
                    quad({{rings[segment + 1u][facet], rings[segment][facet], rings[segment][next],
                           rings[segment + 1u][next]}});
                }
        }
    const auto upload = [this, &diagnostic](const void* data, VkDeviceSize bytes,
                                          VkBufferUsageFlags usage, bool address,
                                          const char* label, Buffer& buffer, bool dynamic = false) {
        return CreateBuffer(bytes, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            address, buffer, diagnostic, dynamic ? 0u : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
            (!dynamic || gpuResources_.MapBufferForHostWrites(buffer, diagnostic)) &&
            WriteBuffer(buffer, data, bytes, label, diagnostic);
    };
    const auto geometryUsage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    RtHeldLightGpu light{};
    std::array<RtFireEmitterGpu, kRtFireEmitterCapacity> fire{};
    const auto initialQuality = *ResolveRtQualityControls(std::nullopt, RtWorkloadPreset::Authored,
        pipelineBundle_.Request().quality == DielectricQuality::High);
    const QualityDustUpload initialDustQuality{initialQuality,{}};
    const WaterContactRippleGpu initialWaterContactRipple{};
    if (!upload(vertices.data(), vertices.size() * sizeof(Vertex), geometryUsage, true, "preview world vertices", vertexBuffer_) ||
        !upload(indices.data(), indices.size() * sizeof(std::uint32_t), geometryUsage, true, "preview world indices", indexBuffer_) ||
        !upload(surfaceCodes.data(), surfaceCodes.size() * sizeof(std::uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, false,
                "preview world surfaces", worldSurfaceBuffer_) ||
        !upload(&light, sizeof(light), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, false, "preview light", heldLightBuffer_, true) ||
        !upload(fire.data(), sizeof(fire), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, false, "preview fire", fireEmitterBuffer_, true) ||
        !upload(&initialDustQuality, sizeof(initialDustQuality), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, false,
                "preview quality controls", qualityControlsBuffer_, true) ||
        !upload(&initialWaterContactRipple, sizeof(initialWaterContactRipple), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, false,
                "preview water contact ripple", waterContactRippleBuffer_, true)) return false;
    VkAccelerationStructureGeometryKHR world{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    world.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR; world.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    auto& triangles = world.geometry.triangles;
    triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    triangles.vertexData.deviceAddress = vertexBuffer_.address; triangles.vertexStride = sizeof(Vertex);
    triangles.maxVertex = static_cast<std::uint32_t>(vertices.size() - 1u);
    triangles.indexType = VK_INDEX_TYPE_UINT32; triangles.indexData.deviceAddress = indexBuffer_.address;
    VkAccelerationStructureBuildRangeInfoKHR range{}; range.primitiveCount = worldIndexCount / 3u;
    constexpr auto fast = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    if (!BuildProfileAccelerationStructure({&world, 1u}, {&range, 1u}, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
                                           fast, blas_, nullptr, diagnostic)) return false;
    range.primitiveCount = (static_cast<std::uint32_t>(indices.size()) - worldIndexCount) / 3u;
    range.primitiveOffset = worldIndexCount * sizeof(std::uint32_t);
    if (!entry && !BuildProfileAccelerationStructure(
                      {&world, 1u}, {&range, 1u}, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
                      fast, waterfallBlas_, nullptr, diagnostic))
        return false;
    const auto buildStatic = [this, fast, &diagnostic](std::uint32_t id, AccelerationStructure& target) {
        const auto instance = staticMeshSlot_.InstanceMetadata()[id];
        std::vector<VkAccelerationStructureGeometryKHR> geometries;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges;
        for (std::uint32_t local = 0u; local < instance.primitiveCount; ++local)
        {
            const auto geometryIndex = instance.primitiveBase + local;
            const auto& primitive = staticMeshSlot_.PrimitiveMetadata()[geometryIndex];
            const auto& material = sceneMaterials_[primitive.materialIndex];
            VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
            geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            geometry.flags = (material.materialFlags[0] & static_cast<std::uint32_t>(RtMaterialFlag::Transmission)) != 0u
                ? VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR : VK_GEOMETRY_OPAQUE_BIT_KHR;
            auto& triangles = geometry.geometry.triangles;
            triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
            triangles.vertexStride = sizeof(horde::scene::assets::StaticRtVertex);
            triangles.vertexData.deviceAddress = staticVertexBuffer_.address + primitive.vertexOffset * triangles.vertexStride;
            triangles.maxVertex = staticMeshSlot_.PrimitiveVertexCounts()[geometryIndex] - 1u;
            triangles.indexType = VK_INDEX_TYPE_UINT32;
            triangles.indexData.deviceAddress = staticIndexBuffer_.address + primitive.indexOffset * sizeof(std::uint32_t);
            triangles.transformData.deviceAddress = staticGeometryTransformBuffer_.address + geometryIndex * sizeof(VkTransformMatrixKHR);
            geometries.push_back(geometry);
            VkAccelerationStructureBuildRangeInfoKHR range{}; range.primitiveCount = primitive.indexCount / 3u;
            ranges.push_back(range);
        }
        return BuildProfileAccelerationStructure(geometries, ranges, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
                                                  fast, target, nullptr, diagnostic);
    };
    if ((!entry && !buildStatic(1u, torchBlas_)) || !buildStatic(7u, rewardLanternRingBlas_) ||
        !buildStatic(8u, rewardLanternBodyBlas_) ||
        (!entry && !buildStatic(9u, dielectricFixtureBlas_)) ||
        (!entry && !characterSlot_.PrepareInitialGeometry(diagnostic)))
        return false;
    constexpr auto updatable = fast | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    if (!entry)
    {
        auto &skeleton = characterSlot_.SkeletonGpu(0u);
        const auto &skeletonVertices = characterSlot_.SkeletonVertices(0u);
        skeleton.vertexStride = sizeof(horde::scene::SkinnedRtVertex);
        skeleton.vertexCount = static_cast<std::uint32_t>(skeletonVertices.size());
        if (!upload(skeletonVertices.data(), skeletonVertices.size() * skeleton.vertexStride,
                    geometryUsage, true, "preview skeleton vertices", skeleton.vertices, true))
            return false;
        VkAccelerationStructureGeometryKHR actor{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        actor.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        actor.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        actor.geometry.triangles.sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        actor.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        actor.geometry.triangles.vertexStride = skeleton.vertexStride;
        actor.geometry.triangles.vertexData.deviceAddress = skeleton.vertices.address;
        actor.geometry.triangles.maxVertex = skeleton.vertexCount - 1u;
        actor.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
        range = {};
        range.primitiveCount = skeleton.vertexCount / 3u;
        if (!BuildProfileAccelerationStructure(
                {&actor, 1u}, {&range, 1u}, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
                updatable, skeleton.accelerationStructure, &skeleton.updateScratch, diagnostic))
            return false;
    }
    tlasInstanceCount_ = entry ? 3u : 7u;
    auto& instances = tlasBuiltInstances_;
    instances = {};
    const auto identity = horde::gameplay::items::IdentityHeldItemTransform();
    const auto instance = [](std::uint32_t id, VkDeviceAddress address, const horde::gameplay::items::HeldItemTransform& transform) {
        VkAccelerationStructureInstanceKHR value{};
        value.transform = {{transform[0], transform[4], transform[8], transform[12],
                            transform[1], transform[5], transform[9], transform[13],
                            transform[2], transform[6], transform[10], transform[14]}};
        value.instanceCustomIndex = id; value.mask = 0x01u;
        value.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        value.accelerationStructureReference = address;
        return value;
    };
    instances[0] = instance(0u, blas_.address, identity);
    if (entry)
    {
        instances[1] = instance(7u, rewardLanternRingBlas_.address, previewTransforms_[1]);
        instances[2] = instance(8u, rewardLanternBodyBlas_.address, previewTransforms_[2]);
    }
    else
    {
        instances[1] = instance(1u, torchBlas_.address, previewTransforms_[0]);
        const auto &skeleton = characterSlot_.SkeletonGpu(0u);
        auto skeletonTransform = identity;
        skeletonTransform[12] = content.skeleton.x;
        skeletonTransform[13] = kRouteFloorWorldY;
        skeletonTransform[14] = content.skeleton.z;
        instances[2] = instance(2u, skeleton.accelerationStructure.address, skeletonTransform);
        instances[3] = instance(7u, rewardLanternRingBlas_.address, previewTransforms_[1]);
        instances[4] = instance(8u, rewardLanternBodyBlas_.address, previewTransforms_[2]);
        instances[5] = instance(9u, dielectricFixtureBlas_.address, previewTransforms_[3]);
        instances[6] = instance(19u, waterfallBlas_.address, previewTransforms_[4]);
        ApplyGlassFixtureVisibility(std::span(instances).first(tlasInstanceCount_));
    }
    if (!upload(instances.data(), tlasInstanceCount_ * sizeof(instances[0]),
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                true, "preview TLAS instances", instanceBuffer_, true)) return false;
    VkAccelerationStructureGeometryKHR top{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    top.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    top.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    top.geometry.instances.data.deviceAddress = instanceBuffer_.address;
    range = {}; range.primitiveCount = tlasInstanceCount_;
    if (!BuildProfileAccelerationStructure({&top, 1u}, {&range, 1u}, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
                                           updatable, tlas_, &tlasUpdateScratch_, diagnostic)) return false;
    tlasInstanceDefinitionsValid_ = true;
    diagnostic.clear();
    return true;
}

bool PresentableTinyRtScene::BuildAccelerationStructures(std::string& diagnostic)
{
    // Query for both RT backends; buffer memory alignment alone does not satisfy
    // the scratch device-address requirement (VUID-pInfos-03710).
    VkPhysicalDeviceAccelerationStructurePropertiesKHR asProperties{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
    VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    properties.pNext = &asProperties;
    auto getProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
        vkGetInstanceProcAddr(instance_, "vkGetPhysicalDeviceProperties2"));
    if (getProperties == nullptr)
        getProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
            vkGetInstanceProcAddr(instance_, "vkGetPhysicalDeviceProperties2KHR"));
    if (getProperties == nullptr)
    {
        diagnostic = "Vulkan properties2 entry point is unavailable for AS scratch alignment.";
        return false;
    }
    getProperties(physicalDevice_, &properties);
    scratchAddressAlignment_ = asProperties.minAccelerationStructureScratchOffsetAlignment;
    if (!IsDeviceAddressAlignment(scratchAddressAlignment_))
    {
        diagnostic = "Device returned an invalid AS scratch address alignment.";
        return false;
    }

    if (sceneProfile_ != RtSceneProfile::Showcase)
        return BuildPreviewAccelerationStructures(diagnostic);

    struct Vertex
    {
        float position[3];
    };
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<Vertex> waterfallVertices;
    std::vector<std::uint32_t> waterfallIndices;
    std::vector<std::uint32_t> worldSurfaceCodes;
    vertices.reserve(3072u);
    indices.reserve(4608u);
    worldSurfaceCodes.reserve(1536u);
    waterfallVertices.reserve(144u);
    waterfallIndices.reserve(240u);

    enum SurfaceMaterial : std::uint32_t
    {
        SurfaceDryStone = 0u,
        SurfaceWetCobble = 1u,
        SurfaceMossyStone = 2u,
        SurfaceDampGround = 3u,
        SurfaceAgedMetal = 4u,
        SurfaceFlame = 5u,
        SurfaceDarkFigure = 6u,
        SurfaceHiddenShell = 7u,
        SurfaceMirror = 8u,
        SurfaceClearGlass = 9u,
        // Keep the existing CPU/shader material ABI stable. Water is appended
        // because every world primitive reads this numeric code in raygen.
        SurfaceWater = 10u,
    };
    enum SurfaceNormal : std::uint32_t
    {
        SurfaceUp = 0u,
        SurfaceDown = 1u,
        SurfaceRight = 2u,
        SurfaceLeft = 3u,
        SurfaceForward = 4u,
        SurfaceBack = 5u,
        SurfaceGalleryCant = 6u,
    };
    const auto surfaceCode = [this](SurfaceMaterial material, SurfaceNormal normal, std::uint32_t authoredIndexPlusOne = 0u) {
        const auto materialId = static_cast<std::uint32_t>(material);
        const auto authoredRecord = authoredIndexPlusOne != 0u ? authoredIndexPlusOne
            : AuthoredWorldMaterialIndexPlusOne(worldMaterialBase_,materialId);
        return materialId | (static_cast<std::uint32_t>(normal) << 8u) |
            (authoredRecord << 16u);
    };

    const auto addQuad = [&vertices, &indices](const Vertex& a, const Vertex& b, const Vertex& c, const Vertex& d) {
        const std::uint32_t base = static_cast<std::uint32_t>(vertices.size());
        vertices.push_back(a);
        vertices.push_back(b);
        vertices.push_back(c);
        vertices.push_back(d);
        indices.insert(indices.end(), {base, base + 1u, base + 2u, base, base + 2u, base + 3u});
    };
    const auto addTriangle = [&vertices, &indices](const Vertex& a, const Vertex& b, const Vertex& c) {
        const std::uint32_t base = static_cast<std::uint32_t>(vertices.size());
        vertices.push_back(a);
        vertices.push_back(b);
        vertices.push_back(c);
        indices.insert(indices.end(), {base, base + 1u, base + 2u});
    };
    const auto addWaterfallQuad = [&waterfallVertices, &waterfallIndices](
                                      const Vertex& a,
                                      const Vertex& b,
                                      const Vertex& c,
                                      const Vertex& d) {
        const std::uint32_t base = static_cast<std::uint32_t>(waterfallVertices.size());
        waterfallVertices.push_back(a);
        waterfallVertices.push_back(b);
        waterfallVertices.push_back(c);
        waterfallVertices.push_back(d);
        waterfallIndices.insert(
            waterfallIndices.end(), {base, base + 1u, base + 2u, base, base + 2u, base + 3u});
    };
    const auto addWorldQuad = [&addQuad, &worldSurfaceCodes, &surfaceCode](const Vertex& a,
                                                                          const Vertex& b,
                                                                          const Vertex& c,
                                                                          const Vertex& d,
                                                                          SurfaceMaterial material,
                                                                          SurfaceNormal normal,
                                                                          std::uint32_t authoredIndexPlusOne = 0u) {
        addQuad(a, b, c, d);
        const std::uint32_t code = surfaceCode(material, normal, authoredIndexPlusOne);
        worldSurfaceCodes.push_back(code);
        worldSurfaceCodes.push_back(code);
    };
    const auto addWorldTriangle = [&addTriangle, &worldSurfaceCodes, &surfaceCode](
                                      const Vertex& a,
                                      const Vertex& b,
                                      const Vertex& c,
                                      SurfaceMaterial material,
                                      SurfaceNormal normal) {
        addTriangle(a, b, c);
        worldSurfaceCodes.push_back(surfaceCode(material, normal));
    };
    const auto addWorldBox = [&addWorldQuad](float minX, float minY, float minZ,
                                             float maxX, float maxY, float maxZ,
                                             SurfaceMaterial material) {
        // Closed solid boxes use outward geometric normals matching winding.
        // Inward metadata launched hemisphere bounces/spawn offsets inside the
        // solid, including the coincident bracket-top/flame-bottom interface.
        addWorldQuad({{minX, minY, minZ}}, {{minX, maxY, minZ}}, {{maxX, maxY, minZ}}, {{maxX, minY, minZ}}, material, SurfaceBack);
        addWorldQuad({{maxX, minY, maxZ}}, {{maxX, maxY, maxZ}}, {{minX, maxY, maxZ}}, {{minX, minY, maxZ}}, material, SurfaceForward);
        addWorldQuad({{minX, minY, maxZ}}, {{minX, maxY, maxZ}}, {{minX, maxY, minZ}}, {{minX, minY, minZ}}, material, SurfaceLeft);
        addWorldQuad({{maxX, minY, minZ}}, {{maxX, maxY, minZ}}, {{maxX, maxY, maxZ}}, {{maxX, minY, maxZ}}, material, SurfaceRight);
        addWorldQuad({{minX, maxY, minZ}}, {{minX, maxY, maxZ}}, {{maxX, maxY, maxZ}}, {{maxX, maxY, minZ}}, material, SurfaceUp);
        addWorldQuad({{minX, minY, maxZ}}, {{minX, minY, minZ}}, {{maxX, minY, minZ}}, {{maxX, minY, maxZ}}, material, SurfaceDown);
    };
    const auto addBox = [&addQuad](float minX, float minY, float minZ, float maxX, float maxY, float maxZ) {
        // Fixed face order for the raygen material normal lookup:
        // -Z, +Z, -X, +X, +Y, -Y (two triangles per face).
        addQuad({{minX, minY, minZ}}, {{minX, maxY, minZ}}, {{maxX, maxY, minZ}}, {{maxX, minY, minZ}});
        addQuad({{maxX, minY, maxZ}}, {{maxX, maxY, maxZ}}, {{minX, maxY, maxZ}}, {{minX, minY, maxZ}});
        addQuad({{minX, minY, maxZ}}, {{minX, maxY, maxZ}}, {{minX, maxY, minZ}}, {{minX, minY, minZ}});
        addQuad({{maxX, minY, minZ}}, {{maxX, maxY, minZ}}, {{maxX, maxY, maxZ}}, {{maxX, minY, maxZ}});
        addQuad({{minX, maxY, minZ}}, {{minX, maxY, maxZ}}, {{maxX, maxY, maxZ}}, {{maxX, maxY, minZ}});
        addQuad({{minX, minY, maxZ}}, {{minX, minY, minZ}}, {{maxX, minY, minZ}}, {{maxX, minY, maxZ}});
    };

    const auto addCeilingPatch = [&addWorldQuad](std::size_t index) {
        const auto& patch = horde::scene::kShowcaseCeilingPatches[index];
        const auto corner = [&patch](std::size_t i) {
            return Vertex{{patch.footprint[i][0], patch.bottomY, patch.footprint[i][1]}};
        };
        addWorldQuad(corner(0), corner(1), corner(2), corner(3), SurfaceDryStone, SurfaceDown);
    };
    const auto addOverheadBox = [&addWorldBox](std::size_t index) {
        const auto& box = horde::scene::kShowcaseLowOverheadVolumes[index];
        addWorldBox(box.footprint[1][0], box.bottomY, box.footprint[1][1],
                    box.footprint[3][0], box.topY, box.footprint[3][1], SurfaceMossyStone);
    };
    addWorldQuad({{-1.85f, kRouteFloorWorldY, 3.4f}}, {{1.85f, kRouteFloorWorldY, 3.4f}}, {{1.85f, kRouteFloorWorldY, -6.4f}}, {{-1.85f, kRouteFloorWorldY, -6.4f}}, SurfaceWetCobble, SurfaceUp);
    if (developmentSupportFixture_)
    {
        using namespace horde::gameplay::simulation;
        const float y = kRouteFloorWorldY + kProofSupportHeight;
        addWorldBox(-kProofSupportHalfWidth, kRouteFloorWorldY, kProofSupportBackZ,
                    kProofSupportHalfWidth, y, kProofSupportTopZ, SurfaceWetCobble);
        addWorldQuad({{-kProofSupportHalfWidth, kRouteFloorWorldY, kProofSupportGroundZ}},
                     {{kProofSupportHalfWidth, kRouteFloorWorldY, kProofSupportGroundZ}},
                     {{kProofSupportHalfWidth, y, kProofSupportTopZ}},
                     {{-kProofSupportHalfWidth, y, kProofSupportTopZ}},
                     SurfaceWetCobble, SurfaceUp);
        addWorldTriangle({{-kProofSupportHalfWidth, kRouteFloorWorldY, kProofSupportTopZ}},
                         {{-kProofSupportHalfWidth, kRouteFloorWorldY, kProofSupportGroundZ}},
                         {{-kProofSupportHalfWidth, y, kProofSupportTopZ}}, SurfaceWetCobble, SurfaceLeft);
        addWorldTriangle({{kProofSupportHalfWidth, kRouteFloorWorldY, kProofSupportGroundZ}},
                         {{kProofSupportHalfWidth, kRouteFloorWorldY, kProofSupportTopZ}},
                         {{kProofSupportHalfWidth, y, kProofSupportTopZ}}, SurfaceWetCobble, SurfaceRight);
    }
    if(developmentWorldRoute_)
    {
        worldRouteGeometry_=horde::scene::PrepareDevelopmentWorldGeometry(stagedWorldPreparation_,developmentRescueJourney_);
        if(developmentRescueJourney_) horde::scene::AppendRescueJourneyGeometry(worldRouteGeometry_);
        if(!worldRouteGeometry_.valid) { diagnostic="Development world route preparation failed finite geometry admission."; return false; }
        for(const auto& triangle:worldRouteGeometry_.triangles)
        {
            const auto vertex=[](const auto& p) { return Vertex{{p[0],p[1],p[2]}}; };
            addWorldTriangle(vertex(triangle.points[0]),vertex(triangle.points[1]),vertex(triangle.points[2]),
                static_cast<SurfaceMaterial>(triangle.material),static_cast<SurfaceNormal>(triangle.normal));
        }
    }
    // The exported collapse excludes its inspection floor. Continue the same
    // ordinary dungeon floor under the sealed, non-walkable stairwell instead.
    addWorldQuad({{-1.92f, kRouteFloorWorldY, 17.48f}}, {{1.92f, kRouteFloorWorldY, 17.48f}},
                 {{1.92f, kRouteFloorWorldY, 3.4f}}, {{-1.92f, kRouteFloorWorldY, 3.4f}}, SurfaceWetCobble, SurfaceUp);
    addCeilingPatch(0u);
    const auto& roofSeam = horde::scene::kShowcaseCollapseRoofSeam;
    addWorldBox(roofSeam.footprint[1][0], roofSeam.bottomY, roofSeam.footprint[1][1],
                roofSeam.footprint[3][0], roofSeam.topY, roofSeam.footprint[3][1], SurfaceMossyStone);
    const auto addRecessedWallX = [&](float x, float logicalX, float minZ, float maxZ,
        float bottom, float top, SurfaceMaterial material, SurfaceNormal normal) {
        for (const auto& panel : horde::scene::TombWallPanels(logicalX, minZ, maxZ,
                bottom, top, developmentRescueJourney_)) {
            if (normal == SurfaceRight)
                addWorldQuad({{x, panel.minimumY, panel.maximumZ}}, {{x, panel.minimumY, panel.minimumZ}},
                    {{x, panel.maximumY, panel.minimumZ}}, {{x, panel.maximumY, panel.maximumZ}}, material, normal);
            else
                addWorldQuad({{x, panel.minimumY, panel.minimumZ}}, {{x, panel.minimumY, panel.maximumZ}},
                    {{x, panel.maximumY, panel.maximumZ}}, {{x, panel.maximumY, panel.minimumZ}}, material, normal);
        }
    };
    addRecessedWallX(-1.85f, -1.85f, -6.4f, 3.4f, kRouteFloorWorldY, 1.35f, SurfaceMossyStone, SurfaceRight);
    addWorldQuad({{1.85f, kRouteFloorWorldY, -6.4f}}, {{1.85f, kRouteFloorWorldY, 3.4f}}, {{1.85f, 1.35f, 3.4f}}, {{1.85f, 1.35f, -6.4f}}, SurfaceMossyStone, SurfaceLeft);
    // The required immutable collapsed-entry asset physically seals this end.
    // Keep the gameplay/collision threshold at 3.4 m while revealing its recess.
    // The former sealed far wall is split around a 1.8 m doorway into the
    // extended showcase route. The matching hidden shell is split below too.
    addWorldQuad({{-1.85f, kRouteFloorWorldY, -6.4f}}, {{-0.90f, kRouteFloorWorldY, -6.4f}}, {{-0.90f, 1.35f, -6.4f}}, {{-1.85f, 1.35f, -6.4f}}, SurfaceMossyStone, SurfaceForward);
    addWorldQuad({{0.90f, kRouteFloorWorldY, -6.4f}}, {{1.85f, kRouteFloorWorldY, -6.4f}}, {{1.85f, 1.35f, -6.4f}}, {{0.90f, 1.35f, -6.4f}}, SurfaceMossyStone, SurfaceForward);
    for (const auto& face : horde::scene::EntryPortalCapFaces())
    {
        const auto vertex = [](const auto& point) {
            return Vertex{{point[0], point[1], point[2]}};
        };
        addWorldQuad(vertex(face.vertices[0]), vertex(face.vertices[1]),
                     vertex(face.vertices[2]), vertex(face.vertices[3]),
                     static_cast<SurfaceMaterial>(face.materialCode),
                     static_cast<SurfaceNormal>(face.normalCode));
    }
    // Give the room-two portal real RT depth instead of three paper-thin cards.
    addWorldBox(-1.20f, kRouteFloorWorldY, -3.55f, -0.78f, 0.95f, -3.25f, SurfaceMossyStone);
    addWorldBox(0.78f, kRouteFloorWorldY, -3.55f, 1.20f, 0.95f, -3.25f, SurfaceMossyStone);
    addOverheadBox(0u);
    // Close the former irregular entry breach with the same opaque roof and
    // shared clearance data. The independent waterfall/finale openings remain.
    addCeilingPatch(1u);
    addCeilingPatch(2u);
    addCeilingPatch(3u);
    addCeilingPatch(4u);
    addCeilingPatch(22u);
    if (!developmentRescueJourney_)
    {
    addWorldQuad({{-1.86f, -0.28f, 1.12f}}, {{-1.86f, 0.46f, 1.12f}}, {{-1.86f, 0.46f, 0.62f}}, {{-1.86f, -0.28f, 0.62f}}, SurfaceFlame, SurfaceRight);
    addWorldQuad({{1.86f, -0.35f, -1.98f}}, {{1.86f, -0.35f, -1.48f}}, {{1.86f, 0.38f, -1.48f}}, {{1.86f, 0.38f, -1.98f}}, SurfaceFlame, SurfaceLeft);
    addWorldQuad({{-1.84f, -0.32f, -0.82f}}, {{-1.84f, 0.24f, -0.62f}}, {{-1.84f, 0.42f, -1.12f}}, {{-1.84f, -0.12f, -1.34f}}, SurfaceMirror, SurfaceRight);
    addWorldQuad({{1.84f, -0.44f, 0.24f}}, {{1.84f, -0.02f, 0.5f}}, {{1.84f, 0.28f, 0.1f}}, {{1.84f, -0.18f, -0.2f}}, SurfaceMirror, SurfaceLeft);
    addWorldQuad({{-0.52f, -0.94f, -0.86f}}, {{0.34f, -0.94f, -0.64f}}, {{0.64f, -0.94f, -1.18f}}, {{-0.38f, -0.94f, -1.42f}}, SurfaceAgedMetal, SurfaceUp);
    for (std::uint32_t i = 0u; i < 8u; ++i)
    {
        const float x = -1.05f + static_cast<float>(i % 4u) * 0.7f + (i >= 4u ? 0.18f : 0.0f);
        const float z = -2.55f - static_cast<float>(i / 4u) * 1.1f - static_cast<float>(i % 2u) * 0.24f;
        const float h = 0.58f + static_cast<float>(i % 3u) * 0.12f;
        addWorldQuad({{x - 0.18f, kRouteFloorWorldY, z}}, {{x + 0.18f, kRouteFloorWorldY, z}}, {{x + 0.14f, kRouteFloorWorldY + h, z}}, {{x - 0.14f, kRouteFloorWorldY + h, z}}, SurfaceDarkFigure, SurfaceForward);
    }

    // The development tomb reuses the stone table as a burial bier with a
    // recovered funerary lid. Legacy material swatches remain diagnostic fixtures.
    }
    constexpr float galleryMinX = -1.55f;
    constexpr float galleryMaxX = -0.72f;
    constexpr float galleryMinY = kRouteFloorWorldY;
    constexpr float galleryMaxY = -0.58f;
    constexpr float galleryMinZ = 0.05f;
    constexpr float galleryMaxZ = 2.35f;
    addWorldQuad({{galleryMinX, galleryMinY, galleryMinZ}}, {{galleryMinX, galleryMaxY, galleryMinZ}}, {{galleryMaxX, galleryMaxY, galleryMinZ}}, {{galleryMaxX, galleryMinY, galleryMinZ}}, SurfaceDryStone, SurfaceBack);
    addWorldQuad({{galleryMaxX, galleryMinY, galleryMaxZ}}, {{galleryMaxX, galleryMaxY, galleryMaxZ}}, {{galleryMinX, galleryMaxY, galleryMaxZ}}, {{galleryMinX, galleryMinY, galleryMaxZ}}, SurfaceDryStone, SurfaceForward);
    addWorldQuad({{galleryMinX, galleryMinY, galleryMaxZ}}, {{galleryMinX, galleryMaxY, galleryMaxZ}}, {{galleryMinX, galleryMaxY, galleryMinZ}}, {{galleryMinX, galleryMinY, galleryMinZ}}, SurfaceDryStone, SurfaceLeft);
    addWorldQuad({{galleryMaxX, galleryMinY, galleryMinZ}}, {{galleryMaxX, galleryMaxY, galleryMinZ}}, {{galleryMaxX, galleryMaxY, galleryMaxZ}}, {{galleryMaxX, galleryMinY, galleryMaxZ}}, SurfaceDryStone, SurfaceRight);
    addWorldQuad({{galleryMinX, galleryMaxY, galleryMinZ}}, {{galleryMinX, galleryMaxY, galleryMaxZ}}, {{galleryMaxX, galleryMaxY, galleryMaxZ}}, {{galleryMaxX, galleryMaxY, galleryMinZ}}, SurfaceDryStone, SurfaceUp);
    addWorldQuad({{galleryMinX, galleryMinY, galleryMaxZ}}, {{galleryMinX, galleryMinY, galleryMinZ}}, {{galleryMaxX, galleryMinY, galleryMinZ}}, {{galleryMaxX, galleryMinY, galleryMaxZ}}, SurfaceDryStone, SurfaceDown);
    if (!developmentRescueJourney_)
    {
    const std::array<SurfaceMaterial, 5u> galleryMaterials{{SurfaceDryStone, SurfaceWetCobble, SurfaceMossyStone, SurfaceDampGround, SurfaceAgedMetal}};
    for (std::size_t i = 0u; i < galleryMaterials.size(); ++i)
    {
        const float z = 2.05f - static_cast<float>(i) * 0.43f;
        addWorldQuad({{-0.80f, -0.54f, z + 0.16f}}, {{-0.80f, -0.54f, z - 0.16f}}, {{-1.22f, -0.08f, z - 0.16f}}, {{-1.22f, -0.08f, z + 0.16f}}, galleryMaterials[i], SurfaceGalleryCant);
    }

    }

    // A thin hidden shell behind the zero-thickness room planes catches rays
    // that start near a join and skip the adjoining face because of ray tMin.
    // It leaves the room-two roof breach unobstructed. Appending it here
    // preserves every existing material index.
    addRecessedWallX(-1.92f, -1.85f, -6.47f, 3.4f, -1.02f, 1.42f, SurfaceHiddenShell, SurfaceRight);
    addWorldQuad({{1.92f, -1.02f, -6.47f}}, {{1.92f, -1.02f, 3.4f}}, {{1.92f, 1.42f, 3.4f}}, {{1.92f, 1.42f, -6.47f}}, SurfaceHiddenShell, SurfaceLeft);
    addWorldQuad({{-1.92f, -1.02f, 3.4f}}, {{1.92f, -1.02f, 3.4f}}, {{1.92f, -1.02f, -6.47f}}, {{-1.92f, -1.02f, -6.47f}}, SurfaceHiddenShell, SurfaceUp);
    addWorldQuad({{-1.92f, -1.02f, -6.47f}}, {{-0.90f, -1.02f, -6.47f}}, {{-0.90f, 1.42f, -6.47f}}, {{-1.92f, 1.42f, -6.47f}}, SurfaceHiddenShell, SurfaceForward);
    addWorldQuad({{0.90f, -1.02f, -6.47f}}, {{1.92f, -1.02f, -6.47f}}, {{1.92f, 1.42f, -6.47f}}, {{0.90f, 1.42f, -6.47f}}, SurfaceHiddenShell, SurfaceForward);
    // Slice A extends the room with static, geometry-only RT proof spaces. It
    // deliberately adds no new flame, glass or mirror surface: the brackets
    // are unlit, the transmission frame is empty and the final mirror frame
    // has only its dry-stone wall behind it.
    constexpr float routeCeiling = horde::scene::kShowcaseRouteCeilingWorldY;
    const auto addRouteFloor = [&addWorldQuad](float minX, float minZ, float maxX, float maxZ, SurfaceMaterial material) {
        addWorldQuad({{minX, kRouteFloorWorldY, maxZ}}, {{maxX, kRouteFloorWorldY, maxZ}},
                     {{maxX, kRouteFloorWorldY, minZ}}, {{minX, kRouteFloorWorldY, minZ}}, material, SurfaceUp);
    };
    const auto addRouteWallX = [&addWorldQuad](float x, float minZ, float maxZ, SurfaceNormal normal) {
        if (normal == SurfaceRight)
        {
            addWorldQuad({{x, kRouteFloorWorldY, maxZ}}, {{x, kRouteFloorWorldY, minZ}},
                         {{x, routeCeiling, minZ}}, {{x, routeCeiling, maxZ}}, SurfaceMossyStone, normal);
        }
        else
        {
            addWorldQuad({{x, kRouteFloorWorldY, minZ}}, {{x, kRouteFloorWorldY, maxZ}},
                         {{x, routeCeiling, maxZ}}, {{x, routeCeiling, minZ}}, SurfaceMossyStone, normal);
        }
    };
    const auto addRouteWallZ = [&addWorldQuad](float z, float minX, float maxX, SurfaceNormal normal) {
        if (normal == SurfaceForward)
        {
            addWorldQuad({{minX, kRouteFloorWorldY, z}}, {{maxX, kRouteFloorWorldY, z}},
                         {{maxX, routeCeiling, z}}, {{minX, routeCeiling, z}}, SurfaceMossyStone, normal);
        }
        else
        {
            addWorldQuad({{minX, kRouteFloorWorldY, z}}, {{minX, routeCeiling, z}},
                         {{maxX, routeCeiling, z}}, {{maxX, kRouteFloorWorldY, z}}, SurfaceMossyStone, normal);
        }
    };

    // A shallow stone surround gives the new 1.8 m opening real RT depth. Its
    // clear width stays exactly x=-0.9..0.9 throughout the player's height.
    addWorldBox(-1.08f, kRouteFloorWorldY, -6.52f, -0.90f, 0.82f, -6.28f, SurfaceMossyStone);
    addWorldBox(0.90f, kRouteFloorWorldY, -6.52f, 1.08f, 0.82f, -6.28f, SurfaceMossyStone);
    addOverheadBox(2u);
    addOverheadBox(3u);
    addOverheadBox(4u);

    // Four overlapping 2.4 m legs form the three-turn shadow corridor. The
    // coplanar overlaps carry the same material metadata and prevent cracks at
    // the bends without introducing thin filler triangles.
    addRouteFloor(-1.20f, -10.0f, 1.20f, -6.4f, SurfaceWetCobble);
    addCeilingPatch(5u);
    addRouteFloor(0.0f, -11.2f, 4.80f, -8.8f, SurfaceWetCobble);
    addCeilingPatch(6u);
    addRouteFloor(3.60f, -15.2f, 6.0f, -10.0f, SurfaceWetCobble);
    addCeilingPatch(7u);
    addRouteFloor(-2.50f, -16.4f, 4.80f, -14.0f, SurfaceWetCobble);
    // A broken roof slot turns the former abstract torch failure into a
    // physical drench at the final zig-zag exit. The rim is deliberately
    // inside the existing route envelope so collision and replay remain
    // unchanged while primary, reflection, and transmission rays see depth.
    // Extend the irregular breach to contain the actual kMoonDirection
    // footprint at the catchment. The moon ray now reaches the cobbles through
    // geometry and can be blocked by the player; no water-only light is added.
    constexpr float waterSlotMinX = -2.90f;
    constexpr float waterSlotMaxX = -1.58f;
    constexpr float waterSlotMinZ = -16.10f;
    constexpr float waterSlotMaxZ = -14.72f;
    addCeilingPatch(8u);
    addCeilingPatch(9u);
    addCeilingPatch(10u);
    addCeilingPatch(11u);

    // Outer perimeter of the corridor union. Open seams at x=-2.5 and x=-8.5
    // connect directly into the skylight room and torch passage respectively.
    addRouteWallX(-1.20f, -10.0f, -6.4f, SurfaceRight);
    addRouteWallZ(-10.0f, -1.20f, 0.0f, SurfaceForward);
    addRouteWallX(0.0f, -11.2f, -10.0f, SurfaceRight);
    addRouteWallX(1.20f, -8.8f, -6.4f, SurfaceLeft);
    addRouteWallZ(-8.8f, 1.20f, 2.05f, SurfaceBack);
    addRouteWallZ(-8.8f, 3.10f, 4.80f, SurfaceBack);
    addRouteWallX(4.80f, -10.0f, -8.8f, SurfaceLeft);
    addRouteWallZ(-11.2f, 0.0f, 3.60f, SurfaceForward);
    addRouteWallZ(-10.0f, 4.80f, 6.0f, SurfaceBack);
    addRouteWallX(3.60f, -14.0f, -11.2f, SurfaceRight);
    addRecessedWallX(6.0f, 6.0f, -15.2f, -10.0f, kRouteFloorWorldY, routeCeiling, SurfaceMossyStone, SurfaceLeft);
    addRouteWallZ(-15.2f, 4.80f, 6.0f, SurfaceForward);
    addRouteWallZ(-14.0f, -2.50f, 3.60f, SurfaceBack);
    addRouteWallX(4.80f, -16.4f, -15.2f, SurfaceLeft);
    addRouteWallZ(-16.4f, -2.50f, 4.80f, SurfaceForward);

    constexpr float waterShaftBase = routeCeiling - 0.02f;
    addWorldBox(waterSlotMinX - 0.16f, waterShaftBase, waterSlotMinZ - 0.16f,
                waterSlotMinX, horde::scene::kWaterShaftTopWorldY, waterSlotMaxZ + 0.16f, SurfaceMossyStone);
    addWorldBox(waterSlotMaxX, waterShaftBase, waterSlotMinZ - 0.16f,
                waterSlotMaxX + 0.16f, horde::scene::kWaterShaftTopWorldY, waterSlotMaxZ + 0.16f, SurfaceMossyStone);
    addWorldBox(waterSlotMinX, waterShaftBase, waterSlotMinZ - 0.16f,
                waterSlotMaxX, horde::scene::kWaterShaftTopWorldY, waterSlotMinZ, SurfaceMossyStone);
    addWorldBox(waterSlotMinX, waterShaftBase, waterSlotMaxZ,
                waterSlotMaxX, horde::scene::kWaterShaftTopWorldY, waterSlotMaxZ + 0.16f, SurfaceMossyStone);

    // Physical restrained growth hangs from the deep masonry shaft rim.
    // Closed stems and thick leaf silhouettes share normal/material metadata
    // and all ordinary RT occlusion/reflection paths; no alpha card or light.
    const auto leafMaterialIndexPlusOne = worldMaterialBase_ + 2u;
    for (const auto& sprig : horde::scene::kWaterShaftSprigs)
    {
        const auto dressing = horde::scene::MakeHangingSprig(sprig.attachment, sprig.length, sprig.phase);
        for (const auto& face : dressing)
        {
            const auto vertex = [&face](std::size_t index) {
                const auto& point = face.vertices[index];
                return Vertex{{point[0], point[1], point[2]}};
            };
            addWorldQuad(vertex(0), vertex(1), vertex(2), vertex(3), SurfaceMossyStone,
                         static_cast<SurfaceNormal>(face.normalCode), leafMaterialIndexPlusOne);
        }
    }

    // Only the three falling streams live in this dedicated local-space mesh.
    // Catchment, runnel and drain stay in the static world and collision is
    // unchanged. The TLAS transform scales local X around the authored centre.
    // The closed, faceted streams replace the former broad water cards. The
    // eight-sided elliptical rings have real depth for Fresnel/refraction and
    // real air gaps between them; no shader coverage mask or overlay is used.
    // The main thin stream brushes the authored z=-15.20 route while two narrow
    // satellites break up its silhouette without sealing the doorway. All three
    // share a linear taper: falling water accelerates and narrows toward the pool,
    // and the shader can solve the matching elliptical exit surface exactly.
    struct WaterStream
    {
        float centreZ;
        float radiusX;
        float radiusZ;
    };
    constexpr std::array<float, 6u> waterStreamY{{2.16f, 1.58f, 1.01f, 0.43f, -0.18f, -0.91f}};
    constexpr std::array<float, 8u> ringCos{{
        1.0f, 0.70710678f, 0.0f, -0.70710678f,
        -1.0f, -0.70710678f, 0.0f, 0.70710678f,
    }};
    constexpr std::array<float, 8u> ringSin{{
        0.0f, 0.70710678f, 1.0f, 0.70710678f,
        0.0f, -0.70710678f, -1.0f, -0.70710678f,
    }};
    constexpr std::array<WaterStream, 3u> waterStreams{{
        {-15.26f, 0.006f, 0.065f},
        {-15.44f, 0.003f, 0.014f},
        {-15.06f, 0.003f, 0.012f},
    }};
    static_assert(waterStreamY.size() == 6u && ringCos.size() == 8u &&
                  waterStreams.size() == 3u,
                  "falling water must retain six rings and three eight-facet streams");
    static_assert((-15.26f - 0.065f) - (-15.44f + 0.014f) > 0.10f &&
                  (-15.06f - 0.012f) - (-15.26f + 0.065f) > 0.10f,
                  "falling water streams must retain real ten-centimetre air gaps");
    for (const WaterStream& stream : waterStreams)
    {
        std::array<std::array<Vertex, 8u>, 6u> rings{};
        for (std::size_t level = 0u; level < waterStreamY.size(); ++level)
        {
            const float fallProgress = std::clamp(
                (waterStreamY[level] + 0.91f) / (2.16f + 0.91f), 0.0f, 1.0f);
            const float radiusScale = 0.70f + fallProgress * 0.30f;
            for (std::size_t facet = 0u; facet < ringCos.size(); ++facet)
            {
                rings[level][facet] = Vertex{{
                    ringCos[facet] * stream.radiusX * radiusScale,
                    waterStreamY[level],
                    (stream.centreZ + 15.26f) + ringSin[facet] * stream.radiusZ * radiusScale,
                }};
            }
        }
        for (std::size_t segment = 0u; segment + 1u < waterStreamY.size(); ++segment)
        {
            for (std::size_t facet = 0u; facet < ringCos.size(); ++facet)
            {
                const std::size_t nextFacet = (facet + 1u) % ringCos.size();
                addWaterfallQuad(rings[segment + 1u][facet], rings[segment][facet],
                                 rings[segment][nextFacet], rings[segment + 1u][nextFacet]);
            }
        }
    }
    // The authoritative contact triangles also feed the fixed RT mesh. This
    // keeps wet-event membership on the exact surfaces the camera can see.
    for (const auto& triangle : horde::gameplay::effects::WaterSurfaceTriangles())
    {
        addWorldTriangle(Vertex{{triangle[0][0], triangle[0][1], triangle[0][2]}},
                         Vertex{{triangle[1][0], triangle[1][1], triangle[1][2]}},
                         Vertex{{triangle[2][0], triangle[2][1], triangle[2][2]}},
                         SurfaceWater, SurfaceUp);
    }

    // Dark throat and metal crossbars make the sink legible through the clear
    // runoff while the final sloped water surface visibly disappears below
    // the floor plane.
    addWorldQuad({{-5.30f, -0.944f, -14.94f}}, {{-4.91f, -0.944f, -14.94f}},
                 {{-4.91f, -0.944f, -15.46f}}, {{-5.30f, -0.944f, -15.46f}},
                 SurfaceDarkFigure, SurfaceUp);
    for (std::uint32_t bar = 0u; bar < 4u; ++bar)
    {
        const float drainZ = -15.40f + static_cast<float>(bar) * 0.14f;
        addWorldBox(-5.31f, -0.922f, drainZ, -4.90f, -0.900f, drainZ + 0.028f,
                    SurfaceAgedMetal);
    }

    // A shallow barred recess at the first bend creates strong moving shadow
    // lines while remaining outside the shared walkable rectangles.
    addRouteWallZ(-8.35f, 2.05f, 3.10f, SurfaceBack);
    addRouteWallX(2.05f, -8.8f, -8.35f, SurfaceRight);
    addRouteWallX(3.10f, -8.8f, -8.35f, SurfaceLeft);
    // Close the unintended downward escape beneath the grate. The closed
    // stone base joins the route floor and embeds into all three retained
    // walls; the bars, walkable bounds and upper light aperture are unchanged.
    for (const auto& base : horde::scene::kWallPanelBottomSolids)
    {
        addWorldBox(base[0][0], base[0][1], base[0][2],
                    base[1][0], base[1][1], base[1][2], SurfaceMossyStone);
    }
    // Extend this small entry-side panel into an open masonry light well.
    // Four real walls rise 2.75 m above its retained roof/jambs, preserving the
    // clear opening, bars and growth below. Consolidate into the existing world
    // BLAS: no extra instance, material, light or special shading response.
    for (const auto& wall : horde::scene::kWallPanelMasonryWell)
    {
        addWorldBox(wall.footprint[1][0], wall.bottomY, wall.footprint[1][1],
                    wall.footprint[3][0], wall.topY, wall.footprint[3][1], SurfaceMossyStone);
    }
    for (std::uint32_t i = 0u; i < 4u; ++i)
    {
        const float x = 2.20f + static_cast<float>(i) * 0.25f;
        addWorldBox(x, -0.78f, -8.82f, x + 0.045f, 0.82f, -8.76f, SurfaceAgedMetal);
    }
    // Two masonry-rooted side sprigs frame the retained central bars. Their
    // wall-facing leaves are real closed geometry using the existing moss tint.
    for (const auto& sprig : horde::scene::kWallPanelSprigs)
    {
        const auto dressing = horde::scene::MakeHangingSprig(
            sprig.attachment, sprig.length, sprig.phase, sprig.leafPlane);
        for (const auto& face : dressing)
        {
            const auto vertex = [&face](std::size_t index) {
                const auto& point = face.vertices[index];
                return Vertex{{point[0], point[1], point[2]}};
            };
            addWorldQuad(vertex(0), vertex(1), vertex(2), vertex(3), SurfaceMossyStone,
                         static_cast<SurfaceNormal>(face.normalCode), leafMaterialIndexPlusOne);
        }
    }

    // Keep the turns open and let the wall returns plus barred recess cast the
    // large torch shadows. Earlier low lintel boxes crossed the route walls,
    // producing the dark overhead slabs and coplanar striping seen in validation.

    // The skylight chamber floor is damp stone. Four roof slabs leave the
    // planned x=-6.7..-4.3, z=-16.6..-13.8 aperture physically open to sky.
    addRouteFloor(-8.50f, -18.0f, -2.50f, -12.4f, SurfaceDampGround);
    addCeilingPatch(12u);
    addCeilingPatch(13u);
    addCeilingPatch(14u);
    addCeilingPatch(15u);
    // A raised masonry well gives the aperture a readable 1.1 m depth from
    // oblique views. The inner clear opening remains exactly the planned
    // x=-6.7..-4.3, z=-16.6..-13.8 footprint all the way to open sky.
    // Sink the bases below the ceiling plane so the join stays closed without
    // leaving coplanar bottom faces to shimmer against the roof slabs.
    constexpr float shaftBase = routeCeiling - 0.02f;
    addWorldBox(-6.86f, shaftBase, -16.76f, -6.70f, 2.45f, -13.64f, SurfaceMossyStone);
    addWorldBox(-4.30f, shaftBase, -16.76f, -4.14f, 2.45f, -13.64f, SurfaceMossyStone);
    addWorldBox(-6.70f, shaftBase, -16.76f, -4.30f, 2.45f, -16.60f, SurfaceMossyStone);
    addWorldBox(-6.70f, shaftBase, -13.80f, -4.30f, 2.45f, -13.64f, SurfaceMossyStone);
    // Supported iron grid belongs only to the separate large skylight. Every
    // bar is a closed ordinary RT solid with its ends embedded in the rim.
    for (const auto& bar : horde::scene::kShowcaseSkylightGrid)
    {
        addWorldBox(bar.footprint[1][0], bar.bottomY, bar.footprint[1][1],
                    bar.footprint[2][0], bar.topY, bar.footprint[0][1], SurfaceAgedMetal);
    }
    addRouteWallZ(-12.4f, -8.50f, -2.50f, SurfaceBack);
    addRouteWallZ(-18.0f, -8.50f, -2.50f, SurfaceForward);
    addRouteWallX(-2.50f, -18.0f, -16.4f, SurfaceLeft);
    addRouteWallX(-2.50f, -14.0f, -12.4f, SurfaceLeft);
    addRouteWallX(-8.50f, -18.0f, -16.8f, SurfaceRight);
    addRouteWallX(-8.50f, -13.6f, -12.4f, SurfaceRight);

    // One straight passage contains four five-metre bays. Brackets are aged
    // metal only: no SurfaceFlame primitive and no coloured illumination are
    // introduced in this blockout slice.
    addRouteFloor(-28.50f, -16.8f, -8.50f, -13.6f, SurfaceWetCobble);
    addCeilingPatch(16u);
    addRouteFloor(-30.50f, -16.8f, -28.50f, -13.6f, SurfaceDryStone);
    addCeilingPatch(17u);
    addRouteWallZ(-13.6f, -30.50f, -8.50f, SurfaceBack);
    addRouteWallZ(-16.8f, -30.50f, -8.50f, SurfaceForward);
    const std::array<float, 4u> torchBayCenters{{-11.0f, -16.0f, -21.0f, -26.0f}};
    for (float x : torchBayCenters)
    {
        addWorldBox(x - 0.12f, 0.24f, -13.66f, x + 0.12f, 0.82f, -13.54f, SurfaceAgedMetal);
        addWorldBox(x - 0.035f, 0.38f, -13.98f, x + 0.035f, 0.46f, -13.62f, SurfaceAgedMetal);
        addWorldBox(x - 0.12f, 0.42f, -14.04f, x + 0.12f, 0.50f, -13.92f, SurfaceAgedMetal);
        // The route-light selector activates one bay's direct-light estimate at
        // a time, but every sconce retains a small physical emissive flame.
        addWorldBox(x - 0.055f, 0.50f, -14.015f, x + 0.055f, 0.78f, -13.945f, SurfaceFlame);
    }

    // The authored threshold remains open. Its narrow jambs sit inside the
    // collision wall inset and the high lintel leaves the full central walking
    // lane unobstructed.
    addWorldBox(-29.62f, kRouteFloorWorldY, -16.80f, -29.38f, 0.88f, -16.62f, SurfaceMossyStone);
    addWorldBox(-29.62f, kRouteFloorWorldY, -13.78f, -29.38f, 0.88f, -13.60f, SurfaceMossyStone);
    addOverheadBox(5u);

    // Dry final reveal room. The far-wall metal surround is an empty hero
    // mirror frame; its centre remains ordinary dry stone in Slice A.
    addRouteFloor(-36.90f, -18.4f, -30.50f, -12.0f, SurfaceDryStone);
    // Upright admitted torch bodies rest on generic stone/iron floor stands.
    // The supports are ordinary world triangles, outside the combat/chest lane;
    // neither reveal nor extinguish rebuilds their immutable world geometry.
    for (const auto& anchor : horde::gameplay::effects::kKeeperTorchAnchors)
    {
        const float x = anchor.position[0];
        const float z = anchor.position[2];
        const float baseHalfExtent = horde::gameplay::kKeeperTorchStandBaseHalfExtent;
        addWorldBox(x - baseHalfExtent, kRouteFloorWorldY, z - baseHalfExtent,
                    x + baseHalfExtent, kRouteFloorWorldY + 0.13f, z + baseHalfExtent, SurfaceMossyStone);
        addWorldBox(x - 0.035f, kRouteFloorWorldY + 0.13f, z - 0.035f,
                    x + 0.035f, anchor.position[1], z + 0.035f, SurfaceAgedMetal);
        addWorldBox(x - 0.08f, anchor.position[1] - 0.05f, z - 0.08f,
                    x + 0.08f, anchor.position[1], z + 0.08f, SurfaceAgedMetal);
    }
    // Four fixed roof slabs leave a real finale aperture. A separate BLAS panel
    // below closes it until the defeated lich's authored roof sequence slides
    // the slab west under the surrounding masonry.
    addCeilingPatch(18u);
    addCeilingPatch(19u);
    addCeilingPatch(20u);
    addCeilingPatch(21u);
    constexpr float finaleShaftBase = routeCeiling - 0.02f;
    addWorldBox(-35.05f, finaleShaftBase, -16.75f, -34.90f, 1.72f, -13.65f, SurfaceMossyStone);
    addWorldBox(-32.50f, finaleShaftBase, -16.75f, -32.35f, 1.72f, -13.65f, SurfaceMossyStone);
    addWorldBox(-34.90f, finaleShaftBase, -16.75f, -32.50f, 1.72f, -16.60f, SurfaceMossyStone);
    addWorldBox(-34.90f, finaleShaftBase, -13.80f, -32.50f, 1.72f, -13.65f, SurfaceMossyStone);
    // A physical aged-metal ceiling fixture establishes the source of the
    // post-lich guidance light. It remains unlit while the seal breaks; the
    // shared chest phase activates the matching ray-query-shadowed light.
    constexpr float chestLampX = horde::gameplay::kRewardChestRoutePosition.x;
    constexpr float chestLampZ = horde::gameplay::kRewardChestRoutePosition.z;
    addWorldBox(chestLampX - 0.018f, 1.18f, chestLampZ - 0.018f,
                chestLampX + 0.018f, 1.34f, chestLampZ + 0.018f,
                SurfaceAgedMetal);
    addWorldBox(chestLampX - 0.14f, 1.08f, chestLampZ - 0.14f,
                chestLampX + 0.14f, 1.18f, chestLampZ + 0.14f,
                SurfaceAgedMetal);
    addRouteWallZ(-12.0f, -36.90f, -30.50f, SurfaceBack);
    addRouteWallZ(-18.4f, -36.90f, -30.50f, SurfaceForward);
    addRouteWallX(-30.50f, -18.4f, -16.8f, SurfaceLeft);
    addRouteWallX(-30.50f, -13.6f, -12.0f, SurfaceLeft);
    addRouteWallX(-36.90f, -18.4f, -12.0f, SurfaceRight);
    addWorldBox(-36.88f, -0.62f, -16.62f, -36.74f, 0.86f, -16.46f, SurfaceAgedMetal);
    addWorldBox(-36.88f, -0.62f, -13.94f, -36.74f, 0.86f, -13.78f, SurfaceAgedMetal);
    addWorldBox(-36.88f, -0.62f, -16.62f, -36.74f, -0.46f, -13.78f, SurfaceAgedMetal);
    addWorldBox(-36.88f, 0.70f, -16.62f, -36.74f, 0.86f, -13.78f, SurfaceAgedMetal);
    addWorldQuad({{-36.72f, -0.44f, -16.44f}}, {{-36.72f, 0.68f, -16.44f}},
                 {{-36.72f, 0.68f, -13.96f}}, {{-36.72f, -0.44f, -13.96f}},
                 SurfaceMirror, SurfaceRight);

    if(developmentRescueJourney_) {
        rescueRopeVertexOffset_=vertices.size();
        horde::gameplay::traversal::RescueTraversal initial;
        const auto rope=horde::scene::RescueRopeTriangleVertices(initial.Snapshot());
        for(std::size_t i=0;i<rope.size();i+=3) {
            auto p=rope[i],q=rope[i+1],r=rope[i+2];
            // Undeployed full topology remains below the closed lower floor.
            p[1]-=8;q[1]-=8;r[1]-=8;
            addWorldTriangle(Vertex{{p[0],p[1],p[2]}},Vertex{{q[0],q[1],q[2]}},Vertex{{r[0],r[1],r[2]}},SurfaceDryStone,
                static_cast<SurfaceNormal>(horde::scene::RescueRopeTriangleNormalCode(p,q,r)));
        }
        rescueWorldSurfaceCodes_=worldSurfaceCodes;
        rescueWorldVertices_.clear();
        for(const auto& vertex:vertices) rescueWorldVertices_.push_back({vertex.position[0],vertex.position[1],vertex.position[2]});
    }
    const std::uint32_t sceneIndexCount = static_cast<std::uint32_t>(indices.size());
    rescueWorldPrimitiveCount_=sceneIndexCount/3u;
    if (worldSurfaceCodes.size() != sceneIndexCount / 3u)
    {
        diagnostic = "World surface metadata does not match the world triangle count.";
        return false;
    }

    const std::uint32_t waterfallFirstVertex = static_cast<std::uint32_t>(vertices.size());
    const std::uint32_t waterfallIndexOffset = static_cast<std::uint32_t>(indices.size());
    vertices.insert(vertices.end(), waterfallVertices.begin(), waterfallVertices.end());
    indices.insert(indices.end(), waterfallIndices.begin(), waterfallIndices.end());
    const std::uint32_t waterfallIndexCount = static_cast<std::uint32_t>(indices.size());

    // Closed-position sliding roof slab. Its TLAS transform moves west after
    // the lich's death animation, physically exposing the sky to primary and
    // visibility rays rather than fading a ceiling texture away.
    if(developmentRescueJourney_) {
        const auto& lid=horde::scene::kRescueBlockoutLid;
        addBox(lid.minimum[0],lid.minimum[1],lid.minimum[2],lid.maximum[0],lid.maximum[1],lid.maximum[2]);
    } else addBox(-34.90f, 1.30f, -16.60f, -32.50f, 1.42f, -13.80f);
    const std::uint32_t finaleRoofIndexCount = static_cast<std::uint32_t>(indices.size());

    // The production torch body comes through the generic static GLB/PBR slot.
    // Retain only the temporary engine-owned faceted flame core for Task 4.
    horde::gameplay::items::HeldItemTransform itemFromEngineFlame =
        horde::gameplay::items::IdentityHeldItemTransform();
    const auto transformFlameVertex = [&itemFromEngineFlame](const Vertex& vertex) {
        const auto& p = vertex.position;
        return Vertex{{
            itemFromEngineFlame[0] * p[0] + itemFromEngineFlame[4] * p[1] +
                itemFromEngineFlame[8] * p[2] + itemFromEngineFlame[12],
            itemFromEngineFlame[1] * p[0] + itemFromEngineFlame[5] * p[1] +
                itemFromEngineFlame[9] * p[2] + itemFromEngineFlame[13],
            itemFromEngineFlame[2] * p[0] + itemFromEngineFlame[6] * p[1] +
                itemFromEngineFlame[10] * p[2] + itemFromEngineFlame[14]}};
    };
    const auto addFacetedFlame = [&addTriangle, &transformFlameVertex](
        float radius, float bottom, float waist, float top) {
        const Vertex lower = transformFlameVertex(Vertex{{0.0f, bottom, 0.0f}});
        const Vertex upper = transformFlameVertex(Vertex{{0.0f, top, 0.0f}});
        const std::array<Vertex, 4u> ring{{
            transformFlameVertex(Vertex{{radius, waist, 0.0f}}),
            transformFlameVertex(Vertex{{0.0f, waist, radius}}),
            transformFlameVertex(Vertex{{-radius, waist, 0.0f}}),
            transformFlameVertex(Vertex{{0.0f, waist, -radius}})}};
        for (std::size_t i = 0u; i < ring.size(); ++i)
        {
            const Vertex& current = ring[i];
            const Vertex& next = ring[(i + 1u) % ring.size()];
            addTriangle(lower, next, current);
            addTriangle(upper, current, next);
        }
    };
    if (productionHeldItemAssetsEnabled_)
    {
        const auto* flameSocket = horde::gameplay::items::FindHeldItemSocket(
            playerTorchAsset_.sockets, "Flame");
        if (flameSocket == nullptr)
        {
            diagnostic = "Player Rag torch is missing the exact Flame socket.";
            return false;
        }
        itemFromEngineFlame = flameSocket->world;
    }
    if (productionHeldItemAssetsEnabled_)
    {
        // One tiny static emissive core stays inside the generic torch BLAS.
        // The reusable tapered volume, smoke, and analytic embers are shader
        // records, so extinguishing never rebuilds/refits this geometry.
        addFacetedFlame(0.030f, -0.025f, 0.010f, 0.095f);
    }
    else
    {
        addFacetedFlame(0.095f, 0.16f, 0.31f, 0.58f);
        addFacetedFlame(0.050f, 0.19f, 0.30f, 0.47f);
    }
    const std::uint32_t torchIndexCount = static_cast<std::uint32_t>(indices.size());

    // The procedural sword proof is retired; instance 3 always uses the
    // production static GLB/PBR registration.
    const std::uint32_t swordIndexCount = static_cast<std::uint32_t>(indices.size());

    // A layered low-poly travelling coat replaces the original two-box torso
    // without adding a new BLAS or TLAS instance. The two main sections taper
    // away from the camera instead of presenting a broad chest slab, while
    // primitive ranges stay stable for raygen material assignment.
    const auto addTaperedCoatSection = [&addQuad](float topHalfWidth,
                                                  float bottomHalfWidth,
                                                  float topY,
                                                  float bottomY,
                                                  float backZ,
                                                  float frontZ) {
        const Vertex backTopLeft{{-topHalfWidth, topY, backZ}};
        const Vertex backTopRight{{topHalfWidth, topY, backZ}};
        const Vertex backBottomLeft{{-bottomHalfWidth, bottomY, backZ}};
        const Vertex backBottomRight{{bottomHalfWidth, bottomY, backZ}};
        const Vertex frontTopLeft{{-topHalfWidth, topY, frontZ}};
        const Vertex frontTopRight{{topHalfWidth, topY, frontZ}};
        const Vertex frontBottomLeft{{-bottomHalfWidth, bottomY, frontZ}};
        const Vertex frontBottomRight{{bottomHalfWidth, bottomY, frontZ}};
        addQuad(backBottomLeft, backBottomRight, backTopRight, backTopLeft);
        addQuad(frontBottomRight, frontBottomLeft, frontTopLeft, frontTopRight);
        addQuad(backBottomLeft, backTopLeft, frontTopLeft, frontBottomLeft);
        addQuad(frontBottomRight, frontTopRight, backTopRight, backBottomRight);
        addQuad(backTopLeft, backTopRight, frontTopRight, frontTopLeft);
        addQuad(frontBottomLeft, frontBottomRight, backBottomRight, backBottomLeft);
    };
    addTaperedCoatSection(0.245f, 0.16f, -0.49f, -0.82f, 0.39f, 0.56f); // 0-11 lower coat
    addTaperedCoatSection(0.22f, 0.28f, -0.22f, -0.56f, 0.36f, 0.56f);   // 12-23 chest
    addBox(-0.34f, -0.50f, 0.37f, -0.20f, -0.30f, 0.58f); // 24-35 left shoulder
    addBox(0.20f, -0.50f, 0.37f, 0.34f, -0.30f, 0.58f);   // 36-47 right shoulder
    addBox(-0.245f, -0.60f, 0.33f, 0.245f, -0.51f, 0.57f); // 48-59 belt
    addBox(-0.055f, -0.61f, 0.565f, 0.055f, -0.49f, 0.605f); // 60-71 buckle
    addBox(-0.17f, -0.31f, 0.36f, -0.035f, -0.17f, 0.55f); // 72-83 left collar
    addBox(0.035f, -0.31f, 0.36f, 0.17f, -0.17f, 0.55f);   // 84-95 right collar
    addBox(-0.13f, -0.98f, 0.50f, -0.035f, -0.70f, 0.59f); // 96-107 left tail
    addBox(0.035f, -0.98f, 0.50f, 0.13f, -0.70f, 0.59f);   // 108-119 right tail
    addQuad({{-0.20f, -0.19f, 0.585f}}, {{-0.13f, -0.19f, 0.585f}},
            {{0.20f, -0.48f, 0.585f}}, {{0.13f, -0.48f, 0.585f}}); // 120-121 strap
    const std::uint32_t playerBodyIndexCount = static_cast<std::uint32_t>(indices.size());

    // A six-sided bevel-ended capsule along +Z gives every articulated limb a
    // recognisable silhouette while retaining one shared phone-cheap limb BLAS.
    constexpr std::size_t limbSides = 6u;
    constexpr std::array<float, 4u> limbRingZ{0.0f, 0.13f, 0.87f, 1.0f};
    constexpr std::array<float, 4u> limbRingRadius{0.72f, 1.0f, 1.0f, 0.72f};
    constexpr float fullTurn = 6.28318530717958647692f;
    std::array<std::array<Vertex, limbSides>, limbRingZ.size()> limbRings{};
    for (std::size_t ring = 0u; ring < limbRingZ.size(); ++ring)
    {
        for (std::size_t side = 0u; side < limbSides; ++side)
        {
            const float angle = fullTurn * static_cast<float>(side) / static_cast<float>(limbSides);
            limbRings[ring][side] = Vertex{{std::cos(angle) * limbRingRadius[ring],
                                                   std::sin(angle) * limbRingRadius[ring],
                                                   limbRingZ[ring]}};
        }
    }
    for (std::size_t ring = 0u; ring + 1u < limbRingZ.size(); ++ring)
    {
        for (std::size_t side = 0u; side < limbSides; ++side)
        {
            const std::size_t next = (side + 1u) % limbSides;
            addQuad(limbRings[ring][side], limbRings[ring][next],
                    limbRings[ring + 1u][next], limbRings[ring + 1u][side]);
        }
    }
    const Vertex limbBase{{0.0f, 0.0f, 0.0f}};
    const Vertex limbTip{{0.0f, 0.0f, 1.0f}};
    for (std::size_t side = 0u; side < limbSides; ++side)
    {
        const std::size_t next = (side + 1u) % limbSides;
        addTriangle(limbBase, limbRings[0u][next], limbRings[0u][side]);
        addTriangle(limbTip, limbRings[3u][side], limbRings[3u][next]);
    }
    const std::uint32_t waterfallPrimitiveCount =
        (waterfallIndexCount - waterfallIndexOffset) / 3u;
    const std::uint32_t finaleRoofPrimitiveCount =
        (finaleRoofIndexCount - waterfallIndexCount) / 3u;
    const std::uint32_t torchPrimitiveCount = (torchIndexCount - finaleRoofIndexCount) / 3u;
    const std::uint32_t swordPrimitiveCount = (swordIndexCount - torchIndexCount) / 3u;
    const std::uint32_t playerBodyPrimitiveCount = (playerBodyIndexCount - swordIndexCount) / 3u;
    const std::uint32_t playerLimbPrimitiveCount = (static_cast<std::uint32_t>(indices.size()) - playerBodyIndexCount) / 3u;
    const VkTransformMatrixKHR transform{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f}};

    const VkMemoryPropertyFlags uploadMemory = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    const VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
    const VkDeviceSize indexBufferSize = sizeof(std::uint32_t) * indices.size();
    const VkDeviceSize worldSurfaceBufferSize = sizeof(std::uint32_t) * worldSurfaceCodes.size();
    if (!CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                      uploadMemory, true, vertexBuffer_, diagnostic, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ||
        !CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                      uploadMemory, true, indexBuffer_, diagnostic, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ||
        !CreateBuffer(sizeof(transform), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                      uploadMemory, true, transformBuffer_, diagnostic, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ||
        !CreateBuffer(sizeof(RtHeldLightGpu), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      uploadMemory, false, heldLightBuffer_, diagnostic) ||
        !CreateBuffer(sizeof(RtFireEmitterGpu) * kRtFireEmitterCapacity,
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      uploadMemory, false, fireEmitterBuffer_, diagnostic) ||
        !CreateBuffer(sizeof(QualityDustUpload), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      uploadMemory, false, qualityControlsBuffer_, diagnostic) ||
        !CreateBuffer(sizeof(WaterContactRippleGpu), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      uploadMemory, false, waterContactRippleBuffer_, diagnostic) ||
        !CreateBuffer(worldSurfaceBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      uploadMemory, false, worldSurfaceBuffer_, diagnostic, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
    {
        return false;
    }

    const auto initialQuality = *ResolveRtQualityControls(std::nullopt, RtWorkloadPreset::Authored,
        pipelineBundle_.Request().quality == DielectricQuality::High);
    const QualityDustUpload initialDustQuality{initialQuality,{}};
    const WaterContactRippleGpu initialWaterContactRipple{};
    const RtHeldLightGpu initialHeldLight{};
    const std::array<RtFireEmitterGpu, kRtFireEmitterCapacity> initialFireEmitters{};
    if (!gpuResources_.MapBufferForHostWrites(heldLightBuffer_, diagnostic) ||
        !gpuResources_.MapBufferForHostWrites(fireEmitterBuffer_, diagnostic) ||
        !gpuResources_.MapBufferForHostWrites(qualityControlsBuffer_, diagnostic) ||
        !gpuResources_.MapBufferForHostWrites(waterContactRippleBuffer_, diagnostic) ||
        !WriteBuffer(vertexBuffer_, vertices.data(), vertexBufferSize, "world vertex", diagnostic) ||
        !WriteBuffer(indexBuffer_, indices.data(), indexBufferSize, "world index", diagnostic) ||
        !WriteBuffer(transformBuffer_, &transform, sizeof(transform), "world transform", diagnostic) ||
        !WriteBuffer(heldLightBuffer_, &initialHeldLight, sizeof(initialHeldLight),
                     "held light", diagnostic) ||
        !WriteBuffer(fireEmitterBuffer_, initialFireEmitters.data(), sizeof(initialFireEmitters),
                     "fire emitters", diagnostic) ||
        !WriteBuffer(qualityControlsBuffer_, &initialDustQuality, sizeof(initialDustQuality),
                     "quality controls", diagnostic) ||
        !WriteBuffer(waterContactRippleBuffer_, &initialWaterContactRipple,
                     sizeof(initialWaterContactRipple), "water contact ripple", diagnostic) ||
        !WriteBuffer(worldSurfaceBuffer_, worldSurfaceCodes.data(), worldSurfaceBufferSize,
                     "world surface metadata", diagnostic))
    {
        return false;
    }

    VkAccelerationStructureGeometryKHR blasGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    blasGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    blasGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    blasGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    blasGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    blasGeometry.geometry.triangles.vertexData.deviceAddress = vertexBuffer_.address;
    blasGeometry.geometry.triangles.vertexStride = sizeof(Vertex);
    blasGeometry.geometry.triangles.maxVertex = static_cast<std::uint32_t>(vertices.size() - 1u);
    blasGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    blasGeometry.geometry.triangles.indexData.deviceAddress = indexBuffer_.address;
    blasGeometry.geometry.triangles.transformData.deviceAddress = transformBuffer_.address;

    VkAccelerationStructureBuildGeometryInfoKHR blasBuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    blasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    blasBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
        (developmentRescueJourney_?VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR:0);
    rescueWorldMaxVertex_=static_cast<std::uint32_t>(vertices.size()-1);
    blasBuildInfo.geometryCount = 1u;
    blasBuildInfo.pGeometries = &blasGeometry;

    std::uint32_t primitiveCount = sceneIndexCount / 3u;
    VkAccelerationStructureBuildSizesInfoKHR blasSizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &blasBuildInfo, &primitiveCount, &blasSizes);

    if (!CreateBuffer(blasSizes.accelerationStructureSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      blas_.backing,
                      diagnostic))
    {
        return false;
    }

    VkAccelerationStructureCreateInfoKHR blasCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    blasCreateInfo.buffer = blas_.backing.buffer;
    blasCreateInfo.size = blasSizes.accelerationStructureSize;
    blasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &blasCreateInfo, nullptr, &blas_.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create BLAS.";
        return false;
    }

    if(developmentRescueJourney_ && !CreateScratchBuffer(blasSizes.updateScratchSize,rescueWorldUpdateScratch_,diagnostic)) return false;
    Buffer blasScratch;
    if (!CreateScratchBuffer(blasSizes.buildScratchSize, blasScratch, diagnostic))
    {
        return false;
    }

    VkAccelerationStructureBuildRangeInfoKHR blasRange{};
    blasRange.primitiveCount = primitiveCount;
    blasBuildInfo.dstAccelerationStructure = blas_.handle;
    blasBuildInfo.scratchData.deviceAddress = blasScratch.AlignedAddress();
    const VkAccelerationStructureBuildRangeInfoKHR* blasRanges[] = {&blasRange};
    struct BlasBuildData
    {
        PresentableTinyRtScene* scene;
        VkAccelerationStructureBuildGeometryInfoKHR* buildInfo;
        const VkAccelerationStructureBuildRangeInfoKHR** ranges;
    } blasBuildData{this, &blasBuildInfo, blasRanges};
    const auto buildBlas = [](VkCommandBuffer commandBuffer, void* userData) {
        auto* buildData = static_cast<BlasBuildData*>(userData);
        buildData->scene->vkCmdBuildAccelerationStructuresKHR_(commandBuffer, 1u, buildData->buildInfo, buildData->ranges);
    };
    if (!RunOneTimeCommands(buildBlas, &blasBuildData, diagnostic))
    {
        DestroyBuffer(blasScratch);
        return false;
    }
    DestroyBuffer(blasScratch);

    VkAccelerationStructureDeviceAddressInfoKHR blasAddressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    blasAddressInfo.accelerationStructure = blas_.handle;
    blas_.address = vkGetAccelerationStructureDeviceAddressKHR_(device_, &blasAddressInfo);

    VkAccelerationStructureBuildRangeInfoKHR waterfallRange{};
    waterfallRange.primitiveCount = waterfallPrimitiveCount;
    waterfallRange.primitiveOffset = waterfallIndexOffset * sizeof(std::uint32_t);
    waterfallRange.firstVertex = waterfallFirstVertex;
    VkAccelerationStructureBuildGeometryInfoKHR waterfallBuildInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    waterfallBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    waterfallBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    waterfallBuildInfo.geometryCount = 1u;
    waterfallBuildInfo.pGeometries = &blasGeometry;
    VkAccelerationStructureBuildSizesInfoKHR waterfallSizes{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                             &waterfallBuildInfo, &waterfallPrimitiveCount,
                                             &waterfallSizes);
    if (!CreateBuffer(waterfallSizes.accelerationStructureSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      waterfallBlas_.backing,
                      diagnostic))
    {
        return false;
    }
    VkAccelerationStructureCreateInfoKHR waterfallCreateInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    waterfallCreateInfo.buffer = waterfallBlas_.backing.buffer;
    waterfallCreateInfo.size = waterfallSizes.accelerationStructureSize;
    waterfallCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &waterfallCreateInfo, nullptr,
                                          &waterfallBlas_.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create dedicated waterfall BLAS.";
        return false;
    }
    Buffer waterfallScratch;
    if (!CreateScratchBuffer(waterfallSizes.buildScratchSize, waterfallScratch, diagnostic))
    {
        return false;
    }
    waterfallBuildInfo.dstAccelerationStructure = waterfallBlas_.handle;
    waterfallBuildInfo.scratchData.deviceAddress = waterfallScratch.AlignedAddress();
    const VkAccelerationStructureBuildRangeInfoKHR* waterfallRanges[] = {&waterfallRange};
    BlasBuildData waterfallBuildData{this, &waterfallBuildInfo, waterfallRanges};
    if (!RunOneTimeCommands(buildBlas, &waterfallBuildData, diagnostic))
    {
        DestroyBuffer(waterfallScratch);
        return false;
    }
    DestroyBuffer(waterfallScratch);
    VkAccelerationStructureDeviceAddressInfoKHR waterfallAddressInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    waterfallAddressInfo.accelerationStructure = waterfallBlas_.handle;
    waterfallBlas_.address =
        vkGetAccelerationStructureDeviceAddressKHR_(device_, &waterfallAddressInfo);

    VkAccelerationStructureBuildRangeInfoKHR finaleRoofRange{};
    finaleRoofRange.primitiveCount = finaleRoofPrimitiveCount;
    finaleRoofRange.primitiveOffset = waterfallIndexCount * sizeof(std::uint32_t);
    finaleRoofRange.firstVertex = 0u;
    VkAccelerationStructureBuildGeometryInfoKHR finaleRoofBuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    finaleRoofBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    finaleRoofBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    finaleRoofBuildInfo.geometryCount = 1u;
    finaleRoofBuildInfo.pGeometries = &blasGeometry;
    VkAccelerationStructureBuildSizesInfoKHR finaleRoofSizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                             &finaleRoofBuildInfo, &finaleRoofPrimitiveCount, &finaleRoofSizes);
    if (!CreateBuffer(finaleRoofSizes.accelerationStructureSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      finaleRoofBlas_.backing,
                      diagnostic))
    {
        return false;
    }
    VkAccelerationStructureCreateInfoKHR finaleRoofCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    finaleRoofCreateInfo.buffer = finaleRoofBlas_.backing.buffer;
    finaleRoofCreateInfo.size = finaleRoofSizes.accelerationStructureSize;
    finaleRoofCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &finaleRoofCreateInfo, nullptr, &finaleRoofBlas_.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create finale sliding-roof BLAS.";
        return false;
    }
    Buffer finaleRoofScratch;
    if (!CreateScratchBuffer(finaleRoofSizes.buildScratchSize, finaleRoofScratch, diagnostic))
    {
        return false;
    }
    finaleRoofBuildInfo.dstAccelerationStructure = finaleRoofBlas_.handle;
    finaleRoofBuildInfo.scratchData.deviceAddress = finaleRoofScratch.AlignedAddress();
    const VkAccelerationStructureBuildRangeInfoKHR* finaleRoofRanges[] = {&finaleRoofRange};
    BlasBuildData finaleRoofBuildData{this, &finaleRoofBuildInfo, finaleRoofRanges};
    if (!RunOneTimeCommands(buildBlas, &finaleRoofBuildData, diagnostic))
    {
        DestroyBuffer(finaleRoofScratch);
        return false;
    }
    DestroyBuffer(finaleRoofScratch);
    VkAccelerationStructureDeviceAddressInfoKHR finaleRoofAddressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    finaleRoofAddressInfo.accelerationStructure = finaleRoofBlas_.handle;
    finaleRoofBlas_.address = vkGetAccelerationStructureDeviceAddressKHR_(device_, &finaleRoofAddressInfo);

    const auto appendStaticGeometries = [this, &diagnostic](
        const std::uint32_t instanceCustomIndex,
        std::vector<VkAccelerationStructureGeometryKHR>& geometries,
        std::vector<VkAccelerationStructureBuildRangeInfoKHR>& ranges,
        std::vector<std::uint32_t>& primitiveCounts) {
        const RtInstanceMetadata instance =
            staticMeshSlot_.InstanceMetadata()[instanceCustomIndex];
        const auto& primitiveMetadata = staticMeshSlot_.PrimitiveMetadata();
        const auto& materialMetadata = staticMeshSlot_.Materials();
        const auto& vertexBuffer = VertexBufferForRole(static_cast<RtGeometryRole>(instance.geometryRole));
        const auto& vertexCounts = staticMeshSlot_.PrimitiveVertexCounts();
        for (std::uint32_t localIndex = 0u; localIndex < instance.primitiveCount; ++localIndex)
        {
            const std::uint32_t geometryIndex = instance.primitiveBase + localIndex;
            const RtPrimitiveMetadata& primitive = primitiveMetadata[geometryIndex];
            if (vertexCounts[geometryIndex] == 0u || primitive.indexCount == 0u)
            {
                diagnostic = "Static RT BLAS primitive has an empty geometry range.";
                return false;
            }
            VkAccelerationStructureGeometryKHR geometry{
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
            geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            if (primitive.materialIndex >= materialMetadata.size())
            {
                diagnostic = "Static RT BLAS primitive has an out-of-range material.";
                return false;
            }
            const bool transmissive =
                (materialMetadata[primitive.materialIndex].materialFlags[0] &
                 static_cast<std::uint32_t>(RtMaterialFlag::Transmission)) != 0u;
            // Transparent-shadow accumulation is not idempotent: Vulkan may
            // otherwise report the same triangle candidate more than once.
            // Preserve non-opaque dielectric filtering and the opaque fast path.
            geometry.flags = transmissive
                ? VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR
                : VK_GEOMETRY_OPAQUE_BIT_KHR;
            geometry.geometry.triangles.sType =
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
            geometry.geometry.triangles.vertexData.deviceAddress =
                vertexBuffer.address +
                static_cast<VkDeviceSize>(primitive.vertexOffset) *
                    sizeof(horde::scene::assets::StaticRtVertex);
            geometry.geometry.triangles.vertexStride =
                sizeof(horde::scene::assets::StaticRtVertex);
            geometry.geometry.triangles.maxVertex =
                vertexCounts[geometryIndex] - 1u;
            geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
            geometry.geometry.triangles.indexData.deviceAddress =
                staticIndexBuffer_.address +
                static_cast<VkDeviceSize>(primitive.indexOffset) * sizeof(std::uint32_t);
            geometry.geometry.triangles.transformData.deviceAddress =
                staticGeometryTransformBuffer_.address +
                static_cast<VkDeviceSize>(geometryIndex) * sizeof(VkTransformMatrixKHR);
            geometries.push_back(geometry);
            VkAccelerationStructureBuildRangeInfoKHR range{};
            range.primitiveCount = primitive.indexCount / 3u;
            ranges.push_back(range);
            primitiveCounts.push_back(range.primitiveCount);
        }
        return true;
    };

    const auto buildRegisteredStaticBlas = [this, &appendStaticGeometries, &buildBlas, &diagnostic](
        const std::uint32_t instanceCustomIndex,
        const char* label,
        AccelerationStructure& accelerationStructure,
        const bool productionProp = false,
        Buffer* retainedUpdateScratch = nullptr) {
        std::vector<VkAccelerationStructureGeometryKHR> geometries;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges;
        std::vector<std::uint32_t> primitiveCounts;
        if (!appendStaticGeometries(
                instanceCustomIndex, geometries, ranges, primitiveCounts))
            return false;
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
            (retainedUpdateScratch != nullptr
                ? VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR : 0u);
        buildInfo.geometryCount = static_cast<std::uint32_t>(geometries.size());
        buildInfo.pGeometries = geometries.data();
        VkAccelerationStructureBuildSizesInfoKHR sizes{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        vkGetAccelerationStructureBuildSizesKHR_(
            device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &buildInfo, primitiveCounts.data(), &sizes);
        if (!CreateBuffer(sizes.accelerationStructureSize,
                          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          true, accelerationStructure.backing, diagnostic))
            return false;
        VkAccelerationStructureCreateInfoKHR createInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
        createInfo.buffer = accelerationStructure.backing.buffer;
        createInfo.size = sizes.accelerationStructureSize;
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        if (vkCreateAccelerationStructureKHR_(
                device_, &createInfo, nullptr, &accelerationStructure.handle) != VK_SUCCESS)
        {
            diagnostic = std::string("Failed to create ") + label + " BLAS.";
            return false;
        }
        Buffer temporaryScratch;
        Buffer& scratch = retainedUpdateScratch != nullptr
            ? *retainedUpdateScratch : temporaryScratch;
        const VkDeviceSize scratchSize = retainedUpdateScratch != nullptr
            ? std::max(sizes.buildScratchSize, sizes.updateScratchSize)
            : sizes.buildScratchSize;
        if (!CreateScratchBuffer(scratchSize, scratch, diagnostic))
            return false;
        buildInfo.dstAccelerationStructure = accelerationStructure.handle;
        buildInfo.scratchData.deviceAddress = scratch.AlignedAddress();
        std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> rangePointers;
        rangePointers.reserve(ranges.size());
        for (const auto& range : ranges) rangePointers.push_back(&range);
        BlasBuildData buildData{this, &buildInfo, rangePointers.data()};
        const auto buildStart = std::chrono::steady_clock::now();
        if (!RunOneTimeCommands(buildBlas, &buildData, diagnostic))
        {
            if (retainedUpdateScratch == nullptr) DestroyBuffer(scratch);
            return false;
        }
        if (retainedUpdateScratch == nullptr) DestroyBuffer(scratch);
        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
        addressInfo.accelerationStructure = accelerationStructure.handle;
        accelerationStructure.address = vkGetAccelerationStructureDeviceAddressKHR_(
            device_, &addressInfo);
        if (productionProp)
        {
            productionPropBlasBytes_ += sizes.accelerationStructureSize;
            productionPropBlasBuildMilliseconds_ +=
                std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - buildStart).count();
        }
        return accelerationStructure.address != 0u;
    };

    // Shared body-only owner excludes the held torch's engine emissive core.
    // Both permanent world instances alias custom metadata1 and the admitted
    // asset buffers/materials; fire visibility is entirely the emitter record.
    if (!buildRegisteredStaticBlas(1u, "world torch body", worldTorchBodyBlas_, true))
        return false;

    std::vector<VkAccelerationStructureGeometryKHR> torchGeometries;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> torchRanges;
    std::vector<std::uint32_t> torchPrimitiveCounts;
    if (productionHeldItemAssetsEnabled_)
    {
        if (!appendStaticGeometries(
                kPlayerTorchInstanceIndex,
                torchGeometries, torchRanges, torchPrimitiveCounts))
            return false;
        // Task 4 owns final fire. Retain only the existing 16-triangle engine
        // flame core as a separate geometry after the authored PBR body.
        constexpr std::uint32_t engineFlamePrimitiveCount = 8u;
        torchGeometries.push_back(blasGeometry);
        VkAccelerationStructureBuildRangeInfoKHR flameRange{};
        flameRange.primitiveCount = engineFlamePrimitiveCount;
        flameRange.primitiveOffset =
            (torchIndexCount - engineFlamePrimitiveCount * 3u) * sizeof(std::uint32_t);
        torchRanges.push_back(flameRange);
        torchPrimitiveCounts.push_back(engineFlamePrimitiveCount);
    }
    else
    {
        torchGeometries.push_back(blasGeometry);
        VkAccelerationStructureBuildRangeInfoKHR torchRange{};
        torchRange.primitiveCount = torchPrimitiveCount;
        torchRange.primitiveOffset = finaleRoofIndexCount * sizeof(std::uint32_t);
        torchRanges.push_back(torchRange);
        torchPrimitiveCounts.push_back(torchPrimitiveCount);
    }

    VkAccelerationStructureBuildGeometryInfoKHR torchBlasBuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    torchBlasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    torchBlasBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    torchBlasBuildInfo.geometryCount = static_cast<std::uint32_t>(torchGeometries.size());
    torchBlasBuildInfo.pGeometries = torchGeometries.data();
    VkAccelerationStructureBuildSizesInfoKHR torchBlasSizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                             &torchBlasBuildInfo, torchPrimitiveCounts.data(), &torchBlasSizes);
    if (!CreateBuffer(torchBlasSizes.accelerationStructureSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      torchBlas_.backing,
                      diagnostic))
    {
        return false;
    }

    VkAccelerationStructureCreateInfoKHR torchBlasCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    torchBlasCreateInfo.buffer = torchBlas_.backing.buffer;
    torchBlasCreateInfo.size = torchBlasSizes.accelerationStructureSize;
    torchBlasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &torchBlasCreateInfo, nullptr, &torchBlas_.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create held-torch BLAS.";
        return false;
    }

    Buffer torchBlasScratch;
    if (!CreateScratchBuffer(torchBlasSizes.buildScratchSize, torchBlasScratch, diagnostic))
    {
        return false;
    }
    torchBlasBuildInfo.dstAccelerationStructure = torchBlas_.handle;
    torchBlasBuildInfo.scratchData.deviceAddress = torchBlasScratch.AlignedAddress();
    std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> torchBlasRangePointers;
    torchBlasRangePointers.reserve(torchRanges.size());
    for (const auto& range : torchRanges) torchBlasRangePointers.push_back(&range);
    BlasBuildData torchBlasBuildData{
        this, &torchBlasBuildInfo, torchBlasRangePointers.data()};
    const auto torchBlasBuildStart = std::chrono::steady_clock::now();
    if (!RunOneTimeCommands(buildBlas, &torchBlasBuildData, diagnostic))
    {
        DestroyBuffer(torchBlasScratch);
        return false;
    }
    if (productionHeldItemAssetsEnabled_)
    {
        heldItemBlasMeasurements_.RecordTorch(
            torchBlasSizes.accelerationStructureSize,
            std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - torchBlasBuildStart).count());
    }
    DestroyBuffer(torchBlasScratch);

    VkAccelerationStructureDeviceAddressInfoKHR torchBlasAddressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    torchBlasAddressInfo.accelerationStructure = torchBlas_.handle;
    torchBlas_.address = vkGetAccelerationStructureDeviceAddressKHR_(device_, &torchBlasAddressInfo);

    std::vector<VkAccelerationStructureGeometryKHR> swordGeometries;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> swordRanges;
    std::vector<std::uint32_t> swordPrimitiveCounts;
    if (genericStaticAssetEnabled_)
    {
        if (!appendStaticGeometries(
                3u, swordGeometries, swordRanges, swordPrimitiveCounts))
            return false;
    }
    else
    {
        swordGeometries.push_back(blasGeometry);
        VkAccelerationStructureBuildRangeInfoKHR range{};
        range.primitiveCount = swordPrimitiveCount;
        range.primitiveOffset = torchIndexCount * sizeof(std::uint32_t);
        swordRanges.push_back(range);
        swordPrimitiveCounts.push_back(swordPrimitiveCount);
    }
    VkAccelerationStructureBuildGeometryInfoKHR swordBlasBuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    swordBlasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    swordBlasBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    swordBlasBuildInfo.geometryCount = static_cast<std::uint32_t>(swordGeometries.size());
    swordBlasBuildInfo.pGeometries = swordGeometries.data();
    VkAccelerationStructureBuildSizesInfoKHR swordBlasSizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                             &swordBlasBuildInfo, swordPrimitiveCounts.data(),
                                             &swordBlasSizes);
    if (!CreateBuffer(swordBlasSizes.accelerationStructureSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      swordBlas_.backing,
                      diagnostic))
    {
        return false;
    }
    VkAccelerationStructureCreateInfoKHR swordBlasCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    swordBlasCreateInfo.buffer = swordBlas_.backing.buffer;
    swordBlasCreateInfo.size = swordBlasSizes.accelerationStructureSize;
    swordBlasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &swordBlasCreateInfo, nullptr, &swordBlas_.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create held-sword BLAS.";
        return false;
    }
    Buffer swordBlasScratch;
    if (!CreateScratchBuffer(swordBlasSizes.buildScratchSize, swordBlasScratch, diagnostic))
    {
        return false;
    }
    swordBlasBuildInfo.dstAccelerationStructure = swordBlas_.handle;
    swordBlasBuildInfo.scratchData.deviceAddress = swordBlasScratch.AlignedAddress();
    std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> swordBlasRangePointers;
    swordBlasRangePointers.reserve(swordRanges.size());
    for (const auto& range : swordRanges) swordBlasRangePointers.push_back(&range);
    BlasBuildData swordBlasBuildData{this, &swordBlasBuildInfo, swordBlasRangePointers.data()};
    const auto staticBlasBuildStart = std::chrono::steady_clock::now();
    if (!RunOneTimeCommands(buildBlas, &swordBlasBuildData, diagnostic))
    {
        DestroyBuffer(swordBlasScratch);
        return false;
    }
    if (genericStaticAssetEnabled_)
    {
        heldItemBlasMeasurements_.RecordSword(
            swordBlasSizes.accelerationStructureSize,
            std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - staticBlasBuildStart).count());
        staticMeshBlasBytes_ = heldItemBlasMeasurements_.TotalBytes();
        staticMeshBlasBuildMilliseconds_ =
            heldItemBlasMeasurements_.TotalBuildMilliseconds();
    }
    DestroyBuffer(swordBlasScratch);
    VkAccelerationStructureDeviceAddressInfoKHR swordBlasAddressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    swordBlasAddressInfo.accelerationStructure = swordBlas_.handle;
    swordBlas_.address = vkGetAccelerationStructureDeviceAddressKHR_(device_, &swordBlasAddressInfo);

    productionPropBlasBytes_ = 0u;
    productionPropBlasBuildMilliseconds_ = 0.0;
    if (!buildRegisteredStaticBlas(
            5u, "production Gothic chest base", gothicChestBaseBlas_, true) ||
        !buildRegisteredStaticBlas(
            6u, "production Gothic chest lid", gothicChestLidBlas_, true) ||
        !buildRegisteredStaticBlas(
            7u, "production reward lantern ring", rewardLanternRingBlas_, true) ||
        !buildRegisteredStaticBlas(
            8u, "production reward lantern body", rewardLanternBodyBlas_, true) ||
        !buildRegisteredStaticBlas(
            9u, "generic dielectric fixture", dielectricFixtureBlas_) ||
        !buildRegisteredStaticBlas(
            kCollapseInstanceIndex, "production collapsed entry", collapseBlas_, true) ||
        !buildRegisteredStaticBlas(
            kPlayerSwordScabbardMetadataIndex, "player sword scabbard",
            playerSwordScabbardBlas_, true) ||
        (sceneProfile_ == RtSceneProfile::Showcase &&
         !buildRegisteredStaticBlas(
             kWaterDropletMetadataIndex, "runtime water contact droplets",
             waterDropletBlas_, false, &waterDropletBlasUpdateScratch_)))
        return false;

    VkAccelerationStructureBuildRangeInfoKHR playerBodyRange{};
    playerBodyRange.primitiveCount = playerBodyPrimitiveCount;
    playerBodyRange.primitiveOffset = swordIndexCount * sizeof(std::uint32_t);
    playerBodyRange.firstVertex = 0u;
    VkAccelerationStructureBuildGeometryInfoKHR playerBodyBlasBuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    playerBodyBlasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    playerBodyBlasBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    playerBodyBlasBuildInfo.geometryCount = 1u;
    playerBodyBlasBuildInfo.pGeometries = &blasGeometry;
    VkAccelerationStructureBuildSizesInfoKHR playerBodyBlasSizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &playerBodyBlasBuildInfo, &playerBodyPrimitiveCount, &playerBodyBlasSizes);
    if (!CreateBuffer(playerBodyBlasSizes.accelerationStructureSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      playerBodyBlas_.backing,
                      diagnostic))
    {
        return false;
    }
    VkAccelerationStructureCreateInfoKHR playerBodyBlasCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    playerBodyBlasCreateInfo.buffer = playerBodyBlas_.backing.buffer;
    playerBodyBlasCreateInfo.size = playerBodyBlasSizes.accelerationStructureSize;
    playerBodyBlasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &playerBodyBlasCreateInfo, nullptr, &playerBodyBlas_.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create first-person player-body BLAS.";
        return false;
    }
    Buffer playerBodyBlasScratch;
    if (!CreateScratchBuffer(playerBodyBlasSizes.buildScratchSize, playerBodyBlasScratch, diagnostic))
    {
        return false;
    }
    playerBodyBlasBuildInfo.dstAccelerationStructure = playerBodyBlas_.handle;
    playerBodyBlasBuildInfo.scratchData.deviceAddress = playerBodyBlasScratch.AlignedAddress();
    const VkAccelerationStructureBuildRangeInfoKHR* playerBodyBlasRanges[] = {&playerBodyRange};
    BlasBuildData playerBodyBlasBuildData{this, &playerBodyBlasBuildInfo, playerBodyBlasRanges};
    if (!RunOneTimeCommands(buildBlas, &playerBodyBlasBuildData, diagnostic))
    {
        DestroyBuffer(playerBodyBlasScratch);
        return false;
    }
    DestroyBuffer(playerBodyBlasScratch);
    VkAccelerationStructureDeviceAddressInfoKHR playerBodyBlasAddressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    playerBodyBlasAddressInfo.accelerationStructure = playerBodyBlas_.handle;
    playerBodyBlas_.address = vkGetAccelerationStructureDeviceAddressKHR_(device_, &playerBodyBlasAddressInfo);

    VkAccelerationStructureBuildRangeInfoKHR playerLimbRange{};
    playerLimbRange.primitiveCount = playerLimbPrimitiveCount;
    playerLimbRange.primitiveOffset = playerBodyIndexCount * sizeof(std::uint32_t);
    playerLimbRange.firstVertex = 0u;
    VkAccelerationStructureBuildGeometryInfoKHR playerLimbBlasBuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    playerLimbBlasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    playerLimbBlasBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    playerLimbBlasBuildInfo.geometryCount = 1u;
    playerLimbBlasBuildInfo.pGeometries = &blasGeometry;
    VkAccelerationStructureBuildSizesInfoKHR playerLimbBlasSizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &playerLimbBlasBuildInfo, &playerLimbPrimitiveCount, &playerLimbBlasSizes);
    if (!CreateBuffer(playerLimbBlasSizes.accelerationStructureSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      playerLimbBlas_.backing,
                      diagnostic))
    {
        return false;
    }
    VkAccelerationStructureCreateInfoKHR playerLimbBlasCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    playerLimbBlasCreateInfo.buffer = playerLimbBlas_.backing.buffer;
    playerLimbBlasCreateInfo.size = playerLimbBlasSizes.accelerationStructureSize;
    playerLimbBlasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &playerLimbBlasCreateInfo, nullptr, &playerLimbBlas_.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create reusable first-person limb BLAS.";
        return false;
    }
    Buffer playerLimbBlasScratch;
    if (!CreateScratchBuffer(playerLimbBlasSizes.buildScratchSize, playerLimbBlasScratch, diagnostic))
    {
        return false;
    }
    playerLimbBlasBuildInfo.dstAccelerationStructure = playerLimbBlas_.handle;
    playerLimbBlasBuildInfo.scratchData.deviceAddress = playerLimbBlasScratch.AlignedAddress();
    const VkAccelerationStructureBuildRangeInfoKHR* playerLimbBlasRanges[] = {&playerLimbRange};
    BlasBuildData playerLimbBlasBuildData{this, &playerLimbBlasBuildInfo, playerLimbBlasRanges};
    if (!RunOneTimeCommands(buildBlas, &playerLimbBlasBuildData, diagnostic))
    {
        DestroyBuffer(playerLimbBlasScratch);
        return false;
    }
    DestroyBuffer(playerLimbBlasScratch);
    VkAccelerationStructureDeviceAddressInfoKHR playerLimbBlasAddressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    playerLimbBlasAddressInfo.accelerationStructure = playerLimbBlas_.handle;
    playerLimbBlas_.address = vkGetAccelerationStructureDeviceAddressKHR_(device_, &playerLimbBlasAddressInfo);

    const auto buildPlayerGeometry = [&](std::uint32_t instanceIndex,
                                         AccelerationStructure& playerBlas,
                                         Buffer& playerScratch) -> bool {
    std::vector<VkAccelerationStructureGeometryKHR> skinnedPlayerGeometries;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> skinnedPlayerRanges;
    std::vector<std::uint32_t> skinnedPlayerPrimitiveCounts;
    if (!appendStaticGeometries(
            instanceIndex, skinnedPlayerGeometries, skinnedPlayerRanges,
            skinnedPlayerPrimitiveCounts))
        return false;
    VkAccelerationStructureBuildGeometryInfoKHR skinnedPlayerBuildInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    skinnedPlayerBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    skinnedPlayerBuildInfo.flags =
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
        VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    skinnedPlayerBuildInfo.geometryCount =
        static_cast<std::uint32_t>(skinnedPlayerGeometries.size());
    skinnedPlayerBuildInfo.pGeometries = skinnedPlayerGeometries.data();
    VkAccelerationStructureBuildSizesInfoKHR skinnedPlayerSizes{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(
        device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &skinnedPlayerBuildInfo, skinnedPlayerPrimitiveCounts.data(),
        &skinnedPlayerSizes);
    if (!CreateBuffer(skinnedPlayerSizes.accelerationStructureSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true,
                      playerBlas.backing, diagnostic))
        return false;
    VkAccelerationStructureCreateInfoKHR skinnedPlayerCreateInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    skinnedPlayerCreateInfo.buffer = playerBlas.backing.buffer;
    skinnedPlayerCreateInfo.size = skinnedPlayerSizes.accelerationStructureSize;
    skinnedPlayerCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &skinnedPlayerCreateInfo,
                                          nullptr, &playerBlas.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create skinned player BLAS.";
        return false;
    }
    if (!CreateScratchBuffer(std::max(skinnedPlayerSizes.buildScratchSize,
                               skinnedPlayerSizes.updateScratchSize), playerScratch, diagnostic))
        return false;
    skinnedPlayerBuildInfo.dstAccelerationStructure = playerBlas.handle;
    skinnedPlayerBuildInfo.scratchData.deviceAddress = playerScratch.AlignedAddress();
    std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> skinnedPlayerRangePointers;
    skinnedPlayerRangePointers.reserve(skinnedPlayerRanges.size());
    for (const auto& range : skinnedPlayerRanges)
        skinnedPlayerRangePointers.push_back(&range);
    BlasBuildData skinnedPlayerBuildData{
        this, &skinnedPlayerBuildInfo, skinnedPlayerRangePointers.data()};
    if (!RunOneTimeCommands(buildBlas, &skinnedPlayerBuildData, diagnostic)) return false;
    VkAccelerationStructureDeviceAddressInfoKHR skinnedPlayerAddressInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    skinnedPlayerAddressInfo.accelerationStructure = playerBlas.handle;
    playerBlas.address = vkGetAccelerationStructureDeviceAddressKHR_(
        device_, &skinnedPlayerAddressInfo);
    return true;
    };
    if (!buildPlayerGeometry(kPlayerWorldBodyInstanceIndex, skinnedPlayerBlas_, skinnedPlayerBlasUpdateScratch_))
        return false;
    if (viewmodelAvailable_ &&
        !buildPlayerGeometry(kPlayerViewmodelInstanceIndex, viewmodelBlas_, viewmodelBlasUpdateScratch_))
        return false;

    if (!characterSlot_.PrepareInitialGeometry(diagnostic))
    {
        return false;
    }
    auto& lichGpu = characterSlot_.LichGpu();
    auto& lichVertexBuffer_ = lichGpu.vertices;
    auto& lichBlas_ = lichGpu.accelerationStructure;
    auto& lichBlasUpdateScratch_ = lichGpu.updateScratch;
    const auto& lichSkinnedVertices_ = characterSlot_.LichVertices();
    lichGpu.vertexStride = sizeof(horde::scene::TexturedSkinnedRtVertex);
    lichGpu.vertexCount = static_cast<std::uint32_t>(lichSkinnedVertices_.size());
    for (std::size_t bucket = 0u; bucket < CharacterRenderSlot::kMaximumSkeletonPoseBuckets; ++bucket)
    {
        auto& skeletonGpu = characterSlot_.SkeletonGpu(bucket);
        const auto& skeletonVertices = characterSlot_.SkeletonVertices(bucket);
        skeletonGpu.vertexStride = sizeof(horde::scene::SkinnedRtVertex);
        skeletonGpu.vertexCount = static_cast<std::uint32_t>(skeletonVertices.size());
        const VkDeviceSize skeletonVertexBufferSize =
            sizeof(horde::scene::SkinnedRtVertex) * skeletonVertices.size();
        if (!CreateBuffer(skeletonVertexBufferSize,
                          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          uploadMemory,
                          true,
                          skeletonGpu.vertices,
                          diagnostic) ||
            !gpuResources_.MapBufferForHostWrites(skeletonGpu.vertices, diagnostic) ||
            !WriteBuffer(skeletonGpu.vertices,
                         skeletonVertices.data(),
                         skeletonVertexBufferSize,
                         bucket == 0u ? "skeleton pose 0 vertex" : "skeleton pose 1 vertex",
                         diagnostic))
        {
            return false;
        }

        VkAccelerationStructureGeometryKHR skeletonGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        skeletonGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        skeletonGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        skeletonGeometry.geometry.triangles.sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        skeletonGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        skeletonGeometry.geometry.triangles.vertexData.deviceAddress = skeletonGpu.vertices.address;
        skeletonGeometry.geometry.triangles.vertexStride = sizeof(horde::scene::SkinnedRtVertex);
        skeletonGeometry.geometry.triangles.maxVertex =
            static_cast<std::uint32_t>(skeletonVertices.size() - 1u);
        skeletonGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
        const std::uint32_t skeletonPrimitiveCount =
            static_cast<std::uint32_t>(skeletonVertices.size() / 3u);
        VkAccelerationStructureBuildGeometryInfoKHR skeletonBuildInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        skeletonBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        skeletonBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                                  VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        skeletonBuildInfo.geometryCount = 1u;
        skeletonBuildInfo.pGeometries = &skeletonGeometry;
        VkAccelerationStructureBuildSizesInfoKHR skeletonSizes{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        vkGetAccelerationStructureBuildSizesKHR_(device_,
                                                 VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                                 &skeletonBuildInfo,
                                                 &skeletonPrimitiveCount,
                                                 &skeletonSizes);
        if (!CreateBuffer(skeletonSizes.accelerationStructureSize,
                          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          true,
                          skeletonGpu.accelerationStructure.backing,
                          diagnostic))
        {
            return false;
        }
        VkAccelerationStructureCreateInfoKHR skeletonCreateInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
        skeletonCreateInfo.buffer = skeletonGpu.accelerationStructure.backing.buffer;
        skeletonCreateInfo.size = skeletonSizes.accelerationStructureSize;
        skeletonCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        if (vkCreateAccelerationStructureKHR_(device_,
                                              &skeletonCreateInfo,
                                              nullptr,
                                              &skeletonGpu.accelerationStructure.handle) != VK_SUCCESS)
        {
            diagnostic = "Failed to create animated skeleton pose BLAS.";
            return false;
        }
        if (!CreateScratchBuffer(std::max(skeletonSizes.buildScratchSize, skeletonSizes.updateScratchSize), skeletonGpu.updateScratch, diagnostic))
        {
            return false;
        }
        VkAccelerationStructureBuildRangeInfoKHR skeletonRange{};
        skeletonRange.primitiveCount = skeletonPrimitiveCount;
        skeletonBuildInfo.dstAccelerationStructure = skeletonGpu.accelerationStructure.handle;
        skeletonBuildInfo.scratchData.deviceAddress = skeletonGpu.updateScratch.AlignedAddress();
        const VkAccelerationStructureBuildRangeInfoKHR* skeletonRanges[] = {&skeletonRange};
        BlasBuildData skeletonBuildData{this, &skeletonBuildInfo, skeletonRanges};
        if (!RunOneTimeCommands(buildBlas, &skeletonBuildData, diagnostic)) return false;
        VkAccelerationStructureDeviceAddressInfoKHR skeletonAddressInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
        skeletonAddressInfo.accelerationStructure = skeletonGpu.accelerationStructure.handle;
        skeletonGpu.accelerationStructure.address =
            vkGetAccelerationStructureDeviceAddressKHR_(device_, &skeletonAddressInfo);
    }

    const VkDeviceSize lichVertexBufferSize = sizeof(horde::scene::TexturedSkinnedRtVertex) * lichSkinnedVertices_.size();
    if (!CreateBuffer(lichVertexBufferSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      uploadMemory,
                      true,
                      lichVertexBuffer_,
                      diagnostic))
    {
        return false;
    }
    if (!gpuResources_.MapBufferForHostWrites(lichVertexBuffer_, diagnostic) ||
        !WriteBuffer(lichVertexBuffer_, lichSkinnedVertices_.data(), lichVertexBufferSize,
                     "lich vertex", diagnostic))
    {
        return false;
    }

    VkAccelerationStructureGeometryKHR lichGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    lichGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    // Meshy's blended flag is unnecessary for this opaque placeholder and
    // would make every ray pay candidate-confirmation costs.
    lichGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    lichGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    lichGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    lichGeometry.geometry.triangles.vertexData.deviceAddress = lichVertexBuffer_.address;
    lichGeometry.geometry.triangles.vertexStride = sizeof(horde::scene::TexturedSkinnedRtVertex);
    lichGeometry.geometry.triangles.maxVertex = static_cast<std::uint32_t>(lichSkinnedVertices_.size() - 1u);
    lichGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
    const std::uint32_t lichPrimitiveCount = static_cast<std::uint32_t>(lichSkinnedVertices_.size() / 3u);
    VkAccelerationStructureBuildGeometryInfoKHR lichBuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    lichBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    lichBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    lichBuildInfo.geometryCount = 1u;
    lichBuildInfo.pGeometries = &lichGeometry;
    VkAccelerationStructureBuildSizesInfoKHR lichSizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                             &lichBuildInfo, &lichPrimitiveCount, &lichSizes);
    if (!CreateBuffer(lichSizes.accelerationStructureSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      lichBlas_.backing,
                      diagnostic))
    {
        return false;
    }
    VkAccelerationStructureCreateInfoKHR lichCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    lichCreateInfo.buffer = lichBlas_.backing.buffer;
    lichCreateInfo.size = lichSizes.accelerationStructureSize;
    lichCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &lichCreateInfo, nullptr, &lichBlas_.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create animated lich BLAS.";
        return false;
    }
    if (!CreateScratchBuffer(std::max(lichSizes.buildScratchSize, lichSizes.updateScratchSize), lichBlasUpdateScratch_, diagnostic))
    {
        return false;
    }
    VkAccelerationStructureBuildRangeInfoKHR lichRange{};
    lichRange.primitiveCount = lichPrimitiveCount;
    lichBuildInfo.dstAccelerationStructure = lichBlas_.handle;
    lichBuildInfo.scratchData.deviceAddress = lichBlasUpdateScratch_.AlignedAddress();
    const VkAccelerationStructureBuildRangeInfoKHR* lichRanges[] = {&lichRange};
    BlasBuildData lichBuildData{this, &lichBuildInfo, lichRanges};
    if (!RunOneTimeCommands(buildBlas, &lichBuildData, diagnostic)) return false;
    VkAccelerationStructureDeviceAddressInfoKHR lichAddressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    lichAddressInfo.accelerationStructure = lichBlas_.handle;
    lichBlas_.address = vkGetAccelerationStructureDeviceAddressKHR_(device_, &lichAddressInfo);

    std::array<VkAccelerationStructureInstanceKHR, PresentableTinyRtScene::kTlasInstanceCount> instances{};
    instances[0].transform = transform;
    instances[0].instanceCustomIndex = 0u;
    instances[0].mask = 0x01u;
    instances[0].instanceShaderBindingTableRecordOffset = 0u;
    instances[0].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    instances[0].accelerationStructureReference = blas_.address;
    instances[1] = instances[0];
    instances[1].instanceCustomIndex = productionHeldItemAssetsEnabled_ ? kPlayerTorchInstanceIndex : 1u;
    instances[1].mask = 0x02u;
    instances[1].accelerationStructureReference = torchBlas_.address;
    instances[1].transform = {{
        1.0f, 0.0f, 0.0f, -0.32f,
        0.0f, 1.0f, 0.0f, -0.36f,
        0.0f, 0.0f, -1.0f, 3.82f}};
    instances[2] = instances[0];
    instances[2].instanceCustomIndex = 2u;
    instances[2].mask = 0x01u;
    instances[2].accelerationStructureReference = characterSlot_.SkeletonGpu(0u).accelerationStructure.address;
    instances[2].transform = {{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, kRouteFloorWorldY,
        0.0f, 0.0f, 1.0f, -4.65f}};
    instances[3] = instances[1];
    instances[3].instanceCustomIndex = 3u;
    instances[3].mask = 0x02u;
    instances[3].accelerationStructureReference = swordBlas_.address;
    instances[kPlayerWorldBodyInstanceIndex] = instances[0];
    instances[kPlayerWorldBodyInstanceIndex].instanceCustomIndex = kPlayerWorldBodyInstanceIndex;
    // The complete coat remains visible in mirror/reflection rays. Keeping its
    // chest out of first-person primary rays avoids a near-camera slab while
    // articulated arms, pelvis, legs and boots stay visible on mask 0x04.
    instances[kPlayerWorldBodyInstanceIndex].mask = 0x10u;
    instances[kPlayerWorldBodyInstanceIndex].accelerationStructureReference = playerBodyBlas_.address;
    instances[kPlayerWorldBodyInstanceIndex].transform = {{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, kShowcaseEyeWorldY,
        0.0f, 0.0f, 1.0f, 0.0f}};
    for (std::size_t i = 5u; i <= 16u; ++i)
    {
        instances[i] = instances[kPlayerWorldBodyInstanceIndex];
        instances[i].instanceCustomIndex = static_cast<std::uint32_t>(i);
        instances[i].mask = 0x04u;
        instances[i].accelerationStructureReference = playerLimbBlas_.address;
        instances[i].transform = {{
            0.07f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.07f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.40f, 0.0f}};
    }
    instances[5] = instances[0];
    instances[5].instanceCustomIndex = 9u;
    instances[5].mask = 0u;
    instances[5].accelerationStructureReference = dielectricFixtureBlas_.address;
    instances[5].transform = {{
        0.20f, 0.0f, 0.0f, -9.10f,
        0.0f, 1.25f, 0.0f, -0.325f,
        0.0f, 0.0f, 0.75f, -15.20f}};
    instances[9].accelerationStructureReference = playerLimbBlas_.address;
    instances[16].mask = 0x10u;
    instances[17] = instances[0];
    instances[17].instanceCustomIndex = 17u;
    instances[17].mask = 0x20u;
    instances[17].accelerationStructureReference = finaleRoofBlas_.address;
    instances[18] = instances[2];
    instances[18].instanceCustomIndex = CharacterRenderSlot::kSecondSkeletonTlasInstanceIndex;
    instances[18].mask = 0u;
    instances[19] = instances[0];
    instances[19].instanceCustomIndex = 19u;
    instances[19].mask = 0x01u;
    instances[19].accelerationStructureReference = waterfallBlas_.address;
    instances[19].transform = {{
        1.0f, 0.0f, 0.0f, -2.32f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, -15.26f}};
    instances[kPlayerViewmodelInstanceIndex] = instances[0];
    instances[kPlayerViewmodelInstanceIndex].instanceCustomIndex = kPlayerViewmodelInstanceIndex;
    instances[kPlayerViewmodelInstanceIndex].mask = 0u;
    instances[kPlayerViewmodelInstanceIndex].accelerationStructureReference =
        viewmodelAvailable_ ? viewmodelBlas_.address : skinnedPlayerBlas_.address;
    // Authored world coordinates use the same ordinary static-PBR instance path.
    instances[kCollapseInstanceIndex] = instances[0];
    instances[kCollapseInstanceIndex].instanceCustomIndex = kCollapseInstanceIndex;
    instances[kCollapseInstanceIndex].mask = 0x01u;
    instances[kCollapseInstanceIndex].accelerationStructureReference = collapseBlas_.address;
    // Metadata slot22 is reserved for the player Rag torch. Its live item
    // remains TLAS slot1; this hidden owner keeps fixed slot correspondence.
    instances[kPlayerTorchInstanceIndex] = instances[1];
    instances[kPlayerTorchInstanceIndex].mask = 0u;
    // Preserve TLAS slot25 as the new scabbard owner while Keeper aliases keep
    // their original physical slots23/24.
    instances[kPlayerSwordScabbardInstanceIndex] = instances[1];
    instances[kPlayerSwordScabbardInstanceIndex].instanceCustomIndex =
        kPlayerSwordScabbardMetadataIndex;
    instances[kPlayerSwordScabbardInstanceIndex].mask = 0u;
    instances[kPlayerSwordScabbardInstanceIndex].accelerationStructureReference =
        playerSwordScabbardBlas_.address;
    if (sceneProfile_ == RtSceneProfile::Showcase)
    {
        instances[kWaterDropletInstanceIndex] = instances[0];
        instances[kWaterDropletInstanceIndex].instanceCustomIndex = kWaterDropletMetadataIndex;
        instances[kWaterDropletInstanceIndex].mask = 0x01u;
        instances[kWaterDropletInstanceIndex].accelerationStructureReference =
            waterDropletBlas_.address;
    }
    ApplyKeeperTorchBodyInstances(instances);
    ApplyGlassFixtureVisibility(instances);
    // Seed the same dormant third-lane owner used by dynamic frames. A zero
    // reference is an inactive instance, but would force a needless definition
    // change before the first live snapshot even while the Keeper stays hidden.
    instances[kKeeperInstanceIndex] = characterSlot_.BuildActiveInstances()[2];
    if (!CreateBuffer(sizeof(instances), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, uploadMemory, true, instanceBuffer_, diagnostic))
    {
        return false;
    }
    if (!gpuResources_.MapBufferForHostWrites(instanceBuffer_, diagnostic) ||
        !WriteBuffer(instanceBuffer_, instances.data(), sizeof(instances), "TLAS instance", diagnostic))
    {
        return false;
    }

    VkAccelerationStructureGeometryKHR tlasGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    tlasGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlasGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    tlasGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    tlasGeometry.geometry.instances.data.deviceAddress = instanceBuffer_.address;

    VkAccelerationStructureBuildGeometryInfoKHR tlasBuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    tlasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    tlasBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    tlasBuildInfo.geometryCount = 1u;
    tlasBuildInfo.pGeometries = &tlasGeometry;

    VkAccelerationStructureBuildSizesInfoKHR tlasSizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    std::uint32_t instanceCount = static_cast<std::uint32_t>(instances.size());
    vkGetAccelerationStructureBuildSizesKHR_(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &tlasBuildInfo, &instanceCount, &tlasSizes);
    if (!CreateBuffer(tlasSizes.accelerationStructureSize,
                      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      true,
                      tlas_.backing,
                      diagnostic))
    {
        return false;
    }

    VkAccelerationStructureCreateInfoKHR tlasCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    tlasCreateInfo.buffer = tlas_.backing.buffer;
    tlasCreateInfo.size = tlasSizes.accelerationStructureSize;
    tlasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR_(device_, &tlasCreateInfo, nullptr, &tlas_.handle) != VK_SUCCESS)
    {
        diagnostic = "Failed to create TLAS.";
        return false;
    }

    if (!CreateScratchBuffer(std::max(tlasSizes.buildScratchSize, tlasSizes.updateScratchSize), tlasUpdateScratch_, diagnostic))
    {
        return false;
    }

    VkAccelerationStructureBuildRangeInfoKHR tlasRange{};
    tlasRange.primitiveCount = instanceCount;
    tlasBuildInfo.dstAccelerationStructure = tlas_.handle;
    tlasBuildInfo.scratchData.deviceAddress = tlasUpdateScratch_.AlignedAddress();
    const VkAccelerationStructureBuildRangeInfoKHR* tlasRanges[] = {&tlasRange};
    BlasBuildData tlasBuildData{this, &tlasBuildInfo, tlasRanges};
    if (!RunOneTimeCommands(buildBlas, &tlasBuildData, diagnostic))
    {
        return false;
    }

    VkAccelerationStructureDeviceAddressInfoKHR tlasAddressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    tlasAddressInfo.accelerationStructure = tlas_.handle;
    tlas_.address = vkGetAccelerationStructureDeviceAddressKHR_(device_, &tlasAddressInfo);
    tlasBuiltInstances_ = instances;
    tlasInstanceDefinitionsValid_ = true;

    diagnostic.clear();
    return true;
}

std::optional<RtCompiledPipelineKey> PresentableTinyRtScene::MakeCompiledPipelineKey(
    const RtPipelineBundlePreflight& preflight) const
{
    if (device_ == VK_NULL_HANDLE ||
        preflight.request.executionBackend != RtExecutionBackend::RayTracingPipeline ||
        preflight.request.executionBackend != ExecutionBackend())
        return std::nullopt;

    const auto artifactIdentity = [](const RtPipelineVariantArtifact& artifact) {
        return RtCachedArtifactIdentity{
            std::string(artifact.canonicalKey), std::string(artifact.spirvSha256),
            std::string(artifact.includeSha256),
            static_cast<std::uint64_t>(artifact.expectedWordCount)};
    };
    const auto binaryIdentity = [](const std::uint32_t* words, const std::size_t byteCount) {
        return std::string(reinterpret_cast<const char*>(words), byteCount);
    };

    RtCompiledPipelineKey key{};
    key.deviceIdentity = static_cast<std::uint64_t>(
        reinterpret_cast<std::uintptr_t>(device_));
    key.backend = preflight.request.executionBackend;
    key.instrumentation = preflight.request.instrumentation;
    key.quality = preflight.request.quality;
    for (std::size_t strategy = 0u; strategy < preflight.strategies.size(); ++strategy)
    {
        const auto& artifact = preflight.strategies[strategy];
        key.strategyArtifacts[strategy] = artifactIdentity(artifact);
        key.strategyStages[strategy] = {
            {VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0u,
             binaryIdentity(artifact.words.data(), artifact.words.size_bytes()),
             "main", {}, {}, {}},
            {VK_SHADER_STAGE_MISS_BIT_KHR, 0u,
             binaryIdentity(kMinimalMissShader, sizeof(kMinimalMissShader)),
             "main", {}, {}, {}},
            {VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0u,
             binaryIdentity(kMinimalClosestHitShader, sizeof(kMinimalClosestHitShader)),
             "main", {}, {}, {}},
        };
    }
    key.sharedShaderModules = {
        {binaryIdentity(kMinimalMissShader, sizeof(kMinimalMissShader)), {}, {},
         sizeof(kMinimalMissShader) / sizeof(kMinimalMissShader[0])},
        {binaryIdentity(kMinimalClosestHitShader, sizeof(kMinimalClosestHitShader)), {}, {},
         sizeof(kMinimalClosestHitShader) / sizeof(kMinimalClosestHitShader[0])},
    };
    key.shaderGroups = {
        {VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR, 0, -1, -1, -1, {}, {}},
        {VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR, 1, -1, -1, -1, {}, {}},
        {VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR, -1, 2, -1, -1, {}, {}},
    };
    key.pipelineCreateFlags = 0u;
    key.maximumRecursionDepth = 1u;
    key.basePipelineIdentity = "VK_NULL_HANDLE";
    key.basePipelineIndex = 0;
    key.pipelineCreateExtensionStateIdentity.clear();

    const auto bindings = TryMakeRtDescriptorSetLayoutBindings(
        preflight.descriptorIo, executionPolicy_.pushConstantStages,
        executionPolicy_.shaderStage);
    if (!bindings) return std::nullopt;
    key.descriptorSetLayoutCreateFlags = 0u;
    key.descriptorBindings.reserve(bindings->count);
    for (std::uint32_t index = 0u; index < bindings->count; ++index)
    {
        const auto& binding = bindings->values[index];
        key.descriptorBindings.push_back({
            binding.binding, static_cast<std::uint32_t>(binding.descriptorType),
            binding.descriptorCount, binding.stageFlags, 0u, {}});
    }
    key.pipelineLayoutCreateFlags = 0u;
    key.pushConstantRanges = {{executionPolicy_.pushConstantStages, 0u,
                               static_cast<std::uint32_t>(sizeof(ScenePushConstants))}};
    key.pipelineLayoutExtensionStateIdentity.clear();
    return key;
}

bool PresentableTinyRtScene::CreateSelectedPipelineBundle(std::string& diagnostic)
{
    RtPipelineBundleBuildApi api{};
    api.user = this;
    api.createDescriptorSetLayout = [](void* user, const RtDescriptorIoContract& contract,
                                       VkDescriptorSetLayout& out, std::string& error) {
        return static_cast<PresentableTinyRtScene*>(user)->CreateBundleDescriptorSetLayout(
            contract, out, error);
    };
    api.createDescriptorPool = [](void* user, const RtDescriptorIoContract& contract,
                                  VkDescriptorPool& out, std::string& error) {
        return static_cast<PresentableTinyRtScene*>(user)->CreateBundleDescriptorPool(
            contract, out, error);
    };
    api.allocateDescriptorSet = [](void* user, VkDescriptorPool pool,
                                   VkDescriptorSetLayout layout, VkDescriptorSet& out,
                                   std::string& error) {
        return static_cast<PresentableTinyRtScene*>(user)->AllocateBundleDescriptorSet(
            pool, layout, out, error);
    };
    api.createDiagnosticBuffer = [](void* user, RtGpuBuffer& out, std::string& error) {
        return static_cast<PresentableTinyRtScene*>(user)->CreateBundleDiagnosticBuffer(
            out, error);
    };
    api.writeDescriptors = [](void* user, RtPipelineBundle& bundle, std::string& error) {
        return static_cast<PresentableTinyRtScene*>(user)->WriteBundleDescriptors(
            bundle, error);
    };
    api.createPipelineLayout = [](void* user, VkDescriptorSetLayout layout,
                                  VkPipelineLayout& out, std::string& error) {
        return static_cast<PresentableTinyRtScene*>(user)->CreateBundlePipelineLayout(
            layout, out, error);
    };
    api.createSharedShaderModules = [](void* user, VkShaderModule& miss,
                                       VkShaderModule& hit, std::string& error) {
        return static_cast<PresentableTinyRtScene*>(user)->CreateBundleSharedShaderModules(
            miss, hit, error);
    };
    api.createEntryShaderModule = [](void* user,
                                      const RtPipelineVariantArtifact& artifact,
                                      VkShaderModule& out, std::string& error) {
        return static_cast<PresentableTinyRtScene*>(user)->CreateBundleEntryShaderModule(
            artifact, out, error);
    };
    api.createStrategyPipeline = [](void* user, RtMaterialStrategy strategy,
                                    VkShaderModule raygen, VkShaderModule miss,
                                    VkShaderModule hit, VkPipelineLayout layout,
                                    VkPipeline& out, std::string& error) {
        return static_cast<PresentableTinyRtScene*>(user)->CreateBundleStrategyPipeline(
            strategy, raygen, miss, hit, layout, out, error);
    };
    api.destroyShaderModule = [](void* user, VkShaderModule& module) noexcept {
        auto& scene = *static_cast<PresentableTinyRtScene*>(user);
        if (scene.device_ != VK_NULL_HANDLE && module != VK_NULL_HANDLE)
            vkDestroyShaderModule(scene.device_, module, nullptr);
        module = VK_NULL_HANDLE;
    };
    api.createStrategySbt = [](void* user, RtMaterialStrategy strategy,
                               VkPipeline pipeline, RtGpuBuffer& out,
                               std::array<VkStridedDeviceAddressRegionKHR, 4u>& regions,
                               std::string& error) {
        return static_cast<PresentableTinyRtScene*>(user)->CreateBundleStrategySbt(
            strategy, pipeline, out, regions, error);
    };
    if (compiledPipelineCache_ != nullptr &&
        ExecutionBackend() == RtExecutionBackend::RayTracingPipeline)
    {
        api.borrowCompiledObjects = [](
            void* user, const RtPipelineBundlePreflight& preflight,
            RtBundleCompiledPipelineLease& lease,
            RtBundleCompiledPipelineObjects& objects) {
            auto& scene = *static_cast<PresentableTinyRtScene*>(user);
            const auto key = scene.MakeCompiledPipelineKey(preflight);
            if (scene.compiledPipelineCache_ == nullptr || !key) return false;
            auto candidateLease = scene.compiledPipelineCache_->Acquire(*key);
            const auto* cached = candidateLease.Get();
            if (!candidateLease || cached == nullptr) return false;
            objects = *cached;
            lease = std::move(candidateLease);
            return true;
        };
        api.publishCompiledObjects = [](
            void* user, const RtPipelineBundlePreflight& preflight,
            RtBundleCompiledPipelineObjects& objects,
            RtBundleCompiledPipelineLease& lease) {
            auto& scene = *static_cast<PresentableTinyRtScene*>(user);
            const auto key = scene.MakeCompiledPipelineKey(preflight);
            if (scene.compiledPipelineCache_ == nullptr || !key ||
                scene.compiledPipelineCache_->Adopt(*key, objects, true, false) !=
                    RtPipelineCacheInsertResult::Adopted)
                return false;
            lease = scene.compiledPipelineCache_->Acquire(*key);
            return static_cast<bool>(lease);
        };
    }
    return BuildRtPipelineBundleResources(pipelineBundle_, api, diagnostic);
}

bool PresentableTinyRtScene::CapturePipelineEvidenceIdentity() noexcept
{
    horde::telemetry::RtPipelineEvidenceIdentity candidate{};
    if (!pipelineBundle_.HasSelection() ||
        !TryMakeRtPipelineEvidenceIdentity(
            pipelineBundle_.Request(),
            pipelineBundle_.Strategy(RtMaterialStrategy::OpaqueFast).artifact,
            pipelineBundle_.Strategy(RtMaterialStrategy::GenericDielectric).artifact,
            candidate))
    {
        pipelineEvidenceIdentity_ = {};
        return false;
    }
#ifdef HORDE_RT_STAGED_PRIMARY_EXPERIMENT
    if (stagedPrimary_) {
        if (!horde::telemetry::AssignRtFixedText(candidate.bundleKey, "staged_primary_v1_investigation_pair") ||
            !horde::telemetry::AssignRtFixedText(candidate.opaqueFast.key, SelectedOpaqueFastKey()) ||
            !horde::telemetry::AssignRtFixedText(candidate.opaqueFast.sha256, SelectedOpaqueFastSha256()) ||
            !horde::telemetry::AssignRtFixedText(candidate.genericDielectric.key, SelectedGenericDielectricKey()) ||
            !horde::telemetry::AssignRtFixedText(candidate.genericDielectric.sha256, SelectedGenericDielectricSha256())) return false;
        candidate.active = candidate.opaqueFast;
    }
#endif
    pipelineEvidenceIdentity_ = candidate;
    return true;
}

bool PresentableTinyRtScene::CreateBundleDescriptorSetLayout(
    const RtDescriptorIoContract& contract,
    VkDescriptorSetLayout& out,
    std::string& diagnostic)
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice_, &properties);
    if (contract.storageBufferDescriptorCount > properties.limits.maxPerStageDescriptorStorageBuffers ||
        contract.storageBufferDescriptorCount > properties.limits.maxDescriptorSetStorageBuffers)
    {
        diagnostic = "Device storage-buffer descriptor limits cannot accommodate the independent player geometry streams.";
        return false;
    }
    if (!ValidateRtSampledDescriptorLimits(contract,
            {properties.limits.maxPerStageDescriptorSamplers, properties.limits.maxDescriptorSetSamplers,
             properties.limits.maxPerStageDescriptorSampledImages, properties.limits.maxDescriptorSetSampledImages}))
    {
        diagnostic = "Device sampled-image/sampler limits cannot accommodate the shared RT environment.";
        return false;
    }
    const auto bindings = TryMakeRtDescriptorSetLayoutBindings(contract,
        executionPolicy_.pushConstantStages, executionPolicy_.shaderStage);
    if (!bindings)
    {
        diagnostic = "Selected RT descriptor roster exceeds its capacity or contains an invalid resource kind.";
        return false;
    }
    const VkDescriptorSetLayoutCreateInfo layoutInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0u,
        bindings->count, bindings->values.data()};
    if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &out) != VK_SUCCESS)
    {
        diagnostic = "Failed to create selected RT descriptor set layout.";
        return false;
    }
    return true;
}

bool PresentableTinyRtScene::CreateBundleDescriptorPool(
    const RtDescriptorIoContract& contract,
    VkDescriptorPool& out,
    std::string& diagnostic)
{
    const std::array<VkDescriptorPoolSize, 4u> poolSizes{{
        {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1u},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1u},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, contract.storageBufferDescriptorCount},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, contract.combinedImageSamplerDescriptorCount},
    }};
    const VkDescriptorPoolCreateInfo poolInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr, 0u, 1u,
        static_cast<std::uint32_t>(poolSizes.size()), poolSizes.data()};
    if (vkCreateDescriptorPool(device_, &poolInfo, nullptr, &out) != VK_SUCCESS)
    {
        diagnostic = "Failed to create selected RT descriptor pool.";
        return false;
    }
    return true;
}

bool PresentableTinyRtScene::AllocateBundleDescriptorSet(
    VkDescriptorPool pool,
    VkDescriptorSetLayout layout,
    VkDescriptorSet& out,
    std::string& diagnostic)
{
    const VkDescriptorSetAllocateInfo allocateInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr, pool, 1u, &layout};
    if (vkAllocateDescriptorSets(device_, &allocateInfo, &out) != VK_SUCCESS)
    {
        diagnostic = "Failed to allocate selected RT descriptor set.";
        return false;
    }
    return true;
}

bool PresentableTinyRtScene::CreateBundleDiagnosticBuffer(
    Buffer& out,
    std::string& diagnostic)
{
    static_assert(sizeof(RtDielectricDiagnostics) == 176u);
    static_assert(alignof(RtDielectricDiagnostics) == 16u);
    const RtDielectricDiagnostics zeros{};
    if (!CreateBuffer(sizeof(zeros), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      false, out, diagnostic))
        return false;
    return WriteBuffer(out, &zeros, sizeof(zeros), "dielectric diagnostics", diagnostic);
}

bool PresentableTinyRtScene::WriteBundleDescriptors(RtPipelineBundle& bundle,
                                                    std::string& diagnostic)
{
    const bool entry = sceneProfile_ == RtSceneProfile::EntryMenu;
    const auto &skeletonVertexBuffer_ =
        entry ? staticVertexBuffer_ : characterSlot_.SkeletonGpu(0u).vertices;
    // The fixed production descriptor layout is shared across content profiles.
    // Unadmitted actor bindings alias valid buffers; no extra actor is allocated.
    const bool preview = sceneProfile_ != RtSceneProfile::Showcase;
    const auto& secondSkeletonVertexBuffer = preview
        ? skeletonVertexBuffer_ : characterSlot_.SkeletonGpu(1u).vertices;
    const auto& lichVertexBuffer_ = preview
        ? staticVertexBuffer_ : characterSlot_.LichGpu().vertices;
    const VkDescriptorSet descriptorSet = bundle.descriptorSet;

    VkWriteDescriptorSetAccelerationStructureKHR asWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
    asWrite.accelerationStructureCount = 1u;
    asWrite.pAccelerationStructures = &tlas_.handle;
    VkWriteDescriptorSet accelerationStructureWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    accelerationStructureWrite.pNext = &asWrite;
    accelerationStructureWrite.dstSet = descriptorSet;
    accelerationStructureWrite.dstBinding = 0u;
    accelerationStructureWrite.descriptorCount = 1u;
    accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo.imageView = storageImageView_;
    VkWriteDescriptorSet imageWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    imageWrite.dstSet = descriptorSet;
    imageWrite.dstBinding = 1u;
    imageWrite.descriptorCount = 1u;
    imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    imageWrite.pImageInfo = &imageInfo;

    VkDescriptorBufferInfo skeletonBufferInfo{};
    skeletonBufferInfo.buffer = skeletonVertexBuffer_.buffer;
    skeletonBufferInfo.offset = 0u;
    skeletonBufferInfo.range = skeletonVertexBuffer_.size;
    VkWriteDescriptorSet skeletonWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    skeletonWrite.dstSet = descriptorSet;
    skeletonWrite.dstBinding = 2u;
    skeletonWrite.descriptorCount = 1u;
    skeletonWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    skeletonWrite.pBufferInfo = &skeletonBufferInfo;

    VkDescriptorBufferInfo secondSkeletonBufferInfo{};
    secondSkeletonBufferInfo.buffer = secondSkeletonVertexBuffer.buffer;
    secondSkeletonBufferInfo.offset = 0u;
    secondSkeletonBufferInfo.range = secondSkeletonVertexBuffer.size;
    VkWriteDescriptorSet secondSkeletonWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    secondSkeletonWrite.dstSet = descriptorSet;
    secondSkeletonWrite.dstBinding = 10u;
    secondSkeletonWrite.descriptorCount = 1u;
    secondSkeletonWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    secondSkeletonWrite.pBufferInfo = &secondSkeletonBufferInfo;

    const auto bufferWrite = [descriptorSet](std::uint32_t binding,
                                             const VkDescriptorBufferInfo* info) {
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = descriptorSet;
        write.dstBinding = binding;
        write.descriptorCount = 1u;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo = info;
        return write;
    };
    const VkDescriptorBufferInfo instanceMetadataInfo{
        instanceMetadataBuffer_.buffer, 0u, instanceMetadataBuffer_.size};
    const VkDescriptorBufferInfo primitiveMetadataInfo{
        primitiveMetadataBuffer_.buffer, 0u, primitiveMetadataBuffer_.size};
    const VkDescriptorBufferInfo materialMetadataInfo{
        materialMetadataBuffer_.buffer, 0u, materialMetadataBuffer_.size};
    const VkDescriptorBufferInfo staticVertexInfo{
        staticVertexBuffer_.buffer, 0u, staticVertexBuffer_.size};
    const auto& worldVertices = preview ? staticVertexBuffer_ : worldPlayerVertexBuffer_;
    const auto& viewVertices = preview ? staticVertexBuffer_ : viewmodelVertexBuffer_;
    const VkDescriptorBufferInfo worldVertexInfo{worldVertices.buffer, 0u, worldVertices.size};
    const VkDescriptorBufferInfo viewVertexInfo{viewVertices.buffer, 0u, viewVertices.size};
    const VkDescriptorBufferInfo staticIndexInfo{
        staticIndexBuffer_.buffer, 0u, staticIndexBuffer_.size};
    const VkDescriptorBufferInfo heldLightInfo{
        heldLightBuffer_.buffer, 0u, heldLightBuffer_.size};
    const VkDescriptorBufferInfo fireEmitterInfo{
        fireEmitterBuffer_.buffer, 0u, fireEmitterBuffer_.size};
    const VkDescriptorBufferInfo qualityControlsInfo{
        qualityControlsBuffer_.buffer, 0u, qualityControlsBuffer_.size};
    const VkDescriptorBufferInfo waterContactRippleInfo{
        waterContactRippleBuffer_.buffer, 0u, waterContactRippleBuffer_.size};
    std::optional<VkDescriptorBufferInfo> dielectricDiagnosticsInfo;
    if (bundle.DescriptorIo().diagnosticIo.descriptorInfo)
        dielectricDiagnosticsInfo.emplace(VkDescriptorBufferInfo{
            bundle.diagnosticBuffer.buffer, 0u, bundle.diagnosticBuffer.size});

    VkDescriptorBufferInfo lichBufferInfo{};
    lichBufferInfo.buffer = lichVertexBuffer_.buffer;
    lichBufferInfo.offset = 0u;
    lichBufferInfo.range = lichVertexBuffer_.size;
    VkWriteDescriptorSet lichBufferWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    lichBufferWrite.dstSet = descriptorSet;
    lichBufferWrite.dstBinding = 7u;
    lichBufferWrite.descriptorCount = 1u;
    lichBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    lichBufferWrite.pBufferInfo = &lichBufferInfo;

    VkDescriptorBufferInfo worldSurfaceBufferInfo{};
    worldSurfaceBufferInfo.buffer = worldSurfaceBuffer_.buffer;
    worldSurfaceBufferInfo.offset = 0u;
    worldSurfaceBufferInfo.range = worldSurfaceBuffer_.size;
    VkWriteDescriptorSet worldSurfaceWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    worldSurfaceWrite.dstSet = descriptorSet;
    worldSurfaceWrite.dstBinding = 6u;
    worldSurfaceWrite.descriptorCount = 1u;
    worldSurfaceWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    worldSurfaceWrite.pBufferInfo = &worldSurfaceBufferInfo;

    const VkDescriptorImageInfo diffuseInfo{materialSampler_, materialDiffuse_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo normalInfo{materialSampler_, materialNormal_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo armInfo{materialSampler_, materialArm_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo lichBaseInfo{materialSampler_, preview ? staticBaseColor_.view : lichBaseColor_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo lichEmissiveInfo{materialSampler_, preview ? staticEmissive_.view : lichEmissive_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo staticBaseInfo{
        materialSampler_, genericStaticAssetEnabled_ ? staticBaseColor_.view : materialDiffuse_.view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo staticNormalInfo{
        materialSampler_, genericStaticAssetEnabled_ ? staticNormal_.view : materialNormal_.view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo staticOrmInfo{
        materialSampler_, genericStaticAssetEnabled_ ? staticOrm_.view : materialArm_.view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo staticEmissiveInfo{
        materialSampler_, genericStaticAssetEnabled_ ? staticEmissive_.view : lichEmissive_.view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo environmentInfo{environmentSampler_, environmentTexture_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const auto sampledWrite = [descriptorSet](std::uint32_t binding,
                                              const VkDescriptorImageInfo* info) {
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = descriptorSet;
        write.dstBinding = binding;
        write.descriptorCount = 1u;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = info;
        return write;
    };
    std::vector<VkWriteDescriptorSet> writes{
        accelerationStructureWrite, imageWrite, skeletonWrite,
        sampledWrite(3u, &diffuseInfo), sampledWrite(4u, &normalInfo),
        sampledWrite(5u, &armInfo), worldSurfaceWrite, lichBufferWrite,
        sampledWrite(8u, &lichBaseInfo), sampledWrite(9u, &lichEmissiveInfo),
        secondSkeletonWrite,
        bufferWrite(kRtBindingInstanceMetadata, &instanceMetadataInfo),
        bufferWrite(kRtBindingPrimitiveMetadata, &primitiveMetadataInfo),
        bufferWrite(kRtBindingMaterials, &materialMetadataInfo),
        bufferWrite(kRtBindingStaticVertices, &staticVertexInfo),
        bufferWrite(kRtBindingStaticIndices, &staticIndexInfo),
        sampledWrite(kRtBindingBaseColorTextures, &staticBaseInfo),
        sampledWrite(kRtBindingNormalTextures, &staticNormalInfo),
        sampledWrite(kRtBindingOrmTextures, &staticOrmInfo),
        sampledWrite(kRtBindingEmissiveTextures, &staticEmissiveInfo),
        bufferWrite(kRtBindingHeldLight, &heldLightInfo),
        bufferWrite(kRtBindingFireEmitters, &fireEmitterInfo),
        bufferWrite(kRtBindingQualityControls, &qualityControlsInfo),
        bufferWrite(kRtBindingWaterContactRipple, &waterContactRippleInfo),
        bufferWrite(kRtBindingWorldPlayerVertices, &worldVertexInfo),
        bufferWrite(kRtBindingViewmodelVertices, &viewVertexInfo),
        sampledWrite(kRtBindingEnvironmentTexture, &environmentInfo)};
    if (bundle.DescriptorIo().diagnosticIo.descriptorWrite &&
        dielectricDiagnosticsInfo.has_value())
        writes.push_back(bufferWrite(kRtBindingDielectricDiagnostics,
                                     &*dielectricDiagnosticsInfo));
    if (writes.size() != bundle.DescriptorIo().descriptorWriteCount)
    {
        diagnostic = "Selected RT descriptor write count disagrees with its contract.";
        return false;
    }
    vkUpdateDescriptorSets(device_, static_cast<std::uint32_t>(writes.size()), writes.data(), 0u, nullptr);

    diagnostic.clear();
    return true;
}

bool PresentableTinyRtScene::CreateBundlePipelineLayout(
    VkDescriptorSetLayout descriptorSetLayout,
    VkPipelineLayout& out,
    std::string& diagnostic)
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice_, &properties);
    if (properties.limits.maxPushConstantsSize < sizeof(ScenePushConstants))
    {
        diagnostic = "Device maxPushConstantsSize is below the required 128-byte RT scene ABI.";
        return false;
    }
    const VkPushConstantRange pushConstantRange{
        executionPolicy_.pushConstantStages,
        0u, sizeof(ScenePushConstants)};
    const VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0u, 1u,
        &descriptorSetLayout, 1u, &pushConstantRange};
    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &out) != VK_SUCCESS)
    {
        diagnostic = "Failed to create selected RT pipeline layout.";
        return false;
    }
    return true;
}

bool PresentableTinyRtScene::CreateBundleSharedShaderModules(
    VkShaderModule& miss,
    VkShaderModule& hit,
    std::string& diagnostic)
{
    if (!CreateShaderModule(device_, kMinimalMissShader,
                            sizeof(kMinimalMissShader), miss) ||
        !CreateShaderModule(device_, kMinimalClosestHitShader,
                            sizeof(kMinimalClosestHitShader), hit))
    {
        diagnostic = "Failed to create shared RT shader modules.";
        return false;
    }
    return true;
}

bool PresentableTinyRtScene::CreateBundleEntryShaderModule(
    const RtPipelineVariantArtifact& artifact,
    VkShaderModule& out,
    std::string& diagnostic)
{
    if (artifact.words.empty() ||
        !CreateShaderModule(device_, artifact.words.data(),
                            artifact.words.size_bytes(), out))
    {
        diagnostic = std::string("Failed to create selected RT entry module: ") +
            std::string(artifact.canonicalKey);
        return false;
    }
    return true;
}

bool PresentableTinyRtScene::CreateBundleStrategyPipeline(
    RtMaterialStrategy strategy,
    VkShaderModule raygenModule,
    VkShaderModule missModule,
    VkShaderModule hitModule,
    VkPipelineLayout layout,
    VkPipeline& out,
    std::string& diagnostic)
{
    const auto backend = ExecutionBackend();
    const std::string_view strategyName =
        strategy == RtMaterialStrategy::GenericDielectric ? "generic-dielectric" : "opaque-fast";
    initialiseMeasurements_.pipelines.push_back({
        strategyName, backend, VK_NOT_READY, 0u, false, false});
    auto& timing = initialiseMeasurements_.pipelines.back();
    if (backend == RtExecutionBackend::RayQueryCompute)
    {
        VkComputePipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        pipelineInfo.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0u,
                              VK_SHADER_STAGE_COMPUTE_BIT, raygenModule, "main", nullptr};
        pipelineInfo.layout = layout;
        const VkPipelineCache pipelineCache = pipelineCache_;
        timing.attempted = true;
        timing.pipelineCacheWasNull = pipelineCache == VK_NULL_HANDLE;
        VkResult result = VK_NOT_READY;
        {
            [[maybe_unused]] CpuElapsedTimer timer(timing.cpuNanoseconds);
            result = vkCreateComputePipelines(device_, pipelineCache, 1u, &pipelineInfo, nullptr, &out);
        }
        timing.result = result;
        if (result != VK_SUCCESS)
        {
            diagnostic = "Failed to create selected hardware RayQuery compute pipeline.";
            return false;
        }
        diagnostic.clear();
        return true;
    }
    std::array<VkPipelineShaderStageCreateInfo, 3u> stages{{
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0u, VK_SHADER_STAGE_RAYGEN_BIT_KHR, raygenModule, "main", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0u, VK_SHADER_STAGE_MISS_BIT_KHR, missModule, "main", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0u, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, hitModule, "main", nullptr},
    }};

    std::array<VkRayTracingShaderGroupCreateInfoKHR, 3u> groups{};
    groups[0].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groups[0].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    groups[0].generalShader = 0u;
    groups[0].closestHitShader = VK_SHADER_UNUSED_KHR;
    groups[0].anyHitShader = VK_SHADER_UNUSED_KHR;
    groups[0].intersectionShader = VK_SHADER_UNUSED_KHR;
    groups[1].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groups[1].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    groups[1].generalShader = 1u;
    groups[1].closestHitShader = VK_SHADER_UNUSED_KHR;
    groups[1].anyHitShader = VK_SHADER_UNUSED_KHR;
    groups[1].intersectionShader = VK_SHADER_UNUSED_KHR;
    groups[2].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groups[2].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    groups[2].generalShader = VK_SHADER_UNUSED_KHR;
    groups[2].closestHitShader = 2u;
    groups[2].anyHitShader = VK_SHADER_UNUSED_KHR;
    groups[2].intersectionShader = VK_SHADER_UNUSED_KHR;

    VkRayTracingPipelineCreateInfoKHR pipelineInfo{VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
    pipelineInfo.stageCount = static_cast<std::uint32_t>(stages.size());
    pipelineInfo.pStages = stages.data();
    pipelineInfo.groupCount = static_cast<std::uint32_t>(groups.size());
    pipelineInfo.pGroups = groups.data();
    pipelineInfo.maxPipelineRayRecursionDepth = 1u;
    pipelineInfo.layout = layout;
    const VkPipelineCache pipelineCache = pipelineCache_;
    timing.attempted = true;
    timing.pipelineCacheWasNull = pipelineCache == VK_NULL_HANDLE;
    VkResult result = VK_NOT_READY;
    {
        [[maybe_unused]] CpuElapsedTimer timer(timing.cpuNanoseconds);
        result = vkCreateRayTracingPipelinesKHR_(
            device_, VK_NULL_HANDLE, pipelineCache, 1u, &pipelineInfo, nullptr, &out);
    }
    timing.result = result;
    if (result != VK_SUCCESS)
    {
        diagnostic = "Failed to create selected material-strategy RT pipeline.";
        return false;
    }

    diagnostic.clear();
    return true;
}

bool PresentableTinyRtScene::CreateBundleStrategySbt(
    RtMaterialStrategy strategy,
    VkPipeline pipeline,
    Buffer& out,
    std::array<VkStridedDeviceAddressRegionKHR, 4u>& regions,
    std::string& diagnostic)
{
    const std::string_view strategyName =
        strategy == RtMaterialStrategy::GenericDielectric ? "generic-dielectric" : "opaque-fast";
    initialiseMeasurements_.shaderBindingTables.push_back(
        {strategyName, 0u, true, false});
    auto& timing = initialiseMeasurements_.shaderBindingTables.back();
    [[maybe_unused]] CpuElapsedTimer timingTimer(timing.cpuNanoseconds);
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
    VkPhysicalDeviceProperties2 properties2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    properties2.pNext = &rtProperties;
    auto getPhysicalDeviceProperties2 = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
        vkGetInstanceProcAddr(instance_, "vkGetPhysicalDeviceProperties2"));
    if (getPhysicalDeviceProperties2 == nullptr)
    {
        getPhysicalDeviceProperties2 = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
            vkGetInstanceProcAddr(instance_, "vkGetPhysicalDeviceProperties2KHR"));
    }
    if (getPhysicalDeviceProperties2 == nullptr)
    {
        diagnostic = "vkGetPhysicalDeviceProperties2 is unavailable for RT pipeline properties.";
        return false;
    }
    getPhysicalDeviceProperties2(physicalDevice_, &properties2);

    constexpr std::uint32_t groupCount = 3u;
    const std::uint32_t handleSize = rtProperties.shaderGroupHandleSize;
    const std::uint32_t baseAlignment = rtProperties.shaderGroupBaseAlignment;
    RtShaderBindingTableLayout layout{};
    if (!TryShaderBindingTableLayout(
            handleSize, rtProperties.shaderGroupHandleAlignment, baseAlignment,
            rtProperties.maxShaderGroupStride, layout) ||
        layout.size > std::numeric_limits<std::size_t>::max() ||
        static_cast<std::uint64_t>(handleSize) * groupCount >
            std::numeric_limits<std::size_t>::max())
    {
        diagnostic = "Invalid or unsupported RT shader binding table alignment/stride/range.";
        return false;
    }
    const VkDeviceSize groupStride = layout.stride;
    const VkDeviceSize regionSize = layout.regionSpacing;
    const VkDeviceSize sbtSize = layout.size;

    const char* label = strategy == RtMaterialStrategy::GenericDielectric
        ? "generic dielectric" : "opaque fast";
    const auto createTable = [&](VkPipeline sourcePipeline,
                                 Buffer& table) -> bool
    {
        std::vector<std::uint8_t> handles(static_cast<std::size_t>(handleSize) * groupCount);
        if (vkGetRayTracingShaderGroupHandlesKHR_(
                device_, sourcePipeline, 0u, groupCount,
                handles.size(), handles.data()) != VK_SUCCESS)
        {
            diagnostic = std::string("Failed to fetch ") + label +
                " RT shader group handles.";
            return false;
        }
        if (!gpuResources_.CreateAlignedBuffer(sbtSize, baseAlignment,
                          VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          table, diagnostic))
            return false;

        std::vector<std::uint8_t> sbtData(static_cast<std::size_t>(sbtSize), 0u);
        for (std::uint32_t group = 0u; group < groupCount; ++group)
        {
            std::memcpy(sbtData.data() + (regionSize * group),
                        handles.data() + (static_cast<std::size_t>(handleSize) * group), handleSize);
        }
        const std::string tableLabel = std::string(label) + " RT shader binding table";
        if (!gpuResources_.WriteBufferRange(table, table.deviceAddressOffset,
                         sbtData.data(), sbtSize,
                         tableLabel.c_str(), diagnostic))
            return false;

        regions[0].deviceAddress = table.AlignedAddress();
        regions[0].stride = groupStride;
        regions[0].size = groupStride;
        regions[1].deviceAddress = table.AlignedAddress() + regionSize;
        regions[1].stride = groupStride;
        regions[1].size = groupStride;
        regions[2].deviceAddress = table.AlignedAddress() + (regionSize * 2u);
        regions[2].stride = groupStride;
        regions[2].size = groupStride;
        regions[3] = {};
        return true;
    };

    if (!createTable(pipeline, out))
        return false;

    diagnostic.clear();
    timing.succeeded = true;
    return true;
}

void PresentableTinyRtScene::ApplyGlassFixtureVisibility(
    std::span<VkAccelerationStructureInstanceKHR> instances) const
{
    if (glassEnabled_ || dielectricFixtureBlas_.address == 0u) return;
    // Keep the fixed instance/BLAS owner, but remove it from every ray mask.
    // Reference identity avoids masking procedural arm custom-index9, which
    // shares this historical index when the fixture is not being inspected.
    for (auto& instance : instances)
        if (instance.instanceCustomIndex == 9u &&
            instance.accelerationStructureReference == dielectricFixtureBlas_.address)
            instance.mask = 0u;
}

bool PresentableTinyRtScene::UpdatePreviewInstances(VkCommandBuffer commandBuffer,
                                                   const RtSceneFrameInputs& frame,
                                                   std::string& diagnostic,
                                                   RtSceneRecordObservation* observation)
{
    uploadedQualityControlsValid_ = false;
    uploadedFireEmittersValid_ = false;
    const bool entry = sceneProfile_ == RtSceneProfile::EntryMenu;
    if (tlasInstanceCount_ != (entry ? 3u : 7u) || tlas_.handle == VK_NULL_HANDLE ||
        (!entry &&
         (frame.skeletonEnemyCount != 1u ||
          frame.roster.selectedEnemy != horde::gameplay::EnemyKind::Skeleton ||
          frame.skeletonEnemies[0].animation != horde::gameplay::EnemyAnimation::Idle ||
          frame.skeletonEnemies[0].action != horde::gameplay::EnemyCombatAction::Locomotion)))
    {
        diagnostic = "Graphics preview requires its admitted seven instances and one idle skeleton.";
        return false;
    }
    if (!entry &&
        !characterSlot_.PrepareFrame(frame.skeletonEnemies, frame.skeletonEnemyCount, frame.roster,
                                     frame.lich, gpuResources_, diagnostic, observation))
        return false;
    auto instances = tlasBuiltInstances_;
    if (!entry)
    {
        instances[2] = characterSlot_.BuildActiveInstances()[0];
        const auto tuning = ClampRtSceneTuning(frame.tuning);
        const auto waterScale = ResolveWaterfallCurtainScale(tuning);
        instances[6].transform.matrix[0][0] = waterScale.depth;
        instances[6].transform.matrix[1][1] = waterScale.vertical;
        instances[6].transform.matrix[2][2] = waterScale.crossLane;
        instances[6].mask = frame.waterQuality == WaterQuality::Off ? 0u : 0x01u;
        instances[5].transform.matrix[0][0] = previewTransforms_[3][0] * tuning.glassDepthScale;
        ApplyGlassFixtureVisibility(std::span(instances).first(tlasInstanceCount_));
    }
    const auto tuning = ClampRtSceneTuning(frame.tuning);
    const std::size_t emitterCount = entry ? 1u : previewFireInputs_.size();
    auto emitters = frame.fireEmitters;
    if (frame.fireEmitterCount != emitterCount)
    {
        diagnostic = "Graphics preview requires its two deterministic socket-bound fire emitters.";
        return false;
    }
    for (std::size_t index = 0u; index < emitterCount; ++index)
    {
        // The actual scene transform and admitted sockets are authoritative.
        // Session phase/noise stays deterministic; no gameplay snapshot is changed.
        if (!entry)
        {
            emitters[index].worldFromFlame = previewFireInputs_[index].worldFromFlame;
            emitters[index].worldFromLight = previewFireInputs_[index].worldFromLight;
        }
        emitters[index].zone = frame.zone;
    }
    if (entry)
    {
        using namespace horde::gameplay::items;
        auto scale = IdentityHeldItemTransform();
        scale[0] = scale[5] = scale[10] = horde::graphics::kEntryMenuLanternScale;
        const auto *hinge = FindHeldItemSocket(rewardLanternRingAsset_.sockets, "Hinge");
        if (hinge == nullptr)
        {
            diagnostic = "Entry menu lost its admitted Hinge.";
            return false;
        }
        HeldItemTransform ring{};
        auto ringSocket = hinge->world;
        for (std::size_t axis = 12; axis < 15; ++axis)
            ringSocket[axis] *= horde::graphics::kEntryMenuLanternScale;
        if (!ComposeWorldFromItem(frame.rewardLanternWorldFromHinge, ringSocket, ring, diagnostic))
            return false;
        ring = MultiplyHeldItemTransforms(ring, scale);
        const auto body = MultiplyHeldItemTransforms(frame.lanternPendulum.worldFromBody, scale);
        const auto apply =
            [](VkAccelerationStructureInstanceKHR &instance, const HeldItemTransform &matrix)
        {
            instance.transform = {{matrix[0], matrix[4], matrix[8], matrix[12], matrix[1],
                                   matrix[5], matrix[9], matrix[13], matrix[2], matrix[6],
                                   matrix[10], matrix[14]}};
        };
        apply(instances[1], ring);
        apply(instances[2], body);
        emitters[0].worldFromFlame = MultiplyHeldItemTransforms(
            body, FindHeldItemSocket(rewardLanternBodyAsset_.sockets, "Flame")->world);
        emitters[0].worldFromLight = MultiplyHeldItemTransforms(
            body, FindHeldItemSocket(rewardLanternBodyAsset_.sockets, "Light")->world);
    }
    const auto quality = ResolveRtQualityControls(frame.shadowQuality, tuning.workloadPreset,
        pipelineBundle_.Request().quality == DielectricQuality::High, mistEnabled_, dustQuality_);
    if (!quality) { diagnostic = "Graphics preview shadow quality is invalid."; return false; }
    FireEmitterUpload fire{};
    const auto fireDetail = frame.fireDetail.value_or(frame.waterQuality == WaterQuality::High
        ? FireEmitterQuality::High : FireEmitterQuality::Mobile);
    if (!BuildFireEmitterUpload(
            std::span<const horde::gameplay::effects::FireEmitterState>(emitters).first(
                emitterCount),
            {{frame.cameraX, horde::gameplay::simulation::PlayerEyeWorldY(frame.playerSupportWorldY), frame.cameraZ}, frame.zone, 24.0f},
            {tuning.fireStrengthScale, tuning.fireTurbulenceScale, tuning.fireSmokeScale},
            fireDetail, fire, diagnostic))
        return false;
    const auto& lightTransform = previewFireInputs_[0].worldFromLight;
    const RtHeldLightGpu light = BuildPlayerFrameLight(
        {lightTransform[12], lightTransform[13], lightTransform[14], frame.torchLightStrength}, frame.playerSupportWorldY);
    auto metadata = staticMeshSlot_.InstanceMetadata();
    if (!glassEnabled_) metadata[9u].flags = 0u;
    auto materials = sceneMaterials_;
    if (!entry)
    {
        if (dielectricFixtureMaterialIndex_ >= materials.size())
        {
            diagnostic =
                "Graphics preview fixture material is outside the admitted material range.";
            return false;
        }
        auto &fixture = materials[dielectricFixtureMaterialIndex_];
        fixture.metallicRoughnessOcclusionTransmission[1] = tuning.glassRoughness;
        fixture.metallicRoughnessOcclusionTransmission[3] = tuning.glassTransmission;
        fixture.iorThicknessAttenuationDistance[0] = tuning.glassIor;
        fixture.iorThicknessAttenuationDistance[2] = tuning.glassAttenuationDistance;
        for (std::size_t channel = 0u; channel < 3u; ++channel)
            fixture.attenuationColor[channel] = tuning.glassAttenuationColor[channel];
    }
    genericTransmissionActive_ = HasActiveGenericTransmission(metadata, staticMeshSlot_.PrimitiveMetadata(), materials);
    framePipelineEvidence_ = pipelineEvidenceIdentity_;
    framePipelineEvidenceValid_ = pipelineEvidenceIdentityValid_;
    framePipelineEvidence_.activeStrategy = genericTransmissionActive_
        ? horde::telemetry::RtMaterialStrategy::GenericDielectric : horde::telemetry::RtMaterialStrategy::OpaqueFast;
    framePipelineEvidence_.active = genericTransmissionActive_
        ? framePipelineEvidence_.genericDielectric : framePipelineEvidence_.opaqueFast;
    switch (frame.waterQuality)
    {
    case WaterQuality::Off: framePipelineEvidence_.waterQuality = horde::telemetry::RtWaterQuality::Off; break;
    case WaterQuality::Mobile: framePipelineEvidence_.waterQuality = horde::telemetry::RtWaterQuality::Mobile; break;
    case WaterQuality::High: framePipelineEvidence_.waterQuality = horde::telemetry::RtWaterQuality::High; break;
    default: framePipelineEvidenceValid_ = false; break;
    }
    const auto contactRipple = MakeWaterContactRippleGpu(frame.waterContact);
    if (!WriteDustQuality(*quality, frame, diagnostic, observation) ||
        !WriteBuffer(waterContactRippleBuffer_, &contactRipple, sizeof(contactRipple),
                     "preview water contact ripple", diagnostic, observation) ||
        !WriteBuffer(heldLightBuffer_, &light, sizeof(light), "preview light", diagnostic, observation) ||
        !WriteBuffer(fireEmitterBuffer_, fire.emitters.data(), sizeof(fire.emitters), "preview fire", diagnostic, observation) ||
        !WriteBuffer(instanceBuffer_, instances.data(), tlasInstanceCount_ * sizeof(instances[0]), "preview instances", diagnostic, observation) ||
        !WriteBuffer(materialMetadataBuffer_, materials.data(), materials.size() * sizeof(materials[0]),
                     "preview materials", diagnostic, observation)) return false;
    const bool diagnosticsAvailable = pipelineBundle_.DiagnosticAvailability() == RtDiagnosticAvailability::Available;
    if (diagnosticsAvailable)
    {
        const RtDielectricDiagnostics cleared{};
        if (!WriteBuffer(pipelineBundle_.diagnosticBuffer, &cleared, sizeof(cleared), "preview diagnostics reset", diagnostic))
        {
            if (observation != nullptr) observation->failure = RtSceneRecordFailure::DiagnosticReset;
            return false;
        }
        if (observation != nullptr) observation->diagnosticResetCompleted = true;
    }
    VkMemoryBarrier hostBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    hostBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    hostBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_SHADER_READ_BIT;
    if (pipelineBundle_.DescriptorIo().diagnosticIo.shaderWriteBarrier) hostBarrier.dstAccessMask |= VK_ACCESS_SHADER_WRITE_BIT;
    ExecuteObservedRtSceneCommand(observation, RtSceneCommandEvent::HostWriteBarrier, [&]() noexcept {
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | executionPolicy_.shaderPipelineStage,
                             0u, 1u, &hostBarrier, 0u, nullptr, 0u, nullptr);
    });
    const bool updateSkeleton = !entry && HasCharacterBlasRefit(characterSlot_.PendingRefit(),
                                                                CharacterBlasRefit::SkeletonPose0);
    RtSceneStageScope refitScope(updateSkeleton ? observation : nullptr, horde::telemetry::RtStage::BlasRefitRecord);
    const auto& skeleton = characterSlot_.SkeletonGpu(0u);
    VkMemoryBarrier blasBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    blasBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    blasBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    ExecuteObservedDynamicBlasCommands(observation, std::array<bool, 1u>{{updateSkeleton}}, [&](std::size_t) noexcept {
        VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR; geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        auto& triangles = geometry.geometry.triangles;
        triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        triangles.vertexData.deviceAddress = skeleton.vertices.address; triangles.vertexStride = skeleton.vertexStride;
        triangles.maxVertex = skeleton.vertexCount - 1u; triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
        VkAccelerationStructureBuildGeometryInfoKHR update{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        update.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        update.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        update.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
        update.srcAccelerationStructure = update.dstAccelerationStructure = skeleton.accelerationStructure.handle;
        update.geometryCount = 1u; update.pGeometries = &geometry;
        update.scratchData.deviceAddress = skeleton.updateScratch.AlignedAddress();
        VkAccelerationStructureBuildRangeInfoKHR range{}; range.primitiveCount = skeleton.vertexCount / 3u;
        const VkAccelerationStructureBuildRangeInfoKHR* ranges[]{&range};
        vkCmdBuildAccelerationStructuresKHR_(commandBuffer, 1u, &update, ranges);
    }, [&]() noexcept {
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             0u, 1u, &blasBarrier, 0u, nullptr, 0u, nullptr);
    });
    refitScope.Complete(updateSkeleton ? 1u : 0u);
    RtSceneStageScope tlasScope(observation, horde::telemetry::RtStage::TlasUpdateRecord);
    VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.data.deviceAddress = instanceBuffer_.address;
    const bool rebuild = !tlasInstanceDefinitionsValid_ || RequiresTlasInstanceRebuild(
        std::span<const VkAccelerationStructureInstanceKHR>(tlasBuiltInstances_).first(tlasInstanceCount_),
        std::span<const VkAccelerationStructureInstanceKHR>(instances).first(tlasInstanceCount_));
    VkAccelerationStructureBuildGeometryInfoKHR update{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    update.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    update.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    update.mode = rebuild ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
    update.srcAccelerationStructure = rebuild ? VK_NULL_HANDLE : tlas_.handle;
    update.dstAccelerationStructure = tlas_.handle; update.geometryCount = 1u; update.pGeometries = &geometry;
    update.scratchData.deviceAddress = tlasUpdateScratch_.AlignedAddress();
    VkAccelerationStructureBuildRangeInfoKHR range{}; range.primitiveCount = tlasInstanceCount_;
    const VkAccelerationStructureBuildRangeInfoKHR* ranges[]{&range};
    ExecuteObservedTlasUpdateCommands(observation, [&]() noexcept {
        vkCmdBuildAccelerationStructuresKHR_(commandBuffer, 1u, &update, ranges);
    }, [&]() noexcept {
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             executionPolicy_.shaderPipelineStage, 0u, 1u, &blasBarrier, 0u, nullptr, 0u, nullptr);
    });
    tlasScope.Complete(1u);
    if (rebuild) { tlasPendingInstances_ = instances; tlasPendingDefinitionsValid_ = true; }
    lastInstanceMasks_ = {};
    for (std::size_t index = 0u; index < tlasInstanceCount_; ++index)
        lastInstanceMasks_[instances[index].instanceCustomIndex] = instances[index].mask;
    lastPlayerPrimaryVisible_ = false;
    lastPlayerWorldBodyInstanceFlags_ = 0u;
    uploadedQualityControls_ = *quality;
    uploadedFireQuality_ = fireDetail;
    uploadedFireEmitters_ = fire;
    uploadedFireEmittersValid_ = true;
    uploadedQualityControlsValid_ = true;
    diagnostic.clear();
    return true;
}

void PresentableTinyRtScene::ApplyKeeperTorchBodyInstances(
    std::array<VkAccelerationStructureInstanceKHR, kTlasInstanceCount>& instances) const
{
    static_assert(horde::gameplay::effects::kKeeperTorchAnchors.size() == kKeeperTorchInstanceCount);
    for (std::size_t index = 0u; index < kKeeperTorchInstanceCount; ++index)
    {
        const auto worldFromItem = horde::gameplay::effects::KeeperTorchWorldFromItem(
            horde::gameplay::effects::kKeeperTorchAnchors[index]);
        auto& instance = instances[kKeeperTorchFirstTlasInstance + index];
        instance = instances[0];
        instance.instanceCustomIndex = 1u; // Existing admitted StaticPbr torch metadata alias.
        instance.mask = instances[0].mask & 0x01u;
        instance.accelerationStructureReference = worldTorchBodyBlas_.address;
        instance.transform = {{
            worldFromItem[0], worldFromItem[4], worldFromItem[8], worldFromItem[12],
            worldFromItem[1], worldFromItem[5], worldFromItem[9], worldFromItem[13],
            worldFromItem[2], worldFromItem[6], worldFromItem[10], worldFromItem[14]}};
    }
}

bool PresentableTinyRtScene::UpdateDynamicInstances(VkCommandBuffer commandBuffer,
                                                     const RtSceneFrameInputs& frame,
                                                     std::string& diagnostic,
                                                     RtSceneRecordObservation* observation)
{
    uploadedQualityControlsValid_ = false;
    uploadedFireEmittersValid_ = false;
    if (sceneProfile_ != RtSceneProfile::Showcase)
        return UpdatePreviewInstances(commandBuffer, frame, diagnostic, observation);
    const bool waterDropletsVisible = genericStaticAssetEnabled_ &&
        frame.waterQuality != WaterQuality::Off &&
        std::any_of(frame.waterContact.droplets.begin(), frame.waterContact.droplets.end(),
                    [](const auto& droplet) { return droplet.active; });
#ifndef NDEBUG
    // Captures after this call are valid only if this frame finishes preparing
    // and recording the current skinned world-body instance successfully.
    playerWorldBodyPoseCurrent_ = false;
#endif
    const RtSceneTuning clampedTuning = ClampRtSceneTuning(frame.tuning);
    const bool glassFixtureVisible =
        glassEnabled_ && clampedTuning.glassFixtureVisible &&
        !clampedTuning.productionRewardPropsVisible;
    const bool productionInspection = clampedTuning.productionRewardPropsVisible &&
        frame.chestReward.phase ==
            horde::gameplay::interactions::ChestRewardPhase::Locked;
    const bool rewardLanternClaimed =
        frame.chestReward.phase ==
            horde::gameplay::interactions::ChestRewardPhase::LanternClaimed &&
        frame.interaction.heldLightKind ==
            horde::gameplay::interactions::HeldLightKind::RewardLantern;
    const ProductionSceneVisibility productionVisibility =
        BuildProductionSceneVisibility({frame.playerRenderRoute,
                                        glassFixtureVisible,
                                        productionInspection,
                                        rewardLanternClaimed,
                                        playerBodyRemainderAvailable_});
    const bool productionRewardWorldVisible =
        productionVisibility.rewardWorldVisible;
    const PlayerRenderRoute effectivePlayerRenderRoute =
        productionVisibility.playerRoute;
    if (effectivePlayerRenderRoute != measuredPlayerRoute_)
    {
        measuredPlayerRoute_ = effectivePlayerRenderRoute;
        playerSkinUpdateCount_ = 0u;
        playerSkinTotalMilliseconds_ = 0.0;
        playerMaxSocketErrorMetres_ = 0.0f;
    }
    const float cameraYaw = frame.cameraYaw;
    const float cameraPitch = frame.cameraPitch;
    const float walkTime = frame.walkTime;
    const float cameraX = frame.cameraX;
    const float cameraZ = frame.cameraZ;
    const float walkAmount = frame.walkAmount;
    const horde::gameplay::CombatSnapshot& combat = frame.combat;
    const horde::gameplay::PlayerCombatSnapshot& playerCombat = frame.playerCombat;
    const horde::gameplay::TorchFailureSnapshot& torchFailure = frame.torchFailure;
    const horde::gameplay::EnemyRosterSnapshot& roster = frame.roster;
    const horde::gameplay::LichSnapshot& lich = frame.lich;
    const WaterfallCurtainScale waterfallScale = ResolveWaterfallCurtainScale(clampedTuning);
    const auto& skeletonGpu = characterSlot_.SkeletonGpu(0u);
    const auto& secondSkeletonGpu = characterSlot_.SkeletonGpu(1u);
    const auto& lichGpu = characterSlot_.LichGpu();
    if (instanceBuffer_.memory == VK_NULL_HANDLE || heldLightBuffer_.memory == VK_NULL_HANDLE ||
        fireEmitterBuffer_.memory == VK_NULL_HANDLE || qualityControlsBuffer_.memory == VK_NULL_HANDLE ||
        waterContactRippleBuffer_.memory == VK_NULL_HANDLE ||
        (pipelineBundle_.DescriptorIo().diagnosticIo.allocateBuffer &&
         pipelineBundle_.diagnosticBuffer.memory == VK_NULL_HANDLE) ||
        skeletonGpu.vertices.memory == VK_NULL_HANDLE ||
        secondSkeletonGpu.vertices.memory == VK_NULL_HANDLE ||
        skeletonGpu.accelerationStructure.handle == VK_NULL_HANDLE ||
        secondSkeletonGpu.accelerationStructure.handle == VK_NULL_HANDLE ||
        lichGpu.accelerationStructure.handle == VK_NULL_HANDLE ||
        finaleRoofBlas_.handle == VK_NULL_HANDLE || waterfallBlas_.handle == VK_NULL_HANDLE ||
        swordBlas_.handle == VK_NULL_HANDLE ||
        playerSwordScabbardBlas_.handle == VK_NULL_HANDLE ||
        gothicChestBaseBlas_.handle == VK_NULL_HANDLE ||
        gothicChestLidBlas_.handle == VK_NULL_HANDLE ||
        rewardLanternRingBlas_.handle == VK_NULL_HANDLE ||
        rewardLanternBodyBlas_.handle == VK_NULL_HANDLE ||
        dielectricFixtureBlas_.handle == VK_NULL_HANDLE ||
        collapseBlas_.handle == VK_NULL_HANDLE ||
        playerBodyBlas_.handle == VK_NULL_HANDLE || playerLimbBlas_.handle == VK_NULL_HANDLE ||
        skinnedPlayerBlas_.handle == VK_NULL_HANDLE ||
        skinnedPlayerBlasUpdateScratch_.address == 0u ||
        tlas_.handle == VK_NULL_HANDLE || skeletonGpu.updateScratch.address == 0u ||
        secondSkeletonGpu.updateScratch.address == 0u ||
        lichGpu.updateScratch.address == 0u || tlasUpdateScratch_.address == 0u)
    {
        diagnostic = "Combat skeleton or held-prop TLAS resources are unavailable.";
        return false;
    }

    using Vec3 = std::array<float, 3>;
    const auto add = [](const Vec3& a, const Vec3& b) { return Vec3{a[0] + b[0], a[1] + b[1], a[2] + b[2]}; };
    const auto subtract = [](const Vec3& a, const Vec3& b) { return Vec3{a[0] - b[0], a[1] - b[1], a[2] - b[2]}; };
    const auto scaled = [](const Vec3& v, float scale) { return Vec3{v[0] * scale, v[1] * scale, v[2] * scale}; };
    const auto dot = [](const Vec3& a, const Vec3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; };
    const auto cross = [](const Vec3& a, const Vec3& b) {
        return Vec3{a[1] * b[2] - a[2] * b[1],
                    a[2] * b[0] - a[0] * b[2],
                    a[0] * b[1] - a[1] * b[0]};
    };
    const auto normalize = [&dot](const Vec3& v) {
        const float length = std::sqrt(std::max(dot(v, v), 0.0000001f));
        return Vec3{v[0] / length, v[1] / length, v[2] / length};
    };
    const Vec3 worldUp{0.0f, 1.0f, 0.0f};
    const Vec3 eye{cameraX, horde::gameplay::simulation::PlayerEyeWorldY(frame.playerSupportWorldY), cameraZ};
    const float bodyYaw=frame.developmentRescueJourney && frame.rescue.equipmentStowed
        ?frame.rescue.bodyYawRadians:cameraYaw;
    const Vec3 bodyForward{std::sin(bodyYaw), 0.0f, -std::cos(bodyYaw)};
    const Vec3 bodyRight{std::cos(bodyYaw), 0.0f, std::sin(bodyYaw)};
    const horde::gameplay::LowerBodyPoseState lowerBodyPose =
        horde::gameplay::EvaluateLowerBodyPose(walkTime, walkAmount);
    const float torsoCos = std::cos(lowerBodyPose.torsoTwistRadians);
    const float torsoSin = std::sin(lowerBodyPose.torsoTwistRadians);
    const Vec3 animatedBodyForward = normalize(add(scaled(bodyForward, torsoCos), scaled(bodyRight, torsoSin)));
    const Vec3 animatedBodyRight = normalize(subtract(scaled(bodyRight, torsoCos), scaled(bodyForward, torsoSin)));
    const PlayerModelWorldBasis playerModelBasis =
        BuildPlayerModelWorldBasis(animatedBodyRight, animatedBodyForward);
    const Vec3 animatedBodyOrigin = add(add(eye, scaled(bodyRight, lowerBodyPose.pelvisSway)),
                                        Vec3{0.0f, lowerBodyPose.pelvisBob * 0.65f, 0.0f});
    const float viewPitch = std::clamp(cameraPitch, -0.32f, 0.28f);
    const Vec3 viewForward = normalize(Vec3{std::sin(cameraYaw), -0.05f + viewPitch, -std::cos(cameraYaw)});
    const Vec3 viewRight = normalize(cross(viewForward, worldUp));
    const Vec3 viewUp = normalize(cross(viewRight, viewForward));
    const auto toWorld = [&eye, &viewRight, &viewUp, &viewForward, &add, &scaled](const Vec3& local) {
        return add(add(add(eye, scaled(viewRight, local[0])), scaled(viewUp, local[1])), scaled(viewForward, local[2]));
    };
    const auto solveElbow = [&subtract, &add, &scaled, &dot, &normalize](const Vec3& shoulder,
                                                                        const Vec3& hand,
                                                                        float upperLength,
                                                                        float lowerLength,
                                                                        const Vec3& poleSeed) {
        const Vec3 delta = subtract(hand, shoulder);
        const float distance = std::sqrt(std::max(dot(delta, delta), 0.0000001f));
        const Vec3 direction = scaled(delta, 1.0f / distance);
        const Vec3 pole = normalize(subtract(poleSeed, scaled(direction, dot(poleSeed, direction))));
        const float along = std::clamp((upperLength * upperLength - lowerLength * lowerLength + distance * distance) / (2.0f * distance),
                                       0.0f,
                                       upperLength);
        const float height = std::sqrt(std::max(upperLength * upperLength - along * along, 0.0f));
        return add(add(shoulder, scaled(direction, along)), scaled(pole, height));
    };
    const auto segmentTransform = [&subtract, &cross, &normalize, &dot](const Vec3& start, const Vec3& end, float radius) {
        const Vec3 delta = subtract(end, start);
        const float length = std::sqrt(std::max(dot(delta, delta), 0.0000001f));
        const Vec3 zAxis = normalize(delta);
        const Vec3 reference = std::abs(zAxis[1]) < 0.95f ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{1.0f, 0.0f, 0.0f};
        const Vec3 xAxis = normalize(cross(reference, zAxis));
        const Vec3 yAxis = cross(zAxis, xAxis);
        return VkTransformMatrixKHR{{
            xAxis[0] * radius, yAxis[0] * radius, zAxis[0] * length, start[0],
            xAxis[1] * radius, yAxis[1] * radius, zAxis[1] * length, start[1],
            xAxis[2] * radius, yAxis[2] * radius, zAxis[2] * length, start[2]}};
    };

    const Vec3 leftShoulderLocal = frame.playerAnimation.leftIk.shoulder;
    const Vec3 leftHandLocal = frame.playerAnimation.leftIk.target;
    const Vec3 rightShoulderLocal = frame.playerAnimation.rightIk.shoulder;
    const Vec3 rightHandLocal = frame.playerAnimation.rightIk.target;
    const Vec3 leftElbowLocal = solveElbow(leftShoulderLocal, leftHandLocal, 0.53f, 0.53f, Vec3{-1.0f, -0.15f, 0.08f});
    const Vec3 rightElbowLocal = solveElbow(rightShoulderLocal, rightHandLocal, 0.53f, 0.53f, Vec3{1.0f, -0.20f, 0.10f});
    const Vec3 leftShoulder = toWorld(leftShoulderLocal);
    const Vec3 leftElbow = toWorld(leftElbowLocal);
    const Vec3 leftHand = toWorld(leftHandLocal);
    const Vec3 rightShoulder = toWorld(rightShoulderLocal);
    const Vec3 rightElbow = toWorld(rightElbowLocal);
    const Vec3 rightHand = toWorld(rightHandLocal);

    bool updateSkinnedPlayer = false;
    bool updateViewmodel = false;
    const bool usesViewmodel = effectivePlayerRenderRoute == PlayerRenderRoute::ModelledViewmodel;
    if (frame.playerMountProfile == horde::gameplay::items::PlayerMountProfile::AnatomicalBody &&
        (!usesViewmodel || !playerBodyRemainderAvailable_))
    {
        diagnostic = "Anatomical player mount requires the dedicated viewmodel and explicit body remainder.";
        return false;
    }
    if (!usesViewmodel) viewmodelPoseCurrent_ = false;
    if (usesViewmodel && (!viewmodelAvailable_ || viewmodelBlas_.handle == VK_NULL_HANDLE))
    {
        diagnostic = "Dedicated modelled RT viewmodel requested without its validated runtime geometry.";
        return false;
    }
    horde::gameplay::items::HeldItemStates renderHeldItems = frame.heldItems;
    horde::gameplay::items::HeldItemTransform finalSkinnedLeftGrip =
        horde::gameplay::items::IdentityHeldItemTransform();
    bool hasFinalSkinnedLeftGrip = false;
    Vec3 skinnedPlayerRootWorld{
        animatedBodyOrigin[0], animatedBodyOrigin[1] - 1.8f,
        animatedBodyOrigin[2]};
    const bool usesSkinnedPlayer =
        effectivePlayerRenderRoute != PlayerRenderRoute::Procedural;
    auto worldFromBodyStow =
        horde::gameplay::items::IdentityHeldItemTransform();
    if (frame.playerAnimation.swordStowBlend > 0.0f && !usesSkinnedPlayer)
    {
        diagnostic = "Sword BodyStow requires the animated player Hips socket.";
        return false;
    }
    if (usesSkinnedPlayer)
    {
        const auto playerSkinBegin = std::chrono::steady_clock::now();
        std::array<float, 3u> rigShoulderCenter{};
        if (!playerRenderSlot_.ShoulderCenter(frame.playerAnimation,
                                              rigShoulderCenter, diagnostic))
            return false;
        const Vec3 intendedShoulderCenter = toWorld(
            EvaluatePlayerTorsoAnchorLocal(frame.playerAnimation));
        const Vec3 shoulderAnchoredRootWorld = subtract(
            intendedShoulderCenter,
            PlayerModelVectorToWorld(playerModelBasis, rigShoulderCenter));
        skinnedPlayerRootWorld = GroundPlayerRootOnRouteFloor(
            shoulderAnchoredRootWorld, frame.playerSupportWorldY,
            playerRenderSlot_.BootGroundingOffsetMetres(
                frame.playerAnimation));
        if (frame.playerMountProfile == horde::gameplay::items::PlayerMountProfile::AnatomicalBody)
        {
            // The authored face lies near model Z=0. Put the grounded body
            // beneath the player, not at the old view-relative .40 m shoulder
            // plane behind which the camera saw the character's back.
            skinnedPlayerRootWorld = GroundPlayerRootOnRouteFloor(
                animatedBodyOrigin, frame.playerSupportWorldY,
                playerRenderSlot_.BootGroundingOffsetMetres(frame.playerAnimation));
        }
        const auto worldPointToPlayer = [&subtract, &playerModelBasis,
                                         &skinnedPlayerRootWorld](
                                            const Vec3& worldPoint) {
            const Vec3 delta = subtract(worldPoint, skinnedPlayerRootWorld);
            return WorldVectorToPlayerModel(playerModelBasis, delta);
        };
        const auto viewVectorToPlayer = [&add, &scaled, &viewRight, &viewUp,
                                         &viewForward,
                                         &playerModelBasis](const Vec3& viewVector) {
            const Vec3 worldVector = add(add(scaled(viewRight, viewVector[0]),
                                             scaled(viewUp, viewVector[1])),
                                         scaled(viewForward, viewVector[2]));
            return WorldVectorToPlayerModel(playerModelBasis, worldVector);
        };
        horde::gameplay::items::HeldItemTransform worldFromHips{};
        if (!playerRenderSlot_.AnimatedHipsWorldTransform(
                frame.playerAnimation, playerModelBasis, skinnedPlayerRootWorld,
                worldFromHips, diagnostic))
            return false;
        worldFromBodyStow =
            horde::gameplay::items::MultiplyHeldItemTransforms(
                worldFromHips,
                horde::gameplay::items::SwordBodyStowFromHips());
        horde::gameplay::animation::PlayerAnimationSnapshot rigAnimation =
            frame.playerAnimation;
        rigAnimation.leftIk.shoulder = worldPointToPlayer(leftShoulder);
        rigAnimation.leftIk.target = worldPointToPlayer(leftHand);
        rigAnimation.leftIk.pole = viewVectorToPlayer(frame.playerAnimation.leftIk.pole);
        rigAnimation.leftIk.gripX = viewVectorToPlayer(frame.playerAnimation.leftIk.gripX);
        rigAnimation.leftIk.gripY = viewVectorToPlayer(frame.playerAnimation.leftIk.gripY);
        rigAnimation.leftIk.gripZ = viewVectorToPlayer(frame.playerAnimation.leftIk.gripZ);
        rigAnimation.rightIk.shoulder = worldPointToPlayer(rightShoulder);
        rigAnimation.rightIk.target = worldPointToPlayer(rightHand);
        rigAnimation.rightIk.pole = viewVectorToPlayer(frame.playerAnimation.rightIk.pole);
        rigAnimation.rightIk.gripX = viewVectorToPlayer(frame.playerAnimation.rightIk.gripX);
        rigAnimation.rightIk.gripY = viewVectorToPlayer(frame.playerAnimation.rightIk.gripY);
        rigAnimation.rightIk.gripZ = viewVectorToPlayer(frame.playerAnimation.rightIk.gripZ);
        if (frame.playerAnimation.swordStowBlend > 0.0f)
        {
            const auto expectedWorldFromItem =
                horde::gameplay::items::BlendHeldItemTransformsAtGrip(
                    worldFromBodyStow, frame.heldItems[1].worldFromItem,
                    horde::gameplay::items::SwordGripSocketTransform(),
                    1.0f - frame.playerAnimation.swordStowBlend);
            const auto expectedWorldFromGrip =
                horde::gameplay::items::MultiplyHeldItemTransforms(
                    expectedWorldFromItem,
                    horde::gameplay::items::SwordGripSocketTransform());
            auto itemArm = rigAnimation.rightIk;
            itemArm.target = worldPointToPlayer(
                {{expectedWorldFromGrip[12], expectedWorldFromGrip[13],
                  expectedWorldFromGrip[14]}});
            const auto worldAxisToPlayer = [&playerModelBasis](const Vec3& axis) {
                return WorldVectorToPlayerModel(playerModelBasis, axis);
            };
            itemArm.gripX = worldAxisToPlayer(
                {{expectedWorldFromGrip[0], expectedWorldFromGrip[1],
                  expectedWorldFromGrip[2]}});
            itemArm.gripY = worldAxisToPlayer(
                {{expectedWorldFromGrip[4], expectedWorldFromGrip[5],
                  expectedWorldFromGrip[6]}});
            itemArm.gripZ = worldAxisToPlayer(
                {{expectedWorldFromGrip[8], expectedWorldFromGrip[9],
                  expectedWorldFromGrip[10]}});
            horde::gameplay::items::HeldItemTransform itemGrip =
                horde::gameplay::items::IdentityHeldItemTransform();
            for (std::size_t axis = 0u; axis < 3u; ++axis)
            {
                itemGrip[axis] = itemArm.gripX[axis];
                itemGrip[4u + axis] = itemArm.gripY[axis];
                itemGrip[8u + axis] = itemArm.gripZ[axis];
                itemGrip[12u + axis] = itemArm.target[axis];
            }
            rigAnimation.rightIk = BlendPlayerArmGripTarget(
                rigAnimation.rightIk, itemGrip, frame.playerAnimation.swordHandGripBlend);
        }
        if(frame.developmentRescueJourney && frame.rescue.equipmentStowed &&
            !ApplyRescueRopeRigTargets(rigAnimation,frame.rescue,playerModelBasis,skinnedPlayerRootWorld,eye)) {
            diagnostic="Rescue rope rig mapping rejected nonfinite authority.";return false;
        }
        if (!playerRenderSlot_.PreparePose(rigAnimation, frame.tickIndex,
                                           playerCpuSkinCadence_, updateSkinnedPlayer,
                                           diagnostic, observation))
            return false;
        if (updateSkinnedPlayer)
        {
            const auto playerSkinEnd = std::chrono::steady_clock::now();
            playerSkinTotalMilliseconds_ +=
                std::chrono::duration<double, std::milli>(playerSkinEnd - playerSkinBegin).count();
            ++playerSkinUpdateCount_;
            playerMaxSocketErrorMetres_ = std::max(
                playerMaxSocketErrorMetres_,
                std::max(playerRenderSlot_.LeftSocketErrorMetres(),
                         playerRenderSlot_.RightSocketErrorMetres()));
        }
        if (updateSkinnedPlayer)
        {
            const auto& skinned = playerRenderSlot_.UniqueVertices();
            const auto& skinnedTangents = playerRenderSlot_.UniqueTangents();
            if (skinned.size() != productionPlayerAsset_.vertices.size() ||
                skinnedTangents.size() != skinned.size())
            {
                diagnostic = "Skinned player pose/tangent frame does not match its static-PBR vertex stream.";
                return false;
            }
            skinnedPlayerUpload_ = productionPlayerAsset_.vertices;
            for (std::size_t vertex = 0u; vertex < skinned.size(); ++vertex)
            {
                std::copy_n(skinned[vertex].position, 4u,
                            skinnedPlayerUpload_[vertex].position.begin());
                std::copy_n(skinned[vertex].normal, 4u,
                            skinnedPlayerUpload_[vertex].normal.begin());
                std::copy_n(skinnedTangents[vertex].tangent, 4u,
                            skinnedPlayerUpload_[vertex].tangent.begin());
            }
            const VkDeviceSize playerOffset =
                static_cast<VkDeviceSize>(playerStaticVertexBase_) *
                sizeof(horde::scene::assets::StaticRtVertex);
            if (!gpuResources_.WriteBufferRange(
                    worldPlayerVertexBuffer_, playerOffset, skinnedPlayerUpload_.data(),
                    skinnedPlayerUpload_.size() *
                        sizeof(horde::scene::assets::StaticRtVertex),
                    "skinned player static-PBR vertices", diagnostic,
                    observation))
                return false;
        }

        updateViewmodel = usesViewmodel && (updateSkinnedPlayer || !viewmodelPoseCurrent_);
        if (updateViewmodel)
        {
            const auto viewSkinBegin = std::chrono::steady_clock::now();
            RtSceneStageScope viewSkinScope(observation, horde::telemetry::RtStage::PlayerSkin);
            if (!viewmodelSkin_.SkinPlayerPoseUniqueTextured(playerRenderSlot_.SolvedPose(),
                    viewmodelPoseVertices_, viewmodelPoseTangents_, diagnostic))
            {
                viewSkinScope.Cancel();
                return false;
            }
            playerSkinTotalMilliseconds_ += std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - viewSkinBegin).count();
            if (!updateSkinnedPlayer) ++playerSkinUpdateCount_;
            if (viewmodelPoseVertices_.size() != viewmodelAsset_.vertices.size() ||
                viewmodelPoseTangents_.size() != viewmodelPoseVertices_.size())
            {
                diagnostic = "Viewmodel skin output disagrees with its independent static-PBR stream.";
                viewSkinScope.Cancel();
                return false;
            }
            if (viewmodelUpload_.empty()) viewmodelUpload_ = viewmodelAsset_.vertices;
            for (std::size_t vertex = 0u; vertex < viewmodelUpload_.size(); ++vertex)
            {
                std::copy_n(viewmodelPoseVertices_[vertex].position, 4u, viewmodelUpload_[vertex].position.begin());
                std::copy_n(viewmodelPoseVertices_[vertex].normal, 4u, viewmodelUpload_[vertex].normal.begin());
                std::copy_n(viewmodelPoseTangents_[vertex].tangent, 4u, viewmodelUpload_[vertex].tangent.begin());
            }
            viewSkinScope.Complete(1u);
            if (!gpuResources_.WriteBuffer(viewmodelVertexBuffer_, viewmodelUpload_.data(),
                    viewmodelUpload_.size() * sizeof(viewmodelUpload_.front()),
                    "modelled player viewmodel vertices", diagnostic, observation)) return false;
            viewmodelPoseCurrent_ = true;
        }

        const auto rigidWorldFromBone = [&playerModelBasis,
                                         &skinnedPlayerRootWorld, &add, &scaled,
                                         &dot, &cross, &normalize](
                                            const horde::scene::SkinnedNodeTransform& bone) {
            const auto vectorToWorld = [&playerModelBasis](const Vec3& local) {
                return PlayerModelVectorToWorld(playerModelBasis, local);
            };
            Vec3 x = normalize(vectorToWorld({bone[0], bone[1], bone[2]}));
            const Vec3 rawY = vectorToWorld({bone[4], bone[5], bone[6]});
            Vec3 y = normalize(add(rawY, scaled(x, -dot(rawY, x))));
            Vec3 z = normalize(cross(x, y));
            const Vec3 rawZ = vectorToWorld({bone[8], bone[9], bone[10]});
            if (dot(z, rawZ) < 0.0f)
            {
                y = scaled(y, -1.0f);
                z = scaled(z, -1.0f);
            }
            const Vec3 position = add(skinnedPlayerRootWorld,
                                      vectorToWorld({bone[12], bone[13], bone[14]}));
            horde::gameplay::items::HeldItemTransform result =
                horde::gameplay::items::IdentityHeldItemTransform();
            result[0] = x[0]; result[1] = x[1]; result[2] = x[2];
            result[4] = y[0]; result[5] = y[1]; result[6] = y[2];
            result[8] = z[0]; result[9] = z[1]; result[10] = z[2];
            result[12] = position[0]; result[13] = position[1]; result[14] = position[2];
            return result;
        };
        const auto& boneSockets = playerRenderSlot_.BoneSockets();
        if (!playerRenderSlot_.ResolveHeldItemVisuals(
                frame.heldItems,
                rigidWorldFromBone(boneSockets.leftGrip),
                rigidWorldFromBone(boneSockets.rightGrip),
                worldFromBodyStow, renderHeldItems, diagnostic))
            return false;
        finalSkinnedLeftGrip = playerRenderSlot_.FinalWorldFromLeftGrip();
        hasFinalSkinnedLeftGrip = true;
    }

    if (!characterSlot_.PrepareFrame(frame.skeletonEnemies,
                                     frame.skeletonEnemyCount,
                                     roster,
                                     lich,
                                     gpuResources_,
                                     diagnostic,
                                     observation))
    {
        return false;
    }
    const CharacterBlasRefit pendingCharacterRefit = characterSlot_.PendingRefit();
    const bool updateSkeletonPose0 =
        HasCharacterBlasRefit(pendingCharacterRefit, CharacterBlasRefit::SkeletonPose0);
    const bool updateSkeletonPose1 =
        HasCharacterBlasRefit(pendingCharacterRefit, CharacterBlasRefit::SkeletonPose1);
    const bool updateLich = HasCharacterBlasRefit(pendingCharacterRefit, CharacterBlasRefit::Lich);
    const auto& lichVertexBuffer_ = lichGpu.vertices;
    const auto& lichBlas_ = lichGpu.accelerationStructure;
    const auto& lichBlasUpdateScratch_ = lichGpu.updateScratch;
    const auto& lichSkinnedVertices_ = characterSlot_.LichVertices();

    const auto heldItemInstanceTransform = [](
        const horde::gameplay::items::HeldItemState& state) {
        const auto renderTransform = HeldItemRenderSlot::BuildInstanceTransform(state);
        return VkTransformMatrixKHR{{
            renderTransform[0], renderTransform[1], renderTransform[2], renderTransform[3],
            renderTransform[4], renderTransform[5], renderTransform[6], renderTransform[7],
            renderTransform[8], renderTransform[9], renderTransform[10], renderTransform[11]}};
    };
    const auto heldTransformToInstanceTransform = [](
        const horde::gameplay::items::HeldItemTransform& transform) {
        return VkTransformMatrixKHR{{
            transform[0], transform[4], transform[8], transform[12],
            transform[1], transform[5], transform[9], transform[13],
            transform[2], transform[6], transform[10], transform[14]}};
    };
    const auto uniformScale = [](float scale) {
        auto result = horde::gameplay::items::IdentityHeldItemTransform();
        result[0] = scale;
        result[5] = scale;
        result[10] = scale;
        return result;
    };
    std::array<VkAccelerationStructureInstanceKHR, PresentableTinyRtScene::kTlasInstanceCount> instances{};
    instances[0].transform = {{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f}};
    instances[0].instanceCustomIndex = 0u;
    instances[0].mask = 0x01u;
    instances[0].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    instances[0].accelerationStructureReference = blas_.address;
    instances[1] = instances[0];
    instances[1].transform = heldItemInstanceTransform(renderHeldItems[0]);
    instances[1].instanceCustomIndex = productionHeldItemAssetsEnabled_ ? kPlayerTorchInstanceIndex : 1u;
    instances[1].mask = productionVisibility.torchMask;
    instances[1].accelerationStructureReference = torchBlas_.address;
    instances[kPlayerTorchInstanceIndex] = instances[1];
    instances[kPlayerTorchInstanceIndex].mask = 0u;
    instances[kPlayerSwordScabbardInstanceIndex] = instances[1];
    instances[kPlayerSwordScabbardInstanceIndex].instanceCustomIndex =
        kPlayerSwordScabbardMetadataIndex;
    instances[kPlayerSwordScabbardInstanceIndex].mask =
        !usesSkinnedPlayer || productionVisibility.swordMask == 0u ? 0u : 0x36u;
    instances[kPlayerSwordScabbardInstanceIndex].accelerationStructureReference =
        playerSwordScabbardBlas_.address;
    instances[kPlayerSwordScabbardInstanceIndex].transform =
        heldTransformToInstanceTransform(worldFromBodyStow);
    const auto characterInstances = characterSlot_.BuildActiveInstances();
    instances[CharacterRenderSlot::kTlasInstanceIndex] = characterInstances[0];
    instances[kKeeperInstanceIndex] = characterInstances[2];
    instances[3] = instances[1];
    instances[3].instanceCustomIndex = 3u;
    instances[3].mask = productionVisibility.swordMask;
    instances[3].accelerationStructureReference = swordBlas_.address;
    instances[3].transform = heldItemInstanceTransform(renderHeldItems[1]);
    instances[kPlayerWorldBodyInstanceIndex] = instances[0];
    instances[kPlayerWorldBodyInstanceIndex].instanceCustomIndex = kPlayerWorldBodyInstanceIndex;
    const PlayerRouteMasks playerRouteMasks = BuildPlayerRouteMasks(
        effectivePlayerRenderRoute, playerBodyRemainderAvailable_);
    instances[kPlayerWorldBodyInstanceIndex].mask = productionVisibility.playerMask;
    instances[kPlayerWorldBodyInstanceIndex].accelerationStructureReference =
        usesSkinnedPlayer
        ? skinnedPlayerBlas_.address : playerBodyBlas_.address;
    instances[kPlayerWorldBodyInstanceIndex].transform = usesSkinnedPlayer
        ? VkTransformMatrixKHR{{
            playerModelBasis.modelXInWorld[0], playerModelBasis.modelYInWorld[0], playerModelBasis.modelZInWorld[0], skinnedPlayerRootWorld[0],
            playerModelBasis.modelXInWorld[1], playerModelBasis.modelYInWorld[1], playerModelBasis.modelZInWorld[1], skinnedPlayerRootWorld[1],
            playerModelBasis.modelXInWorld[2], playerModelBasis.modelYInWorld[2], playerModelBasis.modelZInWorld[2], skinnedPlayerRootWorld[2]}}
        : VkTransformMatrixKHR{{
            animatedBodyRight[0], 0.0f, animatedBodyForward[0], animatedBodyOrigin[0],
            animatedBodyRight[1], 1.0f, animatedBodyForward[1], animatedBodyOrigin[1],
            animatedBodyRight[2], 0.0f, animatedBodyForward[2], animatedBodyOrigin[2]}};
    for (std::size_t i = 5u; i <= 16u; ++i)
    {
        instances[i] = instances[kPlayerWorldBodyInstanceIndex];
        instances[i].instanceCustomIndex = static_cast<std::uint32_t>(i);
        instances[i].mask = productionVisibility.inspectionOverride
            ? 0u : playerRouteMasks.instanceMasks[i];
        instances[i].accelerationStructureReference = playerLimbBlas_.address;
    }
    instances[5].transform = segmentTransform(leftShoulder, leftElbow, 0.065f);
    instances[6].transform = segmentTransform(leftElbow, leftHand, 0.055f);
    instances[7].transform = segmentTransform(rightShoulder, rightElbow, 0.065f);
    instances[8].transform = segmentTransform(rightElbow, rightHand, 0.055f);

    // Complete the silhouette with a leather pelvis, articulated thighs/shins,
    // lifted feet and heel-to-toe roll. All reuse the shared limb BLAS on mask
    // 0x04; no new skinned enemy slot or per-frame resource is introduced.
    const auto legPoint = [&animatedBodyOrigin, &animatedBodyRight, &animatedBodyForward, &add, &scaled](float side, float y, float forward) {
        return add(add(add(animatedBodyOrigin, scaled(animatedBodyRight, side)), Vec3{0.0f, y, 0.0f}),
                   scaled(animatedBodyForward, forward));
    };
    const Vec3 leftHip = legPoint(-0.14f, -0.66f, 0.01f);
    const Vec3 rightHip = legPoint(0.14f, -0.66f, 0.01f);
    const Vec3 pelvisLeft = legPoint(-0.19f, -0.62f, 0.02f);
    const Vec3 pelvisRight = legPoint(0.19f, -0.62f, 0.02f);
    const Vec3 leftKnee = legPoint(-0.14f,
                                   -1.04f + 0.04f * lowerBodyPose.leftKneeBend,
                                   0.08f * lowerBodyPose.leftStride + 0.05f * lowerBodyPose.leftKneeBend);
    const Vec3 rightKnee = legPoint(0.14f,
                                    -1.04f + 0.04f * lowerBodyPose.rightKneeBend,
                                    0.08f * lowerBodyPose.rightStride + 0.05f * lowerBodyPose.rightKneeBend);
    const Vec3 leftAnkle = legPoint(-0.14f,
                                    -1.43f + 0.095f * lowerBodyPose.leftFootLift,
                                    0.18f * lowerBodyPose.leftStride);
    const Vec3 rightAnkle = legPoint(0.14f,
                                     -1.43f + 0.095f * lowerBodyPose.rightFootLift,
                                     0.18f * lowerBodyPose.rightStride);
    const Vec3 leftToe = add(add(leftAnkle, scaled(animatedBodyForward, 0.26f)),
                             Vec3{0.0f, 0.055f * lowerBodyPose.leftToeRoll, 0.0f});
    const Vec3 rightToe = add(add(rightAnkle, scaled(animatedBodyForward, 0.26f)),
                              Vec3{0.0f, 0.055f * lowerBodyPose.rightToeRoll, 0.0f});
    instances[9].transform = segmentTransform(pelvisLeft, pelvisRight, 0.16f);
    instances[10].transform = segmentTransform(leftHip, leftKnee, 0.105f);
    instances[11].transform = segmentTransform(leftKnee, leftAnkle, 0.09f);
    instances[12].transform = segmentTransform(rightHip, rightKnee, 0.105f);
    instances[13].transform = segmentTransform(rightKnee, rightAnkle, 0.09f);
    instances[14].transform = segmentTransform(leftAnkle, leftToe, 0.11f);
    instances[15].transform = segmentTransform(rightAnkle, rightToe, 0.11f);
    if (effectivePlayerRenderRoute == PlayerRenderRoute::HybridBlockPrimary)
    {
        // Chest/lantern instances replace legacy arm slots 5-8. Reuse four
        // otherwise hidden procedural limb slots for the owner-approved block
        // fallback without increasing TLAS or metadata capacity.
        instances[10].transform = segmentTransform(leftShoulder, leftElbow, 0.050f);
        instances[11].transform = segmentTransform(leftElbow, leftHand, 0.045f);
        instances[12].transform = segmentTransform(rightShoulder, rightElbow, 0.050f);
        instances[13].transform = segmentTransform(rightElbow, rightHand, 0.045f);
    }
    const Vec3 headBase = add(add(animatedBodyOrigin, Vec3{0.0f, -0.16f, 0.0f}), scaled(animatedBodyForward, 0.20f));
    const Vec3 headTop = add(add(animatedBodyOrigin, Vec3{0.0f, 0.15f, 0.0f}), scaled(animatedBodyForward, 0.20f));
    instances[16].transform = segmentTransform(headBase, headTop, 0.145f);
    instances[16].mask = productionVisibility.inspectionOverride
        ? 0u : playerRouteMasks.instanceMasks[16];
    auto& viewmodelInstance = instances[kPlayerViewmodelInstanceIndex];
    viewmodelInstance = instances[kPlayerWorldBodyInstanceIndex];
    viewmodelInstance.instanceCustomIndex = kPlayerViewmodelInstanceIndex;
    viewmodelInstance.mask = productionVisibility.inspectionOverride
        ? 0u : playerRouteMasks.instanceMasks[kPlayerViewmodelInstanceIndex];
    viewmodelInstance.accelerationStructureReference = viewmodelAvailable_
        ? viewmodelBlas_.address : skinnedPlayerBlas_.address;
    horde::gameplay::items::HeldItemTransform productionLanternWorldFromFlame =
        horde::gameplay::items::IdentityHeldItemTransform();
    horde::gameplay::items::HeldItemTransform productionLanternWorldFromLight =
        horde::gameplay::items::IdentityHeldItemTransform();
    bool productionLanternVisible = false;
    rewardLanternGripAgreement_ = {};
    rewardLanternAuthorityAgreement_ = {};
    rewardLanternFinalGripPosition_ = {};
    rewardLanternRingGripPosition_ = {};
    rewardLanternBodyPosition_ = {};
    if (productionRewardWorldVisible)
    {
        using horde::gameplay::interactions::ChestRewardPhase;
        using horde::gameplay::interactions::HeldLightKind;
        const bool inspectionOverride = productionInspection;
        const bool glassOnly = inspectionOverride &&
            clampedTuning.productionLanternGlassOnly;
        const bool lanternClaimed = rewardLanternClaimed;
        productionLanternVisible = inspectionOverride ||
            frame.chestReward.phase == ChestRewardPhase::LanternAvailable ||
            lanternClaimed;
        // Inspection and reveal share the exact Task 8 authored composition;
        // glass-only mode changes visibility, never the rigid prop transform.
        const float lanternScale =
            horde::gameplay::items::kClaimedRewardLanternScale;
        // Checkpoint staging is deliberately separate from the asset contract:
        // this rigid world matrix chooses the inspection location, while every
        // prop pivot/socket below is read from the loaded GLBs.
        const auto& stageWorldFromChestBase = kProductionRewardChestStageWorldFromBase;
        const auto* lanternSocket = horde::gameplay::items::FindHeldItemSocket(
            gothicChestBaseAsset_.sockets, "RewardLanternHingeSocket");
        const auto* ringHinge = horde::gameplay::items::FindHeldItemSocket(
            rewardLanternRingAsset_.sockets, "Hinge");
        const auto* ringGrip = horde::gameplay::items::FindHeldItemSocket(
            rewardLanternRingAsset_.sockets, "GripRing");
        const auto* flameSocket = horde::gameplay::items::FindHeldItemSocket(
            rewardLanternBodyAsset_.sockets, "Flame");
        const auto* lightSocket = horde::gameplay::items::FindHeldItemSocket(
            rewardLanternBodyAsset_.sockets, "Light");
        const auto chestLid = std::find_if(gothicChestLidAsset_.nodeTransforms.begin(),
            gothicChestLidAsset_.nodeTransforms.end(), [](const auto& node) {
                return node.name == "ChestLid";
            });
        const auto* chestLidHinge = horde::gameplay::items::FindHeldItemSocket(
            gothicChestBaseAsset_.sockets, "ChestLidHinge");
        if (lanternSocket == nullptr || ringHinge == nullptr || ringGrip == nullptr ||
            flameSocket == nullptr ||
            lightSocket == nullptr || chestLid == gothicChestLidAsset_.nodeTransforms.end() ||
            chestLidHinge == nullptr)
        {
            diagnostic = "Production reward prop is missing a validated authored pivot/socket.";
            return false;
        }
        const auto stageWorldFromLanternHinge =
            horde::gameplay::items::MultiplyHeldItemTransforms(
                stageWorldFromChestBase, lanternSocket->world);
        horde::gameplay::items::HeldItemTransform worldFromRing{};
        horde::gameplay::items::HeldItemTransform worldFromLanternBody{};
        if (lanternClaimed)
        {
            if (!hasFinalSkinnedLeftGrip)
            {
                diagnostic = "Claimed reward lantern requires the final skinned left Grip transform.";
                return false;
            }
            const auto carryGrip=frame.developmentRescueJourney && frame.rescue.equipmentStowed
                ?frame.rewardLanternWorldFromHinge:finalSkinnedLeftGrip;
            RewardLanternVisualTransforms rewardVisuals;
            if (!ComposeClaimedRewardLanternVisuals(
                    carryGrip,
                    ringGrip->world,
                    ringHinge->world,
                    frame.rewardLanternWorldFromHinge,
                    frame.lanternPendulum.worldFromBody,
                    lanternScale,
                    rewardVisuals,
                    diagnostic))
                return false;
            worldFromRing = rewardVisuals.worldFromRing;
            worldFromLanternBody = horde::gameplay::items::MultiplyHeldItemTransforms(
                rewardVisuals.worldFromBody, uniformScale(lanternScale));
            rewardLanternGripAgreement_ = rewardVisuals.gripAgreement;
            rewardLanternAuthorityAgreement_ = MeasureTransformAgreement(
                frame.rewardLanternWorldFromHinge, carryGrip);
            const auto finalRingGrip = horde::gameplay::items::MultiplyHeldItemTransforms(
                rewardVisuals.worldFromRing, ringGrip->world);
            rewardLanternFinalGripPosition_ = {{carryGrip[12],carryGrip[13],carryGrip[14]}};
            rewardLanternRingGripPosition_ = {{finalRingGrip[12],
                                               finalRingGrip[13],
                                               finalRingGrip[14]}};
            rewardLanternBodyPosition_ = {{worldFromLanternBody[12],
                                           worldFromLanternBody[13],
                                           worldFromLanternBody[14]}};
        }
        else
        {
            if (!horde::gameplay::items::ComposeWorldFromItem(
                    stageWorldFromLanternHinge, ringHinge->world,
                    worldFromRing, diagnostic))
                return false;
            worldFromRing = horde::gameplay::items::MultiplyHeldItemTransforms(
                worldFromRing, uniformScale(lanternScale));
            worldFromLanternBody = horde::gameplay::items::MultiplyHeldItemTransforms(
                stageWorldFromLanternHinge, uniformScale(lanternScale));
        }
        productionLanternWorldFromFlame =
            horde::gameplay::items::MultiplyHeldItemTransforms(worldFromLanternBody,
                                                                flameSocket->world);
        productionLanternWorldFromLight =
            horde::gameplay::items::MultiplyHeldItemTransforms(worldFromLanternBody,
                                                                lightSocket->world);

        instances[5] = instances[0];
        instances[5].instanceCustomIndex = 5u;
        instances[5].mask = glassOnly ? 0u : 0x01u;
        instances[5].accelerationStructureReference = gothicChestBaseBlas_.address;
        instances[5].transform = heldTransformToInstanceTransform(stageWorldFromChestBase);
        instances[6] = instances[0];
        instances[6].instanceCustomIndex = 6u;
        instances[6].mask = glassOnly ? 0u : 0x01u;
        instances[6].accelerationStructureReference = gothicChestLidBlas_.address;
        // The chest-base GLB supplies the rear hinge translation. Shared
        // gameplay supplies the deterministic 1.20 second -70 degree angle.
        const float lidProgress = inspectionOverride
            ? 1.0f
            : std::clamp(frame.chestReward.lidOpenProgress, 0.0f, 1.0f);
        const float lidRadians = 1.22173047640f * lidProgress;
        const float lidCos = std::cos(lidRadians);
        const float lidSin = std::sin(lidRadians);
        const horde::gameplay::items::HeldItemTransform rearHingeOpen{{
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, lidCos, -lidSin, 0.0f,
            0.0f, lidSin, lidCos, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f}};
        const auto worldFromChestLid = horde::gameplay::items::MultiplyHeldItemTransforms(
            horde::gameplay::items::MultiplyHeldItemTransforms(
                horde::gameplay::items::MultiplyHeldItemTransforms(
                    stageWorldFromChestBase, chestLidHinge->world), rearHingeOpen),
            chestLid->world);
        instances[6].transform = heldTransformToInstanceTransform(worldFromChestLid);
        instances[7] = instances[0];
        instances[7].instanceCustomIndex = 7u;
        instances[7].mask = productionLanternVisible ? 0x01u : 0u;
        instances[7].accelerationStructureReference = rewardLanternRingBlas_.address;
        instances[7].transform = heldTransformToInstanceTransform(worldFromRing);
        instances[8] = instances[7];
        instances[8].instanceCustomIndex = 8u;
        instances[8].accelerationStructureReference = rewardLanternBodyBlas_.address;
        instances[8].transform = heldTransformToInstanceTransform(worldFromLanternBody);
    }
    else if (glassFixtureVisible)
    {
        instances[5] = instances[0];
        instances[5].instanceCustomIndex = 9u;
        instances[5].mask = 0x01u;
        instances[5].accelerationStructureReference = dielectricFixtureBlas_.address;
        instances[5].transform = {{
            0.20f * clampedTuning.glassDepthScale, 0.0f, 0.0f, -9.10f,
            0.0f, 1.25f, 0.0f, -0.325f,
            0.0f, 0.0f, 0.75f, -15.20f}};
    }
    instances[17] = instances[0];
    instances[17].instanceCustomIndex = 17u;
    instances[17].mask = 0x20u;
    instances[17].accelerationStructureReference = finaleRoofBlas_.address;
    instances[17].transform = {{
        1.0f, 0.0f, 0.0f, -2.72f * std::clamp(lich.finaleSkylightOpenProgress, 0.0f, 1.0f),
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f}};
    instances[CharacterRenderSlot::kSecondSkeletonTlasInstanceIndex] = characterInstances[1];
    instances[19] = instances[0];
    instances[19].instanceCustomIndex = 19u;
    instances[19].mask = 0x01u;
    instances[19].accelerationStructureReference = waterfallBlas_.address;
    instances[19].transform = {{
        waterfallScale.depth, 0.0f, 0.0f, -2.32f,
        0.0f, waterfallScale.vertical, 0.0f, 0.0f,
        0.0f, 0.0f, waterfallScale.crossLane, -15.26f}};
    instances[kCollapseInstanceIndex] = instances[0];
    instances[kCollapseInstanceIndex].instanceCustomIndex = kCollapseInstanceIndex;
    instances[kCollapseInstanceIndex].mask = 0x01u;
    instances[kCollapseInstanceIndex].accelerationStructureReference = collapseBlas_.address;
    instances[kWaterDropletInstanceIndex] = instances[0];
    instances[kWaterDropletInstanceIndex].instanceCustomIndex = kWaterDropletMetadataIndex;
    // Keep the fixed TLAS definition stable. Empty slots are refit to the
    // reserved far-away staging point and cannot force an every-contact TLAS BUILD.
    instances[kWaterDropletInstanceIndex].mask = 0x01u;
    instances[kWaterDropletInstanceIndex].accelerationStructureReference = waterDropletBlas_.address;
    ApplyKeeperTorchBodyInstances(instances);
    for (std::size_t instance = 0u; instance < instances.size(); ++instance)
        lastInstanceMasks_[instance] = instances[instance].mask;
#ifndef NDEBUG
    viewmodelCaptureTransform_ = instances[kPlayerViewmodelInstanceIndex].transform;
    playerWorldBodyCaptureTransform_ =
        instances[kPlayerWorldBodyInstanceIndex].transform;
#endif
    lastPlayerPrimaryVisible_ = productionVisibility.playerPrimaryVisible;
    // Solve every visible torch component from the final hand attachment,
    // including reach clamping, retraction and committed detached transforms.
    horde::gameplay::items::HeldLightState renderedTorchLight;
    auto itemFromFlame = horde::gameplay::items::OriginalTorchFlameSocketTransform();
    auto itemFromLight = horde::gameplay::items::OriginalTorchLightSocketTransform();
    if (productionHeldItemAssetsEnabled_)
    {
        const auto* flame = horde::gameplay::items::FindHeldItemSocket(playerTorchAsset_.sockets, "Flame");
        const auto* light = horde::gameplay::items::FindHeldItemSocket(playerTorchAsset_.sockets, "Light");
        if (flame == nullptr || light == nullptr)
        {
            diagnostic = "Player Rag torch is missing its validated Flame or Light socket.";
            return false;
        }
        itemFromFlame = flame->world;
        itemFromLight = light->world;
    }
    if (!horde::gameplay::items::ComposeHeldLightState(
            renderHeldItems[0].worldFromItem,
            itemFromFlame,
            itemFromLight,
            frame.heldLight.flameStrength, renderedTorchLight, diagnostic)) return false;
    auto renderFireEmitters = frame.fireEmitters;
    if (frame.fireEmitterCount > renderFireEmitters.size())
    {
        diagnostic = "Scene configured fire emitter count exceeds its fixed storage capacity.";
        return false;
    }
    for (std::size_t emitter = 0; emitter < std::min(frame.fireEmitterCount, renderFireEmitters.size()); ++emitter)
    {
        if (renderFireEmitters[emitter].parentObject ==
            horde::gameplay::effects::FireEmitterParentObject::OriginalTorch)
        {
            renderFireEmitters[emitter].worldFromFlame = renderedTorchLight.worldFromFlame;
            renderFireEmitters[emitter].worldFromLight = renderedTorchLight.worldFromLight;
        }
    }
    RtHeldLightGpu heldLightGpu = BuildPlayerFrameLight({
        renderedTorchLight.worldFromLight[12],
        renderedTorchLight.worldFromLight[13],
        renderedTorchLight.worldFromLight[14],
        frame.heldLight.active ? frame.torchLightStrength : 0.0f}, frame.playerSupportWorldY);
    if (productionLanternVisible)
    {
        heldLightGpu.positionStrength = {{
            productionLanternWorldFromLight[12],
            productionLanternWorldFromLight[13],
            productionLanternWorldFromLight[14],
            1.8f}};
    }
    FireEmitterUpload fireEmitterUpload;
    const FireEmitterTuning fireTuning{
        clampedTuning.fireStrengthScale,
        clampedTuning.fireTurbulenceScale,
        clampedTuning.fireSmokeScale,
        frame.torchLightStrength};
    const auto quality = ResolveRtQualityControls(frame.shadowQuality, clampedTuning.workloadPreset,
        pipelineBundle_.Request().quality == DielectricQuality::High, mistEnabled_, dustQuality_);
    if (!quality) { diagnostic = "Scene shadow quality is invalid."; return false; }
    const FireEmitterQuality fireQuality = frame.fireDetail.value_or(frame.waterQuality == WaterQuality::High
        ? FireEmitterQuality::High
        : FireEmitterQuality::Mobile);
    if (!BuildFireEmitterUpload(
            std::span<const horde::gameplay::effects::FireEmitterState>(
                renderFireEmitters.data(),
                std::min(frame.fireEmitterCount, renderFireEmitters.size())),
            {{frame.cameraX, horde::gameplay::simulation::PlayerEyeWorldY(frame.playerSupportWorldY), frame.cameraZ},
             frame.zone, 24.0f},
            fireTuning,
            fireQuality,
            fireEmitterUpload,
            diagnostic))
    {
        return false;
    }
    if (productionLanternVisible)
    {
        horde::gameplay::effects::FireEmitterState lanternEmitter{};
        lanternEmitter.stableId = 0x4c414e54u;
        lanternEmitter.seed = 0x474f5448u;
        lanternEmitter.strength = 0.78f;
        lanternEmitter.colourTemperatureKelvin = 1725.0f;
        lanternEmitter.radius = 0.045f;
        lanternEmitter.height = 0.12f;
        lanternEmitter.coreRadius = 0.021f;
        lanternEmitter.smokeDensity = 0.035f;
        lanternEmitter.emberRate = 0.025f;
        lanternEmitter.parentObject =
            horde::gameplay::effects::FireEmitterParentObject::RewardLantern;
        lanternEmitter.zone = frame.zone;
        lanternEmitter.worldFromFlame = productionLanternWorldFromFlame;
        lanternEmitter.worldFromLight = productionLanternWorldFromLight;
        const FireEmitterTuning lanternFireTuning{
            1.0f * clampedTuning.fireStrengthScale,
            clampedTuning.fireTurbulenceScale,
            clampedTuning.fireSmokeScale};
        if (!AppendFireEmitterUpload(lanternEmitter, lanternFireTuning,
                                     fireQuality, fireEmitterUpload, diagnostic))
            return false;
    }
    const bool diagnosticsAvailable =
        pipelineBundle_.DiagnosticAvailability() == RtDiagnosticAvailability::Available;
    const RtDielectricDiagnostics clearedDielectricDiagnostics{};
    auto frameInstanceMetadata = staticMeshSlot_.InstanceMetadata();
    frameInstanceMetadata[kWaterDropletMetadataIndex].flags =
        static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr) |
        (waterDropletsVisible
            ? static_cast<std::uint32_t>(RtInstanceFlag::Transmissive) : 0u);
    if (effectivePlayerRenderRoute == PlayerRenderRoute::ModelledViewmodel &&
        playerBodyRemainderAvailable_)
        frameInstanceMetadata[kPlayerWorldBodyInstanceIndex].flags |=
            static_cast<std::uint32_t>(RtInstanceFlag::BodyRemainderOnlyPrimary);
    if (effectivePlayerRenderRoute == PlayerRenderRoute::Procedural)
    {
        frameInstanceMetadata[kPlayerWorldBodyInstanceIndex].flags = 0u;
        for (std::size_t i = 5u; i <= 9u; ++i)
            frameInstanceMetadata[i].flags = 0u;
    }
    else
    {
        if (!productionRewardWorldVisible)
        {
            for (std::size_t i = 5u; i <= 8u; ++i)
                frameInstanceMetadata[i].flags = 0u;
        }
        else if (!productionLanternVisible)
        {
            // The locked chest remains in the lich room, but its hidden reward
            // must not select the dielectric pipeline before the lantern is
            // actually revealed.
            frameInstanceMetadata[7u].flags = 0u;
            frameInstanceMetadata[8u].flags = 0u;
        }
        if (!glassFixtureVisible)
            frameInstanceMetadata[9u].flags = 0u;
    }
    lastPlayerWorldBodyInstanceFlags_ = frameInstanceMetadata[kPlayerWorldBodyInstanceIndex].flags;
    auto frameMaterials = sceneMaterials_;
    if (dielectricFixtureMaterialIndex_ >= frameMaterials.size())
    {
        diagnostic = "Runtime dielectric fixture material metadata is unavailable.";
        return false;
    }
    RtMaterialGpu& fixtureMaterial = frameMaterials[dielectricFixtureMaterialIndex_];
    fixtureMaterial.metallicRoughnessOcclusionTransmission[1] =
        clampedTuning.glassRoughness;
    fixtureMaterial.metallicRoughnessOcclusionTransmission[3] =
        clampedTuning.glassTransmission;
    fixtureMaterial.iorThicknessAttenuationDistance[0] = clampedTuning.glassIor;
    fixtureMaterial.iorThicknessAttenuationDistance[2] =
        clampedTuning.glassAttenuationDistance;
    fixtureMaterial.attenuationColor[0] = clampedTuning.glassAttenuationColor[0];
    fixtureMaterial.attenuationColor[1] = clampedTuning.glassAttenuationColor[1];
    fixtureMaterial.attenuationColor[2] = clampedTuning.glassAttenuationColor[2];
    genericTransmissionActive_ = HasActiveGenericTransmission(frameInstanceMetadata,
        staticMeshSlot_.PrimitiveMetadata(), frameMaterials);
    framePipelineEvidence_ = pipelineEvidenceIdentity_;
    framePipelineEvidenceValid_ = pipelineEvidenceIdentityValid_;
    framePipelineEvidence_.activeStrategy = genericTransmissionActive_
        ? horde::telemetry::RtMaterialStrategy::GenericDielectric
        : horde::telemetry::RtMaterialStrategy::OpaqueFast;
    framePipelineEvidence_.active = genericTransmissionActive_
        ? framePipelineEvidence_.genericDielectric
        : framePipelineEvidence_.opaqueFast;
    switch (frame.waterQuality)
    {
    case WaterQuality::Off:
        framePipelineEvidence_.waterQuality = horde::telemetry::RtWaterQuality::Off;
        break;
    case WaterQuality::Mobile:
        framePipelineEvidence_.waterQuality = horde::telemetry::RtWaterQuality::Mobile;
        break;
    case WaterQuality::High:
        framePipelineEvidence_.waterQuality = horde::telemetry::RtWaterQuality::High;
        break;
    default:
        framePipelineEvidenceValid_ = false;
        break;
    }
    ApplyGlassFixtureVisibility(instances);
    const auto contactRipple = MakeWaterContactRippleGpu(frame.waterContact);
    if (!WriteDustQuality(*quality, frame, diagnostic, observation) ||
        !WriteBuffer(waterContactRippleBuffer_, &contactRipple, sizeof(contactRipple),
                     "water contact ripple", diagnostic, observation) ||
        !WriteBuffer(heldLightBuffer_, &heldLightGpu, sizeof(heldLightGpu),
                     "held light", diagnostic, observation) ||
        !WriteBuffer(fireEmitterBuffer_, fireEmitterUpload.emitters.data(),
                     sizeof(fireEmitterUpload.emitters), "fire emitters", diagnostic,
                     observation) ||
        !WriteBuffer(instanceBuffer_, instances.data(), sizeof(instances),
                     "animated TLAS instance", diagnostic, observation) ||
        !WriteBuffer(instanceMetadataBuffer_, frameInstanceMetadata.data(),
                      sizeof(frameInstanceMetadata), "player route metadata", diagnostic,
                      observation) ||
        !WriteBuffer(materialMetadataBuffer_, frameMaterials.data(),
                     frameMaterials.size() * sizeof(RtMaterialGpu),
                     "RT Lab dielectric material", diagnostic, observation))
    {
        return false;
    }
    if (diagnosticsAvailable)
    {
        if (!WriteBuffer(pipelineBundle_.diagnosticBuffer,
                         &clearedDielectricDiagnostics,
                         sizeof(clearedDielectricDiagnostics),
                         "dielectric diagnostics reset", diagnostic))
        {
            if (observation != nullptr)
            {
                observation->failure = RtSceneRecordFailure::DiagnosticReset;
            }
            return false;
        }
        if (observation != nullptr)
        {
            observation->diagnosticResetCompleted = true;
        }
    }

    const bool updateRescueWorldBlas = developmentRescueJourney_ && frame.developmentRescueJourney;
    if(updateRescueWorldBlas) {
        const auto rope=horde::scene::RescueRopeTriangleVertices(frame.rescue);
        if(rescueRopeVertexOffset_+rope.size()!=rescueWorldVertices_.size()) {
            diagnostic="Rescue rope topology changed after world BLAS admission.";return false;
        }
        for(std::size_t i=0;i<rope.size();++i) {
            auto p=rope[i];if(!frame.rescue.ropeDeployed) p[1]-=8;
            rescueWorldVertices_[rescueRopeVertexOffset_+i]=p;
        }
        if(!WriteBuffer(vertexBuffer_,rescueWorldVertices_.data(),rescueWorldVertices_.size()*sizeof(rescueWorldVertices_[0]),
            "rescue world vertex update",diagnostic,observation)) return false;
        if(rescueWorldSurfaceCodes_.size()!=rescueWorldPrimitiveCount_ ||
           rescueWorldSurfaceCodes_.size()*sizeof(rescueWorldSurfaceCodes_[0])!=worldSurfaceBuffer_.size ||
           rope.size()/3>rescueWorldPrimitiveCount_) {
            diagnostic="Rescue rope surface metadata no longer matches admitted primitives.";return false;
        }
        const auto first=rescueWorldPrimitiveCount_-rope.size()/3;
        for(std::size_t i=0;i<rope.size();i+=3)
            rescueWorldSurfaceCodes_[first+i/3]=(rescueWorldSurfaceCodes_[first+i/3]&~0xff00u) |
                (horde::scene::RescueRopeTriangleNormalCode(rope[i],rope[i+1],rope[i+2])<<8u);
        // Positions and their matching facet metadata share the existing host
        // write barrier/fence before BLAS update, TLAS build and ray traversal.
        if(!WriteBuffer(worldSurfaceBuffer_,rescueWorldSurfaceCodes_.data(),worldSurfaceBuffer_.size,
            "rescue rope surface update",diagnostic,observation)) return false;
    }
    const bool updateWaterDropletBlas = genericStaticAssetEnabled_ &&
        sceneProfile_ == RtSceneProfile::Showcase &&
        (waterDropletsVisible || waterDropletGeometryVisible_);
    if (updateWaterDropletBlas)
    {
        const RtInstanceMetadata metadata =
            staticMeshSlot_.InstanceMetadata()[kWaterDropletMetadataIndex];
        if (metadata.primitiveCount != 1u || metadata.primitiveBase >=
                staticMeshSlot_.PrimitiveMetadata().size())
        {
            diagnostic = "Water contact BLAS lost its dedicated one-primitive metadata owner.";
            return false;
        }
        const auto& primitive = staticMeshSlot_.PrimitiveMetadata()[metadata.primitiveBase];
        const auto& vertexCounts = staticMeshSlot_.PrimitiveVertexCounts();
        std::array<horde::scene::assets::StaticRtVertex, kWaterDropletVertexCount> vertices{};
        if (metadata.primitiveBase >= vertexCounts.size() ||
            vertexCounts[metadata.primitiveBase] != kWaterDropletVertexCount ||
            primitive.indexCount != kWaterDropletIndexCount ||
            !UpdateWaterDropletVertices(frame.waterContact, vertices, waterDropletsVisible))
        {
            diagnostic = "Water contact snapshot or fixed droplet topology is invalid.";
            return false;
        }
        const VkDeviceSize vertexOffset = static_cast<VkDeviceSize>(primitive.vertexOffset) *
            sizeof(horde::scene::assets::StaticRtVertex);
        if (!gpuResources_.WriteBufferRange(
                staticVertexBuffer_, vertexOffset, vertices.data(), sizeof(vertices),
                "fixed-topology water contact vertices", diagnostic, observation))
            return false;
        waterDropletGeometryVisible_ = waterDropletsVisible;
    }
    VkMemoryBarrier hostWriteBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    hostWriteBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    hostWriteBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR |
                                     VK_ACCESS_SHADER_READ_BIT;
    if (pipelineBundle_.DescriptorIo().diagnosticIo.shaderWriteBarrier)
        hostWriteBarrier.dstAccessMask |= VK_ACCESS_SHADER_WRITE_BIT;
    ExecuteObservedRtSceneCommand(
        observation, RtSceneCommandEvent::HostWriteBarrier, [&]() noexcept {
            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_HOST_BIT,
                                 VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR |
                                     executionPolicy_.shaderPipelineStage,
                                 0u,
                                 1u,
                                 &hostWriteBarrier,
                                 0u,
                                 nullptr,
                                 0u,
                                 nullptr);
        });

    const std::array<bool, 7u> requestedBlasWork{{
        updateRescueWorldBlas, updateSkinnedPlayer, updateSkeletonPose0,
        updateSkeletonPose1, updateLich, updateViewmodel, updateWaterDropletBlas}};
    const std::uint64_t blasWorkInvocationCount = static_cast<std::uint64_t>(
        std::count(requestedBlasWork.begin(), requestedBlasWork.end(), true));
    RtSceneStageScope blasRefitScope(
        blasWorkInvocationCount != 0u ? observation : nullptr,
        horde::telemetry::RtStage::BlasRefitRecord);

    const auto recordPlayerBlas = [&](std::uint32_t instanceIndex,
                                      const AccelerationStructure& playerBlas,
                                      const Buffer& playerScratch)
    {
        const RtInstanceMetadata playerMetadata =
            staticMeshSlot_.InstanceMetadata()[instanceIndex];
        const auto& primitiveMetadata = staticMeshSlot_.PrimitiveMetadata();
        const auto& vertexBuffer = VertexBufferForRole(static_cast<RtGeometryRole>(playerMetadata.geometryRole));
        const auto& vertexCounts = staticMeshSlot_.PrimitiveVertexCounts();
        std::vector<VkAccelerationStructureGeometryKHR> playerGeometries;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> playerRanges;
        std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> playerRangePointers;
        playerGeometries.reserve(playerMetadata.primitiveCount);
        playerRanges.reserve(playerMetadata.primitiveCount);
        for (std::uint32_t localIndex = 0u;
             localIndex < playerMetadata.primitiveCount; ++localIndex)
        {
            const std::uint32_t geometryIndex = playerMetadata.primitiveBase + localIndex;
            const RtPrimitiveMetadata& primitive = primitiveMetadata[geometryIndex];
            VkAccelerationStructureGeometryKHR geometry{
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
            geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
            geometry.geometry.triangles.sType =
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
            geometry.geometry.triangles.vertexData.deviceAddress =
                vertexBuffer.address +
                static_cast<VkDeviceSize>(primitive.vertexOffset) *
                    sizeof(horde::scene::assets::StaticRtVertex);
            geometry.geometry.triangles.vertexStride =
                sizeof(horde::scene::assets::StaticRtVertex);
            geometry.geometry.triangles.maxVertex =
                vertexCounts[geometryIndex] - 1u;
            geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
            geometry.geometry.triangles.indexData.deviceAddress =
                staticIndexBuffer_.address +
                static_cast<VkDeviceSize>(primitive.indexOffset) * sizeof(std::uint32_t);
            geometry.geometry.triangles.transformData.deviceAddress =
                staticGeometryTransformBuffer_.address +
                static_cast<VkDeviceSize>(geometryIndex) * sizeof(VkTransformMatrixKHR);
            playerGeometries.push_back(geometry);
            VkAccelerationStructureBuildRangeInfoKHR range{};
            range.primitiveCount = primitive.indexCount / 3u;
            playerRanges.push_back(range);
        }
        for (const auto& range : playerRanges) playerRangePointers.push_back(&range);
        VkAccelerationStructureBuildGeometryInfoKHR playerUpdateInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        playerUpdateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        playerUpdateInfo.flags =
            VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
            VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        playerUpdateInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
        playerUpdateInfo.srcAccelerationStructure = playerBlas.handle;
        playerUpdateInfo.dstAccelerationStructure = playerBlas.handle;
        playerUpdateInfo.geometryCount =
            static_cast<std::uint32_t>(playerGeometries.size());
        playerUpdateInfo.pGeometries = playerGeometries.data();
        playerUpdateInfo.scratchData.deviceAddress = playerScratch.AlignedAddress();
        vkCmdBuildAccelerationStructuresKHR_(commandBuffer, 1u, &playerUpdateInfo,
                                             playerRangePointers.data());
    };

    const auto recordSkeletonBlas = [&](const std::size_t bucket) noexcept
    {
        const auto& skeletonBucketGpu = characterSlot_.SkeletonGpu(bucket);
        const auto& skeletonVertices = characterSlot_.SkeletonVertices(bucket);
        VkAccelerationStructureGeometryKHR skeletonGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        skeletonGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        skeletonGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        skeletonGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        skeletonGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        skeletonGeometry.geometry.triangles.vertexData.deviceAddress = skeletonBucketGpu.vertices.address;
        skeletonGeometry.geometry.triangles.vertexStride = sizeof(horde::scene::SkinnedRtVertex);
        skeletonGeometry.geometry.triangles.maxVertex = static_cast<std::uint32_t>(skeletonVertices.size() - 1u);
        skeletonGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
        VkAccelerationStructureBuildGeometryInfoKHR skeletonUpdateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        skeletonUpdateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        skeletonUpdateInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        skeletonUpdateInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
        skeletonUpdateInfo.srcAccelerationStructure = skeletonBucketGpu.accelerationStructure.handle;
        skeletonUpdateInfo.dstAccelerationStructure = skeletonBucketGpu.accelerationStructure.handle;
        skeletonUpdateInfo.geometryCount = 1u;
        skeletonUpdateInfo.pGeometries = &skeletonGeometry;
        skeletonUpdateInfo.scratchData.deviceAddress = skeletonBucketGpu.updateScratch.AlignedAddress();
        VkAccelerationStructureBuildRangeInfoKHR skeletonRange{};
        skeletonRange.primitiveCount = static_cast<std::uint32_t>(skeletonVertices.size() / 3u);
        const VkAccelerationStructureBuildRangeInfoKHR* skeletonRanges[] = {&skeletonRange};
        vkCmdBuildAccelerationStructuresKHR_(commandBuffer, 1u, &skeletonUpdateInfo, skeletonRanges);
    };

    const auto recordLichBlas = [&]() noexcept
    {
        VkAccelerationStructureGeometryKHR lichGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        lichGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        lichGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        lichGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        lichGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        lichGeometry.geometry.triangles.vertexData.deviceAddress = lichVertexBuffer_.address;
        lichGeometry.geometry.triangles.vertexStride = sizeof(horde::scene::TexturedSkinnedRtVertex);
        lichGeometry.geometry.triangles.maxVertex = static_cast<std::uint32_t>(lichSkinnedVertices_.size() - 1u);
        lichGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
        VkAccelerationStructureBuildGeometryInfoKHR lichUpdateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        lichUpdateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        lichUpdateInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        lichUpdateInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
        lichUpdateInfo.srcAccelerationStructure = lichBlas_.handle;
        lichUpdateInfo.dstAccelerationStructure = lichBlas_.handle;
        lichUpdateInfo.geometryCount = 1u;
        lichUpdateInfo.pGeometries = &lichGeometry;
        lichUpdateInfo.scratchData.deviceAddress = lichBlasUpdateScratch_.AlignedAddress();
        VkAccelerationStructureBuildRangeInfoKHR lichRange{};
        lichRange.primitiveCount = static_cast<std::uint32_t>(lichSkinnedVertices_.size() / 3u);
        const VkAccelerationStructureBuildRangeInfoKHR* lichRanges[] = {&lichRange};
        vkCmdBuildAccelerationStructuresKHR_(commandBuffer, 1u, &lichUpdateInfo, lichRanges);
    };

    const auto recordRescueWorldBlas = [&]() noexcept
    {
        VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        geometry.geometryType=VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.flags=VK_GEOMETRY_OPAQUE_BIT_KHR;
        auto& triangles=geometry.geometry.triangles;
        triangles.sType=VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        triangles.vertexFormat=VK_FORMAT_R32G32B32_SFLOAT;
        triangles.vertexData.deviceAddress=vertexBuffer_.address;
        triangles.vertexStride=sizeof(std::array<float,3>);
        triangles.maxVertex=rescueWorldMaxVertex_;
        triangles.indexType=VK_INDEX_TYPE_UINT32;
        triangles.indexData.deviceAddress=indexBuffer_.address;
        triangles.transformData.deviceAddress=transformBuffer_.address;
        VkAccelerationStructureBuildGeometryInfoKHR info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        info.type=VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        info.flags=VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR|
                   VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        info.mode=VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
        info.srcAccelerationStructure=blas_.handle;
        info.dstAccelerationStructure=blas_.handle;
        info.geometryCount=1;
        info.pGeometries=&geometry;
        info.scratchData.deviceAddress=rescueWorldUpdateScratch_.AlignedAddress();
        VkAccelerationStructureBuildRangeInfoKHR range{};
        range.primitiveCount=rescueWorldPrimitiveCount_;
        const VkAccelerationStructureBuildRangeInfoKHR* ranges[]={&range};
        vkCmdBuildAccelerationStructuresKHR_(commandBuffer,1,&info,ranges);
    };

    const auto recordWaterDropletBlas = [&]() noexcept
    {
        const RtInstanceMetadata metadata =
            staticMeshSlot_.InstanceMetadata()[kWaterDropletMetadataIndex];
        const auto& primitive = staticMeshSlot_.PrimitiveMetadata()[metadata.primitiveBase];
        const auto& vertexCounts = staticMeshSlot_.PrimitiveVertexCounts();
        const std::uint32_t geometryIndex = metadata.primitiveBase;
        VkAccelerationStructureGeometryKHR geometry{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.flags = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
        geometry.geometry.triangles.sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        geometry.geometry.triangles.vertexData.deviceAddress =
            staticVertexBuffer_.address +
            static_cast<VkDeviceSize>(primitive.vertexOffset) *
                sizeof(horde::scene::assets::StaticRtVertex);
        geometry.geometry.triangles.vertexStride =
            sizeof(horde::scene::assets::StaticRtVertex);
        geometry.geometry.triangles.maxVertex = vertexCounts[geometryIndex] - 1u;
        geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        geometry.geometry.triangles.indexData.deviceAddress =
            staticIndexBuffer_.address +
            static_cast<VkDeviceSize>(primitive.indexOffset) * sizeof(std::uint32_t);
        geometry.geometry.triangles.transformData.deviceAddress =
            staticGeometryTransformBuffer_.address +
            static_cast<VkDeviceSize>(geometryIndex) * sizeof(VkTransformMatrixKHR);
        VkAccelerationStructureBuildGeometryInfoKHR update{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        update.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        update.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
            VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        update.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
        update.srcAccelerationStructure = waterDropletBlas_.handle;
        update.dstAccelerationStructure = waterDropletBlas_.handle;
        update.geometryCount = 1u;
        update.pGeometries = &geometry;
        update.scratchData.deviceAddress = waterDropletBlasUpdateScratch_.AlignedAddress();
        VkAccelerationStructureBuildRangeInfoKHR range{};
        range.primitiveCount = primitive.indexCount / 3u;
        const VkAccelerationStructureBuildRangeInfoKHR* ranges[] = {&range};
        vkCmdBuildAccelerationStructuresKHR_(commandBuffer, 1u, &update, ranges);
    };

    const DynamicBlasToTlasDependency blasToTlasDependency =
        BuildDynamicBlasToTlasDependency({
            requestedBlasWork[0] || requestedBlasWork[1] || requestedBlasWork[5] ||
                requestedBlasWork[6],
            requestedBlasWork[2], requestedBlasWork[3], requestedBlasWork[4]});
    const auto recordBlasToTlasBarrier = [&]() noexcept
    {
        VkMemoryBarrier dynamicBlasBuildBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        dynamicBlasBuildBarrier.srcAccessMask = blasToTlasDependency.sourceAccessMask;
        dynamicBlasBuildBarrier.dstAccessMask = blasToTlasDependency.destinationAccessMask;
        vkCmdPipelineBarrier(commandBuffer,
                             blasToTlasDependency.sourceStageMask,
                             blasToTlasDependency.destinationStageMask,
                             0u,
                             1u,
                             &dynamicBlasBuildBarrier,
                             0u,
                             nullptr,
                             0u,
                             nullptr);
    };
    const std::uint64_t executedBlasWork = ExecuteObservedDynamicBlasCommands(
        observation, requestedBlasWork,
        [&](const std::size_t index) {
            switch (index)
            {
            case 0u: recordRescueWorldBlas(); break;
            case 1u: recordPlayerBlas(kPlayerWorldBodyInstanceIndex, skinnedPlayerBlas_, skinnedPlayerBlasUpdateScratch_); break;
            case 2u: recordSkeletonBlas(0u); break;
            case 3u: recordSkeletonBlas(1u); break;
            case 4u: recordLichBlas(); break;
            case 5u: recordPlayerBlas(kPlayerViewmodelInstanceIndex, viewmodelBlas_, viewmodelBlasUpdateScratch_); break;
            case 6u: recordWaterDropletBlas(); break;
            default: break;
            }
        },
        recordBlasToTlasBarrier);
    blasRefitScope.Complete(executedBlasWork);

    RtSceneStageScope tlasUpdateScope(
        observation, horde::telemetry::RtStage::TlasUpdateRecord);
    VkAccelerationStructureGeometryKHR tlasGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    tlasGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlasGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    tlasGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    tlasGeometry.geometry.instances.data.deviceAddress = instanceBuffer_.address;

    VkAccelerationStructureBuildGeometryInfoKHR updateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    updateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    updateInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    const bool rebuildTlas = !tlasInstanceDefinitionsValid_ ||
        RequiresTlasInstanceRebuild(tlasBuiltInstances_, instances);
    updateInfo.mode = rebuildTlas ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR
                                 : VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
    updateInfo.srcAccelerationStructure = rebuildTlas ? VK_NULL_HANDLE : tlas_.handle;
    updateInfo.dstAccelerationStructure = tlas_.handle;
    updateInfo.geometryCount = 1u;
    updateInfo.pGeometries = &tlasGeometry;
    updateInfo.scratchData.deviceAddress = tlasUpdateScratch_.AlignedAddress();

    VkAccelerationStructureBuildRangeInfoKHR updateRange{};
    updateRange.primitiveCount = static_cast<std::uint32_t>(instances.size());
    const VkAccelerationStructureBuildRangeInfoKHR* updateRanges[] = {&updateRange};
    VkMemoryBarrier traceBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    traceBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    traceBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_SHADER_READ_BIT;
    ExecuteObservedTlasUpdateCommands(
        observation,
        [&]() noexcept {
            vkCmdBuildAccelerationStructuresKHR_(
                commandBuffer, 1u, &updateInfo, updateRanges);
        },
        [&]() noexcept {
            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                                 executionPolicy_.shaderPipelineStage,
                                 0u,
                                 1u,
                                 &traceBarrier,
                                 0u,
                                 nullptr,
                                 0u,
                                 nullptr);
        });
    tlasUpdateScope.Complete(1u);
    if (rebuildTlas)
    {
        tlasPendingInstances_ = instances;
        tlasPendingDefinitionsValid_ = true;
    }

#ifndef NDEBUG
    playerWorldBodyPoseCurrent_ = usesSkinnedPlayer &&
        !productionPlayerAsset_.vertices.empty() &&
        skinnedPlayerUpload_.size() == productionPlayerAsset_.vertices.size();
#endif
    uploadedQualityControls_ = *quality;
    uploadedFireQuality_ = fireQuality;
    uploadedFireEmitters_ = fireEmitterUpload;
    uploadedFireEmittersValid_ = true;
    uploadedQualityControlsValid_ = true;
    diagnostic.clear();
    return true;
}

bool PresentableTinyRtScene::WriteDustQuality(const RtQualityControlsGpu& quality,
    const RtSceneFrameInputs& frame, std::string& diagnostic, RtSceneRecordObservation* observation)
{
    if (dustQuality_ == horde::graphics::DustQuality::Off)
        return WriteBuffer(qualityControlsBuffer_, &quality, sizeof(quality), "quality controls", diagnostic, observation);
    dustWork_ = {};
    QualityDustUpload upload{quality,{}};
    using namespace horde::scene::atmosphere;
    const float step=frame.walkTime*6.2f;
    DustCamera camera{{frame.cameraX+std::sin(step*0.5f)*0.035f*frame.walkAmount,
        horde::gameplay::simulation::PlayerEyeWorldY(frame.playerSupportWorldY)+std::abs(std::sin(step))*0.035f*frame.walkAmount,frame.cameraZ},
        frame.cameraYaw,frame.cameraPitch+std::sin(step)*0.012f*frame.walkAmount,
        float(dispatchExtent_.width)/float(std::max(dispatchExtent_.height,1u))};
    const auto rotation=static_cast<unsigned>(frame.presentationTransform)&3u;
    if(rotation==1u||rotation==3u) camera.aspect=1.0f/camera.aspect;
    // The compact preview uses its own level subvolume; entry has no dust.
    const auto zones=sceneProfile_==RtSceneProfile::Showcase ? std::span<const IndoorDustZone>(horde::scene::kShowcaseIndoorDust) :
        sceneProfile_==RtSceneProfile::GraphicsPreview ? std::span<const IndoorDustZone>(horde::scene::kPreviewIndoorDust) : std::span<const IndoorDustZone>{};
    const double seconds=std::max(double(frame.walkTime),0.0);
    const auto decision=dustCache_.Build(zones,dustQuality_,camera,seconds,upload.dust,dustWork_);
    if(decision==DustUploadDecision::Unchanged)
        return WriteBuffer(qualityControlsBuffer_, &quality, sizeof(quality), "quality controls", diagnostic, observation);
    if(decision==DustUploadDecision::Invalid) {
        diagnostic="Indoor dust zone/camera admission failed."; return false;
    }
    if(!WriteBuffer(qualityControlsBuffer_, &upload, sizeof(upload), "quality and bounded indoor dust", diagnostic, observation)) {
        dustCache_.Invalidate(); return false;
    }
    dustCache_.Commit(zones,dustQuality_,camera,seconds);
    return true;
}

void PresentableTinyRtScene::NotifyFrameSubmitted() noexcept
{
    if (!tlasPendingDefinitionsValid_) return;
    tlasBuiltInstances_ = tlasPendingInstances_;
    tlasInstanceDefinitionsValid_ = true;
    tlasPendingDefinitionsValid_ = false;
}

bool PresentableTinyRtScene::RecordTraceAndCopy(VkCommandBuffer commandBuffer,
                                                VkImage swapchainImage,
                                                VkImageLayout& swapchainImageLayout,
                                                VkExtent2D swapchainExtent,
                                                const RtSceneFrameInputs& frame,
                                                std::string& diagnostic,
                                                RtSceneRecordObservation* observation
#if HORDE_RT_STAGED_PRIMARY_TIMING
                                                , experimental::StagedPrimaryTiming* stagedTiming,
                                                std::uint32_t timingFrameSlot
#endif
                                                )
{
    // A prior unsubmitted recording must not advance the GPU definition cache.
    tlasPendingDefinitionsValid_ = false;
    uploadedFireEmittersValid_ = false;
#ifndef NDEBUG
    // A rejected presentation attempt invalidates geometry evidence from the
    // prior recorded frame, including failures before dynamic scene updates.
    playerWorldBodyPoseCurrent_ = false;
#endif
    if (observation != nullptr)
    {
        observation->failure = RtSceneRecordFailure::None;
        observation->diagnosticResetCompleted = false;
        if (observation->commands != nullptr)
        {
            *observation->commands = {};
        }
        if (observation->recordedScene != nullptr)
        {
            *observation->recordedScene = {};
        }
    }
    if (!ready_)
    {
        diagnostic = "RT scene is not ready.";
        return false;
    }
    const bool scaledPresentation = dispatchExtent_.width != swapchainExtent.width ||
                                    dispatchExtent_.height != swapchainExtent.height;
    if (scaledPresentation && !scaledBlitSupported_)
    {
        diagnostic = "This Vulkan device cannot linearly upscale the RT storage format to the presentation format.";
        return false;
    }

    if (!UpdateDynamicInstances(commandBuffer, frame, diagnostic, observation))
    {
        return false;
    }

    RtSceneStageScope traceCopyScope(
        observation, horde::telemetry::RtStage::TraceCopyRecord);
    if (storageImageLayout_ != VK_IMAGE_LAYOUT_GENERAL)
    {
        SetImageBarrier(commandBuffer,
                        storageImage_,
                        storageImageLayout_,
                        VK_IMAGE_LAYOUT_GENERAL,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        executionPolicy_.shaderPipelineStage,
                        VK_ACCESS_TRANSFER_READ_BIT,
                        VK_ACCESS_SHADER_WRITE_BIT);
        storageImageLayout_ = VK_IMAGE_LAYOUT_GENERAL;
    }

    const RtStrategyPipelineResources& activeStrategy = pipelineBundle_.Strategy(
        genericTransmissionActive_ ? RtMaterialStrategy::GenericDielectric
                                   : RtMaterialStrategy::OpaqueFast);
    vkCmdBindPipeline(commandBuffer, executionPolicy_.bindPoint,
                      activeStrategy.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, executionPolicy_.bindPoint,
                            pipelineBundle_.pipelineLayout, 0u, 1u,
                            &pipelineBundle_.descriptorSet, 0u, nullptr);
    const std::array<float, 3u> staffWorldPosition = characterSlot_.LichStaffWorldPosition(frame.lich);
    const RtGuidanceLight guidanceLight = ResolveChestGuidanceLight(frame.chestReward);
    const bool guidanceLightActive = guidanceLight.strength > 0.0f;
    const float heldPropDepth = frame.heldItemKinematics.heldPropDepth;
    const RtSceneTuning tuning = ClampRtSceneTuning(frame.tuning);
    const RtLightTuning& torchTuning = tuning.lights[static_cast<std::size_t>(RtLightGroup::Torch)];
    const RtLightTuning& skylightTuning = tuning.lights[static_cast<std::size_t>(RtLightGroup::Skylight)];
    const RtLightTuning& passageTuning = tuning.lights[static_cast<std::size_t>(RtLightGroup::Passage)];
    const RtLightTuning& staffTuning = tuning.lights[static_cast<std::size_t>(RtLightGroup::Staff)];
    const ScenePushConstants pushConstants{
        frame.cameraYaw,
        frame.cameraPitch,
        frame.torchLightStrength,
        frame.walkTime,
        frame.cameraX,
        frame.cameraZ,
        frame.walkAmount,
        static_cast<float>(horde::graphics::EncodeRtPresentationOutputMode(
            frame.presentationTransform, presentationUsesBgra_ && !scaledPresentation)),
        std::clamp(frame.outputExposure, sceneProfile_ == RtSceneProfile::EntryMenu ? 0.0f : 0.2f,
                   1.4f),
        std::clamp(frame.combat.damageFlash, 0.0f, 1.0f),
        horde::gameplay::effects::KeeperShaderPresentationKind(
            frame.roster.selectedEnemy == horde::gameplay::EnemyKind::Lich, frame.lich),
        guidanceLightActive ? 0.0f : std::clamp(frame.lich.staffLightStrength, 0.0f, 2.2f),
        guidanceLightActive ? guidanceLight.position[0] : staffWorldPosition[0],
        guidanceLightActive ? guidanceLight.position[1] : staffWorldPosition[1],
        guidanceLightActive ? guidanceLight.position[2] : staffWorldPosition[2],
        std::clamp(frame.lich.finaleSkylightOpenProgress, 0.0f, 1.0f),
        std::clamp(frame.lich.finaleDawnRevealProgress, 0.0f, 1.0f),
        heldPropDepth,
        static_cast<float>(frame.waterQuality),
        tuning.waterfallWidthScale,
        tuning.fogDensityScale,
        torchTuning.hueDegrees,
        torchTuning.intensityScale,
        skylightTuning.hueDegrees,
        skylightTuning.intensityScale,
        passageTuning.hueDegrees,
        passageTuning.intensityScale,
        staffTuning.hueDegrees,
        staffTuning.intensityScale,
        static_cast<float>(tuning.workloadPreset),
        genericTransmissionActive_ ? 1.0f : 0.0f,
        guidanceLight.strength};
    lastOutputRedBlueSwapApplied_ = horde::graphics::DecodeRtPresentationRedBlueSwap(
        static_cast<std::uint32_t>(pushConstants.outputRedBlueSwap));
    vkCmdPushConstants(commandBuffer,
                       pipelineBundle_.pipelineLayout,
                       executionPolicy_.pushConstantStages,
                       0u,
                       sizeof(pushConstants),
                       &pushConstants);
    ExecuteObservedTraceCopyCommands(
        observation,
        [&]() noexcept {
#ifdef HORDE_RT_STAGED_PRIMARY_EXPERIMENT
            if (stagedPrimary_) {
                stagedPrimary_->Record(commandBuffer,
                    genericTransmissionActive_ ? RtMaterialStrategy::GenericDielectric : RtMaterialStrategy::OpaqueFast,
                    std::as_bytes(std::span{&pushConstants, 1u})
#if HORDE_RT_STAGED_PRIMARY_TIMING
                    , stagedTiming, timingFrameSlot
#endif
                    );
                return;
            }
#endif
            if (executionPolicy_.requiresShaderBindingTable)
            {
                vkCmdTraceRaysKHR_(commandBuffer,
                                   &activeStrategy.sbtRegions[0],
                                   &activeStrategy.sbtRegions[1],
                                   &activeStrategy.sbtRegions[2],
                                   &activeStrategy.sbtRegions[3],
                                   dispatchExtent_.width,
                                   dispatchExtent_.height,
                                   1u);
            }
            else
            {
                vkCmdDispatch(commandBuffer, computeDispatchGroups_[0],
                              computeDispatchGroups_[1], computeDispatchGroups_[2]);
            }
        },
        [&]() noexcept {
            SetImageBarrier(commandBuffer,
                            storageImage_,
                            VK_IMAGE_LAYOUT_GENERAL,
                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                            executionPolicy_.shaderPipelineStage,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_ACCESS_SHADER_WRITE_BIT,
                            VK_ACCESS_TRANSFER_READ_BIT);
            storageImageLayout_ = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

            // Both platforms acquire-wait at TRANSFER. The transition must
            // chain from that stage even when discarding UNDEFINED contents;
            // discarding pixels does not discard presentation's read ownership.
            const VkPipelineStageFlags swapSrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            SetImageBarrier(commandBuffer,
                            swapchainImage,
                            swapchainImageLayout,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            swapSrcStage,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0u,
                            VK_ACCESS_TRANSFER_WRITE_BIT);

            if (scaledPresentation)
            {
                VkImageBlit blitRegion{};
                blitRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
                blitRegion.srcOffsets[1] = {
                    static_cast<std::int32_t>(dispatchExtent_.width),
                    static_cast<std::int32_t>(dispatchExtent_.height), 1};
                blitRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
                blitRegion.dstOffsets[1] = {
                    static_cast<std::int32_t>(swapchainExtent.width),
                    static_cast<std::int32_t>(swapchainExtent.height), 1};
                vkCmdBlitImage(commandBuffer,
                               storageImage_,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               swapchainImage,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1u,
                               &blitRegion,
                               VK_FILTER_LINEAR);
            }
            else
            {
                VkImageCopy copyRegion{};
                copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
                copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
                copyRegion.extent = {dispatchExtent_.width, dispatchExtent_.height, 1u};
                vkCmdCopyImage(commandBuffer,
                               storageImage_,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               swapchainImage,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1u,
                               &copyRegion);
            }
        });

    SetImageBarrier(commandBuffer,
                    swapchainImage,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    VK_ACCESS_TRANSFER_WRITE_BIT,
                    0u);
    swapchainImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    SetImageBarrier(commandBuffer,
                    storageImage_,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_GENERAL,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    executionPolicy_.shaderPipelineStage,
                    VK_ACCESS_TRANSFER_READ_BIT,
                    VK_ACCESS_SHADER_WRITE_BIT);
    storageImageLayout_ = VK_IMAGE_LAYOUT_GENERAL;

    storageImageFrameRecorded_ = true;
    traceCopyScope.Complete(1u);
    if (observation != nullptr && observation->recordedScene != nullptr)
    {
        horde::telemetry::RtRecordedSceneEvidence recorded{};
        bool recordedFactsValid = framePipelineEvidenceValid_;
        recorded.pipeline = framePipelineEvidence_;
        recorded.resources = ResourceInventory();
        const bool fireUploadBounded = uploadedFireEmittersValid_ &&
            uploadedFireEmitters_.activeCount <= kRtActiveFireEmitterCapacity;
        recordedFactsValid = fireUploadBounded && recordedFactsValid;
        if (fireUploadBounded)
        {
            horde::telemetry::RtFireLightingEvidence lighting{};
            lighting.count = uploadedFireEmitters_.activeCount;
            static_assert(kRtActiveFireEmitterCapacity ==
                horde::telemetry::kRtFireLightingEvidenceCapacity);
            // Copy the complete uploaded prefix and suffix. The canonical
            // validator verifies zero unused identity/light vectors.
            for (std::size_t index = 0u; index < lighting.emitters.size(); ++index)
            {
                const auto& packed = uploadedFireEmitters_.emitters[index];
                lighting.emitters[index].stableId = packed.identity[0];
                lighting.emitters[index].positionStrength = packed.lightPositionStrength;
                lighting.emitters[index].colourIntensity = packed.colourIntensity;
            }
            recorded.fireLighting = lighting;
            recordedFactsValid = horde::telemetry::ValidRtFireLightingEvidence(lighting) &&
                recordedFactsValid;
        }
        const auto actualMist = UploadedMistEnabled();
        recordedFactsValid = actualMist.has_value() && recordedFactsValid;
        if (actualMist.has_value())
        {
            recorded.shadowQuality = horde::telemetry::RtShadowQualityEvidence{
                static_cast<horde::telemetry::RtShadowMode>(uploadedQualityControls_.controls[0]),
                uploadedQualityControls_.controls[1], uploadedQualityControls_.controls[2], 0u};
            recorded.actualUploadedMistEnabled = *actualMist;
            recorded.actualUploadedDustQuality = UploadedDustQuality();
            recordedFactsValid = recorded.actualUploadedDustQuality.has_value() && recordedFactsValid;
            horde::telemetry::RtFireQuality fireTier = horde::telemetry::RtFireQuality::Mobile;
            switch (uploadedFireQuality_)
            {
            case FireEmitterQuality::Mobile: break;
            case FireEmitterQuality::High: fireTier = horde::telemetry::RtFireQuality::High; break;
            case FireEmitterQuality::Low: fireTier = horde::telemetry::RtFireQuality::Low; break;
            }
            const auto budget = ResolveFireEmitterQualityBudget(uploadedFireQuality_);
            recorded.fireQuality = horde::telemetry::RtFireQualityEvidence{fireTier, budget.volumeSteps, budget.reflectionSamples};
        }
        switch (playerCpuSkinCadence_)
        {
        case PlayerCpuSkinCadence::Hz30:
            recorded.player.skinCadenceHz = 30u;
            break;
        case PlayerCpuSkinCadence::Hz60:
            recorded.player.skinCadenceHz = 60u;
            break;
        case PlayerCpuSkinCadence::RequiresReviewedBackend:
            recorded.player.skinCadenceHz = 0u;
            break;
        }
        recorded.player.skinUpdateCount = playerSkinUpdateCount_;
        recordedFactsValid =
            CheckedMetresToMicrometres(
                playerMaxSocketErrorMetres_,
                recorded.player.maximumSocketErrorMicrometres) &&
            recordedFactsValid;
        // Primary pixels are populated only from this submission's completed
        // Diagnostic record at its owning fence.
        recorded.player.primaryPixelCountAvailable = false;
        recorded.player.primaryPixelCount = 0u;
        recorded.player.primaryVisible = false;
        recorded.dispatch.sceneReady = true;
        recorded.dispatch.rtDispatchRecorded = true;
        recorded.dispatch.swapchainCopyRecorded = true;
        if (recordedFactsValid)
        {
            *observation->recordedScene = recorded;
        }
        else
        {
            observation->healthy = false;
        }
    }
    diagnostic.clear();
    return true;
}

#ifndef NDEBUG
bool PresentableTinyRtScene::CaptureViewmodelMesh(const std::string& path,
                                                 std::string& diagnostic) const
{
    if (!ready_ || !viewmodelPoseCurrent_ || viewmodelUpload_.empty() ||
        viewmodelUpload_.size() != viewmodelAsset_.vertices.size())
    {
        diagnostic = "No current modelled viewmodel upload is available for geometry capture.";
        return false;
    }
    std::error_code pathError;
    const bool pathExists = std::filesystem::exists(path, pathError);
    if (pathExists || pathError)
    {
        diagnostic = "Viewmodel geometry capture path already exists or cannot be inspected.";
        return false;
    }
    std::ofstream output(path, std::ios::binary);
    output.imbue(std::locale::classic());
    output << std::setprecision(std::numeric_limits<float>::max_digits10)
           << "# Exact CPU viewmodel upload, model-space metres; not GPU readback.\n";
    output << "# model_to_world_row_major_3x4";
    for (const auto& row : viewmodelCaptureTransform_.matrix)
        for (const float value : row) output << ' ' << value;
    output << '\n';
    for (const auto& vertex : viewmodelUpload_)
        output << "v " << vertex.position[0] << ' ' << vertex.position[1] << ' ' << vertex.position[2] << '\n';
    for (const auto& vertex : viewmodelUpload_)
        output << "vt " << vertex.uv0[0] << ' ' << vertex.uv0[1] << '\n';
    for (const auto& vertex : viewmodelUpload_)
        output << "vn " << vertex.normal[0] << ' ' << vertex.normal[1] << ' ' << vertex.normal[2] << '\n';
    for (const auto& primitive : viewmodelAsset_.primitives)
    {
        output << "g " << viewmodelAsset_.materials.at(primitive.materialIndex).name << '\n';
        for (std::uint32_t offset = 0; offset < primitive.indexCount; offset += 3u)
        {
            output << 'f';
            for (std::uint32_t corner = 0; corner < 3u; ++corner)
            {
                const auto index = primitive.vertexOffset +
                    viewmodelAsset_.indices.at(primitive.indexOffset + offset + corner) + 1u;
                output << ' ' << index << '/' << index << '/' << index;
            }
            output << '\n';
        }
    }
    output.close();
    if (!output)
    {
        diagnostic = "Failed to write the viewmodel geometry capture.";
        return false;
    }
    diagnostic.clear();
    return true;
}

bool PresentableTinyRtScene::CapturePlayerWorldBodyMesh(
    const std::string& path, std::string& diagnostic) const
{
    if (!ready_ || !playerWorldBodyPoseCurrent_ ||
        productionPlayerAsset_.vertices.empty() ||
        skinnedPlayerUpload_.size() != productionPlayerAsset_.vertices.size())
    {
        diagnostic = "No current skinned player world-body upload is available for geometry capture.";
        return false;
    }
    for (const auto& row : playerWorldBodyCaptureTransform_.matrix)
        for (const float value : row)
            if (!std::isfinite(value))
            {
                diagnostic = "Current player world-body TLAS transform is not finite.";
                return false;
            }
    if (productionPlayerAsset_.primitives.empty())
    {
        diagnostic = "Player world-body asset has no primitives for geometry capture.";
        return false;
    }
    for (const auto& primitive : productionPlayerAsset_.primitives)
    {
        if (primitive.materialIndex >= productionPlayerAsset_.materials.size() ||
            primitive.indexCount == 0u || primitive.indexCount % 3u != 0u ||
            primitive.indexOffset > productionPlayerAsset_.indices.size() ||
            primitive.indexCount > productionPlayerAsset_.indices.size() -
                                       primitive.indexOffset ||
            primitive.vertexOffset > skinnedPlayerUpload_.size())
        {
            diagnostic = "Player world-body primitive ranges are invalid for geometry capture.";
            return false;
        }
        std::size_t primitiveVertexEnd = skinnedPlayerUpload_.size();
        for (const auto& candidate : productionPlayerAsset_.primitives)
            if (candidate.vertexOffset > primitive.vertexOffset)
                primitiveVertexEnd = std::min<std::size_t>(
                    primitiveVertexEnd, candidate.vertexOffset);
        if (primitiveVertexEnd <= primitive.vertexOffset)
        {
            diagnostic = "Player world-body primitive vertex range is empty or invalid.";
            return false;
        }
        for (std::uint32_t offset = 0u; offset < primitive.indexCount; ++offset)
        {
            const std::uint32_t localIndex = productionPlayerAsset_.indices[
                primitive.indexOffset + offset];
            if (localIndex >= primitiveVertexEnd - primitive.vertexOffset)
            {
                diagnostic = "Player world-body index exceeds its current CPU upload.";
                return false;
            }
        }
    }

    std::error_code pathError;
    const bool pathExists = std::filesystem::exists(path, pathError);
    if (pathExists || pathError)
    {
        diagnostic = "Player world-body geometry capture path already exists or cannot be inspected.";
        return false;
    }
    std::ofstream output(path, std::ios::binary);
    output.imbue(std::locale::classic());
    output << std::setprecision(std::numeric_limits<float>::max_digits10)
           << "# Exact CPU PlayerWorldBody upload, model-space metres; not GPU readback.\n";
    output << "# model_to_world_row_major_3x4";
    for (const auto& row : playerWorldBodyCaptureTransform_.matrix)
        for (const float value : row) output << ' ' << value;
    output << '\n';
    for (const auto& vertex : skinnedPlayerUpload_)
        output << "v " << vertex.position[0] << ' ' << vertex.position[1] << ' '
               << vertex.position[2] << '\n';
    for (const auto& vertex : skinnedPlayerUpload_)
        output << "vt " << vertex.uv0[0] << ' ' << vertex.uv0[1] << '\n';
    for (const auto& vertex : skinnedPlayerUpload_)
        output << "vn " << vertex.normal[0] << ' ' << vertex.normal[1] << ' '
               << vertex.normal[2] << '\n';
    for (const auto& primitive : productionPlayerAsset_.primitives)
    {
        output << "g "
               << productionPlayerAsset_.materials[primitive.materialIndex].name
               << '\n';
        for (std::uint32_t offset = 0u; offset < primitive.indexCount; offset += 3u)
        {
            output << 'f';
            for (std::uint32_t corner = 0u; corner < 3u; ++corner)
            {
                const auto index = primitive.vertexOffset +
                    productionPlayerAsset_.indices[
                        primitive.indexOffset + offset + corner] + 1u;
                output << ' ' << index << '/' << index << '/' << index;
            }
            output << '\n';
        }
    }
    output.close();
    if (!output)
    {
        diagnostic = "Failed to write the player world-body geometry capture.";
        return false;
    }
    diagnostic.clear();
    return true;
}
#endif

bool PresentableTinyRtScene::CaptureStorageImage(StorageImageCapture& capture, std::string& diagnostic)
{
    capture = {};
    if (!ready_ || storageImage_ == VK_NULL_HANDLE ||
        storageImageLayout_ != VK_IMAGE_LAYOUT_GENERAL || !storageImageFrameRecorded_)
    {
        diagnostic = "RT storage image is not ready for capture.";
        return false;
    }

    const VkDeviceSize byteSize = static_cast<VkDeviceSize>(dispatchExtent_.width) *
                                  static_cast<VkDeviceSize>(dispatchExtent_.height) * 4u;
    // Allocate CPU storage BEFORE acquiring the readback buffer or mapping its
    // memory. Allocation failure must not strand a mapped Vulkan allocation.
    // The reporting caller additionally bounds source pixels before this call.
    std::vector<std::uint8_t> pixels;
    try { pixels.resize(static_cast<std::size_t>(byteSize)); }
    catch (...)
    {
        diagnostic = "Failed to allocate RT capture CPU storage.";
        return false;
    }
    Buffer readback;
    if (!CreateBuffer(byteSize,
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      false,
                      readback,
                      diagnostic))
    {
        diagnostic = "Failed to create RT capture readback buffer: " + diagnostic;
        return false;
    }

    struct CaptureCommands
    {
        VkImage image;
        VkBuffer buffer;
        VkExtent2D extent;
        VkPipelineStageFlags shaderStage;
    } commands{storageImage_, readback.buffer, dispatchExtent_, executionPolicy_.shaderPipelineStage};
    const auto record = [](VkCommandBuffer commandBuffer, void* userData) {
        const auto* captureCommands = static_cast<const CaptureCommands*>(userData);
        SetImageBarrier(commandBuffer,
                        captureCommands->image,
                        VK_IMAGE_LAYOUT_GENERAL,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        captureCommands->shaderStage,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_ACCESS_SHADER_WRITE_BIT,
                        VK_ACCESS_TRANSFER_READ_BIT);

        VkBufferImageCopy copy{};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
        copy.imageExtent = {captureCommands->extent.width, captureCommands->extent.height, 1u};
        vkCmdCopyImageToBuffer(commandBuffer,
                               captureCommands->image,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               captureCommands->buffer,
                               1u,
                               &copy);

        SetImageBarrier(commandBuffer,
                        captureCommands->image,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_IMAGE_LAYOUT_GENERAL,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        captureCommands->shaderStage,
                        VK_ACCESS_TRANSFER_READ_BIT,
                        VK_ACCESS_SHADER_WRITE_BIT);
    };
    if (!RunOneTimeCommands(record, &commands, diagnostic))
    {
        DestroyBuffer(readback);
        diagnostic = "Failed to read back RT storage image: " + diagnostic;
        return false;
    }

    void* mapped = nullptr;
    if (vkMapMemory(device_, readback.memory, 0u, byteSize, 0u, &mapped) != VK_SUCCESS || mapped == nullptr)
    {
        DestroyBuffer(readback);
        diagnostic = "Failed to map RT capture readback memory.";
        return false;
    }

    std::memcpy(pixels.data(), mapped, pixels.size());
    vkUnmapMemory(device_, readback.memory);
    DestroyBuffer(readback);
    capture.width = dispatchExtent_.width;
    capture.height = dispatchExtent_.height;
    capture.redBlueSwapNormalised = lastOutputRedBlueSwapApplied_;
    capture.rgba = std::move(pixels);

    if (capture.redBlueSwapNormalised)
    {
        for (std::size_t offset = 0; offset < capture.rgba.size(); offset += 4u)
        {
            std::swap(capture.rgba[offset], capture.rgba[offset + 2u]);
        }
    }

    diagnostic.clear();
    return true;
}

} // namespace horde::vulkan::raytracing
