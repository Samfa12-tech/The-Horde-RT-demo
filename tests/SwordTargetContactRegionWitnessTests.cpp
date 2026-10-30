// Opt-in contact-region witness. This includes the established socket-test
// fixture so the measurement reuses its production import, target skin,
// shared render-pose adapter and final anatomical Grip resolver.
#define main HeldItemSocketTestsEmbeddedMain
#include "HeldItemSocketTests.cpp"
#undef main
#include "gameplay/SwordContactProxy.h"

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

horde::gameplay::SwordContactCapsule BuildMeasuredBladeCapsule(
    const horde::scene::assets::StaticMeshAsset& sword,
    const horde::scene::assets::StaticSocket& grip,
    const HeldItemTransform& worldFromGrip);

std::array<horde::gameplay::SwordContactCapsule, 6u> BuildMeasuredUpperBodyProfile(
    const horde::scene::SkinnedMeshAsset& skeleton,
    const DiagnosticSkeletonRenderSample& sample,
    const std::vector<horde::scene::SkinnedRtVertex>& targetPose,
    std::string& diagnostic);

std::vector<FittedSkeletonCapsule> FitRobustSkeletonCapsules(
    const horde::scene::SkinnedMeshAsset& skeleton,
    const DiagnosticSkeletonRenderSample& sample,
    const std::vector<horde::scene::SkinnedRtVertex>& targetPose,
    std::string& diagnostic);

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
            const auto bladeCapsule = BuildMeasuredBladeCapsule(sword, *grip, worldFromGrip);
            const auto bodyCapsules = BuildMeasuredUpperBodyProfile(
                skeleton, targetSample, targetPose, diagnostic);
            std::array<horde::gameplay::SwordContactTargetPair, 6u> targetPairs{};
            for (std::size_t regionIndex = 0u; regionIndex < targetPairs.size(); ++regionIndex)
                targetPairs[regionIndex] = {1, bodyCapsules[regionIndex],
                    bodyCapsules[regionIndex], true};
            const auto proxy = horde::gameplay::EvaluateSwordContactProxy(
                bladeCapsule, bladeCapsule,
                std::span<const horde::gameplay::SwordContactTargetPair>(targetPairs));
            const auto torsoOnlyProxy = horde::gameplay::EvaluateSwordContactProxy(
                bladeCapsule, bladeCapsule,
                std::span<const horde::gameplay::SwordContactTargetPair>(
                    targetPairs.data(), 2u));
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
            Check(contact.attacking || !proxy.touching,
                  "idle control must remain outside all six individually fitted upper-body capsules");
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
                      << " sixCapsuleProxyGapMm=" << proxy.minimumSeparationMetres * 1000.0f
                      << " sixCapsuleProxyTouching=" << proxy.touching
                      << " torsoOnlyProxyGapMm=" << torsoOnlyProxy.minimumSeparationMetres * 1000.0f
                      << " torsoOnlyProxyTouching=" << torsoOnlyProxy.touching
                      << " torsoCapsuleRadiiMm=" << bodyCapsules[0].radiusMetres * 1000.0f
                      << "," << bodyCapsules[1].radiusMetres * 1000.0f
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

