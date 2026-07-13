#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace horde::scene
{

// std430-compatible layout for both the RT build input and the raygen SSBO.
struct SkinnedRtVertex
{
    float position[4]{};
    float normal[4]{};
};

enum class SkeletonClip
{
    Idle,
    Walking,
    Attack,
    Dead
};

// Deliberately narrow glTF 2.0 reader for the staged Meshy biped. It imports one
// skinned primitive and exactly the four clips needed by the combat proof.
class SkeletonBipedModel
{
public:
    SkeletonBipedModel();
    ~SkeletonBipedModel();
    SkeletonBipedModel(const SkeletonBipedModel&) = delete;
    SkeletonBipedModel& operator=(const SkeletonBipedModel&) = delete;
    SkeletonBipedModel(SkeletonBipedModel&&) noexcept;
    SkeletonBipedModel& operator=(SkeletonBipedModel&&) noexcept;

    bool LoadCombatClips(const std::string& glbPath, std::string& diagnostic);
    bool IsLoaded() const { return loaded_; }
    std::size_t ExpandedVertexCount() const { return expandedIndices_.size(); }
    float ClipDuration(SkeletonClip clip) const;
    bool Skin(SkeletonClip clip, float timeSeconds, std::vector<SkinnedRtVertex>& output, std::string& diagnostic) const;

private:
    struct SourceVertex;
    struct Node;
    struct Channel;
    struct Clip;

    std::vector<SourceVertex> vertices_;
    std::vector<std::uint32_t> expandedIndices_;
    std::vector<Node> nodes_;
    std::vector<std::uint32_t> joints_;
    std::vector<float> inverseBindMatrices_;
    std::vector<Clip> clips_;
    mutable std::vector<SkinnedRtVertex> skinnedUniqueVertices_;
    bool loaded_ = false;
};

} // namespace horde::scene
