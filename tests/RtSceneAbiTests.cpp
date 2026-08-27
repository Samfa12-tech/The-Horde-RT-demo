#include "scene/assets/StaticMeshAsset.h"
#include "vulkan/raytracing/RtSceneAbi.generated.h"
#include "vulkan/raytracing/RtStaticMeshSlot.h"
#include "vulkan/raytracing/RtTextureArrays.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
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
                      value.productionPaneSecondaryRejectCount;
                  })
    {
        Check(sizeof(T) == 32u && alignof(T) == 16u &&
                  offsetof(T, transportOverflowCount) == 0u &&
                  offsetof(T, shadowOverflowCount) == 4u &&
                  offsetof(T, secondaryDielectricRejectCount) == 8u &&
                  offsetof(T, unclosedVolumeCount) == 12u &&
                  offsetof(T, primaryUnclosedVolumeCount) == 16u &&
                  offsetof(T, shadowUnclosedVolumeCount) == 20u &&
                  offsetof(T, productionPaneStackFailureCount) == 24u &&
                  offsetof(T, productionPaneSecondaryRejectCount) == 28u,
              "dielectric diagnostics retain released layout while attributing production pane stack/reject routes");
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
              offsetof(RtInstanceMetadata, assetIndex) == 20u,
          "RtInstanceMetadata CPU offsets match two GLSL uvec4 fields");

    Check(sizeof(RtPrimitiveMetadata) == 16u, "RtPrimitiveMetadata size is 16 bytes");
    Check(offsetof(RtPrimitiveMetadata, vertexOffset) == 0u &&
              offsetof(RtPrimitiveMetadata, indexOffset) == 4u &&
              offsetof(RtPrimitiveMetadata, indexCount) == 8u &&
              offsetof(RtPrimitiveMetadata, materialIndex) == 12u,
          "RtPrimitiveMetadata CPU offsets match one GLSL uvec4 field");
    CheckSplitDielectricDiagnosticLayout<RtDielectricDiagnostics>();

    Check(sizeof(RtMaterialGpu) == 112u && alignof(RtMaterialGpu) == 16u,
          "RtMaterialGpu is seven aligned vec4/uvec4 fields");
    Check(offsetof(RtMaterialGpu, baseColorFactor) == 0u &&
              offsetof(RtMaterialGpu, emissiveFactorStrength) == 16u &&
              offsetof(RtMaterialGpu, metallicRoughnessOcclusionTransmission) == 32u &&
              offsetof(RtMaterialGpu, iorThicknessAttenuationDistance) == 48u &&
              offsetof(RtMaterialGpu, attenuationColor) == 64u &&
              offsetof(RtMaterialGpu, textureLayers) == 80u &&
              offsetof(RtMaterialGpu, materialFlags) == 96u,
          "RtMaterialGpu CPU offsets match generated GLSL declaration");
    Check(sizeof(RtHeldLightGpu) == 16u && alignof(RtHeldLightGpu) == 16u &&
              offsetof(RtHeldLightGpu, positionStrength) == 0u,
          "RtHeldLightGpu appends one exact world-position/strength vec4");
}

void TestGeneratedConstants()
{
    using namespace horde::vulkan::raytracing;
    Check(kRtInstanceMetadataCapacity == 20u, "instance metadata capacity retains all TLAS custom indices");
    Check(kRtStaticAssetCapacity == 8u, "static asset capacity is 8");
    Check(kRtPrimitiveMetadataCapacity == 32u, "primitive capacity is 32");
    Check(kRtMaterialCapacity == 32u, "material capacity is 32");
    Check(kRtTextureLayerCapacity == 16u, "each PBR texture category has 16 layers");
    Check(kRtBindingInstanceMetadata == 11u && kRtBindingPrimitiveMetadata == 12u &&
              kRtBindingMaterials == 13u && kRtBindingStaticVertices == 14u &&
              kRtBindingStaticIndices == 15u && kRtBindingBaseColorTextures == 16u &&
              kRtBindingNormalTextures == 17u && kRtBindingOrmTextures == 18u &&
              kRtBindingEmissiveTextures == 19u && kRtBindingHeldLight == 20u &&
              kRtBindingFireEmitters == 21u && kRtBindingDielectricDiagnostics == 22u,
          "descriptor bindings append dielectric diagnostics at 22 without changing 0-21");
    Check(static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr) == 1u &&
              static_cast<std::uint32_t>(RtInstanceFlag::Emissive) == 2u &&
              static_cast<std::uint32_t>(RtInstanceFlag::Transmissive) == 4u,
          "instance flag enum agrees with hand-checked literals");
    Check(static_cast<std::uint32_t>(RtMaterialFlag::DoubleSided) == 1u &&
              static_cast<std::uint32_t>(RtMaterialFlag::Alpha) == 2u &&
              static_cast<std::uint32_t>(RtMaterialFlag::Transmission) == 4u &&
              static_cast<std::uint32_t>(RtMaterialFlag::BaseColorTexture) == 8u &&
              static_cast<std::uint32_t>(RtMaterialFlag::NormalTexture) == 16u &&
              static_cast<std::uint32_t>(RtMaterialFlag::OrmTexture) == 32u &&
              static_cast<std::uint32_t>(RtMaterialFlag::EmissiveTexture) == 64u &&
              static_cast<std::uint32_t>(RtMaterialFlag::ThinWall) == 512u,
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
    Check(slot.Measurements().vertexBytes == 6u * 64u &&
              slot.Measurements().indexBytes == 6u * 4u &&
              slot.Measurements().materialBytes == 2u * 112u &&
              slot.Measurements().instanceMetadataBytes == 20u * 32u &&
              slot.Measurements().primitiveMetadataBytes == 2u * 16u &&
              slot.Measurements().descriptorCount == 9u,
          "resource measurements use literal ABI sizes and descriptor count");
}

void TestNamedCapacityFailures()
{
    using namespace horde::vulkan::raytracing;
    RtStaticMeshSlot slot;
    std::string diagnostic;
    auto one = MakeAsset(1u, 1u);
    StaticRtAssetRegistration badIndex{
        20u, 1u, static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr), 0u, &one};
    Check(!slot.Initialize(std::span<const StaticRtAssetRegistration>(&badIndex, 1u), diagnostic) &&
              diagnostic == "RtStaticMeshSlot capacity overflow: instanceCustomIndex exceeds RtInstanceMetadata[20].",
          "instance metadata overflow fails initialization by name");

    std::vector<horde::scene::assets::StaticMeshAsset> assets;
    std::vector<StaticRtAssetRegistration> registrations;
    for (std::uint32_t i = 0u; i < 9u; ++i)
    {
        assets.push_back(MakeAsset(1u, 1u));
        registrations.push_back({i, i + 1u, 1u, 0u, &assets.back()});
    }
    // Vector growth moves assets, so rebind pointers after construction.
    for (std::size_t i = 0u; i < registrations.size(); ++i) registrations[i].asset = &assets[i];
    Check(!slot.Initialize(registrations, diagnostic) &&
              diagnostic == "RtStaticMeshSlot capacity overflow: static assets exceed 8.",
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

} // namespace

int main()
{
    TestAbiLayout();
    TestGeneratedConstants();
    TestGenericRegistrationAndMeasurements();
    TestNamedCapacityFailures();
    TestTextureArrayCapacities();
    if (failures != 0)
    {
        std::cerr << failures << " RT scene ABI assertion(s) failed\n";
        return 1;
    }
    std::cout << "RT scene ABI contract passed\n";
    return 0;
}
