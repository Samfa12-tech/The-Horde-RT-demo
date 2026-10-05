#include "scene/assets/StaticMeshAsset.h"
#include "graphics/BoundedDynamicResolution.h"
#include "graphics/TemporalGeometryHistory.h"
#include "vulkan/raytracing/RtSceneAbi.generated.h"
#include "vulkan/raytracing/RtStaticMeshSlot.h"
#include "vulkan/raytracing/RtTextureArrays.h"
#include "vulkan/raytracing/RtSceneTuning.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace
{

int failures = 0;

void Check(bool condition, std::string_view message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

template <typename T>
void CheckSplitDielectricDiagnosticLayout()
{
    if constexpr (requires(T value) {
                      value.primaryUnclosedVolumeCount;
                      value.shadowUnclosedVolumeCount;
                      value.productionPaneStackFailureCount;
                      value.productionPaneSecondaryOriginCount;
                      value.productionPaneSecondaryTerminalCount;
                      value.productionPaneSecondarySameMediumCount;
                      value.productionPaneSecondaryDifferentMediumCount;
                      value.secondaryNearSelfHitCount;
                      value.primaryOpenMissCount;
                      value.primaryOpenOpaqueCount;
                      value.primaryMismatchedExitCount;
                      value.primaryInterfaceBudgetCount;
                      value.primaryVolumeBudgetCount;
                      value.shadowOpenMissCount;
                      value.shadowMismatchedExitCount;
                      value.primaryTirCount;
                      value.primaryInterfaceBudgetOpenVolumeCount;
                      value.primaryInterfaceBudgetClosedVolumeCount;
                      value.shadowMismatchEmptyCount;
                      value.shadowImplicitOriginExitCount;
                      value.secondaryDielectricTerminalCount;
                      value.primaryTirTerminationCount;
                      value.shadowFiniteEndpointVolumeCount;
                      value.primaryOpenOpaqueSameInstanceDifferentMaterialCount;
                      value.primaryOpenOpaqueAfterTirCount;
                      value.primaryOpenOpaqueTerminalInstanceMask;
                      value.primaryOpenOpaqueVolumeInstanceMask;
                      value.primaryOpenOpaqueTerminalMaterialMask;
                      value.primaryClosedVolumeAbsorptionCount;
                      value.primaryCertifiedClosedVolumeRecoveryCount;
                      value.shadowCertifiedClosedVolumeRecoveryCount;
                      value.certifiedClosedVolumeRecoveryReasonMask;
                      value.primaryTorchPixelCount;
                      value.primarySwordPixelCount;
                      value.primaryPlayerPixelCount;
                      value.primaryRewardRingPixelCount;
                      value.primaryRewardBodyPixelCount;
                  })
    {
        Check(sizeof(T) == 176u && alignof(T) == 16u &&
                  offsetof(T, transportOverflowCount) == 0u &&
                  offsetof(T, shadowOverflowCount) == 4u &&
                  offsetof(T, secondaryDielectricRejectCount) == 8u &&
                  offsetof(T, unclosedVolumeCount) == 12u &&
                  offsetof(T, primaryUnclosedVolumeCount) == 16u &&
                  offsetof(T, shadowUnclosedVolumeCount) == 20u &&
                  offsetof(T, productionPaneStackFailureCount) == 24u &&
                  offsetof(T, productionPaneSecondaryOriginCount) == 28u &&
                  offsetof(T, productionPaneSecondaryTerminalCount) == 32u &&
                  offsetof(T, productionPaneSecondarySameMediumCount) == 36u &&
                  offsetof(T, productionPaneSecondaryDifferentMediumCount) == 40u &&
                  offsetof(T, secondaryNearSelfHitCount) == 44u &&
                  offsetof(T, primaryOpenMissCount) == 48u &&
                  offsetof(T, primaryOpenOpaqueCount) == 52u &&
                  offsetof(T, primaryMismatchedExitCount) == 56u &&
                  offsetof(T, primaryInterfaceBudgetCount) == 60u &&
                  offsetof(T, primaryVolumeBudgetCount) == 64u &&
                  offsetof(T, shadowOpenMissCount) == 68u &&
                  offsetof(T, shadowMismatchedExitCount) == 72u &&
                  offsetof(T, primaryTirCount) == 76u &&
                  offsetof(T, primaryInterfaceBudgetOpenVolumeCount) == 80u &&
                  offsetof(T, primaryInterfaceBudgetClosedVolumeCount) == 84u &&
                  offsetof(T, shadowMismatchEmptyCount) == 88u &&
                  offsetof(T, shadowImplicitOriginExitCount) == 92u &&
                  offsetof(T, secondaryDielectricTerminalCount) == 96u &&
                  offsetof(T, primaryTirTerminationCount) == 100u &&
                  offsetof(T, shadowFiniteEndpointVolumeCount) == 104u &&
                  offsetof(T, primaryOpenOpaqueSameInstanceDifferentMaterialCount) == 108u &&
                  offsetof(T, primaryOpenOpaqueAfterTirCount) == 112u &&
                  offsetof(T, primaryOpenOpaqueTerminalInstanceMask) == 116u &&
                  offsetof(T, primaryOpenOpaqueVolumeInstanceMask) == 120u &&
                  offsetof(T, primaryOpenOpaqueTerminalMaterialMask) == 124u &&
                  offsetof(T, primaryClosedVolumeAbsorptionCount) == 128u &&
                  offsetof(T, primaryCertifiedClosedVolumeRecoveryCount) == 132u &&
                  offsetof(T, shadowCertifiedClosedVolumeRecoveryCount) == 136u &&
                  offsetof(T, certifiedClosedVolumeRecoveryReasonMask) == 140u &&
                  offsetof(T, primaryTorchPixelCount) == 144u &&
                  offsetof(T, primarySwordPixelCount) == 148u &&
                  offsetof(T, primaryPlayerPixelCount) == 152u &&
                  offsetof(T, primaryRewardRingPixelCount) == 156u &&
                  offsetof(T, primaryRewardBodyPixelCount) == 160u,
              "diagnostics attribute transport reasons and primary-visible prop/player pixels without reordering stable fields");
    }
    else
    {
        Check(false,
              "dielectric diagnostics must append independent primary and shadow unclosed-volume counts");
    }
}

horde::scene::assets::StaticMeshAsset MakeAsset(std::size_t primitiveCount,
                                                 std::size_t materialCount)
{
    horde::scene::assets::StaticMeshAsset asset;
    horde::scene::assets::StaticNodeTransform root;
    root.name = "Root";
    root.world = {{2.0f, 0.0f, 0.0f, 0.0f,
                   0.0f, 3.0f, 0.0f, 0.0f,
                   0.0f, 0.0f, 4.0f, 0.0f,
                   5.0f, 6.0f, 7.0f, 1.0f}};
    asset.nodeTransforms.push_back(root);
    asset.vertices.resize(primitiveCount * 3u);
    asset.indices.resize(primitiveCount * 3u);
    asset.materials.resize(materialCount);
    for (std::size_t primitiveIndex = 0u; primitiveIndex < primitiveCount; ++primitiveIndex)
    {
        asset.primitives.push_back({
            static_cast<std::uint32_t>(primitiveIndex * 3u),
            static_cast<std::uint32_t>(primitiveIndex * 3u),
            3u,
            static_cast<std::uint32_t>(primitiveIndex % materialCount),
            0u});
        asset.indices[primitiveIndex * 3u] = 0u;
        asset.indices[primitiveIndex * 3u + 1u] = 1u;
        asset.indices[primitiveIndex * 3u + 2u] = 2u;
    }
    return asset;
}

void TestAbiLayout()
{
    using namespace horde::vulkan::raytracing;
    Check((std::is_same_v<horde::scene::assets::StaticRtVertex, StaticRtVertex>),
          "StaticMeshAsset uses the generated StaticRtVertex ABI type");
    Check(sizeof(StaticRtVertex) == 64u &&
              offsetof(StaticRtVertex, position) == 0u &&
              offsetof(StaticRtVertex, normal) == 16u &&
              offsetof(StaticRtVertex, tangent) == 32u &&
              offsetof(StaticRtVertex, uv0) == 48u,
          "generated StaticRtVertex matches four hand-checked vec4 fields");
    Check(sizeof(RtInstanceMetadata) == 32u, "RtInstanceMetadata size is 32 bytes");
    Check(offsetof(RtInstanceMetadata, primitiveBase) == 0u &&
              offsetof(RtInstanceMetadata, primitiveCount) == 4u &&
              offsetof(RtInstanceMetadata, stableObjectId) == 8u &&
              offsetof(RtInstanceMetadata, flags) == 12u &&
              offsetof(RtInstanceMetadata, emitterIndex) == 16u &&
              offsetof(RtInstanceMetadata, assetIndex) == 20u &&
              offsetof(RtInstanceMetadata, geometryRole) == 24u &&
              offsetof(RtInstanceMetadata, reserved1) == 28u,
          "RtInstanceMetadata CPU offsets match two GLSL uvec4 fields");

    Check(sizeof(RtPrimitiveMetadata) == 16u, "RtPrimitiveMetadata size is 16 bytes");
    Check(offsetof(RtPrimitiveMetadata, vertexOffset) == 0u &&
              offsetof(RtPrimitiveMetadata, indexOffset) == 4u &&
              offsetof(RtPrimitiveMetadata, indexCount) == 8u &&
              offsetof(RtPrimitiveMetadata, materialIndex) == 12u,
          "RtPrimitiveMetadata CPU offsets match one GLSL uvec4 field");
    CheckSplitDielectricDiagnosticLayout<RtDielectricDiagnostics>();

    Check(sizeof(RtMaterialGpu) == 128u && alignof(RtMaterialGpu) == 16u,
          "RtMaterialGpu is eight aligned vec4/uvec4 fields");
    Check(offsetof(RtMaterialGpu, baseColorFactor) == 0u &&
              offsetof(RtMaterialGpu, emissiveFactorStrength) == 16u &&
              offsetof(RtMaterialGpu, metallicRoughnessOcclusionTransmission) == 32u &&
              offsetof(RtMaterialGpu, iorThicknessAttenuationDistance) == 48u &&
              offsetof(RtMaterialGpu, attenuationColor) == 64u &&
              offsetof(RtMaterialGpu, textureLayers) == 80u &&
              offsetof(RtMaterialGpu, materialFlags) == 96u &&
              offsetof(RtMaterialGpu, normalScaleUvScaleBlend) == 112u,
          "RtMaterialGpu CPU offsets match generated GLSL declaration");
    Check(sizeof(RtQualityControlsGpu) == 16u && alignof(RtQualityControlsGpu) == 16u &&
              offsetof(RtQualityControlsGpu, controls) == 0u,
          "typed quality record is exactly one aligned uint4 without push expansion");
    Check(sizeof(RtHeldLightGpu) == 16u && alignof(RtHeldLightGpu) == 16u &&
              offsetof(RtHeldLightGpu, positionStrength) == 0u,
          "RtHeldLightGpu appends one exact world-position/strength vec4");
}

void TestGeneratedConstants()
{
    using namespace horde::vulkan::raytracing;
    Check(static_cast<std::uint32_t>(RtMaterialFlag::CertifiedClosedVolume) == 1024u,
          "topology-certified closed volume is an append-only material ABI flag");
    Check(static_cast<std::uint32_t>(RtMaterialFlag::CertifiedRectangularVolume) == 4096u,
          "rectangular geometry certification has a distinct append-only material flag");
    Check(kRtInstanceMetadataCapacity == 22u, "instance metadata preserves viewmodel20 and appends immutable collapse21");
    Check(kRtTlasInstanceCapacity == 24u && kRtTlasInstanceCapacity == kRtInstanceMetadataCapacity + 2u,
          "two world torch physical instances alias admitted metadata instead of expanding material slots");
    Check(kRtActiveFireEmitterCapacity == 4u && kRtFireEmitterCapacity == 4u && sizeof(RtFireEmitterGpu) == 160u,
          "all four important lights are active within the unchanged 640-byte fire storage buffer");
    Check(kRtStaticAssetCapacity == 10u, "static asset capacity admits exactly one additional collapse asset");
    Check(kRtPrimitiveMetadataCapacity == 32u, "primitive capacity is 32");
    Check(kRtMaterialCapacity == 32u, "material capacity is 32");
    Check(kRtTextureLayerCapacity == 16u, "each PBR texture category has 16 layers");
    Check(kRtBindingInstanceMetadata == 11u && kRtBindingPrimitiveMetadata == 12u &&
              kRtBindingMaterials == 13u && kRtBindingStaticVertices == 14u &&
              kRtBindingStaticIndices == 15u && kRtBindingBaseColorTextures == 16u &&
              kRtBindingNormalTextures == 17u && kRtBindingOrmTextures == 18u &&
              kRtBindingEmissiveTextures == 19u && kRtBindingHeldLight == 20u &&
              kRtBindingFireEmitters == 21u && kRtBindingDielectricDiagnostics == 22u &&
              kRtBindingWorldPlayerVertices == 23u && kRtBindingViewmodelVertices == 24u &&
              kRtBindingEnvironmentTexture == 25u && kRtBindingQualityControls == 26u,
          "descriptor bindings preserve player streams23/24 and append environment25 and quality26");
    Check(static_cast<std::uint32_t>(RtGeometryRole::Static) == 0u &&
              static_cast<std::uint32_t>(RtGeometryRole::PlayerWorldBody) == 1u &&
              static_cast<std::uint32_t>(RtGeometryRole::PlayerViewmodel) == 2u &&
              kPlayerWorldBodyInstanceIndex == 4u &&
              kPlayerViewmodelInstanceIndex == 20u &&
              kPlayerViewmodelPrimaryMask == 64u &&
              kPlayerBodyRemainderPrimaryMask == 128u,
          "generated geometry roles and named player instance indices remain explicit");
    Check(static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr) == 1u &&
              static_cast<std::uint32_t>(RtInstanceFlag::Emissive) == 2u &&
              static_cast<std::uint32_t>(RtInstanceFlag::Transmissive) == 4u &&
              static_cast<std::uint32_t>(RtInstanceFlag::BodyRemainderOnlyPrimary) == 8u,
          "instance flag enum agrees with hand-checked literals");
    Check(static_cast<std::uint32_t>(RtMaterialFlag::DoubleSided) == 1u &&
              static_cast<std::uint32_t>(RtMaterialFlag::Alpha) == 2u &&
              static_cast<std::uint32_t>(RtMaterialFlag::Transmission) == 4u &&
              static_cast<std::uint32_t>(RtMaterialFlag::BaseColorTexture) == 8u &&
              static_cast<std::uint32_t>(RtMaterialFlag::NormalTexture) == 16u &&
              static_cast<std::uint32_t>(RtMaterialFlag::OrmTexture) == 32u &&
              static_cast<std::uint32_t>(RtMaterialFlag::EmissiveTexture) == 64u &&
              static_cast<std::uint32_t>(RtMaterialFlag::ThinWall) == 512u &&
              static_cast<std::uint32_t>(RtMaterialFlag::BodyRemainderPrimaryVisible) == 2048u,
          "material flag enum agrees with hand-checked literals");
}

void TestGenericRegistrationAndMeasurements()
{
    using namespace horde::vulkan::raytracing;
    auto asset = MakeAsset(2u, 2u);
    asset.materials[0].baseColorTexture = 2;
    asset.materials[0].normalTexture = 5;
    asset.materials[0].ormTexture = 7;
    asset.materials[0].emissiveTexture = 11;
    asset.materials[1].baseColorTexture = 2;
    asset.materials[1].normalTexture = 9;
    asset.materials[0].normalScale = -0.5f;
    asset.materials[0].textureScale = {{2.0f, 0.5f}};
    const StaticRtAssetRegistration request{
        3u, 0x10203040u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr), 7u, &asset};
    RtStaticMeshSlot slot;
    std::string diagnostic;
    Check(slot.Initialize(std::span<const StaticRtAssetRegistration>(&request, 1u), diagnostic),
          std::string("generic registration initializes: ") + diagnostic);
    Check(slot.InstanceMetadata()[3].primitiveBase == 0u &&
              slot.InstanceMetadata()[3].primitiveCount == 2u &&
              slot.InstanceMetadata()[3].stableObjectId == 0x10203040u &&
              slot.InstanceMetadata()[3].flags == 1u &&
              slot.InstanceMetadata()[3].emitterIndex == 7u &&
              slot.InstanceMetadata()[3].assetIndex == 0u,
          "custom index 3 routes through generic metadata with no object-name branch");
    Check(slot.PrimitiveMetadata().size() == 2u &&
              slot.PrimitiveMetadata()[1].vertexOffset == 3u &&
              slot.PrimitiveMetadata()[1].indexOffset == 3u &&
              slot.PrimitiveMetadata()[1].indexCount == 3u &&
              slot.PrimitiveMetadata()[1].materialIndex == 1u,
          "geometryIndex can select a primitive and its material record");
    Check(slot.GeometryTransforms().size() == 2u &&
              slot.GeometryTransforms()[0] == std::array<float, 12u>{{
                  1.0f, 0.0f, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f, 0.0f,
                  0.0f, 0.0f, 1.0f, 0.0f}},
          "baked geometry uses identity BLAS transforms so node transforms are not applied twice");
    Check(slot.Materials()[0].textureLayers == std::array<std::uint32_t, 4u>{{0u, 0u, 0u, 0u}} &&
              slot.Materials()[1].textureLayers == std::array<std::uint32_t, 4u>{{0u, 1u, 0u, 0u}} &&
              slot.Materials()[0].materialFlags[0] == (8u | 16u | 32u | 64u) &&
              slot.Materials()[1].materialFlags[0] == (8u | 16u),
          "texture layers are dense per PBR category and presence is explicit");
    Check(slot.TextureArrayCounts().baseColor == 1u &&
              slot.TextureArrayCounts().normal == 2u &&
              slot.TextureArrayCounts().orm == 1u &&
              slot.TextureArrayCounts().emissive == 1u,
          "Vulkan array layer counts exactly cover every metadata layer");
    Check(slot.Materials()[0].normalScaleUvScaleBlend ==
              std::array<float, 4u>{{-0.5f, 2.0f, 0.5f, 1.0f}} &&
          slot.Materials()[1].normalScaleUvScaleBlend ==
              std::array<float, 4u>{{1.0f, 1.0f, 1.0f, 1.0f}},
          "signed normal strength and UV scales remain material-local with safe defaults");
    Check(slot.Measurements().vertexBytes == 6u * 64u &&
              slot.Measurements().indexBytes == 6u * 4u &&
              slot.Measurements().materialBytes == 2u * 128u &&
               slot.Measurements().instanceMetadataBytes == 22u * 32u &&
              slot.Measurements().primitiveMetadataBytes == 2u * 16u &&
              slot.Measurements().descriptorCount == 9u,
          "resource measurements use literal ABI sizes and descriptor count");
}

