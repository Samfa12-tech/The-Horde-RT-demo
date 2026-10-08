// Opt-in contact-region witness. This includes the established socket-test
// fixture so the measurement reuses its production import, target skin,
// shared render-pose adapter and final anatomical Grip resolver.
#define main HeldItemSocketTestsEmbeddedMain
#include "HeldItemSocketTests.cpp"
#undef main

namespace
{

struct ContactRegionQuery
{
    float metres = std::numeric_limits<float>::max();
    std::size_t targetTriangle = std::numeric_limits<std::size_t>::max();
    std::size_t bladeTriangle = std::numeric_limits<std::size_t>::max();
    std::size_t exactTrianglePairs = 0u;
};

class ContactRegionTargetTree
{
public:
    explicit ContactRegionTargetTree(std::vector<MeshTriangle> triangles)
        : triangles_(std::move(triangles)), order_(triangles_.size())
    {
        std::iota(order_.begin(), order_.end(), 0u);
        if (!order_.empty()) Build(0u, order_.size());
    }

    ContactRegionQuery Query(const std::vector<MeshTriangle>& blade) const
    {
        ContactRegionQuery result;
        float bestSquared = std::numeric_limits<float>::max();
        for (std::size_t index = 0u; index < blade.size(); ++index)
            QueryTriangle(0u, blade[index], index, bestSquared, result);
        result.metres = std::sqrt(bestSquared);
        return result;
    }

private:
    struct Node
    {
        WorldBounds bounds{};
        std::size_t begin = 0u;
        std::size_t end = 0u;
        std::size_t left = std::numeric_limits<std::size_t>::max();
        std::size_t right = std::numeric_limits<std::size_t>::max();
    };

    std::size_t Build(const std::size_t begin, const std::size_t end)
    {
        const std::size_t nodeIndex = nodes_.size();
        nodes_.emplace_back();
        WorldBounds bounds;
        WorldBounds centers;
        for (std::size_t i = begin; i < end; ++i)
        {
            const auto& triangle = triangles_[order_[i]];
            IncludePoint(bounds, triangle.bounds.minimum);
            IncludePoint(bounds, triangle.bounds.maximum);
            IncludePoint(centers, triangle.center);
        }
        nodes_[nodeIndex].bounds = bounds;
        nodes_[nodeIndex].begin = begin;
        nodes_[nodeIndex].end = end;
        if (end - begin <= 8u) return nodeIndex;
        std::size_t axis = 0u;
        for (std::size_t candidate = 1u; candidate < 3u; ++candidate)
            if (centers.maximum[candidate] - centers.minimum[candidate] >
                centers.maximum[axis] - centers.minimum[axis]) axis = candidate;
        const std::size_t middle = begin + (end - begin) / 2u;
        std::nth_element(order_.begin() + static_cast<std::ptrdiff_t>(begin),
                         order_.begin() + static_cast<std::ptrdiff_t>(middle),
                         order_.begin() + static_cast<std::ptrdiff_t>(end),
            [&](const std::size_t left, const std::size_t right) {
                return triangles_[left].center[axis] < triangles_[right].center[axis];
            });
        const std::size_t left = Build(begin, middle);
        const std::size_t right = Build(middle, end);
        nodes_[nodeIndex].left = left;
        nodes_[nodeIndex].right = right;
        return nodeIndex;
    }

