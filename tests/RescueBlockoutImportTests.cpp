#include "scene/RescueBlockoutGeometry.h"
#include "scene/RescueLidFinish.h"
#include "scene/assets/AssetManifest.h"
#include "scene/assets/StaticMeshAsset.h"
#include "gameplay/traversal/RescueTraversal.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace
{
using namespace horde::scene;

int failures = 0;

void Check(bool condition, std::string_view message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

std::vector<std::uint8_t> Read(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::uint32_t U32(const std::vector<std::uint8_t>& bytes, std::size_t at)
{
    return static_cast<std::uint32_t>(bytes.at(at)) |
        (static_cast<std::uint32_t>(bytes.at(at + 1u)) << 8u) |
        (static_cast<std::uint32_t>(bytes.at(at + 2u)) << 16u) |
        (static_cast<std::uint32_t>(bytes.at(at + 3u)) << 24u);
}

void AppendU32(std::vector<std::uint8_t>& bytes, std::uint32_t value)
{
    for (unsigned shift = 0u; shift < 32u; shift += 8u)
        bytes.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffu));
}

void Write(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
}

std::filesystem::path Mutate(const std::filesystem::path& root,
                             const std::filesystem::path& source,
                             std::string_view name,
                             std::string_view before,
                             std::string_view after)
{
    auto bytes = Read(source);
    if (bytes.size() < 20u || U32(bytes, 0u) != 0x46546c67u)
    {
        Check(false, "roundtrip source is a GLB 2.0 file");
        return {};
    }
    const std::uint32_t jsonLength = U32(bytes, 12u);
    const auto jsonEnd = 20u + static_cast<std::size_t>(jsonLength);
    std::string json(bytes.begin() + 20, bytes.begin() + static_cast<std::ptrdiff_t>(jsonEnd));
    const auto where = json.find(before);
    Check(where != std::string::npos, std::string("mutation source exists for ") + std::string(name));
    if (where == std::string::npos) return {};
    json.replace(where, before.size(), after);
    while ((json.size() & 3u) != 0u) json.push_back(' ');

    std::vector<std::uint8_t> result;
    AppendU32(result, 0x46546c67u);
    AppendU32(result, 2u);
    const std::uint32_t binaryHeader = U32(bytes, static_cast<std::size_t>(20u + jsonLength));
    const std::uint32_t binaryLength = binaryHeader;
    const std::size_t binaryChunkHeader = jsonEnd;
    const std::size_t binaryStart = binaryChunkHeader + 8u;
    AppendU32(result, static_cast<std::uint32_t>(12u + 8u + json.size() + 8u + binaryLength));
    AppendU32(result, static_cast<std::uint32_t>(json.size()));
    AppendU32(result, 0x4e4f534au);
    result.insert(result.end(), json.begin(), json.end());
    AppendU32(result, binaryLength);
    AppendU32(result, 0x004e4942u);
    result.insert(result.end(), bytes.begin() + static_cast<std::ptrdiff_t>(binaryStart),
                  bytes.begin() + static_cast<std::ptrdiff_t>(binaryStart + binaryLength));
    const auto path = root / name;
    Write(path, result);
    return path;
}

bool Load(const std::filesystem::path& glb,
          const horde::scene::assets::AssetManifest& manifest,
          horde::scene::assets::StaticMeshAsset* output,
          std::string& diagnostic)
{
    horde::scene::assets::StaticMeshAsset local;
    return horde::scene::assets::StaticMeshAsset::Load(
        glb, manifest, output != nullptr ? *output : local, diagnostic);
}

void ExpectRejected(const std::filesystem::path& glb,
                    const horde::scene::assets::AssetManifest& manifest,
                    std::string_view reason)
{
    std::string diagnostic;
    Check(!Load(glb, manifest, nullptr, diagnostic), reason);
    if (diagnostic.empty()) Check(false, "rejected GLB reports the native importer diagnostic");
}

bool Near(float a, float b)
{
    return std::abs(a - b) < 0.002f;
}

using QuantizedPoint = std::array<int, 3u>;

QuantizedPoint Quantize(const std::array<float, 4u>& position)
{
    return {{static_cast<int>(std::lround(position[0] * 1000.0f)),
             static_cast<int>(std::lround(position[1] * 1000.0f)),
             static_cast<int>(std::lround(position[2] * 1000.0f))}};
}

std::vector<RescueBlockoutBox> AllRecipeBoxes()
{
    std::vector<RescueBlockoutBox> result(
        kRescueBlockoutBoxes.begin(), kRescueBlockoutBoxes.end());
    result.push_back(kRescueBlockoutLid);
    result.push_back(kRescueBlockoutLanding);
    return result;
}

bool TriangleMatchesBoxFace(const std::array<QuantizedPoint, 3u>& triangle,
                            const RescueBlockoutBox& box,
                            std::size_t axis,
                            float plane)
{
    const int planeMm = static_cast<int>(std::lround(plane * 1000.0f));
    const std::array<int, 3u> low{{
        static_cast<int>(std::lround(box.minimum[0] * 1000.0f)),
        static_cast<int>(std::lround(box.minimum[1] * 1000.0f)),
        static_cast<int>(std::lround(box.minimum[2] * 1000.0f))}};
    const std::array<int, 3u> high{{
        static_cast<int>(std::lround(box.maximum[0] * 1000.0f)),
        static_cast<int>(std::lround(box.maximum[1] * 1000.0f)),
        static_cast<int>(std::lround(box.maximum[2] * 1000.0f))}};
    for (const auto& point : triangle)
    {
        if (point[axis] != planeMm) return false;
        for (std::size_t other = 0u; other < 3u; ++other)
            if (other != axis && point[other] != low[other] && point[other] != high[other])
                return false;
    }
    const int ab0 = triangle[1][(axis + 1u) % 3u] - triangle[0][(axis + 1u) % 3u];
    const int ab1 = triangle[1][(axis + 2u) % 3u] - triangle[0][(axis + 2u) % 3u];
    const int ac0 = triangle[2][(axis + 1u) % 3u] - triangle[0][(axis + 1u) % 3u];
    const int ac1 = triangle[2][(axis + 2u) % 3u] - triangle[0][(axis + 2u) % 3u];
    return ab0 * ac1 - ab1 * ac0 != 0;
}

void CheckRecipeEquivalence(const horde::scene::assets::StaticMeshAsset& asset, const std::vector<RescueBlockoutBox>& boxes=AllRecipeBoxes())
{
    for (const auto& primitive : asset.primitives)
    {
        if (primitive.materialIndex >= asset.materials.size())
        {
            Check(false, "native primitive material is in range for recipe comparison");
            continue;
        }
        const auto& material = asset.materials[primitive.materialIndex];
        const unsigned materialCode = material.name.starts_with("MossyStone") ? 2u :
                                      material.name.starts_with("DryStone") ? 0u :
                                      material.name.starts_with("AgedMetal") ? 4u : 99u;
        Check(materialCode != 99u, "native material name maps to a recorded world material code");
        if (materialCode == 99u) continue;

        std::vector<QuantizedPoint> expected;
        std::vector<std::size_t> sourceBoxIndices;
        for (std::size_t boxIndex = 0u; boxIndex < boxes.size(); ++boxIndex)
            if (boxes[boxIndex].materialCode == materialCode)
                sourceBoxIndices.push_back(boxIndex);
        for (const std::size_t boxIndex : sourceBoxIndices)
        {
            const auto& box = boxes[boxIndex];
            for (unsigned corner = 0u; corner < 8u; ++corner)
            {
                QuantizedPoint point{};
                for (std::size_t axis = 0u; axis < 3u; ++axis)
                {
                    const float value = (corner & (1u << axis)) != 0u
                        ? box.maximum[axis] : box.minimum[axis];
                    point[axis] = static_cast<int>(std::lround(value * 1000.0f));
                }
                expected.insert(expected.end(), 3u, point);
            }
        }
        std::vector<QuantizedPoint> actual;
        const std::size_t vertexCount = sourceBoxIndices.size() * 24u;
        for (std::size_t i = primitive.vertexOffset;
             i < primitive.vertexOffset + vertexCount && i < asset.vertices.size(); ++i)
        {
            actual.push_back(Quantize(asset.vertices[i].position));
        }
        std::sort(expected.begin(), expected.end());
        std::sort(actual.begin(), actual.end());
        Check(actual == expected,
              "native imported vertices exactly reproduce the shared box recipe within 1 mm");

        Check(primitive.indexCount == sourceBoxIndices.size() * 36u,
              "native imported triangle index count is 12 triangles per recipe box");
        std::vector<std::array<unsigned, 6u>> faceTriangleCounts(sourceBoxIndices.size());
        bool everyTriangleMatches = true;
        for (std::size_t index = 0u; index + 2u < primitive.indexCount; index += 3u)
        {
            std::array<QuantizedPoint, 3u> triangle{};
            for (std::size_t corner = 0u; corner < 3u; ++corner)
            {
                const std::size_t vertexIndex = primitive.vertexOffset +
                    asset.indices[primitive.indexOffset + index + corner];
                triangle[corner] = Quantize(asset.vertices.at(vertexIndex).position);
            }
            bool matched = false;
            for (std::size_t sourceBox = 0u; sourceBox < sourceBoxIndices.size() && !matched;
                 ++sourceBox)
            {
                const auto& box = boxes[sourceBoxIndices[sourceBox]];
                for (std::size_t axis = 0u; axis < 3u && !matched; ++axis)
                {
                    for (unsigned side = 0u; side < 2u && !matched; ++side)
                    {
                        const std::size_t face = axis * 2u + side;
                        const float plane = side == 0u ? box.minimum[axis] : box.maximum[axis];
                        if (faceTriangleCounts[sourceBox][face] < 2u &&
                            TriangleMatchesBoxFace(triangle, box, axis, plane))
                        {
                            ++faceTriangleCounts[sourceBox][face];
                            matched = true;
                        }
                    }
                }
            }
            everyTriangleMatches = everyTriangleMatches && matched;
        }
        Check(everyTriangleMatches,
              "native imported triangle indices cover recipe box faces without non-box triangles");
        for (const auto& faceCounts : faceTriangleCounts)
            for (const unsigned count : faceCounts)
                Check(count == 2u, "each recipe box face has its two indexed triangles");
    }
}

} // namespace