void TestExplicitTextureGroups()
{
    using namespace horde::vulkan::raytracing;
    auto asset = MakeAsset(4u, 4u);
    // Groups are generic asset metadata, not string branches or material indices.
    for (std::size_t i = 0; i < asset.materials.size(); ++i)
    {
        auto& material = asset.materials[i];
        material.textureGroup = i == 0 ? 1 : 0;
        material.baseColorTexture = static_cast<std::int32_t>(i);
        material.normalTexture = static_cast<std::int32_t>(i + 4);
    }
    const StaticRtAssetRegistration request{3u, 1u, 1u, 0u, &asset};
    RtStaticMeshSlot slot;
    std::string diagnostic;
    Check(slot.Initialize(std::span(&request, 1), diagnostic), "explicit texture groups initialize");
    Check(slot.Materials()[0].textureLayers == std::array<std::uint32_t, 4u>{{1u, 1u, 0u, 0u}} &&
              slot.Materials()[3].textureLayers == std::array<std::uint32_t, 4u>{{0u, 0u, 0u, 0u}},
          "canonical group order overrides first encountered material");
    asset.materials[2].normalTexture = -1;
    Check(!slot.Initialize(std::span(&request, 1), diagnostic) &&
              diagnostic == "RtStaticMeshSlot texture group has conflicting texture presence.",
          "grouped materials cannot silently disagree on texture presence");
    asset.materials[2].normalTexture = 6;
    auto preceding = MakeAsset(1u, 15u);
    for (std::size_t i = 0; i < preceding.materials.size(); ++i)
        preceding.materials[i].baseColorTexture = static_cast<std::int32_t>(i);
    const std::array<StaticRtAssetRegistration, 2> registrations{{{1u, 2u, 1u, 0u, &preceding}, request}};
    Check(!slot.Initialize(registrations, diagnostic) &&
              diagnostic == "RtStaticMeshSlot capacity overflow: baseColor texture layers exceed 16.",
          "canonical groups still enforce bounded texture capacity");
}