    void QueryTriangle(const std::size_t nodeIndex,
                       const MeshTriangle& query,
                       const std::size_t bladeIndex,
                       float& bestSquared,
                       ContactRegionQuery& result) const
    {
        const Node& node = nodes_[nodeIndex];
        if (DistanceSquaredBetweenBounds(query.bounds, node.bounds) > bestSquared)
            return;
        if (node.left == std::numeric_limits<std::size_t>::max())
        {
            const std::array<std::array<float, 3u>, 3u> queryPoints{{
                query.a, query.b, query.c}};
            for (std::size_t i = node.begin; i < node.end; ++i)
            {
                const std::size_t targetIndex = order_[i];
                const MeshTriangle& target = triangles_[targetIndex];
                const std::array<std::array<float, 3u>, 3u> targetPoints{{
                    target.a, target.b, target.c}};
                ++result.exactTrianglePairs;
                const float distanceSquared = TriangleDistanceSquared(
                    queryPoints, targetPoints);
                if (distanceSquared < bestSquared)
                {
                    bestSquared = distanceSquared;
                    result.targetTriangle = targetIndex;
                    result.bladeTriangle = bladeIndex;
                }
            }
            return;
        }
        const float leftDistance = DistanceSquaredBetweenBounds(
            query.bounds, nodes_[node.left].bounds);
        const float rightDistance = DistanceSquaredBetweenBounds(
            query.bounds, nodes_[node.right].bounds);
        if (leftDistance <= rightDistance)
        {
            QueryTriangle(node.left, query, bladeIndex, bestSquared, result);
            QueryTriangle(node.right, query, bladeIndex, bestSquared, result);
        }
        else
        {
            QueryTriangle(node.right, query, bladeIndex, bestSquared, result);
            QueryTriangle(node.left, query, bladeIndex, bestSquared, result);
        }
    }

