#pragma once
#include "scene/assets/SkinWorkMetrics.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace horde::scene
{

namespace assets { class StaticMeshAsset; }

using SkinnedNodeTransform = std::array<float, 16u>;

// Local-space additive rotation applied after the authored clip pose. Missing
// entries leave a node untouched, so existing skins retain their exact pose.
struct SkinnedNodeRotation
{
    std::string_view nodeName;
    std::array<float, 4u> rotation{{0.0f, 0.0f, 0.0f, 1.0f}};
};

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

// Kept separate from TexturedSkinnedRtVertex so the established mobile lich
// SSBO/BLAS stride remains 48 bytes. Indexed PBR skinning consumes this
// alongside its unique skinned vertices and writes it into StaticRtVertex.
struct SkinnedPbrTangent
{
    float tangent[4]{};
};
static_assert(sizeof(SkinnedPbrTangent) == 16u);

enum class SkinnedClip
{
    Idle,
    Walking,
    Attack,
    Dead
};
using SkeletonClip = SkinnedClip;

// Blend local joint TRS before the single skin/tangent pass. Weight endpoints
// preserve the exact plain clip; no per-vertex interpolation or second skin.
struct SkinnedClipBlend
{
    SkinnedClip clip = SkinnedClip::Idle;
    float timeSeconds = 0.0f;
    float weight = 0.0f;
};

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
    std::size_t firstUniqueVertex = 0u;
    std::size_t uniqueVertexCount = 0u;
};

struct SkinnedArmIkTarget
{
    std::array<float, 3u> target{};
    std::array<float, 3u> pole{};
    std::array<float, 3u> shoulder{};
    bool shoulderTargetEnabled = false;
    SkinnedNodeTransform handOrientation{};
    bool handOrientationTargetEnabled = false;
    // Bounded [0, pi/4] allowance when fitting an overreaching chain. The
    // existing maximum axial stretch still takes precedence over this pose.
    float preferredElbowFlexionRadians = 0.0f;
    float poseWeight = 1.0f;
};

struct SkinnedPlayerSockets
{
    SkinnedNodeTransform hips{};
    SkinnedNodeTransform leftHand{};
    SkinnedNodeTransform rightHand{};
    SkinnedNodeTransform leftGrip{};
    SkinnedNodeTransform rightGrip{};
    // Hip joint, knee and ankle in the same immutable solved palette.
    // Empty when neither grounded leg pose is requested (the legacy fast path).
    std::array<SkinnedNodeTransform, 3u> leftLeg{};
    std::array<SkinnedNodeTransform, 3u> rightLeg{};
};

inline constexpr float kMaximumPlayerPelvisDropMetres = 0.72f;
inline constexpr float kMaximumPlayerCrouchPelvisDropMetres = 0.6f;
// Reversible native pose recipe: move the pelvis back as the knees fold.
// In the imported +Z-forward frame, the deepest admitted pose sets back 18 cm.
inline constexpr float kPlayerPlantedPelvisSetbackPerDrop = 0.25f;

struct SkinnedPlayerDodgePose
{
    float weight = 0.0f;
    float modelX = 0.0f;
    float modelZ = 0.0f;
};
inline bool ValidPlayerDodgePose(const SkinnedPlayerDodgePose& pose)
{
    return std::isfinite(pose.weight) && pose.weight >= 0.0f && pose.weight <= 1.0f &&
        std::isfinite(pose.modelX) && std::isfinite(pose.modelZ) &&
        (pose.weight == 0.0f || std::abs(std::hypot(pose.modelX,pose.modelZ)-1.0f) < .002f);
}
inline std::array<float,3> PlayerDodgeTorsoOffset(const SkinnedPlayerDodgePose& pose)
{
    return {pose.modelX * pose.weight * .04f, -pose.weight * .08f,
            pose.modelZ * pose.weight * .04f};
}

struct SkinnedPlayerPoseData;
struct SkinnedPlayerRigLayout;

