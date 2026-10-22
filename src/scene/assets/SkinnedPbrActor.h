#pragma once
#include "scene/assets/SkinnedMeshAsset.h"
#include "scene/assets/StaticMeshAsset.h"
#include <algorithm>
#include <cstring>
#include <limits>

namespace horde::scene::assets
{
// CPU preparation owns source pose data; the scene's third actor lane owns GPU
// publication. Skinned positions already include the imported node hierarchy.
class SkinnedPbrActor
{
public:
    bool Load(const std::filesystem::path& directory, const char* filename,
              const SkinnedClipSet& clips, std::string& diagnostic)
    {
        AssetManifest manifest;
        if (!AssetManifest::Load(directory / "asset.manifest.json", manifest, diagnostic) ||
            !StaticMeshAsset::Load(directory / filename, manifest, asset_, diagnostic) ||
            !skin_.LoadClips((directory / filename).string(), clips, diagnostic) ||
            !skin_.ValidateStaticVertexLayout(asset_, diagnostic)) return false;
        for (auto& node : asset_.nodeTransforms)
            node.world = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
        return Pose(SkinnedClip::Idle, 0, diagnostic);
    }
    bool Pose(SkinnedClip clip, float time, std::string& diagnostic,
              std::span<const SkinnedNodeRotation> additiveNodeRotations = {},
              const SkinnedClipBlend* blend = nullptr, SkinWorkMetrics* work = nullptr)
    {
        if (!skin_.SkinUniquePbr(clip, time, posed_, tangents_, diagnostic, additiveNodeRotations, blend, work) ||
            posed_.size() != asset_.vertices.size()) return false;
        SkinWorkScope copyScope(work?&work->copyNanoseconds:nullptr);
        float floor = std::numeric_limits<float>::infinity();
        for (const auto& vertex : posed_) floor = std::min(floor, vertex.position[1]);
        asset_.bounds.minimum.fill(std::numeric_limits<float>::infinity());
        asset_.bounds.maximum.fill(-std::numeric_limits<float>::infinity());
        for (std::size_t i = 0; i < posed_.size(); ++i)
        {
            auto& vertex = asset_.vertices[i];
            std::memcpy(vertex.position.data(), posed_[i].position, sizeof(vertex.position));
            vertex.position[1] -= floor;
            std::memcpy(vertex.normal.data(), posed_[i].normal, sizeof(vertex.normal));
            std::memcpy(vertex.tangent.data(), tangents_[i].tangent, sizeof(vertex.tangent));
            for (std::size_t axis = 0; axis < 3; ++axis)
            {
                asset_.bounds.minimum[axis] = std::min(asset_.bounds.minimum[axis], vertex.position[axis]);
                asset_.bounds.maximum[axis] = std::max(asset_.bounds.maximum[axis], vertex.position[axis]);
            }
        }
        if(work){work->copyBytes+=asset_.vertices.size()*sizeof(asset_.vertices.front());++work->copies;}
        return true;
    }
    StaticMeshAsset& Asset() { return asset_; }
    const StaticMeshAsset& Asset() const { return asset_; }
    float Duration(SkinnedClip clip) const { return skin_.ClipDuration(clip); }
private:
    SkinnedMeshAsset skin_;
    StaticMeshAsset asset_;
    std::vector<TexturedSkinnedRtVertex> posed_;
    std::vector<SkinnedPbrTangent> tangents_;
};
}