void TestNamedCapacityFailures()
{
    using namespace horde::vulkan::raytracing;
    RtStaticMeshSlot slot;
    std::string diagnostic;
    auto one = MakeAsset(1u, 1u);
    StaticRtAssetRegistration badIndex{
        kRtInstanceMetadataCapacity, 1u,
        static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr), 0u, &one};
    Check(!slot.Initialize(std::span<const StaticRtAssetRegistration>(&badIndex, 1u), diagnostic) &&
              diagnostic == "RtStaticMeshSlot capacity overflow: instanceCustomIndex exceeds RtInstanceMetadata[" +
                  std::to_string(kRtInstanceMetadataCapacity) + "].",
              "instance metadata overflow fails initialization by name");

    std::vector<horde::scene::assets::StaticMeshAsset> assets;
    std::vector<StaticRtAssetRegistration> registrations;
    assets.reserve(kRtStaticAssetCapacity + 1u);
    registrations.reserve(kRtStaticAssetCapacity + 1u);
    for (std::uint32_t i = 0u; i <= kRtStaticAssetCapacity; ++i)
    {
        assets.push_back(MakeAsset(1u, 1u));
        registrations.push_back({i, i + 1u, 1u, 0u, &assets.back()});
    }
    // Vector growth moves assets, so rebind pointers after construction.
    for (std::size_t i = 0u; i < registrations.size(); ++i) registrations[i].asset = &assets[i];
    Check(!slot.Initialize(registrations, diagnostic) &&
              diagnostic == "RtStaticMeshSlot capacity overflow: static assets exceed " +
                  std::to_string(kRtStaticAssetCapacity) + ".",
          "static asset overflow fails initialization by name");

    auto primitiveOverflow = MakeAsset(33u, 1u);
    StaticRtAssetRegistration primitiveRequest{3u, 1u, 1u, 0u, &primitiveOverflow};
    Check(!slot.Initialize(std::span<const StaticRtAssetRegistration>(&primitiveRequest, 1u), diagnostic) &&
              diagnostic == "RtStaticMeshSlot capacity overflow: primitives exceed 32.",
          "primitive overflow fails initialization by name");

    auto materialOverflow = MakeAsset(1u, 33u);
    StaticRtAssetRegistration materialRequest{3u, 1u, 1u, 0u, &materialOverflow};
    Check(!slot.Initialize(std::span<const StaticRtAssetRegistration>(&materialRequest, 1u), diagnostic) &&
              diagnostic == "RtStaticMeshSlot capacity overflow: materials exceed 32.",
          "material overflow fails initialization by name");

    auto textureOverflow = MakeAsset(1u, 17u);
    for (std::size_t materialIndex = 0u; materialIndex < textureOverflow.materials.size(); ++materialIndex)
        textureOverflow.materials[materialIndex].baseColorTexture = static_cast<std::int32_t>(materialIndex);
    StaticRtAssetRegistration textureRequest{3u, 1u, 1u, 0u, &textureOverflow};
    Check(!slot.Initialize(std::span<const StaticRtAssetRegistration>(&textureRequest, 1u), diagnostic) &&
              diagnostic == "RtStaticMeshSlot capacity overflow: baseColor texture layers exceed 16.",
          "metadata cannot assign a texture layer outside the fixed Vulkan array");
}

