#include "vulkan/raytracing/SimulationFrameAdapter.h"
#include "vulkan/raytracing/EnvironmentSurveyFrame.h"
#include "vulkan/raytracing/PlayerRenderSlot.h"
#include "gameplay/simulation/GameSimulation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>

namespace {
bool FramesWater(const horde::graphics::GraphicsPreviewDescription& scene,
                 const horde::vulkan::raytracing::RtSceneFrameInputs& frame,
                 const float aspect)
{
    using Point = std::array<float, 3u>;
    const auto dot = [](const Point& a, const Point& b) {
        return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
    };
    // Project authored targets through the fixed production RT camera: eye at
    // 0.70m, forward=(sin(yaw),pitch-0.05,-cos(yaw)), focal1.22/vertical0.74.
    // Geometry/frustum checks avoid a literal camera-coordinate assertion.
    const float y = std::clamp(frame.cameraPitch,-0.32f,0.28f)-0.05f;
    const float inverseLength = 1.0f/std::sqrt(1.0f+y*y);
    const float sine=std::sin(frame.cameraYaw), cosine=std::cos(frame.cameraYaw);
    const Point forward{sine*inverseLength,y*inverseLength,-cosine*inverseLength};
    const Point right{cosine,0.0f,sine};
    const Point up{-y*sine*inverseLength,inverseLength,y*cosine*inverseLength};
    float poolLow=std::numeric_limits<float>::max(), poolHigh=-poolLow;
    const auto project = [&](const Point& target, const bool pool) {
        const Point delta{target[0]-frame.cameraX,target[1]-0.70f,target[2]-frame.cameraZ};
        const float depth=dot(delta,forward);
        if (!(depth>0.01f)) return false;
        const float horizontal=1.22f*dot(delta,right)/(aspect*depth);
        const float vertical=1.22f*dot(delta,up)/(0.74f*depth);
        if (!std::isfinite(horizontal) || !std::isfinite(vertical) ||
            std::abs(horizontal)>0.95f || std::abs(vertical)>0.95f) return false;
        if (pool) { poolLow=std::min(poolLow,vertical); poolHigh=std::max(poolHigh,vertical); }
        return true;
    };
    for (const auto& quad : scene.waterQuads)
        for (const auto& vertex : quad.vertices) if (!project(vertex,true)) return false;
    if (!(poolHigh-poolLow>0.18f)) return false;
    for (const auto& stream : scene.waterStreams)
        for (const float height : {scene.waterRingHeights.front(),scene.waterRingHeights.back()})
            for (const float sideX : {-1.0f,1.0f})
                for (const float sideZ : {-1.0f,1.0f})
                    if (!project({scene.waterOrigin[0]+sideX*stream.radiusX,height,
                                  scene.waterOrigin[2]+stream.centreZ+sideZ*stream.radiusZ},false)) return false;
    return true;
}
}