void RunMovingWitness(const bool downstroke)
{
    using namespace horde::gameplay;
    using namespace horde::gameplay::items;
    using horde::scene::SkinnedClip;
    using horde::scene::SkinnedMeshAsset;

    constexpr float tickSeconds = 1.0f / 60.0f;
    constexpr float startRange = 1.52f;
    constexpr std::size_t expectedBladeTriangles = 6905u;
    const std::size_t expectedQueries = downstroke ? 6u : 4u;
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
          "moving witness must load the production sword, Grip, target skin and player rig");
    if (!heldLoaded || grip == nullptr || !skeletonLoaded || !playerLoaded) return;

    struct Case { const char* name; float bearing; };
    constexpr std::array<Case, 2u> cases{{
        {"frontal-moving-1.52", 0.0f},
        {"minus15-moving-1.52", -0.261799388f},
    }};
    std::size_t exactQueries = 0u;
    std::size_t totalExactTrianglePairs = 0u;
    std::uint64_t rigTick = 191000u;

    for (const Case& contact : cases)
    {
        const float startX = startRange * std::sin(contact.bearing);
        const float startZ = -startRange * std::cos(contact.bearing);
        SwordCombat attack;
        SwordCombat control;
        attack.Reset(1u, {startX, startZ});
        control.Reset(1u, {startX, startZ});
        Check(attack.RequestAttack() == PlayerAttackCut::DownwardCut,
              "moving fixture must request the ordinary immediate downward cut");

        std::array<CombatSnapshot, 23u> attackAfter{};
        std::array<CombatSnapshot, 23u> controlAfter{};
        std::uint64_t pulseTick = 0u;
        for (std::uint64_t tick = 1u; tick <= attackAfter.size(); ++tick)
        {
            const auto index = static_cast<std::size_t>(tick - 1u);
            attackAfter[index] = attack.Update(tickSeconds, 0.0f, 0.0f, 0.0f, true, true, false);
            controlAfter[index] = control.Update(tickSeconds, 0.0f, 0.0f, 0.0f, true, true, false);
            if (attackAfter[index].playerAttackPulse) pulseTick = tick;
        }
        constexpr std::uint64_t expectedPulseTick = 17u;
        Check(pulseTick == expectedPulseTick,
              "moving witness must retain the existing tick-17 damage pulse");
        if (pulseTick != expectedPulseTick) continue;

        const auto& pulse = attackAfter[static_cast<std::size_t>(pulseTick - 1u)];
        const auto& beforePulse = attackAfter[static_cast<std::size_t>(pulseTick - 2u)];
        const auto& pulseTarget = controlAfter[static_cast<std::size_t>(pulseTick - 2u)].combatants[0u];
        const float movedMetres = std::hypot(pulseTarget.x - startX, pulseTarget.z - startZ);
        const float range = std::hypot(pulseTarget.x, pulseTarget.z);
        const float facingDot = range > 0.0001f ? -pulseTarget.z / range : 1.0f;
        const bool gate = SwordCombat::IsPlayerTargetInRangeCone(
            0.0f, 0.0f, 0.0f, pulseTarget.x, pulseTarget.z);
        const bool admitted = pulse.playerAttackPulse &&
            pulse.combatants[0u].health < beforePulse.combatants[0u].health;
        Check(movedMetres > 0.15f && movedMetres < 0.18f,
              "pulse authority target must have moved approximately sixteen ordinary walking steps");
        Check(pulseTarget.action == EnemyCombatAction::Locomotion &&
                  pulseTarget.animation == EnemyAnimation::Walking,
              "pulse authority target must remain in its ordinary walking phase");
        Check(admitted && gate,
              "moving pulse must be admitted by the unchanged production range/cone gate");

        const std::array<std::uint64_t, 3u> downstrokeTicks{{19u, 21u, 23u}};
        const std::array<std::uint64_t, 2u> pulseTicks{{16u, expectedPulseTick}};
        const std::size_t sampleCount = downstroke ? downstrokeTicks.size() : pulseTicks.size();
        for (std::size_t sampleIndex = 0u; sampleIndex < sampleCount; ++sampleIndex)
        {
            const std::uint64_t sampleTick = downstroke
                ? downstrokeTicks[sampleIndex] : pulseTicks[sampleIndex];
            const auto& combatSnapshot = attackAfter[static_cast<std::size_t>(sampleTick - 1u)];
            const std::uint64_t targetAuthorityTick = sampleTick - 1u;
            const auto& targetSnapshot = controlAfter[
                static_cast<std::size_t>(targetAuthorityTick - 1u)].combatants[0u];
            const auto targetSample = ResolveSharedCharacterRenderPose(
                targetSnapshot, skeleton.ClipDuration(SkinnedClip::Dead));
            if (downstroke)
            {
                Check(targetSnapshot.health > 0 &&
                          targetSnapshot.action != EnemyCombatAction::Dead &&
                          (targetSample.clip == SkinnedClip::Walking ||
                           targetSample.clip == SkinnedClip::Attack),
                      "late downstroke control must report a living ordinary controller pose");
                Check(combatSnapshot.player.action == PlayerCombatAction::SwingActive,
                      "late downstroke samples must remain on the existing downward swing action");
            }
            else
            {
                Check(targetSample.clip == SkinnedClip::Walking &&
                          targetSnapshot.action == EnemyCombatAction::Locomotion &&
                          targetSnapshot.animation == EnemyAnimation::Walking,
                      "both moving-pulse samples must use the ordinary walking target pose");
            }

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
                    items, input, sampleTick, fixed, diagnostic) &&
                ResolveProductionAnatomicalSword(input, fixed.kinematics, items,
                    playerRig, rigTick++, worldFromGrip, worldFromSword, diagnostic);
            const auto blade = resolved
                ? BuildGripFilteredBladeTriangles(sword, *grip, worldFromSword)
                : std::vector<MeshTriangle>{};
            std::vector<horde::scene::SkinnedRtVertex> targetPose;
            const bool skinned = skeleton.Skin(targetSample.clip, targetSample.clipTime,
                                                targetPose, diagnostic);
            const auto targetTriangles = skinned
                ? BuildDiagnosticTargetTriangles(targetSample, targetPose)
                : std::vector<MeshTriangle>{};
            Check(resolved && skinned && blade.size() == expectedBladeTriangles &&
                      !targetTriangles.empty(),
                  "moving-pulse sample must use the full production blade and walking target skin");
            if (!resolved || !skinned || blade.size() != expectedBladeTriangles ||
                targetTriangles.empty()) continue;

            const ContactRegionTargetTree targetTree(targetTriangles);
            const ContactRegionQuery query = targetTree.Query(blade);
            ++exactQueries;
            totalExactTrianglePairs += query.exactTrianglePairs;
            Check(query.targetTriangle < targetPose.size() / 3u &&
                      query.bladeTriangle < blade.size() && query.exactTrianglePairs > 0u &&
                      std::isfinite(query.metres) && query.metres >= 0.0f,
                  "moving-pulse query must return a finite exact nearest triangle pair");
            if (query.targetTriangle >= targetPose.size() / 3u) continue;

            const RegionLabel region = LabelByNearestAuthoredSegment(
                targetTriangles[query.targetTriangle], targetSample, skeleton, diagnostic);
            const auto triangleCentroid = [](const MeshTriangle& triangle)
            {
                return std::array<float, 3u>{{
                    (triangle.a[0] + triangle.b[0] + triangle.c[0]) / 3.0f,
                    (triangle.a[1] + triangle.b[1] + triangle.c[1]) / 3.0f,
                    (triangle.a[2] + triangle.b[2] + triangle.c[2]) / 3.0f,
                }};
            };
            const auto bladeCentroid = triangleCentroid(blade[query.bladeTriangle]);
            const auto targetCentroid = triangleCentroid(targetTriangles[query.targetTriangle]);
            const float sampleRange = std::hypot(targetSnapshot.x, targetSnapshot.z);
            const float sampleDot = sampleRange > 0.0001f ? -targetSnapshot.z / sampleRange : 1.0f;
            const bool sampleGate = SwordCombat::IsPlayerTargetInRangeCone(
                0.0f, 0.0f, 0.0f, targetSnapshot.x, targetSnapshot.z);
            const float sampleMoved = std::hypot(targetSnapshot.x - startX,
                                                 targetSnapshot.z - startZ);
            const char* strokePhase = combatSnapshot.player.actionTime <
                    SwordCombat::kDownwardCutTravelDuration
                ? "downstroke" : "bottom-hold";
            std::cout << (downstroke ? "sword-moving-downstroke case=" : "sword-moving-pulse case=")
                      << contact.name
                      << " sampleTick=" << sampleTick
                      << " targetAuthorityTick=" << targetAuthorityTick
                      << " playerAction=" << static_cast<int>(combatSnapshot.player.action)
                      << " playerActionTime=" << combatSnapshot.player.actionTime
                      << " visualStrokePhase=" << strokePhase
                      << " targetAction=" << static_cast<int>(targetSnapshot.action)
                      << " targetAnimation=" << static_cast<int>(targetSnapshot.animation)
                      << " targetActionTime=" << targetSnapshot.actionTime
                      << " targetAnimationTime=" << targetSnapshot.animationTime
                      << " targetX=" << targetSnapshot.x << " targetZ=" << targetSnapshot.z
                      << " startX=" << startX << " startZ=" << startZ
                      << " movedMm=" << sampleMoved * 1000.0f
                      << " rangeM=" << sampleRange << " facingDot=" << sampleDot
                      << " gateEligible=" << sampleGate
                      << " pulse17Admitted=" << admitted
                      << " sampleIsNewHitAdmission="
                      << (!downstroke && sampleTick == expectedPulseTick && admitted)
                      << " targetPoseCounterfactual=" << downstroke
                      << " meshGapMm=" << query.metres * 1000.0f
                      << " targetTriangle=" << query.targetTriangle
                      << " bladeTriangle=" << query.bladeTriangle
                      << " exactTrianglePairs=" << query.exactTrianglePairs
                      << " bladeTriangleCentroidM=" << bladeCentroid[0] << ','
                      << bladeCentroid[1] << ',' << bladeCentroid[2]
                      << " targetTriangleCentroidM=" << targetCentroid[0] << ','
                      << targetCentroid[1] << ',' << targetCentroid[2]
                      << " targetMinusBladeCentroidDeltaMm="
                      << (targetCentroid[0] - bladeCentroid[0]) * 1000.0f << ','
                      << (targetCentroid[1] - bladeCentroid[1]) * 1000.0f << ','
                      << (targetCentroid[2] - bladeCentroid[2]) * 1000.0f
                      << " centroidDeltaIsNotNearestSurfaceVector=1"
                      << " bladeTriangles=" << blade.size()
                      << " targetTriangles=" << targetPose.size() / 3u
                      << " nearestAuthoredSkeletonSegment=" << region.segment
                      << " triangleCentroidToSegmentMm="
                      << region.centroidDistanceMetres * 1000.0f
                      << " regionLabelSource=actualJointNodeTransforms_nearestCentroid"
                      << " sourceJointWeightsExposedByRuntimeAssetApi=0\n";
        }
    }

    Check(exactQueries == expectedQueries,
          "moving witness mode must complete its exact bounded full-mesh query budget");
    std::cout << (downstroke ? "sword-moving-downstroke-summary exactQueries="
                             : "sword-moving-pulse-summary exactQueries=") << exactQueries
              << " queryBudget=" << expectedQueries
              << " exactBladeTargetTrianglePairs=" << totalExactTrianglePairs
              << " startingRangeM=" << startRange
              << (downstroke
                    ? " samplePolicy=ticks19_21_23_prior-target-authority_actual-target-dead-after17"
                    : " samplePolicy=tick16-and-pulse17_prior-target-authority")
              << " contactInterpretation=zero-gap-agrees_positive-gap-is-mismatch-evidence"
              << " continuousCollisionClaim=0 presentedFrameClaim=0"
              << " labelAuthority=nearest_actual_imported_skeleton_joint_segment"
              << " vertexWeightClassification=unavailable\n";
}

void TestMovingPulseWitness()
{
    RunMovingWitness(false);
}

void TestMovingDownstrokeWitness()
{
    RunMovingWitness(true);
}

horde::gameplay::SwordContactCapsule BuildMeasuredBladeCapsule(
    const horde::scene::assets::StaticMeshAsset& sword,
    const horde::scene::assets::StaticSocket& grip,
    const HeldItemTransform& worldFromGrip)
{
    WorldBounds gripBounds;
    for (const auto& vertex : sword.vertices)
    {
        const auto point = ItemVertexToGripLocal(
            {{vertex.position[0], vertex.position[1], vertex.position[2]}}, grip.world);
        if (point[1] > 0.15f) IncludePoint(gripBounds, point);
    }
    const float centerX = 0.5f * (gripBounds.minimum[0] + gripBounds.maximum[0]);
    const float centerZ = 0.5f * (gripBounds.minimum[2] + gripBounds.maximum[2]);
    float radius = 0.0f;
    for (const auto& vertex : sword.vertices)
    {
        const auto point = ItemVertexToGripLocal(
            {{vertex.position[0], vertex.position[1], vertex.position[2]}}, grip.world);
        if (point[1] <= 0.15f) continue;
        radius = std::max(radius, std::hypot(point[0] - centerX, point[2] - centerZ));
    }
    const std::array<float, 3u> localStart{{centerX, gripBounds.minimum[1], centerZ}};
    const std::array<float, 3u> localEnd{{centerX, gripBounds.maximum[1], centerZ}};
    return {TransformPoint(worldFromGrip, localStart),
            TransformPoint(worldFromGrip, localEnd), radius};
}

