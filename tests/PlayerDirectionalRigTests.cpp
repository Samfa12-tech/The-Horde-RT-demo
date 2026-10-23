#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/items/HeldLightState.h"
#include "gameplay/items/HeldItemKinematics.h"
#include "scene/DevelopmentWorldGeometry.h"
#include "scene/assets/AssetManifest.h"
#include "scene/assets/StaticMeshAsset.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace
{
using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
using namespace horde::gameplay::animation;
using namespace horde::gameplay::items;
using namespace horde::vulkan::raytracing;
using Vec3 = std::array<float, 3u>;

bool Require(const bool condition, const char* message)
{
    if (condition) return true;
    std::cerr << "FAIL: " << message << '\n';
    return false;
}

bool Near(const float left, const float right, const float tolerance = 0.0001f)
{
    return std::abs(left - right) <= tolerance;
}

Vec3 Add(const Vec3& a, const Vec3& b)
{
    return {{a[0] + b[0], a[1] + b[1], a[2] + b[2]}};
}
Vec3 Scale(const Vec3& value, const float scale)
{
    return {{value[0] * scale, value[1] * scale, value[2] * scale}};
}
float Dot(const Vec3& a, const Vec3& b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
bool TransformsNear(const HeldItemTransform& left, const HeldItemTransform& right,
                    const float tolerance = 0.0001f)
{
    for (std::size_t i = 0; i < left.size(); ++i)
        if (!Near(left[i], right[i], tolerance)) return false;
    return true;
}
Vec3 Cross(const Vec3& a, const Vec3& b)
{
    return {{a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
             a[0] * b[1] - a[1] * b[0]}};
}
Vec3 Unit(const Vec3& value)
{
    return Scale(value, 1.0f / std::sqrt(std::max(Dot(value, value), 1.0e-8f)));
}
HeldItemTransform WorldSocket(const std::array<float, 16u>& bone,
                              const PlayerModelWorldBasis& basis,
                              const Vec3& root)
{
    const auto convert = [&basis](const Vec3& vector) {
        return PlayerModelVectorToWorld(basis, vector);
    };
    const Vec3 x = Unit(convert({{bone[0], bone[1], bone[2]}}));
    const Vec3 rawY = convert({{bone[4], bone[5], bone[6]}});
    Vec3 y = Unit(Add(rawY, Scale(x, -Dot(rawY, x))));
    Vec3 z = Unit(Cross(x, y));
    if (Dot(z, convert({{bone[8], bone[9], bone[10]}})) < 0.0f)
    {
        y = Scale(y, -1.0f);
        z = Scale(z, -1.0f);
    }
    const Vec3 position = Add(root, convert({{bone[12], bone[13], bone[14]}}));
    HeldItemTransform result = IdentityHeldItemTransform();
    for (std::size_t axis = 0; axis < 3u; ++axis)
    {
        result[axis] = x[axis]; result[4u + axis] = y[axis]; result[8u + axis] = z[axis];
        result[12u + axis] = position[axis];
    }
    return result;
}

bool PrepareRenderedPose(PlayerRenderSlot& rig,
                         const SimulationSnapshot& snapshot,
                         const std::uint64_t tick,
                         bool& poseUpdated,
                         std::string& diagnostic,
                         HeldItemStates* resolvedItems = nullptr,
                         HeldLightState* resolvedLight = nullptr,
                         Vec3* resolvedRoot = nullptr,
                         PlayerModelWorldBasis* resolvedBasis = nullptr)
{
    const auto& animation = snapshot.playerAnimation;
    const Vec3 worldUp{{0.0f, 1.0f, 0.0f}};
    const Vec3 eye{{snapshot.playerX,
        horde::gameplay::simulation::PlayerEyeWorldY(snapshot.playerSupportWorldY),
        snapshot.playerZ}};
    const auto lower = horde::gameplay::EvaluateLowerBodyPose(snapshot.walkTime, snapshot.walkAmount);
    const Vec3 bodyForward{{std::sin(snapshot.playerYawRadians), 0.0f, -std::cos(snapshot.playerYawRadians)}};
    const Vec3 bodyRight{{std::cos(snapshot.playerYawRadians), 0.0f, std::sin(snapshot.playerYawRadians)}};
    const float cosine = std::cos(lower.torsoTwistRadians), sine = std::sin(lower.torsoTwistRadians);
    const PlayerModelWorldBasis basis = BuildPlayerModelWorldBasis(
        Unit(Add(Scale(bodyRight, cosine), Scale(bodyForward, -sine))),
        Unit(Add(Scale(bodyForward, cosine), Scale(bodyRight, sine))));
    const Vec3 bodyOrigin = Add(Add(eye, Scale(bodyRight, lower.pelvisSway)),
                                Vec3{{0.0f, lower.pelvisBob * 0.65f, 0.0f}});
    const float pitch = std::clamp(snapshot.playerPitchRadians, -0.32f, 0.28f);
    const Vec3 viewForward = Unit({{std::sin(snapshot.playerYawRadians), -0.05f + pitch,
                                    -std::cos(snapshot.playerYawRadians)}});
    const Vec3 viewRight = Unit(Cross(viewForward, worldUp));
    const Vec3 viewUp = Unit(Cross(viewRight, viewForward));
    const auto viewToWorld = [&](const Vec3& value) {
        return Add(Add(Scale(viewRight, value[0]), Scale(viewUp, value[1])), Scale(viewForward, value[2]));
    };
    const Vec3 torsoCenterWorld = Add(eye, viewToWorld(EvaluatePlayerTorsoAnchorLocal(animation)));
    Vec3 rigShoulderCenter{};
    if (!rig.ShoulderCenter(animation, rigShoulderCenter, diagnostic)) return false;
    Vec3 root = GroundPlayerRootOnRouteFloor(
        Add(torsoCenterWorld, Scale(PlayerModelVectorToWorld(basis, rigShoulderCenter), -1.0f)),
        snapshot.playerSupportWorldY, rig.BootGroundingOffsetMetres(animation));
    if (snapshot.playerMountProfile == PlayerMountProfile::AnatomicalBody)
        root = GroundPlayerRootOnRouteFloor(bodyOrigin, snapshot.playerSupportWorldY,
                                            rig.BootGroundingOffsetMetres(animation));

    const auto worldPointToModel = [&](const Vec3& point) {
        return WorldVectorToPlayerModel(basis,
            Add(Add(eye, viewToWorld(point)), Scale(root, -1.0f)));
    };
    const auto viewVectorToModel = [&](const Vec3& vector) {
        return WorldVectorToPlayerModel(basis, viewToWorld(vector));
    };
    PlayerAnimationSnapshot rigAnimation = animation;
    for (auto* arm : {&rigAnimation.leftIk, &rigAnimation.rightIk})
    {
        arm->shoulder = worldPointToModel(arm->shoulder);
        arm->target = worldPointToModel(arm->target);
        arm->pole = viewVectorToModel(arm->pole);
        arm->gripX = viewVectorToModel(arm->gripX);
        arm->gripY = viewVectorToModel(arm->gripY);
        arm->gripZ = viewVectorToModel(arm->gripZ);
    }
    HeldItemTransform worldFromHips{};
    if (!rig.AnimatedHipsWorldTransform(animation, basis, root, worldFromHips, diagnostic)) return false;
    const HeldItemTransform bodyStow = MultiplyHeldItemTransforms(
        worldFromHips, SwordBodyStowFromHips());
    if (animation.swordStowBlend > 0.0f)
    {
        const auto expectedWorldFromItem = BlendHeldItemTransformsAtGrip(
            bodyStow, snapshot.heldItems[1].worldFromItem, SwordGripSocketTransform(),
            1.0f - animation.swordStowBlend);
        const auto expectedWorldFromGrip = MultiplyHeldItemTransforms(
            expectedWorldFromItem, SwordGripSocketTransform());
        auto itemArm = rigAnimation.rightIk;
        itemArm.target = worldPointToModel(
            {{expectedWorldFromGrip[12], expectedWorldFromGrip[13], expectedWorldFromGrip[14]}});
        itemArm.gripX = WorldVectorToPlayerModel(basis,
            {{expectedWorldFromGrip[0], expectedWorldFromGrip[1], expectedWorldFromGrip[2]}});
        itemArm.gripY = WorldVectorToPlayerModel(basis,
            {{expectedWorldFromGrip[4], expectedWorldFromGrip[5], expectedWorldFromGrip[6]}});
        itemArm.gripZ = WorldVectorToPlayerModel(basis,
            {{expectedWorldFromGrip[8], expectedWorldFromGrip[9], expectedWorldFromGrip[10]}});
        HeldItemTransform itemGrip = IdentityHeldItemTransform();
        for (std::size_t axis = 0u; axis < 3u; ++axis)
        {
            itemGrip[axis] = itemArm.gripX[axis];
            itemGrip[4u + axis] = itemArm.gripY[axis];
            itemGrip[8u + axis] = itemArm.gripZ[axis];
            itemGrip[12u + axis] = itemArm.target[axis];
        }
        rigAnimation.rightIk = BlendPlayerArmGripTarget(
            rigAnimation.rightIk, itemGrip, animation.swordHandGripBlend);
    }
    if (!rig.PreparePose(rigAnimation, tick, PlayerCpuSkinCadence::Hz60,
                         poseUpdated, diagnostic)) return false;

    const auto leftGrip = WorldSocket(rig.BoneSockets().leftGrip, basis, root);
    const auto rightGrip = WorldSocket(rig.BoneSockets().rightGrip, basis, root);
    HeldItemStates renderItems{};
    if (!rig.ResolveHeldItemVisuals(snapshot.heldItems, leftGrip, rightGrip, bodyStow,
                                    renderItems, diagnostic)) return false;
    HeldLightState light{};
    if (!ComposeHeldLightState(renderItems[0].worldFromItem,
            PlayerRagTorchFlameSocketTransform(), PlayerRagTorchLightSocketTransform(),
            1.0f, light, diagnostic) || !light.active) return false;
    if (resolvedItems != nullptr) *resolvedItems = renderItems;
    if (resolvedLight != nullptr) *resolvedLight = light;
    if (resolvedRoot != nullptr) *resolvedRoot = root;
    if (resolvedBasis != nullptr) *resolvedBasis = basis;
    return true;
}

struct ActualFootPose
{
    float sampledTime=0,legacyTime=0,phaseOffset=0;
    std::array<Vec3,4> footAndToeWorld{};
    std::array<float,2> soleY{};
    std::vector<Vec3> bootWorld;
    Vec3 root{};
    HeldItemStates items{};
    HeldLightState light{};
};

bool CheckBackwardFootSampling(const std::filesystem::path& root,PlayerRenderSlot& rig,
                               std::uint64_t& rigTick)
{
    horde::scene::SkinnedMeshAsset feet;
    horde::scene::assets::StaticMeshAsset layout;
    horde::scene::assets::AssetManifest manifest;
    std::string diagnostic;
    const auto directory=root/"assets/models/player/runtime";
    if (!feet.LoadClips((directory/"gothic-traveller-lod0.runtime.glb").string(),
                       horde::scene::PlayerLocomotionClipSet(),diagnostic) ||
        !horde::scene::assets::AssetManifest::Load(directory/"asset.manifest.json",manifest,diagnostic) ||
        !horde::scene::assets::StaticMeshAsset::Load(directory/"gothic-traveller-lod0.runtime.glb",manifest,layout,diagnostic) ||
        !feet.ValidateStaticVertexLayout(layout,diagnostic))
        return Require(false,diagnostic.c_str());
    const float duration=feet.ClipDuration(horde::scene::SkinnedClip::Walking);
    if (!Require(std::isfinite(duration)&&duration>0,"actual Walking duration must be available")) return false;
    std::array<std::vector<std::uint32_t>,2> boots;
    for (std::uint32_t index=0;index<layout.vertices.size();++index)
    {
        const auto& p=layout.vertices[index].position;
        // Same admitted lower-body domain as runtime exact boot grounding;
        // +X is anatomical Left. This observes actual skinned sole vertices.
        if (p[1]<=.45f&&p[0]!=0) boots[p[0]>0?0:1].push_back(index);
    }
    if (!Require(!boots[0].empty()&&!boots[1].empty(),"actual left/right boot domains must be nonempty")) return false;
    auto config=ProductionGameSimulationConfig();
    config.playerStartX=-18.5f;config.playerStartZ=-15.2f;
    config.waterfallSkeletonEncounter=false;
    auto simulation=std::make_unique<GameSimulation>(config);
    InputSnapshot input{};input.damageEnabled=false;input.moveForward=1;input.yawRadians=1.57079632679f;
    for (unsigned tick=0;tick<18;++tick) simulation->StepFixed(input);
    const auto source=simulation->Snapshot();
    constexpr std::array<std::string_view,6> names{{"LeftFoot","LeftToeBase","RightFoot","RightToeBase",
                                                  "LeftArm","RightArm"}};
    const auto capture=[&](const SimulationSnapshot& snapshot,ActualFootPose& out) {
        bool updated=false;PlayerModelWorldBasis basis;
        if (!PrepareRenderedPose(rig,snapshot,rigTick++,updated,diagnostic,&out.items,&out.light,&out.root,&basis) ||
            !updated||!rig.SolvedPose().IsValid()||rig.UniqueVertices().size()!=layout.vertices.size())
            return Require(false,diagnostic.c_str());
        if (!Require(rig.LeftGripAgreement().positionErrorMetres<=kPlayerGripSocketToleranceMetres&&
                     rig.RightGripAgreement().positionErrorMetres<=kPlayerGripSocketToleranceMetres&&
                     rig.LeftGripAgreement().orientationErrorRadians<=kPlayerGripOrientationToleranceRadians&&
                     rig.RightGripAgreement().orientationErrorRadians<=kPlayerGripOrientationToleranceRadians,
                     "backward sample retains actual imported Grip position/orientation agreement")) return false;
        std::array<horde::scene::SkinnedNodeTransform,6> nodes{};
        const float sampled=PlayerLocomotionSampleTime(snapshot.playerAnimation,duration);
        out.sampledTime=sampled;
        out.legacyTime=snapshot.playerAnimation.locomotionTime;
        out.phaseOffset=snapshot.playerAnimation.locomotionPhaseOffsetSeconds;
        if (!feet.NodeTransforms(horde::scene::SkinnedClip::Walking,sampled,names,nodes,diagnostic))
            return Require(false,diagnostic.c_str());
        Vec3 shoulders{};
        if (!rig.ShoulderCenter(snapshot.playerAnimation,shoulders,diagnostic))
            return Require(false,diagnostic.c_str());
        for(unsigned axis=0;axis<3;++axis)
            if(!Require(Near(shoulders[axis],(nodes[4][12+axis]+nodes[5][12+axis])*.5f,.00001f),
                        "shoulder query must sample the same phase as actual feet, pose and grounding"))return false;
        for (unsigned node=0;node<out.footAndToeWorld.size();++node)
            out.footAndToeWorld[node]=Add(out.root,PlayerModelVectorToWorld(basis,
                {{nodes[node][12],nodes[node][13],nodes[node][14]}}));
        out.bootWorld.clear();
        for (unsigned side=0;side<boots.size();++side)
        {
            out.soleY[side]=std::numeric_limits<float>::infinity();
            for (const auto index:boots[side])
            {
                const auto& p=rig.UniqueVertices()[index].position;
                const auto world=Add(out.root,PlayerModelVectorToWorld(basis,{{p[0],p[1],p[2]}}));
                if (!Require(std::all_of(world.begin(),world.end(),[](float v){return std::isfinite(v);}),
                             "actual backward boot vertices remain finite")) return false;
                out.bootWorld.push_back(world);out.soleY[side]=std::min(out.soleY[side],world[1]);
            }
        }
        const float lowest=std::min(out.soleY[0],out.soleY[1]);
        return Require(lowest>=snapshot.playerSupportWorldY-.00001f&&
                       lowest<=snapshot.playerSupportWorldY+.025f,
                       "resolved backward phase retains the established nonpenetrating 25mm sole-grounding bound");
    };
    const auto poseDelta=[](const ActualFootPose& a,const ActualFootPose& b,unsigned index,const char* pair) {
        if(a.bootWorld.size()!=b.bootWorld.size()) {
            std::cerr<<"Backward phase pair index="<<index<<" pair="<<pair<<" boot sizes="
                     <<a.bootWorld.size()<<'/'<<b.bootWorld.size()<<'\n';
            return std::numeric_limits<float>::infinity();
        }
        float error=0;
        std::array<float,7> componentErrors{};
        const char* maximumComponent="none";std::size_t maximumIndex=0;unsigned maximumAxis=0;
        const auto includeScalar=[&](float delta,unsigned component,const char* name,std::size_t element,unsigned axis) {
            componentErrors[component]=std::max(componentErrors[component],delta);
            if(delta>error){error=delta;maximumComponent=name;maximumIndex=element;maximumAxis=axis;}
        };
        const auto include=[&](const Vec3& x,const Vec3& y,unsigned component,const char* name,std::size_t element) {
            for(unsigned axis=0;axis<3;++axis)includeScalar(std::abs(x[axis]-y[axis]),component,name,element,axis);
        };
        include(a.root,b.root,0,"root",0);
        for(unsigned i=0;i<a.footAndToeWorld.size();++i)include(a.footAndToeWorld[i],b.footAndToeWorld[i],1,"foot/toe",i);
        for(std::size_t i=0;i<a.bootWorld.size();++i)include(a.bootWorld[i],b.bootWorld[i],2,"boot vertex",i);
        for(unsigned item=0;item<a.items.size();++item)for(unsigned c=0;c<16;++c)
            includeScalar(std::abs(a.items[item].worldFromItem[c]-b.items[item].worldFromItem[c]),
                          c>=12?3:4,c>=12?"item position":"item basis",item,c);
        for(unsigned c=0;c<16;++c)
            includeScalar(std::abs(a.light.worldFromLight[c]-b.light.worldFromLight[c]),
                          c>=12?5:6,c>=12?"light position":"light basis",0,c);
        if(error>.00001f) {
            const auto precision=std::cerr.precision(9);
            std::cerr<<"Backward phase pair index="<<index<<" pair="<<pair
                <<" sampleTimes="<<a.sampledTime<<'/'<<b.sampledTime
                <<" legacyTimes="<<a.legacyTime<<'/'<<b.legacyTime
                <<" phaseOffsets="<<a.phaseOffset<<'/'<<b.phaseOffset
                <<" maximum="<<error<<" component="<<maximumComponent
                <<" element="<<maximumIndex<<" axis/component="<<maximumAxis
                <<" errors root/foot/boot/itemPosition/itemBasis/lightPosition/lightBasis=";
            for(const auto value:componentErrors)std::cerr<<value<<'/';
            std::cerr<<'\n';std::cerr.precision(precision);
        }
        return error;
    };
    const Vec3 facing{{std::sin(source.playerYawRadians),0,-std::cos(source.playerYawRadians)}};
    const float h=duration/256.f;
    double forwardStanceVelocity=0,backwardStanceVelocity=0;
    double forwardToeVelocity=0,backwardToeVelocity=0;
    float maximumPairError=0,maximumReferenceSlip=0;
    double maximumAnalyticalPhaseError=0;
    // At most eight float roundings construct these clock/offset operands,
    // all bounded by 4*duration. Eight half-ULPs give four ULPs at that bound.
    // This clock budget never changes the independent 10um geometry gate.
    const float operandBound=4*duration;
    const double clockRoundoffBudget=4.0*(std::nextafter(operandBound,
        std::numeric_limits<float>::infinity())-operandBound);
    const auto analyticalPhaseError=[&](double expected,float published) {
        if(expected<0)expected+=duration;
        if(expected>=duration)expected-=duration;
        const double difference=std::abs(double(published)-expected);
        return std::min(difference,std::abs(double(duration)-difference));
    };
    unsigned samples=0,stancePairs=0,wrapPairs=0;
    // Independent phase-paired asset test, not a natural traversal claim.
    // Freeze gameplay/weapon clocks and pose targets while sampling the actual
    // clip in both directions. Include the negative-to-positive loop boundary.
    for(unsigned index=0;index<=16;++index)
    {
        const float phase=index==0?0.f:(index-.5f)*duration/16.f;
        auto before=source,after=source,reverseBefore=source,reverseAfter=source;
        for(auto* snapshot:{&before,&after,&reverseBefore,&reverseAfter})
        {
            snapshot->playerAnimation.locomotionClip=PlayerLocomotionClip::Walk;
            snapshot->playerAnimation.locomotionBlend=1;
        }
        before.playerAnimation.locomotionPhaseOffsetSeconds=0;
        after.playerAnimation.locomotionPhaseOffsetSeconds=0;
        reverseBefore.playerAnimation.locomotionTime=2*duration+phase+h;
        reverseBefore.playerAnimation.locomotionPhaseOffsetSeconds=-2*duration;
        reverseAfter.playerAnimation.locomotionTime=reverseBefore.playerAnimation.locomotionTime+2*h;
        reverseAfter.playerAnimation.locomotionPhaseOffsetSeconds=UpdateBackwardWalkPhaseOffset(
            reverseBefore.playerAnimation.locomotionPhaseOffsetSeconds,2*h,
            2*h*config.movementSpeedMetresPerSecond,-2*h*config.movementSpeedMetresPerSecond,
            config.movementSpeedMetresPerSecond);
        const float publishedBefore=PlayerLocomotionSampleTime(reverseBefore.playerAnimation,duration);
        const float publishedAfter=PlayerLocomotionSampleTime(reverseAfter.playerAnimation,duration);
        const double clockError=std::max(analyticalPhaseError(double(phase)+h,publishedBefore),
                                        analyticalPhaseError(double(phase)-h,publishedAfter));
        maximumAnalyticalPhaseError=std::max(maximumAnalyticalPhaseError,clockError);
        if(!Require(publishedBefore>=0&&publishedBefore<duration&&publishedAfter>=0&&publishedAfter<duration&&
                    clockError<=clockRoundoffBudget,
                    "published reverse phases must match analytical wrapped phase within bounded float roundoff"))return false;
        // Compare identical published float phases, rather than alleging that
        // independently rounded clock cancellation produced identical times.
        // Analytical direction/wrap correctness is checked separately above.
        before.playerAnimation.locomotionTime=publishedAfter;
        after.playerAnimation.locomotionTime=publishedBefore;
        ActualFootPose a,b,ra,rb;
        if(!capture(before,a)||!capture(after,b)||!capture(reverseBefore,ra)||!capture(reverseAfter,rb))return false;
        const float firstError=poseDelta(a,rb,index,"forward-before/reverse-after");
        const float secondError=poseDelta(b,ra,index,"forward-after/reverse-before");
        const float error=std::max(firstError,secondError);
        maximumPairError=std::max(maximumPairError,error);++samples;
        if(index==0)++wrapPairs;
        if(!Require(error<=.00001f,
                    "reversed phase must revisit actual foot/toe/boot/Grip/grounding poses in the opposite order"))return false;
        for(unsigned side=0;side<2;++side)
        {
            // Only intervals whose same boot supplies the lower sole at both
            // ends are labelled stance samples; crossing support is excluded.
            if(a.soleY[side]>a.soleY[1-side]||b.soleY[side]>b.soleY[1-side])continue;
            const auto velocity=[&](const ActualFootPose& x,const ActualFootPose& y,unsigned node) {
                return Dot(Add(y.footAndToeWorld[node],Scale(x.footAndToeWorld[node],-1)),facing)/(2*h);
            };
            const float forward=velocity(a,b,side*2),backward=velocity(ra,rb,side*2);
            const float toeForward=velocity(a,b,side*2+1),toeBackward=velocity(ra,rb,side*2+1);
            forwardStanceVelocity+=forward;backwardStanceVelocity+=backward;
            forwardToeVelocity+=toeForward;backwardToeVelocity+=toeBackward;++stancePairs;
            maximumReferenceSlip=std::max({maximumReferenceSlip,
                std::abs(config.movementSpeedMetresPerSecond+forward),
                std::abs(-config.movementSpeedMetresPerSecond+backward)});
        }
    }
    if(!Require(samples==17&&wrapPairs==1&&stancePairs>0,
                "backward actual-asset proof must include a full cycle, loop wrap and stance intervals"))return false;

    struct StanceSlip
    {
        unsigned intervals=0;
        double footSquared=0,toeSquared=0;
        float maximumFootSpeed=0,maximumToeSpeed=0;
    };
    const auto observeSlip=[](const ActualFootPose& before,const ActualFootPose& after,
                              float elapsed,StanceSlip& result) {
        for(unsigned side=0;side<2;++side)
        {
            if(before.soleY[side]>before.soleY[1-side]||after.soleY[side]>after.soleY[1-side])continue;
            const auto speed=[&](unsigned node) {
                return std::hypot(after.footAndToeWorld[node][0]-before.footAndToeWorld[node][0],
                                  after.footAndToeWorld[node][2]-before.footAndToeWorld[node][2])/elapsed;
            };
            const float foot=speed(side*2),toe=speed(side*2+1);
            result.footSquared+=double(foot)*foot;result.toeSquared+=double(toe)*toe;
            result.maximumFootSpeed=std::max(result.maximumFootSpeed,foot);
            result.maximumToeSpeed=std::max(result.maximumToeSpeed,toe);++result.intervals;
        }
    };
    auto actualBackward=std::make_unique<GameSimulation>(config);
    InputSnapshot backwardInput{};backwardInput.damageEnabled=false;
    backwardInput.moveForward=-1;backwardInput.yawRadians=1.57079632679f;
    const unsigned traceTicks=static_cast<unsigned>(std::ceil(2*duration*60))+3;
    StanceSlip reversedSlip,oldForwardGaitSlip;
    ActualFootPose previousReversed,previousOld;
    unsigned previousTick=0,traceSamples=0;
    float acceptedPath=0;
    bool havePrevious=false;
    for(unsigned tick=1;tick<=traceTicks;++tick)
    {
        const auto previous=actualBackward->Snapshot();
        actualBackward->StepFixed(backwardInput);
        const auto& current=actualBackward->Snapshot();
        acceptedPath+=std::hypot(current.playerX-previous.playerX,current.playerZ-previous.playerZ);
        if(tick%3)continue;
        ActualFootPose reversed,old;
        auto oldGait=current;
        // Counterfactual control: identical accepted actor displacement,
        // gameplay/weapon clocks and inputs, retaining the old forward gait.
        oldGait.playerAnimation.locomotionPhaseOffsetSeconds=0;
        if(!capture(current,reversed)||!capture(oldGait,old))return false;
        if(havePrevious)
        {
            const float elapsed=(tick-previousTick)/60.f;
            observeSlip(previousReversed,reversed,elapsed,reversedSlip);
            observeSlip(previousOld,old,elapsed,oldForwardGaitSlip);
        }
        previousReversed=std::move(reversed);previousOld=std::move(old);
        previousTick=tick;havePrevious=true;++traceSamples;
    }
    if(!Require(acceptedPath>0&&traceSamples>1&&reversedSlip.intervals>0&&oldForwardGaitSlip.intervals>0,
                "actual backwards stance comparison requires accepted travel and both gait contact windows"))return false;
    const auto rms=[](double squared,unsigned count){return std::sqrt(squared/count);};
    std::cout<<"Actual backward acceptedPathMetres="<<acceptedPath<<" traceSamples="<<traceSamples
        <<" reversed/oldForwardGaitStanceIntervals="<<reversedSlip.intervals<<'/'<<oldForwardGaitSlip.intervals
        <<" footWorldSpeedRmsMetresPerSecond="<<rms(reversedSlip.footSquared,reversedSlip.intervals)<<'/'
        <<rms(oldForwardGaitSlip.footSquared,oldForwardGaitSlip.intervals)
        <<" toeWorldSpeedRmsMetresPerSecond="<<rms(reversedSlip.toeSquared,reversedSlip.intervals)<<'/'
        <<rms(oldForwardGaitSlip.toeSquared,oldForwardGaitSlip.intervals)
        <<" footWorldSpeedMaxMetresPerSecond="<<reversedSlip.maximumFootSpeed<<'/'<<oldForwardGaitSlip.maximumFootSpeed
        <<" toeWorldSpeedMaxMetresPerSecond="<<reversedSlip.maximumToeSpeed<<'/'<<oldForwardGaitSlip.maximumToeSpeed
        <<"; actual skinned soles choose stance intervals; foot/toe node slip is measured, not a contact-solver or visual pass.\n";

    std::cout<<"Backward Walking asset pairs="<<samples<<" wrapPairs="<<wrapPairs
        <<" lower-sole stancePairs="<<stancePairs<<" maxPairedPoseErrorMetres="<<maximumPairError
        <<" maxAnalyticalPhaseErrorSeconds="<<maximumAnalyticalPhaseError
        <<" clockRoundoffBudgetSeconds="<<clockRoundoffBudget
        <<" meanFootForward/BackwardMetresPerSecond="<<forwardStanceVelocity/stancePairs<<'/'
        <<backwardStanceVelocity/stancePairs
        <<" meanToeForward/BackwardMetresPerSecond="<<forwardToeVelocity/stancePairs<<'/'
        <<backwardToeVelocity/stancePairs<<" maxReferenceWorldFootSlipMetresPerSecond="<<maximumReferenceSlip
        <<"; slip measured without an invented acceptance threshold; not authored backward run or native visual acceptance.\n";
    return true;
}

bool CheckBackwardSimulationTransitions(PlayerRenderSlot& rig,std::uint64_t& rigTick)
{
    auto config=ProductionGameSimulationConfig();
    config.playerStartX=-18.5f;config.playerStartZ=-15.2f;
    config.waterfallSkeletonEncounter=false;
    auto simulation=std::make_unique<GameSimulation>(config);
    InputSnapshot input{};input.damageEnabled=false;input.yawRadians=1.57079632679f;
    unsigned forwardTicks=0,backwardTicks=0,stoppedTicks=0;
    float maximumEffectivePhaseStep=0;
    std::string diagnostic;
    for(unsigned tick=0;tick<108;++tick)
    {
        input.moveForward=tick<24?1.f:tick<72?-1.f:tick<84?0.f:1.f;
        const auto before=simulation->Snapshot();
        simulation->StepFixed(input);
        const auto& after=simulation->Snapshot();
        const auto& a=before.playerAnimation;const auto& b=after.playerAnimation;
        const float distance=std::hypot(after.playerX-before.playerX,after.playerZ-before.playerZ);
        const float forward=(after.playerX-before.playerX)*std::sin(after.playerYawRadians)-
            (after.playerZ-before.playerZ)*std::cos(after.playerYawRadians);
        const float legacyAdvance=b.locomotionTime-a.locomotionTime;
        const float effectiveAdvance=legacyAdvance+b.locomotionPhaseOffsetSeconds-a.locomotionPhaseOffsetSeconds;
        maximumEffectivePhaseStep=std::max(maximumEffectivePhaseStep,std::abs(effectiveAdvance));
        if(!Require(std::abs(effectiveAdvance)<=std::max(legacyAdvance,distance/config.movementSpeedMetresPerSecond)+.00001f,
                    "direction changes advance continuously from the previous phase rather than negating absolute time"))return false;
        if(distance>.00001f&&forward<-.00001f)
        {
            ++backwardTicks;
            if(!Require(effectiveAdvance<0,"accepted backward travel must reverse the actual shared phase"))return false;
        }
        else if(distance>.00001f&&forward>.00001f)
        {
            ++forwardTicks;
            if(!Require(b.locomotionPhaseOffsetSeconds==a.locomotionPhaseOffsetSeconds,
                        "accepted forward travel preserves the exact nominal phase advance"))return false;
        }
        else if(distance<=.00001f)
        {
            ++stoppedTicks;
            if(!Require(std::abs(effectiveAdvance)<=.00001f,
                        "zero accepted travel must freeze locomotion phase while gameplay clocks continue"))return false;
        }
        if(tick%6==5)
        {
            bool updated=false;
            if(!PrepareRenderedPose(rig,after,rigTick++,updated,diagnostic)||!updated)
                return Require(false,diagnostic.c_str());
        }
    }
    if(!Require(forwardTicks>0&&backwardTicks>0&&stoppedTicks>0,
                "actual transition fixture must contain forward/backward/stopped travel"))return false;
    // A backwards command reaches the existing corridor wall naturally, then
    // continues with zero resolved displacement. No authoritative pose seeding.
    auto blocked=std::make_unique<GameSimulation>(config);input.yawRadians=0;input.moveForward=-1;
    unsigned moving=0,stationary=0;
    for(unsigned tick=0;tick<96;++tick)
    {
        const auto before=blocked->Snapshot();blocked->StepFixed(input);const auto& after=blocked->Snapshot();
        const float distance=std::hypot(after.playerX-before.playerX,after.playerZ-before.playerZ);
        const float phase=(after.playerAnimation.locomotionTime-before.playerAnimation.locomotionTime)+
            after.playerAnimation.locomotionPhaseOffsetSeconds-before.playerAnimation.locomotionPhaseOffsetSeconds;
        if(distance>.00001f)++moving;
        else if(moving)
        {
            ++stationary;
            if(!Require(std::abs(phase)<=.00001f,
                        "held backward input against a wall cannot advance either-direction gait phase"))return false;
        }
    }
    if(!Require(moving>0&&stationary>0,"backward wall fixture must include natural travel and actual blockage"))return false;
    if(!Require(blocked->Snapshot().playerAnimation.locomotionClip==PlayerLocomotionClip::Idle,
                "sustained blocked input must return to Idle instead of holding a raised walk foot"))return false;
    std::cout<<"Backward shared-phase transitions forward/back/stopped="<<forwardTicks<<'/'<<backwardTicks<<'/'
        <<stoppedTicks<<" maxContinuousPhaseStepSeconds="<<maximumEffectivePhaseStep
        <<" naturalWallMoving/BlockedTicks="<<moving<<'/'<<stationary<<'\n';
    return true;
}

}

int main(int argc, char** argv)
{
    if (!Require(argc == 2, "directional rig test requires the explicit source asset root")) return 1;
    const std::filesystem::path root(argv[1]);
    const auto rigPath = root / "assets/models/player/runtime/gothic-traveller-lod0.runtime.glb";
    PlayerRenderSlot rig;
    std::string diagnostic;
    if (!Require(rig.LoadAsset(rigPath.string(), diagnostic), diagnostic.c_str())) return 1;

    GameSimulation runSimulation(ProductionGameSimulationConfig());
    InputSnapshot runInput{};
    runInput.damageEnabled = false;
    runInput.yawRadians = 0.37f;
    runInput.pitchRadians = 0.18f;
    runInput.commands.runToggle = 1u;
    constexpr std::array<std::array<float, 2u>, 4u> directions{{
        {{1.0f, 0.0f}}, {{-1.0f, 0.0f}}, {{0.0f, -1.0f}}, {{0.0f, 1.0f}}}};
    std::uint64_t rigTick = 1u;
    for (const auto& direction : directions)
    {
        runInput.moveForward = direction[0];
        runInput.moveStrafe = direction[1];
        for (int tick = 0; tick < 12; ++tick)
        {
            runSimulation.StepFixed(runInput);
            const auto& snapshot = runSimulation.Snapshot();
            bool poseUpdated = false;
            if (!PrepareRenderedPose(rig, snapshot, rigTick++, poseUpdated, diagnostic))
                return Require(false, diagnostic.c_str()) ? 0 : 1;
            if (!poseUpdated || !rig.SolvedPose().IsValid() || rig.UniqueVertices().empty())
                return Require(false, "rights-cleared runtime rig must skin each directional stride") ? 0 : 1;
            const float leftError = rig.LeftSocketErrorMetres();
            const float rightError = rig.RightSocketErrorMetres();
            if (leftError > kPlayerGripSocketToleranceMetres ||
                rightError > kPlayerGripSocketToleranceMetres)
            {
                std::cerr << "Directional rig socket error at tick " << rigTick - 1u
                          << " clip=" << static_cast<int>(snapshot.playerAnimation.locomotionClip)
                          << " blend=" << snapshot.playerAnimation.locomotionBlend
                          << " left=" << leftError << " right=" << rightError
                          << " tolerance=" << kPlayerGripSocketToleranceMetres << '\n';
                return Require(false, "runtime hand sockets must retain the admitted grip tolerance") ? 0 : 1;
            }
            if (!Require(Near(snapshot.playerYawRadians, runInput.yawRadians) &&
                         Near(snapshot.playerPitchRadians, runInput.pitchRadians) &&
                         Near(snapshot.movementForward, direction[0]) &&
                         Near(snapshot.movementStrafe, direction[1]) &&
                         snapshot.playerAnimation.locomotionClip == MapPlayerLocomotionClip(
                             snapshot.playerAnimation.locomotionBlend),
                         "directional movement stays independent from aim and maps through supported Idle/Walking slots")) return 1;
        }
    }

    GameSimulation measuredWalk;
    InputSnapshot walkInput{};
    walkInput.damageEnabled = false;
    walkInput.moveForward = 1.0f;
    for (int tick = 0; tick < 48; ++tick) measuredWalk.StepFixed(walkInput);
    if (!Require(runSimulation.Snapshot().runActive &&
                 runSimulation.Snapshot().playerAnimation.locomotionTime >
                     measuredWalk.Snapshot().playerAnimation.locomotionTime &&
                 runSimulation.Snapshot().playerAnimation.combatLayer.action ==
                     PlayerUpperBodyAction::None,
                 "run changes actual shared locomotion phase without taking combat-layer priority")) return 1;

    for (const auto& direction : directions)
    {
        auto dodgeConfig = ProductionGameSimulationConfig();
        // Measure the authored dodge in the open chamber so the legacy narrow
        // corridor opening cannot clip its endpoint and obscure the distance
        // disposition this proof is checking.
        dodgeConfig.playerStartX = -5.5f;
        dodgeConfig.playerStartZ = -13.5f;
        dodgeConfig.waterfallSkeletonEncounter = false;
        dodgeConfig.swordStartsStowed = false;
        GameSimulation dodgeSimulation(dodgeConfig);
        InputSnapshot dodgeInput{};
        dodgeInput.damageEnabled = false;
        dodgeInput.yawRadians = runInput.yawRadians;
        dodgeInput.pitchRadians = runInput.pitchRadians;
        dodgeInput.moveForward = direction[0];
        dodgeInput.moveStrafe = direction[1];
        dodgeInput.commands.runToggle = 1u;
        dodgeInput.commands.dodge = 1u;
        const auto start = dodgeSimulation.Snapshot();
        if (!Near(start.playerX, dodgeConfig.playerStartX) ||
            !Near(start.playerZ, dodgeConfig.playerStartZ))
        {
            std::cerr << "Directional dodge chamber spawn rejected; actual start="
                      << start.playerX << ',' << start.playerZ << '\n';
            return Require(false, "dodge proof starts in the open chamber") ? 0 : 1;
        }
        for (int tick = 0; tick < 12; ++tick)
        {
            dodgeSimulation.StepFixed(dodgeInput);
            dodgeInput.moveForward = 0.0f;
            dodgeInput.moveStrafe = 0.0f;
            const auto& snapshot = dodgeSimulation.Snapshot();
            bool poseUpdated = false;
            const auto poseTick = rigTick++;
            if (!PrepareRenderedPose(rig, snapshot, poseTick, poseUpdated, diagnostic))
            {
                std::cerr << "Directional dodge pose rejected: " << diagnostic << '\n';
                return Require(false, "actual directional dodge pose must pass renderer rig admission") ? 0 : 1;
            }
            if (!poseUpdated || !rig.SolvedPose().IsValid() ||
                rig.LeftSocketErrorMetres() > kPlayerGripSocketToleranceMetres ||
                rig.RightSocketErrorMetres() > kPlayerGripSocketToleranceMetres)
            {
                std::cerr << "Directional dodge socket failure at tick " << poseTick
                          << " clip=" << static_cast<int>(snapshot.playerAnimation.locomotionClip)
                          << " blend=" << snapshot.playerAnimation.locomotionBlend
                          << " updated=" << poseUpdated
                          << " left=" << rig.LeftSocketErrorMetres()
                          << " right=" << rig.RightSocketErrorMetres()
                          << " tolerance=" << kPlayerGripSocketToleranceMetres << '\n';
                return Require(false, "directional dodge retains the admitted real-rig socket tolerance") ? 0 : 1;
            }
        }
        const auto& result = dodgeSimulation.Snapshot();
        const float distance = std::hypot(result.playerX - start.playerX,
                                          result.playerZ - start.playerZ);
        if (!(distance > 0.82f && distance < 0.98f &&
              !result.runToggleActive && !result.runActive &&
              result.lastConsumedDodgeSequence == 1u))
        {
            std::cerr << "Directional dodge disposition distance=" << distance
                      << " dodgeSequence=" << result.lastConsumedDodgeSequence
                      << " runToggle=" << result.runToggleActive
                      << " runActive=" << result.runActive
                      << " direction=" << direction[0] << ',' << direction[1]
                      << " action=" << static_cast<int>(result.playerCombat.action) << '\n';
            return Require(false, "forward/back/left/right dodges keep authored distance with run cancelled") ? 0 : 1;
        }
    }

    // Exercise the real route support profile against the same prepared host
    // geometry consumed by the renderer. This is CPU geometry/readiness
    // admission only; it makes no claim about BLAS upload or RT presentation.
    const auto worldGeometry = horde::scene::PrepareDevelopmentWorldGeometry(false);
    if (!Require(worldGeometry.valid &&
                 horde::scene::ValidateDevelopmentWorldGeometry(worldGeometry),
                 "directional rig route proof requires valid prepared world geometry")) return 1;
    // The rebased rescue route shares X/Z with retained dungeon roof planes.
    // At F01 the player stands above those solids, so they are not ceilings
    // for either hand prop. Keep the measured torch/sword envelope solvers and
    // all socket tolerances active; only support-relative ownership changes.
    HeldItemKinematicsInput aboveDungeonRoof{};
    aboveDungeonRoof.cameraX = kWorldRoutePoints[2].x;
    aboveDungeonRoof.cameraZ = kWorldRoutePoints[2].z;
    aboveDungeonRoof.cameraYawRadians = 0.0f;
    aboveDungeonRoof.playerSupportWorldY = kWorldRoutePoints[2].y;
    aboveDungeonRoof.developmentWorldRoute = true;
    aboveDungeonRoof.playerMountProfile = PlayerMountProfile::AnatomicalBody;
    const auto aboveRoofPose = EvaluateHeldItemKinematics(aboveDungeonRoof);
    if (!Require(Near(aboveRoofPose.torchOverheadLowering, 0.0f, 0.000001f) &&
                 Near(aboveRoofPose.swordOverheadLowering, 0.0f, 0.000001f),
                 "dungeon roofs below rebased F01 support do not lower exterior held props")) return 1;
    auto routeConfig = ProductionGameSimulationConfig();
    auto routeSimulation = std::make_unique<GameSimulation>(routeConfig);
    routeSimulation->SetDevelopmentWorldRoute(true);
    bool cpuGeometryReadinessAdmitted = true;
    for (std::size_t zone = 1u; zone < kWorldZones.size(); ++zone)
    {
        cpuGeometryReadinessAdmitted &= routeSimulation->PublishWorldZoneReadiness(
            {routeSimulation->Snapshot().worldRoute.generation,
             static_cast<WorldZoneId>(zone)}, ZoneReadiness::Ready);
    }
    if (!Require(cpuGeometryReadinessAdmitted,
                 "prepared CPU geometry must admit the route host-readiness descriptors")) return 1;

    InputSnapshot routeInput{};
    routeInput.damageEnabled = false;
    routeInput.yawRadians = 3.14159265359f;
    // The playable rescue route starts at F01. Segment 0 and its low-roof
    // step laboratory remain separately covered by the support/clearance
    // fixtures; they are excluded from the actual rescue-scene mesh.
    routeInput.hasAuthoritativePlayerPose = true;
    routeInput.authoritativePlayerX = kWorldRoutePoints[2].x;
    routeInput.authoritativePlayerZ = kWorldRoutePoints[2].z;
    routeSimulation->StepFixed(routeInput);
    routeInput.hasAuthoritativePlayerPose = false;
    routeInput.moveForward = 1.0f;
    bool sawRaisedSlope = false;
    std::uint64_t routePoseSamples = 0u;
    auto inspectRoutePose = [&]() {
        const auto& snapshot = routeSimulation->Snapshot();
        bool updated = false;
        HeldItemStates items{};
        HeldLightState light{};
        if (!PrepareRenderedPose(rig, snapshot, rigTick++, updated, diagnostic,
                                 &items, &light))
        {
            std::cerr << "World-route pose rejected tick=" << snapshot.tickIndex
                      << " root=" << snapshot.playerX << "," << snapshot.playerSupportWorldY
                      << "," << snapshot.playerZ << ": " << diagnostic << '\n';
            return false;
        }
        if (!updated || !rig.SolvedPose().IsValid() || rig.UniqueVertices().empty() ||
            rig.LeftSocketErrorMetres() > kPlayerGripSocketToleranceMetres ||
            rig.RightSocketErrorMetres() > kPlayerGripSocketToleranceMetres)
        {
            std::cerr << "World-route rig failure tick=" << snapshot.tickIndex
                      << " support=" << snapshot.playerSupportWorldY
                      << " left=" << rig.LeftSocketErrorMetres()
                      << " right=" << rig.RightSocketErrorMetres() << '\n';
            return false;
        }
        const auto composedFlame = MultiplyHeldItemTransforms(
            items[0].worldFromItem, PlayerRagTorchFlameSocketTransform());
        const auto composedLight = MultiplyHeldItemTransforms(
            items[0].worldFromItem, PlayerRagTorchLightSocketTransform());
        if (items[0].id != HeldItemId::OriginalTorch || items[1].id != HeldItemId::Sword ||
            items[0].parentMode != HeldItemParentMode::HandSocket ||
            items[1].parentMode != HeldItemParentMode::BodyStow || !light.active ||
            !TransformsNear(composedFlame, light.worldFromFlame) ||
            !TransformsNear(composedLight, light.worldFromLight))
        {
            std::cerr << "World-route equipment attachment failure tick=" << snapshot.tickIndex
                      << " torchParent=" << static_cast<int>(items[0].parentMode)
                      << " swordParent=" << static_cast<int>(items[1].parentMode)
                      << " lightActive=" << light.active << '\n';
            return false;
        }
        ++routePoseSamples;
        return true;
    };
    const auto walkRouteTo = [&](const WorldRoutePoint& target, const float forward) {
        routeInput.yawRadians = 3.14159265359f;
        routeInput.moveForward = forward;
        auto previousSupportId = routeSimulation->Snapshot().playerSupportId;
        for (std::uint32_t tick = 0u; tick < 1800u; ++tick)
        {
            const auto& before = routeSimulation->Snapshot();
            if (std::hypot(target.x - before.playerX, target.z - before.playerZ) < 0.035f)
                return true;
            const float dx = target.x - before.playerX;
            const float dz = target.z - before.playerZ;
            routeInput.yawRadians = std::atan2(dx, -dz) +
                (forward < 0.0f ? 3.14159265359f : 0.0f);
            routeInput.moveForward = forward * std::min(1.0f, std::hypot(dx, dz) / 0.05f);
            routeSimulation->StepFixed(routeInput);
            const auto& snapshot = routeSimulation->Snapshot();
            if (!snapshot.playerGrounded ||
                (snapshot.playerSupportSurface != SupportSurface::Stone &&
                 snapshot.playerSupportSurface != SupportSurface::Earth))
            {
                std::cerr << "World-route support lost tick=" << snapshot.tickIndex
                          << " x=" << snapshot.playerX << " z=" << snapshot.playerZ
                          << " y=" << snapshot.playerSupportWorldY << '\n';
                return false;
            }
            sawRaisedSlope |= snapshot.playerSupportWorldY > kRouteFloorWorldY + 1.0f &&
                snapshot.playerSupportWorldY < kWorldRoutePoints[2].y - 0.05f;
            if (tick % 20u == 19u || snapshot.playerSupportId != previousSupportId)
            {
                if (!inspectRoutePose()) return false;
            }
            previousSupportId = snapshot.playerSupportId;
        }
        std::cerr << "World-route travel timed out at x=" << routeSimulation->Snapshot().playerX
                  << " z=" << routeSimulation->Snapshot().playerZ
                  << " support=" << routeSimulation->Snapshot().playerSupportWorldY
                  << " target=" << target.x << ',' << target.z << '\n';
        return false;
    };
    if (!inspectRoutePose()) return 1;
    for (std::size_t waypoint = 3; waypoint <= 4; ++waypoint)
        if (!Require(walkRouteTo(kWorldRoutePoints[waypoint], 1.0f),
                     "real-rig route travel must reach actual exterior terrain")) return 1;
    if (!Require(sawRaisedSlope && routePoseSamples >= 10u,
                 "real-rig exterior route must sample supported slopes with strict grips")) return 1;
    for (std::size_t waypoint = 4; waypoint-- > 2;)
        if (!Require(walkRouteTo(kWorldRoutePoints[waypoint], -1.0f),
                     "real-rig backward travel must return to the physical rescue landing")) return 1;
    if (!Require(Near(routeSimulation->Snapshot().playerSupportWorldY,
                      kWorldRoutePoints[2].y, 0.03f) &&
                 routeSimulation->Snapshot().playerGrounded && routePoseSamples >= 20u,
                 "real-rig route return must finish on F01 after bounded grip samples")) return 1;
    if (!inspectRoutePose())
        return Require(false, "returned F01 pose retains real equipment and light attachments") ? 0 : 1;

    if(!CheckBackwardFootSampling(root,rig,rigTick)||!CheckBackwardSimulationTransitions(rig,rigTick))return 1;
    std::cout << "Directional runtime rig, aim independence, run stride, dodge disposition, route support poses, and attachment sockets passed; dedicated run/back/strafe/dodge clips remain absent; route_pose_samples="
              << routePoseSamples << " GPU_cost=not-measured.\n";
    return 0;
}
