#include "scene/assets/AssetManifest.h"
#include "scene/assets/StaticMeshAsset.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace
{

int failures = 0;
const std::filesystem::path kFixtureRoot{HORDE_RT_STATIC_GLTF_FIXTURE_DIR};

void Check(bool condition, std::string_view message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

std::vector<std::uint8_t> ReadBytes(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::uint32_t ReadU32(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u) |
           (static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u) |
           (static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u);
}

void AppendU32(std::vector<std::uint8_t>& bytes, std::uint32_t value)
{
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16u) & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 24u) & 0xffu));
}

void WriteBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

std::string ReadText(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void WriteText(const std::filesystem::path& path, std::string_view text)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(text.data(), static_cast<std::streamsize>(text.size()));
}

struct GlbParts
{
    std::string json;
    std::vector<std::uint8_t> binary;
};

GlbParts ReadGlbParts()
{
    const auto bytes = ReadBytes(kFixtureRoot / "valid-multi.glb");
    const std::uint32_t jsonLength = ReadU32(bytes, 12u);
    std::string json(bytes.begin() + 20u, bytes.begin() + 20u + jsonLength);
    while (!json.empty() && json.back() == ' ') json.pop_back();
    const std::size_t binaryHeader = 20u + jsonLength;
    const std::uint32_t binaryLength = ReadU32(bytes, binaryHeader);
    return {json, {bytes.begin() + binaryHeader + 8u,
                   bytes.begin() + binaryHeader + 8u + binaryLength}};
}

void WriteGlb(const std::filesystem::path& path, std::string json,
              std::vector<std::uint8_t> binary)
{
    while ((json.size() & 3u) != 0u) json.push_back(' ');
    while ((binary.size() & 3u) != 0u) binary.push_back(0u);
    std::vector<std::uint8_t> bytes;
    AppendU32(bytes, 0x46546c67u);
    AppendU32(bytes, 2u);
    AppendU32(bytes, static_cast<std::uint32_t>(12u + 8u + json.size() + 8u + binary.size()));
    AppendU32(bytes, static_cast<std::uint32_t>(json.size()));
    AppendU32(bytes, 0x4e4f534au);
    bytes.insert(bytes.end(), json.begin(), json.end());
    AppendU32(bytes, static_cast<std::uint32_t>(binary.size()));
    AppendU32(bytes, 0x004e4942u);
    bytes.insert(bytes.end(), binary.begin(), binary.end());
    WriteBytes(path, bytes);
}

std::filesystem::path RewriteGlb(const std::filesystem::path& root,
                                 std::string_view name,
                                 std::string_view before,
                                 std::string_view after)
{
    auto parts = ReadGlbParts();
    const auto offset = parts.json.find(before);
    Check(offset != std::string::npos, std::string("test mutation source exists for ") + std::string(name));
    if (offset != std::string::npos)
    {
        parts.json.replace(offset, before.size(), after);
    }
    const auto path = root / name;
    WriteGlb(path, std::move(parts.json), std::move(parts.binary));
    return path;
}

std::filesystem::path RewriteManifest(const std::filesystem::path& root,
                                      std::string_view name,
                                      std::string_view before,
                                      std::string_view after)
{
    std::string text = ReadText(kFixtureRoot / "valid.manifest.json");
    const auto offset = text.find(before);
    Check(offset != std::string::npos, std::string("manifest mutation source exists for ") + std::string(name));
    if (offset != std::string::npos) text.replace(offset, before.size(), after);
    const auto path = root / name;
    WriteText(path, text);
    return path;
}

void ExpectManifestFailure(const std::filesystem::path& path, std::string_view expected)
{
    horde::scene::assets::AssetManifest manifest;
    std::string diagnostic;
    Check(!horde::scene::assets::AssetManifest::Load(path, manifest, diagnostic),
          std::string("manifest must fail: ") + path.filename().string());
    Check(diagnostic == expected,
          std::string("manifest diagnostic for ") + path.filename().string() +
              " expected '" + std::string(expected) + "' got '" + diagnostic + "'");
}

