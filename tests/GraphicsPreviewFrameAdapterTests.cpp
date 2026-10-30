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

template <typename Check>
void AttackWarningAdapterContracts(const Check& check)
{
    using namespace horde::gameplay;
    using namespace horde::gameplay::simulation;
    using namespace horde::vulkan::raytracing;
    SimulationSnapshot source{};
    source.modernCombatRules=true; source.tickIndex=782;
    source.playerX=4.f; source.playerZ=6.f; source.playerSupportWorldY=2.f;
    source.activeEnemyKind=EnemyKind::Skeleton; source.skeletonAttackerId=EntityId::SkeletonA;
    source.swordCombat.attackerIndex=0;
    source.skeletonEnemyCount=2;
    source.skeletonEnemies[0].id=EntityId::SkeletonA;
    source.skeletonEnemies[0].x=2.f; source.skeletonEnemies[0].z=-4.f;
    source.skeletonEnemies[0].worldY=1.f;
    source.skeletonEnemies[1].id=EntityId::SkeletonB;
    source.skeletonEnemies[1].x=-3.f; source.skeletonEnemies[1].z=-9.f;
    source.skeletonEnemies[1].worldY=2.f;
    const auto setPhase=[](SimulationSnapshot& state,EnemyAttackKind kind,
                           EnemyCombatAction action,float seconds) {
        const auto index=static_cast<std::size_t>(state.swordCombat.attackerIndex);
        auto& combat=state.swordCombat.combatants[index];
        combat.attackKind=kind; combat.action=action; combat.actionTime=seconds;
        auto& actor=state.skeletonEnemies[index];
        actor.attackKind=kind; actor.action=action; actor.actionTime=seconds;
        state.attackWarning=BuildAttackWarningPresentation(state.swordCombat,state.lich,
            state.activeEnemyKind,state.skeletonAttackerId,true);
    };
    setPhase(source,EnemyAttackKind::GuardBreak,EnemyCombatAction::AttackWindup,0.f);
    const RtAttackWarningOptions enabled{true,false}, reduced{true,true};
    const auto build=[&](const SimulationSnapshot& value,RtAttackWarningOptions option=RtAttackWarningOptions{true,false}) {
        return BuildRtSceneFrameInputs(value,1.f,WaterQuality::High,option);
    };
    const auto near=[](float a,float b) { return std::abs(a-b)<.00001f; };
    const auto axisLength=[](const RtSceneFrameInputs& value,std::size_t column) {
        const auto& m=value.attackWarningRingWorldFromItem;
        return std::sqrt(m[column*4]*m[column*4]+m[column*4+1]*m[column*4+1]+m[column*4+2]*m[column*4+2]);
    };
    const auto valid=[](const RtSceneFrameInputs& value) {
        const auto& m=value.attackWarningRingWorldFromItem;
        const float determinant=m[0]*(m[5]*m[10]-m[9]*m[6])-
            m[4]*(m[1]*m[10]-m[9]*m[2])+m[8]*(m[1]*m[6]-m[5]*m[2]);
        return std::all_of(m.begin(),m.end(),[](float v){return std::isfinite(v);}) && determinant>0.f;
    };
    check(!BuildRtSceneFrameInputs(source,1.f).attackWarningRingVisible &&
          !BuildRtSceneFrameInputs(source,1.f,WaterQuality::High).attackWarningRingVisible &&
          !BuildRtSceneFrameInputs(source,1.f,RtSceneTuning{}).attackWarningRingVisible &&
          !BuildRtSceneFrameInputs(source,1.f,WaterQuality::High,RtSceneTuning{}).attackWarningRingVisible &&
          !BuildRtSceneFrameInputs(source,1.f,RtSceneTuning{},WaterQuality::High,
              FireEmitterQuality::High,horde::graphics::ShadowQuality::Higher).attackWarningRingVisible &&
          !BuildRtSceneFrameInputs(source,1.f,WaterQuality::High,RtSceneTuning{},
              FireEmitterQuality::High,horde::graphics::ShadowQuality::Higher).attackWarningRingVisible &&
          !build(source,{false,true}).attackWarningRingVisible,
          "every established gameplay overload and disabled reduced-motion option keeps the warning OFF");
    const auto open=build(source);
    const auto tuningFirst=BuildRtSceneFrameInputs(source,1.f,RtSceneTuning{},WaterQuality::High,
        std::nullopt,std::nullopt,enabled);
    const auto waterFirst=BuildRtSceneFrameInputs(source,1.f,WaterQuality::High,RtSceneTuning{},
        std::nullopt,std::nullopt,enabled);
    check(open.attackWarningRingVisible && tuningFirst.attackWarningRingWorldFromItem==open.attackWarningRingWorldFromItem &&
          waterFirst.attackWarningRingWorldFromItem==open.attackWarningRingWorldFromItem &&
          valid(open) && near(axisLength(open,0),axisLength(open,1)),
          "explicit options forward consistently to one finite round yellow world-geometry presentation");
    const auto centre=open.attackWarningRingWorldFromItem;
    check(centre[12]==source.skeletonEnemies[0].x && centre[14]==source.skeletonEnemies[0].z &&
          centre[13]>source.skeletonEnemies[0].worldY &&
          centre[12]!=source.swordCombat.combatants[0].x,
          "ring uses the identified actual actor root and support height rather than a combat compatibility root");
    const float eyeY=PlayerEyeWorldY(source.playerSupportWorldY);
    const std::array<float,3> delta{source.playerX-centre[12],eyeY-centre[13],source.playerZ-centre[14]};
    const float facing=centre[8]*delta[0]+centre[9]*delta[1]+centre[10]*delta[2];
    check(facing>0.f && near(centre[8]*delta[1]-centre[9]*delta[0],0.f) &&
          near(centre[9]*delta[2]-centre[10]*delta[1],0.f),
          "annulus local XY faces the actual supported-height camera from its world-space source centre");
    auto movedCamera=source; movedCamera.playerX=-8.f; movedCamera.playerZ=3.f;
    const auto changedView=build(movedCamera);
    check(changedView.attackWarningRingVisible && valid(changedView) &&
          changedView.attackWarningRingWorldFromItem[12]==centre[12] &&
          changedView.attackWarningRingWorldFromItem[13]==centre[13] &&
          changedView.attackWarningRingWorldFromItem[14]==centre[14] &&
          changedView.attackWarningRingWorldFromItem[8]!=centre[8],
          "moving camera changes the facing basis while the ring stays on the enemy instead of following the screen");
    auto walkingCamera=source;
    walkingCamera.walkTime=.23f; walkingCamera.walkAmount=.8f;
    walkingCamera.playerCrouchDropMetres=.31f;
    const auto walkingFrame=build(walkingCamera);
    const auto& walkingRing=walkingFrame.attackWarningRingWorldFromItem;
    const float step=walkingCamera.walkTime*6.2f;
    const std::array<float,3> walkingDelta{
        walkingCamera.playerX+std::sin(step*.5f)*.035f*walkingCamera.walkAmount-centre[12],
        eyeY-walkingCamera.playerCrouchDropMetres+
            std::abs(std::sin(step))*.035f*walkingCamera.walkAmount-centre[13],
        walkingCamera.playerZ-centre[14]};
    check(walkingFrame.attackWarningRingVisible && valid(walkingFrame) &&
          near(walkingRing[8]*walkingDelta[1]-walkingRing[9]*walkingDelta[0],0.f) &&
          near(walkingRing[9]*walkingDelta[2]-walkingRing[10]*walkingDelta[1],0.f) &&
          walkingRing[12]==centre[12] && walkingRing[13]==centre[13] && walkingRing[14]==centre[14],
          "walking bob and crouched camera orient the ring without moving its enemy anchor or phase");
    auto exterior=source; exterior.developmentRescueJourney=true; exterior.rescue.exteriorSide=true;
    const auto exteriorFrame=build(exterior);
    check(exteriorFrame.roster.selectedEnemy==EnemyKind::Lich && exteriorFrame.attackWarningRingVisible &&
          exteriorFrame.attackWarningRingWorldFromItem[12]==centre[12],
          "exterior render-roster override cannot replace the authoritative skeleton warning source");
    auto middle=source;
    setPhase(middle,EnemyAttackKind::GuardBreak,EnemyCombatAction::AttackWindup,
        CombatTimeline::kSkeletonAttackWindupSeconds*.5f);
    auto endpoint=source;
    setPhase(endpoint,EnemyAttackKind::GuardBreak,EnemyCombatAction::AttackWindup,
        CombatTimeline::kSkeletonAttackWindupSeconds);
    auto contact=source;
    setPhase(contact,EnemyAttackKind::GuardBreak,EnemyCombatAction::AttackActive,0.f);
    const auto midFrame=build(middle), endFrame=build(endpoint), contactFrame=build(contact);
    check(axisLength(open,0)>axisLength(midFrame,0) && axisLength(midFrame,0)>axisLength(endFrame,0) &&
          axisLength(endFrame,0)>axisLength(contactFrame,0),
          "yellow contracts through actual windup and closes only when the source changes to Contact");
    auto laterContact=contact;
    setPhase(laterContact,EnemyAttackKind::GuardBreak,EnemyCombatAction::AttackActive,
        CombatTimeline::kSkeletonAttackActiveSeconds);
    check(build(laterContact).attackWarningRingWorldFromItem==contactFrame.attackWarningRingWorldFromItem,
          "contact geometry stays closed without a second active-phase countdown or timer");
    auto recovered=contact;
    setPhase(recovered,EnemyAttackKind::GuardBreak,EnemyCombatAction::AttackRecovery,0.f);
    check(!build(recovered).attackWarningRingVisible && !build(SimulationSnapshot{}).attackWarningRingVisible,
          "actual recovery and reset remove the prior source warning instead of holding a closed ring");
    check(build(source,reduced).attackWarningRingWorldFromItem==build(middle,reduced).attackWarningRingWorldFromItem &&
          build(middle,reduced).attackWarningRingWorldFromItem==build(endpoint,reduced).attackWarningRingWorldFromItem &&
          axisLength(build(contact,reduced),0)<axisLength(build(endpoint,reduced),0),
          "reduced motion is static throughout windup and changes closure only at the authoritative contact phase");
    auto red=source;
    setPhase(red,EnemyAttackKind::Unblockable,EnemyCombatAction::AttackWindup,0.f);
    const auto redStart=build(red);
    setPhase(red,EnemyAttackKind::Unblockable,EnemyCombatAction::AttackWindup,
        CombatTimeline::kSkeletonAttackWindupSeconds/16.f);
    const auto redPulse=build(red);
    check(axisLength(redStart,1)>axisLength(redStart,0)*1.4f && axisLength(redPulse,0)>axisLength(redStart,0) &&
          redStart.attackWarningRingBaseColour!=open.attackWarningRingBaseColour &&
          redStart.attackWarningRingEmission!=open.attackWarningRingEmission,
          "red has an ellipse and a bounded expansion pulse as non-colour distinctions from smooth round yellow");
    const auto reducedRed=build(red,reduced);
    setPhase(red,EnemyAttackKind::Unblockable,EnemyCombatAction::AttackWindup,
        CombatTimeline::kSkeletonAttackWindupSeconds*.75f);
    check(reducedRed.attackWarningRingWorldFromItem==build(red,reduced).attackWarningRingWorldFromItem &&
          axisLength(reducedRed,1)>axisLength(reducedRed,0)*1.4f &&
          near(axisLength(build(source,reduced),0),axisLength(build(source,reduced),1)),
          "reduced motion preserves static red ellipse versus yellow circle without pulsing or countdown");
    for(int failure=0;failure<15;++failure) {
        auto invalid=source;
        if(failure==0)invalid.swordCombat.combatants[0].attackKind=EnemyAttackKind::Normal;
        if(failure==1)invalid.attackWarning.source=EntityId::SkeletonB;
        if(failure==2)invalid.skeletonEnemyCount=0;
        if(failure==3)invalid.skeletonEnemies[0].id=EntityId::Invalid;
        if(failure==4)invalid.skeletonEnemies[0].dead=true;
        if(failure==5)invalid.skeletonEnemies[0].action=EnemyCombatAction::AttackRecovery;
        if(failure==6)invalid.skeletonEnemies[1].id=EntityId::SkeletonA;
        if(failure==7)invalid.attackWarning.phaseElapsedSeconds=std::numeric_limits<float>::quiet_NaN();
        if(failure==8)invalid.skeletonEnemies[0].worldY=std::numeric_limits<float>::quiet_NaN();
        if(failure==9)invalid.playerX=std::numeric_limits<float>::quiet_NaN();
        if(failure==10)invalid.swordCombat.combatants[0].actionTime=.2f;
        if(failure==11)invalid.paused=true;
        if(failure==12)invalid.playerAlive=false;
        if(failure==13)invalid.modernCombatRules=false;
        if(failure==14)invalid.skeletonEnemyCount=invalid.skeletonEnemies.size()+1;
        const auto hidden=build(invalid);
        check(!hidden.attackWarningRingVisible &&
              hidden.attackWarningRingWorldFromItem==items::IdentityHeldItemTransform(),
              "normal stale missing duplicate dead paused or malformed source clears visibility and its old transform");
    }
    auto handed=source; handed.swordCombat.attackerIndex=1; handed.skeletonAttackerId=EntityId::SkeletonB;
    handed.skeletonEnemies[0].dead=true; handed.swordCombat.combatants[0].health=0;
    setPhase(handed,EnemyAttackKind::GuardBreak,EnemyCombatAction::AttackWindup,.2f);
    const auto second=build(handed);
    check(second.attackWarningRingVisible && second.attackWarningRingWorldFromItem[12]==handed.skeletonEnemies[1].x &&
          second.attackWarningRingWorldFromItem[14]==handed.skeletonEnemies[1].z,
          "attacker handoff replaces the previous actor's root and clock with the actual surviving source");
    auto keeper=source; keeper.activeEnemyKind=EnemyKind::Lich;
    keeper.lich.revealComplete=true; keeper.lich.revealPhase=KeeperRevealPhase::Combat;
    keeper.lich.readableCastPresentation=true; keeper.lich.phase=LichPhase::Charging;
    keeper.lich.phaseTime=.3f; keeper.lich.x=-12.f; keeper.lich.y=3.f; keeper.lich.z=-16.f;
    const auto keeperRecord=[](SimulationSnapshot& state) {
        state.attackWarning=BuildAttackWarningPresentation(state.swordCombat,state.lich,
            state.activeEnemyKind,state.skeletonAttackerId,true);
    };
    keeperRecord(keeper);
    const auto keeperFrame=build(keeper);
    check(keeperFrame.attackWarningRingVisible && keeperFrame.attackWarningRingWorldFromItem[12]==keeper.lich.x &&
          keeperFrame.attackWarningRingWorldFromItem[14]==keeper.lich.z &&
          keeperFrame.attackWarningRingWorldFromItem[13]>keeper.lich.y,
          "Keeper ring follows its actual floating root rather than either skeleton or compatibility actor");
    keeper.lich.phase=LichPhase::Recovering; keeper.lich.phaseTime=0.f;
    keeper.lich.staffLightStrength=LichEncounter::kStaffLightPeak; keeperRecord(keeper);
    check(build(keeper).attackWarningRingVisible,"Keeper actual discharge uses its existing Contact window");
    keeper.lich.phaseTime=LichEncounter::kDischargeVisibleBurstDuration; keeperRecord(keeper);
    check(!build(keeper).attackWarningRingVisible,"Keeper recovery clears at the actual discharge endpoint");
    const auto before=source; (void)build(source);
    check(source.attackWarning==before.attackWarning && source.tickIndex==before.tickIndex &&
          source.playerX==before.playerX && source.playerSupportWorldY==before.playerSupportWorldY &&
          source.skeletonEnemies[0].x==before.skeletonEnemies[0].x &&
          source.skeletonEnemies[0].actionTime==before.skeletonEnemies[0].actionTime &&
          source.swordCombat.combatants[0].actionTime==before.swordCombat.combatants[0].actionTime &&
          source.heldItems[0].worldFromItem==before.heldItems[0].worldFromItem &&
          source.fireEmitterCount==before.fireEmitterCount,
          "ring adaptation consumes immutable snapshot inputs without advancing combat equipment or effects");
}
}