void TestMaterialAuthoringValidation()
{
    using namespace horde::vulkan::raytracing;
    auto asset = MakeAsset(1u, 1u);
    const StaticRtAssetRegistration request{3u, 1u, 1u, 0u, &asset};
    RtStaticMeshSlot slot;
    std::string diagnostic;
    asset.materials[0].normalScale = std::numeric_limits<float>::max();
    asset.materials[0].textureScale = {{1.0f / 1024.0f, 1024.0f}};
    Check(slot.Initialize(std::span<const StaticRtAssetRegistration>(&request, 1u), diagnostic) &&
              slot.Materials()[0].normalScaleUvScaleBlend[0] == std::numeric_limits<float>::max(),
          "finite normal strengths and exact texture scale boundaries survive GPU packing");
    asset.materials[0].normalScale = std::numeric_limits<float>::infinity();
    Check(!slot.Initialize(std::span<const StaticRtAssetRegistration>(&request, 1u), diagnostic),
          "a constructed asset cannot bypass finite normal scale validation");
    asset.materials[0].normalScale = 1.0f;
    asset.materials[0].textureScale[0] = 0.0f;
    Check(!slot.Initialize(std::span<const StaticRtAssetRegistration>(&request, 1u), diagnostic),
          "a constructed asset cannot bypass positive texture scale validation");
}

