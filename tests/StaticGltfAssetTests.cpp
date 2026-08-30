#include "scene/assets/AssetManifest.h"
#include "scene/assets/AssetValidation.h"
#include "scene/assets/DielectricTopologyMath.h"
#include "scene/assets/StaticMeshAsset.h"
#include "third_party/cgltf/cgltf.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

namespace
{

int failures = 0;
std::filesystem::path executablePath;
const std::filesystem::path kFixtureRoot{HORDE_RT_STATIC_GLTF_FIXTURE_DIR};
const std::filesystem::path kDielectricFixtureRoot{HORDE_RT_DIELECTRIC_FIXTURE_DIR};

void Check(bool condition, std::string_view message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

std::uint64_t CurrentProcessId()
{
#if defined(_WIN32)
    return static_cast<std::uint64_t>(_getpid());
#else
    return static_cast<std::uint64_t>(getpid());
#endif
}

class ScopedStaticGltfTestDirectory
{
public:
    ScopedStaticGltfTestDirectory()
    {
        base_ = std::filesystem::absolute(
            std::filesystem::temp_directory_path()).lexically_normal();
        if (!base_.has_filename()) base_ = base_.parent_path();
#if defined(NDEBUG)
        constexpr std::string_view configuration = "release";
#else
        constexpr std::string_view configuration = "debug";
#endif
        root_ = (base_ / (std::string("horde-rt-static-gltf-tests-") +
                          std::string(configuration) + "-pid-" +
                          std::to_string(CurrentProcessId()))).lexically_normal();
        safe_ = root_.parent_path() == base_ &&
            root_.filename().string().starts_with(
                "horde-rt-static-gltf-tests-");
        if (!safe_)
        {
            std::cerr << "Unsafe temporary root rejected: base='" << base_.string()
                      << "' root='" << root_.string() << "'\n";
            return;
        }
        std::error_code error;
        std::filesystem::remove_all(root_, error);
        error.clear();
        std::filesystem::create_directories(root_, error);
        ready_ = !error && std::filesystem::is_directory(root_);
        if (!ready_)
        {
            std::cerr << "Temporary root setup failed: base='" << base_.string()
                      << "' root='" << root_.string() << "' safe=" << safe_
                      << " error='" << error.message() << "'\n";
        }
    }

    ~ScopedStaticGltfTestDirectory()
    {
        if (!safe_) return;
        std::error_code error;
        std::filesystem::remove_all(root_, error);
    }

    ScopedStaticGltfTestDirectory(const ScopedStaticGltfTestDirectory&) = delete;
    ScopedStaticGltfTestDirectory& operator=(
        const ScopedStaticGltfTestDirectory&) = delete;

    [[nodiscard]] bool Ready() const { return ready_; }
    [[nodiscard]] const std::filesystem::path& Path() const { return root_; }

private:
    std::filesystem::path base_;
    std::filesystem::path root_;
    bool safe_ = false;
    bool ready_ = false;
};

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

void WriteFloat(std::vector<std::uint8_t>& bytes, std::size_t offset, float value)
{
    static_assert(sizeof(float) == 4u);
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}

void AppendFloat(std::vector<std::uint8_t>& bytes, float value)
{
    static_assert(sizeof(float) == 4u);
    const auto offset = bytes.size();
    bytes.resize(offset + sizeof(value));
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}

void AppendU16(std::vector<std::uint8_t>& bytes, std::uint16_t value)
{
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
}

bool NearlyEqual(float actual, float expected, float epsilon = 0.00001f)
{
    return std::abs(actual - expected) <= epsilon;
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

std::filesystem::path WriteClosedDielectricGlb(const std::filesystem::path& root,
                                               std::string_view name,
                                               bool omitFrontFace,
                                               bool duplicateBottomTriangle,
                                               bool includeVolume = true,
                                               float thicknessFactor = 1.0f,
                                               int windingMutation = 0,
                                               std::string_view nodeTransform = {})
{
    constexpr std::array<std::array<float, 3u>, 8u> positions{{
        {{-0.5f, -0.5f, -0.5f}}, {{0.5f, -0.5f, -0.5f}},
        {{0.5f, 0.5f, -0.5f}}, {{-0.5f, 0.5f, -0.5f}},
        {{-0.5f, -0.5f, 0.5f}}, {{0.5f, -0.5f, 0.5f}},
        {{0.5f, 0.5f, 0.5f}}, {{-0.5f, 0.5f, 0.5f}}}};
    constexpr std::array<std::uint16_t, 36u> closedIndices{{
        0, 2, 1, 0, 3, 2,
        4, 5, 6, 4, 6, 7,
        0, 4, 7, 0, 7, 3,
        1, 2, 6, 1, 6, 5,
        0, 1, 5, 0, 5, 4,
        3, 7, 6, 3, 6, 2}};
    std::vector<std::uint16_t> indices(closedIndices.begin(), closedIndices.end());
    if (omitFrontFace) indices.erase(indices.begin() + 6, indices.begin() + 12);
    if (duplicateBottomTriangle) indices.insert(indices.end(), {0u, 1u, 5u});
    if (windingMutation == 1)
    {
        for (std::size_t triangle = 0u; triangle + 2u < indices.size(); triangle += 3u)
            std::swap(indices[triangle + 1u], indices[triangle + 2u]);
    }
    else if (windingMutation == 2 && indices.size() >= 3u)
    {
        std::swap(indices[1], indices[2]);
    }

    std::vector<std::uint8_t> binary;
    for (const auto& p : positions)
        for (float value : p) AppendFloat(binary, value);
    const std::size_t normalOffset = binary.size();
    for (const auto& p : positions)
    {
        const float length = std::sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
        AppendFloat(binary, p[0] / length);
        AppendFloat(binary, p[1] / length);
        AppendFloat(binary, p[2] / length);
    }
    const std::size_t uvOffset = binary.size();
    for (std::size_t vertex = 0u; vertex < positions.size(); ++vertex)
    {
        AppendFloat(binary, (vertex & 1u) != 0u ? 1.0f : 0.0f);
        AppendFloat(binary, (vertex & 2u) != 0u ? 1.0f : 0.0f);
    }
    const std::size_t indexOffset = binary.size();
    for (std::uint16_t index : indices) AppendU16(binary, index);

    const std::string extensionNames = includeVolume
        ? "[\"KHR_materials_transmission\",\"KHR_materials_volume\",\"KHR_materials_ior\"]"
        : "[\"KHR_materials_transmission\",\"KHR_materials_ior\"]";
    const std::string volumeExtension = includeVolume
        ? ",\"KHR_materials_volume\":{\"thicknessFactor\":" +
              std::to_string(thicknessFactor) +
              ",\"attenuationDistance\":2,\"attenuationColor\":[0.75,0.9,1]}"
        : "";
    const std::string json =
        "{\"asset\":{\"version\":\"2.0\"},"
        "\"extensionsUsed\":" + extensionNames + ","
        "\"extensionsRequired\":" + extensionNames + ","
        "\"buffers\":[{\"byteLength\":" + std::to_string(binary.size()) + "}],"
        "\"bufferViews\":["
        "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":96,\"target\":34962},"
        "{\"buffer\":0,\"byteOffset\":" + std::to_string(normalOffset) +
            ",\"byteLength\":96,\"target\":34962},"
        "{\"buffer\":0,\"byteOffset\":" + std::to_string(uvOffset) +
            ",\"byteLength\":64,\"target\":34962},"
        "{\"buffer\":0,\"byteOffset\":" + std::to_string(indexOffset) +
            ",\"byteLength\":" + std::to_string(indices.size() * 2u) +
            ",\"target\":34963}],"
        "\"accessors\":["
        "{\"bufferView\":0,\"componentType\":5126,\"count\":8,\"type\":\"VEC3\",\"min\":[-0.5,-0.5,-0.5],\"max\":[0.5,0.5,0.5]},"
        "{\"bufferView\":1,\"componentType\":5126,\"count\":8,\"type\":\"VEC3\"},"
        "{\"bufferView\":2,\"componentType\":5126,\"count\":8,\"type\":\"VEC2\"},"
        "{\"bufferView\":3,\"componentType\":5123,\"count\":" +
            std::to_string(indices.size()) + ",\"type\":\"SCALAR\"}],"
        "\"materials\":[{\"name\":\"ClosedGlass\",\"doubleSided\":true,"
        "\"pbrMetallicRoughness\":{\"baseColorFactor\":[0.92,0.97,1,1],\"metallicFactor\":0,\"roughnessFactor\":0.12},"
        "\"extensions\":{\"KHR_materials_transmission\":{\"transmissionFactor\":0.94},"
        "\"KHR_materials_ior\":{\"ior\":1.52}" + volumeExtension + "}}],"
        "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0,\"NORMAL\":1,\"TEXCOORD_0\":2},"
        "\"indices\":3,\"material\":0,\"mode\":4}]}],"
        "\"nodes\":[{\"name\":\"Root\",\"mesh\":0" +
            std::string(nodeTransform) +
            "},{\"name\":\"grip\"}],\"scenes\":[{\"nodes\":[0,1]}],\"scene\":0}";
    const auto path = root / name;
    WriteGlb(path, json, std::move(binary));
    return path;
}

void TestTransmissionDefaultThinWallSemantics(
    const std::filesystem::path& temporaryRoot,
    const horde::scene::assets::AssetManifest& sourceManifest)
{
    auto manifest = sourceManifest;
    manifest.materialOverrides.clear();
    manifest.budgets.maxIndices = 48u;
    manifest.lods[0].maxTriangles = 16u;
    horde::scene::assets::StaticMeshAsset asset;
    std::string diagnostic;

    const auto extensionOnly = WriteClosedDielectricGlb(
        temporaryRoot, "transmission-only-lod0.runtime.glb", false, false, false);
    Check(horde::scene::assets::StaticMeshAsset::Load(
              extensionOnly, manifest, asset, diagnostic) &&
              (asset.materials[0].flags & 512u) != 0u,
          std::string("transmission without KHR_volume maps to ThinWall: ") + diagnostic);

    const auto zeroThickness = WriteClosedDielectricGlb(
        temporaryRoot, "zero-thickness-lod0.runtime.glb", false, false, true, 0.0f);
    Check(horde::scene::assets::StaticMeshAsset::Load(
              zeroThickness, manifest, asset, diagnostic) &&
              (asset.materials[0].flags & 512u) != 0u,
          std::string("KHR_volume zero thickness maps to ThinWall: ") + diagnostic);

    horde::scene::assets::MaterialOverride explicitFalse;
    explicitFalse.material = "ClosedGlass";
    explicitFalse.hasThinWall = true;
    explicitFalse.thinWall = false;
    manifest.materialOverrides = {explicitFalse};
    Check(horde::scene::assets::StaticMeshAsset::Load(
              zeroThickness, manifest, asset, diagnostic) &&
              (asset.materials[0].flags & 512u) != 0u,
          "zero-thickness glTF semantics remain ThinWall despite a contradictory false override");

    horde::scene::assets::MaterialOverride thickOverride;
    thickOverride.material = "ClosedGlass";
    thickOverride.hasThicknessFactor = true;
    thickOverride.thicknessFactor = 0.004f;
    manifest.materialOverrides = {thickOverride};
    Check(horde::scene::assets::StaticMeshAsset::Load(
              extensionOnly, manifest, asset, diagnostic) &&
              (asset.materials[0].flags & 512u) == 0u &&
              NearlyEqual(asset.materials[0].thicknessFactor, 0.004f),
          std::string("audited positive thickness override selects real closed-volume intent: ") + diagnostic);
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

void ExpectAssetFailureContains(
    const std::filesystem::path& path,
    const horde::scene::assets::AssetManifest& manifest,
    std::initializer_list<std::string_view> expectedFragments)
{
    horde::scene::assets::StaticMeshAsset asset;
    std::string diagnostic;
    Check(!horde::scene::assets::StaticMeshAsset::Load(path, manifest, asset, diagnostic),
          std::string("asset must fail: ") + path.filename().string());
    for (const std::string_view fragment : expectedFragments)
    {
        Check(diagnostic.find(fragment) != std::string::npos,
              std::string("asset diagnostic for ") + path.filename().string() +
                  " must contain '" + std::string(fragment) + "', got '" + diagnostic + "'");
    }
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
              manifest.materialOverrides[0].emissiveStrength == 2.0f &&
              manifest.materialOverrides[0].transmissionFactor == 0.73f &&
              manifest.materialOverrides[0].ior == 1.61f &&
              manifest.materialOverrides[0].thicknessFactor == 0.42f &&
              manifest.materialOverrides[0].attenuationDistance == 2.5f &&
              manifest.materialOverrides[0].attenuationColor ==
                  std::array<float, 3u>{{0.8f, 0.9f, 1.0f}} &&
              manifest.materialOverrides[0].roughnessFactor == 0.18f &&
              manifest.materialOverrides[0].thinWall,
          "dielectric material overrides are retained");

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
        RewriteManifest(temporaryRoot, "zero-lod.manifest.json", "\"maxTriangles\": 8", "\"maxTriangles\": 0"),
        "Asset manifest LOD 'lod0' maxTriangles must be greater than zero.");
    ExpectManifestFailure(
        RewriteManifest(temporaryRoot, "bad-profile.manifest.json", "\"android\": \"astc\"", "\"android\": \"rgba8\""),
        "Asset manifest runtimeTextureProfile must be Android ASTC, Windows RGBA8, and mipmapped.");

    ExpectManifestFailure(
        RewriteManifest(temporaryRoot, "duplicate-root-key.manifest.json",
                        "\"schema\": 1,", "\"schema\": 1, \"schema\": 1,"),
        "Asset manifest JSON contains duplicate key 'schema'.");

    const auto trailingGarbage = temporaryRoot / "trailing-garbage.manifest.json";
    WriteText(trailingGarbage, ReadText(kFixtureRoot / "valid.manifest.json") + " trailing");
    ExpectManifestFailure(trailingGarbage,
                          "Asset manifest JSON has trailing content.");

    ExpectManifestFailure(
        RewriteManifest(temporaryRoot, "wrong-nesting.manifest.json",
                        "\"schema\": 1,", "\"metadata\": {\"schema\": 1},"),
        "Asset manifest contains unsupported root field 'metadata'.");

    const auto oversizedManifest = temporaryRoot / "oversized.manifest.json";
    {
        std::ofstream output(oversizedManifest, std::ios::binary | std::ios::trunc);
        output.seekp(static_cast<std::streamoff>(1024u * 1024u));
        output.put('x');
    }
    ExpectManifestFailure(oversizedManifest,
                          "Asset manifest exceeds the bounded JSON size limit.");
}

void TestAccessorRangeRejectsOverflow()
{
    std::uint8_t byte = 0u;
    cgltf_buffer buffer{};
    buffer.data = &byte;
    buffer.size = std::numeric_limits<std::size_t>::max();
    cgltf_buffer_view view{};
    view.buffer = &buffer;
    view.size = std::numeric_limits<std::size_t>::max();
    cgltf_accessor accessor{};
    accessor.buffer_view = &view;
    accessor.type = cgltf_type_scalar;
    accessor.component_type = cgltf_component_type_r_32f;
    accessor.count = 4u;
    accessor.stride = std::numeric_limits<std::size_t>::max() / 2u;

    std::string diagnostic;
    Check(!horde::scene::assets::ValidateAccessorRange(accessor, 0u, diagnostic),
          "accessor range validation rejects overflowing count/stride arithmetic");
    Check(diagnostic == "Static GLB accessor data is out of range.",
          "overflowing accessor range uses the bounded range diagnostic");
}

void TestUnsupportedAlphaMaterialIsRejected(
    const std::filesystem::path& temporaryRoot,
    const horde::scene::assets::AssetManifest& manifest)
{
    const auto alpha = RewriteGlb(
        temporaryRoot, "alpha-mask.glb", "{\"name\":\"FixtureMaterial\"",
        "{\"alphaMode\":\"MASK\",\"name\":\"FixtureMaterial\"");
    ExpectAssetFailure(alpha, manifest,
                       "Static GLB material 0 uses unsupported alphaMode; only OPAQUE is supported.");
}

void TestCyclicNodeGraphIsRejectedBeforeTraversal(
    const std::filesystem::path& temporaryRoot)
{
    const auto cyclic = RewriteGlb(
        temporaryRoot, "cyclic-node.glb", "{\"name\":\"grip\",\"rotation\"",
        "{\"name\":\"grip\",\"children\":[0],\"rotation\"");
    const auto manifest = kFixtureRoot / "valid.manifest.json";
#if defined(_WIN32)
    const std::string executable = executablePath.string();
    const std::string cyclicArgument = cyclic.string();
    const std::string manifestArgument = manifest.string();
    _putenv_s("HORDE_TEST_CYCLIC_PATH", cyclicArgument.c_str());
    _putenv_s("HORDE_TEST_CYCLIC_MANIFEST", manifestArgument.c_str());
    const char* arguments[]{executable.c_str(), nullptr};
    const int result = static_cast<int>(_spawnv(_P_WAIT, executable.c_str(), arguments));
    _putenv_s("HORDE_TEST_CYCLIC_PATH", "");
    _putenv_s("HORDE_TEST_CYCLIC_MANIFEST", "");
#else
    const std::string command = "\"" + executablePath.string() +
        "\" --probe-cyclic \"" + cyclic.string() + "\" \"" + manifest.string() + "\"";
    const int result = std::system(command.c_str());
#endif
    Check(result == 0,
          std::string("cyclic node graph is rejected cleanly before recursive world-transform traversal; child result=") +
              std::to_string(result));
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
    Check(asset.vertices[0].position == std::array<float, 4u>{{1.0f, 2.0f, 3.0f, 1.0f}} &&
              asset.vertices[4].position == std::array<float, 4u>{{4.0f, 5.0f, 6.0f, 1.0f}},
          "node transforms are baked exactly once into runtime positions");
    Check(asset.bounds.minimum == std::array<float, 3u>{{1.0f, 2.0f, 3.0f}} &&
              asset.bounds.maximum == std::array<float, 3u>{{5.0f, 6.0f, 6.0f}},
          "asset bounds are computed from baked positions");
    Check(asset.materials.size() == 1u, "one PBR material is retained");
    if (!asset.materials.empty())
    {
        const auto& material = asset.materials[0];
        Check(material.baseColorFactor == std::array<float, 4u>{{0.2f, 0.3f, 0.4f, 0.8f}},
              "base colour factor is decoded");
        Check(material.metallicFactor == 0.7f && material.roughnessFactor == 0.18f &&
                  material.occlusionStrength == 0.6f,
              "metallic, roughness, and occlusion factors are decoded");
        Check(material.transmissionFactor == 0.73f && material.ior == 1.61f &&
                  material.thicknessFactor == 0.42f && material.attenuationDistance == 2.5f &&
                  material.attenuationColor == std::array<float, 3u>{{0.8f, 0.9f, 1.0f}} &&
                  material.roughnessFactor == 0.18f,
              "KHR dielectric fields are decoded and audited sidecar overrides are applied");
        Check(material.emissiveStrength == 2.0f,
              "audited manifest material override replaces KHR emissive strength");
        Check((material.flags & 512u) != 0u,
              "thin-wall sidecar intent reaches the generated material ABI flag");
        Check(material.baseColorTexture == 0 && material.normalTexture == 0 &&
                  material.ormTexture == 0 && material.emissiveTexture == 0,
              "four runtime texture categories receive deterministic layers");
    }
}

void TestThickDielectricTopology(const std::filesystem::path& temporaryRoot,
                                 const horde::scene::assets::AssetManifest& sourceManifest)
{
    auto manifest = sourceManifest;
    manifest.materialOverrides.clear();
    manifest.budgets.maxIndices = 48u;
    manifest.lods[0].maxTriangles = 16u;
    horde::scene::assets::StaticMeshAsset asset;
    std::string diagnostic;
    const auto closed = WriteClosedDielectricGlb(
        temporaryRoot, "closed-dielectric-lod0.runtime.glb", false, false);
    Check(horde::scene::assets::StaticMeshAsset::Load(closed, manifest, asset, diagnostic),
          std::string("closed manifold thick dielectric loads: ") + diagnostic);
    Check(asset.materials.size() == 1u &&
              (asset.materials[0].flags & 1024u) != 0u,
          "validated outward closed thick dielectric receives the runtime certification flag");

    const auto open = WriteClosedDielectricGlb(
        temporaryRoot, "open-dielectric-lod0.runtime.glb", true, false);
    ExpectAssetFailureContains(
        open, manifest,
        {"material 'ClosedGlass' component 1", "node 'Root' primitive 0",
         "boundary edge", "set thinWall"});

    const auto nonManifold = WriteClosedDielectricGlb(
        temporaryRoot, "non-manifold-dielectric-lod0.runtime.glb", false, true);
    ExpectAssetFailureContains(
        nonManifold, manifest,
        {"material 'ClosedGlass' component 1", "node 'Root' primitive 0",
         "referenced more than twice"});

    const auto inward = WriteClosedDielectricGlb(
        temporaryRoot, "inward-dielectric-lod0.runtime.glb", false, false,
        true, 1.0f, 1);
    ExpectAssetFailureContains(
        inward, manifest,
        {"material 'ClosedGlass' component 1", "node 'Root' primitive 0",
         "inward-wound after baked node transforms", "reverse every triangle winding"});

    const auto flipped = WriteClosedDielectricGlb(
        temporaryRoot, "single-face-flipped-dielectric-lod0.runtime.glb", false,
        false, true, 1.0f, 2);
    ExpectAssetFailureContains(
        flipped, manifest,
        {"material 'ClosedGlass' component 1", "node 'Root' primitive 0",
         "used once in each direction"});

    const auto transformed = WriteClosedDielectricGlb(
        temporaryRoot, "valid-transformed-dielectric-lod0.runtime.glb", false,
        false, true, 1.0f, 0,
        ",\"matrix\":[2,0,0,0,0,3,0,0,0,0,4,0,1,2,3,1]");
    const bool transformedLoaded = horde::scene::assets::StaticMeshAsset::Load(
        transformed, manifest, asset, diagnostic);
    Check(transformedLoaded,
          std::string("outward thick dielectric remains valid after positive baked transform: ") + diagnostic);

    const auto millimetre = WriteClosedDielectricGlb(
        temporaryRoot, "millimetre-dielectric-lod0.runtime.glb", false, false,
        true, 1.0f, 0,
        ",\"matrix\":[0.001,0,0,0,0,0.006,0,0,0,0,0.004,0,-9.1,-0.3,-15.2,1]");
    const bool millimetreLoaded = horde::scene::assets::StaticMeshAsset::Load(
        millimetre, manifest, asset, diagnostic);
    Check(millimetreLoaded,
          std::string("one-to-six millimetre closed dielectric remains valid after baking: ") + diagnostic);

    const auto negativeScale = WriteClosedDielectricGlb(
        temporaryRoot, "negative-scale-dielectric-lod0.runtime.glb", false,
        false, true, 1.0f, 0, ",\"scale\":[-1,1,1]");
    ExpectAssetFailure(
        negativeScale, manifest,
        "Static GLB node 'Root' has a negative-determinant transform; bake the reflection and reverse triangle winding/normals before runtime import.");
}

void TestBakedNodeTransformAndUnitScale(const std::filesystem::path& temporaryRoot)
{
    constexpr float kInvSqrtTwo = 0.70710678118f;
    auto parts = ReadGlbParts();
    for (std::size_t vertexIndex = 0u; vertexIndex < 4u; ++vertexIndex)
    {
        const std::size_t normalOffset = 48u + vertexIndex * 12u;
        WriteFloat(parts.binary, normalOffset, kInvSqrtTwo);
        WriteFloat(parts.binary, normalOffset + 4u, 0.0f);
        WriteFloat(parts.binary, normalOffset + 8u, kInvSqrtTwo);
        const std::size_t tangentOffset = 96u + vertexIndex * 16u;
        WriteFloat(parts.binary, tangentOffset, kInvSqrtTwo);
        WriteFloat(parts.binary, tangentOffset + 4u, 0.0f);
        WriteFloat(parts.binary, tangentOffset + 8u, -kInvSqrtTwo);
        WriteFloat(parts.binary, tangentOffset + 12u, 1.0f);
    }
    const std::string_view sourceMatrix =
        "\"matrix\":[1,0,0,0,0,1,0,0,0,0,1,0,4,5,6,1]";
    const std::string_view transformedMatrix =
        "\"matrix\":[2,0,0,0,0,0,3,0,0,-4,0,0,4,5,6,1]";
    const auto matrixOffset = parts.json.find(sourceMatrix);
    Check(matrixOffset != std::string::npos, "node transform mutation source exists");
    if (matrixOffset != std::string::npos)
        parts.json.replace(matrixOffset, sourceMatrix.size(), transformedMatrix);
    const auto transformedGlb = temporaryRoot / "rotated-nonuniform.glb";
    WriteGlb(transformedGlb, std::move(parts.json), std::move(parts.binary));

    const auto scaledManifestPath = RewriteManifest(
        temporaryRoot, "two-metres.manifest.json", "\"metresPerUnit\": 1.0", "\"metresPerUnit\": 2.0");
    horde::scene::assets::AssetManifest manifest;
    std::string diagnostic;
    Check(horde::scene::assets::AssetManifest::Load(scaledManifestPath, manifest, diagnostic),
          std::string("two-metre manifest loads: ") + diagnostic);
    horde::scene::assets::StaticMeshAsset asset;
    Check(horde::scene::assets::StaticMeshAsset::Load(
              transformedGlb, manifest, asset, diagnostic),
          std::string("rotated non-uniform GLB loads: ") + diagnostic);
    if (asset.vertices.size() >= 6u && asset.nodeTransforms.size() >= 3u && !asset.sockets.empty())
    {
        Check(asset.nodeTransforms[0].world[12] == 2.0f &&
                  asset.nodeTransforms[0].world[13] == 4.0f &&
                  asset.nodeTransforms[0].world[14] == 6.0f &&
                  asset.nodeTransforms[2].world[12] == 8.0f &&
                  asset.nodeTransforms[2].world[13] == 10.0f &&
                  asset.nodeTransforms[2].world[14] == 12.0f,
              "metresPerUnit scales every node world translation");
        Check(asset.sockets[0].world[12] == 2.0f &&
                  asset.sockets[0].world[13] == 4.0f &&
                  asset.sockets[0].world[14] == 6.0f,
              "metresPerUnit scales socket world translation");
        Check(asset.vertices[4].position == std::array<float, 4u>{{8.0f, 10.0f, 12.0f, 1.0f}} &&
                  asset.vertices[5].position == std::array<float, 4u>{{12.0f, 10.0f, 12.0f, 1.0f}},
              "rotation, non-uniform scale, translation, and unit scale bake into positions once");
        Check(NearlyEqual(asset.vertices[4].normal[0], 0.89442719f) &&
                  NearlyEqual(asset.vertices[4].normal[1], -0.44721359f) &&
                  NearlyEqual(asset.vertices[4].normal[2], 0.0f),
              "inverse-transpose produces the hand-checked non-uniform rotated normal");
        Check(NearlyEqual(asset.vertices[4].tangent[0], 0.44721359f) &&
                  NearlyEqual(asset.vertices[4].tangent[1], 0.89442719f) &&
                  NearlyEqual(asset.vertices[4].tangent[2], 0.0f),
              "node linear transform produces the hand-checked tangent");
    }
}

void TestExactLodSuffixSelection(const std::filesystem::path& temporaryRoot)
{
    const auto manifestPath = RewriteManifest(
        temporaryRoot, "colliding-lods.manifest.json",
        "\"name\": \"lod0\", \"maxTriangles\": 8",
        "\"name\": \"lod1\", \"maxTriangles\": 3 }, { \"name\": \"lod10\", \"maxTriangles\": 8");
    horde::scene::assets::AssetManifest manifest;
    std::string diagnostic;
    Check(horde::scene::assets::AssetManifest::Load(manifestPath, manifest, diagnostic),
          std::string("colliding LOD manifest loads: ") + diagnostic);

    const auto runtimeGlb = temporaryRoot / "fixture_lod10.runtime.glb";
    WriteBytes(runtimeGlb, ReadBytes(kFixtureRoot / "valid-multi.glb"));
    horde::scene::assets::StaticMeshAsset asset;
    const bool loaded = horde::scene::assets::StaticMeshAsset::Load(
        runtimeGlb, manifest, asset, diagnostic);
    Check(loaded,
          std::string("exact LOD suffix selects lod10 rather than substring lod1: ") + diagnostic);
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
    ExpectAssetFailure(badIndex, manifest, "Static GLB failed cgltf structural validation.");

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
        "Static GLB node 'Root' has a negative-determinant transform; bake the reflection and reverse triangle winding/normals before runtime import.");
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
    horde::scene::assets::StaticMeshAsset rejectedVertexAsset;
    Check(!horde::scene::assets::StaticMeshAsset::Load(
              kFixtureRoot / "valid-multi.glb", manifest, rejectedVertexAsset, diagnostic) &&
              diagnostic == "Static GLB exceeds manifest maxVertices capacity.",
          "vertex budget overflow is rejected with the capacity diagnostic");
    Check(rejectedVertexAsset.vertices.empty() && rejectedVertexAsset.indices.empty() &&
              rejectedVertexAsset.primitives.empty() && rejectedVertexAsset.materials.empty() &&
              rejectedVertexAsset.nodeTransforms.empty(),
          "geometry budgets are preflighted before material, transform, vertex, or index decoding");

    const auto primitiveManifest = RewriteManifest(
        temporaryRoot, "primitive-budget.manifest.json", "\"maxPrimitives\": 4", "\"maxPrimitives\": 1");
    Check(AssetManifest::Load(primitiveManifest, manifest, diagnostic), "primitive budget manifest loads");
    ExpectAssetFailure(kFixtureRoot / "valid-multi.glb", manifest,
                       "Static GLB exceeds manifest maxPrimitives capacity.");

    const auto twoMaterials = RewriteGlb(
        temporaryRoot, "material-overflow.glb", "}],\"meshes\"",
        "},{\"name\":\"UnusedMaterial\",\"pbrMetallicRoughness\":{\"baseColorFactor\":[1,1,1,1],\"metallicFactor\":0,\"roughnessFactor\":1}}],\"meshes\""
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
    const auto categoryLocalTexture = temporaryRoot / "category-local-texture.glb";
    WriteGlb(categoryLocalTexture, textureParts.json, textureParts.binary);
    const auto textureManifest = RewriteManifest(
        temporaryRoot, "texture-budget.manifest.json", "\"maxTextureLayersPerKind\": 4", "\"maxTextureLayersPerKind\": 1");
    Check(AssetManifest::Load(textureManifest, manifest, diagnostic), "texture budget manifest loads");
    horde::scene::assets::StaticMeshAsset boundaryAsset;
    Check(horde::scene::assets::StaticMeshAsset::Load(
              categoryLocalTexture, manifest, boundaryAsset, diagnostic),
          std::string("one category-local layer meets a capacity of one even at global texture index 1: ") + diagnostic);
    Check(!boundaryAsset.materials.empty() && boundaryAsset.materials[0].baseColorTexture == 0,
          "global glTF texture index 1 is remapped to category-local baseColor layer 0");

    const auto materialsOffset = textureParts.json.find("}],\"meshes\"");
    Check(materialsOffset != std::string::npos, "texture material overflow mutation source exists");
    if (materialsOffset != std::string::npos)
    {
        textureParts.json.replace(
            materialsOffset, std::string_view("}],\"meshes\"").size(),
            "},{\"name\":\"SecondBaseColor\",\"pbrMetallicRoughness\":{\"baseColorTexture\":{\"index\":0}}}],\"meshes\"");
    }
    const auto secondPrimitiveOffset = textureParts.json.find("\"indices\":5,\"material\":0");
    Check(secondPrimitiveOffset != std::string::npos, "second primitive material mutation source exists");
    if (secondPrimitiveOffset != std::string::npos)
    {
        textureParts.json.replace(secondPrimitiveOffset,
                                  std::string_view("\"indices\":5,\"material\":0").size(),
                                  "\"indices\":5,\"material\":1");
    }
    const auto textureOverflow = temporaryRoot / "texture-overflow.glb";
    WriteGlb(textureOverflow, std::move(textureParts.json), std::move(textureParts.binary));
    ExpectAssetFailure(textureOverflow, manifest,
                       "Static GLB exceeds manifest maxTextureLayersPerKind capacity for baseColor.");

    const auto indexManifest = RewriteManifest(
        temporaryRoot, "index-budget.manifest.json", "\"maxIndices\": 24", "\"maxIndices\": 11");
    Check(AssetManifest::Load(indexManifest, manifest, diagnostic), "index budget manifest loads");
    ExpectAssetFailure(kFixtureRoot / "valid-multi.glb", manifest,
                       "Static GLB exceeds manifest maxIndices capacity.");

    const auto lodManifest = RewriteManifest(
        temporaryRoot, "lod-budget.manifest.json", "\"maxTriangles\": 8", "\"maxTriangles\": 3");
    Check(AssetManifest::Load(lodManifest, manifest, diagnostic), "LOD budget manifest loads");
    ExpectAssetFailure(kFixtureRoot / "valid-multi.glb", manifest,
                       "Static GLB exceeds selected LOD 'lod0' maxTriangles capacity.");

    const auto socketManifestPath = RewriteManifest(
        temporaryRoot, "socket.manifest.json", "[\"grip\"]", "[\"blade_grip\"]");
    Check(AssetManifest::Load(socketManifestPath, manifest, diagnostic), "missing socket manifest loads");
    ExpectAssetFailure(kFixtureRoot / "valid-multi.glb", manifest,
                       "Static GLB is missing required socket 'blade_grip'.");

    (void)validManifest;
}

void TestProductionDielectricFixture()
{
    horde::scene::assets::AssetManifest manifest;
    horde::scene::assets::StaticMeshAsset asset;
    std::string diagnostic;
    Check(horde::scene::assets::AssetManifest::Load(
              kDielectricFixtureRoot / "asset.manifest.json", manifest, diagnostic),
          std::string("production dielectric manifest loads: ") + diagnostic);
    Check(horde::scene::assets::StaticMeshAsset::Load(
              kDielectricFixtureRoot / "closed-glass-lod0.runtime.glb",
              manifest, asset, diagnostic),
          std::string("closed/manifold production dielectric fixture loads: ") + diagnostic);
    Check(asset.primitives.size() == 1u && asset.materials.size() == 1u &&
              asset.indices.size() == 36u && asset.materials[0].transmissionFactor > 0.9f &&
              asset.materials[0].thicknessFactor > 0.0f && asset.materials[0].ior > 1.0f,
          "production dielectric fixture keeps one closed twelve-triangle volume and KHR material properties");
}

void TestRuntimeOfflineDielectricComponentParity()
{
    using horde::scene::assets::AssetManifest;
    AssetManifest manifest;
    AssetManifest centimetreManifest;
    AssetManifest tenMetreManifest;
    horde::scene::assets::StaticMeshAsset asset;
    std::string diagnostic;
    const std::filesystem::path fixtureRoot =
        kFixtureRoot.parent_path() / "dielectric-topology";
    Check(horde::scene::assets::AssetManifest::Load(
              fixtureRoot / "asset.manifest.json",
              manifest, diagnostic),
          std::string("dielectric topology parity manifest loads: ") + diagnostic);
    diagnostic.clear();
    Check(AssetManifest::Load(fixtureRoot / "centimetre-units.manifest.json",
                              centimetreManifest, diagnostic),
          std::string("centimetre topology parity manifest loads: ") + diagnostic);
    diagnostic.clear();
    Check(AssetManifest::Load(fixtureRoot / "ten-metre-units.manifest.json",
                              tenMetreManifest, diagnostic),
          std::string("ten-metre topology parity manifest loads: ") + diagnostic);

    const auto expectPass = [&](std::string_view name, const AssetManifest& selectedManifest) {
        diagnostic.clear();
        const bool loaded = horde::scene::assets::StaticMeshAsset::Load(
            fixtureRoot / name, selectedManifest, asset, diagnostic);
        Check(loaded,
              std::string("runtime/offline parity fixture must pass: ") +
                  std::string(name) + ": " + diagnostic);
    };
    const auto expectDefaultPass = [&](std::string_view name) {
        expectPass(name, manifest);
    };
    expectDefaultPass("closed-dielectric-lod0.runtime.glb");
    expectDefaultPass("split-shell-dielectric-lod0.runtime.glb");
    expectDefaultPass("disconnected-shells-dielectric-lod0.runtime.glb");
    expectDefaultPass("valid-transformed-dielectric-lod0.runtime.glb");
    expectDefaultPass("millimetre-dielectric-lod0.runtime.glb");
    expectPass("non-unit-min-seam-dielectric-lod0.runtime.glb", centimetreManifest);
    expectPass("non-unit-min-seam-reordered-dielectric-lod0.runtime.glb", centimetreManifest);
    expectPass("non-unit-min-scale-seam-dielectric-lod0.runtime.glb", centimetreManifest);
    expectPass("non-unit-max-disconnected-dielectric-lod0.runtime.glb", tenMetreManifest);
    expectDefaultPass("large-trs-matrix-seam-dielectric-lod0.runtime.glb");
    Check(asset.nodeTransforms.size() == 4u &&
              asset.nodeTransforms[1].world == asset.nodeTransforms[3].world,
          "cgltf-equivalent parent/child TRS and matrix paths produce bit-identical runtime worlds");
    expectDefaultPass("weld-domain-upper-inside-dielectric-lod0.runtime.glb");
    expectDefaultPass("weld-domain-lower-inside-dielectric-lod0.runtime.glb");

    ExpectAssetFailureContains(
        fixtureRoot / "open-dielectric-lod0.runtime.glb", manifest,
        {"material 'GenericClosedGlass' component 1",
         "node 'GenericDielectricFixture' primitive 0", "boundary edge"});
    ExpectAssetFailureContains(
        fixtureRoot / "non-manifold-dielectric-lod0.runtime.glb", manifest,
        {"material 'GenericClosedGlass' component 1",
         "node 'GenericDielectricFixture' primitive 0", "referenced more than twice"});
    ExpectAssetFailureContains(
        fixtureRoot / "inward-dielectric-lod0.runtime.glb", manifest,
        {"material 'GenericClosedGlass' component 1",
         "node 'GenericDielectricFixture' primitive 0", "inward-wound"});
    ExpectAssetFailureContains(
        fixtureRoot / "single-face-flipped-dielectric-lod0.runtime.glb", manifest,
        {"material 'GenericClosedGlass' component 1",
         "node 'GenericDielectricFixture' primitive 0", "used once in each direction"});
    ExpectAssetFailureContains(
        fixtureRoot / "mixed-orientation-shells-dielectric-lod0.runtime.glb", manifest,
        {"material 'GenericClosedGlass' component 2",
         "node 'SmallInwardShell' primitive 1", "inward-wound"});
    ExpectAssetFailureContains(
        fixtureRoot / "split-flipped-face-dielectric-lod0.runtime.glb", manifest,
        {"material 'GenericClosedGlass' component 1",
         "node 'SplitFlippedA' primitive 0", "node 'SplitFlippedB' primitive 1",
         "used once in each direction"});
    ExpectAssetFailureContains(
        fixtureRoot / "non-unit-min-diagonal-dielectric-lod0.runtime.glb",
        centimetreManifest,
        {"material 'GenericClosedGlass' component 2", "boundary edge"});
    ExpectAssetFailureContains(
        fixtureRoot / "non-unit-max-scale-seam-dielectric-lod0.runtime.glb",
        tenMetreManifest,
        {"material 'GenericClosedGlass' component 2", "boundary edge"});
    ExpectAssetFailureContains(
        fixtureRoot / "mixed-orientation-shells-reordered-dielectric-lod0.runtime.glb",
        manifest,
        {"material 'GenericClosedGlass' component 2", "SmallInwardShell",
         "inward-wound"});
    for (const std::string_view name : {
             "position-nan-dielectric-lod0.runtime.glb",
             "position-positive-infinity-dielectric-lod0.runtime.glb",
             "position-negative-infinity-dielectric-lod0.runtime.glb",
             "weld-domain-upper-at-dielectric-lod0.runtime.glb",
             "weld-domain-upper-outside-dielectric-lod0.runtime.glb",
             "weld-domain-lower-at-dielectric-lod0.runtime.glb",
             "weld-domain-lower-outside-dielectric-lod0.runtime.glb"})
    {
        ExpectAssetFailureContains(
            fixtureRoot / name, manifest,
            {"material 'GenericClosedGlass'", "node 'GenericDielectricFixture'",
             "primitive 0", "finite deterministic dielectric weld domain"});
    }
    ExpectAssetFailure(
        fixtureRoot / "negative-scale-dielectric-lod0.runtime.glb", manifest,
        "Static GLB node 'GenericDielectricFixture' has a negative-determinant transform; bake the reflection and reverse triangle winding/normals before runtime import.");

    AssetManifest rejectedManifest;
    diagnostic.clear();
    Check(!AssetManifest::Load(fixtureRoot / "zero-units.manifest.json",
                               rejectedManifest, diagnostic) &&
              diagnostic == "Asset manifest metresPerUnit must be finite and greater than zero.",
          "runtime rejects zero metresPerUnit before dielectric topology validation");
    diagnostic.clear();
    Check(!AssetManifest::Load(fixtureRoot / "nonfinite-float-units.manifest.json",
                               rejectedManifest, diagnostic) &&
              diagnostic == "Asset manifest metresPerUnit must be finite and greater than zero.",
          "runtime rejects metresPerUnit outside the finite float range before dielectric topology validation");
    diagnostic.clear();
    Check(!AssetManifest::Load(fixtureRoot / "underflow-float-units.manifest.json",
                               rejectedManifest, diagnostic) &&
              diagnostic == "Asset manifest metresPerUnit must be finite and greater than zero.",
          "runtime rejects positive metresPerUnit that underflows to float32 zero");
    diagnostic.clear();
    Check(!AssetManifest::Load(fixtureRoot / "nan-units.manifest.json",
                               rejectedManifest, diagnostic) &&
              diagnostic == "Asset manifest JSON is invalid.",
          "runtime rejects non-JSON NaN metresPerUnit before schema conversion");
}

void TestExactDielectricWeldCellDomain()
{
    const std::filesystem::path fixture =
        kFixtureRoot.parent_path() / "dielectric-topology" /
        "weld-cell-domain-cases.txt";
    std::ifstream input(fixture);
    Check(static_cast<bool>(input), "exact weld-cell domain case file opens");
    std::string line;
    while (std::getline(input, line))
    {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream fields(line);
        std::string name;
        std::string hexadecimal;
        std::string expectedText;
        fields >> name >> hexadecimal >> expectedText;
        std::size_t parsed = 0u;
        const double scaled = std::stod(hexadecimal, &parsed);
        Check(parsed == hexadecimal.size(), name + ": exact hexadecimal value parses");
        std::int64_t coordinate = 0;
        const bool accepted =
            horde::scene::assets::detail::DielectricWeldCellCoordinate(
                scaled, coordinate);
        const bool expected = expectedText == "true";
        Check(accepted == expected,
              name + ": exact exclusive +/-2^63 weld-cell acceptance matches");
        if (accepted)
        {
            Check(coordinate == static_cast<std::int64_t>(std::floor(scaled)),
                  name + ": accepted coordinate retains floor semantics");
        }
    }
}

} // namespace

int main(int argc, char** argv)
{
    executablePath = std::filesystem::absolute(argv[0]);
#if defined(_WIN32)
    const char* cyclicEnvironment = std::getenv("HORDE_TEST_CYCLIC_PATH");
    const char* manifestEnvironment = std::getenv("HORDE_TEST_CYCLIC_MANIFEST");
    if (cyclicEnvironment != nullptr && cyclicEnvironment[0] != '\0' &&
        manifestEnvironment != nullptr && manifestEnvironment[0] != '\0')
    {
        horde::scene::assets::AssetManifest manifest;
        horde::scene::assets::StaticMeshAsset asset;
        std::string diagnostic;
        if (!horde::scene::assets::AssetManifest::Load(
                manifestEnvironment, manifest, diagnostic)) return 3;
        const bool loaded = horde::scene::assets::StaticMeshAsset::Load(
            cyclicEnvironment, manifest, asset, diagnostic);
        const bool safeDiagnostic =
            diagnostic == "Static GLB failed cgltf structural validation." ||
            diagnostic == "Static GLB contains an out-of-range accessor or index reference.";
        return !loaded && safeDiagnostic ? 0 : 2;
    }
#endif
    if (argc == 4 && std::string_view(argv[1]) == "--probe-cyclic")
    {
        horde::scene::assets::AssetManifest manifest;
        horde::scene::assets::StaticMeshAsset asset;
        std::string diagnostic;
        if (!horde::scene::assets::AssetManifest::Load(argv[3], manifest, diagnostic)) return 3;
        const bool loaded = horde::scene::assets::StaticMeshAsset::Load(
            argv[2], manifest, asset, diagnostic);
        return !loaded && diagnostic == "Static GLB failed cgltf structural validation." ? 0 : 2;
    }

    const ScopedStaticGltfTestDirectory scopedTemporaryRoot;
    Check(scopedTemporaryRoot.Ready(),
          "process/config-unique static GLB temporary directory is safely created");
    if (!scopedTemporaryRoot.Ready()) return 1;
    const auto& temporaryRoot = scopedTemporaryRoot.Path();
    std::cout << "Static GLB temporary root="
              << temporaryRoot.filename().string() << '\n';

    TestManifestContract(temporaryRoot);
    TestAccessorRangeRejectsOverflow();
    TestCyclicNodeGraphIsRejectedBeforeTraversal(temporaryRoot);
    TestExactDielectricWeldCellDomain();
    TestProductionDielectricFixture();
    TestRuntimeOfflineDielectricComponentParity();

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
        TestUnsupportedAlphaMaterialIsRejected(temporaryRoot, manifest);
        TestTransmissionDefaultThinWallSemantics(temporaryRoot, manifest);
        TestThickDielectricTopology(temporaryRoot, manifest);
        TestBakedNodeTransformAndUnitScale(temporaryRoot);
        TestExactLodSuffixSelection(temporaryRoot);
        TestMalformedAndUnsupportedGlbs(temporaryRoot, manifest);
        TestBudgetFailures(temporaryRoot, manifest);
    }

    if (failures != 0)
    {
        std::cerr << failures << " static glTF asset assertion(s) failed\n";
        return 1;
    }
    std::cout << "Static GLB asset contract passed\n";
    return 0;
}