horde::gameplay::SwordContactCapsule BuildMeasuredSkeletonCapsule(
    const std::vector<FittedSkeletonCapsule>& capsules,
    const std::array<float, 12u>& targetTransform,
    const std::size_t segment)
{
    if (capsules.size() != 24u || segment >= capsules.size())
        return {};
    const auto& capsule = capsules[segment];
    return {ApplySkeletonTransform(targetTransform, capsule.start),
            ApplySkeletonTransform(targetTransform, capsule.end), capsule.radius};
}

std::array<horde::gameplay::SwordContactCapsule, 6u> BuildMeasuredUpperBodyProfile(
    const horde::scene::SkinnedMeshAsset& skeleton,
    const DiagnosticSkeletonRenderSample& sample,
    const std::vector<horde::scene::SkinnedRtVertex>& targetPose,
    std::string& diagnostic)
{
    auto capsules = FitRobustSkeletonCapsules(skeleton, sample, targetPose, diagnostic);
    if (capsules.size() != 24u) return {};
    // Six separately fitted actual parent/child skin capsules: two trunk,
    // two weapon-arm, and two neck/head. Do not merge bent joints into broad
    // enclosing capsules; that made an idle hand profile over half a metre.
    return {{BuildMeasuredSkeletonCapsule(capsules, sample.transform, 10u),
             BuildMeasuredSkeletonCapsule(capsules, sample.transform, 11u),
             BuildMeasuredSkeletonCapsule(capsules, sample.transform, 18u),
             BuildMeasuredSkeletonCapsule(capsules, sample.transform, 19u),
             BuildMeasuredSkeletonCapsule(capsules, sample.transform, 21u),
             BuildMeasuredSkeletonCapsule(capsules, sample.transform, 22u)}};
}

std::array<horde::gameplay::SwordContactCapsule, 24u> BuildThinImportedJointProfile(
    const horde::scene::SkinnedMeshAsset& skeleton,
    const DiagnosticSkeletonRenderSample& sample,
    std::string& diagnostic)
{
    std::vector<std::string_view> names;
    for (const JointSegment& segment : kImportedJointSegments)
        for (const char* name : {segment.start, segment.end})
            if (std::find(names.begin(), names.end(), name) == names.end())
                names.emplace_back(name);
    std::vector<horde::scene::SkinnedNodeTransform> transforms(names.size());
    if (!skeleton.NodeTransforms(sample.clip, sample.clipTime, names, transforms,
                                 diagnostic))
        return {};
    const auto worldPosition = [&](const char* name) {
        const auto found = std::find(names.begin(), names.end(), name);
        const auto& matrix = transforms[static_cast<std::size_t>(found - names.begin())];
        return ApplySkeletonTransform(sample.transform,
            {{matrix[12], matrix[13], matrix[14]}});
    };
    std::array<horde::gameplay::SwordContactCapsule, 24u> result{};
    for (std::size_t index = 0u; index < kImportedJointSegments.size(); ++index)
        result[index] = {worldPosition(kImportedJointSegments[index].start),
                         worldPosition(kImportedJointSegments[index].end), 0.025f};
    return result;
}

std::array<float, 3u> SkeletonWorldToLocal(
    const std::array<float, 12u>& transform,
    const std::array<float, 3u>& point)
{
    const float x = point[0] - transform[3];
    const float y = point[1] - transform[7];
    const float z = point[2] - transform[11];
    return {{transform[0] * x + transform[4] * y + transform[8] * z,
             transform[1] * x + transform[5] * y + transform[9] * z,
             transform[2] * x + transform[6] * y + transform[10] * z}};
}

std::vector<FittedSkeletonCapsule> FitRobustSkeletonCapsules(
    const horde::scene::SkinnedMeshAsset& skeleton,
    const DiagnosticSkeletonRenderSample& sample,
    const std::vector<horde::scene::SkinnedRtVertex>& targetPose,
    std::string& diagnostic)
{
    if (targetPose.empty() || targetPose.size() % 3u != 0u) return {};
    std::vector<std::string_view> names;
    for (const JointSegment& segment : kImportedJointSegments)
        for (const char* name : {segment.start, segment.end})
            if (std::find(names.begin(), names.end(), name) == names.end())
                names.emplace_back(name);
    std::vector<horde::scene::SkinnedNodeTransform> transforms(names.size());
    if (!skeleton.NodeTransforms(sample.clip, sample.clipTime, names, transforms,
                                 diagnostic))
        return {};
    const auto nodePosition = [&](const char* name) {
        const auto found = std::find(names.begin(), names.end(), name);
        const auto& matrix = transforms[static_cast<std::size_t>(found - names.begin())];
        return std::array<float, 3u>{{matrix[12], matrix[13], matrix[14]}};
    };
    std::vector<FittedSkeletonCapsule> result;
    result.reserve(kImportedJointSegments.size());
    for (const JointSegment& segment : kImportedJointSegments)
        result.push_back({nodePosition(segment.start), nodePosition(segment.end), 0.0f});

    std::array<std::vector<float>, 24u> distancesBySegment;
    for (const auto& vertex : targetPose)
    {
        const std::array<float, 3u> point{{vertex.position[0], vertex.position[1], vertex.position[2]}};
        std::size_t nearestSegment = 0u;
        float nearestSquared = std::numeric_limits<float>::max();
        for (std::size_t segmentIndex = 0u; segmentIndex < result.size(); ++segmentIndex)
        {
            const float distanceSquared = PointSegmentDistanceSquared(
                point, result[segmentIndex].start, result[segmentIndex].end);
            if (distanceSquared < nearestSquared)
            {
                nearestSquared = distanceSquared;
                nearestSegment = segmentIndex;
            }
        }
        distancesBySegment[nearestSegment].push_back(std::sqrt(nearestSquared));
    }
    // Robust radius is the 99th percentile of actual skinned vertices assigned
    // to the nearest imported joint edge. Per-triangle max fitting puts remote
    // corners from an adjacent limb onto one edge and produced false contacts.
    constexpr float quantile = 0.99f;
    for (std::size_t segmentIndex = 0u; segmentIndex < result.size(); ++segmentIndex)
    {
        auto& distances = distancesBySegment[segmentIndex];
        if (distances.empty()) continue;
        std::sort(distances.begin(), distances.end());
        const auto index = static_cast<std::size_t>(std::floor(
            quantile * static_cast<float>(distances.size() - 1u)));
        result[segmentIndex].radius = distances[index];
    }
    return result;
}

