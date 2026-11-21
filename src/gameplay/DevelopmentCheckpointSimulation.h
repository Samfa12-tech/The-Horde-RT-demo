#pragma once

#include "gameplay/DevelopmentCheckpoints.h"
#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/traversal/BellwetherNorthRoute.h"

namespace horde::gameplay
{

enum class DevelopmentCheckpointStageFailure : std::uint8_t
{
    None,
    AuthoredTerrainRequired,
    MissingTerrain,
    StaleTerrain,
    BaseCheckpointRejected,
    SimulationStateNotReached,
};

inline constexpr const char* DevelopmentCheckpointStageFailureName(
    DevelopmentCheckpointStageFailure failure) noexcept
{
    switch (failure)
    {
    case DevelopmentCheckpointStageFailure::None: return "none";
    case DevelopmentCheckpointStageFailure::AuthoredTerrainRequired: return "authored-terrain-required";
    case DevelopmentCheckpointStageFailure::MissingTerrain: return "missing-terrain";
    case DevelopmentCheckpointStageFailure::StaleTerrain: return "stale-terrain";
    case DevelopmentCheckpointStageFailure::BaseCheckpointRejected: return "base-checkpoint-rejected";
    case DevelopmentCheckpointStageFailure::SimulationStateNotReached: return "simulation-state-not-reached";
    }
    return "unknown";
}

struct DevelopmentCheckpointStageEvidence
{
    DevelopmentCheckpointStageFailure failure = DevelopmentCheckpointStageFailure::None;
    // Admission-time identities, before checkpoint imports can change them.
    std::uint64_t worldGeneration = 0u;
    std::uint64_t terrainGeneration = 0u;
    std::uint32_t consumedAttackEdges = 0u;
    std::uint32_t consumedParryEdges = 0u;
    std::uint32_t playerSwingEvents = 0u;
    std::uint32_t playerParrySucceededEvents = 0u;
    std::uint32_t playerDamagedEvents = 0u;
    std::uint32_t playerKilledEvents = 0u;
    std::uint32_t enemyHitEvents = 0u;
    PlayerCombatAction action = PlayerCombatAction::Idle;
    float actionTime = 0.0f;
    struct BellwetherVisit {
        unsigned roadsideCaptions = 0;
        bool welcomeCaption = false;
        unsigned exchangeCaptions = 0;
        unsigned quietReturns = 0;
    };
    std::optional<BellwetherVisit> bellwetherVisit;
};

// Optional, non-owning boundary callbacks for the helper's exact shared
// StepFixed invocations. The observer must not modify simulation inputs/state.
struct DevelopmentCheckpointStepFixedObservation
{
    void* user = nullptr;
    void (*beginStepFixed)(void*) noexcept = nullptr;
    void (*completeStepFixed)(void*) noexcept = nullptr;
    // The repeat-visit harness can present the ordinary snapshot after each
    // tick. Normal presentation acknowledgement is allowed. False stops staging.
    bool (*presentBellwetherVisitStep)(void*) = nullptr;
};

// Stages a debug-only visual checkpoint by advancing the same fixed-step
// command/combat/animation authority used by live play. Platforms only select
// the requested checkpoint; no renderer or platform-owned animation state is
// introduced.
inline bool StageDevelopmentCheckpointSimulation(
    simulation::GameSimulation& gameSimulation,
    const DevelopmentCheckpoint& checkpoint,
    DevelopmentCheckpointStageEvidence* evidence = nullptr,
    const DevelopmentCheckpointStepFixedObservation* stepObservation = nullptr)
{
    if (evidence != nullptr)
    {
        *evidence = {};
        evidence->worldGeneration = gameSimulation.worldRoute_.generation;
        evidence->terrainGeneration = gameSimulation.chapterTerrainGeneration_;
        evidence->failure = DevelopmentCheckpointStageFailure::SimulationStateNotReached;
    }
    const auto reject = [&](DevelopmentCheckpointStageFailure failure)
    {
        if (evidence != nullptr) evidence->failure = failure;
        return false;
    };
    const auto chapterTerrain=gameSimulation.chapterTerrain_;
    if (checkpoint.stagesRescueExterior)
    {
        if (!gameSimulation.config_.authoredChapterTerrain)
            return reject(DevelopmentCheckpointStageFailure::AuthoredTerrainRequired);
        if (!chapterTerrain)
            return reject(DevelopmentCheckpointStageFailure::MissingTerrain);
        if (gameSimulation.chapterTerrainGeneration_ != gameSimulation.worldRoute_.generation)
            return reject(DevelopmentCheckpointStageFailure::StaleTerrain);
    }
    if (checkpoint.stagesWaterfallGuards &&
        (checkpoint.waterfallGuardFixedTicks > 30u ||
         checkpoint.combatPose != DevelopmentCombatPose::Rest ||
         !gameSimulation.BeginMotionEvidenceEquipmentSeed()))
        return false;
    if (!gameSimulation.ApplyShowcaseCheckpoint(checkpoint.baseShowcaseCheckpointId))
    {
        if (checkpoint.stagesWaterfallGuards) gameSimulation.EndMotionEvidenceEquipmentSeed();
        return reject(DevelopmentCheckpointStageFailure::BaseCheckpointRejected);
    }

    if(checkpoint.developmentWorldRoute || gameSimulation.Snapshot().developmentWorldRoute)
        gameSimulation.SetDevelopmentWorldRoute(checkpoint.developmentWorldRoute,checkpoint.stagedWorldPreparation);

    if (checkpoint.developmentSupportFixture || gameSimulation.Snapshot().developmentSupportFixture)
        gameSimulation.SetDevelopmentSupportFixture(checkpoint.developmentSupportFixture,
            gameSimulation.Snapshot().playerSupportGeneration + 1u);

    const auto stepFixed = [&](const simulation::InputSnapshot& input,
                               const float fixedDeltaSeconds,
                               const std::uint64_t inputPublicationSequence)
    {
        const bool observe = stepObservation != nullptr &&
            stepObservation->beginStepFixed != nullptr &&
            stepObservation->completeStepFixed != nullptr;
        if (observe)
            stepObservation->beginStepFixed(stepObservation->user);
        gameSimulation.StepFixed(input, fixedDeltaSeconds, inputPublicationSequence);
        if (observe)
            stepObservation->completeStepFixed(stepObservation->user);
    };

    const std::uint64_t initialConsumedAttackSequence =
        gameSimulation.Snapshot().lastConsumedAttackSequence;
    const std::uint64_t initialConsumedParrySequence =
        gameSimulation.Snapshot().lastConsumedParrySequence;
    const auto accumulateEvents = [&]()
    {
        if (evidence == nullptr) return;
        for (const simulation::GameplayEvent& event : gameSimulation.Events().Events())
        {
            if (event.type == simulation::GameplayEventType::PlayerSwing)
                ++evidence->playerSwingEvents;
            if (event.type == simulation::GameplayEventType::PlayerParrySucceeded)
                ++evidence->playerParrySucceededEvents;
            if (event.type == simulation::GameplayEventType::PlayerDamaged)
                ++evidence->playerDamagedEvents;
            if (event.type == simulation::GameplayEventType::PlayerKilled)
                ++evidence->playerKilledEvents;
            if (event.type == simulation::GameplayEventType::EnemyHit)
                ++evidence->enemyHitEvents;
        }
    };
    bool routePresentationFailed=false;
    const auto finalize = [&](bool staged)
    {
        staged=staged && !routePresentationFailed;
        if (evidence != nullptr)
        {
            evidence->failure = staged ? DevelopmentCheckpointStageFailure::None
                : DevelopmentCheckpointStageFailure::SimulationStateNotReached;
            evidence->consumedAttackEdges = static_cast<std::uint32_t>(
                gameSimulation.Snapshot().lastConsumedAttackSequence -
                initialConsumedAttackSequence);
            evidence->consumedParryEdges = static_cast<std::uint32_t>(
                gameSimulation.Snapshot().lastConsumedParrySequence -
                initialConsumedParrySequence);
            evidence->action = gameSimulation.Snapshot().playerCombat.action;
            evidence->actionTime = gameSimulation.Snapshot().playerCombat.actionTime;
        }
        accumulateEvents();
        gameSimulation.ClearEvents();
        return staged;
    };
    const auto walkTo=[&](simulation::InputSnapshot& input,const auto& advance,float x,float z) {
        for(int tick=0;tick<2600 && !routePresentationFailed;++tick) {
            const auto& state=gameSimulation.Snapshot();
            const float dx=x-state.playerX,dz=z-state.playerZ;
            const float distance=std::hypot(dx,dz);
            if(distance<.008f)return true;
            input.yawRadians=std::atan2(dx,-dz);
            input.moveForward=std::min(1.0f,distance/.055f);
            input.runHeld=distance>.3f;
            advance();
        }
        return false;
    };

    const auto completedForestProgress = [] {
        dialogue::Progress progress;
        progress.grateConsumed=progress.rescueConsumed=progress.reunionStarted=progress.proofAccepted=true;
        progress.forestNightConsumed=progress.forestWaystoneConsumed=progress.forestClueConsumed=progress.forestVillageConsumed=true;
        return progress;
    };
    if(checkpoint.bellwetherBranchView!=BellwetherBranchView::None) {
        using namespace simulation;
        using namespace bellwether;
        const bool millYard=checkpoint.bellwetherBranchView==BellwetherBranchView::MillYard;
        const bool rigTrial=checkpoint.bellwetherBranchView==BellwetherBranchView::RigLeakTrial;
        const bool riverLanding=rigTrial || checkpoint.bellwetherBranchView==BellwetherBranchView::RiverLanding;
        const bool east=millYard || riverLanding;
        const auto junction=east?traversal::kBellwetherMillJunction:traversal::kBellwetherChurchJunction;
        const bool returnsToKit=checkpoint.bellwetherBranchView==BellwetherBranchView::KitIntroduction;
        const bool handoff=checkpoint.bellwetherBranchView==BellwetherBranchView::RegisterHandoff || returnsToKit;
        if(handoff && !gameSimulation.config_.bellwetherRecordKeeperAdmitted)return finalize(false);
        if(returnsToKit && !gameSimulation.config_.bellwetherTavernKeeperAdmitted)return finalize(false);
        if(!chapterTerrain || checkpoint.bellwetherStations!=junction+1)
            return finalize(false);
        campaign::Checkpoint record;
        record.id=campaign::CheckpointId::BellwetherTavern;
        record.zone=campaign::Destination(record.id);
        record.contentRevision=campaign::kBellwetherContentRevision;
        record.schema=campaign::kTavernConversationSchema;
        if(rigTrial) {
            // Stand in for the artificer's future hand-off in this Debug entry point.
            record.schema=campaign::kMillPreparationSchema;
            record.millPreparation=MillPreparation::RigTrialReady;
        }
        record.position=campaign::kBellwetherCheckpointPosition;
        record.firstAscentCompleted=true;
        record.swordStowed=true;
        record.dialogue=completedForestProgress();
        record.dialogue.hearthLinesResolved=dialogue::kHearthLines.size();
        record.dialogue.tavernConversationLinesResolved=dialogue::kTavernConversation.size();
        InputSnapshot input;
        input.damageEnabled=false;input.tutorialEnabled=false;
        input.yawRadians=record.yaw;input.pitchRadians=record.pitch;
        input.commands.interact=gameSimulation.latestInteractSequence_;
        input.commands.dialogueSkip=gameSimulation.chapterSkipFloor_;
        const auto token=gameSimulation.BeginCampaignRestore(record);
        if(!token || !gameSimulation.CommitCampaignRestore(*token,{token->worldGeneration,record.zone},chapterTerrain,
                input,gameSimulation.Snapshot().inputPublicationSequence))return finalize(false);
        for(std::size_t zone=1;zone<kWorldZones.size();++zone)
            if(!gameSimulation.PublishChapterTerrain({token->worldGeneration,static_cast<WorldZoneId>(zone)},chapterTerrain))
                return finalize(false);
        const auto& state=gameSimulation.Snapshot();
        const auto advance=[&] {
            stepFixed(input,1.f/60.f,state.inputPublicationSequence+1);
            accumulateEvents();
            gameSimulation.ClearEvents();
        };
        advance();
        const auto walkStation=[&](const auto& point){return walkTo(input,advance,point.x,point.z);};
        const auto& route=traversal::kBellwetherTavernRoute;
        for(std::size_t i=route.size();i>junction;--i)
            if(!walkStation(route[i-1]))return finalize(false);
        if(riverLanding) {
            if(!walkStation(traversal::kBellwetherMillApproach[1]) || !walkStation(traversal::kBellwetherSalvagerMark))
                return finalize(false);
            for(const auto& station:traversal::kBellwetherRiverLanding)
                if(!walkStation(station))return finalize(false);
            if(!state.waterContact.wetGround || !walkStation(traversal::kBellwetherRiverLandingView) ||
               state.waterContact.wetGround)return finalize(false);
        } else if(millYard) {
            const auto& mill=traversal::kBellwetherMillApproach;
            for(auto station=mill.begin()+1;station!=mill.end();++station)
                if(!walkStation(*station))return finalize(false);
            if(!walkStation(traversal::kBellwetherMillWorkMark) || !walkStation(mill.back()))
                return finalize(false);
            if(!walkStation(traversal::kBellwetherMillYardView))return finalize(false);
        } else {
            for(const auto& station:traversal::kBellwetherNorthApproach)
                if(!walkStation(station))return finalize(false);
            if(checkpoint.bellwetherBranchView==BellwetherBranchView::Church) {
                for(const auto& station:traversal::kBellwetherChurchApproach)
                    if(!walkStation(station))return finalize(false);
            } else {
                for(const auto& station:traversal::kBellwetherWaysideApproach)
                    if(!walkStation(station))return finalize(false);
            }
        }
        input.moveForward=0;input.runHeld=false;
        input.yawRadians=handoff?0.f:checkpoint.yaw;
        input.pitchRadians=handoff?-.12f:checkpoint.pitch;
        for(unsigned tick=0;tick<120;++tick)advance();
        if(east || checkpoint.bellwetherBranchView==BellwetherBranchView::Church)
            return finalize(state.playerGrounded &&
                state.chapterCompanion.bellwetherArrival==dialogue::BellwetherArrivalStage::Seated &&
                (!east || std::hypot(state.playerX-checkpoint.cameraX,state.playerZ-checkpoint.cameraZ)<.02f));
        if(state.waysideArchiveAction!=ArchiveAction::Inspect)return finalize(false);
        if(checkpoint.bellwetherBranchView==BellwetherBranchView::Archive)return finalize(true);
        // Stage through the same handlers without consuming sequence numbers
        // reserved for the next live interact or skip from a platform publisher.
        gameSimulation.InteractWithClaimedChapter();advance();
        if(state.chapterDialogue.line!=dialogue::Line::LanternFirstHelp)return finalize(false);
        gameSimulation.chapterDialogue_.Skip();advance();
        if(state.waysideArchiveAction!=ArchiveAction::Open)return finalize(false);
        gameSimulation.InteractWithClaimedChapter();advance();
        for(unsigned tick=0;tick<70 && state.waysideArchive.stage!=ArchiveStage::Open;++tick)advance();
        if(state.waysideArchive.stage!=ArchiveStage::Open ||
           state.chapterDialogue.line!=dialogue::Line::LanternFirstIdentity)return finalize(false);
        if(handoff || checkpoint.bellwetherBranchView==BellwetherBranchView::RegisterRecovered) {
            for(const auto line:{dialogue::Line::LanternKingName,dialogue::Line::LanternFirstRequest,dialogue::Line::None}) {
                gameSimulation.chapterDialogue_.Skip();advance();
                if(state.chapterDialogue.line!=line)return finalize(false);
            }
            if(state.waysideArchiveAction!=ArchiveAction::TakeRegister)return finalize(false);
            gameSimulation.InteractWithClaimedChapter();advance();
            const auto saved=gameSimulation.LatestCampaignBoundary();
            if(!state.waysideArchive.registerRecovered || !saved || !saved->waysideRegisterRecovered ||
               state.chapterDialogue.line!=dialogue::Line::LanternRegisterReturn)return finalize(false);
        }
        if(handoff) {
            gameSimulation.chapterDialogue_.Skip();advance();
            const auto& wayside=traversal::kBellwetherWaysideApproach;
            for(std::size_t i=wayside.size()-1;i>0;--i)
                if(!walkStation(wayside[i-1]))return finalize(false);
            const auto approach=returnsToKit?traversal::kBellwetherRecordKeeperApproach:
                traversal::BellwetherStation{checkpoint.cameraX,checkpoint.cameraZ};
            if(!walkStation(traversal::kBellwetherNorthApproach.back()) ||
               !walkStation(traversal::kBellwetherChurchApproach.front()) ||
               !walkStation(approach))return finalize(false);
            input.moveForward=0;input.runHeld=false;
            input.yawRadians=returnsToKit?1.57079632679f:checkpoint.yaw;
            input.pitchRadians=returnsToKit?-.07f:checkpoint.pitch;
            advance();
            if(state.recordKeeperAction!=dialogue::RecordKeeperAction::ShowRegister || !state.recordKeeper)
                return finalize(false);
            gameSimulation.InteractWithClaimedChapter();advance();
            const auto saved=gameSimulation.LatestCampaignBoundary();
            if(!saved || !saved->waysideRegisterInterpreted || state.chapterDialogue.line!=dialogue::Line::RecordKeeperRegister)
                return finalize(false);
            if(!returnsToKit && approach.z!=traversal::kBellwetherRecordKeeperApproach.z) {
                const auto& foot=state.recordKeeper->foot;
                const float facing=std::atan2(state.playerX-foot[0],foot[2]-state.playerZ);
                for(unsigned tick=0;tick<40 && std::abs(state.recordKeeper->yaw-facing)>.01f;++tick)advance();
                if(std::abs(state.recordKeeper->yaw-facing)>.01f)return finalize(false);
            }
        }
        if(returnsToKit) {
            for(const auto line:{dialogue::Line::RecordKeeperTwoRoads,dialogue::Line::RecordKeeperRoadOrder,dialogue::Line::None}) {
                gameSimulation.chapterDialogue_.Skip();advance();
                if(state.chapterDialogue.line!=line)return finalize(false);
            }
            if(!walkStation(traversal::kBellwetherChurchApproach.front()))return finalize(false);
            const auto& north=traversal::kBellwetherNorthApproach;
            for(auto station=north.rbegin();station!=north.rend();++station)
                if(!walkStation(*station))return finalize(false);
            for(std::size_t i=traversal::kBellwetherChurchJunction+1;i<route.size();++i)
                if(!walkStation(route[i]))return finalize(false);
            if(!walkTo(input,advance,checkpoint.cameraX,checkpoint.cameraZ))return finalize(false);
            input.moveForward=0;input.runHeld=false;
            input.yawRadians=checkpoint.yaw;input.pitchRadians=checkpoint.pitch;
            advance();
            if(!state.NeedsRaiseLanternPrompt() || !state.tavernKeeper || !state.recordKeeper)return finalize(false);
            input.commands.toggleHeldLightPose=state.lastConsumedToggleHeldLightPoseSequence+1;
            for(unsigned tick=0;tick<90 && state.interaction.heldLightPose!=horde::gameplay::interactions::HeldLightPose::High;++tick)
                advance();
            // The Windows capture must present this pose before the exchange starts.
            if(state.interaction.heldLightPose!=horde::gameplay::interactions::HeldLightPose::High ||
               state.chapterDialogue.line!=dialogue::Line::None || state.chapterDialogue.tavernLanternLinesStarted)
                return finalize(false);
        }
        return finalize(state.lastConsumedInteractSequence==input.commands.interact &&
            gameSimulation.chapterSkipFloor_==input.commands.dialogueSkip &&
            state.chapterCompanion.bellwetherArrival==dialogue::BellwetherArrivalStage::Seated);
    }
    const bool doorClosed=checkpoint.bellwetherStaffedView==BellwetherStaffedView::DoorClosed;
    const bool repeatVisit=checkpoint.bellwetherStaffedView==BellwetherStaffedView::RepeatVisit || doorClosed;
    if (checkpoint.bellwetherStaffedView != BellwetherStaffedView::None && !repeatVisit) {
        using namespace simulation;
        if (!chapterTerrain || !gameSimulation.config_.bellwetherTavernKeeperAdmitted)
            return finalize(false);
        // Seed an already-earned tavern save, then use the ordinary restore
        // transaction and input edges. The full approach is tested separately.
        campaign::Checkpoint record;
        record.schema = campaign::kTavernConversationSchema;
        record.contentRevision = campaign::kBellwetherContentRevision;
        record.id = campaign::CheckpointId::BellwetherTavern;
        record.zone = campaign::Destination(record.id);
        record.firstAscentCompleted = true;
        record.position = campaign::kBellwetherCheckpointPosition;
        record.yaw = checkpoint.yaw;
        record.pitch = checkpoint.pitch;
        record.swordStowed = true;
        record.dialogue = completedForestProgress();
        record.dialogue.hearthLinesResolved = static_cast<std::uint8_t>(dialogue::kHearthLines.size());
        InputSnapshot input;
        input.damageEnabled = false;
        input.tutorialEnabled = false;
        input.yawRadians = record.yaw;
        input.pitchRadians = record.pitch;
        const auto token = gameSimulation.BeginCampaignRestore(record);
        if (!token || !gameSimulation.CommitCampaignRestore(*token, {token->worldGeneration, record.zone},
                chapterTerrain, input, gameSimulation.Snapshot().inputPublicationSequence))
            return finalize(false);
        const auto advance = [&] {
            stepFixed(input, 1.f/60.f, gameSimulation.Snapshot().inputPublicationSequence+1);
            accumulateEvents();
            gameSimulation.ClearEvents();
        };
        advance();
        if (!gameSimulation.Snapshot().tavernConversationAvailable) return finalize(false);
        input.commands.interact = gameSimulation.Snapshot().lastConsumedInteractSequence + 1;
        advance();
        if (gameSimulation.Snapshot().chapterDialogue.line != dialogue::Line::TavernIntroduce)
            return finalize(false);
        if (checkpoint.bellwetherStaffedView == BellwetherStaffedView::LanternGlance) {
            for (int tick=0; tick<124; ++tick) advance();
            const auto& staged=gameSimulation.Snapshot();
            return finalize(staged.chapterDialogue.line==dialogue::Line::TavernIntroduce &&
                staged.chapterCompanion.bellwetherArrival==dialogue::BellwetherArrivalStage::Seated &&
                staged.chapterCompanion.motion.sitPlaybackSeconds==dialogue::kTavernSitDurationSeconds &&
                std::abs(staged.chapterCompanion.acting.headYaw)>.1f &&
                staged.chapterCompanion.acting.chestPitch==0.f);
        }
        ++input.commands.dialogueSkip;
        advance();
        const auto& reply = gameSimulation.Snapshot();
        if (!reply.tavernKeeper || reply.tavernKeeper->stage != dialogue::TavernKeeperStage::AtHearth ||
            reply.chapterDialogue.line != dialogue::Line::TavernHordeArmy ||
            reply.chapterDialogue.source.x != reply.tavernKeeper->foot[0] ||
            reply.chapterCompanion.bellwetherArrival != dialogue::BellwetherArrivalStage::Seated)
            return finalize(false);
        if (checkpoint.bellwetherStaffedView == BellwetherStaffedView::KeeperReply) return finalize(true);
        if (!walkTo(input, advance, checkpoint.cameraX, checkpoint.cameraZ)) return finalize(false);
        input.moveForward = 0;
        input.runHeld = false;
        input.yawRadians = checkpoint.yaw;
        input.pitchRadians = checkpoint.pitch;
        advance();
        return finalize(gameSimulation.Snapshot().chapterDialogue.line == dialogue::Line::None &&
            gameSimulation.Snapshot().tavernKeeper.has_value());
    }

    if(checkpoint.bellwetherSitTicks!=0 || checkpoint.bellwetherAnkleTicks!=0) {
        using namespace simulation;
        using namespace dialogue;
        if(repeatVisit && !gameSimulation.config_.bellwetherTavernKeeperAdmitted)return finalize(false);
        DevelopmentCheckpointStageEvidence::BellwetherVisit visit;
        const auto reportVisit=[&] {
            if(repeatVisit && evidence) evidence->bellwetherVisit=visit;
        };
        reportVisit();
        const bool ankle=checkpoint.bellwetherAnkleTicks!=0;
        const auto stations=ankle ? traversal::kBellwetherAnkleStation+1 : traversal::kBellwetherTavernRoute.size();
        const float ankleSeconds=checkpoint.bellwetherAnkleTicks/60.f;
        if(!chapterTerrain || checkpoint.bellwetherSitTicks>120 || checkpoint.bellwetherStations!=stations ||
           ankleSeconds>=dialogue::kAnkleAssessmentDurationSeconds)return finalize(false);
        // Model the public 1.7 continuation with the ordinary validated save
        // transaction. Both actors then travel the route through fixed steps.
        campaign::Checkpoint record;
        record.contentRevision=2;record.id=campaign::CheckpointId::ForestEndpoint;
        record.zone=WorldZoneId::Lookout;record.firstAscentCompleted=true;
        const auto lookout=kWorldRoutePoints.back();
        const auto support=traversal::ChapterExteriorSupport(chapterTerrain.get(),lookout.x,lookout.z);
        if(!support.grounded)return finalize(false);
        record.position={lookout.x,support.worldY,lookout.z};record.yaw=2.51f;record.pitch=-.15f;
        record.swordStowed=repeatVisit;
        record.dialogue=completedForestProgress();
        InputSnapshot input;
        input.damageEnabled=false;input.tutorialEnabled=false;
        input.yawRadians=record.yaw;input.pitchRadians=record.pitch;
        const auto token=gameSimulation.BeginCampaignRestore(record);
        if(!token || !gameSimulation.CommitCampaignRestore(*token,{token->worldGeneration,record.zone},chapterTerrain,
                input,gameSimulation.Snapshot().inputPublicationSequence))return finalize(false);
        for(std::size_t zone=1;zone<kWorldZones.size();++zone)
            if(!gameSimulation.PublishChapterTerrain({token->worldGeneration,static_cast<WorldZoneId>(zone)},chapterTerrain))
                return finalize(false);
        bool returningQuietly=false,replayedCaption=false;
        const auto advance=[&] {
            stepFixed(input,1.f/60.f,gameSimulation.Snapshot().inputPublicationSequence+1);
            accumulateEvents();gameSimulation.ClearEvents();
            if(repeatVisit) {
                const auto line=gameSimulation.Snapshot().chapterDialogue.line;
                if(visit.roadsideCaptions<kRoadLines.size() && line==kRoadLines[visit.roadsideCaptions])
                    ++visit.roadsideCaptions;
                visit.welcomeCaption|=line==Line::TavernWelcome;
                if(visit.exchangeCaptions<kTavernConversation.size() && line==kTavernConversation[visit.exchangeCaptions])
                    ++visit.exchangeCaptions;
                replayedCaption|=returningQuietly && IsBellwetherLine(line);
                reportVisit();
                if(stepObservation && stepObservation->presentBellwetherVisitStep)
                    routePresentationFailed=!stepObservation->presentBellwetherVisitStep(stepObservation->user);
            }
        };
        advance(); // Release the restored input barrier with an exact neutral publication.
        const auto& route=traversal::kBellwetherTavernRoute;
        const auto walkStation=[&](std::size_t index) {
            return walkTo(input,advance,route[index].x,route[index].z);
        };
        const auto waitFor=[&](const auto& ready,unsigned maxTicks) {
            input.moveForward=0;input.runHeld=false;
            for(unsigned tick=0;tick<maxTicks && !routePresentationFailed && !ready();++tick)advance();
            return !routePresentationFailed && ready();
        };
        for(std::size_t index=0;index<stations;++index) {
            if(!walkStation(index))return finalize(false);
            if(repeatVisit && index==traversal::kBellwetherAnkleStation &&
               !waitFor([&]{return visit.roadsideCaptions==kRoadLines.size();},3600))return finalize(false);
            if(repeatVisit && index==traversal::kBellwetherSouthDoorStation &&
               !waitFor([&]{return visit.welcomeCaption;},3600))return finalize(false);
        }
        if(repeatVisit) {
            const auto hearth=campaign::kBellwetherCheckpointPosition;
            const auto walkToHearth=[&] {return walkTo(input,advance,hearth.x,hearth.z);};
            const auto& state=gameSimulation.Snapshot();
            const auto conversationSaved=[&] {
                const auto saved=gameSimulation.LatestCampaignBoundary();
                return saved && saved->id==campaign::CheckpointId::BellwetherTavern &&
                    saved->dialogue.tavernConversationLinesResolved==kTavernConversation.size();
            };
            if(!walkToHearth() || !waitFor([&]{return state.tavernConversationAvailable;},5400))return finalize(false);
            input.commands.interact=state.lastConsumedInteractSequence+1;
            advance();
            if(!waitFor(conversationSaved,1800) || visit.exchangeCaptions!=kTavernConversation.size())
                return finalize(false);

            if(doorClosed) {
                for(std::size_t index=route.size();index>traversal::kBellwetherSouthDoorStation;--index)
                    if(!walkStation(index-1))return finalize(false);
                const auto sill=route[traversal::kBellwetherSouthDoorStation];
                if(!walkTo(input,advance,sill.x,sill.z+.29f))return finalize(false);
                input.moveForward=0;input.runHeld=false;
                input.yawRadians=-.41f;
                input.pitchRadians=checkpoint.pitch;
                advance();
                if(state.tavernDoorAction!=bellwether::DoorAction::Close)return finalize(false);
                input.commands.interact=state.lastConsumedInteractSequence+1;
                advance();
                if(!waitFor([&]{return state.tavernDoor.opening==0.f;},100))return finalize(false);
                if(!walkTo(input,advance,checkpoint.cameraX,checkpoint.cameraZ))return finalize(false);
                input.moveForward=0;input.runHeld=false;
                input.yawRadians=checkpoint.yaw;
                input.pitchRadians=checkpoint.pitch;
                advance();
                return finalize(conversationSaved() && state.tavernDoorAction==bellwether::DoorAction::Open);
            }

            // Retrace the mapped portal from the hearth to the square and back.
            // Use one campaign session throughout and check that Directions is offered on both returns.
            constexpr auto square=traversal::kBellwetherSouthApproachStation-1;
            returningQuietly=true;
            for(unsigned trip=0;trip<2;++trip) {
                for(std::size_t index=route.size();index>square;--index)
                    if(!walkStation(index-1))return finalize(false);
                if(!state.playerGrounded || state.playerZ<=route[traversal::kBellwetherSouthDoorStation].z)
                    return finalize(false);
                for(std::size_t index=square+1;index<route.size();++index)
                    if(!walkStation(index))return finalize(false);
                if(!walkToHearth())return finalize(false);
                input.moveForward=0;input.runHeld=false;
                advance();
                if(!conversationSaved() || replayedCaption || state.tavernConversationAvailable ||
                   !state.tavernDirectionsAvailable || state.chapterDialogue.line!=Line::None ||
                   state.chapterCompanion.bellwetherArrival!=BellwetherArrivalStage::Seated ||
                   !state.tavernKeeper || state.tavernKeeper->stage!=TavernKeeperStage::AtHearth)
                    return finalize(false);
                ++visit.quietReturns;
                reportVisit();
            }
            if(!walkTo(input,advance,checkpoint.cameraX,checkpoint.cameraZ))return finalize(false);
            input.moveForward=0;input.runHeld=false;
            input.yawRadians=checkpoint.yaw;input.pitchRadians=checkpoint.pitch;
            advance();
            return finalize(conversationSaved() && !replayedCaption && state.chapterDialogue.line==Line::None);
        }
        if(!walkTo(input,advance,checkpoint.cameraX,checkpoint.cameraZ))return finalize(false);
        input.moveForward=0;input.runHeld=false;
        input.yawRadians=checkpoint.yaw;input.pitchRadians=checkpoint.pitch;
        const float wanted=std::min(checkpoint.bellwetherSitTicks/60.f,dialogue::kTavernSitDurationSeconds);
        for(unsigned tick=0;tick<5400;++tick) {
            const auto& kit=gameSimulation.Snapshot().chapterCompanion;
            if(ankle) {
                if(kit.bellwetherArrival==dialogue::BellwetherArrivalStage::AnkleAssessment &&
                   kit.motion.anklePlaybackSeconds+.00001f>=ankleSeconds)
                    return finalize(kit.grounded && !kit.walking);
                advance();
                continue;
            }
            if(kit.motion.sitPlaybackSeconds+.00001f>=wanted) {
                if(!gameSimulation.Snapshot().chapterDialogue.forestVillageConsumed)return finalize(false);
                if(wanted<dialogue::kTavernSitDurationSeconds)return finalize(true);
                const auto earned=gameSimulation.LatestCampaignBoundary();
                if(!earned || earned->id!=campaign::CheckpointId::BellwetherTavern)return finalize(false);
                const auto reload=gameSimulation.BeginCampaignRestore(*earned);
                if(!reload || !gameSimulation.CommitCampaignRestore(*reload,{reload->worldGeneration,earned->zone},
                        chapterTerrain,input,gameSimulation.Snapshot().inputPublicationSequence))return finalize(false);
                advance();
                return finalize(gameSimulation.Snapshot().chapterCompanion.bellwetherArrival==
                    dialogue::BellwetherArrivalStage::Seated);
            }
            advance();
        }
        return finalize(false);
    }

    if(checkpoint.combatFeedbackCapture) {
        if(checkpoint.combatFeedbackCapture>5u || !gameSimulation.config_.combatFoundation1_7 ||
           !gameSimulation.config_.specialSkeletonAttacks1_7)return finalize(false);
        const bool hit=checkpoint.combatFeedbackCapture==5u;
        const bool red=checkpoint.combatFeedbackCapture==3u || checkpoint.combatFeedbackCapture==4u;
        const bool contact=checkpoint.combatFeedbackCapture%2u==0u;
        gameSimulation.BeginCombatPractice(red?EnemyKind::Lich:EnemyKind::Skeleton);
        simulation::InputSnapshot cueInput;
        cueInput.damageEnabled=false;cueInput.tutorialEnabled=false;
        cueInput.hasAuthoritativePlayerPose=true;
        cueInput.authoritativePlayerX=checkpoint.cameraX;cueInput.authoritativePlayerZ=checkpoint.cameraZ;
        cueInput.yawRadians=checkpoint.yaw;cueInput.pitchRadians=checkpoint.pitch;
        stepFixed(cueInput,0.f,gameSimulation.Snapshot().inputPublicationSequence+1u);
        cueInput.hasAuthoritativePlayerPose=false;
        for(unsigned tick=0;tick<1800u;++tick) {
            if(hit && tick%45u==0u)cueInput.commands.attack=gameSimulation.Snapshot().lastConsumedAttackSequence+1u;
            stepFixed(cueInput,1.f/60.f,gameSimulation.Snapshot().inputPublicationSequence+1u);
            if(hit && gameSimulation.Snapshot().enemyHitParticles.Active()) {
                for(unsigned settle=0;settle<6u;++settle)
                    stepFixed(cueInput,1.f/60.f,gameSimulation.Snapshot().inputPublicationSequence+1u);
                return finalize(gameSimulation.Snapshot().enemyHitParticles.Active());
            }
            if(hit)continue;
            const auto& warning=gameSimulation.Snapshot().attackWarning;
            if(warning.kind==(red?EnemyAttackKind::Unblockable:EnemyAttackKind::GuardBreak) &&
               warning.phase==(contact?simulation::AttackWarningPhase::Contact:simulation::AttackWarningPhase::Windup) &&
               warning.phaseElapsedSeconds >= (contact?.025f:warning.phaseDurationSeconds*.5f))
                return finalize(true);
        }
        return finalize(false);
    }

    if(checkpoint.stagesRescueExterior) {
        // A diagnostic exterior seed is a new authored chapter presentation,
        // even in a process which has already walked this route. Retaining the
        // first-arrival flag skips the genuine lowering step on repeated seeds;
        // retaining Kit/proof would also borrow the preceding run's progression.
        // Ordinary checkpoint/retry/save paths deliberately keep their policy.
        gameSimulation.chapterDialogue_.Reset();
        gameSimulation.dialogueDucking_.Reset();
        gameSimulation.chapterCompanion_.Reset();
        gameSimulation.chapterFirstExterior_=false;
        gameSimulation.chapterVisibleRaise_=0;
        // Explicit companion evidence first executes the accepted-hit/death
        // seed, then the same rope/native-grounding path as ordinary captures.
        // Historical route captures retain their existing imported state.
        if(checkpoint.keeperDeathFixedTicks != 0u) {
            const auto* death=FindDevelopmentCheckpoint("chapter-keeper-death-retired");
            if(!death || !StageDevelopmentCheckpointSimulation(gameSimulation,*death)) return finalize(false);
        }
        using namespace interactions;
        ChestRewardSnapshot chest;chest.phase=ChestRewardPhase::LanternClaimed;chest.lidOpenProgress=1;
        InteractionState interaction;ResetInteractionState(interaction,HeldLightKind::RewardLantern);
        FinaleSequenceSnapshot finale;finale.lichDefeated=true;
        gameSimulation.ImportRewardCheckpoint(chest,interaction,finale);
        for(std::size_t i=1;i<simulation::kWorldZones.size();++i)
            if(!gameSimulation.PublishChapterTerrain({gameSimulation.worldRoute_.generation,
                static_cast<simulation::WorldZoneId>(i)},chapterTerrain))return finalize(false);
        simulation::InputSnapshot routeInput;routeInput.damageEnabled=false;routeInput.tutorialEnabled=false;
        routeInput.yawRadians=checkpoint.yaw;routeInput.pitchRadians=checkpoint.pitch;
        const auto advance=[&](){
            stepFixed(routeInput,1.0f/60.0f,gameSimulation.Snapshot().inputPublicationSequence+1);
            // Diagnostic exterior preparation can cover thousands of ticks in
            // one owner call. Observe and retain evidence before draining each
            // step, as live owners do; never erase a recorded overflow count.
            // Combat checkpoint staging retains its existing finalizer path.
            accumulateEvents();
            gameSimulation.ClearEvents();
        };
        for(int tick=0;tick<420&&!gameSimulation.Snapshot().rescue.ropeReady;++tick)advance();
        if(!gameSimulation.Snapshot().rescue.ropeReady)return finalize(false);
        routeInput.commands.interact=gameSimulation.Snapshot().lastConsumedInteractSequence+1;advance();
        if(!gameSimulation.Snapshot().rescue.equipmentStowed)return finalize(false);
        const auto freezeTraversal=[&]() {
            stepFixed(routeInput,0,gameSimulation.Snapshot().inputPublicationSequence+1);
            return finalize(gameSimulation.Snapshot().rescue.equipmentStowed);
        };
        if(checkpoint.rescueTraversalCapture==1u || checkpoint.rescueTraversalCapture==2u) {
            for(int tick=0;tick<1200 && gameSimulation.Snapshot().rescue.equipmentStowed;++tick) {
                const auto& s=gameSimulation.Snapshot().rescue;
                const bool match=checkpoint.rescueTraversalCapture==1u
                    ?s.phase==traversal::Phase::Ascent&&s.supportWorldY>.25f
                    :s.phase==traversal::Phase::PullUp&&s.plantedPelvisDropMetres>.35f&&s.ropeHandsActive;
                if(match)return freezeTraversal();
                advance();
            }
            return finalize(false);
        }
        for(int tick=0;tick<1200&&gameSimulation.Snapshot().rescue.equipmentStowed;++tick)advance();
        if(!gameSimulation.Snapshot().rescue.exteriorSide || gameSimulation.Snapshot().rescue.equipmentStowed)return finalize(false);
        if(checkpoint.rescueTraversalCapture==3u || checkpoint.rescueTraversalCapture==4u) {
            routeInput.commands.interact=gameSimulation.Snapshot().lastConsumedInteractSequence+1;advance();
            if(!gameSimulation.Snapshot().rescue.equipmentStowed)return finalize(false);
            for(int tick=0;tick<1200 && gameSimulation.Snapshot().rescue.equipmentStowed;++tick) {
                const auto& s=gameSimulation.Snapshot().rescue;
                if(checkpoint.rescueTraversalCapture==3u && s.phase==traversal::Phase::Descent&&s.ropeHandsActive)
                    return freezeTraversal();
                advance();
            }
            if(checkpoint.rescueTraversalCapture==3u || gameSimulation.Snapshot().rescue.exteriorSide)
                return finalize(false);
            routeInput.commands.interact=gameSimulation.Snapshot().lastConsumedInteractSequence+1;advance();
            for(int tick=0;tick<1200 && gameSimulation.Snapshot().rescue.equipmentStowed;++tick) {
                const auto& s=gameSimulation.Snapshot().rescue;
                if(s.phase==traversal::Phase::Ascent&&s.supportWorldY>.25f)return freezeTraversal();
                advance();
            }
            return finalize(false);
        }
        if(checkpoint.rescueTraversalCapture!=0u)return finalize(false);
        // Capture route stations through real fixed-step movement and native
        // support/collision after ascent, never a teleport through blockers.
        if(checkpoint.exteriorRouteStation>=simulation::kWorldRoutePoints.size() ||
           checkpoint.bellwetherStations>traversal::kBellwetherTavernRoute.size())return finalize(false);
        for(std::size_t station=3;station<=checkpoint.exteriorRouteStation;++station) {
            const auto destination=simulation::kWorldRoutePoints[station];
            if(!walkTo(routeInput,advance,destination.x,destination.z))return finalize(false);
        }
        // A mapped approach capture continues from F04 using the same motor;
        // the camera target is never used as an authoritative teleport.
        if(checkpoint.walksBeyondExteriorStation && !walkTo(routeInput,advance,checkpoint.cameraX,checkpoint.cameraZ))
            return finalize(false);
        for(std::size_t station=0;station<checkpoint.bellwetherStations;++station) {
            const auto destination=traversal::kBellwetherTavernRoute[station];
            if(!walkTo(routeInput,advance,destination.x,destination.z))return finalize(false);
        }
        routeInput.moveForward=0;routeInput.runHeld=false;
        // The native replay really ascends before freezing. Never seed a
        // camera-only exterior or invent ready resources for a missing scene.
        routeInput.yawRadians=checkpoint.yaw;routeInput.pitchRadians=checkpoint.pitch;
        // Route shots are taken after stopping and looking at the bend. Let
        // the physical lantern and locomotion pose settle in simulation time;
        // render-only settling frames cannot advance those owners.
        for(int tick=0;tick<120;++tick)advance();
        if(checkpoint.raisesLanternForCompanion) {
            routeInput.commands.toggleHeldLightPose=gameSimulation.Snapshot().lastConsumedToggleHeldLightPoseSequence+1;
            for(int tick=0;tick<100;++tick) {
                advance();
                const auto& state=gameSimulation.Snapshot();
                // Diagnostic acknowledgement of the exact current raised pose;
                // this is not evidence of an owner completing the reunion.
                gameSimulation.AcknowledgeChapterPresentation(state.tickIndex,state.chapterDialogue.generation);
            }
            if(!gameSimulation.Snapshot().chapterDialogue.proofAccepted) return finalize(false);
        }
        stepFixed(routeInput,0,gameSimulation.Snapshot().inputPublicationSequence+1);
        return finalize(std::hypot(gameSimulation.Snapshot().playerX-checkpoint.cameraX,
            gameSimulation.Snapshot().playerZ-checkpoint.cameraZ)<.01f);
    }

    simulation::InputSnapshot input;
    input.damageEnabled = false;
    input.hasAuthoritativePlayerPose = true;
    input.authoritativePlayerX = checkpoint.cameraX;
    input.authoritativePlayerZ = checkpoint.cameraZ;
    input.yawRadians = checkpoint.yaw;
    input.pitchRadians = checkpoint.pitch;
    input.torchLightStrength = 1.8f;
    stepFixed(input, 0.0f,
              gameSimulation.Snapshot().inputPublicationSequence + 1u);
    if (checkpoint.keeperDeathFixedTicks != 0u)
    {
        if (!gameSimulation.config_.developmentRescueJourney ||
            checkpoint.keeperDeathFixedTicks > 360u) return finalize(false);
        // Explicit diagnostic setup, not evidence of a player-completed fight.
        // Preserve legacy checkpoint imports. This separate seed uses live
        // recognition, accepted-hit lockout and the actual death clock.
        gameSimulation.legacyCombatCheckpoint_ = false;
        auto& keeper = gameSimulation.lichEncounter_;
        keeper.SetReadableCombat(true);
        keeper.SetChapterDeathPresentation(true);
        keeper.BeginRetryRecognition();
        constexpr float tickSeconds = 1.0f / 60.0f;
        for (int tick = 0; tick < 180 && !keeper.Snapshot().revealComplete; ++tick)
            keeper.Update(tickSeconds, checkpoint.cameraX, checkpoint.cameraZ, false, true);
        keeper.Update(tickSeconds, checkpoint.cameraX, checkpoint.cameraZ, false, true);
        for (int hit = 0; hit < 3; ++hit)
        {
            if (!keeper.TryAcceptPlayerHit(keeper.Snapshot().x, keeper.Snapshot().z))
                return finalize(false);
            if (hit < 2)
                for (int tick = 0; tick < 120; ++tick)
                    keeper.Update(tickSeconds, checkpoint.cameraX, checkpoint.cameraZ, false, true);
        }
        if (gameSimulation.finaleSequence_.NotifyLichDefeated())
            gameSimulation.chestRewardSequence_.BeginUnlockCountdown();
        input.tutorialEnabled = false;
        for (std::uint32_t tick = 0; tick < checkpoint.keeperDeathFixedTicks; ++tick)
            stepFixed(input, tickSeconds, gameSimulation.Snapshot().inputPublicationSequence + 1u);
        if(gameSimulation.Snapshot().lich.phase != LichPhase::Dead)return finalize(false);
        // Explicit composed diagnostic: retain the real death clock while
        // importing a separately requested frozen reward-hand pose below.
        // Existing death-only views keep their original early completion.
        if(checkpoint.rewardPose==DevelopmentRewardPose::None)return finalize(true);
    }
    if (checkpoint.stagesUnlockedChest)
    {
        using namespace horde::gameplay::interactions;
        ChestRewardSnapshot chest;
        chest.phase = ChestRewardPhase::ClosedUnlocked;
        InteractionState interaction = gameSimulation.Snapshot().interaction;
        FinaleSequenceSnapshot finale;
        finale.phase = FinaleSequencePhase::LichFalling;
        finale.endingPhase = FinaleEndingPhase::LichFalling;
        finale.lichDefeated = true;
        gameSimulation.ImportRewardCheckpointState(chest, interaction, finale, nullptr,
            simulation::GameSimulation::RewardCheckpointPlacement::AuthoredCapture);
    }
    if (checkpoint.rewardPose != DevelopmentRewardPose::None)
    {
        using namespace horde::gameplay::interactions;
        ChestRewardSnapshot chest;
        chest.phase = ChestRewardPhase::LanternClaimed;
        chest.lidOpenProgress = 1.0f;
        InteractionState interaction;
        interaction.heldLightKind = HeldLightKind::RewardLantern;
        interaction.heldLightPose = checkpoint.rewardPose == DevelopmentRewardPose::HeldLow
            ? HeldLightPose::Low
            : HeldLightPose::High;
        interaction.heldLightPoseProgress = 1.0f;
        FinaleSequenceSnapshot finale;
        finale.phase = FinaleSequencePhase::RevealingLantern;
        finale.endingPhase = FinaleEndingPhase::LichFalling;
        finale.lichDefeated = true;
        finale.lanternClaimed = true;

        // Resolve the exact high/low shared hand target before authoring the
        // frozen body state beneath it. The final import then preserves those
        // finite angles/velocities without advancing a simulation tick.
        gameSimulation.ImportRewardCheckpointState(chest, interaction, finale, nullptr,
            simulation::GameSimulation::RewardCheckpointPlacement::AuthoredCapture);
        LanternPendulumSnapshot pendulum;
        const auto& hinge = gameSimulation.Snapshot().rewardLanternWorldFromHinge;
        pendulum.initialized = true;
        pendulum.previousPivotPosition = {{hinge[12], hinge[13], hinge[14]}};
        pendulum.forwardAngleRadians = checkpoint.rewardForwardAngleRadians;
        pendulum.strafeAngleRadians = checkpoint.rewardStrafeAngleRadians;
        pendulum.forwardAngularVelocity = checkpoint.rewardForwardAngularVelocity;
        pendulum.strafeAngularVelocity = checkpoint.rewardStrafeAngularVelocity;
        pendulum.torsionAngleRadians = checkpoint.rewardTorsionAngleRadians;
        pendulum.torsionAngularVelocity = checkpoint.rewardTorsionAngularVelocity;
        pendulum.previousPivotVelocity = checkpoint.rewardPreviousPivotVelocity;
        pendulum.previousHandForward = {{hinge[8], hinge[9], hinge[10]}};
        pendulum.worldFromBody = ComposeLanternPendulumBodyTransform(
            hinge, pendulum.forwardAngleRadians, pendulum.strafeAngleRadians,
            pendulum.torsionAngleRadians,
            gameSimulation.Snapshot().heldItemKinematics.
                rewardLanternPresentationYawRadians);
        gameSimulation.ImportRewardCheckpointState(chest, interaction, finale, &pendulum,
            simulation::GameSimulation::RewardCheckpointPlacement::AuthoredCapture);
    }
    constexpr float fixedDelta =
        static_cast<float>(simulation::FixedStepRunner::kFixedDeltaSeconds);
    if (checkpoint.stagesWaterfallGuards)
    {
        for (std::uint32_t tick = 0u; tick < checkpoint.waterfallGuardFixedTicks; ++tick)
            stepFixed(input, fixedDelta,
                      gameSimulation.Snapshot().inputPublicationSequence + 1u);
        const auto& state = gameSimulation.Snapshot();
        const bool valid = state.skeletonEnemyCount == 2u && state.activeSkeletonCount == 2u &&
            state.skeletonEnemies[0].id == simulation::EntityId::SkeletonA &&
            state.skeletonEnemies[1].id == simulation::EntityId::SkeletonB &&
            state.skeletonEnemies[0].health == 1 && state.skeletonEnemies[1].health == 1 &&
            IsWaterfallSkeletonRoom(state.playerX, state.playerZ);
        if (!valid) gameSimulation.EndMotionEvidenceEquipmentSeed();
        // Successful staging deliberately remains scoped until the capture owner
        // retires it, so the frozen RT snapshot retains the real guard poses.
        return finalize(valid);
    }
    if (checkpoint.combatPose == DevelopmentCombatPose::Rest)
        return finalize(true);

    if (checkpoint.combatPose == DevelopmentCombatPose::ParryActive)
    {
        // Drive the same monotonic parry command and fixed-step combat path as
        // live play, then freeze the first 60 Hz sample at/after 0.10 s into
        // the 0.22 s active window (the authored sample is 0.11 s).
        input.commands.parry =
            gameSimulation.Snapshot().lastConsumedParrySequence + 1u;
        constexpr float kParryCaptureActionTimeSeconds = 0.10f;
        for (std::uint32_t tick = 0u; tick < 20u; ++tick)
        {
            stepFixed(input, fixedDelta,
                      gameSimulation.Snapshot().inputPublicationSequence + 1u);
            const PlayerCombatSnapshot& after = gameSimulation.Snapshot().playerCombat;
            if (after.action == PlayerCombatAction::ParryActive &&
                after.actionTime >= kParryCaptureActionTimeSeconds)
                return finalize(true);
        }
        return finalize(false);
    }

    input.commands.attack = gameSimulation.Snapshot().lastConsumedAttackSequence + 1u;
    bool upwardEdgePublished = false;
    for (std::uint32_t tick = 0u; tick < 90u; ++tick)
    {
        const PlayerCombatSnapshot& before = gameSimulation.Snapshot().playerCombat;
        if (checkpoint.combatPose == DevelopmentCombatPose::UpwardSliceActive &&
            !upwardEdgePublished && before.action == PlayerCombatAction::SwingActive &&
            before.actionTime >= 0.05f)
        {
            ++input.commands.attack;
            upwardEdgePublished = true;
        }
        stepFixed(input, fixedDelta,
                  gameSimulation.Snapshot().inputPublicationSequence + 1u);
        const PlayerCombatSnapshot& after = gameSimulation.Snapshot().playerCombat;
        const bool reachedDownward =
            checkpoint.combatPose == DevelopmentCombatPose::DownwardCutActive &&
            after.action == PlayerCombatAction::SwingActive &&
            after.actionTime >= SwordCombat::kSwingActiveDuration - 0.025f;
        const bool reachedUpward =
            checkpoint.combatPose == DevelopmentCombatPose::UpwardSliceActive &&
            after.action == PlayerCombatAction::UpwardSliceActive &&
            after.actionTime >= SwordCombat::kUpwardSliceActiveDuration - 0.025f;
        if (reachedDownward || reachedUpward)
            return finalize(true);
    }
    return finalize(false);
}

// Platform owner-thread boundary. A publisher consumes actual scene readiness;
// the returned result must gate capture/benchmark admission, not just its label.
template<class PublishReadyWorld>
[[nodiscard]] inline bool StageDevelopmentCheckpointForScene(
    simulation::GameSimulation& gameSimulation,
    const DevelopmentCheckpoint& checkpoint,
    PublishReadyWorld&& publishReadyWorld,
    bool liveJourney = false,
    DevelopmentCheckpointStageEvidence* evidence = nullptr,
    const DevelopmentCheckpointStepFixedObservation* stepObservation = nullptr)
{
    // Early platform-boundary rejection must not retain a prior successful
    // capture's events, identities or failure disposition.
    if (evidence != nullptr) *evidence = {};
    const bool chapter = DevelopmentCheckpointRequiresAuthoredChapter(checkpoint);
    if (checkpoint.id == 192)
    {
        if (liveJourney)
            gameSimulation.StartLiveDevelopmentRescueJourney(true);
        else
        {
            if (!gameSimulation.ApplyShowcaseCheckpoint(checkpoint.baseShowcaseCheckpointId))
            {
                if (evidence != nullptr)
                    evidence->failure = DevelopmentCheckpointStageFailure::BaseCheckpointRejected;
                return false;
            }
            gameSimulation.SetDevelopmentRescueJourney(true, true);
        }
        publishReadyWorld();
        return true; // The live lower safe start does not require exterior readiness yet.
    }
    if (chapter)
    {
        // Explicit checkpoint import changes the generation. Publish actual
        // renderer readiness only afterwards; old callbacks cannot populate it.
        gameSimulation.SetDevelopmentRescueJourney(true, true);
        publishReadyWorld();
    }
    else if (gameSimulation.Snapshot().developmentRescueJourney)
        gameSimulation.SetDevelopmentRescueJourney(false);
    return StageDevelopmentCheckpointSimulation(gameSimulation, checkpoint, evidence, stepObservation);
}

} // namespace horde::gameplay
