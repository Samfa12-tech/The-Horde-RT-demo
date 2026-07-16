#include "scene/SkeletonBipedModel.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace
{

std::filesystem::path FindRepoRoot()
{
    std::filesystem::path candidate = std::filesystem::current_path();
    for (int depth = 0; depth < 6; ++depth)
    {
        if (std::filesystem::exists(candidate / "assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb")) return candidate;
        if (!candidate.has_parent_path()) break;
        candidate = candidate.parent_path();
    }
    return {};
}

bool Require(bool condition, const char* message)
{
    if (condition) return true;
    std::cerr << "FAIL: " << message << '\n';
    return false;
}

bool FiniteTexturedVertices(const std::vector<horde::scene::TexturedSkinnedRtVertex>& vertices)
{
    if (vertices.empty()) return false;
    bool sawDistinctUv = false;
    const float firstU = vertices.front().texcoord[0];
    const float firstV = vertices.front().texcoord[1];
    for (const auto& vertex : vertices)
    {
        for (float value : vertex.position) if (!std::isfinite(value)) return false;
        for (float value : vertex.normal) if (!std::isfinite(value)) return false;
        for (float value : vertex.texcoord) if (!std::isfinite(value)) return false;
        sawDistinctUv = sawDistinctUv || std::abs(vertex.texcoord[0] - firstU) > 0.0001f || std::abs(vertex.texcoord[1] - firstV) > 0.0001f;
        if (vertex.texcoord[2] != 0.0f || vertex.texcoord[3] != 0.0f) return false;
    }
    return sawDistinctUv;
}

} // namespace