int main()
{
    using namespace horde::scene;
    using namespace horde::scene::assets;
    const std::filesystem::path sourceRoot{HORDE_RT_SOURCE_DIR};
    const auto assetRoot = sourceRoot / "assets/source/development-rescue-ground-stake-v1";
    const auto roundtrip = assetRoot / "rescue-shaft-roundtrip.glb";
    AssetManifest manifest;
    std::string diagnostic;
    Check(AssetManifest::Load(assetRoot / "asset.manifest.json", manifest, diagnostic),
          std::string("source manifest loads through the native manifest reader: ") + diagnostic);
    const auto& anchor = horde::gameplay::traversal::kAnchor;
    const auto& stake = kRescueBlockoutBoxes[8u];
    Check(kRescueBlockoutBoxes.size() == 9u &&
          std::all_of(kRescueBlockoutBoxes.begin(), kRescueBlockoutBoxes.begin() + 4,
              [](const RescueBlockoutBox& box) {
                  return box.minimum[1] == 1.30f && box.maximum[1] == 2.05f &&
                         box.materialCode == 2u;
              }) &&
          Near(stake.minimum[1],1.75f) && Near(stake.maximum[1],2.73f) &&
          stake.materialCode==4u && stake.minimum[1]<kRescueBlockoutLanding.maximum[1] &&
          RescueLandingContains(stake.minimum[0],stake.minimum[2]) &&
          RescueLandingContains(stake.maximum[0],stake.maximum[2]) &&
          Near(anchor.x,-33.4f) && Near(anchor.y,2.65f) && Near(anchor.z,-14.979f) &&
          anchor.x>=stake.minimum[0] && anchor.x<=stake.maximum[0] &&
          anchor.y>=stake.minimum[1] && anchor.y<=stake.maximum[1] &&
          anchor.z+.04f<stake.minimum[2] &&
          kRescueBlockoutLid.minimum[1] == 2.05f &&
          kRescueBlockoutLid.maximum[1] == 2.19f &&
          kRescueBlockoutLanding.minimum[1] == 1.92f &&
          kRescueBlockoutLanding.maximum[1] == 2.05f &&
          Near(kRescueBlockoutLanding.minimum[2], -14.95f) &&
          Near(kRescueBlockoutLandingCollision.minZ, -14.95f) &&
          RescueLandingContains(-33.7f, -14.94f) &&
          !RescueLandingContains(-33.7f, -15.0f),
          "shared recipe preserves the shaft, lid and apron with a buried short stake and open overhead clearance");

    StaticMeshAsset imported;
    const bool loaded = Load(roundtrip, manifest, &imported, diagnostic);
    Check(loaded, std::string("Blender reimported GLB loads through native cgltf importer: ") + diagnostic);
    if (loaded)
    {
        Check(imported.primitives.size() == 3u && imported.materials.size() == 3u,
              "roundtrip retains the three existing opaque material-code groups");
        Check(imported.vertices.size() == 264u && imported.indices.size() == 396u,
              "roundtrip retains bounded cube-face geometry and real indices");
        CheckRecipeEquivalence(imported);
        bool hasDryStone = false;
        bool hasMossyStone = false;
        bool hasAgedMetal = false;
        for (const auto& material : imported.materials)
        {
            hasDryStone = hasDryStone || material.name.starts_with("DryStone");
            hasMossyStone = hasMossyStone || material.name.starts_with("MossyStone");
            hasAgedMetal = hasAgedMetal || material.name.starts_with("AgedMetal");
            Check(material.baseColorTexture < 0 && material.normalTexture < 0 &&
                  material.ormTexture < 0 && material.emissiveTexture < 0,
                  "existing material codes do not add or reference texture layers");
            Check((material.flags & 2u) == 0u,
                  "roundtrip material remains opaque under the existing static importer policy");
        }
        Check(hasDryStone && hasMossyStone && hasAgedMetal,
              "GLB retains names for the three existing world material codes");
        Check(Near(imported.bounds.minimum[0], -35.12f) &&
              Near(imported.bounds.maximum[0], -32.28f) &&
              Near(imported.bounds.minimum[1], 1.30f) &&
              Near(imported.bounds.maximum[1], 2.73f) &&
              Near(imported.bounds.minimum[2], -16.82f) &&
              Near(imported.bounds.maximum[2], -11.92f),
              "native transformed bounds remain in the authored +Y world coordinate system");
        Check(RescueLandingContains(-33.7f, -12.8f) &&
              kRescueBlockoutLandingCollision.topY == 2.05f &&
              !RescueLandingContains(-33.7f, -11.7f),
              "shared landing collision contains the existing upper landing and excludes the edge");
    }

    const auto lidSource=sourceRoot / "assets/source/development-rescue-lid-finish-v1";
    AssetManifest lidManifest; StaticMeshAsset finishedLid;
    Check(AssetManifest::Load(lidSource / "asset.manifest.json",lidManifest,diagnostic) &&
        Load(lidSource / "rescue-lid-finish-roundtrip.glb",lidManifest,&finishedLid,diagnostic),
        std::string("original finished lid passes the Blender/native round trip: ")+diagnostic);
    Check(finishedLid.primitives.size()==6 && finishedLid.indices.size()==216 &&
        finishedLid.materials.size()==1,"finished lid adds only 72 opaque triangles, one existing stone family");
    const auto pieces=RescueLidFinishBoxes();
    if(finishedLid.primitives.size()==pieces.size())for(std::size_t i=0;i<pieces.size();++i) {
        StaticMeshAsset part=finishedLid; auto primitive=finishedLid.primitives[i];
        const auto end=i+1<pieces.size()?finishedLid.primitives[i+1].vertexOffset:finishedLid.vertices.size();
        part.vertices.assign(finishedLid.vertices.begin()+primitive.vertexOffset,finishedLid.vertices.begin()+end);
        part.indices.assign(finishedLid.indices.begin()+primitive.indexOffset,
            finishedLid.indices.begin()+primitive.indexOffset+primitive.indexCount);
        primitive.vertexOffset=0;primitive.indexOffset=0;part.primitives={primitive};
        CheckRecipeEquivalence(part,{pieces[i]});
    }

    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto temporary = std::filesystem::temp_directory_path() /
        ("horde-rescue-blockout-import-tests-" + std::to_string(unique));
    std::error_code error;
    std::filesystem::create_directories(temporary, error);
    Check(!error, "bounded test scratch directory is available");
    if (!error)
    {
        const auto invalidBounds = Mutate(temporary, roundtrip, "invalid-bounds.glb",
            "\"translation\":[-34.96500015258789,1.6749999523162842,-15.199999809265137]",
            "\"translation\":[1e30,1.6749999523162842,-15.199999809265137]");
        ExpectRejected(invalidBounds, manifest,
                       "native importer rejects transformed bounds outside its finite geometry domain");

        const auto textureTable = Mutate(temporary, roundtrip, "texture-table.glb",
            "\"asset\":{",
            "\"textures\":[{\"source\":0}],\"images\":[{\"uri\":\"missing.png\"}],\"asset\":{");
        const auto missingTexture = Mutate(temporary, textureTable, "missing-texture-layer.glb",
            "\"pbrMetallicRoughness\":{",
            "\"pbrMetallicRoughness\":{\"baseColorTexture\":{\"index\":0},");
        auto noTextureLayers = manifest;
        noTextureLayers.budgets.maxTextureLayersPerKind = 0u;
        ExpectRejected(missingTexture, noTextureLayers,
                       "native importer rejects a referenced texture with no admitted runtime texture layer");

        const auto alphaCutout = Mutate(temporary, roundtrip, "alpha-cutout.glb",
            "\"doubleSided\":true,",
            "\"alphaMode\":\"MASK\",\"doubleSided\":true,");
        ExpectRejected(alphaCutout, manifest,
                       "native importer preserves its OPAQUE-only alpha-cutout rejection");

        const auto unlit = Mutate(temporary, roundtrip, "unlit.glb",
            "\"asset\":{",
            "\"extensionsRequired\":[\"KHR_materials_unlit\"],\"asset\":{");
        ExpectRejected(unlit, manifest,
                       "native importer rejects required KHR_materials_unlit material semantics");
        const auto tempRoot = std::filesystem::temp_directory_path().lexically_normal();
        if (temporary.parent_path().lexically_normal() == tempRoot &&
            temporary.filename().string().starts_with("horde-rescue-blockout-import-tests-"))
            std::filesystem::remove_all(temporary, error);
    }
    return failures == 0 ? 0 : 1;
}
