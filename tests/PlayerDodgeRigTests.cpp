#include "scene/assets/SkinnedMeshAsset.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace
{
using namespace horde::scene;
using namespace horde::vulkan::raytracing;
using namespace horde::gameplay::animation;
using Vertices = std::vector<TexturedSkinnedRtVertex>;
using Tangents = std::vector<SkinnedPbrTangent>;
constexpr float kPi = 3.14159265358979323846f;
constexpr std::array<std::array<float, 2>, 4> kDirections{{
    {{0, 1}}, {{0, -1}}, {{1, 0}}, {{-1, 0}}}};
constexpr std::array<std::string_view, 3> kLeftLeg{"LeftUpLeg", "LeftLeg", "LeftFoot"};
constexpr std::array<std::string_view, 3> kRightLeg{"RightUpLeg", "RightLeg", "RightFoot"};
int failures = 0, checks = 0;
std::size_t samples = 0;
std::string context;

void Check(bool value, const char* message)
{
    ++checks;
    if (!value) { ++failures; std::cerr << "FAIL: " << message << ' ' << context << '\n'; }
}
bool Near(float a, float b, float tolerance = .0001f)
{
    return std::isfinite(a) && std::isfinite(b) && std::abs(a-b) <= tolerance;
}
float Distance(const SkinnedNodeTransform& a, const SkinnedNodeTransform& b)
{
    return std::hypot(std::hypot(a[12]-b[12], a[13]-b[13]), a[14]-b[14]);
}
bool TransformNear(const SkinnedNodeTransform& a, const SkinnedNodeTransform& b)
{
    for (std::size_t i = 0; i < a.size(); ++i) if (!Near(a[i], b[i])) return false;
    return true;
}
bool ExactGeometry(const Vertices& a, const Tangents& at, const Vertices& b, const Tangents& bt)
{
    return !a.empty() && a.size() == b.size() && at.size() == bt.size() && at.size() == a.size() &&
        std::memcmp(a.data(), b.data(), a.size()*sizeof(a[0])) == 0 &&
        std::memcmp(at.data(), bt.data(), at.size()*sizeof(at[0])) == 0;
}
bool GeometryNear(const Vertices& a, const Tangents& at, const Vertices& b, const Tangents& bt)
{
    if (a.empty() || a.size() != b.size() || at.size() != bt.size() || at.size() != a.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        for (unsigned axis = 0; axis < 4; ++axis)
            if (!Near(a[i].position[axis], b[i].position[axis]) ||
                !Near(a[i].normal[axis], b[i].normal[axis]) ||
                a[i].texcoord[axis] != b[i].texcoord[axis] ||
                !Near(at[i].tangent[axis], bt[i].tangent[axis])) return false;
    return true;
}
bool FiniteOrthonormalGeometry(const Vertices& vertices, const Tangents& tangents)
{
    if (vertices.empty() || vertices.size() != tangents.size()) return false;
    for (std::size_t i = 0; i < vertices.size(); ++i)
    {
        const auto& v = vertices[i];
        const auto& t = tangents[i].tangent;
        for (unsigned axis = 0; axis < 4; ++axis)
            if (!std::isfinite(v.position[axis]) || !std::isfinite(v.normal[axis]) ||
                !std::isfinite(v.texcoord[axis]) || !std::isfinite(t[axis])) return false;
        const float normalLength = std::hypot(std::hypot(v.normal[0], v.normal[1]), v.normal[2]);
        const float tangentLength = std::hypot(std::hypot(t[0], t[1]), t[2]);
        const float dot = v.normal[0]*t[0] + v.normal[1]*t[1] + v.normal[2]*t[2];
        if (!Near(normalLength, 1, .001f) || !Near(tangentLength, 1, .001f) ||
            std::abs(dot) > .001f || (t[3] != -1 && t[3] != 1)) return false;
    }
    return true;
}
bool Skin(const SkinnedMeshAsset& asset, const SkinnedPlayerPose& pose,
          Vertices& vertices, Tangents& tangents, std::string& diagnostic)
{
    if (asset.SkinPlayerPoseUniqueTextured(pose, vertices, tangents, diagnostic)) return true;
    Check(false, diagnostic.c_str());
    return false;
}
float Weight(unsigned phase)
{
    const float p = static_cast<float>(phase)/12.f;
    return 16.f*p*p*(1.f-p)*(1.f-p);
}
bool Evaluate(const SkinnedMeshAsset& asset, SkinnedClip clip, float time,
              const SkinnedArmIkTarget& left, const SkinnedArmIkTarget& right,
              SkinnedPlayerPose& pose, std::string& diagnostic, SkinnedPlayerDodgePose dodge = {})
{
    if (asset.EvaluatePlayerPose(clip, time, left, right, pose, diagnostic, 0, 0, dodge)) return true;
    Check(false, diagnostic.c_str());
    return false;
}

bool TestAssetSweep(SkinnedMeshAsset& world, SkinnedMeshAsset& view, std::string& diagnostic)
{
    SkinnedArmIkTarget released;
    released.poseWeight = 0;
    for (auto clip : {SkinnedClip::Idle, SkinnedClip::Walking})
    for (unsigned clipPhase = 0; clipPhase < 4; ++clipPhase)
    {
        const float time = world.ClipDuration(clip)*clipPhase/4.f;
        context = "clip=" + std::to_string(int(clip)) + " clipPhase=" + std::to_string(clipPhase);
        SkinnedPlayerPose baseline;
        if (!Evaluate(world, clip, time, released, released, baseline, diagnostic)) return false;
        Vertices originalVertices;
        Tangents originalTangents;
        if (!Skin(world, baseline, originalVertices, originalTangents, diagnostic)) return false;
        std::array<std::array<SkinnedNodeTransform, 3>, 2> original{};
        if (!world.NodeTransforms(clip, time, kLeftLeg, original[0], diagnostic) ||
            !world.NodeTransforms(clip, time, kRightLeg, original[1], diagnostic))
        { Check(false, diagnostic.c_str()); return false; }
        float floor = 0;
        if (!world.BootGroundingMinimumY(clip, time, floor, diagnostic))
        { Check(false, diagnostic.c_str()); return false; }
        for (const auto& direction : kDirections)
        {
            auto previous = original;
            for (unsigned phase = 0; phase <= 12; ++phase)
            {
                context = "clip=" + std::to_string(int(clip)) + " clipPhase=" + std::to_string(clipPhase) +
                    " direction=" + std::to_string(direction[0]) + "," + std::to_string(direction[1]) +
                    " dodgePhase=" + std::to_string(phase);
                const SkinnedPlayerDodgePose dodge{Weight(phase), direction[0], direction[1]};
                Check(ValidPlayerDodgePose(dodge), "finite cardinal dodge is admitted");
                SkinnedPlayerPose solved;
                if (!Evaluate(world, clip, time, released, released, solved, diagnostic, dodge)) return false;
                Vertices vertices;
                Tangents tangents;
                if (!Skin(world, solved, vertices, tangents, diagnostic)) return false;
                Check(FiniteOrthonormalGeometry(vertices, tangents), "dodge skin has finite unit normal/tangent frames");
                const auto& sockets = solved.Sockets();
                const auto offset = PlayerDodgeTorsoOffset(dodge);
                Check(Near(offset[1], -.08f*dodge.weight) &&
                    Near(std::hypot(offset[0], offset[2]), .04f*dodge.weight),
                    "torso offset stays within the admitted eight-centimetre dip and four-centimetre lunge");
                for (unsigned axis = 0; axis < 3; ++axis)
                {
                    Check(Near(sockets.hips[12+axis], baseline.Sockets().hips[12+axis]+offset[axis]),
                        "solved hips use the published torso offset once");
                    Check(Near(sockets.leftHand[12+axis], baseline.Sockets().leftHand[12+axis]+offset[axis]) &&
                        Near(sockets.rightHand[12+axis], baseline.Sockets().rightHand[12+axis]+offset[axis]),
                        "released hands follow the same torso translation");
                }
                const auto minimum = std::min_element(vertices.begin(), vertices.end(),
                    [](const auto& a, const auto& b) { return a.position[1] < b.position[1]; });
                if (minimum != vertices.end() && minimum->position[1] < floor-.001f)
                    std::cerr << "floor=" << floor << " meshMinimum=" << minimum->position[1] << ' ' << context << '\n';
                Check(minimum != vertices.end() && minimum->position[1] >= floor-.001f,
                    "dodge mesh stays above the authored boot contact plane within one millimetre");
                if (dodge.weight == 0)
                {
                    Check(ExactGeometry(vertices, tangents, originalVertices, originalTangents),
                        "zero-weight onset/recovery preserves exact legacy vertex and tangent bytes");
                    if (phase == 12)
                        for (unsigned side = 0; side < 2; ++side)
                            Check(Distance(original[side][1], previous[side][1]) < .12f,
                                "final fixed step returns continuously to the authored knee");
                    previous = original;
                }
                else for (unsigned side = 0; side < 2; ++side)
                {
                    const auto& leg = side ? sockets.rightLeg : sockets.leftLeg;
                    Check(Near(Distance(leg[0], leg[1]), Distance(original[side][0], original[side][1])) &&
                        Near(Distance(leg[1], leg[2]), Distance(original[side][1], original[side][2])),
                        "directional leg solve preserves both rigid bone lengths");
                    bool orientationUnchanged = true;
                    for (unsigned i = 0; i < 12; ++i)
                        orientationUnchanged &= leg[2][i] == original[side][2][i];
                    Check(orientationUnchanged && leg[2][13] == original[side][2][13] &&
                        leg[2][15] == original[side][2][15], "ankle height and complete authored orientation stay exact");
                    const float dx = leg[2][12]-original[side][2][12];
                    const float dz = leg[2][14]-original[side][2][14];
                    const float along = dx*direction[0]+dz*direction[1];
                    Check(along >= -.0001f && along <= .0801f &&
                        std::abs(dx*direction[1]-dz*direction[0]) < .0001f,
                        "ankles move only along the admitted direction and at most eight centimetres");
                    Check(Distance(leg[1], previous[side][1]) < .12f,
                        "fixed-step knee motion remains continuous without a pole flip");
                    previous[side] = leg;
                }
                if (phase == 6)
                {
                    SkinnedPlayerPose independent;
                    if (!Evaluate(view, clip, time, released, released, independent, diagnostic, dodge)) return false;
                    Vertices primary, reference;
                    Tangents primaryTangents, referenceTangents;
                    if (!Skin(view, solved, primary, primaryTangents, diagnostic) ||
                        !Skin(view, independent, reference, referenceTangents, diagnostic)) return false;
                    Check(GeometryNear(primary, primaryTangents, reference, referenceTangents),
                        "world and primary meshes consume the same solved dodge palette");
                }
                ++samples;
            }
        }
        // A finite unused direction must not change the old fast path.
        SkinnedPlayerPose zero;
        if (!Evaluate(world, clip, time, released, released, zero, diagnostic, {0, 123, -456})) return false;
        Vertices zeroVertices;
        Tangents zeroTangents;
        if (!Skin(world, zero, zeroVertices, zeroTangents, diagnostic)) return false;
        Check(ExactGeometry(zeroVertices, zeroTangents, originalVertices, originalTangents),
            "zero weight ignores finite unused direction without changing geometry bytes");
        Vertices originalPrimary, zeroPrimary;
        Tangents originalPrimaryTangents, zeroPrimaryTangents;
        if (!Skin(view, baseline, originalPrimary, originalPrimaryTangents, diagnostic) ||
            !Skin(view, zero, zeroPrimary, zeroPrimaryTangents, diagnostic)) return false;
        Check(ExactGeometry(originalPrimary, originalPrimaryTangents, zeroPrimary, zeroPrimaryTangents),
            "shared zero layer also preserves exact primary vertex and tangent bytes");
    }
    return true;
}

bool TestTurningAndGrips(SkinnedMeshAsset& world, std::string& diagnostic)
{
    std::array<SkinnedArmIkTarget, 2> arms{};
    for (unsigned side = 0; side < 2; ++side)
    {
        const float sign = side ? -1.f : 1.f;
        arms[side].target = {{sign*.16f, 1.1f, .35f}};
        arms[side].pole = {{sign, 0, 1}};
        arms[side].handOrientation = {{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}};
        arms[side].handOrientationTargetEnabled = true;
    }
    for (auto clip : {SkinnedClip::Idle, SkinnedClip::Walking})
    {
        const float time = world.ClipDuration(clip)*.21f;
        SkinnedPlayerPose baseline;
        if (!Evaluate(world, clip, time, arms[0], arms[1], baseline, diagnostic)) return false;
        std::array<SkinnedNodeTransform, 2> previousKnees{};
        for (unsigned turn = 0; turn <= 72; ++turn)
        {
            context = "turn=" + std::to_string(turn) + " clip=" + std::to_string(int(clip));
            const float angle = 2*kPi*static_cast<float>(turn)/72.f;
            const SkinnedPlayerDodgePose dodge{1, std::sin(angle), std::cos(angle)};
            SkinnedPlayerPose solved;
            if (!Evaluate(world, clip, time, arms[0], arms[1], solved, diagnostic, dodge)) return false;
            const auto& sockets = solved.Sockets();
            for (unsigned side = 0; side < 2; ++side)
            {
                const auto& hand = side ? sockets.rightHand : sockets.leftHand;
                const auto& grip = side ? sockets.rightGrip : sockets.leftGrip;
                const auto& referenceGrip = side ? baseline.Sockets().rightGrip : baseline.Sockets().leftGrip;
                for (unsigned axis = 0; axis < 3; ++axis)
                    Check(Near(hand[12+axis], arms[side].target[axis]),
                        "arm solve retains the exact unchanged hand target after the leg solve");
                Check(TransformNear(grip, referenceGrip),
                    "held grip position and orientation do not inherit the dodge torso offset");
                const auto& knee = (side ? sockets.rightLeg : sockets.leftLeg)[1];
                if (turn) Check(Distance(knee, previousKnees[side]) < .08f,
                    "rotating direction crosses every leading-foot boundary continuously");
                previousKnees[side] = knee;
            }
            Vertices vertices;
            Tangents tangents;
            if (!Skin(world, solved, vertices, tangents, diagnostic)) return false;
            Check(FiniteOrthonormalGeometry(vertices, tangents), "turning with active hand IK retains finite normalized skin");
            ++samples;
        }
    }
    return true;
}

PlayerAnimationSnapshot Animation(SkinnedPlayerDodgePose dodge)
{
    PlayerAnimationSnapshot animation;
    animation.locomotionTime = .21f;
    animation.dodge = {dodge.weight, dodge.modelX, dodge.modelZ};
    for (unsigned side = 0; side < 2; ++side)
    {
        auto& arm = side ? animation.rightIk : animation.leftIk;
        const float sign = side ? -1.f : 1.f;
        arm.target = {{sign*.16f, 1.1f, .35f}};
        arm.pole = {{sign, 0, 1}};
        arm.gripX = {{1, 0, 0}}; arm.gripY = {{0, 1, 0}}; arm.gripZ = {{0, 0, 1}};
    }
    return animation;
}

bool TestRenderSlot(PlayerRenderSlot& slot, std::string& diagnostic)
{
    constexpr std::uint64_t tick = 123;
    const auto basis = BuildPlayerModelWorldBasis({{1, 0, 0}}, {{0, 0, -1}});
    const std::array<float, 3> root{{.3f, 0, -.2f}};
    for (auto clip : {PlayerLocomotionClip::Idle, PlayerLocomotionClip::Walk})
    for (const auto& direction : kDirections)
    for (float weight : {.2f, .6f, 1.f, .6f, 0.f})
    {
        context = "render clip=" + std::to_string(int(clip)) + " weight=" + std::to_string(weight);
        const SkinnedPlayerDodgePose dodge{weight, direction[0], direction[1]};
        auto animation = Animation(dodge);
        animation.locomotionClip = clip;
        auto neutral = animation; neutral.dodge = {};
        std::array<float, 3> shoulder{}, neutralShoulder{};
        if (!slot.ShoulderCenter(animation, shoulder, diagnostic) ||
            !slot.ShoulderCenter(neutral, neutralShoulder, diagnostic))
        { Check(false, diagnostic.c_str()); return false; }
        const auto offset = PlayerDodgeTorsoOffset(dodge);
        for (unsigned axis = 0; axis < 3; ++axis)
            Check(Near(shoulder[axis], neutralShoulder[axis]+offset[axis]),
                "pre-solve shoulders use exactly the same model-space torso offset");
        Check(slot.BootGroundingOffsetMetres(animation) == slot.BootGroundingOffsetMetres(neutral),
            "dodge does not move the grounded root to conceal leg geometry");
        auto mount = horde::gameplay::items::IdentityHeldItemTransform();
        if (!slot.AnimatedHipsWorldTransform(animation, basis, root, mount, diagnostic))
        { Check(false, diagnostic.c_str()); return false; }
        bool updated = false;
        if (!slot.PreparePose(animation, tick, PlayerCpuSkinCadence::Hz30, updated, diagnostic))
        { Check(false, diagnostic.c_str()); return false; }
        Check(updated, "same-tick changed dodge invalidates the cached palette");
        const auto& hips = slot.BoneSockets().hips;
        const auto worldHips = PlayerModelVectorToWorld(basis, {{hips[12], hips[13], hips[14]}});
        for (unsigned axis = 0; axis < 3; ++axis)
            Check(Near(mount[12+axis], root[axis]+worldHips[axis]),
                "pre-solve equipment mount agrees with solved hips through the proper world rotation");
        Check(slot.LeftSocketErrorMetres() <= kPlayerGripSocketToleranceMetres &&
            slot.RightSocketErrorMetres() <= kPlayerGripSocketToleranceMetres,
            "renderer dodge preserves the existing fifteen-millimetre grip admission");
        const auto vertices = slot.UniqueVertices();
        const auto tangents = slot.UniqueTangents();
        updated = true;
        Check(slot.PreparePose(animation, tick, PlayerCpuSkinCadence::Hz30, updated, diagnostic) && !updated &&
            ExactGeometry(vertices, tangents, slot.UniqueVertices(), slot.UniqueTangents()),
            "identical same-tick request reuses the exact palette and skin");
    }
    return true;
}

bool TestRejectedRequests(SkinnedMeshAsset& world, PlayerRenderSlot& slot, std::string& diagnostic)
{
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const std::array<SkinnedPlayerDodgePose, 13> invalid{{
        {-.001f, 0, 1}, {1.001f, 0, 1}, {nan, 0, 1}, {inf, 0, 1},
        {1, nan, 1}, {1, 0, inf}, {1, 0, 0}, {1, 0, .5f}, {1, 2, 0},
        {1, 1, 1}, {0, nan, 0}, {0, 0, inf}, {.5f, inf, 0}}};
    struct Request { SkinnedPlayerDodgePose dodge; float planted = 0, crouch = 0; };
    std::vector<Request> rejected;
    for (const auto& dodge : invalid)
    {
        Check(!ValidPlayerDodgePose(dodge), "malformed direction or weight is rejected by the public validator");
        rejected.push_back({dodge});
    }
    rejected.push_back({{1, 0, 1}, .1f, 0});
    rejected.push_back({{1, 0, 1}, 0, .1f});
    SkinnedArmIkTarget released; released.poseWeight = 0;
    SkinnedPlayerPose pose;
    for (std::size_t i = 0; i < rejected.size(); ++i)
    {
        context = "rejectedRequest=" + std::to_string(i);
        const auto& request = rejected[i];
        if (!Evaluate(world, SkinnedClip::Idle, 0, released, released, pose, diagnostic, {1, 0, 1})) return false;
        Vertices vertices;
        Tangents tangents;
        if (!Skin(world, pose, vertices, tangents, diagnostic)) return false;
        Check(!world.EvaluatePlayerPose(SkinnedClip::Idle, 0, released, released, pose, diagnostic,
            request.planted, request.crouch, request.dodge) && !pose.IsValid() && !diagnostic.empty(),
            "invalid or combined layer invalidates a previously valid native palette");
        Check(!world.SkinPlayerPoseUniqueTextured(pose, vertices, tangents, diagnostic) &&
            vertices.empty() && tangents.empty(), "failed pose cannot skin stale vertices or tangents");
        bool updated = false;
        if (!slot.PreparePose(Animation({1, 0, 1}), 123, PlayerCpuSkinCadence::Hz30, updated, diagnostic))
        { Check(false, diagnostic.c_str()); return false; }
        auto animation = Animation(request.dodge);
        animation.plantedPelvisDropMetres = request.planted;
        animation.crouchPelvisDropMetres = request.crouch;
        updated = true;
        Check(!slot.PreparePose(animation, 123, PlayerCpuSkinCadence::Hz30, updated, diagnostic) &&
            !updated && !slot.SolvedPose().IsValid() && !diagnostic.empty(),
            "renderer validates invalid dodge before same-tick cadence reuse");
        Check(slot.BoneSockets().hips == SkinnedNodeTransform{} &&
            slot.BoneSockets().leftGrip == SkinnedNodeTransform{} &&
            slot.BoneSockets().rightGrip == SkinnedNodeTransform{}, "failed renderer request clears stale sockets");
    }
    for (auto unsupported : {SkinnedClip::Attack, SkinnedClip::Dead})
    {
        Check(!world.EvaluatePlayerPose(unsupported, 0, released, released, pose, diagnostic, 0, 0, {1, 0, 1}) &&
            !pose.IsValid(), "dodge layer remains restricted to existing Idle and Walking clips");
    }
    bool updated = false;
    Check(slot.PreparePose(Animation({}), 123, PlayerCpuSkinCadence::Hz30, updated, diagnostic) &&
        updated && slot.SolvedPose().IsValid(), "valid neutral request recovers after rejected input");
    return true;
}
}

int main(int argc, char** argv)
{
    if (argc != 2) { std::cerr << "Expected explicit source asset root.\n"; return 2; }
    const std::filesystem::path root(argv[1]);
    const auto path = root/"assets/models/player/runtime/gothic-traveller-lod0.runtime.glb";
    SkinnedMeshAsset world, view;
    PlayerRenderSlot slot;
    std::string diagnostic;
    if (!world.LoadClips(path.string(), PlayerLocomotionClipSet(), diagnostic) ||
        !view.LoadClips((root/"assets/models/player/viewmodel/runtime/gothic-traveller-viewmodel.runtime.glb").string(),
            PlayerLocomotionClipSet(), diagnostic) || !slot.LoadAsset(path.string(), diagnostic))
    { std::cerr << diagnostic << '\n'; return 1; }
    const bool completed = TestAssetSweep(world, view, diagnostic) &&
        TestTurningAndGrips(world, diagnostic) && TestRenderSlot(slot, diagnostic) &&
        TestRejectedRequests(world, slot, diagnostic);
    std::cout << "dodge_rig_samples=" << samples << " checks=" << checks << " failures=" << failures
        << "; native CPU runtime-asset rig only, GPU/device acceptance not measured\n";
    return completed && failures == 0 ? 0 : 1;
}
