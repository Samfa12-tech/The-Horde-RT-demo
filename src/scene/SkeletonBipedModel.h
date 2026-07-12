#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace horde::scene
{

// std430-compatible layout for both the RT build input and the raygen SSBO.
struct SkinnedRtVertex
{
    float position[4]{};
    float normal[4]{};
};

// Deliberately narrow glTF 2.0 reader for the staged Meshy biped. It imports one
// skinned primitive and samples its named Idle_5 clip on the CPU for RT BLAS refits.
class SkeletonBipedModel
{
public:
    SkeletonBipedModel();
    ~SkeletonBipedModel();
    SkeletonBipedModel(const SkeletonBipedModel&) = delete;
    SkeletonBipedModel& operator=(const SkeletonBipedModel&) = delete;
    SkeletonBipedModel(SkeletonBipedModel&&) noexcept;
    SkeletonBipedModel& operator=(SkeletonBipedModel&&) noexcept;

    bool LoadIdleClip(const std::string& glbPath, std::string& diagnostic);
    bool IsLoaded() const { return loaded_; }
    std::size_t ExpandedVertexCount() const { return expandedIndices_.size(); }
    bool SkinIdle(float timeSeconds, std::vector<SkinnedRtVertex>& output, std::string& diagnostic) const;

private:
    struct SourceVertex;
    struct Node;
    struct Channel;

    std::vector<SourceVertex> vertices_;
    std::vector<std::uint32_t> expandedIndices_;
    std::vector<Node> nodes_;
    std::vector<std::uint32_t> joints_;
    std::vector<float> inverseBindMatrices_;
    std::vector<Channel> channels_;
    mutable std::vector<SkinnedRtVertex> skinnedUniqueVertices_;
    float clipDuration_ = 0.0f;
    bool loaded_ = false;
};

} // namespace horde::scene