void RunContactBoundaryWitness()
{
    using namespace horde::gameplay;
    using namespace horde::gameplay::items;
    using horde::scene::SkinnedClip;
    using horde::scene::SkinnedMeshAsset;

    struct Case { const char* name; float range; float bearing; bool upward; bool moving; bool idle; bool gate; };
    const float coneEdge = std::acos(SwordCombat::kPlayerHitConeDot);
    const std::array<Case, 9u> cases{{
        {"upward-frontal-static-1.20", 1.20f, 0.0f, true, false, false, true},
        {"upward-minus15-static-1.20", 1.20f, -0.261799388f, true, false, false, true},
        // The target closes 165.3 mm before pulse 17 in this ordinary approach fixture.
        {"downward-range-inside-approach-1.71", 1.875f, 0.0f, false, false, false, true},
        {"downward-range-outside-approach-1.73", 1.895f, 0.0f, false, false, false, false},
        {"downward-cone-inside", 1.20f, coneEdge - 0.02f, false, false, false, true},
        {"downward-cone-outside", 1.20f, coneEdge + 0.02f, false, false, false, false},
        {"upward-frontal-moving-1.52", 1.52f, 0.0f, true, true, false, true},
        {"upward-minus15-moving-1.52", 1.52f, -0.261799388f, true, true, false, true},
        {"idle-frontal-negative-1.20", 1.20f, 0.0f, false, false, true, true},
    }};
    constexpr float tickSeconds = 1.0f / 60.0f;
    constexpr std::size_t bladeTriangleCount = 6905u;
    const horde::gameplay::SwordContactCapsule unitBlade{
        {{0.0f, 0.0f, 0.0f}}, {{0.0f, 1.0f, 0.0f}}, 0.05f};
    const std::array<horde::gameplay::SwordContactTargetPair, 2u> proxyContracts{{
        {9, {{{0.5f, 0.0f, 0.0f}}, {{0.5f, 1.0f, 0.0f}}, 0.05f},
            {{{0.08f, 0.0f, 0.0f}}, {{0.08f, 1.0f, 0.0f}}, 0.05f}, true},
        {3, {{{0.0f, 0.0f, 0.0f}}, {{0.0f, 1.0f, 0.0f}}, 1.0f},
            {{{0.0f, 0.0f, 0.0f}}, {{0.0f, 1.0f, 0.0f}}, 1.0f}, false},
    }};
    const auto proxyContractResult = horde::gameplay::EvaluateSwordContactProxy(
        unitBlade, unitBlade,
        std::span<const horde::gameplay::SwordContactTargetPair>(proxyContracts));
    Check(proxyContractResult.targetId == 9 && proxyContractResult.touching &&
              proxyContractResult.sample == horde::gameplay::SwordContactSample::Current &&
              std::abs(proxyContractResult.minimumSeparationMetres + 0.02f) < 0.0001f,
          "pure contact proxy must ignore ineligible targets and deterministically report nearest endpoint sample");
    const std::filesystem::path root = HORDE_RT_SOURCE_DIR;
    horde::scene::assets::StaticMeshAsset sword;
    horde::scene::assets::StaticMeshAsset torch;
    SkinnedMeshAsset skeleton;
    std::string diagnostic;
    const bool heldLoaded = LoadProductionHeldAssets(sword, torch, diagnostic);
    const auto* grip = FindHeldItemSocket(sword.sockets, "Grip");
    const bool skeletonLoaded = skeleton.LoadCombatClips(
        (root / "assets/models/enemies/meshy/skeleton_biped_merged_animations_v01.glb").string(), diagnostic);
    horde::vulkan::raytracing::PlayerRenderSlot playerRig;
    const bool playerLoaded = playerRig.LoadAsset(
        (root / "assets/models/player/runtime/gothic-traveller-lod0.runtime.glb").string(), diagnostic);
    Check(heldLoaded && grip != nullptr && !sword.vertices.empty() && skeletonLoaded && playerLoaded,
          "contact-boundary witness must load production sword/Grip, target skin and player rig");
    if (!heldLoaded || grip == nullptr || !skeletonLoaded || !playerLoaded) return;

    std::size_t exactQueries = 0u;
    std::size_t exactPairs = 0u;
    std::uint64_t rigTick = 193000u;
    for (const Case& contact : cases)
    {
        const float startX = contact.range * std::sin(contact.bearing);
        const float startZ = -contact.range * std::cos(contact.bearing);
        SwordCombat attack;
        SwordCombat control;
        attack.Reset(1u, {startX, startZ});
        control.Reset(1u, {startX, startZ});
        if (!contact.idle)
            Check(attack.RequestAttack() == PlayerAttackCut::DownwardCut,
                  "contact-boundary witness must start with the production downward cut");
        std::array<CombatSnapshot, 56u> controlAfter{};
        CombatSnapshot selectedPlayer{};
        SkeletonCombatantSnapshot selectedTarget{};
        bool upwardQueued = !contact.upward;
        bool upwardAccepted = !contact.upward;
        std::uint64_t pulseTick = 0u;
        for (std::uint64_t tick = 1u; tick <= controlAfter.size(); ++tick)
        {
            if (contact.upward && !upwardQueued &&
                attack.Snapshot().player.action == PlayerCombatAction::SwingActive)
            {
                upwardQueued = true;
                upwardAccepted = attack.RequestAttack() == PlayerAttackCut::UpwardSlice;
            }
            const CombatSnapshot attackAfter = attack.Update(
                tickSeconds, 0.0f, 0.0f, 0.0f, true, true, false);
            controlAfter[static_cast<std::size_t>(tick - 1u)] = control.Update(
                tickSeconds, 0.0f, 0.0f, 0.0f, true, true, false);
            if (contact.idle && tick == 23u)
            {
                pulseTick = tick;
                selectedPlayer = attackAfter;
                selectedTarget = controlAfter[static_cast<std::size_t>(tick - 2u)].combatants[0u];
                upwardAccepted = true;
                break;
            }
            const PlayerAttackCut wanted = contact.upward
                ? PlayerAttackCut::UpwardSlice : PlayerAttackCut::DownwardCut;
            if (!attackAfter.playerAttackPulse || attackAfter.playerAttackCut != wanted) continue;
            pulseTick = tick;
            selectedPlayer = attackAfter;
            // The hit query precedes target motion. Pair its player pose with the control's prior target pose.
            selectedTarget = controlAfter[static_cast<std::size_t>(tick > 1u ? tick - 2u : 0u)].combatants[0u];
            break;
        }
        Check(upwardAccepted && pulseTick > 0u,
              "requested stroke must reach its existing production contact pulse");
        if (!upwardAccepted || pulseTick == 0u) continue;

        const auto targetSample = ResolveSharedCharacterRenderPose(
            selectedTarget, skeleton.ClipDuration(SkinnedClip::Dead));
        HeldItemFixedStepInput input;
        input.playerX = 0.0f;
        input.playerZ = 0.0f;
        input.playerYawRadians = 0.0f;
        input.playerMountProfile = PlayerMountProfile::AnatomicalBody;
        input.playerCombat = selectedPlayer.player;
        input.logicalViewAspect = 1.0f;
        HeldItemStates items = MakeDefaultHeldItemStates();
        HeldItemFixedStepState fixed;
        HeldItemTransform worldFromGrip{};
        HeldItemTransform worldFromSword{};
        const bool resolved = ResolveHeldItemsFixedStep(items, input, pulseTick, fixed, diagnostic) &&
            ResolveProductionAnatomicalSword(input, fixed.kinematics, items,
                playerRig, rigTick++, worldFromGrip, worldFromSword, diagnostic);
        const auto blade = resolved ? BuildGripFilteredBladeTriangles(sword, *grip, worldFromSword)
                                    : std::vector<MeshTriangle>{};
        std::vector<horde::scene::SkinnedRtVertex> targetPose;
        const bool skinned = skeleton.Skin(targetSample.clip, targetSample.clipTime, targetPose, diagnostic);
        const auto targetTriangles = skinned ? BuildDiagnosticTargetTriangles(targetSample, targetPose)
                                             : std::vector<MeshTriangle>{};
        Check(resolved && skinned && blade.size() == bladeTriangleCount && !targetTriangles.empty(),
              "boundary pulse must resolve actual production sword and imported target pose");
        if (!resolved || !skinned || blade.size() != bladeTriangleCount || targetTriangles.empty()) continue;

        const ContactRegionQuery query = ContactRegionTargetTree(targetTriangles).Query(blade);
        const auto bladeCapsule = BuildMeasuredBladeCapsule(sword, *grip, worldFromGrip);
        const auto upperBodyCapsules = BuildMeasuredUpperBodyProfile(
            skeleton, targetSample, targetPose, diagnostic);
        std::array<horde::gameplay::SwordContactTargetPair, 6u> targetPairs{};
        for (std::size_t index = 0u; index < targetPairs.size(); ++index)
            targetPairs[index] = {1, upperBodyCapsules[index], upperBodyCapsules[index], true};
        const auto proxy = horde::gameplay::EvaluateSwordContactProxy(
            bladeCapsule, bladeCapsule,
            std::span<const horde::gameplay::SwordContactTargetPair>(targetPairs));
        std::array<float, 6u> proxyRegionGapMm{};
        for (std::size_t regionIndex = 0u; regionIndex < targetPairs.size(); ++regionIndex)
        {
            const auto regionProxy = horde::gameplay::EvaluateSwordContactProxy(
                bladeCapsule, bladeCapsule,
                std::span<const horde::gameplay::SwordContactTargetPair>(
                    targetPairs.data() + regionIndex, 1u));
            proxyRegionGapMm[regionIndex] = regionProxy.minimumSeparationMetres * 1000.0f;
        }
        const RegionLabel nearestRegion = LabelByNearestAuthoredSegment(
            targetTriangles[query.targetTriangle], targetSample, skeleton, diagnostic);
        ++exactQueries;
        exactPairs += query.exactTrianglePairs;
        const float targetRange = std::hypot(selectedTarget.x, selectedTarget.z);
        const float facingDot = targetRange > 0.0001f ? -selectedTarget.z / targetRange : 1.0f;
        const bool gate = SwordCombat::IsPlayerTargetInRangeCone(
            0.0f, 0.0f, 0.0f, selectedTarget.x, selectedTarget.z);
        Check(std::isfinite(query.metres) && query.targetTriangle < targetPose.size() / 3u &&
                  query.bladeTriangle < blade.size() && query.exactTrianglePairs > 0u,
              "boundary witness must return a finite exact imported-mesh gap");
        Check(!contact.idle || !proxy.touching,
              "idle negative control must remain outside the measured contact proxy");
        Check(gate == contact.gate, "boundary case must retain unchanged range/cone admission");
        std::cout << "sword-contact-boundary case=" << contact.name
                  << " pulseTick=" << pulseTick
                  << " cut=" << static_cast<int>(selectedPlayer.playerAttackCut)
                  << " playerAction=" << static_cast<int>(selectedPlayer.player.action)
                  << " playerActionTime=" << selectedPlayer.player.actionTime
                  << " targetAction=" << static_cast<int>(selectedTarget.action)
                  << " targetAnimation=" << static_cast<int>(selectedTarget.animation)
                  << " targetX=" << selectedTarget.x << " targetZ=" << selectedTarget.z
                  << " targetRangeM=" << targetRange << " facingDot=" << facingDot
                  << " movingTarget=" << (std::hypot(selectedTarget.x - startX,
                                                       selectedTarget.z - startZ) > 0.001f)
                  << " targetMovedMm=" << std::hypot(selectedTarget.x - startX, selectedTarget.z - startZ) * 1000.0f
                  << " gateEligible=" << gate << " expectedGate=" << contact.gate
                  << " exactMeshGapMm=" << query.metres * 1000.0f
                  << " upperBodyProxyGapMm=" << proxy.minimumSeparationMetres * 1000.0f
                  << " upperBodyProxyTouching=" << proxy.touching
                  << " bladeCapsuleRadiusMm=" << bladeCapsule.radiusMetres * 1000.0f
                  << " trunkLowerRadiusMm=" << upperBodyCapsules[0].radiusMetres * 1000.0f
                  << " trunkUpperRadiusMm=" << upperBodyCapsules[1].radiusMetres * 1000.0f
                  << " weaponArmUpperRadiusMm=" << upperBodyCapsules[2].radiusMetres * 1000.0f
                  << " weaponArmLowerRadiusMm=" << upperBodyCapsules[3].radiusMetres * 1000.0f
                  << " neckRadiusMm=" << upperBodyCapsules[4].radiusMetres * 1000.0f
                  << " headRadiusMm=" << upperBodyCapsules[5].radiusMetres * 1000.0f
                  << " trunkLowerGapMm=" << proxyRegionGapMm[0]
                  << " trunkUpperGapMm=" << proxyRegionGapMm[1]
                  << " weaponArmUpperGapMm=" << proxyRegionGapMm[2]
                  << " weaponArmLowerGapMm=" << proxyRegionGapMm[3]
                  << " neckGapMm=" << proxyRegionGapMm[4]
                  << " headGapMm=" << proxyRegionGapMm[5]
                  << " nearestAuthoredSegment=" << nearestRegion.segment
                  << " targetTriangle=" << query.targetTriangle << " bladeTriangle=" << query.bladeTriangle
                  << " exactTrianglePairs=" << query.exactTrianglePairs
                  << " targetTriangles=" << targetPose.size() / 3u << " bladeTriangles=" << blade.size()
                  << " targetPoseSource=actual_control_simulation_snapshot"
                  << " bladePoseSource=actual_anatomical_player_rig_Grip\n";
    }
    Check(exactQueries == cases.size(), "contact-boundary witness must complete its nine-query mesh budget");
    std::cout << "sword-contact-boundary-summary exactQueries=" << exactQueries
              << " queryBudget=" << cases.size() << " exactBladeTargetTrianglePairs=" << exactPairs
              << " containsUpwardStaticAndMoving=1 containsRangeAndConeEdgesAndIdle=1"
              << " continuousCollisionClaim=0 presentedFrameClaim=0 sourceJointWeightsExposedByRuntimeAssetApi=0\n";
}