void TestTextureArrayCapacities()
{
    using namespace horde::vulkan::raytracing;
    std::string diagnostic;
    Check(RtTextureArrays::Validate({16u, 16u, 16u, 16u}, diagnostic),
          "all four arrays accept the exact 16-layer boundary");
    Check(!RtTextureArrays::Validate({17u, 1u, 1u, 1u}, diagnostic) &&
              diagnostic == "RtTextureArrays capacity overflow: baseColor layers exceed 16.",
          "base colour overflow is named");
    Check(!RtTextureArrays::Validate({1u, 17u, 1u, 1u}, diagnostic) &&
              diagnostic == "RtTextureArrays capacity overflow: normal layers exceed 16.",
          "normal overflow is named");
    Check(!RtTextureArrays::Validate({1u, 1u, 17u, 1u}, diagnostic) &&
              diagnostic == "RtTextureArrays capacity overflow: ORM layers exceed 16.",
          "ORM overflow is named");
    Check(!RtTextureArrays::Validate({1u, 1u, 1u, 17u}, diagnostic) &&
              diagnostic == "RtTextureArrays capacity overflow: emissive layers exceed 16.",
          "emissive overflow is named");
}

void TestPrimaryShadowPolicy()
{
    using namespace horde::vulkan::raytracing;
    Check(ResolvePrimaryAreaShadowSamples(RtWorkloadPreset::Authored,true) == 1 &&
          ResolvePrimaryAreaShadowSamples(RtWorkloadPreset::Lean,true) == 1 &&
          ResolvePrimaryAreaShadowSamples(RtWorkloadPreset::Max,false) == 2 &&
          ResolvePrimaryAreaShadowSamples(RtWorkloadPreset::Max,true) == 4,
          "only compiled High plus Max admits four primary area visibility samples");
    Check(RtSceneTuning{}.workloadPreset == RtWorkloadPreset::Authored,
          "accepted default workload remains Authored");
}