int main()
{
    using namespace horde::scene;
    static_assert(sizeof(SkinnedRtVertex) == 32u);
    static_assert(sizeof(TexturedSkinnedRtVertex) == 48u);

    const std::filesystem::path root = FindRepoRoot();
    if (!Require(!root.empty(), "repo assets were not found")) return 1;

    std::string diagnostic;
    SkinnedCharacterModel skeleton;
    const auto skeletonPath = root / "assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb";
    if (!Require(skeleton.LoadCombatClips(skeletonPath.string(), diagnostic), diagnostic.c_str())) return 1;
    if (!Require(skeleton.HasTexcoords(), "skeleton TEXCOORD_0 was not imported")) return 1;
    if (!Require(skeleton.ExpandedVertexCount() == 28206u, "skeleton expanded vertex count changed")) return 1;

    std::vector<SkinnedRtVertex> skeletonPlain;
    std::vector<TexturedSkinnedRtVertex> skeletonTextured;
    if (!Require(skeleton.Skin(SkinnedClip::Idle, 0.25f, skeletonPlain, diagnostic), diagnostic.c_str())) return 1;
    if (!Require(skeleton.SkinTextured(SkinnedClip::Idle, 0.25f, skeletonTextured, diagnostic), diagnostic.c_str())) return 1;
    if (!Require(skeletonPlain.size() == skeletonTextured.size(), "textured skinning changed skeleton vertex count")) return 1;
    if (!Require(FiniteTexturedVertices(skeletonTextured), "skeleton textured vertices are invalid")) return 1;

    SkinnedCharacterModel lich;
    const auto lichPath = root / "assets/models/enemies/meshy/lich_placeholder_merged_animations_v01.glb";
    if (!Require(lich.LoadClips(lichPath.string(), LichPlaceholderClipSet(), diagnostic), diagnostic.c_str())) return 1;
    if (!Require(lich.HasTexcoords(), "lich TEXCOORD_0 was not imported")) return 1;
    if (!Require(lich.ExpandedVertexCount() == 27564u, "lich expanded vertex count changed")) return 1;
    if (!Require(lich.ClipDuration(SkinnedClip::Idle) > 2.3f, "lich idle mapping is wrong")) return 1;
    if (!Require(lich.ClipDuration(SkinnedClip::Walking) > 1.0f, "lich walking mapping is wrong")) return 1;
    if (!Require(lich.ClipDuration(SkinnedClip::Attack) == 0.0f, "lich attack must remain explicitly unmapped")) return 1;
    if (!Require(lich.ClipDuration(SkinnedClip::Dead) > 2.9f, "lich death mapping is wrong")) return 1;

    std::vector<TexturedSkinnedRtVertex> lichTextured;
    if (!Require(lich.SkinTextured(SkinnedClip::Idle, 0.5f, lichTextured, diagnostic), diagnostic.c_str())) return 1;
    if (!Require(FiniteTexturedVertices(lichTextured), "lich textured vertices are invalid")) return 1;
    std::ifstream emissionFile(root / "assets/textures/meshy/lich_placeholder_v01/emissive-2048-rgba8.ktx2",
                               std::ios::binary | std::ios::ate);
    if (!Require(static_cast<bool>(emissionFile), "derived lich emission audit KTX2 is missing")) return 1;
    const std::size_t emissionFileSize = static_cast<std::size_t>(emissionFile.tellg());
    std::vector<unsigned char> emissionKtx(emissionFileSize);
    emissionFile.seekg(0, std::ios::beg);
    emissionFile.read(reinterpret_cast<char*>(emissionKtx.data()), static_cast<std::streamsize>(emissionKtx.size()));
    const auto readU32 = [&emissionKtx](std::size_t offset) {
        return static_cast<std::uint32_t>(emissionKtx[offset]) |
               (static_cast<std::uint32_t>(emissionKtx[offset + 1u]) << 8u) |
               (static_cast<std::uint32_t>(emissionKtx[offset + 2u]) << 16u) |
               (static_cast<std::uint32_t>(emissionKtx[offset + 3u]) << 24u);
    };
    const auto readU64 = [&readU32](std::size_t offset) {
        return static_cast<std::uint64_t>(readU32(offset)) |
               (static_cast<std::uint64_t>(readU32(offset + 4u)) << 32u);
    };
    constexpr unsigned char kKtx2Identifier[12] = {
        0xABu, 0x4Bu, 0x54u, 0x58u, 0x20u, 0x32u, 0x30u, 0xBBu, 0x0Du, 0x0Au, 0x1Au, 0x0Au};
    bool validIdentifier = emissionKtx.size() >= 104u;
    for (std::size_t i = 0; validIdentifier && i < 12u; ++i)
    {
        validIdentifier = emissionKtx[i] == kKtx2Identifier[i];
    }
    if (!Require(validIdentifier && readU32(12u) == 43u && readU32(20u) == 2048u &&
                 readU32(24u) == 2048u && readU32(40u) == 1u,
                 "derived lich emission audit KTX2 header changed")) return 1;
    const std::uint64_t levelOffset = readU64(80u);
    const std::uint64_t levelLength = readU64(88u);
    if (!Require(levelLength == 2048ull * 2048ull * 4ull &&
                 levelOffset <= emissionKtx.size() && levelLength <= emissionKtx.size() - levelOffset,
                 "derived lich emission KTX2 payload is invalid")) return 1;
    std::vector<unsigned char> emissionPixels(
        emissionKtx.begin() + static_cast<std::ptrdiff_t>(levelOffset),
        emissionKtx.begin() + static_cast<std::ptrdiff_t>(levelOffset + levelLength));
    std::size_t emissiveVertexCount = 0u;
    std::size_t outerEmissiveVertexCount = 0u;
    float outerX = 0.0f, outerY = 0.0f, outerZ = 0.0f;
    float emissiveMinX = 1.0e9f, emissiveMaxX = -1.0e9f;
    for (std::size_t vertexIndex = 0; vertexIndex < lichTextured.size(); ++vertexIndex)
    {
        const auto& vertex = lichTextured[vertexIndex];
        const float wrappedU = vertex.texcoord[0] - std::floor(vertex.texcoord[0]);
        const float wrappedV = vertex.texcoord[1] - std::floor(vertex.texcoord[1]);
        const std::size_t x = std::min<std::size_t>(2047u, static_cast<std::size_t>(wrappedU * 2048.0f));
        const std::size_t y = std::min<std::size_t>(2047u, static_cast<std::size_t>(wrappedV * 2048.0f));
        const std::size_t offset = (y * 2048u + x) * 4u;
        if (offset + 2u < emissionPixels.size() &&
            (emissionPixels[offset] || emissionPixels[offset + 1u] || emissionPixels[offset + 2u]))
        {
            ++emissiveVertexCount;
            emissiveMinX = std::min(emissiveMinX, vertex.position[0]);
            emissiveMaxX = std::max(emissiveMaxX, vertex.position[0]);
            if (vertex.position[0] > 0.55f)
            {
                ++outerEmissiveVertexCount;
                outerX += vertex.position[0];
                outerY += vertex.position[1];
                outerZ += vertex.position[2];
            }
        }
    }
    if (!Require(emissiveVertexCount > 0u, "no emissive lich vertices were found by UV audit")) return 1;
    if (!Require(outerEmissiveVertexCount == 40u, "audited staff crystal vertex set changed")) return 1;
    if (!Require(outerX / 40.0f > 0.90f && outerY / 40.0f > 0.70f,
                 "audited staff crystal sample moved into the robe or eye cluster")) return 1;
    std::vector<SkinnedRtVertex> unavailableAttack;
    if (!Require(!lich.Skin(SkinnedClip::Attack, 0.5f, unavailableAttack, diagnostic), "unmapped lich attack unexpectedly skinned")) return 1;

    std::cout << "Skinned character model smoke passed: skeleton=" << skeleton.ExpandedVertexCount()
              << " lich=" << lich.ExpandedVertexCount() << " textured std430 stride=" << sizeof(TexturedSkinnedRtVertex)
              << " emissiveVertices=" << emissiveVertexCount << " emissiveX=" << emissiveMinX << ".." << emissiveMaxX
              << " outer=" << outerEmissiveVertexCount << " avg="
              << outerX / static_cast<float>(std::max<std::size_t>(1u, outerEmissiveVertexCount)) << ','
              << outerY / static_cast<float>(std::max<std::size_t>(1u, outerEmissiveVertexCount)) << ','
              << outerZ / static_cast<float>(std::max<std::size_t>(1u, outerEmissiveVertexCount)) << '\n';
    return 0;
}