void RunContactCallbackContractWitness()
{
    using namespace horde::gameplay;
    constexpr float tickSeconds = 1.0f / 60.0f;
    SwordCombat combat;
    combat.Reset(1u, {0.0f, -1.28f}, nullptr, 2);
    Check(combat.RequestAttack() == PlayerAttackCut::DownwardCut,
          "callback witness starts with the existing downward attack request");
    struct Sample { PlayerAttackCut cut; std::uint64_t id; float elapsed; std::int32_t targetHealth; };
    std::vector<Sample> samples;
    std::vector<CombatSnapshot> contacts;
    const PlayerSwordContactEvaluator evaluator =
        [&samples](const PlayerCombatSnapshot& player,
                   const PlayerAttackCut cut,
                   const std::uint64_t attackId,
                   const CombatSnapshot& current) -> std::optional<std::int32_t> {
        Check(current.activePlayerAttackId == attackId,
              "active combat snapshot must publish the callback's current cut ID");
        samples.push_back({cut, attackId, player.actionTime,
                               current.combatants[0].health});
            if ((cut == PlayerAttackCut::DownwardCut && player.actionTime >= 0.12f) ||
                cut == PlayerAttackCut::UpwardSlice)
                return 0;
            return std::nullopt;
        };
    bool queuedUpward = false;
    for (std::uint64_t tick = 1u; tick <= 80u; ++tick)
    {
        if (!queuedUpward && combat.Snapshot().player.action == PlayerCombatAction::SwingActive)
        {
            queuedUpward = true;
            Check(combat.RequestAttack() == PlayerAttackCut::UpwardSlice,
                  "callback witness buffers the existing upward continuation");
        }
        const CombatSnapshot& current = combat.Update(
            tickSeconds, 0.0f, 0.0f, 0.0f, true, true, false, evaluator);
        if (current.playerContactPulse) contacts.push_back(current);
        if (contacts.size() == 2u) break;
    }
    Check(samples.size() > 2u,
          "contact evaluator must sample active fixed ticks rather than one authored pulse only");
    Check(contacts.size() == 2u,
          "callback witness must accept one downward and one upward contact");
    if (contacts.size() == 2u)
    {
        Check(contacts[0].playerContactCut == PlayerAttackCut::DownwardCut &&
                  contacts[1].playerContactCut == PlayerAttackCut::UpwardSlice &&
                  contacts[0].playerContactAttackId != contacts[1].playerContactAttackId,
              "each cut must carry a distinct stable attack id");
        Check(contacts[0].playerContactTargetId == 0 &&
                  contacts[0].combatants[0].health == 1 &&
                  contacts[0].combatants[0].action == EnemyCombatAction::Staggered,
              "first accepted contact must be nonlethal and stagger a two-hit target");
        Check(contacts[1].playerContactTargetId == 0 &&
                  contacts[1].combatants[0].health == 0 &&
                  contacts[1].combatants[0].action == EnemyCombatAction::Dead &&
                  !contacts[1].combatants[0].playerHitPulse,
              "lethal contact must suppress a same-tick incoming-hit pulse");
    }
    std::cout << "sword-contact-callback-summary activeSamples=" << samples.size()
              << " acceptedContacts=" << contacts.size()
              << " separateCutAttackIds=" << (contacts.size() == 2u ? 1 : 0)
              << " nonfatalThenFatal=1 callbacksAfterTargetUpdate=1 legacyWhenCallbackEmpty=default\n";

    SwordCombat singleCombatant;
    singleCombatant.Reset(1u, {0.0f, -1.28f}, nullptr, 2);
    Check(singleCombatant.RequestAttack() == PlayerAttackCut::DownwardCut,
          "single-combatant callback validation starts a downward cut");
    std::size_t singleCombatantCalls = 0u;
    const PlayerSwordContactEvaluator singleCombatantEvaluator =
        [&singleCombatantCalls](const PlayerCombatSnapshot&,
                                const PlayerAttackCut,
                                const std::uint64_t,
                                const CombatSnapshot&) -> std::optional<std::int32_t> {
            ++singleCombatantCalls;
            return singleCombatantCalls == 1u ? 1 : 0;
        };
    std::optional<CombatSnapshot> singleCombatantContact;
    for (std::uint64_t tick = 1u; tick <= 80u && !singleCombatantContact; ++tick)
    {
        const CombatSnapshot& current = singleCombatant.Update(
            tickSeconds, 0.0f, 0.0f, 0.0f, true, true, false,
            singleCombatantEvaluator);
        if (current.playerContactPulse) singleCombatantContact = current;
    }
    Check(singleCombatantCalls >= 2u,
          "a rejected nonexistent skeleton ID must leave the active cut eligible for a later valid contact");
    Check(singleCombatantContact && singleCombatantContact->playerContactTargetId == 0 &&
              singleCombatantContact->combatants[0].health == 1,
          "single-combatant callback must reject nonexistent skeleton ID 1 and accept existing ID 0");
    if (singleCombatantContact)
        Check(singleCombatantContact->activePlayerAttackId ==
                  singleCombatantContact->playerContactAttackId,
              "snapshot must publish the active cut ID alongside its accepted contact");
    bool observedIdleAttackId = false;
    for (std::uint64_t tick = 81u; tick <= 160u; ++tick)
    {
        const CombatSnapshot& current = singleCombatant.Update(
            tickSeconds, 0.0f, 0.0f, 0.0f, true, true, false,
            singleCombatantEvaluator);
        if (current.player.action == PlayerCombatAction::Idle)
        {
            observedIdleAttackId = true;
            Check(current.activePlayerAttackId == 0u,
                  "snapshot must clear the cut ID when the player returns to idle");
            break;
        }
    }
    Check(observedIdleAttackId,
          "single-combatant cut must eventually return to idle in the bounded ID witness");

    SwordCombat keeperContact;
    keeperContact.Reset(1u, {0.0f, -1.28f}, nullptr, 2);
    Check(keeperContact.RequestAttack() == PlayerAttackCut::DownwardCut,
          "keeper callback validation starts a downward cut");
    const PlayerSwordContactEvaluator keeperEvaluator =
        [](const PlayerCombatSnapshot&, const PlayerAttackCut, const std::uint64_t,
           const CombatSnapshot&) -> std::optional<std::int32_t> { return 2; };
    std::optional<CombatSnapshot> keeperPulse;
    for (std::uint64_t tick = 1u; tick <= 80u && !keeperPulse; ++tick)
    {
        const CombatSnapshot& current = keeperContact.Update(
            tickSeconds, 0.0f, 0.0f, 0.0f, true, true, false, keeperEvaluator);
        if (current.playerContactPulse) keeperPulse = current;
    }
    Check(keeperPulse && keeperPulse->playerContactTargetId == 2 &&
              keeperPulse->combatants[0].health == 2,
          "shared encounter keeper ID 2 remains valid without a second skeleton combatant");

    SwordCombat skipDeadTarget;
    skipDeadTarget.Reset(2u, {0.0f, -1.28f}, nullptr, 1);
    Check(skipDeadTarget.RequestAttack() == PlayerAttackCut::DownwardCut,
          "dead-target callback witness starts with a downward cut");
    std::size_t upwardCallbackCalls = 0u;
    bool queuedFollowup = false;
    std::vector<CombatSnapshot> liveTargetContacts;
    const PlayerSwordContactEvaluator skipDeadEvaluator =
        [&upwardCallbackCalls](const PlayerCombatSnapshot&,
                               const PlayerAttackCut cut,
                               const std::uint64_t,
                               const CombatSnapshot&) -> std::optional<std::int32_t> {
            if (cut == PlayerAttackCut::DownwardCut) return 0;
            if (cut == PlayerAttackCut::UpwardSlice)
            {
                ++upwardCallbackCalls;
                return upwardCallbackCalls == 1u ? 0 : 1;
            }
            return std::nullopt;
        };
    for (std::uint64_t tick = 1u; tick <= 120u && liveTargetContacts.size() < 2u; ++tick)
    {
        if (!queuedFollowup &&
            skipDeadTarget.Snapshot().player.action == PlayerCombatAction::SwingActive)
        {
            queuedFollowup = true;
            Check(skipDeadTarget.RequestAttack() == PlayerAttackCut::UpwardSlice,
                  "dead-target callback witness buffers the upward continuation");
        }
        const CombatSnapshot& current = skipDeadTarget.Update(
            tickSeconds, 0.0f, 0.0f, 0.0f, true, true, false,
            skipDeadEvaluator);
        if (current.playerContactPulse) liveTargetContacts.push_back(current);
    }
    Check(liveTargetContacts.size() == 2u,
          "an invalid dead-target selection must not consume the follow-up cut before a live selection");
    Check(upwardCallbackCalls >= 2u,
          "a dead skeleton target must be rejected and retried during the same cut");
    if (liveTargetContacts.size() == 2u)
    {
        Check(liveTargetContacts[0].playerContactTargetId == 0 &&
                  liveTargetContacts[0].combatants[0].health == 0,
              "first cut kills skeleton 0 so the follow-up begins with a dead skeleton");
        Check(liveTargetContacts[1].playerContactTargetId == 1 &&
                  liveTargetContacts[1].combatants[1].health == 0 &&
                  liveTargetContacts[0].playerContactAttackId !=
                      liveTargetContacts[1].playerContactAttackId,
              "follow-up cut must bypass dead skeleton 0 and accept live skeleton 1");
    }

    SwordCombat parry;
    parry.Reset(1u, {0.0f, -1.28f});
    parry.RequestParry();
    const CombatSnapshot& parrySnapshot = parry.Update(
        tickSeconds, 0.0f, 0.0f, 0.0f, true, true, false);
    Check(parrySnapshot.player.action == PlayerCombatAction::ParryStartup &&
              parrySnapshot.activePlayerAttackId == 0u,
          "parry actions must not publish a stale cut attack ID");
}