// One solved animation/IK pose may drive multiple independently owned meshes.
// Consumers cannot edit the palette or substitute their own grip transforms.
class SkinnedPlayerPose
{
public:
    SkinnedPlayerPose();
    ~SkinnedPlayerPose();
    SkinnedPlayerPose(SkinnedPlayerPose&&) noexcept;
    SkinnedPlayerPose& operator=(SkinnedPlayerPose&&) noexcept;
    SkinnedPlayerPose(const SkinnedPlayerPose&) = delete;
    SkinnedPlayerPose& operator=(const SkinnedPlayerPose&) = delete;
    bool IsValid() const;
    // Borrowed until this pose is evaluated/reset/destroyed; copy for retention.
    const SkinnedPlayerSockets& Sockets() const;

private:
    friend class SkinnedMeshAsset;
    std::unique_ptr<SkinnedPlayerPoseData> data_;
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
    bool HasTangents() const { return hasTangents_; }
    std::size_t ExpandedVertexCount() const { return expandedIndices_.size(); }
    std::size_t UniqueVertexCount() const;
    // Static indices/UVs remain resident while skinning replaces positions,
    // normals and tangents. Validate their exact addressing before GPU upload.
    bool ValidateStaticVertexLayout(const assets::StaticMeshAsset& asset,
                                    std::string& diagnostic) const;
    std::size_t BootGroundingCandidateVertexCount() const
    {
        return bootGroundingVertexIndices_.size();
    }
    const std::vector<SkinnedPrimitiveRange>& PrimitiveRanges() const { return primitiveRanges_; }
    float ClipDuration(SkinnedClip clip) const;
    bool BootGroundingMinimumY(SkinnedClip clip,
                               float timeSeconds,
                               float& minimumY,
                               std::string& diagnostic) const;
    bool HasNode(std::string_view name) const;
    static constexpr std::size_t kMaximumNodeTransformBatchSize = 32u;
    bool NodeTransforms(SkinnedClip clip,
                        float timeSeconds,
                        std::span<const std::string_view> nodeNames,
                        std::span<SkinnedNodeTransform> outputs,
                        std::string& diagnostic) const;
    bool NodeTransform(SkinnedClip clip,
                       float timeSeconds,
                       std::string_view nodeName,
                       SkinnedNodeTransform& output,
                       std::string& diagnostic) const;
    bool Skin(SkinnedClip clip, float timeSeconds, std::vector<SkinnedRtVertex>& output,
              std::string& diagnostic,
              std::span<const SkinnedNodeRotation> additiveNodeRotations = {}, SkinWorkMetrics* work = nullptr) const;
    bool SkinTextured(SkinnedClip clip, float timeSeconds,
                      std::vector<TexturedSkinnedRtVertex>& output, std::string& diagnostic,
                      std::span<const SkinnedNodeRotation> additiveNodeRotations = {}, SkinWorkMetrics* work = nullptr) const;
    bool SkinUniqueTextured(SkinnedClip clip,
                            float timeSeconds,
                            std::vector<TexturedSkinnedRtVertex>& output,
                            std::string& diagnostic, SkinWorkMetrics* work = nullptr) const;
    // Plain clip skinning for indexed PBR actors, without player rig/IK rules.
    bool SkinUniquePbr(SkinnedClip clip, float timeSeconds,
                       std::vector<TexturedSkinnedRtVertex>& output,
                       std::vector<SkinnedPbrTangent>& outputTangents,
                       std::string& diagnostic,
                       std::span<const SkinnedNodeRotation> additiveNodeRotations = {},
                       const SkinnedClipBlend* blend = nullptr, SkinWorkMetrics* work = nullptr) const;
    bool SkinPlayerUniqueTextured(SkinnedClip clip,
                                  float timeSeconds,
                                  const SkinnedArmIkTarget& leftArm,
                                  const SkinnedArmIkTarget& rightArm,
                                  std::vector<TexturedSkinnedRtVertex>& output,
                                  std::vector<SkinnedPbrTangent>& outputTangents,
                                  SkinnedPlayerSockets& sockets,
                                  std::string& diagnostic) const;
    // Positive drop is stationary Idle only: it lowers/sets back the torso and
    // folds both legs without changing lengths or authored ankle transforms.
    // [0, 0.72] metres; zero keeps the old palette for every supported clip.
    // Independent crouch drop [0, 0.6] supports Idle/Walking, vertically lowering
    // the torso while preserving each animated ankle/foot descendant transform.
    // The two drops are mutually exclusive. Arm targets remain caller-owned in
    // the same model frame (including the shared lowered eye); no extra shift.
    // Optional dodge translates the pelvis and articulates directional ankle
    // targets in model space before the same arm solve. It is exclusive with
    // the grounded drops; zero retains the exact original palette. These are
    // visual pose constraints, not movement/collision or world-foot locking.
    bool EvaluatePlayerPose(SkinnedClip clip, float timeSeconds,
                            const SkinnedArmIkTarget& leftArm,
                            const SkinnedArmIkTarget& rightArm,
                            SkinnedPlayerPose& pose, std::string& diagnostic,
                            float plantedPelvisDropMetres = 0.0f,
                            float crouchPelvisDropMetres = 0.0f,
                            SkinnedPlayerDodgePose dodge = {}) const;
    // Requires exact ordered joint names and inverse-bind agreement. A successful
    // binding is cached by immutable rig identity, never by raw asset addresses.
    bool SkinPlayerPoseUniqueTextured(const SkinnedPlayerPose& pose,
                                      std::vector<TexturedSkinnedRtVertex>& output,
                                      std::vector<SkinnedPbrTangent>& outputTangents,
                                      std::string& diagnostic) const;

private:
    bool SkinUniqueGeometry(SkinnedClip clip, float timeSeconds,
                            std::string& diagnostic,
                            std::span<const SkinnedNodeRotation> additiveNodeRotations,
                            std::vector<SkinnedPbrTangent>* outputTangents = nullptr,
                            const SkinnedClipBlend* blend = nullptr, SkinWorkMetrics* work = nullptr) const;
    void CopyUniqueTextured(std::vector<TexturedSkinnedRtVertex>& output, SkinWorkMetrics* work = nullptr) const;
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
    std::vector<float> idleBootMinimumY_;
    std::vector<float> walkingBootMinimumY_;
    std::vector<std::uint32_t> bootGroundingVertexIndices_;
    mutable std::vector<SkinnedRtVertex> skinnedUniqueVertices_;
    mutable std::vector<SkinnedRtVertex> texturedSkinScratch_;
    std::shared_ptr<const SkinnedPlayerRigLayout> playerRigLayout_;
    mutable std::weak_ptr<const SkinnedPlayerRigLayout> compatiblePoseRig_;
    bool hasTexcoords_ = false;
    bool hasTangents_ = false;
    bool loaded_ = false;
};

using SkinnedCharacterModel = SkinnedMeshAsset;

} // namespace horde::scene
