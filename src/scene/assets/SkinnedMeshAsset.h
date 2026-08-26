#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace horde::scene
{

using SkinnedNodeTransform = std::array<float, 16u>;

// std430-compatible layout for both the RT build input and the raygen SSBO.
struct SkinnedRtVertex
{
    float position[4]{};
    float normal[4]{};
};
static_assert(sizeof(SkinnedRtVertex) == 32u);
static_assert(offsetof(SkinnedRtVertex, normal) == 16u);

// Textured characters use a separate record so adding UVs cannot silently
// change the 32-byte stride consumed by the existing skeleton raygen path.
// TEXCOORD_0 occupies xy; zw are deterministic std430 padding.
struct TexturedSkinnedRtVertex
{
    float position[4]{};
    float normal[4]{};
    float texcoord[4]{};
};
static_assert(sizeof(TexturedSkinnedRtVertex) == 48u);
static_assert(offsetof(TexturedSkinnedRtVertex, normal) == 16u);
static_assert(offsetof(TexturedSkinnedRtVertex, texcoord) == 32u);

enum class SkinnedClip
{
    Idle,
    Walking,
    Attack,
    Dead
};
using SkeletonClip = SkinnedClip;

struct SkinnedClipBinding
{
    std::string name;
    bool loops = false;
    bool required = true;
};

struct SkinnedClipSet
{
    std::array<SkinnedClipBinding, 4u> clips{};
};

struct SkinnedPrimitiveRange
{
    std::size_t firstExpandedVertex = 0u;
    std::size_t expandedVertexCount = 0u;
    std::string materialName;
};

struct SkinnedArmIkTarget
{
    std::array<float, 3u> target{};
    std::array<float, 3u> pole{};
};

struct SkinnedPlayerSockets
{
    SkinnedNodeTransform leftHand{};
    SkinnedNodeTransform rightHand{};
};

const SkinnedClipSet& SkeletonCombatClipSet();
const SkinnedClipSet& LichPlaceholderClipSet();
const SkinnedClipSet& PlayerLocomotionClipSet();

// Deliberately narrow glTF 2.0 reader for the audited Meshy bipeds. It imports
// one skinned primitive into four semantic clip slots selected by the caller.
class SkinnedMeshAsset
{
public:
    SkinnedMeshAsset();
    ~SkinnedMeshAsset();
    SkinnedMeshAsset(const SkinnedMeshAsset&) = delete;
    SkinnedMeshAsset& operator=(const SkinnedMeshAsset&) = delete;
    SkinnedMeshAsset(SkinnedMeshAsset&&) noexcept;
    SkinnedMeshAsset& operator=(SkinnedMeshAsset&&) noexcept;

    bool LoadClips(const std::string& glbPath, const SkinnedClipSet& clipSet, std::string& diagnostic);
    bool LoadCombatClips(const std::string& glbPath, std::string& diagnostic);
    bool IsLoaded() const { return loaded_; }
    bool HasTexcoords() const { return hasTexcoords_; }
    std::size_t ExpandedVertexCount() const { return expandedIndices_.size(); }
    std::size_t UniqueVertexCount() const;
    const std::vector<SkinnedPrimitiveRange>& PrimitiveRanges() const { return primitiveRanges_; }
    float ClipDuration(SkinnedClip clip) const;
    bool HasNode(std::string_view name) const;
    bool NodeTransform(SkinnedClip clip,
                       float timeSeconds,
                       std::string_view nodeName,
                       SkinnedNodeTransform& output,
                       std::string& diagnostic) const;
    bool Skin(SkinnedClip clip, float timeSeconds, std::vector<SkinnedRtVertex>& output, std::string& diagnostic) const;
    bool SkinTextured(SkinnedClip clip, float timeSeconds, std::vector<TexturedSkinnedRtVertex>& output, std::string& diagnostic) const;
    bool SkinUniqueTextured(SkinnedClip clip,
                            float timeSeconds,
                            std::vector<TexturedSkinnedRtVertex>& output,
                            std::string& diagnostic) const;
    bool SkinPlayerUniqueTextured(SkinnedClip clip,
                                  float timeSeconds,
                                  const SkinnedArmIkTarget& leftArm,
                                  const SkinnedArmIkTarget& rightArm,
                                  std::vector<TexturedSkinnedRtVertex>& output,
                                  SkinnedPlayerSockets& sockets,
                                  std::string& diagnostic) const;

private:
    struct SourceVertex;
    struct Node;
    struct Channel;
    struct Clip;

    std::vector<SourceVertex> vertices_;
    std::vector<std::uint32_t> expandedIndices_;
    std::vector<SkinnedPrimitiveRange> primitiveRanges_;
    std::vector<Node> nodes_;
    std::vector<std::uint32_t> joints_;
    std::vector<float> inverseBindMatrices_;
    std::vector<Clip> clips_;
    mutable std::vector<SkinnedRtVertex> skinnedUniqueVertices_;
    mutable std::vector<SkinnedRtVertex> texturedSkinScratch_;
    bool hasTexcoords_ = false;
    bool loaded_ = false;
};

using SkinnedCharacterModel = SkinnedMeshAsset;

} // namespace horde::scene
