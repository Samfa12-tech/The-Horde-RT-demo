#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/ShowcaseCheckpoints.h"
#include "gameplay/CorridorCollision.h"
#include "gameplay/SpatialAudio.h"
#include <cmath>
#include <iostream>
#include <vector>

using namespace horde::gameplay;
using namespace horde::gameplay::simulation;
namespace {
int failures=0, checks=0;
void Check(bool value,const char* message) {
    ++checks;
    if(!value) { ++failures; std::cerr<<"FAIL: "<<message<<'\n'; }
}
bool Near(float a,float b,float epsilon=.0001f) { return std::abs(a-b)<=epsilon; }
GameSimulationConfig Rules() {
    auto c=ProductionGameSimulationConfig(); c.swordStartsStowed=false; return c;
}
void WindowAndLifecycle() {
    for (int tick=0;tick<=13;++tick)
        Check(DodgeProtectionWindow::Contains(tick/60.0f,true)==(tick>=2&&tick<10),
              "half-open protection boundaries match the documented gameplay ticks");
    Check(!DodgeProtectionWindow::Contains(.1f,false)&&
          !DodgeProtectionWindow::Contains(NAN,true),"rejected/nonfinite dodge cannot protect");
    GameSimulation sim(Rules()); InputSnapshot input; input.tutorialEnabled=false;
    input.commands.dodge=1; input.moveStrafe=1;
    for(int tick=1;tick<=13;++tick) {
        if(tick==4) input.commands.dodge=20; // spam cannot restart duration
        sim.StepFixed(input);
        Check(sim.Snapshot().dodgeInvulnerable==(tick>=2&&tick<10),
              "accepted dodge publishes protection once; spam never extends it");
        Check(sim.Snapshot().acceptedDodgeSequence==1,"spam does not replace accepted identity");
    }
    Check(Near(sim.Snapshot().playerX,.90f),"protection does not multiply existing displacement");
    input.paused=true; sim.AdvanceFrame(input,10.0);
    Check(!sim.Snapshot().dodgeInvulnerable&&!sim.Snapshot().dodgeActive,
          "pause cancels transient protection without advancing it");
    input.paused=false; input.commands={}; sim.ResetRoute(); input.commands.dodge=21;
    sim.StepFixed(input); sim.StepFixed(input);
    Check(sim.Snapshot().dodgeInvulnerable,"fresh post-reset dodge can protect");
    sim.SynchronizePausedInput(input,2,PausedInputPolicy::DiscardAllCommands);
    Check(!sim.Snapshot().dodgeInvulnerable&&sim.Events().Empty(),"focus/lifecycle cancels protection and feedback");
    sim.RetryEncounter(); Check(!sim.Snapshot().dodgeInvulnerable,"retry cannot retain immunity");
    sim.ApplyShowcaseCheckpoint(0); input.commands.dodge=22;
    sim.StepFixed(input); sim.StepFixed(input);
    Check(!sim.Snapshot().dodgeInvulnerable,"historical capture imports preserve their legacy rule identity");
}
void TeachingContract() {
    CombatTeaching teaching; CombatSnapshot combat; LichSnapshot keeper;
    combat.attackerIndex=0;
    combat.combatants[0].action=EnemyCombatAction::AttackWindup;
    combat.combatants[0].actionTime=.4f;
    teaching.Reset(true); teaching.Update(true,true,false,combat,keeper,EnemyKind::Skeleton);
    Check(teaching.Snapshot().cue==CombatTeachingCue::ParryWindup&&
          !teaching.Snapshot().slowdownActive,"wind-up is distinct from anticipatory parry input cue");
    combat.combatants[0].actionTime=.90f;
    teaching.Update(true,true,false,combat,keeper,EnemyKind::Skeleton);
    Check(teaching.Snapshot().cue==CombatTeachingCue::ParryNow&&
          Near(teaching.Snapshot().simulationTimeScale,.45f),"optional slowdown is narrowly tied to an unlearned window");
    teaching.ParrySucceeded(); teaching.Update(true,true,false,combat,keeper,EnemyKind::Skeleton);
    Check(!teaching.Snapshot().slowdownActive&&teaching.Snapshot().promptOpacity==0,
          "understood parry stops teaching prompts and dilation");
    keeper.phase=LichPhase::Charging; keeper.phaseTime=1.10f;
    teaching.Update(true,true,false,combat,keeper,EnemyKind::Lich);
    Check(teaching.Snapshot().cue==CombatTeachingCue::DodgeNow&&teaching.Snapshot().safePractice,
          "Keeper teaches a distinct short dodge interval without progression");
    teaching.DodgeSucceeded(); teaching.Update(true,true,false,combat,keeper,EnemyKind::Lich);
    Check(teaching.Snapshot().stage==TutorialStage::Complete&&!teaching.Snapshot().safePractice&&
          teaching.Snapshot().promptOpacity==0,"completed lesson cannot keep immunity or permanent prompts");
    teaching.Reset(true); teaching.Skip(); teaching.Update(true,true,false,combat,keeper,EnemyKind::Lich);
    Check(!teaching.Snapshot().safePractice&&!teaching.Snapshot().slowdownActive,"skip restores ordinary gameplay clocks/damage");
    GameSimulation sim(Rules()); InputSnapshot input; sim.BeginCombatPractice(EnemyKind::Lich);
    for(int i=0;i<900;++i) { sim.StepFixed(input); sim.ClearEvents(); }
    Check(sim.Snapshot().combatTeaching.practiceMisses>0&&sim.Snapshot().playerVitals.vitality==3&&
          sim.Snapshot().playerVitals.invulnerabilityRemaining==0,
          "real Keeper practice misses are harmless and do not refresh post-hit protection");
    input.commands.tutorialSkip=1; sim.StepFixed(input);
    Check(sim.Snapshot().combatTeaching.stage==TutorialStage::Skipped,"monotonic skip is owned by simulation");
    for(int i=0;i<900&&sim.Snapshot().playerVitals.phase==PlayerLifePhase::Alive;++i) {
        sim.StepFixed(input); sim.ClearEvents();
    }
    Check(sim.Snapshot().playerVitals.phase!=PlayerLifePhase::Alive,"skip removes safe-practice damage suppression");
    input.commands.tutorialReplay=1; sim.StepFixed(input);
    Check(sim.Snapshot().playerVitals.vitality==3&&sim.Snapshot().combatTeaching.stage==TutorialStage::Parry,
          "replay explicitly resets opening and restores teachable state");
}
void KeeperContract() {
    LichEncounter keeper; keeper.SetReadableCombat(true); keeper.BeginRetryRecognition();
    for(int i=0;i<60;++i) keeper.Update(1.0f/60,kKeeperRetryPosition.x,kKeeperRetryPosition.z,true,true);
    Check(keeper.Snapshot().revealComplete,"live Keeper recognition finishes without frozen import");
    auto target=keeper.Snapshot();
    Check(keeper.TryAcceptPlayerHit(target.x,target.z)&&keeper.Snapshot().health==2&&
          keeper.Snapshot().phase==LichPhase::Repelling&&!keeper.Snapshot().damagePulse,
          "first accepted hit starts knockback-only repel");
    Check(!keeper.TryAcceptPlayerHit(target.x,target.z),"existing two-second accepted-hit lockout remains exact");
    int warning=0,pulses=0; bool warningBefore=false;
    for(int i=0;i<130;++i) {
        keeper.Update(1.0f/60,target.x+4,target.z,true,true);
        if(keeper.Snapshot().dischargeWarningPulse) { ++warning; warningBefore=pulses==0; }
        if(keeper.Snapshot().damagePulse) ++pulses;
    }
    Check(warning==1&&pulses==1&&warningBefore,"early cast gets one distinct warning before discharge");
    target=keeper.Snapshot(); Check(keeper.TryAcceptPlayerHit(target.x,target.z)&&keeper.Snapshot().health==1,
          "second accepted hit preserves three-hit defeat");
    for(int i=0;i<121;++i) keeper.Update(1.0f/60,target.x+4,target.z,true,true);
    target=keeper.Snapshot(); Check(keeper.TryAcceptPlayerHit(target.x,target.z)&&keeper.Snapshot().phase==LichPhase::Dead&&
          keeper.Snapshot().health==0&&!keeper.Snapshot().damagePulse&&!keeper.Snapshot().dischargeWarningPulse,
          "lethal third hit cancels pending repel, charge and warning");
    for(int i=0;i<180;++i) {
        keeper.Update(1.0f/60,target.x+4,target.z,true,true);
        Check(!keeper.Snapshot().damagePulse&&!keeper.Snapshot().dischargeWarningPulse,"dead Keeper never replays queued damage");
    }
}
void ActualIncomingHits() {
    for(int ticksBefore:{1,2,5,9,10,12}) {
        GameSimulation sim(Rules()); InputSnapshot input; input.tutorialEnabled=false;
        input.yawRadians=-1.57079632679f; // Side-step along the safe finale Z axis, not out of the encounter.
        sim.BeginCombatPractice(EnemyKind::Lich);
        int ticks=0;
        const float trigger=LichEncounter::kChargeDuration-ticksBefore/60.0f;
        while(ticks++<600 && !(sim.Snapshot().lich.phase==LichPhase::Charging &&
                              sim.Snapshot().lich.phaseTime+.00001f>=trigger)) {
            sim.StepFixed(input); sim.ClearEvents();
        }
        Check(ticks<600,"real Keeper reaches the scheduled dodge boundary");
        input.moveStrafe=1; input.commands.dodge=1;
        bool hit=false;
        for(int i=0;i<30;++i) {
            sim.StepFixed(input);
            if(sim.Snapshot().lich.damagePulse) {
                hit=true;
                const auto& state=sim.Snapshot();
                const bool protectedNow=state.dodgeInvulnerable;
                Check((state.playerVitals.vitality==3)==protectedNow,
                      "actual lightning resolves with the same half-open dodge policy");
                Check(protectedNow ? state.playerVitals.invulnerabilityRemaining==0 :
                                    Near(state.playerVitals.invulnerabilityRemaining,1),
                      "protected strike cannot refresh the separate post-hit guard");
                Check(state.dodgeProtectedHitCount==(protectedNow?1:0),
                      "real damage pulse records a unique protection outcome");
                const auto& trace=state.combatContactTrace;
                const auto& sample=trace.samples[(trace.nextIndex+trace.kCapacity-1)%trace.kCapacity];
                Check(state.dodgeActive ? sample.commandSequence==1&&sample.consumedTick!=0 :
                                         sample.commandSequence==0&&sample.consumedTick==0,
                      "incoming hit trace joins only an active dodge; expired input never acquires a later hit");
                break;
            }
            sim.ClearEvents();
        }
        if(!hit) std::cerr<<"lightning missed fixture ticksBefore="<<ticksBefore
            <<" phase="<<int(sim.Snapshot().lich.phase)<<" phaseTime="<<sim.Snapshot().lich.phaseTime
            <<" player="<<sim.Snapshot().playerX<<','<<sim.Snapshot().playerZ
            <<" keeper="<<sim.Snapshot().lich.x<<','<<sim.Snapshot().lich.z<<'\n';
        Check(hit,"fixture tests an actual line-of-sight lightning hit, not an assigned field");
    }
    // Real melee shares the common resolution point. Moving directly towards
    // the approaching attacker stays in its bounded contact radius.
    for(int ticksBefore:{1,2,5,10}) {
        auto config=Rules(); config.waterfallSkeletonEncounter=false;
        config.playerStartX=0; config.playerStartZ=-3.75f;
        GameSimulation sim(config); InputSnapshot input; input.tutorialEnabled=false;
        const float trigger=CombatTimeline::kSkeletonAttackWindupSeconds-ticksBefore/60.0f;
        int ticks=0;
        while(ticks++<600 && !(sim.Snapshot().swordCombat.attackerIndex>=0 &&
            sim.Snapshot().swordCombat.combatants[sim.Snapshot().swordCombat.attackerIndex].action==EnemyCombatAction::AttackWindup &&
            sim.Snapshot().swordCombat.combatants[sim.Snapshot().swordCombat.attackerIndex].actionTime+.00001f>=trigger)) {
            sim.StepFixed(input); sim.ClearEvents();
        }
        Check(ticks<600,"real melee attacker reaches its scheduled boundary");
        input.commands.dodge=1; input.moveForward=1;
        bool hit=false;
        for(int i=0;i<30;++i) {
            sim.StepFixed(input);
            if(sim.Snapshot().swordCombat.playerHitPulse) {
                hit=true;
                Check((sim.Snapshot().playerVitals.vitality==3)==sim.Snapshot().dodgeInvulnerable,
                      "actual skeleton melee uses the same dodge resolution as lightning");
                const auto& trace=sim.Snapshot().combatContactTrace;
                const auto& sample=trace.samples[(trace.nextIndex+trace.kCapacity-1)%trace.kCapacity];
                const auto index=sample.target==EntityId::SkeletonA?0u:1u;
                Check((sample.target==EntityId::SkeletonA||sample.target==EntityId::SkeletonB)&&
                      Near(sample.targetX,sim.Snapshot().swordCombat.combatants[index].x)&&
                      Near(sample.targetZ,sim.Snapshot().swordCombat.combatants[index].z),
                      "incoming contact trace freezes the actual attacker position rather than player impact coordinates");
                break;
            }
            sim.ClearEvents();
        }
        Check(hit,"melee boundary fixture retains actual incoming contact coverage");
    }
}
void DeliveryRates() {
    std::vector<std::vector<double>> deliveries;
    for(int fps:{15,30,60,120}) deliveries.push_back(std::vector<double>(fps,1.0/fps));
    deliveries.push_back({.008,.024,.017,.033,.010,.025,.013,.020});
    for(double stall:{.05,.10,.25}) deliveries.push_back({1.0/60,stall,1.0/120,1.0/30,.02,.04});
    for(const auto& frames:deliveries) {
        auto config=Rules(); config.waterfallSkeletonEncounter=false;
        config.playerStartX=0; config.playerStartZ=-3.75f;
        GameSimulation sim(config),reference(config); InputSnapshot input; input.tutorialEnabled=false;
        input.commands.dodge=1; input.commands.attack=2; input.commands.parry=1; input.moveStrafe=1;
        using EventIdentity=std::array<std::uint64_t,5>;
        std::vector<EventIdentity> deliveredEvents,referenceEvents;
        const auto drain=[](GameSimulation& owner,std::vector<EventIdentity>& output) {
            for(const auto& event:owner.Events().Events()) output.push_back({event.sequence,event.tickIndex,
                static_cast<std::uint64_t>(event.type),static_cast<std::uint64_t>(event.source),
                static_cast<std::uint64_t>(event.target)});
            owner.ClearEvents();
        };
        bool finite=true;
        for(double delta:frames) {
            const auto ticks=sim.AdvanceFrame(input,delta);
            drain(sim,deliveredEvents);
            for(std::uint32_t tick=0;tick<ticks;++tick) { reference.StepFixed(input); drain(reference,referenceEvents); }
            const auto& snap=sim.Snapshot();
            finite &= std::isfinite(snap.playerX)&&std::isfinite(snap.dodgeElapsedSeconds)&&
                !snap.dodgeInvulnerable == !DodgeProtectionWindow::Contains(snap.dodgeElapsedSeconds,snap.dodgeActive);
        }
        Check(finite,"rate/jitter/stall catch-up retains finite coherent protection publication");
        const auto& actual=sim.Snapshot(); const auto& expected=reference.Snapshot();
        Check(actual.tickIndex==expected.tickIndex&&actual.playerCombat.action==expected.playerCombat.action&&
              Near(actual.playerCombat.actionTime,expected.playerCombat.actionTime)&&
              Near(actual.playerX,expected.playerX)&&Near(actual.playerZ,expected.playerZ)&&
              actual.lastConsumedAttackSequence==expected.lastConsumedAttackSequence&&
              actual.lastConsumedParrySequence==expected.lastConsumedParrySequence&&
              actual.lastConsumedDodgeSequence==expected.lastConsumedDodgeSequence&&
              actual.swordCombat.combatants[0].health==expected.swordCombat.combatants[0].health&&
              actual.swordCombat.combatants[0].action==expected.swordCombat.combatants[0].action&&
              Near(actual.swordCombat.combatants[0].actionTime,expected.swordCombat.combatants[0].actionTime)&&
              actual.playerVitals.vitality==expected.playerVitals.vitality&&deliveredEvents==referenceEvents,
              "15/30/60/120, jitter and bounded stalls preserve actual combat/contact/event ordering against fixed-step replay");
        input.paused=true; sim.AdvanceFrame(input,.25);
        Check(!sim.Snapshot().dodgeInvulnerable,"stall+pause cannot replay transient immunity");
    }
}
void CoherentSlowdown() {
    auto config=Rules(); config.waterfallSkeletonEncounter=false;
    config.playerStartX=0; config.playerStartZ=-3.75f;
    GameSimulation sim(config); InputSnapshot input; input.tutorialSlowdownEnabled=true;
    int ticks=0;
    while(ticks++<600 && sim.Snapshot().combatTeaching.cue!=CombatTeachingCue::ParryNow) {
        sim.StepFixed(input); sim.ClearEvents();
    }
    Check(ticks<600,"real opening attack enters optional teaching slowdown");
    const auto before=sim.Snapshot();
    input.commands.attack=1; input.commands.parry=1; input.commands.dodge=1;
    input.moveStrafe=1;
    sim.StepFixed(input);
    const auto after=sim.Snapshot();
    Check(after.tickIndex==before.tickIndex+1 && Near(after.gameplayTimeScale,.45f),
          "input owner still consumes one real fixed tick while gameplay time is eased");
    Check(Near(after.walkTime-before.walkTime,.45f/60) && Near(after.dodgeElapsedSeconds,.45f/60),
          "movement and dodge protection use the same scaled gameplay delta");
    Check(after.playerCombat.action==PlayerCombatAction::SwingWindup &&
          Near(after.playerCombat.actionTime,.45f/60) && after.lastConsumedAttackSequence==1 &&
          after.lastConsumedParrySequence==1 && after.lastConsumedDodgeSequence==1,
          "attack wins simultaneous parry; all input edges remain responsive during slowdown");
    const auto attacker=before.swordCombat.attackerIndex;
    Check(attacker>=0 && Near(after.swordCombat.combatants[attacker].actionTime-
          before.swordCombat.combatants[attacker].actionTime,.45f/60),
          "enemy contact clock cannot run ahead of the slowed player pose and defense");
    input.commands.tutorialSkip=1;
    sim.StepFixed(input);
    Check(sim.Snapshot().combatTeaching.stage==TutorialStage::Skipped &&
          !sim.Snapshot().combatTeaching.slowdownActive,"skip exits teaching slowdown through the owner command path");
    const auto skipped=sim.Snapshot(); sim.StepFixed(input);
    Check(Near(sim.Snapshot().walkTime-skipped.walkTime,1.0f/60) &&
          sim.Snapshot().gameplayTimeScale==1,"ordinary clock resumes without double stepping");
    input.paused=true; const auto pauseTick=sim.Snapshot().tickIndex;
    sim.AdvanceFrame(input,.25);
    Check(sim.Snapshot().tickIndex==pauseTick&&!sim.Snapshot().dodgeInvulnerable&&
          sim.Snapshot().combatContactTrace.count==0,"pause clears transient defense and contact trace without catch-up");
}
void LiveParryAndRiposte() {
    auto config=Rules(); config.waterfallSkeletonEncounter=false;
    config.playerStartX=0; config.playerStartZ=-3.75f;
    GameSimulation sim(config); InputSnapshot input;
    int ticks=0;
    while(ticks++<600 && !(sim.Snapshot().swordCombat.attackerIndex>=0 &&
        sim.Snapshot().swordCombat.combatants[sim.Snapshot().swordCombat.attackerIndex].action==EnemyCombatAction::AttackWindup &&
        sim.Snapshot().swordCombat.combatants[sim.Snapshot().swordCombat.attackerIndex].actionTime>=
            CombatTimeline::kSkeletonAttackWindupSeconds-.10f)) { sim.StepFixed(input); sim.ClearEvents(); }
    Check(ticks<600,"live lesson reaches a real attack tell before parry input");
    input.commands.parry=1; bool succeeded=false;
    for(int tick=0;tick<20;++tick) {
        sim.StepFixed(input);
        for(const auto& event:sim.Events().Events()) if(event.type==GameplayEventType::PlayerParrySucceeded) succeeded=true;
        sim.ClearEvents(); if(succeeded) break;
    }
    Check(succeeded&&sim.Snapshot().combatTeaching.parryLearned&&
          sim.Snapshot().playerVitals.vitality==3,
          "actual successful parry teaches without health loss and retains its semantic result");
    input.commands.attack=1; sim.StepFixed(input);
    Check(sim.Snapshot().playerCombat.action==PlayerCombatAction::SwingWindup&&
          sim.Snapshot().combatPresentation.parrySuccessActive,
          "modern lesson preserves immediate next-tick riposte with sustained success presentation");
}
void IntegratedKeeperRepelAndReward() {
    for(float side:{-1.0f,1.0f}) {
        GameSimulation sim(Rules()); InputSnapshot input;
        sim.BeginCombatPractice(EnemyKind::Lich);
        for(int tick=0;tick<60;++tick) { sim.StepFixed(input); sim.ClearEvents(); }
        input.hasAuthoritativePlayerPose=true;
        input.authoritativePlayerX=sim.Snapshot().lich.x+side*.95f;
        input.authoritativePlayerZ=sim.Snapshot().lich.z;
        input.yawRadians=side>0 ? -1.57079632679f : 1.57079632679f;
        input.commands.attack=1;
        int tick=0;
        while(tick++<60 && sim.Snapshot().lich.health==3) { sim.StepFixed(input); sim.ClearEvents(); }
        Check(sim.Snapshot().lich.health==2 && sim.Snapshot().keeperRepelRemainingSeconds>0,
              "real accepted nonfatal Keeper contact schedules owner-resolved repel");
        const float initialX=sim.Snapshot().playerX, initialZ=sim.Snapshot().playerZ;
        input.hasAuthoritativePlayerPose=false;
        bool safe=true;
        for(int i=0;i<24;++i) {
            sim.StepFixed(input); sim.ClearEvents();
            const auto& s=sim.Snapshot();
            safe &= IsShowcasePlayerPositionWalkable(s.playerX,s.playerZ) && s.playerGrounded &&
                Near(s.playerSupportWorldY,kRouteFloorWorldY) && !s.lich.damagePulse;
        }
        const auto& repelled=sim.Snapshot();
        Check(safe && repelled.keeperRepelRemainingSeconds==0 &&
              repelled.keeperRepelTravelledMetres<=LichEncounter::kRepelDistance+.0001f &&
              std::hypot(repelled.playerX-initialX,repelled.playerZ-initialZ)<=1.001f,
              "integrated repel is bounded, collision-safe and carries no damage on either side");
        input.commands.dodge=1; input.moveStrafe=1;
        sim.StepFixed(input);
        Check(sim.Snapshot().dodgeActive,"defensive input is still available after the repel");
        sim.RetryEncounter();
        Check(sim.Snapshot().lich.health==3 && sim.Snapshot().keeperRepelRemainingSeconds==0 &&
              !sim.Snapshot().dodgeInvulnerable,"retry discards both repel and defense transients");
    }
    GameSimulation sim(Rules()); InputSnapshot input; sim.BeginCombatPractice(EnemyKind::Lich);
    for(int tick=0;tick<60;++tick) { sim.StepFixed(input); sim.ClearEvents(); }
    std::uint64_t defeatedSequence=0, unlockSequence=0;
    int hits=0, defeated=0, unlocked=0;
    auto collect=[&] {
        for(const auto& event:sim.Events().Events()) {
            if(event.type==GameplayEventType::EnemyHit && event.target==EntityId::Lich) ++hits;
            if(event.type==GameplayEventType::LichDefeated && event.target==EntityId::Lich) {
                ++defeated; defeatedSequence=event.sequence;
            }
            if(event.type==GameplayEventType::ChestUnlocked) { ++unlocked; unlockSequence=event.sequence; }
        }
        sim.ClearEvents();
    };
    input.hasAuthoritativePlayerPose=true;
    for(int hit=1;hit<=3;++hit) {
        input.authoritativePlayerX=sim.Snapshot().lich.x+.95f;
        input.authoritativePlayerZ=sim.Snapshot().lich.z;
        input.yawRadians=-1.57079632679f;
        input.commands.attack=hit;
        for(int i=0;i<150;++i) { sim.StepFixed(input); collect(); }
    }
    for(int i=0;i<240;++i) { sim.StepFixed(input); collect(); }
    Check(hits==3 && defeated==1 && unlocked==1 && unlockSequence>defeatedSequence &&
          sim.Snapshot().lich.health==0 && sim.Snapshot().keeperRepelRemainingSeconds==0,
          "live three-hit defeat cancels future repel/discharge and preserves ordered reward admission");
    Check(!sim.Snapshot().lich.damagePulse && sim.Snapshot().torchFailure.phase==TorchFailurePhase::Settled,
          "dead Keeper never restores attack or the failed held torch");
}
}
void OutgoingTraceContract() {
    GameSimulation sim(Rules()); InputSnapshot input; input.damageEnabled=false;
    sim.BeginCombatPractice(EnemyKind::Skeleton);
    input.commands.attack=1;
    bool sawDown=false,sawUp=false; std::uint64_t downId=0,upId=0;
    for(int tick=1;tick<=44;++tick) {
        if(tick==2) input.commands.attack=2;
        sim.StepFixed(input);
        const auto& state=sim.Snapshot(); const auto& trace=state.combatContactTrace;
        if(trace.count==0) continue;
        const auto& sample=trace.samples[(trace.nextIndex+trace.kCapacity-1)%trace.kCapacity];
        if(sample.tick!=state.tickIndex || sample.cut==PlayerAttackCut::None) continue;
        Check(sample.hasSwordTransform && sample.worldFromSword==state.heldItems[1].worldFromItem,
              "contact trace samples the actual published sword attachment on its owning fixed tick");
        Check(!sample.hasBlade&&!sample.hasSeparation,
              "forgiving contact policy cannot fabricate physical blade/triangle measurements");
        if(sample.cut==PlayerAttackCut::DownwardCut) {
            sawDown=true; downId=sample.attackId;
            Check(sample.commandSequence==1&&sample.consumedTick==1,
                  "buffered continuation must not relabel the downward cut's source edge");
        } else {
            sawUp=true; upId=sample.attackId;
            Check(sample.commandSequence==2&&sample.consumedTick==2,
                  "upward contact samples retain the actual buffered input edge and consumed tick");
        }
    }
    Check(sawDown&&sawUp&&downId!=0&&upId!=downId,
          "each cut has a distinct identity across a buffered continuation");
    sim.SynchronizePausedInput(input,2,PausedInputPolicy::DiscardAllCommands);
    Check(sim.Snapshot().combatContactTrace.count==0,
          "lifecycle cancellation cannot replay old contact observations");
}
void RouteMeleeOcclusion() {
    auto config=Rules(); config.waterfallSkeletonEncounter=false;
    config.playerStartX=.40f; config.playerStartZ=-6.20f;
    config.playerStartYawRadians=0.0f;
    GameSimulation clearSide(config); InputSnapshot input;
    input.damageEnabled=false; input.tutorialEnabled=false;
    input.yawRadians=3.14159265359f;
    const auto clearBefore=clearSide.Snapshot();
    Check(IsShowcasePlayerPositionWalkable(clearBefore.playerX,clearBefore.playerZ) &&
          clearBefore.playerGrounded && Near(clearBefore.playerSupportWorldY,kRouteFloorWorldY) &&
          IsSkeletonEnemyPositionWalkable(clearBefore.swordCombat.combatants[1].x,
                                          clearBefore.swordCombat.combatants[1].z),
          "clear-side fixture begins on the supported route with a legal skeleton pose");
    Check(!IsRouteAudioObstructed(clearBefore.playerX,clearBefore.playerZ,
                                  clearBefore.swordCombat.combatants[1].x,
                                  clearBefore.swordCombat.combatants[1].z),
          "clear-side route melee fixture has an open segment through the arch aperture");
    input.commands.attack=1;
    for(int tick=0;tick<40;++tick) clearSide.StepFixed(input);
    Check(clearSide.Snapshot().swordCombat.combatants[1].health==
              clearBefore.swordCombat.combatants[1].health-1,
          "actual forgiving combat consumer still accepts a clear-side route hit");

    // This is a seeded regression through the real forgiving SwordCombat
    // consumer at route coordinates. Production route reachability for these
    // poses is not established by this fixture. The geometry oracle proves
    // that the segment crosses masonry; it does not supply combat hit authority.
    constexpr float playerZ=-6.80f, yawNorth=3.14159265359f;
    const auto probe=[&](const char* caseName,const float playerX,
                        const float targetX,const float targetZ) {
        Check(IsShowcasePlayerPositionWalkable(playerX,playerZ),
              "occlusion probes start at a legal player position south of the arch return");
        const bool obstructed=IsRouteAudioObstructed(playerX,playerZ,targetX,targetZ);
        Check(IsSkeletonEnemyPositionWalkable(targetX,targetZ),
              "occlusion probe target starts at a legal skeleton route position");
        Check(obstructed,"occlusion probe segment crosses authored +X return masonry");
        const std::array<SkeletonSpawnPose,kSkeletonEnemyCapacity> spawns{{
            {{targetX,targetZ},yawNorth,0.0f},
            {{-1.0f,-4.65f},0.0f,0.0f},
        }};
        SwordCombat combat;
        combat.Reset(kSkeletonEnemyCapacity,{0.0f,-4.65f},&spawns,2);
        combat.RequestAttack();
        bool hit=false, actualTargetLineObstructed=false;
        bool actualPlayerPoseLegal=false, actualTargetPoseLegal=false;
        int contactTick=0;
        float actualTargetX=targetX, actualTargetZ=targetZ;
        for(int tick=1;tick<=40;++tick) {
            const auto& state=combat.Update(1.0f/60.0f,playerX,playerZ,yawNorth,
                                            true,true,false);
            if(state.combatants[0].health==1) {
                hit=true; contactTick=tick;
                actualTargetX=state.combatants[0].x;
                actualTargetZ=state.combatants[0].z;
                actualPlayerPoseLegal=IsShowcasePlayerPositionWalkable(playerX,playerZ);
                actualTargetPoseLegal=IsSkeletonEnemyPositionWalkable(actualTargetX,actualTargetZ);
                actualTargetLineObstructed=IsRouteAudioObstructed(
                    playerX,playerZ,actualTargetX,actualTargetZ);
                break;
            }
        }
        if(hit && actualTargetLineObstructed) {
            std::cerr<<"KNOWN DEFECT: forgiving sword consumer accepted "<<caseName
                     <<" contact at attackTick="<<contactTick<<" player=("<<playerX<<','
                     <<playerZ<<") actualTarget=("<<actualTargetX<<','<<actualTargetZ
                     <<") distance="<<std::hypot(actualTargetX-playerX,
                                                   actualTargetZ-playerZ)<<'\n';
        }
        Check(obstructed && hit && actualTargetLineObstructed &&
              actualPlayerPoseLegal && actualTargetPoseLegal,
              "diagnostic records an accepted hit through route masonry at legal actual poses");
    };
    probe("straight-wall",.96f,.96f,-5.95f);
    probe("oblique-wall",.96f,1.55f,-6.06f);
    // At the south face z=-6.50, this segment crosses x=0.955405:
    // 55 mm inside the west/south corner of the authored return (x=0.90).
    probe("corner-edge",.55f,1.55f,-6.06f);
}
int main() {
    WindowAndLifecycle(); TeachingContract(); KeeperContract(); ActualIncomingHits(); DeliveryRates(); CoherentSlowdown();
    IntegratedKeeperRepelAndReward();
    OutgoingTraceContract();
    RouteMeleeOcclusion();
    LiveParryAndRiposte();
    std::cout<<"combat foundation checks="<<checks<<" failures="<<failures<<'\n';
    return failures?1:0;
}