void TestDynamicResolutionFoundation()
{
    using namespace horde::graphics;
    BoundedDynamicResolution controller(75,60,90);
    DynamicResolutionObservation sample{1,25,0.000001,25000000,true,true,true,false,false};
    std::optional<std::uint32_t> request;
    for (std::uint64_t i = 1; i <= 29; ++i) { sample.submissionSequence = i; request = controller.Observe(sample); }
    Check(!request && controller.EffectivePercent() == 75,"DRS waits thirty valid owning timings");
    sample.submissionSequence = 30;
    request = controller.Observe(sample);
    Check(request == 70u && controller.EffectivePercent() == 75,"DRS proposes bounded step before allocating");
    Check(!controller.CompleteResize(65,true),"mismatched resize completion cannot mutate scale");
    Check(controller.CompleteResize(70,false) && controller.EffectivePercent() == 75,
          "failed resize retains effective scale and clears proposal");
    sample.submissionSequence = 31;
    Check(!controller.Observe(sample),"failed allocation starts a fresh dwell");
    controller.Reset(75);
    for (std::uint64_t i = 1; i <= 30; ++i) { sample.submissionSequence = i; request = controller.Observe(sample); }
    Check(controller.CompleteResize(70,true) && controller.EffectivePercent() == 70,
          "effective scale commits only after successful resize");
    for (std::uint64_t i = 31; i < 200; ++i)
    {
        sample.submissionSequence = i;
        if (auto next = controller.Observe(sample)) controller.CompleteResize(*next,true);
    }
    Check(controller.EffectivePercent() == 60,"sustained slow GPU timings stop at lower bound");
    sample.gpuMilliseconds = 8;
    sample.elapsedTicks = 8000000;
    for (std::uint64_t i = 200; i < 500; ++i)
    {
        sample.submissionSequence = i;
        if (auto next = controller.Observe(sample)) controller.CompleteResize(*next,true);
    }
    Check(controller.EffectivePercent() == 90,"sustained headroom stops at upper bound");
    controller.Reset(75);
    sample.gpuMilliseconds = 16.67;
    sample.elapsedTicks = 16670000;
    for (std::uint64_t i = 1; i < 100; ++i)
    { sample.submissionSequence = i; Check(!controller.Observe(sample),"deadband avoids resolution oscillation"); }
    controller.Reset(75);
    sample.gpuMilliseconds = 30;
    sample.elapsedTicks = 30000000;
    for (std::uint64_t i = 1; i < 100; ++i)
    {
        sample.submissionSequence = 1;
        Check(!controller.Observe(sample),"duplicate collected query cannot advance controller");
    }
    // Each invalid category resets streak/dwell; resume cannot reuse stale load.
    for (int invalid = 0; invalid < 9; ++invalid)
    {
        controller.Reset(75);
        sample = {1,30,0.000001,30000000,true,true,true,false,false};
        for (std::uint64_t i = 1; i <= 29; ++i) { sample.submissionSequence = i; controller.Observe(sample); }
        sample.submissionSequence = 30;
        switch (invalid)
        {
            case 0: sample.validGpuTiming = false; break;
            case 1: sample.successfullyPresented = false; break;
            case 2: sample.foregroundGameplay = false; break;
            case 3: sample.capped = true; break;
            case 4: sample.transitionInProgress = true; break;
            case 5: sample.gpuMilliseconds = std::numeric_limits<double>::infinity(); break;
            case 6: sample.timestampResolutionMilliseconds = 1; break;
            case 7: sample.elapsedTicks = 1; break;
            case 8: sample.elapsedTicks = 15000000; break;
        }
        Check(!controller.Observe(sample),"unreliable/background/capped/transition timing freezes DRS");
        sample = {31,30,0.000001,30000000,true,true,true,false,false};
        Check(!controller.Observe(sample),"resume requires fresh hysteresis and dwell");
    }
}