void ExpectAssetFailure(const std::filesystem::path& path,
                        const horde::scene::assets::AssetManifest& manifest,
                        std::string_view expected)
{
    horde::scene::assets::StaticMeshAsset asset;
    std::string diagnostic;
    Check(!horde::scene::assets::StaticMeshAsset::Load(path, manifest, asset, diagnostic),
          std::string("asset must fail: ") + path.filename().string());
    Check(diagnostic == expected,
          std::string("asset diagnostic for ") + path.filename().string() +
              " expected '" + std::string(expected) + "' got '" + diagnostic + "'");
}

void TestManifestContract(const std::filesystem::path& temporaryRoot)
{
    using horde::scene::assets::AssetManifest;
    AssetManifest manifest;
    std::string diagnostic;
    Check(AssetManifest::Load(kFixtureRoot / "valid.manifest.json", manifest, diagnostic),
          std::string("valid manifest loads: ") + diagnostic);
    Check(manifest.schema == 1u, "schema 1 is retained");
    Check(manifest.assetName == "fixture-static-mesh", "asset name is retained");
    Check(manifest.metresPerUnit == 1.0f, "metre scale is retained");
    Check(manifest.upAxis == "+Y" && manifest.forwardAxis == "+Z", "coordinate contract is retained");
    Check(manifest.budgets.maxVertices == 16u && manifest.budgets.maxIndices == 24u,
          "geometry budgets are retained");
    Check(manifest.lods.size() == 1u && manifest.lods[0].name == "lod0" &&
              manifest.lods[0].maxTriangles == 8u,
          "LOD contract is retained");
    Check(manifest.requiredSockets.size() == 1u && manifest.requiredSockets[0] == "grip",
          "socket contract is retained");
    Check(manifest.textureProfile.androidEncoding == "astc" &&
              manifest.textureProfile.windowsEncoding == "rgba8" && manifest.textureProfile.mipmapped,
          "runtime texture profile is retained");
    Check(manifest.materialOverrides.size() == 1u &&
              manifest.materialOverrides[0].material == "FixtureMaterial" &&
              manifest.materialOverrides[0].emissiveStrength == 2.0f,
          "material override is retained");

    ExpectManifestFailure(kFixtureRoot / "bad-schema.manifest.json",
                          "Asset manifest schema must be 1.");
    ExpectManifestFailure(kFixtureRoot / "bad-scale.manifest.json",
                          "Asset manifest metresPerUnit must be finite and greater than zero.");
    ExpectManifestFailure(kFixtureRoot / "bad-axis.manifest.json",
                          "Asset manifest coordinateSystem must be +Y up and +Z forward.");
    ExpectManifestFailure(
        RewriteManifest(temporaryRoot, "bad-budget.manifest.json", "\"maxVertices\": 16", "\"maxVertices\": 0"),
        "Asset manifest budgets must all be greater than zero.");
    ExpectManifestFailure(
        RewriteManifest(temporaryRoot, "bad-profile.manifest.json", "\"android\": \"astc\"", "\"android\": \"rgba8\""),
        "Asset manifest runtimeTextureProfile must be Android ASTC, Windows RGBA8, and mipmapped.");
}

