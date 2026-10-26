#include "gameplay/effects/KeeperCastPresentation.h"
#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/simulation/AttackWarningPresentation.h"
#include "gameplay/simulation/SkeletonRevivalBridge.h"
#include "gameplay/ShowcaseCheckpoints.h"
#include "gameplay/CorridorCollision.h"
#include "gameplay/SpatialAudio.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
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
bool LearnBlockFromRealContact(GameSimulation& sim,InputSnapshot& input) {
    input.tutorialEnabled=true; input.guardHeld=true; ++input.commands.parry;
    bool learned=false;
    for(int tick=0;tick<600 && !learned;++tick) {
        sim.StepFixed(input);
        const auto& state=sim.Snapshot(); const auto& trace=state.combatContactTrace;
        learned=state.combatTeaching.blockLearned && state.swordCombat.playerHitPulse && trace.count>0 &&
            trace.samples[(trace.nextIndex+trace.kCapacity-1)%trace.kCapacity].outcome==CombatContactOutcome::GuardBlocked;
        sim.ClearEvents();
    }
    input.guardHeld=false; sim.StepFixed(input); sim.ClearEvents();
    return learned;
}
void SkeletonRevivalBridgeContract() {
    using Phase=SkeletonRevivalPhase;
    SkeletonRevivalBridge bridge; SkeletonRevivalInput input;
    input.playerAlive=true; input.bothDead=true; input.bothDeathAnimationsComplete=true; input.presentationReady=true;
    input.actorClearance=true; input.riseDurationSeconds=.20f;
    bridge.Step(.05f,input);
    Check(!bridge.Snapshot().enabled && bridge.Snapshot().phase==Phase::Disabled && !bridge.Snapshot().consumed,
          "revival controller defaults disabled even when supplied ready/dead fixture values");
    bridge.Reset(true); const auto firstGeneration=bridge.Snapshot().generation;
    Check(!bridge.AcknowledgeLivePair(firstGeneration,true),
          "a claimed live pair cannot arm final completion during unconsumed practice");
    for(int tick=0;tick<10;++tick) bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::Practice && !bridge.Snapshot().consumed &&
          !bridge.Snapshot().signalPulse && !bridge.Snapshot().revivePulse,
          "premature deaths cannot fake finished practice or automatically revive the pair");
    input.practiceFinished=true; input.paused=true; bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::Practice,"paused lesson completion cannot advance bridge state");
    input.paused=false; input.playerAlive=false; bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::Practice,"dead player cannot consume a bridge through lesson input");
    input.playerAlive=true; input.presentationReady=false; bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::FirstFight && !bridge.Snapshot().consumed,
          "real practice exit is logical even while an unadmitted presentation cannot signal");
    input.presentationReady=true;
    for(float duration:{0.f,-.1f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
        input.riseDurationSeconds=duration; bridge.Step(.05f,input);
        Check(bridge.Snapshot().phase==Phase::FirstFight && !bridge.Snapshot().consumed && !bridge.Snapshot().signalPulse,
              "readiness alone cannot authorize an invalid or absent reviewed rise duration");
    }
    input.riseDurationSeconds=.20f; input.bothDeathAnimationsComplete=false;
    bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::FirstFight && !bridge.Snapshot().consumed && !bridge.Snapshot().signalPulse,
          "zero-health guards still collapsing cannot consume the signal or start the stand pose");
    input.bothDeathAnimationsComplete=true; bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::Signal && bridge.Snapshot().consumed && bridge.Snapshot().signalPulse &&
          bridge.Snapshot().phaseSeconds==0.f && bridge.Snapshot().SuppressesActors(),
          "the bridge consumes once before publishing its signal and suppressed presentation phase");
    Check(!bridge.AcknowledgeLivePair(firstGeneration,true) && bridge.Snapshot().signalPulse,
          "live acknowledgement refuses a consumed non-Live phase without clearing its signal pulse");
    bridge.CancelTransient();
    Check(bridge.Snapshot().consumed && bridge.Snapshot().phase==Phase::Signal && !bridge.Snapshot().signalPulse &&
          bridge.Snapshot().generation==firstGeneration,
          "transient cancellation clears pulses without rearming consumption or changing phase identity");
    for(float delta:{0.f,-.1f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
        bridge.Step(delta,input);
        Check(bridge.Snapshot().phase==Phase::Signal && bridge.Snapshot().phaseSeconds==0.f &&
              bridge.Snapshot().consumed && !bridge.Snapshot().signalPulse && !bridge.Snapshot().revivePulse,
              "nonpositive or nonfinite delta cannot advance clocks or replay bridge pulses");
    }
    input.presentationReady=false; bridge.Step(.05f,input);
    Check(bridge.Snapshot().phaseSeconds==0.f && bridge.Snapshot().consumed,
          "readiness loss freezes an already consumed signal rather than silently replaying it");
    input.presentationReady=true; bridge.Step(100.f,input);
    Check(Near(bridge.Snapshot().phaseSeconds,.05f) && bridge.Snapshot().phase==Phase::Signal,
          "a long positive frame advances at most one bounded gameplay step");
    for(int tick=0;tick<12 && bridge.Snapshot().phase==Phase::Signal;++tick) bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::Rising && bridge.Snapshot().SuppressesActors(),
          "the authored signal lead reaches a nonattacking nontargetable rising phase");
    const float risingTime=bridge.Snapshot().phaseSeconds;
    input.paused=true; bridge.Step(.05f,input); input.paused=false;
    input.playerAlive=false; bridge.Step(.05f,input); input.playerAlive=true;
    input.presentationReady=false; bridge.Step(.05f,input); input.presentationReady=true;
    Check(bridge.Snapshot().phase==Phase::Rising && bridge.Snapshot().phaseSeconds==risingTime &&
          !bridge.Snapshot().signalPulse && !bridge.Snapshot().revivePulse,
          "pause death and unready output cannot progress or republish the rising action");
    input.riseDurationSeconds=.001f; bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::Rising,
          "caller duration changes cannot shorten the already admitted latched rise");
    for(int tick=0;tick<10 && bridge.Snapshot().phase==Phase::Rising;++tick) bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::Rearming && bridge.Snapshot().SuppressesActors(),
          "finished rise still suppresses actors through the fair rearming interval");
    input.actorClearance=false;
    for(int tick=0;tick<30;++tick) bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::Rearming && bridge.Snapshot().waitingForClearance &&
          Near(bridge.Snapshot().phaseSeconds,SkeletonRevivalBridge::kRearmingSeconds) && !bridge.Snapshot().revivePulse,
          "blocked actor clearance holds bounded rearm state instead of spawning a collider inside the player");
    input.actorClearance=true; input.paused=true; bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::Rearming && !bridge.Snapshot().revivePulse,
          "clearing actor space while paused cannot trigger revival");
    input.paused=false; bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::Live && bridge.Snapshot().revivePulse && bridge.Snapshot().consumed &&
          !bridge.Snapshot().SuppressesActors(),
          "admitted presentation and clear space publish exactly one live revival after rearm");
    for(int tick=0;tick<3;++tick) bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::Live && !bridge.Snapshot().revivePulse && !bridge.Snapshot().signalPulse,
          "stale pre-revive both-dead snapshots cannot manufacture final completion");
    input.bothDead=false; bridge.Step(.05f,input); // One alive is not a full-pair acknowledgement.
    input.bothDead=true; bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::Live,
          "one living actor followed by both dead cannot fake a fully revived pair's final defeat");
    input.bothAlive=true; bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::Live,"contradictory alive/dead input cannot acknowledge or finish revival");
    input.bothDead=false; bridge.Step(.05f,input);
    input.bothDead=true; input.bothAlive=false; bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::Complete && bridge.Snapshot().consumed,
          "final defeat requires observed live actor state after genuine revival handoff");
    for(int tick=0;tick<10;++tick) bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::Complete && !bridge.Snapshot().signalPulse && !bridge.Snapshot().revivePulse,
          "completed bridge never duplicates its signal or revival");

    for(Phase retryPhase:{Phase::Signal,Phase::Rising,Phase::Live}) {
        SkeletonRevivalBridge retry; retry.Reset(true);
        input={}; input.playerAlive=true; input.practiceFinished=true; input.bothDead=true;
        input.bothDeathAnimationsComplete=true;
        input.presentationReady=input.actorClearance=true; input.riseDurationSeconds=.1f;
        retry.Step(.05f,input);
        for(int tick=0;tick<40 && retry.Snapshot().phase!=retryPhase;++tick) retry.Step(.05f,input);
        Check(retry.Snapshot().phase==retryPhase,"retry fixture reaches the intended consumed bridge phase");
        const auto generation=retry.Snapshot().generation;
        retry.Retry(true,false);
        Check(retry.Snapshot().phase==Phase::Live && retry.Snapshot().consumed &&
              retry.Snapshot().generation>generation && !retry.Snapshot().signalPulse && !retry.Snapshot().revivePulse,
              "retry after signal rising or live retains consumption and requests no scream or second rise");
        retry.Step(.05f,input);
        Check(retry.Snapshot().phase==Phase::Live,"retry also rejects a stale all-dead publication until ordinary standing initialization");
        input.bothDead=false; input.bothAlive=true; retry.Step(.05f,input);
        input.bothDead=true; input.bothAlive=false; retry.Step(.05f,input);
        Check(retry.Snapshot().phase==Phase::Complete,"ordinary post-retry live pair still permits its real final defeat");
    }
    bridge.Reset(true); bridge.Retry(false,false);
    Check(bridge.Snapshot().phase==Phase::Practice && !bridge.Snapshot().consumed,
          "retry before consumption preserves unfinished practice instead of inventing completion");
    bridge.Retry(true,false);
    Check(bridge.Snapshot().phase==Phase::FirstFight && !bridge.Snapshot().consumed,
          "finished pre-signal retry preserves the real first fight without consuming unavailable presentation");
    bridge.Reset(true); input={}; input.playerAlive=true; input.practiceSkipped=true; input.bothDead=true;
    bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::Live && bridge.Snapshot().consumed &&
          !bridge.Snapshot().signalPulse && !bridge.Snapshot().revivePulse,
          "explicit skip opts out even without a ready presentation and never forces a second fight");
    bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::Complete,"skip of an already defeated first pair can finish without resurrection");
    const auto consumedGeneration=bridge.Snapshot().generation;
    bridge.Reset(true);
    Check(bridge.Snapshot().phase==Phase::Practice && !bridge.Snapshot().consumed &&
          bridge.Snapshot().generation>consumedGeneration,
          "explicit replay or new game alone resets the consumed bridge and invalidates stale phase identity");
    bridge.RestoreCompleted(); const auto restoredGeneration=bridge.Snapshot().generation;
    bridge.Retry(true,false); bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::Complete && bridge.Snapshot().consumed &&
          bridge.Snapshot().generation>restoredGeneration && !bridge.Snapshot().signalPulse && !bridge.Snapshot().revivePulse,
          "validated earned-load restoration remains terminal even across retry with no sound or resurrection");
    bridge.Reset(false); bridge.Step(.05f,input);
    Check(bridge.Snapshot().phase==Phase::Disabled && !bridge.Snapshot().consumed,
          "disabled new game cannot accidentally admit a previously restored revival");
}
void InPlacePairRevivalContract() {
    // Explicit unit contacts, not a natural route or renderer acceptance claim.
    constexpr float dt=1.f/60.f, deathDuration=1.f;
    const std::array<SkeletonSpawnPose,kSkeletonCombatantCapacity> roots{{
        {{-5.5f,-15.95f},.31f,.17f}, {{-5.5f,-14.45f},-.42f,.63f},
    }};
    SwordCombat combat; combat.Reset(2,kWaterfallSkeletonPairCenter,&roots,2);
    const auto step=[&](SwordCombat& owner, const PlayerSwordContactEvaluator& contact=PlayerSwordContactEvaluator{}) -> const CombatSnapshot& {
        return owner.Update(dt,-4.5f,-15.3f,0.f,true,false,false,contact);
    };
    const auto strike=[&](std::int32_t target) {
        const int health=combat.Snapshot().combatants[target].health;
        if(combat.RequestAttack()==PlayerAttackCut::None)return false;
        bool hit=false;
        for(int tick=0;tick<90;++tick) {
            const auto& state=step(combat,[target](const auto&,auto,auto,const auto&) { return std::optional<std::int32_t>{target}; });
            hit=hit || state.combatants[target].health<health;
            if(hit && state.player.action==PlayerCombatAction::Idle)return true;
        }
        return false;
    };
    const auto rejectedUnchanged=[&](SwordCombat& owner,float duration) {
        const auto before=owner.Snapshot();
        return !owner.ReviveDefeatedPairInPlace(duration) &&
            std::memcmp(&before,&owner.Snapshot(),sizeof(before))==0;
    };
    Check(rejectedUnchanged(combat,deathDuration),"revival rejects a living pair without mutating either actor or player");
    Check(strike(0) && strike(0),"unit contacts actually defeat the first two-health actor");
    Check(rejectedUnchanged(combat,deathDuration),"one corpse and one survivor cannot be partially revived");
    Check(strike(1) && strike(1),"unit contacts actually defeat the second two-health actor");
    Check(combat.Snapshot().encounterComplete && !combat.DefeatedPairReadyToRevive(deathDuration) &&
          rejectedUnchanged(combat,deathDuration),"both defeated actors remain ineligible while the last death animation is falling");
    for(int tick=0;tick<90;++tick)step(combat);
    Check(combat.DefeatedPairReadyToRevive(deathDuration),"two actual settled unit corpses satisfy the verified-duration predicate");
    for(float duration:{0.f,-1.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()})
        Check(!combat.DefeatedPairReadyToRevive(duration) && rejectedUnchanged(combat,duration),
              "invalid death duration cannot revive or partially mutate the pair");
    auto invalidRoots=roots; invalidRoots[1].facingRadians=std::numeric_limits<float>::quiet_NaN();
    SwordCombat invalid; invalid.RestoreCampaignDefeated(invalidRoots);
    Check(rejectedUnchanged(invalid,deathDuration),"nonfinite corpse transform rejects the entire pair before mutation");
    SwordCombat incomplete; incomplete.Reset(1,kWaterfallSkeletonPairCenter,&roots,2);
    Check(rejectedUnchanged(incomplete,deathDuration),"a single-actor encounter cannot use the two-record revival operation");

    const auto settled=combat.Snapshot();
    // Queue state is deliberately not exposed. Compare its future output with
    // a copied control while enemy AI is outside its explicit arena.
    const auto samePlayer=[](const CombatSnapshot& a,const CombatSnapshot& b) {
        return a.player.action==b.player.action && a.player.reaction==b.player.reaction &&
            a.player.actionTime==b.player.actionTime && a.player.reactionTime==b.player.reactionTime &&
            a.player.comboQueued==b.player.comboQueued && a.player.guardPoseWeight==b.player.guardPoseWeight &&
            a.swordSwingRadians==b.swordSwingRadians && a.activePlayerAttackId==b.activePlayerAttackId &&
            a.playerAttackPulse==b.playerAttackPulse && a.playerAttackCut==b.playerAttackCut &&
            a.playerContactPulse==b.playerContactPulse && a.playerContactCut==b.playerContactCut &&
            a.playerContactAttackId==b.playerContactAttackId && a.playerContactTargetId==b.playerContactTargetId &&
            a.parriedAttackerIndex==b.parriedAttackerIndex;
    };
    for(int intent=0;intent<3;++intent) {
        auto candidate=combat;
        candidate.SetGuardHeld(true);
        if(intent==0)candidate.RequestParry(); // Queued fresh parry remains queued.
        else {
            candidate.RequestParry();for(int tick=0;tick<30;++tick)step(candidate);
            if(intent==1)candidate.RequestAttack(); // Queued guard-to-attack.
            else {
                candidate.RequestAttack();for(int tick=0;tick<13;++tick)step(candidate);
                Check(candidate.RequestAttack()==PlayerAttackCut::UpwardSlice,
                      "revival combo fixture queues a real accepted upward continuation");
                // RequestAttack changes authoritative player_.comboQueued,
                // while Snapshot still describes the previous publication.
                // Publish the accepted intent without advancing its clock so
                // the before/after comparison observes the same owner state.
                const auto beforePublish=candidate.Snapshot();
                candidate.Update(0.f,-4.5f,-15.3f,0.f,true,false,false);
                Check(candidate.Snapshot().player.comboQueued &&
                      candidate.Snapshot().player.action==beforePublish.player.action &&
                      candidate.Snapshot().player.actionTime==beforePublish.player.actionTime &&
                      candidate.Snapshot().activePlayerAttackId==beforePublish.activePlayerAttackId,
                      "zero-delta combo publication keeps its current action clock and attack identity");
            }
        }
        auto control=candidate;const auto playerBefore=candidate.Snapshot();
        Check(candidate.ReviveDefeatedPairInPlace(deathDuration) && samePlayer(playerBefore,candidate.Snapshot()),
              "enemy-only revival preserves every published player pose field pulses and active attack identity");
        const auto revived=candidate.Snapshot();
        for(std::size_t index=0;index<2;++index) {
            const auto& actor=revived.combatants[index];const auto& corpse=settled.combatants[index];
            Check(actor.x==corpse.x && actor.z==corpse.z && actor.facingRadians==corpse.facingRadians &&
                  actor.health==2 && actor.action==EnemyCombatAction::Locomotion && actor.animation==EnemyAnimation::Idle &&
                  actor.animationTime==0 && actor.actionTime==0 && actor.reaction==CombatReaction::None &&
                  actor.reactionTime==0 && actor.damageFlash==0 && !actor.playerHitPulse && !actor.parrySuccessPulse,
                  "both same-index actors revive at exact corpse roots/facing with original health and no stale enemy contact debt");
        }
        Check(revived.aliveCount==2 && !revived.encounterComplete && revived.attackerIndex==-1 &&
              rejectedUnchanged(candidate,deathDuration),"live pair is acknowledged atomically and a duplicate revival is an unchanged refusal");
        bool preserved=true,sawGuard=false;
        for(int tick=0;tick<100;++tick) {
            const auto& actual=step(candidate);const auto& expected=step(control);
            preserved=preserved && samePlayer(actual,expected);
            sawGuard=sawGuard || actual.player.action==PlayerCombatAction::Guard;
        }
        candidate.RequestAttack();control.RequestAttack();
        preserved=preserved && samePlayer(step(candidate),step(control));
        Check(preserved && sawGuard,"revival preserves queued parry/guard attack/combo, recovery, held return and subsequent attack serials");
    }
    auto engaged=combat;
    engaged.Update(dt,-4.5f,-15.3f,0.f,true,true,true); // Real room engagement while corpses remain settled.
    Check(engaged.ReviveDefeatedPairInPlace(deathDuration),"engaged corpse pair can revive without an encounter Reset");
    const auto rearmed=engaged.Snapshot();
    const auto& resumed=engaged.Update(dt,-7.98f,-15.2f,0.f,true,false,true);
    Check((resumed.combatants[0].x!=rearmed.combatants[0].x || resumed.combatants[0].z!=rearmed.combatants[0].z) &&
          (resumed.combatants[1].x!=rearmed.combatants[1].x || resumed.combatants[1].z!=rearmed.combatants[1].z),
          "in-place revival retains room engagement so both guards can resume toward the legal room edge");
    SkeletonRevivalBridge bridge;bridge.Reset(true);SkeletonRevivalInput input;
    input.playerAlive=input.practiceFinished=input.bothDead=input.bothDeathAnimationsComplete=true;
    input.presentationReady=input.actorClearance=true;input.riseDurationSeconds=.1f;
    bridge.Step(.05f,input);
    for(int tick=0;tick<30 && !bridge.Snapshot().revivePulse;++tick)bridge.Step(.05f,input);
    Check(bridge.Snapshot().revivePulse && combat.ReviveDefeatedPairInPlace(deathDuration),
          "one bridge pulse can hand off the actual two settled actor records in place");
    input.bothDead=combat.Snapshot().encounterComplete;input.bothAlive=combat.Snapshot().aliveCount==2;
    const auto publication=bridge.Snapshot();
    Check(!bridge.AcknowledgeLivePair(publication.generation-1,true) &&
          !bridge.AcknowledgeLivePair(publication.generation,false) &&
          !bridge.AcknowledgeLivePair(publication.generation,true,true),
          "stale generation partial pair and contradictory death claims cannot acknowledge a revival");
    Check(bridge.AcknowledgeLivePair(publication.generation,input.bothAlive,input.bothDead) &&
          bridge.Snapshot().phase==SkeletonRevivalPhase::Live &&
          bridge.Snapshot().phaseSeconds==publication.phaseSeconds &&
          bridge.Snapshot().generation==publication.generation &&
          bridge.Snapshot().signalPulse==publication.signalPulse && bridge.Snapshot().revivePulse==publication.revivePulse,
          "actual atomic pair acknowledgement preserves phase clocks generation and the current revive pulse without a second Step");
    Check(strike(0) && strike(0) && strike(1) && strike(1),
          "the acknowledged same pair is genuinely defeated again by explicit unit contacts");
    input.bothDead=combat.Snapshot().encounterComplete;input.bothAlive=combat.Snapshot().aliveCount==2;
    bridge.Step(dt,input);
    Check(bridge.Snapshot().phase==SkeletonRevivalPhase::Complete && !bridge.Snapshot().revivePulse,
          "immediate actual-pair acknowledgement permits final defeat before any subsequent live-pair Step observer");
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
    Check(teaching.Snapshot().stage==TutorialStage::Block && teaching.Snapshot().safePractice &&
          teaching.Snapshot().cue==CombatTeachingCue::Block,
          "opening lesson first explains held block with explicit practice protection");
    teaching.BlockSucceeded(); teaching.Update(true,true,false,combat,keeper,EnemyKind::Skeleton);
    Check(teaching.Snapshot().cue==CombatTeachingCue::ParryWindup&&
          !teaching.Snapshot().slowdownActive,"wind-up is distinct from anticipatory parry input cue");
    combat.combatants[0].actionTime=.90f;
    teaching.Update(true,true,false,combat,keeper,EnemyKind::Skeleton);
    Check(teaching.Snapshot().cue==CombatTeachingCue::ParryNow&&
          Near(teaching.Snapshot().simulationTimeScale,.25f),"optional quarter-speed ease is narrowly tied to an unlearned parry prompt");
    teaching.Update(true,false,false,combat,keeper,EnemyKind::Skeleton);
    Check(teaching.Snapshot().cue==CombatTeachingCue::ParryNow &&
          !teaching.Snapshot().slowdownActive && Near(teaching.Snapshot().simulationTimeScale,1.0f),
          "saved slowdown opt-out explains an unslowed live lesson without changing contact timing");
    teaching.Update(true,true,true,combat,keeper,EnemyKind::Skeleton);
    Check(!teaching.Snapshot().safePractice&&!teaching.Snapshot().slowdownActive&&
          Near(teaching.Snapshot().simulationTimeScale,1.0f)&&teaching.Snapshot().promptOpacity==0.0f,
          "suspension such as player death cancels practice, prompt and eased time immediately");
    combat.player.action=PlayerCombatAction::ParryActive;
    teaching.Update(true,true,false,combat,keeper,EnemyKind::Skeleton);
    Check(teaching.Snapshot().cue==CombatTeachingCue::ParryActive&&
          !teaching.Snapshot().slowdownActive&&Near(teaching.Snapshot().simulationTimeScale,1.0f),
          "slowdown expires as the real parry window opens");
    combat.player.action=PlayerCombatAction::Idle;
    teaching.ParrySucceeded(); teaching.Update(true,true,false,combat,keeper,EnemyKind::Skeleton);
    Check(!teaching.Snapshot().slowdownActive&&teaching.Snapshot().promptOpacity==1&&
          teaching.Snapshot().stage==TutorialStage::Dodge && teaching.Snapshot().safePractice &&
          Near(teaching.Snapshot().simulationTimeScale,1.0f),
          "understood parry advances to visible dodge teaching without continuing parry dilation");
    keeper.phase=LichPhase::Charging; keeper.phaseTime=1.10f;
    teaching.Update(true,true,false,combat,keeper,EnemyKind::Lich);
    Check(teaching.Snapshot().cue==CombatTeachingCue::DodgeNow&&teaching.Snapshot().safePractice&&
          Near(teaching.Snapshot().simulationTimeScale,.25f),
          "Keeper teaches the actual short dodge interval at optional quarter speed");
    teaching.DodgeSucceeded(); teaching.Update(true,true,false,combat,keeper,EnemyKind::Lich);
    Check(teaching.Snapshot().stage==TutorialStage::AttackFromGuard && teaching.Snapshot().safePractice,
          "block, parry and dodge cannot falsely complete the outstanding guard attack lesson");
    teaching.BeginGuardAttack(); combat.activePlayerAttackId=1;
    combat.player.action=PlayerCombatAction::SwingActive;
    teaching.ObserveGuardAttack(combat,true,false,true);
    combat.activePlayerAttackId=0; combat.player.action=PlayerCombatAction::Guard;
    teaching.ObserveGuardAttack(combat,true,false,false);
    teaching.Update(true,true,false,combat,keeper,EnemyKind::Lich);
    Check(teaching.Snapshot().stage==TutorialStage::Ready && teaching.Snapshot().safePractice &&
          teaching.Snapshot().cue==CombatTeachingCue::Ready && !teaching.Snapshot().slowdownActive,
          "all four actual skill witnesses become ready while protection awaits an explicit exit");
    teaching.Skip(); combat.combatants[0].action=EnemyCombatAction::Locomotion;
    keeper.phase=LichPhase::MaintainingRange;
    teaching.Update(true,true,false,combat,keeper,EnemyKind::Lich);
    Check(teaching.Snapshot().stage==TutorialStage::Complete && !teaching.Snapshot().safePractice &&
          teaching.Snapshot().promptOpacity==0 && Near(teaching.Snapshot().simulationTimeScale,1.f),
          "explicit ready exit at a safe boundary restores ordinary clocks and damage");
    teaching.Update(false,true,false,combat,keeper,EnemyKind::Lich);
    Check(!teaching.Snapshot().enabled&&!teaching.Snapshot().slowdownActive&&
          Near(teaching.Snapshot().simulationTimeScale,1.0f),
          "disabling teaching restores ordinary gameplay speed");
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
    Check(sim.Snapshot().playerVitals.vitality==3&&sim.Snapshot().combatTeaching.stage==TutorialStage::Block,
          "replay explicitly resets opening and restores teachable state");

    auto hitConfig=Rules(); hitConfig.waterfallSkeletonEncounter=false;
    hitConfig.playerStartX=0; hitConfig.playerStartZ=-3.75f;
    GameSimulation hit(hitConfig); InputSnapshot hitInput;
    hitInput.commands.attack=1;
    bool ordinaryHit=false;
    for(int tick=0;tick<40 && !ordinaryHit;++tick) {
        hit.StepFixed(hitInput);
        const auto state=hit.Snapshot();
        const auto owner=state.swordCombat.attackerIndex;
        if(owner<0) continue;
        const auto& actor=state.swordCombat.combatants[owner];
        ordinaryHit=actor.health==1 && actor.action==EnemyCombatAction::Staggered &&
            actor.reaction==CombatReaction::Hit;
        if(ordinaryHit)
            Check(state.combatTeaching.cue!=CombatTeachingCue::Learned &&
                  !state.combatTeaching.parryLearned,
                  "an ordinary sword-hit stagger never claims a successful parry");
    }
    Check(ordinaryHit,"feedback regression observes a real nonfatal sword-hit stagger on the current attacker");

    CombatTeaching feedback; feedback.Reset(true);
    combat.attackerIndex=0; combat.player.action=PlayerCombatAction::Idle;
    combat.combatants[0].action=EnemyCombatAction::Staggered;
    combat.combatants[0].reaction=CombatReaction::Parried;
    feedback.ParrySucceeded();
    for(float elapsed:{0.f,.25f,.50f,.75f}) {
        combat.combatants[0].actionTime=elapsed;
        feedback.Update(true,false,false,combat,keeper,EnemyKind::Skeleton);
        Check(feedback.Snapshot().cue==CombatTeachingCue::Learned &&
              feedback.Snapshot().promptOpacity==1.f && feedback.Snapshot().parryLearned,
              "actual parry success remains readable through its existing parried stagger without a new timer");
    }
    combat.combatants[0].action=EnemyCombatAction::Locomotion;
    combat.combatants[0].reaction=CombatReaction::None;
    feedback.Update(true,false,false,combat,keeper,EnemyKind::Skeleton);
    Check(feedback.Snapshot().cue==CombatTeachingCue::Block &&
          feedback.Snapshot().promptOpacity==1.f && feedback.Snapshot().safePractice,
          "success feedback ends with its stagger and returns to the genuinely unfinished block lesson");
    combat.combatants[0].action=EnemyCombatAction::Staggered;
    combat.combatants[0].reaction=CombatReaction::Hit;
    feedback.Update(true,false,false,combat,keeper,EnemyKind::Skeleton);
    Check(feedback.Snapshot().cue!=CombatTeachingCue::Learned,
          "a later ordinary hit cannot reuse an earlier learned parry as success feedback");
    combat.combatants[0].reaction=CombatReaction::Parried;
    feedback.DodgeSucceeded(); feedback.Update(true,false,false,combat,keeper,EnemyKind::Skeleton);
    Check(feedback.Snapshot().stage==TutorialStage::Block && feedback.Snapshot().safePractice &&
          feedback.Snapshot().cue==CombatTeachingCue::Learned && feedback.Snapshot().promptOpacity==1.f,
          "parry and dodge alone retain the real parry result without faking the missing two skills");
    feedback.Skip(); feedback.Update(true,false,false,combat,keeper,EnemyKind::Skeleton);
    Check(feedback.Snapshot().promptOpacity==0.f,
          "skip suppresses lesson success copy while retaining ordinary attack readability");
}
void AttackWarningPresentationContract() {
    const AttackWarningPresentation empty;
    Check(empty.source==EntityId::Invalid && empty.kind==EnemyAttackKind::Normal &&
          empty.phase==AttackWarningPhase::None && empty.phaseElapsedSeconds==0.f &&
          empty.phaseDurationSeconds==0.f && empty.contactBoundarySeconds==0.f,
          "absent warning clears source kind phase and all clock data together");
    Check(!ProductionGameSimulationConfig().specialSkeletonAttacks1_7,
          "warning description does not enable the normal yellow attack policy");
    CombatSnapshot combat; LichSnapshot keeper;
    combat.attackerIndex=0;
    auto& first=combat.combatants[0];
    first.action=EnemyCombatAction::AttackWindup;
    first.attackKind=EnemyAttackKind::GuardBreak;
    first.actionTime=.42f;
    const auto build=[&](EntityId source=EntityId::SkeletonA,bool eligible=true) {
        return BuildAttackWarningPresentation(combat,keeper,EnemyKind::Skeleton,source,eligible);
    };
    const auto windup=build();
    Check(windup.source==EntityId::SkeletonA && windup.kind==EnemyAttackKind::GuardBreak &&
          windup.phase==AttackWarningPhase::Windup && windup.phaseElapsedSeconds==first.actionTime &&
          windup.phaseDurationSeconds==CombatTimeline::kSkeletonAttackWindupSeconds &&
          windup.contactBoundarySeconds==CombatTimeline::kSkeletonAttackWindupSeconds,
          "yellow warning describes the actual token's unchanged windup clock and contact boundary");
    Check(build(EntityId::SkeletonA,false)==empty && build()==windup && first.actionTime==.42f,
          "pause eligibility suppresses the record and resume derives the retained clock without advancing or restarting it");
    for(EntityId wrong:{EntityId::Invalid,EntityId::Player,EntityId::Lich,EntityId::SkeletonB})
        Check(build(wrong)==empty,"stale or unrelated source cannot borrow the current skeleton token");
    first.attackKind=EnemyAttackKind::Normal;
    Check(build()==empty,"normal swings retain the uncluttered no-ring baseline");
    first.attackKind=EnemyAttackKind::Unblockable;
    Check(build().kind==EnemyAttackKind::Unblockable,
          "an authoritative red skeleton kind is described without assigning a new attack policy");
    first.attackKind=EnemyAttackKind::GuardBreak;
    for(auto action:{EnemyCombatAction::Locomotion,EnemyCombatAction::AttackRecovery,
                     EnemyCombatAction::Staggered,EnemyCombatAction::Dead}) {
        first.action=action;
        Check(build()==empty,"recovery stagger interruption and death immediately clear the whole warning");
    }
    first.action=EnemyCombatAction::AttackWindup; first.health=0;
    Check(build()==empty,"zero health clears a stale windup even before its action field changes");
    first.health=1;
    for(float invalid:{-.01f,std::numeric_limits<float>::quiet_NaN(),
                       std::numeric_limits<float>::infinity(),CombatTimeline::kSkeletonAttackWindupSeconds+1.f}) {
        first.actionTime=invalid;
        Check(build()==empty,"negative nonfinite and inconsistent windup clocks cannot manufacture a warning");
    }
    first.actionTime=CombatTimeline::kSkeletonAttackWindupSeconds;
    Check(build().phase==AttackWarningPhase::Windup &&
          build().phaseElapsedSeconds==CombatTimeline::kSkeletonAttackWindupSeconds,
          "an exact frozen windup endpoint stays Windup until the authoritative action changes");
    first.action=EnemyCombatAction::AttackActive; first.actionTime=0.f;
    Check(build().phase==AttackWarningPhase::Contact && build().phaseElapsedSeconds==0.f &&
          build().phaseDurationSeconds==CombatTimeline::kSkeletonAttackActiveSeconds &&
          build().contactBoundarySeconds==CombatTimeline::kSkeletonAttackWindupSeconds && !first.playerHitPulse,
          "actual Active begins the contact phase even for a whiff and does not claim a hit pulse");
    first.actionTime=CombatTimeline::kSkeletonAttackActiveSeconds;
    Check(build().phase==AttackWarningPhase::Contact,"a frozen active endpoint retains its actual source phase");
    first.actionTime=CombatTimeline::kSkeletonAttackActiveSeconds+1.f;
    Check(build()==empty,"an inconsistent active clock cannot prolong the contact description");
    first.action=EnemyCombatAction::AttackWindup; first.actionTime=.72f;
    auto& second=combat.combatants[1]; second=first; second.actionTime=.15f;
    combat.attackerIndex=1;
    const auto handed=build(EntityId::SkeletonB);
    Check(build()==empty && handed.source==EntityId::SkeletonB && handed.phaseElapsedSeconds==.15f,
          "token handoff rebinds source and time atomically rather than retaining A's clock on B");
    CombatTeaching teaching; teaching.Reset(false);
    teaching.Update(false,false,false,combat,keeper,EnemyKind::Skeleton);
    Check(!teaching.Snapshot().enabled && handed==build(EntityId::SkeletonB),
          "disabled teaching leaves the independent authoritative warning unchanged");
    combat.attackerIndex=-1;
    Check(build(EntityId::SkeletonB)==empty,"released attack token clears any previous warning identity");
    combat.attackerIndex=static_cast<std::int32_t>(combat.combatants.size());
    Check(build(EntityId::SkeletonB)==empty,"an out-of-range token never reads another actor");
    combat={};
    Check(build()==empty,"reset combat carries neither a warning nor an old source clock");

    keeper.revealComplete=true; keeper.revealPhase=KeeperRevealPhase::Combat;
    keeper.readableCastPresentation=true; keeper.phase=LichPhase::Charging;
    const auto keeperBuild=[&](bool eligible=true) {
        return BuildAttackWarningPresentation(combat,keeper,EnemyKind::Lich,EntityId::Invalid,eligible);
    };
    keeper.phaseTime=LichEncounter::kChargeDuration;
    const auto charge=keeperBuild();
    Check(charge.source==EntityId::Lich && charge.kind==EnemyAttackKind::Unblockable &&
          charge.phase==AttackWarningPhase::Windup && charge.phaseElapsedSeconds==LichEncounter::kChargeDuration &&
          charge.phaseDurationSeconds==LichEncounter::kChargeDuration &&
          charge.contactBoundarySeconds==LichEncounter::kChargeDuration,
          "Keeper uses the existing charge duration and never infers discharge from a frozen endpoint");
    Check(keeperBuild(false)==empty && keeperBuild()==charge,
          "Keeper pause suppresses presentation while resume retains its unchanged authoritative charge");
    for(float invalid:{-.01f,std::numeric_limits<float>::quiet_NaN(),
                       std::numeric_limits<float>::infinity(),LichEncounter::kChargeDuration+1.f}) {
        keeper.phaseTime=invalid;
        Check(keeperBuild()==empty,"Keeper invalid charge clocks fail closed without a replacement timer");
    }
    keeper.phase=LichPhase::Recovering; keeper.phaseTime=0.f;
    keeper.staffLightStrength=LichEncounter::kStaffLightPeak; keeper.damagePulse=false;
    const auto contact=keeperBuild();
    Check(contact.phase==AttackWarningPhase::Contact && contact.phaseElapsedSeconds==0.f &&
          contact.phaseDurationSeconds==LichEncounter::kDischargeVisibleBurstDuration &&
          contact.contactBoundarySeconds==LichEncounter::kChargeDuration && !keeper.damagePulse,
          "actual Keeper recovery entry describes the visible contact burst without claiming damage");
    keeper.phaseTime=std::nextafter(LichEncounter::kDischargeVisibleBurstDuration,0.f);
    Check(keeperBuild().phase==AttackWarningPhase::Contact,"existing discharge remains described just before its half-open endpoint");
    keeper.phaseTime=LichEncounter::kDischargeVisibleBurstDuration;
    Check(keeperBuild()==empty,"Keeper warning clears at the existing burst endpoint rather than full recovery");
    keeper.phaseTime=0.f; keeper.staffLightStrength=0.f;
    Check(keeperBuild()==empty,"a recovery phase without the actual discharge discriminator stays neutral");
    keeper.staffLightStrength=LichEncounter::kStaffLightPeak;
    for(auto phase:{LichPhase::Dormant,LichPhase::MaintainingRange,LichPhase::Repelling,LichPhase::Dead}) {
        keeper.phase=phase;
        Check(keeperBuild()==empty,"Keeper interruption rest and death clear the source and clock");
    }
    keeper.phase=LichPhase::Charging; keeper.health=0;
    Check(keeperBuild()==empty,"zero-health Keeper cannot preserve a charging warning");
    keeper.health=3; keeper.revealComplete=false;
    Check(keeperBuild()==empty,"unrevealed Keeper cannot publish an early attack warning");
    keeper.revealComplete=true; keeper.revealPhase=KeeperRevealPhase::Warning;
    Check(keeperBuild()==empty,"reveal animation is not a live attack phase");
    Check(BuildAttackWarningPresentation(combat,keeper,EnemyKind::None,EntityId::Invalid,true)==empty,
          "no selected encounter cannot inherit either owner's warning");
}
void AttackReadabilityContract() {
    Check(CombatTeachingSnapshot{}.attackReadability==AttackReadability::Neutral,
          "readability defaults to neutral before any live attack");
    CombatSnapshot combat; LichSnapshot keeper;
    combat.attackerIndex=0;
    combat.combatants[0].action=EnemyCombatAction::AttackWindup;
    combat.combatants[0].actionTime=0.0f;
    Check(EvaluateAttackReadability(combat,keeper,EnemyKind::Skeleton,true)==AttackReadability::Neutral,
          "ordinary normal skeleton attacks do not falsely display a yellow guard-break warning");
    combat.combatants[0].attackKind=EnemyAttackKind::GuardBreak;
    Check(EvaluateAttackReadability(combat,keeper,EnemyKind::Skeleton,true)==AttackReadability::Parryable,
          "yellow classification starts at the existing skeleton wind-up, not the later parry-now lesson");
    combat.combatants[0].action=EnemyCombatAction::AttackActive;
    Check(EvaluateAttackReadability(combat,keeper,EnemyKind::Skeleton,true)==AttackReadability::Parryable,
          "existing skeleton active phase retains its parryable attack category");
    combat.combatants[0].action=EnemyCombatAction::AttackRecovery;
    Check(EvaluateAttackReadability(combat,keeper,EnemyKind::Skeleton,true)==AttackReadability::Neutral,
          "skeleton recovery returns immediately to neutral rather than retaining a yellow cue");
    combat.combatants[0].action=EnemyCombatAction::AttackWindup;
    combat.combatants[0].health=0;
    Check(EvaluateAttackReadability(combat,keeper,EnemyKind::Skeleton,true)==AttackReadability::Neutral,
          "a dead skeleton cannot publish a stale attack category");
    combat.combatants[0].health=1; combat.attackerIndex=-1;
    Check(EvaluateAttackReadability(combat,keeper,EnemyKind::Skeleton,true)==AttackReadability::Neutral,
          "an absent attack token is neutral");
    combat.attackerIndex=static_cast<std::int32_t>(combat.combatants.size());
    Check(EvaluateAttackReadability(combat,keeper,EnemyKind::Skeleton,true)==AttackReadability::Neutral,
          "an out-of-range attack token cannot read an unrelated combatant");
    combat.attackerIndex=0; combat.aliveCount=0;
    Check(EvaluateAttackReadability(combat,keeper,EnemyKind::Skeleton,true)==AttackReadability::Neutral,
          "an empty live roster is neutral even if an old token remains");
    combat.aliveCount=1;

    keeper.revealComplete=true; keeper.revealPhase=KeeperRevealPhase::Combat;
    keeper.readableCastPresentation=true; keeper.phase=LichPhase::Charging; keeper.phaseTime=0.0f;
    Check(EvaluateAttackReadability(combat,keeper,EnemyKind::Lich,true)==AttackReadability::Unblockable,
          "red classification starts at real charge onset, before the narrow dodge-now lesson");
    keeper.phase=LichPhase::Recovering; keeper.phaseTime=0.0f;
    keeper.staffLightStrength=LichEncounter::kStaffLightPeak;
    Check(EvaluateAttackReadability(combat,keeper,EnemyKind::Lich,true)==AttackReadability::Unblockable,
          "the existing visible discharge remains red at the charge-to-recovery boundary");
    keeper.phaseTime=LichEncounter::kDischargeVisibleBurstDuration;
    Check(EvaluateAttackReadability(combat,keeper,EnemyKind::Lich,true)==AttackReadability::Neutral,
          "red leaves at the existing half-open discharge endpoint, not the end of all recovery");
    keeper.phase=LichPhase::MaintainingRange;
    Check(EvaluateAttackReadability(combat,keeper,EnemyKind::Lich,true)==AttackReadability::Neutral,
          "Keeper rest and positioning keep the neutral baseline");
    keeper.phase=LichPhase::Charging; keeper.revealComplete=false;
    Check(EvaluateAttackReadability(combat,keeper,EnemyKind::Lich,true)==AttackReadability::Neutral,
          "uncompleted reveal never displays a premature live attack");
    keeper.revealComplete=true; keeper.revealPhase=KeeperRevealPhase::Warning;
    Check(EvaluateAttackReadability(combat,keeper,EnemyKind::Lich,true)==AttackReadability::Neutral,
          "reveal presentation is neutral even for an inconsistent stale charge phase");
    keeper.revealPhase=KeeperRevealPhase::Combat; keeper.health=0;
    Check(EvaluateAttackReadability(combat,keeper,EnemyKind::Lich,true)==AttackReadability::Neutral,
          "death suppresses a stale red charge");
    keeper.health=3;
    Check(EvaluateAttackReadability(combat,keeper,EnemyKind::Lich,false)==AttackReadability::Neutral &&
          EvaluateAttackReadability(combat,keeper,EnemyKind::Skeleton,false)==AttackReadability::Neutral,
          "ineligible paused, dead or legacy presentation cannot publish attack warnings");

    for(int lesson=0;lesson<3;++lesson) {
        CombatTeaching teaching; teaching.Reset(lesson!=0);
        combat.combatants[0].action=EnemyCombatAction::Locomotion;
        if(lesson==1) {
            teaching.BlockSucceeded(); teaching.ParrySucceeded(); teaching.DodgeSucceeded();
            teaching.BeginGuardAttack(); combat.activePlayerAttackId=1;
            combat.player.action=PlayerCombatAction::SwingActive;
            teaching.ObserveGuardAttack(combat,true,false,true);
            combat.activePlayerAttackId=0; combat.player.action=PlayerCombatAction::Guard;
            teaching.ObserveGuardAttack(combat,true,false,false);
        }
        if(lesson!=0) {
            teaching.Skip(); teaching.Update(true,false,false,combat,keeper,EnemyKind::Skeleton);
        }
        combat.combatants[0].action=EnemyCombatAction::AttackWindup;
        teaching.Update(lesson!=0,false,false,combat,keeper,EnemyKind::Skeleton);
        Check(teaching.Snapshot().attackReadability==AttackReadability::Parryable &&
              !teaching.Snapshot().safePractice && !teaching.Snapshot().slowdownActive &&
              teaching.Snapshot().promptOpacity==0,
              "disabled, completed and skipped lessons retain early classification without practice or tutorial opacity");
        teaching.Update(lesson!=0,false,false,combat,keeper,EnemyKind::Lich);
        Check(teaching.Snapshot().attackReadability==AttackReadability::Unblockable,
              "red attack classification also survives disabled/completed/skipped lessons");
        teaching.Update(lesson!=0,false,true,combat,keeper,EnemyKind::Lich);
        Check(teaching.Snapshot().attackReadability==AttackReadability::Neutral,
              "suspension clears category without advancing an independent warning clock");
    }

    // Shared snapshot publication must use the phase advanced this tick, not
    // CombatTeaching::Update's deliberately pre-step lesson state.
    auto config=Rules(); config.waterfallSkeletonEncounter=false;
    config.specialSkeletonAttacks1_7=true;
    config.playerStartX=0; config.playerStartZ=-3.75f;
    GameSimulation sim(config); InputSnapshot input;
    input.tutorialEnabled=false; input.damageEnabled=false;
    bool windup=false, active=false, recovery=false, normal=false;
    for(int tick=0;tick<500 && !recovery;++tick) {
        sim.StepFixed(input);
        const auto state=sim.Snapshot();
        const auto index=state.swordCombat.attackerIndex;
        if(index>=0) {
            const auto phase=state.swordCombat.combatants[index].action;
            const bool yellow=state.swordCombat.combatants[index].attackKind==EnemyAttackKind::GuardBreak;
            if(!normal && !yellow && phase==EnemyCombatAction::AttackWindup) {
                normal=true;
                Check(state.attackWarning==AttackWarningPresentation{},
                      "actual normal swing snapshot leaves the ring record empty under the diagnostic special policy");
            }
            if(!windup && yellow && phase==EnemyCombatAction::AttackWindup) {
                windup=true;
                Check(state.combatTeaching.attackReadability==AttackReadability::Parryable &&
                      !state.combatTeaching.enabled,
                      "latest live wind-up is yellow in the same snapshot with teaching disabled");
                Check(state.attackWarning.source==state.skeletonAttackerId &&
                      state.attackWarning.kind==EnemyAttackKind::GuardBreak &&
                      state.attackWarning.phase==AttackWarningPhase::Windup &&
                      state.attackWarning.phaseElapsedSeconds==state.swordCombat.combatants[index].actionTime &&
                      state.attackWarning.phaseDurationSeconds==CombatTimeline::kSkeletonAttackWindupSeconds &&
                      state.attackWarning.contactBoundarySeconds==CombatTimeline::kSkeletonAttackWindupSeconds,
                      "same real fixed-step snapshot publishes the final token phase and authored clock with teaching disabled");
                input.paused=true; sim.AdvanceFrame(input,0.0);
                Check(sim.Snapshot().combatTeaching.attackReadability==AttackReadability::Neutral,
                      "pause publishes neutral without waiting for a gameplay step");
                Check(sim.Snapshot().attackWarning==AttackWarningPresentation{},
                      "zero-tick pause clears the entire attack warning record immediately");
                input.paused=false; sim.AdvanceFrame(input,0.0);
                Check(sim.Snapshot().combatTeaching.attackReadability==AttackReadability::Parryable,
                      "resuming publishes the still-current phase without advancing a cue timer");
                Check(sim.Snapshot().attackWarning==state.attackWarning,
                      "zero-tick resume republishes the retained source clock rather than restarting the ring");
            }
            if(!active && yellow && phase==EnemyCombatAction::AttackActive) {
                active=true;
                Check(state.combatTeaching.attackReadability==AttackReadability::Parryable,
                      "same-tick active publication retains the existing yellow attack category");
                Check(state.attackWarning.source==state.skeletonAttackerId &&
                      state.attackWarning.phase==AttackWarningPhase::Contact &&
                      state.attackWarning.phaseElapsedSeconds==state.swordCombat.combatants[index].actionTime &&
                      state.attackWarning.phaseDurationSeconds==CombatTimeline::kSkeletonAttackActiveSeconds,
                      "real contact phase publication uses the just-advanced actor clock and unchanged active duration");
            }
            if(yellow && phase==EnemyCombatAction::AttackRecovery) {
                recovery=true;
                Check(state.combatTeaching.attackReadability==AttackReadability::Neutral,
                      "same-tick recovery publication clears yellow despite the previous lesson cue");
                Check(state.attackWarning==AttackWarningPresentation{},
                      "real recovery snapshot removes the previous warning source kind and time together");
            }
        }
        sim.ClearEvents();
    }
    Check(normal && windup && active && recovery,"readability fixture witnesses actual normal and special attack phase progression");
    input.damageEnabled=true;
    for(int tick=0;tick<1200 && sim.Snapshot().playerAlive;++tick) {
        sim.StepFixed(input); sim.ClearEvents();
    }
    Check(!sim.Snapshot().playerAlive &&
          sim.Snapshot().combatTeaching.attackReadability==AttackReadability::Neutral,
          "the real lethal damage snapshot suppresses attack classification immediately");
    Check(sim.Snapshot().attackWarning==AttackWarningPresentation{},
          "actual player death publishes no retained enemy warning even if its attack phase remains frozen");
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
    int warning=0,pulses=0,warningTick=-1,pulseTick=-1; bool warningBefore=false;
    float warningLight=0.0f,warningTilt=0.0f;
    for(int i=0;i<130 && pulses==0;++i) {
        keeper.Update(1.0f/60,target.x+4,target.z,true,true);
        if(keeper.Snapshot().phase==LichPhase::Charging || keeper.Snapshot().phase==LichPhase::Repelling)
            Check(effects::KeeperShaderPresentationKind(true,keeper.Snapshot())==1.0f,
                  "charge glow and warning never render premature lightning");
        if(keeper.Snapshot().dischargeWarningPulse) {
            ++warning; warningTick=i; warningBefore=pulses==0;
            warningLight=keeper.Snapshot().staffLightStrength;
            warningTilt=keeper.Snapshot().presentationTiltRadians;
        }
        if(keeper.Snapshot().damagePulse) { ++pulses; pulseTick=i; }
    }
    Check(warning==1&&pulses==1&&warningBefore&&pulseTick-warningTick==18,
          "one damage pulse follows the one warning by the authored 300ms lead");
    Check(warningLight>1.9f&&warningLight<=LichEncounter::kStaffLightPeak&&warningTilt<-.07f,
          "charge warning uses a bounded late staff-glow surge and visible anticipatory pose");
    Check(effects::KeeperShaderPresentationKind(true,keeper.Snapshot())==2.0f,
          "real discharge boundary enables rendered lightning on the damage tick");
    Check(keeper.Snapshot().phase==LichPhase::Recovering&&
          Near(keeper.Snapshot().phaseTime,0.0f)&&
          Near(keeper.Snapshot().staffLightStrength,LichEncounter::kStaffLightPeak),
          "single charge-boundary damage pulse begins a separate visible discharge burst");
    for(int i=0;i<3;++i) keeper.Update(0.05f,target.x+4,target.z,true,true);
    keeper.Update(0.029f,target.x+4,target.z,true,true);
    Check(keeper.Snapshot().phase==LichPhase::Recovering&&
          keeper.Snapshot().phaseTime<LichEncounter::kDischargeVisibleBurstDuration&&
          keeper.Snapshot().staffLightStrength>0.0f,
          "visible staff discharge remains active just before its 180ms endpoint");
    keeper.Update(0.002f,target.x+4,target.z,true,true);
    Check(keeper.Snapshot().phase==LichPhase::Recovering&&
          keeper.Snapshot().staffLightStrength<=0.001f&&
          keeper.Snapshot().phaseTime<LichEncounter::kRecoveryDuration&&
          LichEncounter::kRecoveryDuration==1.80f,
          "staff discharge glow ends while the independent 1.8s recovery continues");
    Check(effects::KeeperShaderPresentationKind(true,keeper.Snapshot())==1.0f,
          "ended discharge cannot leave lightning enabled during recovery");
    int recoveryDamagePulses=0,recoveryWarnings=0;
    for(int i=0;i<7;++i) {
        keeper.Update(0.05f,target.x+4,target.z,true,true);
        recoveryDamagePulses+=keeper.Snapshot().damagePulse?1:0;
        recoveryWarnings+=keeper.Snapshot().dischargeWarningPulse?1:0;
    }
    Check(recoveryDamagePulses==0&&recoveryWarnings==0,
          "the one charge-boundary damage pulse and warning do not replay during recovery");
    target=keeper.Snapshot(); Check(keeper.TryAcceptPlayerHit(target.x,target.z)&&keeper.Snapshot().health==1,
          "second accepted hit preserves three-hit defeat");
    for(int i=0;i<121;++i) keeper.Update(1.0f/60,target.x+4,target.z,true,true);
    target=keeper.Snapshot(); Check(keeper.TryAcceptPlayerHit(target.x,target.z)&&keeper.Snapshot().phase==LichPhase::Dead&&
          keeper.Snapshot().health==0&&!keeper.Snapshot().damagePulse&&!keeper.Snapshot().dischargeWarningPulse,
          "lethal third hit cancels pending repel, charge and warning");
    Check(effects::KeeperShaderPresentationKind(false,keeper.Snapshot())==0.0f &&
          effects::KeeperShaderPresentationKind(true,keeper.Snapshot())==1.0f,
          "death cancels live discharge and skeleton selection cannot render it");
    LichEncounter legacy; legacy.ImportCombatCheckpoint();
    Check(effects::KeeperShaderPresentationKind(true,legacy.Snapshot())==2.0f,
          "historical imported captures preserve their presentation discriminator");
    {
        LichEncounter chapter = keeper;
        chapter.SetChapterDeathPresentation(true);
        for (int tick = 0; tick < 193; ++tick)
            chapter.Update(1.0f/60, target.x+4, target.z, true, true);
        Check(chapter.Snapshot().deathAnimationComplete && chapter.Snapshot().deathDissolveProgress==0,
              "chapter corpse retains the complete death clip and short settled hold");
        for (int tick = 0; tick < 23; ++tick)
            chapter.Update(1.0f/60, target.x+4, target.z, true, true);
        const float dissolve = chapter.Snapshot().deathDissolveProgress;
        Check(dissolve>0 && dissolve<1,
              "chapter corpse has a bounded visible dissolve before actor-slot retirement");
        for (int tick = 0; tick < 20; ++tick)
            chapter.Update(0, target.x+4, target.z, true, true);
        Check(chapter.Snapshot().deathDissolveProgress==dissolve,
              "paused presentation cannot advance corpse retirement");
        for (int tick = 0; tick < 120; ++tick)
            chapter.Update(1.0f/60, target.x+4, target.z, true, true);
        Check(chapter.Snapshot().deathDissolveProgress==1 && chapter.Snapshot().health==0 &&
              !chapter.Snapshot().damagePulse && !chapter.Snapshot().dischargeWarningPulse,
              "chapter retirement completes without changing defeat or rearming damage");
        chapter.BeginRetryRecognition();
        Check(chapter.Snapshot().deathDissolveProgress==0 && chapter.Snapshot().health==3,
              "retry reconstructs a visible living Keeper");
        chapter.ImportCombatCheckpoint();
        Check(chapter.Snapshot().deathDissolveProgress==0,
              "legacy import clears chapter-only dissolve state");
    }
    for(int i=0;i<180;++i) {
        keeper.Update(1.0f/60,target.x+4,target.z,true,true);
        Check(!keeper.Snapshot().damagePulse&&!keeper.Snapshot().dischargeWarningPulse,"dead Keeper never replays queued damage");
    }
    Check(keeper.Snapshot().deathAnimationComplete && Near(keeper.Snapshot().roomMistDensityScale,1.0f),
          "room mist remains full after the authored death animation, during the one-second hold");
    for(int i=0;i<19;++i) keeper.Update(.05f,target.x+4,target.z,true,true);
    Check(Near(keeper.Snapshot().roomMistDensityScale,1.0f),
          "mist cannot fade before death clip plus the full one-second delay");
    keeper.Update(.017f,target.x+4,target.z,true,true);
    Check(Near(keeper.Snapshot().roomMistDensityScale,1.0f),"mist starts from unchanged density at its hold boundary");
    for(int i=0;i<10;++i) keeper.Update(.05f,target.x+4,target.z,true,true);
    const float heldDensity=keeper.Snapshot().roomMistDensityScale;
    Check(std::abs(heldDensity-.5f)<.001f &&
          std::abs(effects::KeeperShaderPresentationKind(true,keeper.Snapshot())-.875f)<.001f,
          "half fade reaches half extinction and scattering through the actual render discriminator");
    for(int i=0;i<20;++i) keeper.Update(0,target.x+4,target.z,true,true);
    Check(keeper.Snapshot().roomMistDensityScale==heldDensity,
          "zero simulation ticks cannot advance the post-death dissolve");
    for(int i=0;i<11;++i) keeper.Update(.05f,target.x+4,target.z,true,true);
    Check(keeper.Snapshot().roomMistDensityScale==0 &&
          effects::KeeperShaderPresentationKind(true,keeper.Snapshot())==.75f &&
          !keeper.Snapshot().damagePulse && !keeper.Snapshot().dischargeWarningPulse,
          "complete dissolve preserves Keeper classification and never restores damage or electricity");
    keeper.BeginRetryRecognition();
    Check(keeper.Snapshot().roomMistDensityScale==1 && keeper.Snapshot().health==3,
          "retry discards death-clock density and restores the living encounter");
    keeper.ImportCombatCheckpoint();
    Check(keeper.Snapshot().roomMistDensityScale==1 &&
          effects::KeeperShaderPresentationKind(true,keeper.Snapshot())==2,
          "legacy capture import restores its exact full-density presentation");
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
void HeldGuardDamageContract() {
    // The ordinary encounter starts a real skeleton wind-up. The deliberately
    // early fresh parry expires before contact; only held guard may block it.
    for(bool frontal:{true,false}) {
        auto config=Rules(); config.waterfallSkeletonEncounter=false;
        config.playerStartX=0; config.playerStartZ=-3.75f;
        config.playerStartYawRadians=frontal?0.0f:3.14159265359f;
        GameSimulation sim(config); InputSnapshot input;
        input.tutorialEnabled=false; input.yawRadians=config.playerStartYawRadians;
        input.guardHeld=true; input.commands.parry=1;
        int contacts=0, damageEvents=0, successfulParries=0; bool sawGuard=false;
        const int desiredHits=frontal?2:1;
        for(int tick=0;tick<900 && contacts<desiredHits;++tick) {
            // A malicious/repeated command stream must not turn a held input
            // into periodic free parry windows or erase the incoming contacts.
            if(tick>0) ++input.commands.parry;
            sim.StepFixed(input);
            const auto& state=sim.Snapshot();
            sawGuard=sawGuard || state.playerCombat.action==PlayerCombatAction::Guard;
            for(const auto& event:sim.Events().Events()) {
                if(event.type==GameplayEventType::PlayerParrySucceeded) ++successfulParries;
                if(event.type==GameplayEventType::PlayerDamaged) ++damageEvents;
            }
            if(state.swordCombat.playerHitPulse) {
                ++contacts;
                const auto& trace=state.combatContactTrace;
                const auto& sample=trace.samples[(trace.nextIndex+trace.kCapacity-1)%trace.kCapacity];
                if(frontal)
                    Check(state.playerVitals.vitality==3 && !state.playerVitals.halfHeart &&
                          state.playerVitals.invulnerabilityRemaining==0.f &&
                          sample.outcome==CombatContactOutcome::GuardBlocked,
                          "each actual normal frontal guard contact fully blocks without granting post-hit immunity");
                else
                    Check(state.playerVitals.vitality==2 && !state.playerVitals.halfHeart &&
                          Near(state.playerVitals.invulnerabilityRemaining,1.f) &&
                          sample.outcome==CombatContactOutcome::DamageAccepted,
                          "rear melee stays full damage with its ordinary post-hit protection");
            }
            sim.ClearEvents();
        }
        Check(sawGuard && contacts==desiredHits && successfulParries==0 &&
              damageEvents==(frontal?0:1),
              "held guard blocks real normal contacts without edge-spam parries or fake damage feedback");
    }

    GameSimulation red(Rules()); InputSnapshot redInput; redInput.tutorialEnabled=false;
    red.BeginCombatPractice(EnemyKind::Lich);
    int ticks=0;
    while(ticks++<600 && !red.Snapshot().lich.revealComplete) {
        red.StepFixed(redInput); red.ClearEvents();
    }
    Check(ticks<600,"held-guard red-attack fixture completes real Keeper recognition");
    redInput.guardHeld=true; redInput.commands.parry=1;
    bool sawGuard=false, sawRedHit=false; int redParries=0;
    for(int tick=0;tick<600 && !sawRedHit;++tick) {
        red.StepFixed(redInput);
        const auto& state=red.Snapshot();
        sawGuard=sawGuard || state.playerCombat.action==PlayerCombatAction::Guard;
        for(const auto& event:red.Events().Events())
            if(event.type==GameplayEventType::PlayerParrySucceeded) ++redParries;
        if(state.lich.damagePulse) {
            sawRedHit=true;
            Check(state.playerVitals.vitality==2 && !state.playerVitals.halfHeart,
                  "real red Keeper discharge bypasses guard and deals the unchanged full heart");
        }
        red.ClearEvents();
    }
    Check(sawGuard && sawRedHit && redParries==0,
          "red-attack rejection is witnessed while guard has actually been active");

    auto legacyConfig=Rules(); legacyConfig.combatFoundation1_7=false;
    GameSimulation legacy(legacyConfig); InputSnapshot legacyInput;
    legacyInput.tutorialEnabled=false; legacyInput.damageEnabled=false;
    legacyInput.guardHeld=true; legacyInput.commands.parry=1;
    bool sawLegacyActive=false, sawLegacyGuard=false;
    for(int tick=0;tick<90;++tick) {
        legacy.StepFixed(legacyInput);
        sawLegacyActive=sawLegacyActive || legacy.Snapshot().playerCombat.action==PlayerCombatAction::ParryActive;
        sawLegacyGuard=sawLegacyGuard || legacy.Snapshot().playerCombat.action==PlayerCombatAction::Guard;
    }
    Check(sawLegacyActive && !sawLegacyGuard && legacy.Snapshot().playerCombat.action==PlayerCombatAction::Idle,
          "historical configuration keeps the original one-shot parry despite a held field");
}

void YellowGuardBreakAndDefences() {
    auto config=Rules(); config.waterfallSkeletonEncounter=false;
    config.specialSkeletonAttacks1_7=true; // Diagnostic only; production default stays OFF.
    config.playerStartX=0; config.playerStartZ=-3.75f;
    const auto reachYellow=[](GameSimulation& sim,InputSnapshot& input,float lead) {
        for(int tick=0;tick<700;++tick) {
            const auto& state=sim.Snapshot(); const auto owner=state.swordCombat.attackerIndex;
            if(owner>=0) {
                const auto& actor=state.swordCombat.combatants[owner];
                if(actor.action==EnemyCombatAction::AttackWindup && actor.attackKind==EnemyAttackKind::GuardBreak &&
                   actor.actionTime+.00001f>=CombatTimeline::kSkeletonAttackWindupSeconds-lead) return true;
            }
            sim.StepFixed(input); sim.ClearEvents();
        }
        return false;
    };
    GameSimulation held(config); InputSnapshot input;
    input.tutorialEnabled=false; input.guardHeld=true; input.commands.parry=1;
    Check(reachYellow(held,input,.02f),"held guard fixture reaches a real opted-in yellow attack after normal blocks");
    Check(held.Snapshot().playerVitals.vitality==3 && !held.Snapshot().playerVitals.halfHeart,
          "the earlier normal attacks were fully blocked before the yellow attempt");
    bool broke=false;
    for(int tick=0;tick<5 && !broke;++tick) {
        held.StepFixed(input); const auto& state=held.Snapshot();
        if(state.swordCombat.playerHitPulse) {
            const auto& trace=state.combatContactTrace;
            const auto& sample=trace.samples[(trace.nextIndex+trace.kCapacity-1)%trace.kCapacity];
            broke=sample.guardBroken && sample.incomingAttackKind==EnemyAttackKind::GuardBreak &&
                sample.outcome==CombatContactOutcome::DamageAccepted;
        }
        held.ClearEvents();
    }
    Check(broke && held.Snapshot().playerVitals.vitality==2 && !held.Snapshot().playerVitals.halfHeart &&
          held.Snapshot().playerCombat.action!=PlayerCombatAction::Guard,
          "actual yellow contact breaks held guard and deals exactly one full heart");
    bool freeRearm=false;
    for(int tick=0;tick<35;++tick) {
        ++input.commands.parry; held.StepFixed(input);
        const auto action=held.Snapshot().playerCombat.action;
        freeRearm=freeRearm || action==PlayerCombatAction::Guard ||
            action==PlayerCombatAction::ParryStartup || action==PlayerCombatAction::ParryActive;
        held.ClearEvents();
    }
    Check(!freeRearm,"yellow guard break requires physical release even with fresh command spam while held");
    input.guardHeld=false; held.StepFixed(input);
    input.guardHeld=true; ++input.commands.parry; bool rearmed=false;
    for(int tick=0;tick<40;++tick) {
        held.StepFixed(input); rearmed=rearmed || held.Snapshot().playerCombat.action==PlayerCombatAction::Guard;
        held.ClearEvents();
    }
    Check(rearmed,"release then a genuine fresh press can re-enter held guard after a yellow break");

    GameSimulation parry(config); InputSnapshot parryInput;
    parryInput.tutorialEnabled=false; parryInput.damageEnabled=false;
    Check(reachYellow(parry,parryInput,.10f),"yellow parry fixture reaches the unchanged real defense deadline");
    parryInput.damageEnabled=true; parryInput.guardHeld=true; parryInput.commands.parry=1;
    bool succeeded=false;
    for(int tick=0;tick<20 && !succeeded;++tick) {
        parry.StepFixed(parryInput);
        for(const auto& event:parry.Events().Events())
            succeeded=succeeded || event.type==GameplayEventType::PlayerParrySucceeded;
        parry.ClearEvents();
    }
    Check(succeeded && parry.Snapshot().playerVitals.vitality==3,
          "a real timed parry still defeats yellow without damage or a guard break");

    GameSimulation dodge(config); InputSnapshot dodgeInput;
    dodgeInput.damageEnabled=false;
    Check(reachYellow(dodge,dodgeInput,5.f/60),"yellow dodge fixture reaches a real incoming threat");
    dodgeInput.damageEnabled=true; dodgeInput.moveStrafe=1; dodgeInput.commands.dodge=1;
    bool protectedContact=false;
    for(int tick=0;tick<30;++tick) {
        if(tick==12) dodgeInput.moveStrafe=0;
        dodge.StepFixed(dodgeInput); const auto& state=dodge.Snapshot();
        if(state.swordCombat.playerHitPulse) {
            const auto& trace=state.combatContactTrace;
            const auto& sample=trace.samples[(trace.nextIndex+trace.kCapacity-1)%trace.kCapacity];
            protectedContact=sample.incomingAttackKind==EnemyAttackKind::GuardBreak &&
                sample.outcome==CombatContactOutcome::DodgeProtected && !sample.guardBroken;
        }
        dodge.ClearEvents();
    }
    Check(protectedContact && dodge.Snapshot().combatTeaching.dodgeLearned && dodge.Snapshot().playerVitals.vitality==3,
          "an actual outward dodge of yellow keeps the shared protection window and teaches from its real threat");
    std::cout<<"Yellow outward dodge protected-contact="<<protectedContact
             <<" learned="<<dodge.Snapshot().combatTeaching.dodgeLearned<<'\n';
}

void FourSkillPracticeAndExit() {
    auto config=Rules(); config.waterfallSkeletonEncounter=false;
    config.playerStartX=0; config.playerStartZ=-3.75f;
    GameSimulation sim(config); InputSnapshot input;
    Check(LearnBlockFromRealContact(sim,input) && sim.Snapshot().combatTeaching.stage==TutorialStage::Parry,
          "four-skill practice advances only after a real guarded normal contact");
    const auto reachTell=[&](float lead) {
        for(int tick=0;tick<600;++tick) {
            const auto& state=sim.Snapshot(); const auto owner=state.swordCombat.attackerIndex;
            if(owner>=0 && state.swordCombat.combatants[owner].action==EnemyCombatAction::AttackWindup &&
               state.swordCombat.combatants[owner].actionTime+.00001f>=CombatTimeline::kSkeletonAttackWindupSeconds-lead)
                return true;
            sim.StepFixed(input); sim.ClearEvents();
        }
        return false;
    };
    Check(reachTell(.10f),"four-skill practice reaches actual parry timing after learning block");
    ++input.commands.parry;
    for(int tick=0;tick<20 && !sim.Snapshot().combatTeaching.parryLearned;++tick) {
        sim.StepFixed(input); sim.ClearEvents();
    }
    Check(sim.Snapshot().combatTeaching.parryLearned && sim.Snapshot().combatTeaching.stage==TutorialStage::Dodge,
          "real successful parry advances the protected lesson to dodge");
    Check(reachTell(5.f/60),"four-skill practice waits for the next real attack before its outward dodge");
    input.moveStrafe=1; ++input.commands.dodge;
    for(int tick=0;tick<30;++tick) {
        if(tick==12) input.moveStrafe=0;
        sim.StepFixed(input); sim.ClearEvents();
    }
    input.moveStrafe=0;
    Check(sim.Snapshot().combatTeaching.dodgeLearned && sim.Snapshot().combatTeaching.stage==TutorialStage::AttackFromGuard,
          "an actual protected sideways dodge advances to the guard-attack lesson");
    input.guardHeld=false; sim.StepFixed(input);
    input.guardHeld=true; ++input.commands.parry;
    for(int tick=0;tick<45;++tick) { sim.StepFixed(input); sim.ClearEvents(); }
    const auto before=sim.Snapshot(); const auto owner=before.swordCombat.attackerIndex;
    Check(owner>=0 && before.playerCombat.action==PlayerCombatAction::Guard,
          "guard-attack practice starts in a real held pose after dodge recovery and release");
    if(owner>=0)
        input.yawRadians=std::atan2(before.swordCombat.combatants[owner].x-before.playerX,
                                   before.playerZ-before.swordCombat.combatants[owner].z);
    ++input.commands.attack; bool actualHit=false;
    for(int tick=0;tick<100 && !sim.Snapshot().combatTeaching.attackFromGuardLearned;++tick) {
        sim.StepFixed(input);
        for(const auto& event:sim.Events().Events()) actualHit=actualHit || event.type==GameplayEventType::EnemyHit;
        sim.ClearEvents();
    }
    const auto ready=sim.Snapshot().combatTeaching;
    std::cout<<"Four-skill actual flags="<<ready.blockLearned<<ready.parryLearned<<ready.dodgeLearned
             <<ready.attackFromGuardLearned<<" hit="<<actualHit<<" stage="<<int(ready.stage)
             <<" protected-contacts="<<sim.Snapshot().dodgeProtectedHitCount<<'\n';
    Check(actualHit && ready.blockLearned && ready.parryLearned && ready.dodgeLearned &&
          ready.attackFromGuardLearned && ready.stage==TutorialStage::Ready && ready.safePractice &&
          sim.Snapshot().playerCombat.action==PlayerCombatAction::Guard,
          "all four skills require actual outcomes and legitimate held-guard return before protected Ready");
    Check(reachTell(.30f),"ready practice still has a live threat for safe-exit validation");
    ++input.commands.tutorialSkip; sim.StepFixed(input);
    Check(sim.Snapshot().combatTeaching.exitPending && sim.Snapshot().combatTeaching.safePractice &&
          sim.Snapshot().combatTeaching.stage==TutorialStage::Ready,
          "explicit finish during an enemy windup keeps protection until a safe attack boundary");
    input.paused=true; sim.AdvanceFrame(input,.20);
    Check(sim.Snapshot().combatTeaching.exitPending && sim.Snapshot().combatTeaching.stage==TutorialStage::Ready,
          "pause cannot acknowledge or advance a pending practice exit");
    input.paused=false;
    for(int tick=0;tick<300 && sim.Snapshot().combatTeaching.stage!=TutorialStage::Complete;++tick) {
        sim.StepFixed(input); sim.ClearEvents();
    }
    Check(sim.Snapshot().combatTeaching.stage==TutorialStage::Complete && !sim.Snapshot().combatTeaching.safePractice,
          "finish acknowledges only after the live windup and contact have completed safely");
    sim.RetryEncounter();
    Check(sim.Snapshot().combatTeaching.stage==TutorialStage::Complete &&
          sim.Snapshot().combatTeaching.attackFromGuardLearned,
          "ordinary retry retains genuinely learned skills and does not silently re-enter protection");
    ++input.commands.tutorialReplay; sim.StepFixed(input);
    const auto replay=sim.Snapshot().combatTeaching;
    Check(replay.stage==TutorialStage::Block && !replay.blockLearned && !replay.parryLearned &&
          !replay.dodgeLearned && !replay.attackFromGuardLearned,
          "explicit replay resets all four learning witnesses for actual repeated practice");

    // The small observer must reject button presses, whiffs, release,
    // interruption and changing attack identity rather than manufacture credit.
    for(int failure=0;failure<5;++failure) {
        CombatTeaching teaching; teaching.Reset(true); CombatSnapshot combat;
        if(failure!=0) teaching.BeginGuardAttack();
        combat.activePlayerAttackId=10; combat.player.action=PlayerCombatAction::SwingActive;
        teaching.ObserveGuardAttack(combat,true,false,failure!=1);
        if(failure==2) teaching.ObserveGuardAttack(combat,false,false,false);
        if(failure==3) teaching.ObserveGuardAttack(combat,true,true,false);
        if(failure==4) { combat.activePlayerAttackId=11; teaching.ObserveGuardAttack(combat,true,false,true); }
        combat.activePlayerAttackId=0; combat.player.action=PlayerCombatAction::Guard;
        teaching.ObserveGuardAttack(combat,true,false,false);
        Check(!teaching.Snapshot().attackFromGuardLearned,
              "guard attack cannot be learned without origin, actual contact, held continuity and matching recovery identity");
    }
    CombatTeaching empty; empty.Reset(true); CombatSnapshot defeated; LichSnapshot keeper;
    defeated.aliveCount=0;
    for(auto& actor:defeated.combatants) { actor.health=0; actor.action=EnemyCombatAction::Dead; }
    empty.Update(true,false,false,defeated,keeper,EnemyKind::Skeleton);
    Check(empty.Snapshot().cue==CombatTeachingCue::Replay && empty.Snapshot().safePractice &&
          !empty.Snapshot().blockLearned && !empty.Snapshot().parryLearned,
          "all guards dead before lessons offers explicit replay without fake learning or automatic revival");
    empty.Skip(); empty.Update(true,false,false,defeated,keeper,EnemyKind::Skeleton);
    Check(empty.Snapshot().stage==TutorialStage::Skipped && !empty.Snapshot().safePractice,
          "a safely defeated practice pair still permits an explicit unearned skip");
    empty.Reset(true); empty.RestoreEnded(); empty.Update(true,false,true,defeated,keeper,EnemyKind::Lich);
    Check(empty.Snapshot().stage==TutorialStage::Skipped && !empty.Snapshot().safePractice &&
          !empty.Snapshot().blockLearned && !empty.Snapshot().parryLearned && !empty.Snapshot().dodgeLearned &&
          !empty.Snapshot().attackFromGuardLearned,
          "earned restore is terminal even when paused and never invents four learned flags");
}

void GuardAttackAndReleaseAtContact() {
    for(bool attack:{false,true}) {
        auto config=Rules(); config.waterfallSkeletonEncounter=false;
        config.playerStartX=0; config.playerStartZ=-3.75f;
        GameSimulation sim(config); InputSnapshot input; input.tutorialEnabled=false;
        input.guardHeld=true; input.commands.parry=1;
        int ticks=0;
        while(ticks++<600) {
            const auto& state=sim.Snapshot();
            const auto index=state.swordCombat.attackerIndex;
            if(index>=0 && state.swordCombat.combatants[index].action==EnemyCombatAction::AttackWindup &&
               state.swordCombat.combatants[index].actionTime+.00001f>=
                   CombatTimeline::kSkeletonAttackWindupSeconds-1.0f/120.0f) break;
            sim.StepFixed(input); sim.ClearEvents();
        }
        Check(ticks<600 && sim.Snapshot().playerCombat.action==PlayerCombatAction::Guard,
              "same-tick guard fixture reaches the final real melee wind-up while guarding");
        if(attack) ++input.commands.attack;
        else input.guardHeld=false;
        bool actualHit=false;
        for(int tick=0;tick<4 && !actualHit;++tick) {
            sim.StepFixed(input);
            for(const auto& event:sim.Events().Events())
                if(event.type==GameplayEventType::PlayerDamaged) actualHit=true;
            sim.ClearEvents();
        }
        Check(actualHit && sim.Snapshot().playerVitals.vitality==2 &&
              !sim.Snapshot().playerVitals.halfHeart,
              attack?"attack starts before contact and immediately ends guard mitigation":
                     "release before contact immediately ends guard mitigation");
        if(attack)
            Check(sim.Snapshot().playerCombat.action==PlayerCombatAction::SwingWindup,
                  "damage fixture attacks from guard without an intervening idle action");
    }
}

void GuardInterruptionRearm() {
    GameSimulation sim(Rules()); InputSnapshot input;
    input.tutorialEnabled=false; input.damageEnabled=false;
    const auto enterGuard=[&]() {
        input.guardHeld=false; sim.StepFixed(input);
        input.guardHeld=true; ++input.commands.parry;
        bool entered=false;
        for(int tick=0;tick<45;++tick) {
            sim.StepFixed(input);
            entered=entered || sim.Snapshot().playerCombat.action==PlayerCombatAction::Guard;
        }
        Check(entered,"fresh post-release guard fixture reaches held state");
    };
    const auto cannotRearmWhileHeld=[&](const char* message) {
        bool rearmed=false;
        for(int tick=0;tick<40;++tick) {
            sim.StepFixed(input);
            const auto action=sim.Snapshot().playerCombat.action;
            rearmed=rearmed || action==PlayerCombatAction::Guard ||
                    action==PlayerCombatAction::ParryStartup || action==PlayerCombatAction::ParryActive;
        }
        Check(!rearmed,message);
    };
    enterGuard();
    input.paused=true; sim.AdvanceFrame(input,1.0/60.0);
    Check(sim.Snapshot().playerCombat.action!=PlayerCombatAction::Guard,
          "pause cancels logical guard without waiting for a fixed gameplay tick");
    input.paused=false;
    cannotRearmWhileHeld("resuming with old held input cannot regain guard or a free parry");
    enterGuard();
    ++input.commands.dodge; input.moveStrafe=1.0f; sim.StepFixed(input); input.moveStrafe=0.0f;
    Check(sim.Snapshot().dodgeActive && sim.Snapshot().playerCombat.action!=PlayerCombatAction::Guard,
          "accepted dodge interrupts guard immediately");
    cannotRearmWhileHeld("holding through dodge expiry cannot silently restore guard");
    enterGuard();
    sim.ResetRoute();
    cannotRearmWhileHeld("route reset discards held guard until a real release");
    enterGuard();
    sim.SynchronizePausedInput(input,2,PausedInputPolicy::DiscardAllCommands);
    input.paused=false;
    cannotRearmWhileHeld("lifecycle synchronization cannot replay held guard into a new owner interval");
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
    Check(LearnBlockFromRealContact(sim,input),"slowdown fixture earns the prerequisite block from actual normal contact");
    int ticks=0;
    while(ticks++<600 && sim.Snapshot().combatTeaching.cue!=CombatTeachingCue::ParryNow) {
        sim.StepFixed(input); sim.ClearEvents();
    }
    Check(ticks<600,"real opening attack enters optional teaching slowdown");
    if(sim.Snapshot().combatTeaching.cue==CombatTeachingCue::ParryNow)
        sim.StepFixed(input);
    Check(Near(sim.Snapshot().gameplayTimeScale,.25f),
          "focus cancellation fixture first reaches live optional slowdown");
    sim.SynchronizePausedInput(input,2,PausedInputPolicy::DiscardAllCommands);
    Check(Near(sim.Snapshot().gameplayTimeScale,1.0f)&&
          Near(sim.Snapshot().combatTeaching.simulationTimeScale,1.0f)&&
          !sim.Snapshot().combatTeaching.slowdownActive,
          "focus synchronization immediately clears both cached and teaching slowdown");
    // Use the same live encounter-distance setup as the slowdown probe above;
    // the default spawn has no enemy in range to teach a defensive window.
    GameSimulation retryProbe(config); InputSnapshot retryInput;
    retryInput.tutorialSlowdownEnabled=true;
    Check(LearnBlockFromRealContact(retryProbe,retryInput),"retry slowdown fixture also earns its block without seeding progress");
    int retryTicks=0;
    while(retryTicks++<600&&retryProbe.Snapshot().combatTeaching.cue!=CombatTeachingCue::ParryNow)
        retryProbe.StepFixed(retryInput);
    // Cue state is derived from the last completed combat snapshot; the next
    // fixed step is where that newly published cue owns the gameplay delta.
    if(retryProbe.Snapshot().combatTeaching.cue==CombatTeachingCue::ParryNow)
        retryProbe.StepFixed(retryInput);
    Check(retryTicks<600&&Near(retryProbe.Snapshot().gameplayTimeScale,.25f),
          "retry boundary fixture reaches the real optional slowdown");
    retryProbe.RetryEncounter();
    Check(Near(retryProbe.Snapshot().gameplayTimeScale,1.0f)&&
          !retryProbe.Snapshot().combatTeaching.slowdownActive,
          "retry reconstruction immediately restores normal gameplay speed");
    sim.StepFixed(input); sim.ClearEvents();
    const auto before=sim.Snapshot();
    input.commands.attack=1; ++input.commands.parry; input.commands.dodge=1;
    input.moveStrafe=1;
    sim.StepFixed(input);
    const auto after=sim.Snapshot();
    Check(after.tickIndex==before.tickIndex+1 && Near(after.gameplayTimeScale,.25f),
          "input owner still consumes one real fixed tick while gameplay time is eased");
    Check(Near(after.walkTime-before.walkTime,.25f/60) && Near(after.dodgeElapsedSeconds,.25f/60),
          "movement and dodge protection use the same scaled gameplay delta");
    Check(after.playerCombat.action==PlayerCombatAction::SwingWindup &&
          Near(after.playerCombat.actionTime,.25f/60) && after.lastConsumedAttackSequence==1 &&
          after.lastConsumedParrySequence==2 && after.lastConsumedDodgeSequence==1,
          "attack wins simultaneous parry; all input edges remain responsive during slowdown");
    const auto attacker=before.swordCombat.attackerIndex;
    Check(attacker>=0 && Near(after.swordCombat.combatants[attacker].actionTime-
          before.swordCombat.combatants[attacker].actionTime,.25f/60),
          "enemy contact clock cannot run ahead of the slowed player pose and defense");
    input.paused=true;
    sim.AdvanceFrame(input,.25);
    Check(sim.Snapshot().gameplayTimeScale==1.0f&&
          !sim.Snapshot().combatTeaching.slowdownActive,
          "pause immediately restores normal gameplay speed");
    input.paused=false;
    input.commands.tutorialSkip=1;
    sim.StepFixed(input);
    Check((sim.Snapshot().combatTeaching.stage==TutorialStage::Skipped || sim.Snapshot().combatTeaching.exitPending) &&
          !sim.Snapshot().combatTeaching.slowdownActive,"skip request exits slowdown without dropping protection in a live attack");
    for(int tick=0;tick<300 && sim.Snapshot().combatTeaching.stage!=TutorialStage::Skipped;++tick)
        sim.StepFixed(input);
    Check(sim.Snapshot().combatTeaching.stage==TutorialStage::Skipped,
          "requested skip reaches the unchanged safe combat boundary before ordinary damage resumes");
    const auto skipped=sim.Snapshot(); sim.StepFixed(input);
    Check(Near(sim.Snapshot().walkTime-skipped.walkTime,1.0f/60) &&
          sim.Snapshot().gameplayTimeScale==1,"ordinary clock resumes without double stepping");
    sim.ResetRoute();
    Check(sim.Snapshot().gameplayTimeScale==1.0f&&!sim.Snapshot().combatTeaching.slowdownActive,
          "route reconstruction cannot retain a teaching time scale");
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
    Check(sim.Snapshot().combatTeaching.cue==CombatTeachingCue::Learned &&
          sim.Snapshot().combatTeaching.promptOpacity==1.f,
          "the actual parry result publishes visible success feedback after the learned flag changes");
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
    Check(sim.Snapshot().lich.roomMistDensityScale==0,
          "actual fixed-step defeat/reward continuation publishes completed room-mist dissolve");
    const auto paused=sim.Snapshot();
    input.paused=true;
    sim.AdvanceFrame(input,10.0);
    Check(sim.Snapshot().lich.roomMistDensityScale==paused.lich.roomMistDensityScale,
          "paused publication cannot restart or advance death presentation");
    sim.RetryEncounter();
    Check(sim.Snapshot().lich.roomMistDensityScale==1 && sim.Snapshot().lich.health==3,
          "integrated retry restores full mist with the living Keeper");
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
void EnemyAttackKindAuthority() {
    RoutePosition player{0.f,-3.37f}; float yaw=0.f; bool waterfall=false;
    const auto step=[&](SwordCombat& combat)->const CombatSnapshot& {
        return combat.Update(1.f/60,player.x,player.z,yaw,true,true,waterfall);
    };
    const auto nextWindup=[&](SwordCombat& combat,std::size_t actor) {
        for(int tick=0;tick<300;++tick) {
            const auto previous=combat.Snapshot().combatants[actor].action;
            if(step(combat).combatants[actor].action==EnemyCombatAction::AttackWindup &&
               previous!=EnemyCombatAction::AttackWindup) return true;
        }
        return false;
    };
    const auto nextContact=[&](SwordCombat& combat,std::size_t actor) {
        for(int tick=0;tick<160;++tick)
            if(step(combat).combatants[actor].playerHitPulse) return true;
        return false;
    };
    SwordCombat combat; combat.Reset(1u,{0.f,-4.65f},nullptr,2);
    for(int ordinal=1;ordinal<=6;++ordinal) {
        Check(nextWindup(combat,0),"category fixture reaches each actual skeleton windup");
        const auto expected=ordinal==3 || ordinal==6
            ? EnemyAttackKind::GuardBreak : EnemyAttackKind::Normal;
        Check(combat.Snapshot().combatants[0].attackKind==expected,
              "specials are default off and follow each actor's deterministic third-attempt policy");
        // Enable during attack one, disable during the first yellow, and
        // enable during attack five. No visible windup may change category.
        if(ordinal==1 || ordinal==5) combat.SetSpecialAttacksEnabled(true);
        if(ordinal==3) combat.SetSpecialAttacksEnabled(false);
        step(combat);
        Check(combat.Snapshot().combatants[0].attackKind==expected,
              "a mid-windup policy toggle cannot recolor the already latched attack");
        Check(nextContact(combat,0) &&
              combat.Snapshot().combatants[0].playerHitKind==expected &&
              combat.Snapshot().combatants[0].attackKind==expected,
              "incoming contact carries its actual windup category even after a policy toggle");
        step(combat);
        Check(!combat.Snapshot().combatants[0].playerHitPulse &&
              combat.Snapshot().combatants[0].playerHitKind==EnemyAttackKind::Normal,
              "hit category is a one-tick contact value, never stale damage authority");
    }
    combat.Reset(1u,{0.f,-4.65f},nullptr,2);
    for(int ordinal=1;ordinal<=3;++ordinal) {
        Check(nextWindup(combat,0) &&
              combat.Snapshot().combatants[0].attackKind==EnemyAttackKind::Normal,
              "reset restores default-off attacks including the third attempt");
        Check(nextContact(combat,0),"default-off reset preserves ordinary incoming contact");
    }

    // Normal and opted-in yellow retain the same actual timed parry, source
    // identity and successful no-incoming-hit result.
    for(int parryOrdinal:{1,3}) {
        combat.Reset(1u,{0.f,-4.65f},nullptr,2);
        combat.SetSpecialAttacksEnabled(true);
        for(int ordinal=1;ordinal<parryOrdinal;++ordinal) {
            Check(nextWindup(combat,0) && nextContact(combat,0),
                  "parry category fixture advances only real preceding attacks");
        }
        Check(nextWindup(combat,0),"parry category fixture acquires its intended windup");
        const auto expected=parryOrdinal==3 ? EnemyAttackKind::GuardBreak : EnemyAttackKind::Normal;
        for(int tick=0;tick<90 &&
              combat.Snapshot().combatants[0].action==EnemyCombatAction::AttackWindup &&
              combat.Snapshot().combatants[0].actionTime<CombatTimeline::kSkeletonAttackWindupSeconds-.10f;++tick)
            step(combat);
        Check(combat.Snapshot().combatants[0].action==EnemyCombatAction::AttackWindup &&
              combat.Snapshot().combatants[0].actionTime>=CombatTimeline::kSkeletonAttackWindupSeconds-.10f,
              "normal and yellow use the unchanged authored windup deadline before the parry edge");
        combat.RequestParry(); bool parried=false;
        for(int tick=0;tick<20 && !parried;++tick)
            parried=step(combat).combatants[0].parrySuccessPulse;
        Check(parried && combat.Snapshot().combatants[0].attackKind==expected &&
              combat.Snapshot().parriedAttackerIndex==0 && !combat.Snapshot().combatants[0].playerHitPulse,
              "normal and yellow attacks remain timed-parryable without changing contact timing");
    }

    player={-4.5f,-15.3f}; waterfall=true;
    const std::array<SkeletonSpawnPose,kSkeletonCombatantCapacity> spawns{{
        {{-5.5f,-15.95f},0.f,0.f}, {{-5.5f,-14.45f},0.f,0.f},
    }};
    for(std::size_t killed:{0u,1u}) {
        combat.Reset(kSkeletonCombatantCapacity,kWaterfallSkeletonPairCenter,&spawns,1);
        combat.SetSpecialAttacksEnabled(true);
        yaw=killed==0 ? -.994421107f : -2.275290391f;
        for(int tick=0;tick<6;++tick) step(combat);
        combat.RequestAttack();
        for(int tick=0;tick<40 && combat.Snapshot().combatants[killed].health>0;++tick) step(combat);
        const std::size_t survivor=1u-killed;
        Check(combat.Snapshot().combatants[killed].health==0 &&
              combat.Snapshot().combatants[survivor].health==1 &&
              combat.Snapshot().attackerIndex==static_cast<std::int32_t>(survivor),
              "either kill order leaves the single living actor owning its own attack attempt");
        Check(combat.Snapshot().combatants[survivor].action==EnemyCombatAction::AttackWindup &&
              combat.Snapshot().combatants[survivor].attackKind==EnemyAttackKind::Normal &&
              nextContact(combat,survivor),
              "surviving A or B keeps its first normal attack independent of the dead actor");
        for(int ordinal=2;ordinal<=3;++ordinal) {
            Check(nextWindup(combat,survivor) &&
                  combat.Snapshot().combatants[survivor].attackKind==
                      (ordinal==3 ? EnemyAttackKind::GuardBreak : EnemyAttackKind::Normal) &&
                  nextContact(combat,survivor),
                  "survivor's third attack is yellow regardless of which stable actor died first");
        }
    }
    combat.RestoreCampaignDefeated(spawns);
    Check(combat.Snapshot().aliveCount==0 && combat.Snapshot().attackerIndex==-1 &&
          combat.Snapshot().combatants[0].attackKind==EnemyAttackKind::Normal &&
          combat.Snapshot().combatants[1].attackKind==EnemyAttackKind::Normal,
          "earned defeated restore has no retained special attack or attacker debt");
}
void NonfatalPairHitPreservesIndependentActions() {
    constexpr float playerStartX=-4.5f, playerStartZ=-15.3f;
    constexpr float yawTowardB=-2.275290391f;
    const std::array<SkeletonSpawnPose,kSkeletonCombatantCapacity> spawns{{
        {{-5.5f,-15.95f},0.0f,0.0f}, {{-5.5f,-14.45f},0.0f,0.0f},
    }};
    {
        SwordCombat combat;
        combat.Reset(kSkeletonCombatantCapacity,{-5.5f,-15.2f},&spawns,2);
        const CombatSnapshot* state=nullptr;
        for(int tick=0;tick<6;++tick)
            state=&combat.Update(1.0f/60.0f,playerStartX,playerStartZ,yawTowardB,true,true,true);
        Check(state->attackerIndex==0 &&
              state->combatants[0].action==EnemyCombatAction::AttackWindup,
              "direct production-pair SwordCombat fixture acquires A's windup");
        combat.RequestAttack();
        bool hitB=false;
        for(int tick=0;tick<40;++tick) {
            state=&combat.Update(1.0f/60.0f,playerStartX,playerStartZ,yawTowardB,true,true,true);
            if(state->combatants[1].health==1) { hitB=true; break; }
        }
        Check(hitB,"direct production-pair SwordCombat accepts a forgiving nonfatal hit on B");
        if(hitB) {
            Check(state->combatants[1].action==EnemyCombatAction::Staggered &&
                  state->combatants[1].reaction==CombatReaction::Hit,
                  "direct pair hit preserves B stagger while A remains nearer");
            Check(state->attackerIndex==0 &&
                  state->combatants[0].action==EnemyCombatAction::AttackWindup,
                  "direct pair hit preserves A's ongoing attack token");
            bool bStaggerCompleted=false;
            for(int tick=0;tick<55;++tick) {
                state=&combat.Update(1.0f/60.0f,playerStartX,playerStartZ,
                                     yawTowardB,true,true,true);
                if(state->combatants[1].action!=EnemyCombatAction::Staggered) {
                    bStaggerCompleted=true;
                    break;
                }
                Check(state->attackerIndex==0 &&
                      state->combatants[0].action!=EnemyCombatAction::Locomotion,
                      "B's independent stagger cannot interrupt A before completion");
            }
            Check(bStaggerCompleted && state->attackerIndex==0 &&
                  state->combatants[0].action!=EnemyCombatAction::Locomotion,
                  "finishing non-owner B stagger cannot release A's active token");
        }
        combat.Reset(kSkeletonCombatantCapacity,{-5.5f,-15.2f},&spawns,2);
        Check(combat.Snapshot().attackerIndex==-1 &&
              combat.Snapshot().combatants[0].health==2 &&
              combat.Snapshot().combatants[1].health==2,
              "pair reset clears ownership and restores both combatants");
    }
    const auto lethalPairHit=[&](const bool targetB) {
        SwordCombat combat;
        combat.Reset(kSkeletonCombatantCapacity,{-5.5f,-15.2f},&spawns,1);
        const float aimYaw=targetB ? yawTowardB : -0.994421107f;
        const CombatSnapshot* state=nullptr;
        for(int tick=0;tick<6;++tick)
            state=&combat.Update(1.0f/60.0f,playerStartX,playerStartZ,
                                 aimYaw,true,true,true);
        const std::int32_t targetIndex=targetB ? 1 : 0;
        Check(state->attackerIndex==0 &&
              state->combatants[0].action==EnemyCombatAction::AttackWindup,
              "lethal compatibility fixture begins with A owning its windup");
        combat.RequestAttack();
        bool targetDied=false;
        for(int tick=0;tick<40;++tick) {
            state=&combat.Update(1.0f/60.0f,playerStartX,playerStartZ,
                                 aimYaw,true,true,true);
            if(state->combatants[targetIndex].health==0) {
                targetDied=true;
                break;
            }
        }
        Check(targetDied,"legacy one-hit pair attack still kills its selected target");
        if(targetDied && targetB) {
            Check(state->combatants[1].action==EnemyCombatAction::Dead &&
                  state->attackerIndex==0 &&
                  state->combatants[0].action==EnemyCombatAction::AttackWindup,
                  "killing non-owner B leaves A's live windup token intact");
        } else if(targetDied) {
            Check(state->combatants[0].action==EnemyCombatAction::Dead &&
                  state->attackerIndex==1 &&
                  state->combatants[1].action==EnemyCombatAction::AttackWindup,
                  "killing token owner A releases it for live B");
        }
    };
    lethalPairHit(true);
    lethalPairHit(false);
    const auto runCase=[&](const bool moveNearerB) {
        auto config=ProductionGameSimulationConfig();
        config.swordStartsStowed=false;
        config.playerStartX=playerStartX;
        config.playerStartZ=playerStartZ;
        config.playerStartYawRadians=yawTowardB;
        GameSimulation sim(config); InputSnapshot input; input.tutorialEnabled=false;
        input.yawRadians=yawTowardB;

        // Production Waterfall spawns put A at (-5.5,-15.95) and B at
        // (-5.5,-14.45). A starts nearer and owns the windup. In one case,
        // ordinary movement makes B nearer before a forgiving aimed hit.
        sim.StepFixed(input);
        auto state=sim.Snapshot();
        Check(state.swordCombat.combatantCount==2 &&
              state.swordCombat.combatants[0].health==2 &&
              state.swordCombat.combatants[1].health==2,
              "production pair fixture starts with both modern skeletons at two health");
        Check(state.swordCombat.attackerIndex==0 &&
              state.swordCombat.combatants[0].action==EnemyCombatAction::AttackWindup,
              "A acquires the live attack token before B is contacted");

        if(moveNearerB) {
            input.moveForward=1.0f;
            for(int tick=0;tick<10;++tick) sim.StepFixed(input);
            input.moveForward=0.0f;
            state=sim.Snapshot();
            const auto& pair=state.swordCombat.combatants;
            Check(std::hypot(state.playerX-pair[1].x,state.playerZ-pair[1].z)<
                  std::hypot(state.playerX-pair[0].x,state.playerZ-pair[0].z),
                  "ordinary movement reaches the branch where B is nearest while A winds up");
            Check(state.swordCombat.attackerIndex==0 &&
                  pair[0].action==EnemyCombatAction::AttackWindup,
                  "A retains its active windup while the player moves toward B");
        }

        input.commands.attack=1;
        bool hitB=false;
        for(int tick=0;tick<40;++tick) {
            sim.StepFixed(input);
            state=sim.Snapshot();
            hitB=hitB || state.swordCombat.combatants[1].health==1;
            if(hitB) break;
        }
        Check(hitB,"live GameSimulation caller accepts the forgiving nonfatal hit on B");
        if(hitB) {
            const auto& pair=state.swordCombat.combatants;
            Check(pair[1].action==EnemyCombatAction::Staggered &&
                  pair[1].reaction==CombatReaction::Hit,
                  moveNearerB
                    ? "when B becomes nearest, its nonfatal hit keeps the stagger"
                    : "when A remains nearest, B keeps its nonfatal stagger");
            Check(state.swordCombat.attackerIndex==0 &&
                  pair[0].action==EnemyCombatAction::AttackWindup &&
                  pair[0].actionTime>0.0f,
                  moveNearerB
                    ? "hitting nearer B cannot cancel A's owned windup"
                    : "hitting B cannot cancel A's owned windup when A remains nearest");
            if(!moveNearerB) {
                bool bStaggerCompleted=false;
                for(int tick=0;tick<55;++tick) {
                    sim.StepFixed(input);
                    state=sim.Snapshot();
                    if(state.swordCombat.combatants[1].action!=EnemyCombatAction::Staggered) {
                        bStaggerCompleted=true;
                        break;
                    }
                }
                Check(bStaggerCompleted && state.swordCombat.attackerIndex==0 &&
                      state.swordCombat.combatants[0].action!=EnemyCombatAction::Locomotion,
                      "live B stagger completion preserves A's active token");
            }
        }
    };
    runCase(false);
    runCase(true);
}
void WaterfallRoomEngagementAndTeaching() {
    constexpr float yawEast=1.57079632679f;
    constexpr float startX=-7.6f, startZ=-15.2f, edgeX=-7.9f;
    Check(IsWaterfallSkeletonArena(startX,startZ) &&
          IsWaterfallSkeletonRoom(edgeX,startZ) &&
          !IsWaterfallSkeletonArena(edgeX,startZ),
          "room-edge regression begins inside activation and retreats within the actual room");

    // Exercise a completed attack at the same room edge, independently of
    // teaching and outgoing damage. An attack token is temporary ownership,
    // not the lifetime of the already engaged encounter.
    SwordCombat combat;
    combat.Reset(kSkeletonCombatantCapacity,kWaterfallSkeletonPairCenter,
                 &kWaterfallSkeletonGuardSpawns,2);
    for(int tick=0;tick<200 &&
        combat.Snapshot().combatants[0].action!=EnemyCombatAction::AttackWindup;++tick)
        combat.Update(1.f/60,startX,startZ,yawEast,true,true,true);
    Check(combat.Snapshot().attackerIndex==0 &&
          combat.Snapshot().combatants[0].action==EnemyCombatAction::AttackWindup,
          "room-edge recovery fixture acquires the nearer guard before retreat");
    int contacts=0;
    for(int tick=0;tick<420;++tick) {
        const auto& state=combat.Update(1.f/60,edgeX,startZ,yawEast,true,false,true);
        for(const auto& actor:state.combatants)
            contacts+=actor.playerHitPulse ? 1 : 0;
    }
    Check(contacts>=2,"finishing a recovery outside activation retains room engagement for the next attack");
    combat.Update(1.f/60,-8.6f,startZ,yawEast,true,false,true);
    Check(combat.Snapshot().attackerIndex==-1,
          "leaving the actual Waterfall room releases its attack ownership");
    for(int tick=0;tick<30;++tick)
        combat.Update(1.f/60,edgeX,startZ,yawEast,true,false,true);
    Check(combat.Snapshot().attackerIndex==-1,
          "re-entry outside activation cannot engage without crossing the original arena");
    combat.Update(1.f/60,startX,startZ,yawEast,true,true,true);
    Check(combat.Snapshot().attackerIndex>=0,"crossing the original arena re-engages the living pair");
    combat.Reset(kSkeletonCombatantCapacity,kWaterfallSkeletonPairCenter,
                 &kWaterfallSkeletonGuardSpawns,2);
    combat.Update(1.f/60,edgeX,startZ,yawEast,true,false,true);
    Check(combat.Snapshot().attackerIndex==-1,
          "reset clears encounter engagement as well as temporary attack ownership");

    // Real shared input and hit resolution: kill A with the ordinary two-cut
    // sequence before either lesson has been learned, after walking backwards
    // outside the activation circle. B must still take its turn.
    auto config=Rules();
    config.playerStartX=startX; config.playerStartZ=startZ;
    config.playerStartYawRadians=yawEast;
    GameSimulation sim(config); InputSnapshot input;
    input.yawRadians=yawEast; input.tutorialSlowdownEnabled=false;
    for(int tick=0;tick<200 &&
        sim.Snapshot().swordCombat.combatants[0].action!=EnemyCombatAction::AttackWindup;++tick)
        sim.StepFixed(input);
    Check(sim.Snapshot().swordCombat.attackerIndex==0,
          "live teaching encounter begins with A owning the attack token");
    input.moveForward=-1.f;
    for(int tick=0;tick<12;++tick) sim.StepFixed(input);
    input.moveForward=0.f;
    Check(IsWaterfallSkeletonRoom(sim.Snapshot().playerX,sim.Snapshot().playerZ) &&
          !IsWaterfallSkeletonArena(sim.Snapshot().playerX,sim.Snapshot().playerZ),
          "ordinary backwards movement reaches the room-edge handoff witness");
    const auto& beforeCut=sim.Snapshot();
    // Aim toward A's side of the broad forgiving cone so B walking closer
    // during A's stagger cannot change the second cut's intended target.
    input.yawRadians=std::atan2(beforeCut.swordCombat.combatants[0].x-beforeCut.playerX,
        beforeCut.playerZ-beforeCut.swordCombat.combatants[0].z)-.65f;
    input.commands.attack=1; sim.StepFixed(input);
    input.commands.attack=2;
    for(int tick=0;tick<120 && sim.Snapshot().swordCombat.combatants[0].health>0;++tick)
        sim.StepFixed(input);
    const auto killed=sim.Snapshot();
    Check(killed.swordCombat.combatants[0].health==0 &&
          killed.swordCombat.combatants[1].health==2 &&
          !killed.combatTeaching.parryLearned && !killed.combatTeaching.dodgeLearned,
          "ordinary two-cut attack kills token owner A before the teaching lessons complete");
    bool bWindup=false,bPracticeContact=false;
    for(int tick=0;tick<500 && !bPracticeContact;++tick) {
        sim.StepFixed(input);
        const auto state=sim.Snapshot();
        bWindup=bWindup || (state.swordCombat.attackerIndex==1 &&
            state.swordCombat.combatants[1].action==EnemyCombatAction::AttackWindup);
        bPracticeContact=state.swordCombat.combatants[1].playerHitPulse &&
            state.combatTeaching.practiceMisses>0;
        sim.ClearEvents();
    }
    Check(bWindup && bPracticeContact,
          "living B inherits ownership, approaches and contacts after A dies at the room edge");
    Check(sim.Snapshot().combatTeaching.safePractice &&
          sim.Snapshot().playerVitals.vitality==3 && !sim.Snapshot().playerVitals.halfHeart &&
          sim.Snapshot().playerVitals.invulnerabilityRemaining==0,
          "unlearned safe practice suppresses actual B health loss without granting post-hit immunity");
    input.commands.tutorialSkip=1; sim.StepFixed(input);
    for(int tick=0;tick<500 && sim.Snapshot().playerVitals.vitality==3;++tick) {
        sim.StepFixed(input); sim.ClearEvents();
    }
    Check(sim.Snapshot().combatTeaching.stage==TutorialStage::Skipped &&
          !sim.Snapshot().combatTeaching.safePractice &&
          sim.Snapshot().playerVitals.vitality==2 && !sim.Snapshot().playerVitals.halfHeart,
          "skipping teaching lets the same surviving B contact deal ordinary full-heart damage");
    std::cout<<"Waterfall room-edge contacts="<<contacts
             <<" killed-A-position=("<<killed.playerX<<','<<killed.playerZ<<')'
             <<" guard-health-after-cuts="<<killed.swordCombat.combatants[0].health
             <<','<<killed.swordCombat.combatants[1].health
             <<" B-windup="<<bWindup<<" B-practice-contact="<<bPracticeContact
             <<" skipped-health="<<sim.Snapshot().playerVitals.vitality<<'\n';
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

    // These are seeded legal-pose regressions through the same forgiving
    // SwordCombat consumer. Production route reachability for these exact
    // combinations is not established by this fixture. The route query only
    // decides whether authored masonry blocks contact.
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
        combat.Reset(1u,{0.0f,-4.65f},&spawns,2);
        combat.RequestAttack();
        bool hitThroughMasonry=false;
        for(int tick=1;tick<=40;++tick) {
            // Keep the seeded target stationary to isolate the route fixture;
            // player melee admission still runs on every active-window tick.
            const auto& state=combat.Update(1.0f/60.0f,playerX,playerZ,yawNorth,
                                            true,false,false);
            hitThroughMasonry=hitThroughMasonry || state.combatants[0].health<2;
        }
        Check(!hitThroughMasonry,
              caseName);
    };
    probe("straight wall blocks forgiving sword contact",.96f,.96f,-5.95f);
    probe("oblique wall blocks forgiving sword contact",.96f,1.55f,-6.06f);
    // At the south face z=-6.50, this segment crosses x=0.955405:
    // 55 mm inside the west/south corner of the authored return (x=0.90).
    probe("corner edge blocks forgiving sword contact",.55f,1.55f,-6.06f);

    // The same full-height masonry rejection applies to incoming melee/parry;
    // a close enemy may wind up across the return but cannot damage through it.
    const std::array<SkeletonSpawnPose,kSkeletonEnemyCapacity> incomingSpawns{{
        {{.96f,-5.95f},yawNorth,0.0f}, {{-1.0f,-4.65f},0.0f,0.0f},
    }};
    SwordCombat incoming;
    incoming.Reset(1u,{0.0f,-4.65f},&incomingSpawns,2);
    bool sawIncomingActive=false, incomingWallHit=false, incomingWallParry=false;
    for(int tick=0;tick<120;++tick) {
        if(incoming.Snapshot().combatants[0].action==EnemyCombatAction::AttackWindup)
            incoming.RequestParry();
        const auto& state=incoming.Update(1.0f/60.0f,.96f,playerZ,yawNorth,true,true,false);
        sawIncomingActive=sawIncomingActive || state.combatants[0].action==EnemyCombatAction::AttackActive;
        incomingWallHit=incomingWallHit || state.combatants[0].playerHitPulse;
        incomingWallParry=incomingWallParry || state.combatants[0].parrySuccessPulse;
    }
    Check(sawIncomingActive && !incomingWallHit && !incomingWallParry,
          "incoming melee cannot damage or parry through full-height masonry");

    // Moving combatants are rechecked at their current published pose. The
    // seed is intentionally allowed to pursue; any accepted hit must occur
    // only after the target has moved into a clear route segment.
    const float movingPlayerX=.96f, movingPlayerZ=-7.40f;
    const float movingTargetX=1.55f, movingTargetZ=-6.06f;
    const std::array<SkeletonSpawnPose,kSkeletonEnemyCapacity> movingSpawns{{
        {{movingTargetX,movingTargetZ},yawNorth,0.0f},
        {{-1.0f,-4.65f},0.0f,0.0f},
    }};
    SwordCombat moving;
    moving.Reset(1u,{0.0f,-4.65f},&movingSpawns,2);
    moving.RequestAttack();
    Check(IsShowcasePlayerPositionWalkable(movingPlayerX,movingPlayerZ) &&
          IsRouteAudioObstructed(movingPlayerX,movingPlayerZ,movingTargetX,movingTargetZ),
          "moving occlusion fixture starts legal with a masonry-blocked line");
    const float initialTargetX=moving.Snapshot().combatants[0].x;
    const float initialTargetZ=moving.Snapshot().combatants[0].z;
    bool movingContactWasClear=true;
    bool activeWhileBlocked=false;
    int previousHealth=moving.Snapshot().combatants[0].health;
    for(int tick=1;tick<=60;++tick) {
        const auto previousTarget=moving.Snapshot().combatants[0];
        const auto& state=moving.Update(1.0f/60.0f,movingPlayerX,movingPlayerZ,yawNorth,
                                        true,true,false);
        const auto& target=state.combatants[0];
        Check(IsSkeletonEnemyPositionWalkable(target.x,target.z),
              "moving occlusion target remains on the supported skeleton route");
        if(IsRouteAudioObstructed(movingPlayerX,movingPlayerZ,previousTarget.x,previousTarget.z) &&
           (state.player.action==PlayerCombatAction::SwingActive ||
            state.player.action==PlayerCombatAction::UpwardSliceActive))
            activeWhileBlocked=true;
        if(target.health<previousHealth)
            movingContactWasClear=movingContactWasClear &&
                !IsRouteAudioObstructed(movingPlayerX,movingPlayerZ,
                                        previousTarget.x,previousTarget.z);
        previousHealth=target.health;
    }
    Check(std::hypot(moving.Snapshot().combatants[0].x-initialTargetX,
                     moving.Snapshot().combatants[0].z-initialTargetZ)>0.01f,
          "moving occlusion target advances under ordinary skeleton navigation");
    Check(activeWhileBlocked && previousHealth==2 && movingContactWasClear,
          "active swing rejects a moving target while route masonry blocks contact");
    const std::array<SkeletonSpawnPose,kSkeletonEnemyCapacity> clearMovingSpawns{{
        {{.40f,-5.60f},yawNorth,0.0f}, {{-1.0f,-4.65f},0.0f,0.0f},
    }};
    SwordCombat clearMoving;
    clearMoving.Reset(1u,{0.0f,-4.65f},&clearMovingSpawns,2);
    clearMoving.RequestAttack();
    bool clearMovingHit=false;
    for(int tick=1;tick<=40;++tick) {
        const auto before=clearMoving.Snapshot().combatants[0];
        const auto& state=clearMoving.Update(1.0f/60.0f,.40f,-7.10f,yawNorth,true,true,false);
        if(state.combatants[0].health<before.health) {
            clearMovingHit=true;
            Check(!IsRouteAudioObstructed(.40f,-7.10f,before.x,before.z),
                  "moving clear-side contact checks the actual pre-navigation pose");
        }
    }
    Check(clearMovingHit && clearMoving.Snapshot().combatants[0].health==1 &&
          std::hypot(clearMoving.Snapshot().combatants[0].x-.40f,
                     clearMoving.Snapshot().combatants[0].z+5.60f)>.01f,
          "clear moving target advances and receives exactly one forgiving hit");
}
void RulesEligibilityDoesNotChoosePracticeExit() {
    // A frozen/legacy checkpoint can suppress current mechanics without the
    // human choosing Skip. Its subsequent ordinary Retry returns to the same
    // still-requested lesson; it does not invent any completed skills.
    GameSimulation sim(Rules()); InputSnapshot input;
    Check(sim.ApplyShowcaseCheckpoint(0),"eligibility fixture imports the existing legacy checkpoint");
    sim.StepFixed(input);
    const auto inactive=sim.Snapshot().combatTeaching;
    Check(!sim.Snapshot().modernCombatRules && !inactive.enabled && !inactive.safePractice && !inactive.exitPending &&
          inactive.stage==TutorialStage::Block && !inactive.blockLearned && !inactive.parryLearned,
          "inactive rules suppress practice without recording a user exit or invented skills");
    sim.RetryEncounter(); sim.StepFixed(input);
    Check(sim.Snapshot().modernCombatRules && sim.Snapshot().combatTeaching.enabled &&
          sim.Snapshot().combatTeaching.safePractice && sim.Snapshot().combatTeaching.stage==TutorialStage::Block,
          "ordinary retry reactivates the unchanged requested four-skill practice");
    input.tutorialEnabled=false; sim.StepFixed(input);
    Check(sim.Snapshot().combatTeaching.stage==TutorialStage::Skipped && !sim.Snapshot().combatTeaching.safePractice,
          "an actual user disable still commits its safe exit under active rules");
    input.tutorialEnabled=true;
    Check(sim.ApplyShowcaseCheckpoint(0),"explicit-skip persistence fixture imports legacy eligibility");
    sim.StepFixed(input); sim.RetryEncounter(); sim.StepFixed(input);
    Check(sim.Snapshot().combatTeaching.stage==TutorialStage::Skipped && !sim.Snapshot().combatTeaching.safePractice,
          "eligibility changes cannot undo an actual user Skip or grant new immunity");

    auto config=Rules();config.waterfallSkeletonEncounter=false;config.playerStartX=0;config.playerStartZ=-3.75f;
    GameSimulation learned(config);InputSnapshot learning;
    Check(LearnBlockFromRealContact(learned,learning),"progress preservation begins with an actual blocked contact");
    learning.guardHeld=false; learned.StepFixed(learning);
    Check(learned.ApplyShowcaseCheckpoint(0),"learned-progress fixture imports a legacy checkpoint");
    learned.StepFixed(learning);
    Check(learned.Snapshot().combatTeaching.blockLearned && learned.Snapshot().combatTeaching.stage==TutorialStage::Parry &&
          !learned.Snapshot().combatTeaching.enabled && !learned.Snapshot().combatTeaching.safePractice && !learned.Snapshot().combatTeaching.exitPending,
          "ineligible rules retain actual learned progress without choosing Skip");
    learned.RetryEncounter();learned.StepFixed(learning);
    Check(learned.Snapshot().combatTeaching.blockLearned && learned.Snapshot().combatTeaching.stage==TutorialStage::Parry &&
          learned.Snapshot().combatTeaching.safePractice,
          "retry resumes the next genuine lesson rather than resetting or fabricating flags");

    CombatTeaching teaching;teaching.Reset(true);CombatSnapshot combat;LichSnapshot keeper;
    teaching.Skip();teaching.Update(true,false,false,combat,keeper,EnemyKind::Skeleton,false);
    Check(teaching.Snapshot().exitPending && teaching.Snapshot().stage==TutorialStage::Block && !teaching.Snapshot().enabled && !teaching.Snapshot().safePractice,
          "an explicit pending Skip survives inactive rules without acknowledging them as safe gameplay");
    teaching.Update(true,false,false,combat,keeper,EnemyKind::Skeleton,true);
    Check(teaching.Snapshot().stage==TutorialStage::Skipped && !teaching.Snapshot().safePractice,
          "pending human Skip commits at the next active safe boundary");
    teaching.Reset(true);teaching.Update(true,false,false,combat,keeper,EnemyKind::Skeleton,false);
    teaching.Update(false,false,false,combat,keeper,EnemyKind::Skeleton,true);
    Check(teaching.Snapshot().stage==TutorialStage::Skipped && !teaching.Snapshot().safePractice,
          "a raw user disable on reactivation is not lost by changing eligibility");
    // A pending exit must remember protection even if its public enabled flag
    // was suppressed by an intervening inactive profile.
    teaching.Reset(true);combat.combatants[0].action=EnemyCombatAction::AttackWindup;
    teaching.Update(false,false,false,combat,keeper,EnemyKind::Skeleton,true);
    Check(teaching.Snapshot().enabled && teaching.Snapshot().exitPending && teaching.Snapshot().safePractice,
          "a real disable during a windup retains active protection until its safe boundary");
    teaching.Update(false,false,false,combat,keeper,EnemyKind::Skeleton,false);
    Check(!teaching.Snapshot().enabled && !teaching.Snapshot().safePractice && teaching.Snapshot().exitPending,
          "inactive profiles publish no teaching while retaining the deliberate protected exit");
    teaching.Update(false,false,false,combat,keeper,EnemyKind::Skeleton,true);
    Check(teaching.Snapshot().enabled && teaching.Snapshot().safePractice && teaching.Snapshot().exitPending,
          "reactivation restores only the pending exit protection rather than dropping it mid-windup");
    combat.combatants[0].action=EnemyCombatAction::AttackRecovery;
    teaching.Update(false,false,false,combat,keeper,EnemyKind::Skeleton,true);
    Check(!teaching.Snapshot().enabled && !teaching.Snapshot().safePractice && teaching.Snapshot().stage==TutorialStage::Skipped,
          "the remembered exit still ends at the unchanged safe recovery boundary");
}

namespace {
// This profile uses shared simulation contacts and real lesson/kill outcomes.
// Authoritative player-pose staging isolates legal encounter positions; it is
// NOT a natural route/playthrough, renderer, clip-admission or listening proof.
constexpr float kRevivalDeadSeconds=89.f/30.f;
constexpr float kRevivalStandSeconds=1.5f;
struct RevivalIntegrationFixture {
    static GameSimulationConfig Config() {
        auto config=Rules(); config.skeletonRevival1_7=true;
        config.playerStartX=-7.6f; config.playerStartZ=-15.2f;
        config.playerStartYawRadians=1.57079632679f;
        return config;
    }
    GameSimulation sim{Config()};
    InputSnapshot input{};
    int cries=0, defeats=0;
    RevivalIntegrationFixture() {
        input.yawRadians=1.57079632679f; input.tutorialSlowdownEnabled=false;
        Check(sim.ConfigureSkeletonRevivalPresentation(1,kRevivalDeadSeconds,kRevivalStandSeconds),
              "revival integration admits the exact Dead89/30 and Stand1.5 owner receipt");
    }
    void Step(int ticks=1) {
        for(int tick=0;tick<ticks;++tick) {
            sim.StepFixed(input);
            for(const auto& event:sim.Events().Events()) {
                cries+=event.type==GameplayEventType::LichRevivalCry ? 1 : 0;
                defeats+=event.type==GameplayEventType::EnemyDefeated ? 1 : 0;
            }
            sim.ClearEvents();
        }
    }
    template<class Predicate> bool Until(Predicate predicate,int maximumTicks=600) {
        for(int tick=0;tick<maximumTicks && !predicate();++tick) Step();
        return predicate();
    }
    void Stage(float x,float z,float yaw) {
        input.guardHeld=false; input.moveForward=input.moveStrafe=0;
        input.hasAuthoritativePlayerPose=true;
        input.authoritativePlayerX=x; input.authoritativePlayerZ=z; input.yawRadians=yaw;
        Step();
        input.hasAuthoritativePlayerPose=false;
        Step(); // Real false guard input clears the lifecycle rearm requirement.
    }
    void NearActor(std::size_t index,float distance=1.15f) {
        const auto actor=sim.Snapshot().swordCombat.combatants[index];
        Stage(actor.x-distance,actor.z,1.57079632679f);
    }
    void AimAt(std::size_t index) {
        const auto& state=sim.Snapshot(); const auto& actor=state.swordCombat.combatants[index];
        input.yawRadians=std::atan2(actor.x-state.playerX,state.playerZ-actor.z);
    }
    bool Tell(float lead) {
        return Until([&] {
            const auto& state=sim.Snapshot();const auto owner=state.swordCombat.attackerIndex;
            return owner>=0 && state.swordCombat.combatants[owner].action==EnemyCombatAction::AttackWindup &&
                state.swordCombat.combatants[owner].actionTime+.00001f>=
                    CombatTimeline::kSkeletonAttackWindupSeconds-lead;
        });
    }
    bool LearnAndFinish() {
        NearActor(0);
        input.guardHeld=true; ++input.commands.parry;
        if(!Until([&]{return sim.Snapshot().combatTeaching.blockLearned;})) return false;
        input.guardHeld=false; Step();
        if(!Tell(.10f)) return false;
        ++input.commands.parry;
        if(!Until([&]{return sim.Snapshot().combatTeaching.parryLearned;},30)) return false;
        if(!Tell(5.f/60.f)) return false;
        input.moveStrafe=1; ++input.commands.dodge;
        Step(12); input.moveStrafe=0; Step(18);
        if(!sim.Snapshot().combatTeaching.dodgeLearned) return false;
        const auto owner=sim.Snapshot().swordCombat.attackerIndex;
        if(owner<0) return false;
        NearActor(static_cast<std::size_t>(owner),1.f);
        input.guardHeld=true; ++input.commands.parry; Step(45);
        if(sim.Snapshot().playerCombat.action!=PlayerCombatAction::Guard) return false;
        AimAt(static_cast<std::size_t>(owner)); ++input.commands.attack;
        if(!Until([&]{return sim.Snapshot().combatTeaching.attackFromGuardLearned;},100)) return false;
        const auto learned=sim.Snapshot().combatTeaching;
        Check(learned.blockLearned && learned.parryLearned && learned.dodgeLearned &&
              learned.attackFromGuardLearned && learned.stage==TutorialStage::Ready && learned.safePractice,
              "revival integration earns all four lessons through actual contacts and guard-attack recovery");
        if(!Tell(.30f)) return false;
        ++input.commands.tutorialSkip; Step();
        Check(sim.Snapshot().combatTeaching.exitPending && sim.Snapshot().combatTeaching.safePractice,
              "revival lesson Finish waits protected at a real live windup");
        input.paused=true; const auto tick=sim.Snapshot().tickIndex;
        sim.AdvanceFrame(input,.2);
        Check(sim.Snapshot().tickIndex==tick && sim.Snapshot().combatTeaching.exitPending &&
              sim.Snapshot().skeletonRevival.phase==SkeletonRevivalPhase::Practice,
              "pause cannot finish practice or consume a revival");
        input.paused=false;
        return Until([&]{return sim.Snapshot().combatTeaching.stage==TutorialStage::Complete;},300);
    }
    bool Kill(std::size_t index) {
        if(sim.Snapshot().swordCombat.combatants[index].health<=0) return true;
        // A lethal downward contact may leave a legitimately queued upward
        // continuation/recovery. Wait for its real completion before treating
        // the next stable actor as an independent two-cut request; staging is
        // not a recovery cancel and rejected busy commands must not be retried.
        input.guardHeld=false; input.moveForward=input.moveStrafe=0;
        const bool ready=Until([&]{return sim.Snapshot().playerCombat.action==PlayerCombatAction::Idle;},180);
        Check(ready,"independent next-target cuts wait for all accepted attack/recovery ownership to finish");
        if(!ready) return false;
        NearActor(index,1.f); AimAt(index);
        const auto before=sim.Snapshot();
        ++input.commands.attack; Step(); const auto first=sim.Snapshot(); ++input.commands.attack;
        const bool killed=Until([&]{return sim.Snapshot().swordCombat.combatants[index].health==0;},120);
        if(!killed) {
            const auto& state=sim.Snapshot();
            std::cerr<<"revival Kill target="<<index<<" player=("<<state.playerX<<','<<state.playerZ
                <<") yaw="<<state.playerYawRadians<<" life="<<int(state.playerVitals.phase)
                <<" vitality="<<state.playerVitals.vitality<<" action="<<int(state.playerCombat.action)
                <<" before-action="<<int(before.playerCombat.action)<<" first-action="<<int(first.playerCombat.action)
                <<" attack-command="<<input.commands.attack<<" consumed="<<state.lastConsumedAttackSequence
                <<" cut="<<int(state.swordCombat.playerAttackCut)<<" active-id="<<state.swordCombat.activePlayerAttackId
                <<" trace-count="<<state.combatContactTrace.count<<'\n';
            for(std::size_t actor=0;actor<2;++actor) {
                const auto& a=state.swordCombat.combatants[actor];
                std::cerr<<" actor="<<actor<<" health="<<a.health<<" before-health="
                    <<before.swordCombat.combatants[actor].health<<" action="<<int(a.action)
                    <<" root=("<<a.x<<','<<a.z<<") facing="<<a.facingRadians<<'\n';
            }
        }
        Check(killed,"revival integration defeats the observed stable actor using actual sword contacts");
        return killed;
    }
    bool KillPair() { return Kill(0) && Kill(1); }
};
void RevivalRetryKeepsConsumed(const RevivalIntegrationFixture& source) {
    auto retry=source; const auto before=retry.sim.Snapshot().skeletonRevival;
    retry.sim.RetryEncounter();
    const auto& state=retry.sim.Snapshot();
    Check(before.consumed && state.skeletonRevival.consumed &&
          state.skeletonRevival.generation>before.generation && state.skeletonRevival.phase==SkeletonRevivalPhase::Live &&
          state.swordCombat.aliveCount==2 && !state.swordCombat.encounterComplete && !state.openingEncounterComplete,
          "ordinary Retry at a consumed phase restores the actual final pair without replaying rise or granting completion");
    const int cries=retry.cries; retry.input.damageEnabled=false; retry.input.guardHeld=false; retry.Step(90);
    Check(retry.cries==cries && !retry.sim.Snapshot().skeletonRevival.signalPulse &&
          !retry.sim.Snapshot().skeletonRevival.revivePulse,
          "a consumed Retry never replays cry or revival after its atomic live-pair acknowledgement");
}
void RevivalIntegrationEarnedPath() {
    using Phase=SkeletonRevivalPhase;
    RevivalIntegrationFixture f;
    Check(f.sim.Snapshot().skeletonRevival.enabled && f.sim.Snapshot().skeletonRevival.phase==Phase::Practice &&
          f.sim.Snapshot().skeletonRevivalPresentationReady &&
          Near(f.sim.Snapshot().skeletonRevivalDeadDurationSeconds,kRevivalDeadSeconds) &&
          Near(f.sim.Snapshot().skeletonRevivalStandDurationSeconds,kRevivalStandSeconds),
          "opt-in shared revival publishes the admitted asset durations without awarding practice or kills");
    const bool finished=f.LearnAndFinish();
    Check(finished && !f.sim.Snapshot().combatTeaching.safePractice &&
          f.sim.Snapshot().skeletonRevival.phase==Phase::FirstFight && f.cries==0 &&
          !f.sim.Snapshot().openingEncounterComplete,
          "real safe Finish arms only the first fight; it does not defeat actors or signal the Keeper");
    if(!finished) return;
    f.input.damageEnabled=false;
    if(!f.KillPair()) return;
    const auto dead=f.sim.Snapshot();
    Check(dead.swordCombat.aliveCount==0 && dead.swordCombat.encounterComplete &&
          dead.skeletonRevival.phase==Phase::FirstFight && f.cries==0 && !dead.openingEncounterComplete,
          "the actual first pair's last hit waits for settled death clips without opening Keeper progression");
    int unsettledTicks=0;
    while(f.sim.Snapshot().skeletonRevival.phase==Phase::FirstFight && unsettledTicks<240) {
        const auto& pair=f.sim.Snapshot().swordCombat.combatants;
        if(pair[0].animationTime<kRevivalDeadSeconds || pair[1].animationTime<kRevivalDeadSeconds)
            Check(f.cries==0,"unsettled actual corpse poses cannot emit a revival cry");
        f.Step(); ++unsettledTicks;
    }
    Check(f.sim.Snapshot().skeletonRevival.phase==Phase::Signal && f.cries==1 &&
          f.sim.Snapshot().skeletonRevival.consumed && f.sim.Snapshot().swordCombat.aliveCount==0,
          "only two settled corpses consume the bridge and emit exactly one semantic Keeper cry");
    if(f.sim.Snapshot().skeletonRevival.phase!=Phase::Signal) return;
    RevivalRetryKeepsConsumed(f);
    const auto corpse=f.sim.Snapshot().swordCombat.combatants;
    Check(f.Until([&]{return f.sim.Snapshot().skeletonRevival.phase==Phase::Rising;},30),
          "the single cry leads into the authored rise after the bounded signal lead");
    const auto frozen=f.sim.Snapshot().skeletonRevival;
    f.input.paused=true; f.sim.AdvanceFrame(f.input,.3);
    Check(f.sim.Snapshot().skeletonRevival.phase==frozen.phase &&
          Near(f.sim.Snapshot().skeletonRevival.phaseSeconds,frozen.phaseSeconds) && f.cries==1,
          "pause freezes the current rise and cannot replay its signal");
    f.input.paused=false;
    Check(f.sim.ConfigureSkeletonRevivalPresentation(2,kRevivalDeadSeconds,kRevivalStandSeconds,false),
          "resource invalidation is an explicit admitted owner update");
    f.Step(30);
    Check(!f.sim.Snapshot().skeletonRevivalPresentationReady &&
          f.sim.Snapshot().skeletonRevival.phase==frozen.phase &&
          Near(f.sim.Snapshot().skeletonRevival.phaseSeconds,frozen.phaseSeconds) && f.cries==1 &&
          f.sim.Snapshot().swordCombat.aliveCount==0,
          "invalidated presentation freezes rise ownership without reviving, damaging or replaying the cry");
    Check(!f.sim.ConfigureSkeletonRevivalPresentation(1,kRevivalDeadSeconds,kRevivalStandSeconds) &&
          !f.sim.ConfigureSkeletonRevivalPresentation(3,kRevivalDeadSeconds,.5f),
          "stale generation and changed in-flight clip duration cannot admit a different rise");
    Check(f.sim.ConfigureSkeletonRevivalPresentation(3,kRevivalDeadSeconds,kRevivalStandSeconds),
          "fresh matching presentation resumes the same consumed rise");
    RevivalRetryKeepsConsumed(f);
    // Stage onto an actual corpse before rearm, then leave through ordinary
    // movement with guard held: no root relocation or fake clearance flag.
    f.Stage(corpse[0].x,corpse[0].z,1.57079632679f);
    f.input.guardHeld=true; ++f.input.commands.parry; f.Step(45);
    Check(f.Until([&]{return f.sim.Snapshot().skeletonRevival.phase==Phase::Rearming;},330),
          "the admitted Dead-plus-Stand duration reaches rearm with actors still defeated");
    Check(f.Until([&]{return f.sim.Snapshot().skeletonRevival.waitingForClearance;},60),
          "a player occupying the actual corpse root holds rearm after its normal grace duration");
    Check(f.sim.Snapshot().swordCombat.aliveCount==0 && !f.sim.Snapshot().openingEncounterComplete && f.cries==1,
          "clearance waiting cannot create a partial live actor or unlock the Keeper");
    RevivalRetryKeepsConsumed(f);
    f.input.moveForward=-1.f;
    Check(f.Until([&]{return f.sim.Snapshot().skeletonRevival.phase==Phase::Live;},120),
          "ordinary player movement out of both corpse margins permits atomic rearm");
    f.input.moveForward=0;
    const auto live=f.sim.Snapshot();
    Check(live.swordCombat.aliveCount==2 && !live.swordCombat.encounterComplete &&
          live.skeletonRevival.revivePulse && live.skeletonRevival.consumed &&
          !live.openingEncounterComplete && f.cries==1,
          "the owner publishes exactly one full live pair without checkpoint or final-defeat credit");
    for(std::size_t i=0;i<2;++i) {
        const auto& actor=live.swordCombat.combatants[i];
        Check(actor.health==2 && actor.animation==EnemyAnimation::Idle && actor.action==EnemyCombatAction::Locomotion &&
              actor.animationTime==0 && actor.actionTime==0 && !actor.playerHitPulse &&
              Near(actor.x,corpse[i].x) && Near(actor.z,corpse[i].z) &&
              Near(actor.facingRadians,corpse[i].facingRadians) && live.skeletonEnemies[i].idlePhaseSeconds==0,
              "same-index new life starts at exact corpse root/facing and Idle0, including actor B without its old offset");
    }
    Check(live.playerCombat.action==PlayerCombatAction::Guard && live.playerCombat.guardPoseWeight>.99f,
          "enemy-only atomic revival preserves the player's actual continuously held guard");
    RevivalRetryKeepsConsumed(f);
    // A copied owner branch retains the actual earned provenance, not seeded
    // completion flags. Prove ordinary death and Retry cannot replay the bridge.
    auto death=f; death.NearActor(0); death.input.damageEnabled=true;
    Check(death.Until([&]{return !death.sim.Snapshot().playerAlive;},1800),
          "the genuinely revived pair regains actual full lethal damage authority");
    Check(death.sim.Snapshot().skeletonRevival.consumed && death.cries==1 &&
          !death.sim.Snapshot().openingEncounterComplete,
          "real live-pair death preserves consumed revival without granting final defeat");
    death.sim.RetryEncounter();
    Check(death.sim.Snapshot().playerAlive && death.sim.Snapshot().skeletonRevival.phase==Phase::Live &&
          death.sim.Snapshot().swordCombat.aliveCount==2 && death.cries==1,
          "Retry after a real final-pair death restores the final fight with no second cry");
    const auto keeperArrival=[](RevivalIntegrationFixture& owner) {
        // Arrival does not retroactively trigger a missed upstream event.
        // Visit the existing failure strip and observe its real clock; never
        // import a settled torch or bypass the opening/reveal admission rules.
        owner.Stage(-2.1f,-15.2f,-1.57079632679f);
        const bool settled=owner.Until([&]{return owner.sim.Snapshot().torchFailure.phase==TorchFailurePhase::Settled;},360);
        Check(settled,"Keeper fixture visits the real upstream torch trigger and waits for actual settled state");
        owner.Stage(kKeeperRetryPosition.x,kKeeperRetryPosition.z,-1.57079632679f);
        return settled;
    };
    const auto printKeeperGate=[](const RevivalIntegrationFixture& owner) {
        const auto& state=owner.sim.Snapshot();
        std::cout<<"revival Keeper gate player=("<<state.playerX<<','<<state.playerZ<<") zone="
            <<int(state.zone)<<" active-enemy="<<int(state.activeEnemyKind)<<" rules="<<state.modernCombatRules
            <<" opening-complete="<<state.openingEncounterComplete<<" bridge="<<int(state.skeletonRevival.phase)
            <<" alive-guards="<<state.swordCombat.aliveCount<<" torch-phase="<<int(state.torchFailure.phase)
            <<" torch-triggered="<<state.torchFailure.triggered<<" held="<<state.torchFailure.heldByPlayer
            <<" arrival-threshold="<<HasReachedKeeperArrivalThreshold(state.playerX,state.playerZ)
            <<" lich-phase="<<int(state.lich.phase)<<" reveal-phase="<<int(state.lich.revealPhase)
            <<" reveal-started="<<state.lich.revealStarted<<" reveal-complete="<<state.lich.revealComplete<<'\n';
    };
    auto keeperBefore=f;
    keeperArrival(keeperBefore); keeperBefore.Step(720); printKeeperGate(keeperBefore);
    Check(!keeperBefore.sim.Snapshot().lich.revealStarted && !keeperBefore.sim.Snapshot().openingEncounterComplete,
          "actual settled-torch arrival cannot trigger Keeper reveal while either revived guard remains alive");
    if(!f.KillPair()) return;
    Check(f.Until([&]{return f.sim.Snapshot().skeletonRevival.phase==Phase::Complete;},10) &&
          f.sim.Snapshot().openingEncounterComplete && f.cries==1 && f.defeats==4,
          "actual fully acknowledged final-pair deaths alone complete the opening after four real defeat events");
    keeperArrival(f);
    const bool revealed=f.Until([&]{return f.sim.Snapshot().lich.revealStarted;},720);
    printKeeperGate(f);
    Check(revealed,"the same real settled-torch Keeper arrival gate becomes eligible only after actual final-pair defeat");
    std::cout<<"revival integration actual outcomes: cry="<<f.cries<<" enemy-defeats="<<f.defeats
             <<" settled-wait-ticks="<<unsettledTicks<<" final-phase="<<int(f.sim.Snapshot().skeletonRevival.phase)<<'\n';
}
void RevivalIntegrationReplayAndOptOut() {
    using Phase=SkeletonRevivalPhase;
    // Initial tutorial OFF is an explicit preference opt-out, not an earned
    // completion: the ordinary first pair remains the only required pair.
    RevivalIntegrationFixture off; off.input.tutorialEnabled=false; off.input.damageEnabled=false; off.Step();
    Check(off.sim.Snapshot().skeletonRevival.phase==Phase::Live && off.sim.Snapshot().skeletonRevival.consumed &&
          !off.sim.Snapshot().combatTeaching.blockLearned && off.cries==0,
          "initial tutorial OFF opts out without inventing lesson witnesses or a revival");
    if(off.KillPair()) {
        off.Step();
        Check(off.sim.Snapshot().openingEncounterComplete && off.cries==0,
              "tutorial-OFF's actual ordinary pair defeat completes without a forced second fight");
    }
    RevivalIntegrationFixture early; early.input.damageEnabled=false;
    if(!early.KillPair()) return;
    early.Step(240);
    Check(early.sim.Snapshot().skeletonRevival.phase==Phase::Practice && !early.sim.Snapshot().skeletonRevival.consumed &&
          !early.sim.Snapshot().openingEncounterComplete && early.cries==0 &&
          !early.sim.Snapshot().combatTeaching.blockLearned && !early.sim.Snapshot().combatTeaching.parryLearned &&
          !early.sim.Snapshot().combatTeaching.dodgeLearned && !early.sim.Snapshot().combatTeaching.attackFromGuardLearned,
          "early real kills cannot manufacture four learned skills or consume the revival");
    auto skip=early; ++skip.input.commands.tutorialSkip; skip.Step(2);
    Check(skip.sim.Snapshot().combatTeaching.stage==TutorialStage::Skipped &&
          skip.sim.Snapshot().openingEncounterComplete && skip.cries==0,
          "explicit Skip after early kills retains the existing opt-out rather than auto-reviving corpses");
    ++early.input.commands.tutorialReplay; early.Step();
    Check(early.sim.Snapshot().skeletonRevival.phase==Phase::Practice && !early.sim.Snapshot().skeletonRevival.consumed &&
          early.sim.Snapshot().swordCombat.aliveCount==2 && !early.sim.Snapshot().openingEncounterComplete &&
          early.sim.Snapshot().combatTeaching.stage==TutorialStage::Block && early.cries==0,
          "explicit Replay restarts real actors and unearned practice without retaining corpse/rise debt");
    GameSimulation disabled(Rules());
    Check(!disabled.Snapshot().skeletonRevival.enabled &&
          !disabled.ConfigureSkeletonRevivalPresentation(1,kRevivalDeadSeconds,kRevivalStandSeconds),
          "the normal default remains revival-OFF even if a presentation receipt is offered");
}
void RevivalIntegrationProfile() {
    std::cout<<"revival integration: actual outcomes with unit-staged player poses; not a natural playthrough\n";
    RevivalIntegrationEarnedPath(); RevivalIntegrationReplayAndOptOut();
}
} // namespace

int main(int argc,char** argv) {
    if(argc==2 && std::strcmp(argv[1],"--revival-integration")==0) {
        RevivalIntegrationProfile();
        std::cout<<"revival integration checks="<<checks<<" failures="<<failures<<'\n';
        return failures?1:0;
    }
    if(argc!=1) { std::cerr<<"unknown combat foundation profile\n"; return 2; }
    SkeletonRevivalBridgeContract();
    InPlacePairRevivalContract();
    RulesEligibilityDoesNotChoosePracticeExit();
    WindowAndLifecycle(); TeachingContract(); AttackWarningPresentationContract(); AttackReadabilityContract(); KeeperContract(); ActualIncomingHits(); DeliveryRates(); CoherentSlowdown();
    HeldGuardDamageContract(); GuardAttackAndReleaseAtContact(); GuardInterruptionRearm();
    YellowGuardBreakAndDefences(); FourSkillPracticeAndExit();
    IntegratedKeeperRepelAndReward();
    OutgoingTraceContract();
    EnemyAttackKindAuthority();
    NonfatalPairHitPreservesIndependentActions();
    WaterfallRoomEngagementAndTeaching();
    RouteMeleeOcclusion();
    LiveParryAndRiposte();
    std::cout<<"combat foundation checks="<<checks<<" failures="<<failures<<'\n';
    return failures?1:0;
}