    std::vector<MeshTriangle> triangles_;
    std::vector<std::size_t> order_;
    std::vector<Node> nodes_;
};

struct JointSegment
{
    const char* name;
    const char* start;
    const char* end;
};

constexpr std::array<JointSegment, 24u> kImportedJointSegments{{
    {"Armature-Hips", "Armature", "Hips"},
    {"Hips-LeftUpLeg", "Hips", "LeftUpLeg"},
    {"LeftUpLeg-LeftLeg", "LeftUpLeg", "LeftLeg"},
    {"LeftLeg-LeftFoot", "LeftLeg", "LeftFoot"},
    {"LeftFoot-LeftToeBase", "LeftFoot", "LeftToeBase"},
    {"Hips-RightUpLeg", "Hips", "RightUpLeg"},
    {"RightUpLeg-RightLeg", "RightUpLeg", "RightLeg"},
    {"RightLeg-RightFoot", "RightLeg", "RightFoot"},
    {"RightFoot-RightToeBase", "RightFoot", "RightToeBase"},
    {"Hips-Spine02", "Hips", "Spine02"},
    {"Spine02-Spine01", "Spine02", "Spine01"},
    {"Spine01-Spine", "Spine01", "Spine"},
    {"Spine-LeftShoulder", "Spine", "LeftShoulder"},
    {"LeftShoulder-LeftArm", "LeftShoulder", "LeftArm"},
    {"LeftArm-LeftForeArm", "LeftArm", "LeftForeArm"},
    {"LeftForeArm-LeftHand", "LeftForeArm", "LeftHand"},
    {"Spine-RightShoulder", "Spine", "RightShoulder"},
    {"RightShoulder-RightArm", "RightShoulder", "RightArm"},
    {"RightArm-RightForeArm", "RightArm", "RightForeArm"},
    {"RightForeArm-RightHand", "RightForeArm", "RightHand"},
    {"Spine-neck", "Spine", "neck"},
    {"neck-Head", "neck", "Head"},
    {"Head-head_end", "Head", "head_end"},
    {"Head-headfront", "Head", "headfront"},
}};

struct RegionLabel
{
    const char* segment = "unavailable";
    float centroidDistanceMetres = std::numeric_limits<float>::max();
};

RegionLabel LabelByNearestAuthoredSegment(
    const MeshTriangle& triangle,
    const DiagnosticSkeletonRenderSample& sample,
    const horde::scene::SkinnedMeshAsset& skeleton,
    std::string& diagnostic)
{
    std::vector<std::string_view> names;
    for (const JointSegment& segment : kImportedJointSegments)
    {
        for (const char* name : {segment.start, segment.end})
            if (std::find(names.begin(), names.end(), name) == names.end())
                names.emplace_back(name);
    }
    std::vector<horde::scene::SkinnedNodeTransform> transforms(names.size());
    if (!skeleton.NodeTransforms(sample.clip, sample.clipTime, names, transforms,
                                 diagnostic))
        return {};
    const auto position = [&](const char* name) {
        const auto found = std::find(names.begin(), names.end(), name);
        const auto& matrix = transforms[static_cast<std::size_t>(found - names.begin())];
        return ApplySkeletonTransform(sample.transform,
            {{matrix[12], matrix[13], matrix[14]}});
    };
    const auto centroid = Scale(Add(Add(triangle.a, triangle.b), triangle.c), 1.0f / 3.0f);
    RegionLabel label;
    float bestSquared = std::numeric_limits<float>::max();
    for (const JointSegment& segment : kImportedJointSegments)
    {
        const float distanceSquared = PointSegmentDistanceSquared(
            centroid, position(segment.start), position(segment.end));
        if (distanceSquared < bestSquared)
        {
            bestSquared = distanceSquared;
            label.segment = segment.name;
        }
    }
    label.centroidDistanceMetres = std::sqrt(bestSquared);
    return label;
}

void TestSwordTargetContactRegionWitness()
{
    using namespace horde::gameplay;
    using namespace horde::gameplay::items;
    using horde::scene::SkinnedClip;
    using horde::scene::SkinnedMeshAsset;

    constexpr float tickSeconds = 1.0f / 60.0f;
    constexpr std::size_t sampleCount = 8u;
    constexpr std::size_t expectedCases = 3u;
    constexpr std::size_t queryBudget = 24u;
    constexpr std::size_t expectedBladeTriangles = 6905u;
    const std::filesystem::path root = HORDE_RT_SOURCE_DIR;
    horde::scene::assets::StaticMeshAsset sword;
    horde::scene::assets::StaticMeshAsset torch;
    SkinnedMeshAsset skeleton;
    std::string diagnostic;
    const bool heldLoaded = LoadProductionHeldAssets(sword, torch, diagnostic);
    const auto* grip = FindHeldItemSocket(sword.sockets, "Grip");
    const bool skeletonLoaded = skeleton.LoadCombatClips(
        (root / "assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb").string(),
        diagnostic);
    horde::vulkan::raytracing::PlayerRenderSlot playerRig;
    const bool playerLoaded = playerRig.LoadAsset(
        (root / "assets/models/player/runtime/gothic-traveller-lod0.runtime.glb").string(),
        diagnostic);
    Check(heldLoaded && grip != nullptr && !sword.vertices.empty() &&
              skeletonLoaded && playerLoaded,
          "contact-region witness must load the production sword, Grip, target skin and player rig");
    if (!heldLoaded || grip == nullptr || !skeletonLoaded || !playerLoaded) return;

    struct Case
    {
        const char* name;
        float bearing;
        bool attacking;
    };
    constexpr std::array<Case, expectedCases> cases{{
        {"frontal-1.28", 0.0f, true},
        {"minus15-1.28", -0.261799388f, true},
        {"idle-frontal-1.28", 0.0f, false},
    }};
    std::size_t exactQueries = 0u;
    std::size_t totalExactTrianglePairs = 0u;
    std::uint64_t rigTick = 190000u;

    for (const Case& contact : cases)
    {
        const float targetX = 1.28f * std::sin(contact.bearing);
        const float targetZ = -1.28f * std::cos(contact.bearing);
        SwordCombat attack;
        SwordCombat control;
        attack.Reset(1u, {targetX, targetZ});
        control.Reset(1u, {targetX, targetZ});
        const bool requested = !contact.attacking ||
            attack.RequestAttack() == PlayerAttackCut::DownwardCut;
        Check(requested, "attack witness cases must request the existing downward cut");
        std::vector<CombatSnapshot> attackAfter;
        std::vector<CombatSnapshot> controlAfter;
        attackAfter.reserve(23u);
        controlAfter.reserve(23u);
        std::uint64_t pulseTick = 0u;
        for (std::uint64_t tick = 1u; tick <= 23u; ++tick)
        {
            const auto attackSnapshot = attack.Update(
                tickSeconds, 0.0f, 0.0f, 0.0f, true, true, false);
            const auto controlSnapshot = control.Update(
                tickSeconds, 0.0f, 0.0f, 0.0f, true, true, false);
            attackAfter.push_back(attackSnapshot);
            controlAfter.push_back(controlSnapshot);
            if (attackSnapshot.playerAttackPulse && pulseTick == 0u)
                pulseTick = tick;
        }
        Check(!contact.attacking || pulseTick == 17u,
              "attack witness must retain the existing immediate-request pulse tick");
        if (contact.attacking && pulseTick != 17u) continue;

        for (std::size_t sampleIndex = 0u; sampleIndex < sampleCount; ++sampleIndex)
        {
            const std::uint64_t tick = 16u + sampleIndex;
            const auto& combatSnapshot = contact.attacking
                ? attackAfter[static_cast<std::size_t>(tick - 1u)]
                : controlAfter[static_cast<std::size_t>(tick - 1u)];
            const auto& targetSnapshot = controlAfter[static_cast<std::size_t>(tick - 1u)]
                .combatants[0u];
            const auto targetSample = ResolveSharedCharacterRenderPose(
                targetSnapshot, skeleton.ClipDuration(SkinnedClip::Dead));
            Check(targetSample.clip == SkinnedClip::Attack &&
                      targetSnapshot.action == EnemyCombatAction::AttackWindup,
                  "all eight witness samples must keep the target in the ordinary AttackWindup pose phase");

            HeldItemFixedStepInput input;
            input.playerX = 0.0f;
            input.playerZ = 0.0f;
            input.playerYawRadians = 0.0f;
            input.playerMountProfile = PlayerMountProfile::AnatomicalBody;
            input.playerCombat = combatSnapshot.player;
            input.logicalViewAspect = 1.0f;
            HeldItemStates items = MakeDefaultHeldItemStates();
            HeldItemFixedStepState fixed;
            HeldItemTransform worldFromGrip{};
            HeldItemTransform worldFromSword{};
            const bool resolved = ResolveHeldItemsFixedStep(
                    items, input, tick, fixed, diagnostic) &&
                ResolveProductionAnatomicalSword(input, fixed.kinematics, items,
                    playerRig, rigTick++, worldFromGrip, worldFromSword, diagnostic);
            const auto blade = resolved
                ? BuildGripFilteredBladeTriangles(sword, *grip, worldFromSword)
                : std::vector<MeshTriangle>{};
            std::vector<horde::scene::SkinnedRtVertex> targetPose;
            const bool skinned = skeleton.Skin(targetSample.clip,
                targetSample.clipTime, targetPose, diagnostic);
            auto targetTriangles = skinned
                ? BuildDiagnosticTargetTriangles(targetSample, targetPose)
                : std::vector<MeshTriangle>{};
            Check(resolved && skinned && blade.size() == expectedBladeTriangles &&
                      !targetTriangles.empty(),
                  "each contact-region sample must use the full production blade and imported target skin");
            if (!resolved || !skinned || blade.size() != expectedBladeTriangles ||
                targetTriangles.empty())
                continue;

            const ContactRegionTargetTree targetTree(std::move(targetTriangles));
            const ContactRegionQuery query = targetTree.Query(blade);
            ++exactQueries;
            totalExactTrianglePairs += query.exactTrianglePairs;
            Check(query.targetTriangle < targetPose.size() / 3u &&
                      query.bladeTriangle < blade.size() &&
                      query.exactTrianglePairs > 0u &&
                      std::isfinite(query.metres) && query.metres >= 0.0f,
                  "each witness must return a finite exact nearest target/blade triangle pair");
            if (query.targetTriangle >= targetPose.size() / 3u) continue;

            const auto exactTargetTriangles = BuildDiagnosticTargetTriangles(targetSample, targetPose);
            const RegionLabel region = LabelByNearestAuthoredSegment(
                exactTargetTriangles[query.targetTriangle], targetSample, skeleton, diagnostic);
            Check(std::isfinite(region.centroidDistanceMetres) &&
                      std::string(region.segment) != "unavailable",
                  "nearest target triangle must receive a label from actual imported skeleton segments");
            const float range = std::hypot(targetSnapshot.x, targetSnapshot.z);
            const float facingDot = range > 0.0001f
                ? (-targetSnapshot.z / range) : 1.0f;
            const bool gate = SwordCombat::IsPlayerTargetInRangeCone(
                0.0f, 0.0f, 0.0f, targetSnapshot.x, targetSnapshot.z);
            const bool damageAtPulse = contact.attacking && tick == pulseTick &&
                attackAfter[static_cast<std::size_t>(tick - 1u)].combatants[0u].health <
                attackAfter[static_cast<std::size_t>(tick - 2u)].combatants[0u].health;
            Check(gate, "witness target must remain inside the unchanged production range/cone gate");
            Check(!contact.attacking || combatSnapshot.player.action == PlayerCombatAction::SwingActive,
                  "attack region samples must remain on the active downward-cut phase");
            Check(contact.attacking || combatSnapshot.player.action == PlayerCombatAction::Idle,
                  "negative-control region samples must retain the idle player pose");
            std::cout << "sword-target-contact-region case=" << contact.name
                      << " snapshotTick=" << tick
                      << " playerAction=" << static_cast<int>(combatSnapshot.player.action)
                      << " playerActionTime=" << combatSnapshot.player.actionTime
                      << " targetAction=" << static_cast<int>(targetSnapshot.action)
                      << " targetActionTime=" << targetSnapshot.actionTime
                      << " targetAnimationTime=" << targetSnapshot.animationTime
                      << " rangeM=" << range << " facingDot=" << facingDot
                      << " gateEligible=" << gate
                      << " damageAtPulse=" << damageAtPulse
                      << " gapMm=" << query.metres * 1000.0f
                      << " targetTriangle=" << query.targetTriangle
                      << " bladeTriangle=" << query.bladeTriangle
                      << " exactTrianglePairs=" << query.exactTrianglePairs
                      << " bladeTriangles=" << blade.size()
                      << " targetTriangles=" << (targetPose.size() / 3u)
                      << " nearestAuthoredSkeletonSegment=" << region.segment
                      << " triangleCentroidToSegmentMm="
                      << region.centroidDistanceMetres * 1000.0f
                      << " regionLabelSource=actualJointNodeTransforms_nearestCentroid"
                      << " sourceJointWeightsExposedByRuntimeAssetApi=0\n";
        }
    }

    Check(exactQueries == queryBudget,
          "contact-region witness must complete exactly 24 full-mesh queries");
    Check(exactQueries == expectedCases * sampleCount,
          "contact-region witness must cover exactly three cases by eight fixed snapshots");
    std::cout << "sword-target-contact-region-summary exactQueries=" << exactQueries
              << " queryBudget=" << queryBudget
              << " exactBladeTargetTrianglePairs=" << totalExactTrianglePairs
              << " measuredSnapshots=discrete_fixed_tick_pairs"
              << " continuousCollisionClaim=0"
              << " presentedFrameClaim=0"
              << " labelAuthority=nearest_actual_imported_skeleton_joint_segment"
              << " vertexWeightClassification=unavailable\n";
}

} // namespace

int main()
{
    TestSwordTargetContactRegionWitness();
    return failures == 0 ? 0 : 1;
}