void TestAcceptedStaticGlbContract(const horde::scene::assets::AssetManifest& manifest)
{
    using horde::scene::assets::StaticMeshAsset;
    using horde::scene::assets::StaticRtVertex;
    StaticMeshAsset asset;
    std::string diagnostic;
    Check(StaticMeshAsset::Load(kFixtureRoot / "valid-multi.glb", manifest, asset, diagnostic),
          std::string("valid multi-geometry GLB loads: ") + diagnostic);
    Check(sizeof(StaticRtVertex) == 64u, "StaticRtVertex is exactly 64 bytes");
    Check(asset.vertices.size() == 8u && asset.indices.size() == 12u,
          "both 16-bit and 32-bit indexed primitives are expanded into the static asset");
    Check(asset.primitives.size() == 2u && asset.primitives[0].indexCount == 6u &&
              asset.primitives[1].indexCount == 6u,
          "multiple meshes and primitives retain independent records");
    Check(asset.primitives[0].vertexOffset == 0u && asset.primitives[1].vertexOffset == 4u &&
              asset.primitives[0].materialIndex == 0u && asset.primitives[1].materialIndex == 0u,
          "primitive offsets and material mapping are deterministic");
    Check(asset.nodeTransforms.size() == 3u && asset.sockets.size() == 1u &&
              asset.sockets[0].name == "grip",
          "TRS, matrix, and named socket nodes are retained");
    Check(asset.nodeTransforms[0].world[12] == 1.0f && asset.nodeTransforms[0].world[13] == 2.0f &&
              asset.nodeTransforms[0].world[14] == 3.0f &&
              asset.nodeTransforms[2].world[12] == 4.0f && asset.nodeTransforms[2].world[13] == 5.0f &&
              asset.nodeTransforms[2].world[14] == 6.0f,
          "node TRS and matrix transforms use the glTF column-major contract");
    Check(asset.bounds.minimum[0] == 0.0f && asset.bounds.minimum[1] == 0.0f &&
              asset.bounds.maximum[0] == 1.0f && asset.bounds.maximum[1] == 1.0f,
          "asset bounds are computed from real positions");
    Check(asset.materials.size() == 1u, "one PBR material is retained");
    if (!asset.materials.empty())
    {
        const auto& material = asset.materials[0];
        Check(material.baseColorFactor == std::array<float, 4u>{{0.2f, 0.3f, 0.4f, 0.8f}},
              "base colour factor is decoded");
        Check(material.metallicFactor == 0.7f && material.roughnessFactor == 0.25f &&
                  material.occlusionStrength == 0.6f,
              "metallic, roughness, and occlusion factors are decoded");
        Check(material.transmissionFactor == 0.4f && material.ior == 1.7f &&
                  material.thicknessFactor == 0.5f && material.attenuationDistance == 3.0f,
              "KHR transmission, IOR, and volume fields are decoded");
        Check(material.emissiveStrength == 2.0f,
              "audited manifest material override replaces KHR emissive strength");
        Check(material.baseColorTexture == 0 && material.normalTexture == 0 &&
                  material.ormTexture == 0 && material.emissiveTexture == 0,
              "four runtime texture categories receive deterministic layers");
    }
}