int main()
{
    using namespace horde::vulkan::raytracing;
    bool passed = true;
    const auto check = [&passed](bool condition, const char* label) {
        if (!condition) { std::cerr << label << "\n"; passed = false; }
    };
    AttackWarningAdapterContracts(check);
    {
        using namespace horde::gameplay::items;
        horde::gameplay::simulation::SimulationSnapshot source{};source.tickIndex=908;
        auto frame=BuildRtSceneFrameInputs(source,1.f);std::string error;
        check(!frame.playerUtilityHand && !frame.healingFlaskVisible,
              "ordinary frame adaptation leaves flask hand and prop OFF");
        check(!ApplyFlaskUtilityCandidateFrame(source,frame,error) && !frame.playerUtilityHand,
              "flask frame trial requires both explicit simulation gates");
        source.healingFlaskEnabled=source.flaskUtilityCandidate=true;
        UtilityHandReturn hand;check(hand.Begin() && hand.SynchronizeUsing(.4),"real hand owner supplies the trial sample");
        source.flaskHandReturn=hand.Snapshot();const auto before=source.flaskHandReturn;
        check(ApplyFlaskUtilityCandidateFrame(source,frame,error) && frame.playerUtilityHand==source.flaskHandReturn &&
              frame.healingFlaskVisible && source.flaskHandReturn==before,
              "explicit adaptation copies immutable owner time without a renderer clock or prop trajectory");
        horde::gameplay::healing::FlaskUpgradeSite site;
        site.worldFromItem[12]=site.supportPoint[0]=-3.27f;site.worldFromItem[14]=site.supportPoint[2]=22.85f;
        source.flaskUpgrade={site,source.worldRoute.generation,7,true};
        check(ApplyFlaskUtilityCandidateFrame(source,frame,error) && frame.healingFlaskVisible &&
              frame.healingFlaskUpgradeVisible && frame.healingFlaskUpgradeWorldFromItem==site.worldFromItem,
              "world pickup and carried flask coexist with independent immutable frame transforms");
        for(int failure=0;failure<3;++failure) {
            auto bad=source;
            if(failure==0)++bad.flaskUpgrade.worldGeneration;
            if(failure==1)bad.flaskUpgrade.resourceGeneration=0;
            if(failure==2)bad.flaskUpgrade.site->worldFromItem[12]=std::numeric_limits<float>::quiet_NaN();
            check(!ApplyFlaskUtilityCandidateFrame(bad,frame,error) && frame.healingFlaskUpgradeVisible &&
                  frame.healingFlaskUpgradeWorldFromItem==site.worldFromItem && frame.playerUtilityHand==source.flaskHandReturn,
                  "stale or corrupt world receipt cannot partially replace a coherent hand/world frame");
        }
        source.healingFlask.resources.upgraded=true;source.flaskUpgrade={};
        check(ApplyFlaskUtilityCandidateFrame(source,frame,error) && !frame.healingFlaskUpgradeVisible && frame.healingFlaskVisible,
              "upgrade entitlement removes only world pickup and never borrows the in-use hand instance");
        source.healingFlask.resources.upgraded=false;
        auto stale=frame;--stale.tickIndex;
        check(!ApplyFlaskUtilityCandidateFrame(source,stale,error) && stale.playerUtilityHand==frame.playerUtilityHand,
              "stale frame cannot receive a newer hand owner");
        auto invalid=source;invalid.flaskHandReturn.profileSeconds=std::numeric_limits<double>::quiet_NaN();
        check(!ApplyFlaskUtilityCandidateFrame(invalid,frame,error) && frame.playerUtilityHand==source.flaskHandReturn,
              "corrupt hand profile rejects before mutating the prior coherent frame");
        check(hand.Cancel() && hand.Step(.05),"cancelled owner returns from its actual lift sample");
        source.flaskHandReturn=hand.Snapshot();
        check(ApplyFlaskUtilityCandidateFrame(source,frame,error) && frame.playerUtilityHand->profileSeconds<.4 &&
              frame.playerUtilityHand->cancelled && frame.healingFlaskVisible,
              "cancelled return stays simulation-owned in the frame");
        hand.Reset();source.flaskHandReturn=hand.Snapshot();
        check(ApplyFlaskUtilityCandidateFrame(source,frame,error) && !frame.playerUtilityHand && !frame.healingFlaskVisible,
              "reset/Idle clears the trial hand and prop while preserving ordinary rendering");
    }
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
    check(!frame.attackWarningRingVisible,"ordinary graphics preview has no opted-in attack warning");
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
    check(!entryFrame.attackWarningRingVisible,"ordinary entry menu has no opted-in attack warning");
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