void RunInvalidContactProxyInputWitness()
{
    using namespace horde::gameplay;
    const SwordContactCapsule origin{{{0.0f, 0.0f, 0.0f}},
                                     {{0.0f, 1.0f, 0.0f}}, 0.05f};
    const SwordContactCapsule farBlade{{{4.0f, 0.0f, 0.0f}},
                                       {{4.0f, 1.0f, 0.0f}}, 0.05f};
    const SwordContactCapsule farTarget{{{7.0f, 0.0f, 0.0f}},
                                        {{7.0f, 1.0f, 0.0f}}, 0.05f};
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();
    const SwordContactCapsule nanPoint{{{nan, 0.0f, 0.0f}},
                                       {{nan, 1.0f, 0.0f}}, 0.05f};
    const SwordContactCapsule infinitePoint{{{infinity, 0.0f, 0.0f}},
                                            {{infinity, 1.0f, 0.0f}}, 0.05f};
    const SwordContactCapsule negativeRadius{{{0.0f, 0.0f, 0.0f}},
                                             {{0.0f, 1.0f, 0.0f}}, -0.05f};
    const SwordContactCapsule nanRadius{{{0.0f, 0.0f, 0.0f}},
                                        {{0.0f, 1.0f, 0.0f}}, nan};
    const SwordContactCapsule infiniteRadius{{{0.0f, 0.0f, 0.0f}},
                                             {{0.0f, 1.0f, 0.0f}}, infinity};
    const float largest = std::numeric_limits<float>::max();
    const SwordContactCapsule overflowingSegment{
        {{-largest, 0.0f, 0.0f}}, {{largest, 1.0f, 0.0f}}, 0.05f};
    std::size_t checked = 0u;
    const auto expectNoTouch = [&checked](const char* label,
                                          const SwordContactCapsule& previousBlade,
                                          const SwordContactCapsule& currentBlade,
                                          const SwordContactCapsule& previousTarget,
                                          const SwordContactCapsule& currentTarget) {
        const std::array<SwordContactTargetPair, 1u> targets{{
            {0, previousTarget, currentTarget, true}}};
        const SwordContactProxyResult result = EvaluateSwordContactProxy(
            previousBlade, currentBlade, targets);
        Check(!result.touching && !std::isnan(result.minimumSeparationMetres),
              label);
        ++checked;
    };
    expectNoTouch("nonfinite previous blade endpoint must not fabricate contact",
                  nanPoint, farBlade, origin, farTarget);
    expectNoTouch("nonfinite current blade endpoint must not fabricate contact",
                  farBlade, infinitePoint, farTarget, origin);
    expectNoTouch("nonfinite previous target endpoint must not fabricate contact",
                  origin, farBlade, nanPoint, farTarget);
    expectNoTouch("negative current target radius must not fabricate contact",
                  farBlade, origin, farTarget, negativeRadius);
    expectNoTouch("negative current blade radius must not fabricate contact",
                  farBlade, negativeRadius, farTarget, origin);
    expectNoTouch("NaN target radius must not fabricate contact",
                  farBlade, origin, farTarget, nanRadius);
    expectNoTouch("infinite current target endpoint must not fabricate contact",
                  farBlade, origin, farTarget, infinitePoint);
    expectNoTouch("infinite current target radius must not fabricate contact",
                  farBlade, origin, farTarget, infiniteRadius);
    expectNoTouch("finite but overflowing segment arithmetic must not fabricate contact",
                  overflowingSegment, farBlade, overflowingSegment, farTarget);

    const std::array<SwordContactTargetPair, 1u> validCurrentTarget{{
        {7, origin, origin, true}}};
    const SwordContactProxyResult validCurrentContact = EvaluateSwordContactProxy(
        nanPoint, origin, validCurrentTarget);
    Check(validCurrentContact.touching && validCurrentContact.targetId == 7 &&
              validCurrentContact.sample == SwordContactSample::Current,
          "a valid contacting current sample must remain usable when the previous blade sample is malformed");
    ++checked;

    const SwordContactCapsule zeroRadiusPoint{{{0.0f, 0.0f, 0.0f}},
                                              {{0.0f, 0.0f, 0.0f}}, 0.0f};
    const std::array<SwordContactTargetPair, 1u> pointTarget{{
        {0, zeroRadiusPoint, zeroRadiusPoint, true}}};
    const SwordContactProxyResult pointContact = EvaluateSwordContactProxy(
        zeroRadiusPoint, zeroRadiusPoint, pointTarget);
    Check(pointContact.touching && pointContact.minimumSeparationMetres == 0.0f,
          "valid zero-radius point segments must retain exact coincident contact");
    ++checked;
    std::cout << "sword-contact-proxy-invalid-input-summary cases=" << checked
              << " malformedInputsRejected=1 zeroRadiusPointSupported=1\n";
}