void TestMalformedAndUnsupportedGlbs(const std::filesystem::path& temporaryRoot,
                                     const horde::scene::assets::AssetManifest& manifest)
{
    ExpectAssetFailure(kFixtureRoot / "bad-magic.glb", manifest,
                       "Static GLB header magic is not 'glTF'.");
    ExpectAssetFailure(kFixtureRoot / "bad-version.glb", manifest,
                       "Static GLB version must be 2.");
    ExpectAssetFailure(kFixtureRoot / "bad-chunk.glb", manifest,
                       "Static GLB chunks are malformed or truncated.");

    ExpectAssetFailure(
        RewriteGlb(temporaryRoot, "bad-accessor.glb", "\"POSITION\":0", "\"POSITION\":99"), manifest,
        "Static GLB contains an out-of-range accessor or index reference.");

    auto parts = ReadGlbParts();
    parts.binary[192u] = 9u;
    const auto badIndex = temporaryRoot / "bad-index.glb";
    WriteGlb(badIndex, std::move(parts.json), std::move(parts.binary));
    ExpectAssetFailure(badIndex, manifest, "Static GLB index is out of range for primitive 0.");

    ExpectAssetFailure(
        RewriteGlb(temporaryRoot, "bad-mode.glb", "\"material\":0,\"mode\":4", "\"material\":0,\"mode\":1"), manifest,
        "Static GLB primitive 0 mode must be TRIANGLES.");
    ExpectAssetFailure(
        RewriteGlb(temporaryRoot, "missing-normal.glb", "\"NORMAL\":1,", ""), manifest,
        "Static GLB primitive 0 is missing NORMAL.");
    ExpectAssetFailure(
        RewriteGlb(temporaryRoot, "missing-uv.glb", "\"TEXCOORD_0\":3", "\"TEXCOORD_1\":3"), manifest,
        "Static GLB primitive 0 is missing TEXCOORD_0.");
    ExpectAssetFailure(
        RewriteGlb(temporaryRoot, "missing-tangent.glb", "\"TANGENT\":2,", ""), manifest,
        "Static GLB primitive 0 uses a normal texture but is missing TANGENT.");
    ExpectAssetFailure(
        RewriteGlb(temporaryRoot, "non-finite-transform.glb", "\"translation\":[1,2,3]", "\"translation\":[1e40,2,3]"), manifest,
        "Static GLB node 'Root' contains a non-finite transform.");
    ExpectAssetFailure(
        RewriteGlb(temporaryRoot, "negative-scale.glb", "\"translation\":[1,2,3]", "\"translation\":[1,2,3],\"scale\":[-1,1,1]"), manifest,
        "Static GLB node 'Root' contains a negative scale.");
    ExpectAssetFailure(
        RewriteGlb(temporaryRoot, "missing-socket.glb", "\"name\":\"grip\"", "\"name\":\"not-grip\""), manifest,
        "Static GLB is missing required socket 'grip'.");

    ExpectAssetFailure(
        RewriteGlb(temporaryRoot, "sparse.glb", "\"bufferView\":0,\"componentType\":5126,\"count\":4,\"type\":\"VEC3\"",
                   "\"bufferView\":0,\"componentType\":5126,\"count\":4,\"type\":\"VEC3\",\"sparse\":{\"count\":1,\"indices\":{\"bufferView\":4,\"componentType\":5123},\"values\":{\"bufferView\":0}}"),
        manifest, "Static GLB accessor 0 uses sparse data, which is unsupported.");
    ExpectAssetFailure(
        RewriteGlb(temporaryRoot, "morph.glb", "\"material\":0,\"mode\":4", "\"material\":0,\"mode\":4,\"targets\":[{\"POSITION\":0}]"),
        manifest, "Static GLB primitive 0 uses morph targets, which are unsupported.");
    ExpectAssetFailure(
        RewriteGlb(temporaryRoot, "draco.glb", "\"material\":0,\"mode\":4",
                   "\"material\":0,\"mode\":4,\"extensions\":{\"KHR_draco_mesh_compression\":{\"bufferView\":0,\"attributes\":{\"POSITION\":0}}}"),
        manifest, "Static GLB primitive 0 uses Draco compression, which is unsupported.");
    ExpectAssetFailure(
        RewriteGlb(temporaryRoot, "meshopt.glb", "\"buffer\":0,\"byteOffset\":0,\"byteLength\":48",
                   "\"buffer\":0,\"byteOffset\":0,\"byteLength\":48,\"extensions\":{\"KHR_meshopt_compression\":{\"buffer\":0,\"byteOffset\":0,\"byteLength\":48,\"byteStride\":12,\"count\":4,\"mode\":\"ATTRIBUTES\",\"filter\":\"NONE\"}}"),
        manifest, "Static GLB buffer view 0 uses meshopt compression, which is unsupported.");
    ExpectAssetFailure(
        RewriteGlb(temporaryRoot, "unknown-required-extension.glb",
                   "\"extensionsRequired\":[", "\"extensionsRequired\":[\"EXT_unknown_required\","),
        manifest, "Static GLB requires unsupported extension 'EXT_unknown_required'.");
}