int main()
{
    using namespace horde::vulkan::raytracing;
    bool passed = true;
    const auto check = [&passed](bool condition, const char* label) {
        if (!condition) { std::cerr << label << "\n"; passed = false; }
    };
    horde::gameplay::simulation::SimulationSnapshot game{};
    game.tickIndex = 417;
    game.playerX = 0.31f;
    const auto legacy = BuildRtSceneFrameInputs(game, 1.0f, WaterQuality::High);
    check(!legacy.fireDetail, "legacy adapters retain coupled fixture fallback");
    const auto independent = BuildRtSceneFrameInputs(game, 1.0f, WaterQuality::Off, RtSceneTuning{}, FireEmitterQuality::High);
    check(independent.waterQuality == WaterQuality::Off && independent.fireDetail == FireEmitterQuality::High,
          "explicit fire quality remains independent of water off");
    const auto lowerFireHigherShadow = BuildRtSceneFrameInputs(game,1.0f,WaterQuality::Off,
        RtSceneTuning{},FireEmitterQuality::Low,horde::graphics::ShadowQuality::Higher);
    check(lowerFireHigherShadow.fireDetail == FireEmitterQuality::Low &&
          lowerFireHigherShadow.shadowQuality == horde::graphics::ShadowQuality::Higher &&
          lowerFireHigherShadow.tuning.workloadPreset == RtWorkloadPreset::Authored &&
          lowerFireHigherShadow.tuning.fogDensityScale == 1.0f && game.tickIndex == 417,
          "independent shadow/fire adapter does not promote Max/mist or mutate simulation");
    horde::graphics::GraphicsPreviewSession session;
    horde::gameplay::effects::FireEmitterFixedStepInput torch;
    torch.worldFromFlame[12] = -3.65f; torch.worldFromFlame[13] = 0.965f;
    torch.worldFromLight[12] = -3.65f; torch.worldFromLight[13] = 0.935f;
    session.SetFireSockets(0, torch);
    session.SetMotion(true);
    session.SelectCamera(horde::graphics::GraphicsPreviewCamera::Skeleton);
    session.Advance(0.1);
    const auto frame = BuildGraphicsPreviewFrameInputs(session, 1.2f, WaterQuality::Mobile, FireEmitterQuality::High);
    check(frame.tickIndex == 6 && frame.walkTime == 0.1f, "preview fixed clock is the frame time authority");
    check(frame.previewMotion && frame.outputExposure == 1.2f, "preview motion and exposure survive adaptation");
    check(frame.skeletonEnemyCount == 1 && frame.roster.renderedEnemyCount == 1 &&
          frame.skeletonEnemies[0].animation == horde::gameplay::EnemyAnimation::Idle &&
          frame.skeletonEnemies[0].action == horde::gameplay::EnemyCombatAction::Locomotion, "real single idle actor admission");
    check(frame.fireEmitterCount == 2 && frame.fireEmitters[0].worldFromFlame == torch.worldFromFlame &&
          frame.fireEmitters[0].worldFromLight == torch.worldFromLight, "actual socket matrices reach preview fire inputs");
    check(frame.fireDetail == FireEmitterQuality::High && frame.waterQuality == WaterQuality::Mobile, "preview explicit fire split");
    check(frame.lich.staffLightStrength == 0.0f, "unadmitted lich has no light");
    session.Pause(true); session.Advance(0.1);
    const auto paused = BuildGraphicsPreviewFrameInputs(session, 1.2f, WaterQuality::Off, FireEmitterQuality::Mobile);
    check(paused.tickIndex == frame.tickIndex && paused.skeletonEnemies[0].animationTime == frame.skeletonEnemies[0].animationTime,
          "paused preview keeps animation time fixed");
    session.Reset();
    const auto reset = BuildGraphicsPreviewFrameInputs(session, 1.2f, WaterQuality::Mobile, FireEmitterQuality::High);
    check(reset.tickIndex == 0 && reset.skeletonEnemies[0].animationTime == 0.0f && reset.fireEmitters[0].phase == 0.0f,
          "comparison reset rewinds actor and fire timeline");
    check(game.tickIndex == 417 && game.playerX == 0.31f, "preview adaptation preserves paused game snapshot");
    session.SetMotion(false);
    session.SelectCamera(horde::graphics::GraphicsPreviewCamera::Water);
    const auto water = BuildGraphicsPreviewFrameInputs(session,1.0f,WaterQuality::High,FireEmitterQuality::High);
    const auto description = horde::graphics::MakeGraphicsPreviewDescription();
    check(FramesWater(description,water,16.0f/9.0f) && FramesWater(description,water,9.0f/16.0f),
          "Water preset frames readable pool and falling-stream extents in landscape and portrait");
    auto missedPool = water;
    missedPool.cameraX=-3.55f; missedPool.cameraZ=-14.40f;
    missedPool.cameraYaw=0.90f; missedPool.cameraPitch=-0.12f;
    check(!FramesWater(description,missedPool,16.0f/9.0f),
          "frustum fixture rejects the actual former close pose that cropped the pool");
    const auto previewBefore = session.Snapshot();
    const auto selected = BuildGraphicsPreviewFrameInputs(session,1.0f,WaterQuality::Mobile,
        FireEmitterQuality::Low,RtSceneTuning{},horde::graphics::ShadowQuality::Current);
    check(selected.fireDetail == FireEmitterQuality::Low && selected.shadowQuality == horde::graphics::ShadowQuality::Current &&
          selected.tuning.workloadPreset == RtWorkloadPreset::Authored &&
          session.Snapshot().tick == previewBefore.tick && session.Snapshot().timeSeconds == previewBefore.timeSeconds,
          "preview independent policy adapter consumes immutable timeline without advancing gameplay");
    horde::graphics::EntryMenuSession entry;
    entry.Advance(1.0/30);
    const auto entryFrame=BuildEntryMenuFrameInputs(entry,.92f,FireEmitterQuality::High,
        horde::graphics::ShadowQuality::Current);
    check(entryFrame.tickIndex==2 && entryFrame.skeletonEnemyCount==0 &&
          entryFrame.fireEmitterCount==1 && entryFrame.waterQuality==WaterQuality::Off &&
          entryFrame.lich.staffLightStrength==0 && entryFrame.torchLightStrength==0,
          "entry admits one socket-bound flame without gameplay actors or duplicate torch light");
    entry.Play();entry.Advance(.25);entry.Advance(.10);
    const auto black=BuildEntryMenuFrameInputs(entry,.92f,FireEmitterQuality::High,
        horde::graphics::ShadowQuality::Current);
    check(entry.ReadyToPlay() && black.outputExposure==0 &&
          game.tickIndex==417 && game.playerX==.31f,
          "Play reaches explicit black presentation without advancing paused gameplay");
    // Reproduce the first hidden survey failure through the ordinary fresh
    // production snapshot; no pose/checkpoint import or simulation advancement.
    using namespace horde::gameplay::items;
    auto config=horde::gameplay::simulation::ProductionGameSimulationConfig();
    config.developmentRescueJourney=true;
    horde::gameplay::simulation::GameSimulation simulation(config);
    const auto frozen=simulation.Snapshot();
    auto surveySource=BuildRtSceneFrameInputs(frozen,.62f,WaterQuality::High);
    const auto leftGripFor=[](const RtSceneFrameInputs& f) {
        const float y=std::clamp(f.cameraPitch,-.32f,.28f)-.05f;
        const float inv=1.f/std::sqrt(1.f+y*y),sy=std::sin(f.cameraYaw),cy=std::cos(f.cameraYaw);
        const std::array<float,3> right{cy,0,sy},up{-sy*y*inv,inv,cy*y*inv},forward{sy*inv,y*inv,-cy*inv};
        auto out=IdentityHeldItemTransform();
        const auto& k=f.heldItemKinematics;
        const std::array<std::array<float,3>,3> local{k.leftGripXInView,k.leftGripYInView,k.leftGripZInView};
        for(unsigned axis=0;axis<3;++axis)for(unsigned row=0;row<3;++row)
            out[axis*4+row]=right[row]*local[axis][0]+up[row]*local[axis][1]+forward[row]*local[axis][2];
        for(unsigned row=0;row<3;++row)
            out[12+row]=right[row]*k.leftHandLocal[0]+up[row]*k.leftHandLocal[1]+forward[row]*k.leftHandLocal[2];
        out[12]+=f.cameraX;
        out[13]+=horde::gameplay::simulation::PlayerEyeWorldY(f.playerSupportWorldY)-f.playerCrouchDropMetres;
        out[14]+=f.cameraZ;return out;
    };
    const auto gripError=[&](const RtSceneFrameInputs& f) {
        HeldItemTransform visual{};std::string error;
        if(!ComposeWorldFromItem(leftGripFor(f),PlayerRagTorchGripSocketTransform(),visual,error))return 1000000.f;
        auto rendered=f.heldItems[0];rendered.worldFromItem=visual;
        return MeasurePlayerGripAgreement(f.heldItems[0],rendered).positionErrorMetres;
    };
    check(gripError(surveySource)<.001f,"fresh ordinary production frame has coherent held torch Grip");
    const EnvironmentSurveyPose pose{136.3f,131.2f,-24.4464f,3.14159265f,-.1368f,.5f};
    auto oldBroken=surveySource;oldBroken.cameraX=pose.x;oldBroken.cameraZ=pose.z;
    oldBroken.cameraYaw=pose.yaw;oldBroken.cameraPitch=pose.pitch;oldBroken.playerSupportWorldY=pose.supportY;
    check(gripError(oldBroken)>100.f,"camera-only survey override reproduces actual distant held Grip failure");
    std::string rebaseError;
    auto survey=surveySource;
    check(RebaseEnvironmentSurveyFrame(survey,pose,rebaseError) && rebaseError.empty(),"coherent render-only survey frame is admitted");
    check(gripError(survey)<.001f,"rebase preserves ordinary final held Grip tolerance at nonzero yaw/pitch and distant support");
    const auto rigidError=MeasureTransformAgreement(
        MultiplyHeldItemTransforms(survey.heldItems[0].worldFromItem,PlayerRagTorchLightSocketTransform()),
        survey.heldLight.worldFromLight);
    check(rigidError.positionErrorMetres<.001f && rigidError.orientationErrorRadians<.002f,
          "held light remains on the same real item socket after rebase");
    check(survey.playerAnimation==surveySource.playerAnimation &&
          survey.heldItemKinematics.leftHandLocal==surveySource.heldItemKinematics.leftHandLocal &&
          survey.heldItemKinematics.leftGripXInView==surveySource.heldItemKinematics.leftGripXInView &&
          survey.tickIndex==frozen.tickIndex && survey.chestReward==frozen.chestReward &&
          survey.rescue.firstAscentCompleted==frozen.rescue.firstAscentCompleted &&
          survey.lich.x==surveySource.lich.x && survey.lich.z==surveySource.lich.z &&
          simulation.Snapshot().playerX==frozen.playerX && simulation.Snapshot().playerZ==frozen.playerZ &&
          simulation.Snapshot().tickIndex==frozen.tickIndex && simulation.Snapshot().heldItems[0].worldFromItem==frozen.heldItems[0].worldFromItem,
          "survey changes render inputs only, retaining local IK and all frozen simulation/progression");
    // Body stow uses the body yaw/height frame, not the camera's pitched frame.
    const auto sourceSword=surveySource.heldItems[1].worldFromItem;
    check(surveySource.heldItems[1].parentMode==HeldItemParentMode::BodyStow &&
          std::abs(survey.heldItems[1].worldFromItem[13]-sourceSword[13]-(pose.supportY-surveySource.playerSupportWorldY))<.001f,
          "stowed sword follows grounded body height without camera pitch rotation");
    auto roundTrip=survey;
    check(RebaseEnvironmentSurveyFrame(roundTrip,{surveySource.cameraX,surveySource.cameraZ,
          surveySource.playerSupportWorldY,surveySource.cameraYaw,surveySource.cameraPitch,surveySource.walkTime},rebaseError) &&
          MeasureTransformAgreement(roundTrip.heldItems[0].worldFromItem,surveySource.heldItems[0].worldFromItem).positionErrorMetres<.001f,
          "render-only rigid rebase round trip does not accumulate an item offset");
    auto emitters=surveySource;emitters.fireEmitterCount=2;
    emitters.fireEmitters[0].parentObject=horde::gameplay::effects::FireEmitterParentObject::OriginalTorch;
    emitters.fireEmitters[0].worldFromFlame=emitters.heldLight.worldFromFlame;
    emitters.fireEmitters[0].worldFromLight=emitters.heldLight.worldFromLight;
    emitters.fireEmitters[0].previousPivotPosition={1,2,3};
    emitters.fireEmitters[0].previousPivotVelocity={.2f,.3f,.4f};
    emitters.fireEmitters[0].leanX=.1f;emitters.fireEmitters[0].leanZ=-.1f;
    emitters.fireEmitters[1].parentObject=horde::gameplay::effects::FireEmitterParentObject::WorldObject;
    emitters.fireEmitters[1].worldFromFlame[12]=29;
    const auto worldFire=emitters.fireEmitters[1];
    check(RebaseEnvironmentSurveyFrame(emitters,pose,rebaseError) &&
          MeasureTransformAgreement(emitters.fireEmitters[0].worldFromLight,emitters.heldLight.worldFromLight).positionErrorMetres<.001f &&
          emitters.fireEmitters[0].previousPivotPosition!=std::array<float,3>{1,2,3} &&
          emitters.fireEmitters[0].leanX==.1f && emitters.fireEmitters[0].leanZ==-.1f &&
          emitters.fireEmitters[1].worldFromFlame==worldFire.worldFromFlame &&
          emitters.fireEmitters[1].worldFromLight==worldFire.worldFromLight,
          "only attached player fire transforms/history rebase; local lean and world lights remain fixed");
    auto dropped=surveySource;dropped.heldItems[0].parentMode=HeldItemParentMode::AuthoredWorldTrajectory;
    dropped.heldItems[0].detached=true;dropped.fireEmitterCount=1;
    dropped.fireEmitters[0].parentObject=horde::gameplay::effects::FireEmitterParentObject::OriginalTorch;
    const auto droppedBefore=dropped;
    check(RebaseEnvironmentSurveyFrame(dropped,pose,rebaseError) &&
          dropped.heldItems[0].worldFromItem==droppedBefore.heldItems[0].worldFromItem &&
          dropped.heldItems[0].worldFromDetach==droppedBefore.heldItems[0].worldFromDetach &&
          dropped.heldLight.worldFromLight==droppedBefore.heldLight.worldFromLight &&
          dropped.fireEmitters[0].worldFromFlame==droppedBefore.fireEmitters[0].worldFromFlame &&
          dropped.torchFailure.droppedX==droppedBefore.torchFailure.droppedX,
          "detached torch trajectory and its world light remain at actual world position");
    auto lantern=surveySource;
    lantern.heldItems[0].id=HeldItemId::RewardLantern;
    lantern.rewardLanternWorldFromHinge=lantern.heldItems[0].worldFromItem;
    lantern.lanternPendulum.worldFromBody=lantern.rewardLanternWorldFromHinge;
    lantern.lanternPendulum.worldFromBody[13]-=.4f;
    lantern.lanternPendulum.previousPivotPosition={1,2,3};
    lantern.lanternPendulum.previousHandForward={0,0,1};
    lantern.fireEmitterCount=1;
    lantern.fireEmitters[0].parentObject=horde::gameplay::effects::FireEmitterParentObject::RewardLantern;
    lantern.fireEmitters[0].worldFromLight=lantern.heldLight.worldFromLight;
    check(RebaseEnvironmentSurveyFrame(lantern,pose,rebaseError) &&
          MeasureTransformAgreement(lantern.rewardLanternWorldFromHinge,lantern.heldItems[0].worldFromItem).positionErrorMetres<.001f &&
          std::abs(std::hypot(std::hypot(lantern.lanternPendulum.worldFromBody[12]-lantern.rewardLanternWorldFromHinge[12],
              lantern.lanternPendulum.worldFromBody[13]-lantern.rewardLanternWorldFromHinge[13]),
              lantern.lanternPendulum.worldFromBody[14]-lantern.rewardLanternWorldFromHinge[14])-.4f)<.001f &&
          MeasureTransformAgreement(lantern.fireEmitters[0].worldFromLight,lantern.heldLight.worldFromLight).positionErrorMetres<.001f,
          "carried lantern hinge, pendulum body and matching emitter share one rigid render rebase");
    auto corrupt=surveySource;corrupt.fireEmitterCount=1;
    corrupt.fireEmitters[0].previousPivotVelocity[0]=std::numeric_limits<float>::quiet_NaN();
    const auto corruptBefore=corrupt;
    check(!RebaseEnvironmentSurveyFrame(corrupt,pose,rebaseError) &&
          corrupt.heldItems[0].worldFromItem==corruptBefore.heldItems[0].worldFromItem &&
          corrupt.cameraX==corruptBefore.cameraX,
          "late nonfinite attached fire history rejects without partially rebasing earlier fields");
    for(unsigned badCase=0;badCase<6;++badCase) {
        auto invalid=surveySource;auto badPose=pose;
        if(badCase==0)badPose.pitch=-.698f;
        if(badCase==1)badPose.x=std::numeric_limits<float>::quiet_NaN();
        if(badCase==2)invalid.walkAmount=.2f;
        if(badCase==3)invalid.rescue.phase=horde::gameplay::traversal::Phase::Ascent;
        if(badCase==4)invalid.heldItems[1].visualStowBlend=.5f;
        if(badCase==5)invalid.playerAnimation.dodge={.5f,0,1};
        const auto before=invalid;
        check(!RebaseEnvironmentSurveyFrame(invalid,badPose,rebaseError) && !rebaseError.empty() &&
              invalid.cameraX==before.cameraX && invalid.heldItems[0].worldFromItem==before.heldItems[0].worldFromItem &&
              invalid.playerAnimation==before.playerAnimation && invalid.tickIndex==before.tickIndex,
              "invalid or unsettled survey request rejects without frame mutation");
    }
    return passed ? 0 : 1;
}