void TestTemporalGeometryFoundation()
{
    using namespace horde::graphics::temporal;
    const auto close = [](float a,float b) { return std::abs(a-b) < 1e-5f; };
    Frame previous;
    previous.camera = MakeHordeCamera(0,0,0,0.05f,0,0,1000,1000);
    const Camera walkingCamera = MakeHordeCamera(1,-2,0.7f,-0.2f,0.8f,1,1920,1080);
    const float expectedU = 0.7f, expectedV = 0.8f;
    const Vec3 walkingRay = Unit(walkingCamera.forward*1.22f +
        walkingCamera.right*((expectedU*2-1)*1920.0f/1080.0f) +
        walkingCamera.up*((expectedV*2-1)*-0.74f));
    const auto roundTrip = Project(walkingCamera,walkingCamera.origin+walkingRay*8);
    Check(roundTrip && close(roundTrip->u,expectedU) && close(roundTrip->v,expectedV),
          "walking yaw/pitch camera projects exact shader ray back to UV");
    previous.objects.push_back({7,1,kIdentity,{{-1,0.70f,-4},{1,0.70f,-4},{0,1.70f,-4}}});
    Hit hit{7,{0,1,2},{0.5f,0.5f,0},false};
    GeometryHistory history;
    Check(!history.Reproject(previous,hit),"initial history cannot yield motion");
    Check(history.Commit(previous,1,true,true),"bounded geometry history commits presented owner");
    auto motion = history.Reproject(previous,hit);
    Check(motion && close(motion->toPreviousU,0) && close(motion->toPreviousV,0) &&
          close(motion->current.depth,4),"static camera and geometry yield zero motion and linear depth");
    Frame current = previous;
    current.objects[0].transform[3] = 1;
    motion = history.Reproject(current,hit);
    Check(motion && close(motion->toPreviousU,-1.22f/8),
          "static object transform motion projects through previous and current world transforms");
    current = previous;
    current.camera = MakeHordeCamera(1,0,0,0.05f,0,0,1000,1000);
    motion = history.Reproject(current,hit);
    Check(motion && close(motion->toPreviousU,1.22f/8),"camera motion matches actual ray projection basis");
    current = previous;
    current.objects[0].vertices[0].x += 1;
    motion = history.Reproject(current,hit);
    Check(motion && close(motion->toPreviousU,-1.22f/16),
          "deforming vertex motion uses actual previous pose with hit barycentrics");
    Check(motion && GeometryHistory::ValidatePreviousSurface(*motion,4,{0,0,1},7) &&
          !GeometryHistory::ValidatePreviousSurface(*motion,3,{0,0,1},7) &&
          !GeometryHistory::ValidatePreviousSurface(*motion,4,{0,0,-1},7) &&
          !GeometryHistory::ValidatePreviousSurface(*motion,4,{0,0,1},8),
          "disocclusion rejects inconsistent previous depth/normal/identity");
    current.objects[0].topologyGeneration = 2;
    Check(!history.Reproject(current,hit),"topology changes cannot reuse vertex correspondence");
    current = previous;
    current.objects[0].vertices[2] = current.objects[0].vertices[0];
    Check(!history.Reproject(current,hit),"degenerate current deformation cannot reuse radiance history");
    current = previous;
    current.objects[0].identity = 8;
    Check(!history.Reproject(current,hit),"new or reassigned instance rejects previous pose");
    current = previous;
    current.objects[0].transform[3] = 100;
    Check(!history.Reproject(current,hit),"offscreen motion cannot address history");
    current = previous;
    current.camera.width = 500;
    Check(!history.Reproject(current,hit),"render dimension change requires history reset");
    Hit reactive = hit;
    reactive.reactive = true;
    Check(!history.Reproject(previous,reactive),"glass water volume or miss is explicit reactive invalid motion");
    hit.triangle[2] = 99;
    Check(!history.Reproject(previous,hit),"invalid vertex indices do not read previous buffers");
    hit = {7,{0,1,2},{0.5f,0.5f,0},false};
    Check(!history.Commit(previous,1,true,true),"duplicate sequence cannot replace owning history");
    Check(!history.Commit(previous,2,true,false) && !history.Reproject(previous,hit) &&
          history.LastReason() == ResetReason::FailedFrame,"failed presentation invalidates history");
    for (auto reason : {ResetReason::Retry,ResetReason::CameraCut,ResetReason::Checkpoint,
                        ResetReason::PreviewTransition,ResetReason::Lifecycle,ResetReason::BackendChange,
                        ResetReason::SettingsChange,ResetReason::Resize})
    {
        history.Commit(previous,3+history.Generation(),true,true);
        const auto generation = history.Generation();
        history.Invalidate(reason);
        Check(history.Generation() == generation+1 && history.LastReason() == reason &&
              !history.Reproject(previous,hit),"every discontinuity invalidates owned previous geometry");
    }
    current = previous;
    current.objects[0].vertices.resize(kMaximumVertices+1);
    Check(!history.Commit(current,100,true,true),"previous deformation memory cap rejects excess vertices");
    current = previous;
    current.objects.push_back(current.objects.front());
    Check(!Valid(current),"duplicate stable identities reject ambiguous previous pose lookup");
    previous.objects[0].vertices.reserve(kMaximumVertices*2);
    Check(history.Commit(previous,101,true,true) && history.RetainedVertexBytes() == 3*sizeof(Vec3),
          "caller spare capacity is not retained in bounded history");
    Check(ProposedAttachmentBytes(1920,1080) == 66355200ull &&
          !ProposedAttachmentBytes(std::numeric_limits<std::uint64_t>::max(),2) &&
          !ProposedAttachmentBytes(0,1080),"HDR depth motion history estimate is exact and overflow safe");
}

} // namespace

int main()
{
    TestAbiLayout();
    TestGeneratedConstants();
    TestGenericRegistrationAndMeasurements();
    TestExplicitTextureGroups();
    TestNamedCapacityFailures();
    TestMaterialAuthoringValidation();
    TestTextureArrayCapacities();
    TestPrimaryShadowPolicy();
    TestDynamicResolutionFoundation();
    TestTemporalGeometryFoundation();
    if (failures != 0)
    {
        std::cerr << failures << " RT scene ABI assertion(s) failed\n";
        return 1;
    }
    std::cout << "RT scene ABI contract passed\n";
    return 0;
}