void TestBudgetFailures(const std::filesystem::path& temporaryRoot,
                        const horde::scene::assets::AssetManifest& validManifest)
{
    using horde::scene::assets::AssetManifest;
    AssetManifest manifest;
    std::string diagnostic;

    const auto vertexManifest = RewriteManifest(
        temporaryRoot, "vertex-budget.manifest.json", "\"maxVertices\": 16", "\"maxVertices\": 7");
    Check(AssetManifest::Load(vertexManifest, manifest, diagnostic), "vertex budget manifest loads");
    ExpectAssetFailure(kFixtureRoot / "valid-multi.glb", manifest,
                       "Static GLB exceeds manifest maxVertices capacity.");

    const auto primitiveManifest = RewriteManifest(
        temporaryRoot, "primitive-budget.manifest.json", "\"maxPrimitives\": 4", "\"maxPrimitives\": 1");
    Check(AssetManifest::Load(primitiveManifest, manifest, diagnostic), "primitive budget manifest loads");
    ExpectAssetFailure(kFixtureRoot / "valid-multi.glb", manifest,
                       "Static GLB exceeds manifest maxPrimitives capacity.");

    const auto twoMaterials = RewriteGlb(
        temporaryRoot, "material-overflow.glb", "}],\"meshes\"",
        "},{\"name\":\"UnusedMaterial\",\"pbrMetallicRoughness\":{\"baseColorFactor\":[1,1,1,1],\"metallicFactor\":0,\"roughnessFactor\":1}}],\"meshes\"
    );
    const auto materialManifest = RewriteManifest(
        temporaryRoot, "material-budget.manifest.json", "\"maxMaterials\": 4", "\"maxMaterials\": 1");
    Check(AssetManifest::Load(materialManifest, manifest, diagnostic), "material budget manifest loads");
    ExpectAssetFailure(twoMaterials, manifest,
                       "Static GLB exceeds manifest maxMaterials capacity.");

    auto textureParts = ReadGlbParts();
    const auto texturesOffset = textureParts.json.find("\"textures\":[{\"source\":0}]");
    const auto baseColorOffset = textureParts.json.find("\"baseColorTexture\":{\"index\":0}");
    Check(texturesOffset != std::string::npos && baseColorOffset != std::string::npos,
          "texture overflow mutation sources exist");
    textureParts.json.replace(texturesOffset, std::string_view("\"textures\":[{\"source\":0}]").size(),
                              "\"textures\":[{\"source\":0},{\"source\":0}]");
    const auto adjustedBaseColorOffset = textureParts.json.find("\"baseColorTexture\":{\"index\":0}");
    textureParts.json.replace(adjustedBaseColorOffset,
                              std::string_view("\"baseColorTexture\":{\"index\":0}").size(),
                              "\"baseColorTexture\":{\"index\":1}");
    const auto textureOverflow = temporaryRoot / "texture-overflow.glb";
    WriteGlb(textureOverflow, std::move(textureParts.json), std::move(textureParts.binary));
    const auto textureManifest = RewriteManifest(
        temporaryRoot, "texture-budget.manifest.json", "\"maxTextureLayersPerKind\": 4", "\"maxTextureLayersPerKind\": 1");
    Check(AssetManifest::Load(textureManifest, manifest, diagnostic), "texture budget manifest loads");
    // A single material references one layer, so the valid fixture is the exact boundary.
    horde::scene::assets::StaticMeshAsset boundaryAsset;
    Check(horde::scene::assets::StaticMeshAsset::Load(
              kFixtureRoot / "valid-multi.glb", manifest, boundaryAsset, diagnostic),
          "one texture layer meets a capacity of one");
    ExpectAssetFailure(textureOverflow, manifest,
                       "Static GLB exceeds manifest maxTextureLayersPerKind capacity for baseColor.");

    const auto indexManifest = RewriteManifest(
        temporaryRoot, "index-budget.manifest.json", "\"maxIndices\": 24", "\"maxIndices\": 11");
    Check(AssetManifest::Load(indexManifest, manifest, diagnostic), "index budget manifest loads");
    ExpectAssetFailure(kFixtureRoot / "valid-multi.glb", manifest,
                       "Static GLB exceeds manifest maxIndices capacity.");

    const auto socketManifestPath = RewriteManifest(
        temporaryRoot, "socket.manifest.json", "[\"grip\"]", "[\"blade_grip\"]");
    Check(AssetManifest::Load(socketManifestPath, manifest, diagnostic), "missing socket manifest loads");
    ExpectAssetFailure(kFixtureRoot / "valid-multi.glb", manifest,
                       "Static GLB is missing required socket 'blade_grip'.");

    (void)validManifest;
}

} // namespace

int main()
{
    const auto temporaryRoot = std::filesystem::temp_directory_path() / "horde-rt-static-gltf-tests";
    std::error_code removeError;
    std::filesystem::remove_all(temporaryRoot, removeError);
    std::filesystem::create_directories(temporaryRoot);

    TestManifestContract(temporaryRoot);

    horde::scene::assets::AssetManifest manifest;
    std::string diagnostic;
    if (!horde::scene::assets::AssetManifest::Load(
            kFixtureRoot / "valid.manifest.json", manifest, diagnostic))
    {
        std::cerr << "FAIL: fixture manifest failed before asset tests: " << diagnostic << '\n';
        ++failures;
    }
    else
    {
        TestAcceptedStaticGlbContract(manifest);
        TestMalformedAndUnsupportedGlbs(temporaryRoot, manifest);
        TestBudgetFailures(temporaryRoot, manifest);
    }

    std::filesystem::remove_all(temporaryRoot, removeError);
    if (failures != 0)
    {
        std::cerr << failures << " static glTF asset assertion(s) failed\n";
        return 1;
    }
    std::cout << "Static GLB asset contract passed\n";
    return 0;
}