void RunThinJointActiveWindowWitness()
{
    using namespace horde::gameplay;
    using namespace horde::gameplay::items;
    using horde::scene::SkinnedClip;
    using horde::scene::SkinnedMeshAsset;
    struct Case
    {
        const char* name;
        float range;
        float bearing;
        bool upward;
        bool idle;
    };
    constexpr std::array<Case, 7u> cases{{
        {"down-frontal-1.28", 1.28f, 0.0f, false, false},
        {"down-minus15-1.28", 1.28f, -0.261799388f, false, false},
        {"up-frontal-1.20", 1.20f, 0.0f, true, false},
        {"up-minus15-1.20", 1.20f, -0.261799388f, true, false},
        {"up-frontal-moving-1.52", 1.52f, 0.0f, true, false},
        {"up-minus15-moving-1.52", 1.52f, -0.261799388f, true, false},
        {"idle-frontal-1.20", 1.20f, 0.0f, true, true},
    }};
    constexpr float tickSeconds = 1.0f / 60.0f;
    constexpr std::uint64_t maxTick = 38u;
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
          "thin-joint window witness loads actual production sword, target skin and player rig");
    if (!heldLoaded || grip == nullptr || !skeletonLoaded || !playerLoaded) return;

    std::size_t exactQueries = 0u;
    std::size_t thinJointTouchSamples = 0u;
    std::size_t thinJointAcceptedContacts = 0u;
    std::size_t thinJointFalseAccepts = 0u;
    std::size_t thinJointMissedNearContacts = 0u;
    std::size_t rootConeOnlyPulses = 0u;
    std::size_t movingRootConeOnlyPulses = 0u;
    std::uint64_t rigTick = 201000u;
    for (const Case& contact : cases)
    {
        const float startX = contact.range * std::sin(contact.bearing);
        const float startZ = -contact.range * std::cos(contact.bearing);
        SwordCombat attack;
        SwordCombat control;
        attack.Reset(1u, {startX, startZ});
        control.Reset(1u, {startX, startZ});
        if (!contact.idle)
            Check(attack.RequestAttack() == PlayerAttackCut::DownwardCut,
                  "active-window case starts with production downward cut");
        std::array<CombatSnapshot, maxTick> attackAfter{};
        std::array<CombatSnapshot, maxTick> controlAfter{};
        bool upwardQueued = !contact.upward;
        std::uint64_t firstActiveTick = 0u;
        for (std::uint64_t tick = 1u; tick <= maxTick; ++tick)
        {
            if (contact.upward && !contact.idle && !upwardQueued &&
                attack.Snapshot().player.action == PlayerCombatAction::SwingActive)
            {
                upwardQueued = true;
                Check(attack.RequestAttack() == PlayerAttackCut::UpwardSlice,
                      "upward active-window case queues the supported continuation");
            }
            attackAfter[static_cast<std::size_t>(tick - 1u)] = attack.Update(
                tickSeconds, 0.0f, 0.0f, 0.0f, true, true, false);
            controlAfter[static_cast<std::size_t>(tick - 1u)] = control.Update(
                tickSeconds, 0.0f, 0.0f, 0.0f, true, true, false);
            const auto action = attackAfter[static_cast<std::size_t>(tick - 1u)].player.action;
            const bool selectedAction = contact.upward
                ? action == PlayerCombatAction::UpwardSliceActive
                : action == PlayerCombatAction::SwingActive;
            if (selectedAction && firstActiveTick == 0u) firstActiveTick = tick;
        }
        Check(contact.idle || firstActiveTick > 0u,
              "each attack active-window profile reaches its authored active action");

        const std::uint64_t firstTick = contact.upward ? 27u : 13u;
        const std::uint64_t lastTick = contact.upward ? 38u : 24u;
        for (std::uint64_t tick = firstTick; tick <= lastTick; ++tick)
        {
            const auto& playerSnapshot = contact.idle
                ? controlAfter[static_cast<std::size_t>(tick - 1u)]
                : attackAfter[static_cast<std::size_t>(tick - 1u)];
            const SkeletonCombatantSnapshot& targetSnapshot =
                controlAfter[static_cast<std::size_t>(tick - 1u)].combatants[0u];
            const auto targetSample = ResolveSharedCharacterRenderPose(
                targetSnapshot, skeleton.ClipDuration(SkinnedClip::Dead));
            HeldItemFixedStepInput input;
            input.playerX = 0.0f;
            input.playerZ = 0.0f;
            input.playerYawRadians = 0.0f;
            input.playerMountProfile = PlayerMountProfile::AnatomicalBody;
            input.playerCombat = playerSnapshot.player;
            input.readableCombatPose = true;
            input.logicalViewAspect = 1.0f;
            HeldItemStates items = MakeDefaultHeldItemStates();
            HeldItemFixedStepState fixed;
            HeldItemTransform worldFromGrip{};
            HeldItemTransform worldFromSword{};
            const bool resolved = ResolveHeldItemsFixedStep(items, input, tick, fixed, diagnostic) &&
                ResolveProductionAnatomicalSword(input, fixed.kinematics, items,
                    playerRig, rigTick++, worldFromGrip, worldFromSword, diagnostic);
            const auto blade = resolved
                ? BuildGripFilteredBladeTriangles(sword, *grip, worldFromSword)
                : std::vector<MeshTriangle>{};
            std::vector<horde::scene::SkinnedRtVertex> targetPose;
            const bool skinned = skeleton.Skin(targetSample.clip, targetSample.clipTime,
                                               targetPose, diagnostic);
            const auto targetTriangles = skinned
                ? BuildDiagnosticTargetTriangles(targetSample, targetPose)
                : std::vector<MeshTriangle>{};
            Check(resolved && skinned && blade.size() == expectedBladeTriangles &&
                      !targetTriangles.empty(),
                  "active-window sample uses the actual full blade and imported target mesh");
            if (!resolved || !skinned || blade.size() != expectedBladeTriangles ||
                targetTriangles.empty())
                continue;
            const ContactRegionQuery exact = ContactRegionTargetTree(targetTriangles).Query(blade);
            Check(exact.targetTriangle < targetTriangles.size() &&
                      std::isfinite(exact.metres),
                  "active-window query returns a finite closest target triangle");
            if (exact.targetTriangle >= targetTriangles.size()) continue;
            const auto bladeCapsule = BuildMeasuredBladeCapsule(sword, *grip, worldFromGrip);
            const auto thinJointCapsules = BuildThinImportedJointProfile(
                skeleton, targetSample, diagnostic);
            std::array<SwordContactTargetPair, 24u> targetPairs{};
            for (std::size_t index = 0u; index < targetPairs.size(); ++index)
                targetPairs[index] = {1, thinJointCapsules[index],
                                      thinJointCapsules[index], true};
            std::array<bool, 24u> touchingJoint{};
            std::string touchingJointNames;
            for (std::size_t index = 0u; index < targetPairs.size(); ++index)
            {
                const auto segmentContact = EvaluateSwordContactProxy(
                    bladeCapsule, bladeCapsule,
                    std::span<const SwordContactTargetPair>(targetPairs.data() + index, 1u));
                touchingJoint[index] = segmentContact.touching;
                if (segmentContact.touching)
                {
                    if (!touchingJointNames.empty()) touchingJointNames += ',';
                    touchingJointNames += kImportedJointSegments[index].name;
                }
            }
            const auto thinJoint = EvaluateSwordContactProxy(
                bladeCapsule, bladeCapsule,
                std::span<const SwordContactTargetPair>(targetPairs));
            constexpr float targetBoneRadiusMetres = 0.025f;
            const float exactContactLimit = bladeCapsule.radiusMetres + targetBoneRadiusMetres;
            const bool meshWithinProxyReach = exact.metres <= exactContactLimit;
            ++exactQueries;
            const float range = std::hypot(targetSnapshot.x, targetSnapshot.z);
            const bool gate = SwordCombat::IsPlayerTargetInRangeCone(
                0.0f, 0.0f, 0.0f, targetSnapshot.x, targetSnapshot.z);
            const bool activeAttack = !contact.idle &&
                (playerSnapshot.player.action == PlayerCombatAction::SwingActive ||
                 playerSnapshot.player.action == PlayerCombatAction::UpwardSliceActive);
            const bool targetMoved = std::hypot(targetSnapshot.x - startX,
                                                targetSnapshot.z - startZ) > 0.001f;
            const bool accepted = activeAttack && gate && thinJoint.touching;
            const bool falseAccept = accepted && exact.metres > exactContactLimit;
            const bool missedNear = activeAttack && gate && meshWithinProxyReach &&
                                    !thinJoint.touching;
            const bool rootConeOnlyPulse = activeAttack && playerSnapshot.playerAttackPulse &&
                                           gate && !thinJoint.touching;
            thinJointTouchSamples += thinJoint.touching ? 1u : 0u;
            thinJointAcceptedContacts += accepted ? 1u : 0u;
            thinJointFalseAccepts += falseAccept ? 1u : 0u;
            thinJointMissedNearContacts += missedNear ? 1u : 0u;
            rootConeOnlyPulses += rootConeOnlyPulse ? 1u : 0u;
            movingRootConeOnlyPulses += rootConeOnlyPulse && targetMoved ? 1u : 0u;
            const RegionLabel nearestRegion = LabelByNearestAuthoredSegment(
                targetTriangles[exact.targetTriangle], targetSample, skeleton, diagnostic);
            std::cout << "sword-thin-joint-active-window case=" << contact.name
                      << " tick=" << tick
                      << " activeStartTick=" << firstActiveTick
                      << " cut=" << static_cast<int>(contact.upward
                          ? PlayerAttackCut::UpwardSlice : PlayerAttackCut::DownwardCut)
                      << " playerAction=" << static_cast<int>(playerSnapshot.player.action)
                      << " playerActionTime=" << playerSnapshot.player.actionTime
                      << " targetAction=" << static_cast<int>(targetSnapshot.action)
                      << " targetActionTime=" << targetSnapshot.actionTime
                      << " rangeM=" << range << " gateEligible=" << gate
                      << " exactMeshGapMm=" << exact.metres * 1000.0f
                      << " bladeRadiusMm=" << bladeCapsule.radiusMetres * 1000.0f
                      << " fixedBoneRadiusMm=" << targetBoneRadiusMetres * 1000.0f
                      << " exactContactLimitMm=" << exactContactLimit * 1000.0f
                      << " thinJointGapMm=" << thinJoint.minimumSeparationMetres * 1000.0f
                      << " thinJointTouch=" << thinJoint.touching
                      << " attackEligible=" << activeAttack
                      << " acceptedContact=" << accepted
                      << " falseAcceptBeyondLimit=" << falseAccept
                      << " missedNearContact=" << missedNear
                      << " movingTarget=" << targetMoved
                      << " rootConeOnlyPulse=" << rootConeOnlyPulse
                      << " touchingJointNames="
                      << (touchingJointNames.empty() ? "none" : touchingJointNames)
                      << " nearestAuthoredSegment=" << nearestRegion.segment
                      << " nearestTriangleSegmentDistanceMm=" << nearestRegion.centroidDistanceMetres * 1000.0f
                      << " nearestTargetTriangle=" << exact.targetTriangle
                      << " targetTriangles=" << targetPose.size() / 3u
                      << " bladeTriangles=" << blade.size()
                      << " targetPoseSource=actual_control_snapshot bladePoseSource=actual_anatomical_Grip\n";
            if (accepted)
            {
                for (std::size_t index = 0u; index < touchingJoint.size(); ++index)
                {
                    if (!touchingJoint[index]) continue;
                    const auto localStart = SkeletonWorldToLocal(
                        targetSample.transform, thinJointCapsules[index].start);
                    const auto localEnd = SkeletonWorldToLocal(
                        targetSample.transform, thinJointCapsules[index].end);
                    std::cout << "sword-thin-joint-contact-pose case=" << contact.name
                              << " tick=" << tick
                              << " clip=" << static_cast<int>(targetSample.clip)
                              << " clipTime=" << targetSample.clipTime
                              << " targetRootX=" << targetSnapshot.x
                              << " targetRootZ=" << targetSnapshot.z
                              << " targetFacing=" << targetSnapshot.facingRadians
                              << " targetTransform=";
                    for (std::size_t component = 0u; component < targetSample.transform.size(); ++component)
                        std::cout << (component == 0u ? "" : ",") << targetSample.transform[component];
                    std::cout << " joint=" << kImportedJointSegments[index].name
                              << " radiusMm=" << targetBoneRadiusMetres * 1000.0f
                              << " rootLocalStart=" << localStart[0] << ',' << localStart[1] << ',' << localStart[2]
                              << " rootLocalEnd=" << localEnd[0] << ',' << localEnd[1] << ',' << localEnd[2]
                              << "\n";
                }
            }
        }
    }
    Check(exactQueries == cases.size() * 12u,
          "thin-joint active-window witness must complete all 84 full-mesh samples");
    std::cout << "sword-thin-joint-active-window-summary exactQueries=" << exactQueries
              << " queryBudget=" << cases.size() * 12u
              << " thinJointRadiusMm=25 bladeRadiusMm=36.5078"
              << " thinJointTouchSamples=" << thinJointTouchSamples
              << " acceptedActiveContacts=" << thinJointAcceptedContacts
              << " falseAcceptsBeyondBladePlusBoneRadii=" << thinJointFalseAccepts
              << " missedMeshNearContacts=" << thinJointMissedNearContacts
              << " rootConeOnlyPulseSamples=" << rootConeOnlyPulses
              << " movingRootConeOnlyPulseSamples=" << movingRootConeOnlyPulses
              << " sampleWindows=down13to24_up27to38 continuousCollisionClaim=0\n";
}

} // namespace

int main(int argc, char** argv)
{
    if (argc == 1)
        TestSwordTargetContactRegionWitness();
    else if (argc == 2 && std::string(argv[1]) == "--moving-pulse")
        TestMovingPulseWitness();
    else if (argc == 2 && std::string(argv[1]) == "--moving-downstroke")
        TestMovingDownstrokeWitness();
    else if (argc == 2 && std::string(argv[1]) == "--contact-boundaries")
        RunContactBoundaryWitness();
    else if (argc == 2 && std::string(argv[1]) == "--contact-callback")
        RunContactCallbackContractWitness();
    else if (argc == 2 && std::string(argv[1]) == "--contact-proxy-invalid-inputs")
        RunInvalidContactProxyInputWitness();
    else if (argc == 2 && std::string(argv[1]) == "--thin-joint-active-window")
        RunThinJointActiveWindowWitness();
    else
    {
        std::cerr << "usage: horde_rt_sword_target_contact_region_witness [--moving-pulse|--moving-downstroke|--contact-boundaries|--contact-callback|--contact-proxy-invalid-inputs|--thin-joint-active-window]\n";
        return 2;
    }
    return failures == 0 ? 0 : 1;
}
