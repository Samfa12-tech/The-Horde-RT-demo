#include "gameplay/simulation/GameSimulation.h"
#include "gameplay/dialogue/ResidentYaw.h"
#include "gameplay/traversal/BellwetherEastRoute.h"
#include "gameplay/traversal/BellwetherNorthRoute.h"
#include "gameplay/traversal/BellwetherWaymarker.h"
#include "audio/KitAnkleAudio.h"
#include "../../../assets/models/world/source/bellwether-road-cache-v1/geometry.generated.h"
#include "../../../assets/models/world/source/bellwether-privy-v1/geometry.generated.h"
#include "../../../assets/models/world/source/bellwether-privy-rag-v1/geometry.generated.h"
#include "../../../assets/models/world/source/bellwether-grate-v1/geometry.generated.h"
#include "../../../assets/models/world/source/bellwether-tally-v1/geometry.generated.h"
#include <cmath>
namespace horde::gameplay::simulation {
namespace {
std::optional<std::array<float,3>> PlaceBellwetherResident(const TriangleSupportSurface& terrain,
    traversal::BellwetherStation mark) {
    constexpr float radius=.28f,height=1.78f;
    const auto support=traversal::ChapterExteriorStepSupport(terrain,mark.x,mark.z,radius,height);
    if(!support.grounded || !traversal::ResolveChapterStepMovement(terrain,mark.x,mark.z,mark.x,mark.z,
            support.worldY,radius,height).accepted)
        return std::nullopt;
    return std::array{mark.x,support.worldY,mark.z};
}

bool AtRiverLeakTest(const dialogue::Context& c) {
    constexpr auto station=traversal::kBellwetherRiverLanding.back();
    return std::hypot(c.player.x-station.x,c.player.z-station.z)<=.5f;
}

bool OnRiverPracticeLanding(float x,float z) {
    constexpr auto wet=traversal::kBellwetherRiverLanding.back();
    constexpr auto dry=traversal::kBellwetherRiverDryMark;
    return x>=dry.x-.75f && x<=wet.x+.5f && std::abs(z-dry.z)<=.6f;
}
} // namespace

bool GameSimulation::SquareWaymarkerReadable(const dialogue::Context& c, float yaw) const {
    return config_.developmentRescueJourney && config_.authoredChapterTerrain &&
        !legacyCombatCheckpoint_ && c.exterior && c.alive && !c.paused && !c.combat &&
        !c.traversing && playerSupport_.grounded && chapterTerrain_ &&
        chapterTerrainGeneration_==worldRoute_.generation &&
        chapterDialogue_.State().line==dialogue::Line::None &&
        bellwether::SquareWaymarkerReadableFrom(c.player.x,c.listener.y,c.player.z,yaw);
}

void GameSimulation::EnsureBellwetherResidentPoses(bool requireAuthoredTerrain) {
    if(!chapterTerrain_ || chapterTerrainGeneration_!=worldRoute_.generation)return;
    const auto& kit=chapterCompanion_.State();
    if(kit.bellwetherArrival!=dialogue::BellwetherArrivalStage::Seated)return;
    if((requireAuthoredTerrain || config_.bellwetherCombinedResidents) &&
       (!config_.developmentRescueJourney || !config_.authoredChapterTerrain || !kit.grounded ||
        !rescueTraversal_.Snapshot().exteriorSide || rescueTraversal_.IsActive()))return;
    if(!recordKeeper_ && (config_.bellwetherCombinedResidents ? IsBellwetherResidentReady(traversal::kRecordKeeperResident) :
            config_.bellwetherSpecialist==bellwether::Specialist::RecordKeeper)) {
        if(const auto foot=PlaceBellwetherResident(*chapterTerrain_,traversal::kBellwetherRecordKeeperMark))
            recordKeeper_=dialogue::RecordKeeperPose{*foot};
    }
    if(!artificer_ && (config_.bellwetherCombinedResidents ? IsBellwetherResidentReady(traversal::kArtificerResident) :
            config_.bellwetherSpecialist==bellwether::Specialist::Artificer)) {
        if(const auto foot=PlaceBellwetherResident(*chapterTerrain_,traversal::kBellwetherMillArtificerMark))
            artificer_=bellwether::ArtificerPose{*foot};
    }
    if(!salvager_ && (config_.bellwetherCombinedResidents ? IsBellwetherResidentReady(traversal::kSalvagerResident) :
            config_.bellwetherSpecialist==bellwether::Specialist::Salvager)) {
        if(const auto foot=PlaceBellwetherResident(*chapterTerrain_,traversal::kBellwetherSalvagerStand))
            salvager_=bellwether::SalvagerPose{*foot};
    }
    if(!relicDealer_ && IsBellwetherResidentReady(traversal::kRelicDealerResident)) {
        if(const auto foot=PlaceBellwetherResident(*chapterTerrain_,traversal::kBellwetherRelicDealerStand))
            relicDealer_=bellwether::RelicDealerPose{*foot};
    }
}

void GameSimulation::InteractWithClaimedChapter() {
    if(TryRescueInteraction() || TryFlaskUpgradeInteraction())return;
    // Interact runs before movement. Use the same camera input as this edge
    // so turning away cannot activate the prompt from the previous tick.
    const auto context=ChapterInteractionContext(false,lastInput_.yawRadians);
    if(const auto rag=CurrentPrivyRagAction(context);rag!=bellwether::PrivyRagAction::None) {
        InteractWithPrivyRag(rag,context);
        return;
    }
    const auto door=TavernDoorAction(context);
    if(door!=bellwether::DoorAction::None) {
        tavernDoor_.destination=door==bellwether::DoorAction::Open?1.f:0.f;
        return;
    }
    if(chapterDialogue_.TryStartGardenGrateTalk(context))return;
    if(IsBellwetherTavernKeeperReady() && chapterDialogue_.TryStartTavernConversation(context))return;
    if(chapterDialogue_.TryStartKitTreasureTalk(context))return;
    if(const auto tally=CurrentMillTallyAction(context);tally!=campaign::MillTallyAction::None) {
        InteractWithMillTally(tally);
        return;
    }
    if(CanBuyLanternFitting(context)) {
        BuyLanternFitting(context);
        return;
    }
    if(const auto mill=CurrentMillAction(context);mill!=bellwether::MillAction::None) {
        InteractWithMill(mill);
        return;
    }
    if(const auto salvager=CurrentSalvagerAction(context);salvager!=bellwether::SalvagerAction::None) {
        InteractWithSalvager(salvager,context);
        return;
    }
    if(const auto silver=CurrentOrdinarySilverAction(context);silver!=campaign::OrdinarySilverAction::None) {
        InteractWithOrdinarySilver(silver);
        return;
    }
    if(chapterDialogue_.TryStartRelicDealer(context))return;
    const auto records=RecordKeeperAction(context);
    if(records==dialogue::RecordKeeperAction::CollectReward) {
        ClaimRegisterReward(context);
        return;
    }
    if(chapterDialogue_.TryStartRecordKeeper(records,context)) {
        if(records==dialogue::RecordKeeperAction::ShowRegister)
            waysideArchive_.registerInterpreted=true;
        if(records==dialogue::RecordKeeperAction::LearnAirWard)
            waysideArchive_.airWardLearned=true;
        return;
    }
    switch(WaysideArchiveAction(context)) {
    case bellwether::ArchiveAction::Inspect:
        if(chapterDialogue_.TryStartShrineSpeech(dialogue::ShrineSpeech::Help,context))
            waysideArchive_.stage=bellwether::ArchiveStage::Inspected;
        return;
    case bellwether::ArchiveAction::Open:
        waysideArchive_.stage=bellwether::ArchiveStage::Opening;
        return;
    case bellwether::ArchiveAction::Listen:
        TryStartShrineReveal(context);
        return;
    case bellwether::ArchiveAction::TakeRegister:
        waysideArchive_.registerRecovered=true;
        chapterDialogue_.TryStartShrineSpeech(dialogue::ShrineSpeech::RegisterReturn,context);
        return;
    case bellwether::ArchiveAction::Recap:
        chapterDialogue_.TryStartShrineSpeech(dialogue::ShrineSpeech::Recap,context);
        return;
    case bellwether::ArchiveAction::RestoreBinding:
        waysideArchive_.stage=bellwether::ArchiveStage::Closing;
        return;
    default:
        using dialogue::TavernGuidance;
        const auto guidance=waysideArchive_.registerInterpreted?TavernGuidance::Expeditions:
            waysideArchive_.registerRecovered?TavernGuidance::Register:TavernGuidance::Shrine;
        chapterDialogue_.ShowTavernDirections(context,guidance);
    }
    // The physical fingerpost is a local caption, not a speech line. Keep it
    // behind every existing campaign/service interaction in this dispatch.
    if(SquareWaymarkerReadable(context,context.yaw))
        squareWaymarkerTextVisible_=bellwether::StepSquareWaymarkerTextVisible(
            true,false,false,true,squareWaymarkerTextVisible_);
}

campaign::MillTallyAction GameSimulation::CurrentMillTallyAction(const dialogue::Context& c) const {
    using campaign::MillTallyAction;
    using campaign::MillTallyProgress;
    if(!config_.bellwetherTrade || !campaignBoundary_ || campaignBoundaryRevision_==UINT64_MAX ||
       !c.claimed || !c.alive || !c.exterior || c.paused || c.combat || c.traversing || !c.kitSeated ||
       !playerSupport_.grounded || !chapterTerrain_ || chapterTerrainGeneration_!=worldRoute_.generation ||
       chapterDialogue_.State().line!=dialogue::Line::None ||
       campaignBoundary_->dialogue.tavernConversationLinesResolved!=dialogue::kTavernConversation.size())
        return MillTallyAction::None;
    const auto faces=[&](const std::array<float,3>& target) {
        const float dx=target[0]-c.player.x,dz=target[2]-c.player.z;
        const float distance=std::hypot(dx,dz);
        return distance<=1.35f && std::abs(c.listener.y-target[1])<=1.45f &&
            std::sin(c.yaw)*dx-std::cos(c.yaw)*dz>=.84f*distance &&
            chapterTerrain_->NonWalkableVolumeClear(c.player.x,c.player.z,target[0],target[2],
                target[1]-.025f,target[1]+.025f,.025f);
    };
    const auto progress=treasurePurse_.millTally;
    if(progress==MillTallyProgress::Unfound)
        return faces(bellwether::kMillTallyCaseReach)?MillTallyAction::Take:MillTallyAction::None;
    if(c.relicDealerSource && (progress==MillTallyProgress::ClaimRecorded || treasurePurse_.CanSellMillTally()) &&
       faces(bellwether::kMillTallyTrayReach))
        return progress==MillTallyProgress::ClaimRecorded?MillTallyAction::Recover:MillTallyAction::Sell;
    if((progress==MillTallyProgress::Carried || progress==MillTallyProgress::Recovered ||
        progress==MillTallyProgress::Sold) && faces(bellwether::kMillTallyBoardReach))
        return progress==MillTallyProgress::Sold?MillTallyAction::RecordClaim:MillTallyAction::Return;
    return MillTallyAction::None;
}

campaign::OrdinarySilverAction GameSimulation::CurrentOrdinarySilverAction(const dialogue::Context& c) const {
    using campaign::OrdinarySilverAction;
    using campaign::OrdinarySilverProgress;
    if(!config_.bellwetherTrade || !campaignBoundary_ ||
       campaignBoundaryRevision_==UINT64_MAX || !c.exterior || !c.claimed || !c.alive ||
       c.combat || c.paused || c.traversing || !c.kitSeated || !playerSupport_.grounded ||
       !chapterTerrain_ || chapterTerrainGeneration_!=worldRoute_.generation ||
       chapterDialogue_.State().tavernConversationLinesResolved!=dialogue::kTavernConversation.size())
        return OrdinarySilverAction::None;
    if(chapterDialogue_.CanStartRelicDealer(c)) {
        if(treasurePurse_.CanSellOrdinarySilver())return OrdinarySilverAction::Sell;
        if(treasurePurse_.ordinarySilver==OrdinarySilverProgress::Sold)return OrdinarySilverAction::Sold;
    }
    if(!treasurePurse_.CanCollectOrdinarySilver())return OrdinarySilverAction::None;
    const auto& mark=bellwether::kRoadSilverCacheReach;
    const float dx=mark[0]-c.player.x,dz=mark[2]-c.player.z;
    const float distanceSquared=dx*dx+dz*dz;
    const float facing=std::sin(c.yaw)*dx-std::cos(c.yaw)*dz;
    if(distanceSquared>4.f || std::abs(c.listener.y-mark[1])>1.35f || facing<0.f ||
       facing*facing<.65f*.65f*distanceSquared ||
       !chapterTerrain_->NonWalkableVolumeClear(c.player.x,c.player.z,mark[0],mark[2],
           mark[1]-.025f,mark[1]+.025f,.025f))
        return OrdinarySilverAction::None;
    return OrdinarySilverAction::Take;
}

bellwether::PrivyRagAction GameSimulation::CurrentPrivyRagAction(const dialogue::Context& c) const {
    using bellwether::PrivyRagAction;
    if(!config_.developmentRescueJourney || !config_.authoredChapterTerrain ||
       !config_.combatFoundation1_7 || legacyCombatCheckpoint_ || !campaignBoundary_ ||
       campaignBoundary_->privyRagDelivered ||
       campaignBoundary_->contentRevision<campaign::kBellwetherContentRevision ||
       campaignBoundary_->dialogue.tavernConversationLinesResolved!=dialogue::kTavernConversation.size() ||
       chapterDialogue_.State().tavernConversationLinesResolved!=dialogue::kTavernConversation.size() ||
       !c.claimed || !c.alive || !c.exterior || c.paused || c.combat || c.traversing || !c.kitSeated ||
       !playerSupport_.grounded || !chapterTerrain_ || chapterTerrainGeneration_!=worldRoute_.generation ||
       !chapterDialogue_.CanInteractPrivyRag(c) || privyRagState_==bellwether::PrivyRagState::Delivered)
        return PrivyRagAction::None;
    const bool taking=privyRagState_==bellwether::PrivyRagState::Available;
    if(!taking && campaignBoundaryRevision_==UINT64_MAX)return PrivyRagAction::None;
    const auto& mark=taking?bellwether::kPrivyRagBasket:bellwether::kPrivyRagShelf;
    constexpr float interactionHeight=.09f;
    const float targetY=mark[1]+interactionHeight;
    const float dx=mark[0]-c.player.x,dz=mark[2]-c.player.z;
    const float distance=std::hypot(dx,dz);
    const float facing=std::sin(c.yaw)*dx-std::cos(c.yaw)*dz;
    if(distance>2.f || std::abs(c.listener.y-targetY)>1.35f || facing<.65f*distance ||
       !chapterTerrain_->NonWalkableVolumeClear(c.player.x,c.player.z,mark[0],mark[2],
           targetY-.025f,targetY+.025f,.025f))
        return PrivyRagAction::None;
    return taking?PrivyRagAction::Take:PrivyRagAction::Pass;
}

void GameSimulation::InteractWithPrivyRag(bellwether::PrivyRagAction action,const dialogue::Context& context) {
    using bellwether::PrivyRagAction;
    using bellwether::PrivyRagState;
    if(action==PrivyRagAction::Take) {
        if(CurrentPrivyRagAction(context)==PrivyRagAction::Take && privyRagState_==PrivyRagState::Available)
            privyRagState_=PrivyRagState::Carried;
        return;
    }
    if(action!=PrivyRagAction::Pass || CurrentPrivyRagAction(context)!=PrivyRagAction::Pass ||
       privyRagState_!=PrivyRagState::Carried || !chapterDialogue_.CanInteractPrivyRag(context))return;
    const auto updated=PreparePrivyRagDeliveredCheckpoint();
    if(!updated)return;
    CommitPrivyRagDeliveredCheckpoint(*updated);
    constexpr auto& mouth=bellwether::kBellwetherPrivyMouth;
    chapterDialogue_.TryStartPrivyRagThanks(context,dialogue::Tuple{mouth[0],mouth[1],mouth[2]});
}

bellwether::SalvagerAction GameSimulation::CurrentSalvagerAction(const dialogue::Context& c) const {
    using namespace bellwether;
    if(!IsBellwetherResidentReady(traversal::kSalvagerResident) || !campaignBoundary_ ||
       campaignBoundaryRevision_==UINT64_MAX || !chapterDialogue_.CanStartSalvager(c))
        return SalvagerAction::None;
    if(salvagerShorePracticeCompleted_)return SalvagerAction::RecallPractice;
    if(shorePracticeStage_==ShorePracticeStage::ReportBack)return SalvagerAction::ReportPractice;
    return shorePracticeStage_==ShorePracticeStage::Idle?SalvagerAction::StartPractice:SalvagerAction::RecallPractice;
}

void GameSimulation::InteractWithSalvager(bellwether::SalvagerAction action,const dialogue::Context& c) {
    using namespace bellwether;
    using dialogue::Line;
    if(action==SalvagerAction::None)return;
    if(action==SalvagerAction::ReportPractice) {
        if(CommitShorePractice())chapterDialogue_.TryStartSalvager(Line::SalvagerShoreReturn,c);
    } else if(action==SalvagerAction::StartPractice) {
        if(chapterDialogue_.TryStartSalvager(Line::SalvagerShoreTask,c))
            shorePracticeStage_=ShorePracticeStage::FindDryMark;
    } else {
        chapterDialogue_.TryStartSalvager(salvagerShorePracticeCompleted_?
            Line::SalvagerShoreReturn:Line::SalvagerShoreReminder,c);
    }
}

void GameSimulation::AdvanceShorePractice() {
    using bellwether::ShorePracticeStage;
    if(shorePracticeStage_==ShorePracticeStage::Idle)return;
    if(!salvager_ || !UsesRescueExterior() || rescueTraversal_.IsActive() ||
       chapterTerrainGeneration_!=worldRoute_.generation || playerVitals_.Snapshot().phase!=PlayerLifePhase::Alive) {
        shorePracticeStage_=ShorePracticeStage::Idle;
        shorePracticeBreathTicksRemaining_=0;
        return;
    }
    constexpr auto dry=traversal::kBellwetherRiverDryMark;
    constexpr auto wet=traversal::kBellwetherRiverLanding.back();
    const bool wetGround=playerSupport_.grounded && waterContact_.Snapshot().wetGround;
    const bool atDry=playerSupport_.grounded && !wetGround && std::hypot(playerX_-dry.x,playerZ_-dry.z)<=.48f;
    const bool atWet=wetGround && std::hypot(playerX_-wet.x,playerZ_-wet.z)<=.40f;
    switch(shorePracticeStage_) {
    case ShorePracticeStage::FindDryMark:
    case ShorePracticeStage::RecoverBreath:
        if(atDry)shorePracticeStage_=ShorePracticeStage::EnterShallows;
        return;
    case ShorePracticeStage::EnterShallows:
        if(atDry) {
            // Turning back before reaching the rail is a safe restart.
            shorePracticeBreathTicksRemaining_=0;
            return;
        }
        if(millLeakTestPhase_!=bellwether::MillLeakTestPhase::Idle)return;
        if(!shorePracticeBreathTicksRemaining_) {
            if(wetGround && OnRiverPracticeLanding(playerX_,playerZ_)) {
                shorePracticeBreathTicksRemaining_=bellwether::kShorePracticeBreathTicks;
                if(atWet)shorePracticeStage_=ShorePracticeStage::Retreat;
            }
            return;
        }
        if(atWet)shorePracticeStage_=ShorePracticeStage::Retreat;
        break;
    case ShorePracticeStage::Retreat:
        if(atDry) {
            shorePracticeStage_=ShorePracticeStage::ReportBack;
            shorePracticeBreathTicksRemaining_=0;
            return;
        }
        break;
    default: return;
    }
    // The active rehearsal owns a fixed number of simulation ticks, even if
    // movement stalls or the player steps off the course. Reaching safe dry
    // footing on the last available tick succeeds before the next breath tick.
    if(--shorePracticeBreathTicksRemaining_==0)
        shorePracticeStage_=ShorePracticeStage::RecoverBreath;
}

bool GameSimulation::CanWorkAtMill(const dialogue::Context& c) const {
    return c.claimed && c.exterior && c.alive && !c.paused && !c.traversing && !c.combat &&
        c.kitSeated && playerSupport_.grounded && chapterTerrain_ &&
        chapterTerrainGeneration_==worldRoute_.generation && campaignBoundary_ &&
        campaignBoundaryRevision_!=UINT64_MAX &&
        campaignBoundary_->dialogue.tavernConversationLinesResolved==dialogue::kTavernConversation.size() &&
        chapterDialogue_.State().line==dialogue::Line::None;
}

bool GameSimulation::ChapterTargetInReach(const dialogue::Context& c,const std::array<float,3>& mark,
    float reach,float facingCosine) const {
    const float dx=mark[0]-c.player.x,dz=mark[2]-c.player.z;
    const float distance=std::hypot(dx,dz);
    return distance<=reach && std::abs(c.listener.y-mark[1])<=1.1f &&
        std::sin(c.yaw)*dx-std::cos(c.yaw)*dz>=facingCosine*distance &&
        chapterTerrain_->NonWalkableVolumeClear(c.player.x,c.player.z,mark[0],mark[2],
            mark[1]-.02f,mark[1]+.02f,.025f);
}

bellwether::MillAction GameSimulation::CurrentMillAction(const dialogue::Context& c) const {
    using namespace bellwether;
    if(!CanWorkAtMill(c) || millPreparation_==MillPreparation::RigTested || millDriveTurnSeconds_)
        return MillAction::None;
    if(millPreparation_==MillPreparation::GearingVerified) {
        if(!IsBellwetherResidentReady(traversal::kArtificerResident) || !artificer_)return MillAction::None;
        auto speaker=artificer_->foot;
        speaker[1]+=1.37f;
        return ChapterTargetInReach(c,speaker,2.5f)?MillAction::TakeRig:MillAction::None;
    }
    if(millPreparation_==MillPreparation::FitChecked || millPreparation_==MillPreparation::BearingSeated) {
        if(!IsBellwetherResidentReady(traversal::kArtificerResident) || !artificer_ ||
           !ChapterTargetInReach(c,traversal::kMillDriveWork,1.75f))return MillAction::None;
        return millPreparation_==MillPreparation::FitChecked?MillAction::SeatBearing:MillAction::TurnDrive;
    }
    if(millPreparation_==MillPreparation::RigTrialReady) {
        const bool wet=waterContact_.Snapshot().wetGround;
        constexpr auto dry=traversal::kBellwetherRiverDryMark;
        // An active breath attempt owns the shared cue. Once it expires the
        // player can still choose the rig at the rail instead of retrying.
        const bool practisingBreath=shorePracticeStage_==ShorePracticeStage::EnterShallows ||
            shorePracticeStage_==ShorePracticeStage::Retreat;
        if(millLeakTestPhase_==MillLeakTestPhase::Idle && wet && AtRiverLeakTest(c) && !practisingBreath)
            return MillAction::StartLeakTest;
        if(millLeakTestPhase_==MillLeakTestPhase::ReturnToDry && !wet &&
           std::hypot(c.player.x-dry.x,c.player.z-dry.z)<=.55f)
            return MillAction::ConfirmSeal;
        return MillAction::None;
    }
    const bool atBin=millPreparation_==MillPreparation::BearingNeeded;
    const auto& mark=atBin?kMillBearingBin:kMillComparison;
    if(!ChapterTargetInReach(c,mark,1.75f))return MillAction::None;
    if(atBin)return MillAction::RecoverBearing;
    return millPreparation_==MillPreparation::Unexamined?MillAction::InspectGearing:MillAction::CheckFit;
}

void GameSimulation::AdvanceMillDriveTest(float seconds) {
    if(!millDriveTurnSeconds_)return;
    const auto c=ChapterInteractionContext(false);
    if(!CanWorkAtMill(c) || !IsBellwetherResidentReady(traversal::kArtificerResident) ||
       !artificer_ || !ChapterTargetInReach(c,traversal::kMillDriveWork,1.75f)) {
        millDriveTurnSeconds_.reset();
        return;
    }
    *millDriveTurnSeconds_+=seconds;
    if(*millDriveTurnSeconds_+1.e-5f>=bellwether::kMillDriveTestSeconds) {
        CommitMillPreparation(bellwether::MillPreparation::GearingVerified);
        millDriveTurnSeconds_.reset();
    }
}

void GameSimulation::AdvanceMillLeakTest(float seconds) {
    using bellwether::MillLeakTestPhase;
    if(millLeakTestPhase_==MillLeakTestPhase::Idle)return;
    const auto c=ChapterInteractionContext(false);
    const bool onLanding=OnRiverPracticeLanding(c.player.x,c.player.z);
    const bool testing=millLeakTestPhase_==MillLeakTestPhase::Testing;
    if(!c.alive || !c.exterior || c.traversing || c.combat || !playerSupport_.grounded || !onLanding ||
       (testing && (!waterContact_.Snapshot().wetGround || !AtRiverLeakTest(c)))) {
        millLeakTestPhase_=MillLeakTestPhase::Idle;
        millLeakTestSeconds_=0.f;
        return;
    }
    if(testing) {
        millLeakTestSeconds_+=seconds;
        if(millLeakTestSeconds_+1.e-5f>=bellwether::kMillLeakTestSeconds)
            millLeakTestPhase_=MillLeakTestPhase::ReturnToDry;
    }
}

dialogue::RecordKeeperAction GameSimulation::RecordKeeperAction(const dialogue::Context& c) const {
    using Action=dialogue::RecordKeeperAction;
    if(!IsBellwetherResidentReady(traversal::kRecordKeeperResident) || !chapterDialogue_.CanStartRecordKeeper(c))return Action::None;
    if(waysideArchive_.registerInterpreted) {
        if(campaignBoundary_ && campaignBoundary_->waysideArchive==campaign::WaysideArchiveProgress::Restored &&
           !waysideArchive_.airWardLearned && campaignBoundaryRevision_!=UINT64_MAX)
            return Action::LearnAirWard;
        if(campaignBoundary_ && campaignBoundary_->waysideRegisterInterpreted &&
           campaignBoundaryRevision_!=UINT64_MAX && treasurePurse_.CanClaimRegisterReward())
            return Action::CollectReward;
        return Action::RecallRoutes;
    }
    return waysideArchive_.registerRecovered?Action::ShowRegister:Action::ShowRoadMark;
}

bellwether::ArchiveAction GameSimulation::WaysideArchiveAction(const dialogue::Context& c) const {
    using namespace bellwether;
    const auto stage=waysideArchive_.stage;
    if(!c.shrineLanternSource || c.paused || chapterDialogue_.State().line!=dialogue::Line::None ||
       stage==ArchiveStage::Opening || stage==ArchiveStage::Closing)
        return ArchiveAction::None;
    const float dx=kWaysideMechanism[0]-c.player.x,dz=kWaysideMechanism[2]-c.player.z;
    const float distance=std::hypot(dx,dz);
    // Leave room for the entire outward sweep of the lower stone.
    if(-dz<1.60f || distance>2.7f || std::abs(dx)>.8f ||
       (std::sin(c.yaw)*dx-std::cos(c.yaw)*dz)<.90f*distance ||
       std::abs(c.listener.y-kWaysideMechanism[1])>.9f) return ArchiveAction::None;
    const float y=kWaysideMechanism[1];
    if(!chapterTerrain_->NonWalkableVolumeClear(c.player.x,c.player.z,kWaysideMechanism[0],
            kWaysideMechanism[2]+.08f,y-.06f,y+.06f,.025f))return ArchiveAction::None;
    if(stage==ArchiveStage::Closed)return ArchiveAction::Inspect;
    if(stage==ArchiveStage::Inspected)return ArchiveAction::Open;
    if(stage==ArchiveStage::Restored)return ArchiveAction::Recap;
    if(!waysideArchive_.lanternRevealStarted)return ArchiveAction::Listen;
    if(campaignBoundary_ && campaignBoundary_->waysideRegisterInterpreted &&
       campaignBoundaryRevision_!=UINT64_MAX)return ArchiveAction::RestoreBinding;
    if(waysideArchive_.registerRecovered)return ArchiveAction::Recap;
    const auto& mark=kWaysideRoadRegister;
    if(!chapterTerrain_->NonWalkableVolumeClear(c.player.x,c.player.z,mark[0],mark[2],
            mark[1],mark[1]+.025f,.025f) ||
       !ArchiveContactClear(c.player.x,c.player.z,mark[0],mark[2],mark[1],mark[1]+.025f,.025f))
        return ArchiveAction::None;
    return ArchiveAction::TakeRegister;
}

void GameSimulation::AdvanceWaysideArchive(float seconds) {
    using namespace bellwether;
    const bool closing=waysideArchive_.stage==ArchiveStage::Closing;
    if((!closing && waysideArchive_.stage!=ArchiveStage::Opening) || seconds<=0 || !chapterTerrain_ ||
       chapterTerrainGeneration_!=worldRoute_.generation || !chapterTerrain_->HingedContact())return;
    const float progress=std::clamp(waysideArchive_.opening+(closing?-seconds:seconds),0.f,1.f);
    const auto candidate=ArchivePose::AtOpening(progress);
    // Expand the body query by the greatest point travel between poses.
    const float swingMargin=std::abs(progress-waysideArchive_.opening)*1.57079632679f*1.30f;
    if(UsesRescueExterior() && !candidate.VolumeClear(*chapterTerrain_->HingedContact(),
            playerX_,playerZ_,playerX_,playerZ_,playerSupport_.worldY+.001f,
            playerSupport_.worldY+posture_.Height(),kPlayerCollisionRadius+swingMargin))return;
    waysideArchive_.opening=progress;
    waysideArchive_.pose=candidate;
    if(closing && progress<=0.f)waysideArchive_.stage=ArchiveStage::Restored;
    else if(!closing && progress>=1.f) {
        waysideArchive_.stage=ArchiveStage::Open;
        TryStartShrineReveal(ChapterInteractionContext(false));
    }
}

bool GameSimulation::ArchiveContactClear(float x0,float z0,float x1,float z1,float y0,float y1,float radius) const {
    if(!chapterTerrain_ || chapterTerrainGeneration_!=worldRoute_.generation ||
       !chapterTerrain_->HingedContact())return true;
    return waysideArchive_.pose.VolumeClear(*chapterTerrain_->HingedContact(),x0,z0,x1,z1,y0,y1,radius);
}

bellwether::DoorAction GameSimulation::TavernDoorAction(const dialogue::Context& c) const {
    using namespace bellwether;
    if(!c.claimed || !c.exterior || !c.alive || c.paused || c.traversing || c.combat || !c.kitSeated ||
       !playerSupport_.grounded || !chapterTerrain_ || chapterTerrainGeneration_!=worldRoute_.generation ||
       !chapterTerrain_->HingedContact(TriangleSupportSurface::HingedPart::TavernDoor) ||
       (tavernKeeper_.Pose() && tavernKeeper_.Pose()->stage!=dialogue::TavernKeeperStage::AtHearth) ||
       chapterDialogue_.State().line!=dialogue::Line::None || tavernDoor_.opening!=tavernDoor_.destination)
        return DoorAction::None;
    const auto pose=TavernDoor::PoseAt(tavernDoor_.opening);
    const auto& m=pose.worldFromModel;
    constexpr float pullX=1.82f,pullY=1.12f;
    const float x=m[3]+m[0]*pullX,y=m[7]+pullY,z=m[11]+m[8]*pullX;
    const float dx=x-c.player.x,dz=z-c.player.z,distance=std::hypot(dx,dz);
    if(distance>3.f || std::abs(c.listener.y-y)>1.25f ||
       std::sin(c.yaw)*dx-std::cos(c.yaw)*dz<.65f*distance ||
       !chapterTerrain_->NonWalkableVolumeClear(c.player.x,c.player.z,x,z,y-.02f,y+.02f,.02f))
        return DoorAction::None;
    return tavernDoor_.opening==0.f?DoorAction::Open:DoorAction::Close;
}

void GameSimulation::AdvanceTavernDoor(float seconds) {
    using namespace bellwether;
    if(seconds<=0 || tavernDoor_.opening==tavernDoor_.destination || !chapterTerrain_ ||
       chapterTerrainGeneration_!=worldRoute_.generation)return;
    const auto* contact=chapterTerrain_->HingedContact(TriangleSupportSurface::HingedPart::TavernDoor);
    if(!contact)return;
    constexpr float duration=1.35f;
    const float delta=tavernDoor_.destination-tavernDoor_.opening;
    const float step=seconds/duration;
    const float next=std::abs(delta)<=step?tavernDoor_.destination:tavernDoor_.opening+std::copysign(step,delta);
    const float travel=std::abs(next-tavernDoor_.opening)*kSouthDoorOpenRadians*kSouthDoorSwingRadius;
    if(UsesRescueExterior() && !TavernDoor::PoseAt(next).VolumeClear(*contact,
            playerX_,playerZ_,playerX_,playerZ_,playerSupport_.worldY+.001f,
            playerSupport_.worldY+posture_.Height(),kPlayerCollisionRadius+travel))return;
    tavernDoor_.opening=next;
}

bool GameSimulation::TavernDoorContactClear(float x0,float z0,float x1,float z1,float y0,float y1,float radius) const {
    if(!chapterTerrain_ || chapterTerrainGeneration_!=worldRoute_.generation)return true;
    const auto* contact=chapterTerrain_->HingedContact(TriangleSupportSurface::HingedPart::TavernDoor);
    return !contact || bellwether::TavernDoor::PoseAt(tavernDoor_.opening).VolumeClear(*contact,x0,z0,x1,z1,y0,y1,radius);
}

void GameSimulation::TryStartShrineReveal(const dialogue::Context& context) {
    if(waysideArchive_.stage==bellwether::ArchiveStage::Open && !waysideArchive_.lanternRevealStarted &&
       chapterDialogue_.TryStartShrineSpeech(dialogue::ShrineSpeech::Reveal,context))
        waysideArchive_.lanternRevealStarted=true;
}

std::optional<dialogue::Tuple> GameSimulation::HeldRewardLanternSource() const {
    const auto& lantern=lanternPendulum_.Snapshot();
    if(rescueTraversal_.Snapshot().equipmentStowed ||
       interactionState_.heldLightKind!=interactions::HeldLightKind::RewardLantern || !lantern.initialized)
        return std::nullopt;
    const auto& body=lantern.worldFromBody;
    constexpr float lightBelowHinge=items::kRewardLanternLightBelowHinge*items::kClaimedRewardLanternScale;
    return dialogue::Tuple{body[12]-body[4]*lightBelowHinge,body[13]-body[5]*lightBelowHinge,
                           body[14]-body[6]*lightBelowHinge};
}

dialogue::Context GameSimulation::ChapterInteractionContext(bool paused,std::optional<float> facingYaw) const {
    using namespace horde::gameplay::dialogue;
    Context c;
    c.player={playerX_,playerSupport_.worldY,playerZ_};
    c.listener={playerX_,kShowcaseEyeWorldY+PlayerHeightDelta(playerSupport_.worldY)-posture_.drop,playerZ_};
    c.yaw=facingYaw && std::isfinite(*facingYaw)?*facingYaw:playerYawRadians_;c.paused=paused;
    c.alive=playerVitals_.Snapshot().phase==PlayerLifePhase::Alive;
    c.combat=combatSnapshot_.attackerIndex>=0 || lichEncounter_.Snapshot().phase==LichPhase::Charging;
    c.claimed=chestRewardSequence_.Snapshot().phase==interactions::ChestRewardPhase::LanternClaimed;
    c.exterior=config_.developmentRescueJourney&&rescueTraversal_.Snapshot().exteriorSide;
    c.traversing=config_.developmentRescueJourney&&rescueTraversal_.IsActive();
    const auto& kit=chapterCompanion_.State();
    c.kitAtChair=kit.grounded && kit.bellwetherArrival>=BellwetherArrivalStage::ChairTurn;
    c.kitSeated=c.kitAtChair && kit.bellwetherArrival==BellwetherArrivalStage::Seated;
    const auto hearthMark=campaign::kBellwetherCheckpointPosition;
    c.tavernInReach=c.kitAtChair && playerSupport_.grounded &&
        std::abs(playerSupport_.worldY-hearthMark.y)<.001f &&
        std::hypot(playerX_-hearthMark.x,playerZ_-hearthMark.z)<=1.75f;
    if(c.exterior && kit.grounded) {
        const float lowering=kit.motion.sitPlaybackSeconds/kTavernSitDurationSeconds;
        c.companionSource=Tuple{kit.x,kit.y+kCompanionSpeechHeight-.27f*lowering,kit.z};
    }
    if(c.tavernInReach && c.kitSeated && c.companionSource && chapterTerrain_ &&
       chapterTerrainGeneration_==worldRoute_.generation &&
       chapterDialogue_.State().tavernConversationLinesResolved==kTavernConversation.size()) {
        const auto& kitVoice=*c.companionSource;
        c.kitTreasureTopicInReach=ChapterTargetInReach(c,{kitVoice.x,kitVoice.y,kitVoice.z},2.65f,.93f);
    }
    if(c.tavernInReach && c.kitSeated && c.companionSource && waysideArchive_.lanternRevealStarted) {
        const auto source=HeldRewardLanternSource();
        if(source && std::hypot(source->x-c.companionSource->x,source->y-c.companionSource->y,
                               source->z-c.companionSource->z)<=4.f)
            c.tavernLanternSource=source;
    }
    const auto& keeper=tavernKeeper_.Pose();
    if(keeper && IsBellwetherTavernKeeperReady() && c.exterior && chapterTerrain_ &&
       chapterTerrainGeneration_==worldRoute_.generation && config_.combatFoundation1_7 && !legacyCombatCheckpoint_) {
        c.tavernKeeperSource=Tuple{keeper->foot[0],keeper->foot[1]+kCompanionSpeechHeight,keeper->foot[2]};
        c.keeperAtHearth=keeper->stage==TavernKeeperStage::AtHearth;
        if(c.keeperAtHearth && !c.tavernInReach && playerSupport_.grounded &&
           traversal::IsOnBellwetherTavernRoomFloor(playerX_,playerSupport_.worldY,playerZ_)) {
            const auto& source=*c.tavernKeeperSource;
            const float dx=source.x-c.player.x,dz=source.z-c.player.z;
            const float distance=std::hypot(dx,dz),height=std::min(c.listener.y,source.y);
            c.tavernFittingInReach=distance<=2.f &&
                (std::sin(c.yaw)*dx-std::cos(c.yaw)*dz)>=.65f*distance &&
                chapterTerrain_->NonWalkableVolumeClear(c.player.x,c.player.z,source.x,source.z,
                    height-.025f,height+.025f,.025f);
        }
    }
    if(recordKeeper_ && IsBellwetherResidentReady(traversal::kRecordKeeperResident) &&
       c.exterior && c.kitSeated && playerSupport_.grounded && chapterTerrain_ &&
       chapterTerrainGeneration_==worldRoute_.generation) {
        const auto& foot=recordKeeper_->foot;
        const Tuple source{foot[0],foot[1]+kCompanionSpeechHeight,foot[2]};
        const float dx=source.x-c.player.x,dz=source.z-c.player.z;
        const float distance=std::hypot(dx,dz),height=std::min(source.y,c.listener.y);
        if(distance<=4.f && std::abs(source.y-c.listener.y)<=1.2f &&
           chapterTerrain_->NonWalkableVolumeClear(c.player.x,c.player.z,source.x,source.z,
               height-.025f,height+.025f,.025f)) {
            c.recordKeeperSource=source;
            c.recordKeeperInReach=distance<=2.5f &&
                (std::sin(c.yaw)*dx-std::cos(c.yaw)*dz)>=.65f*distance;
        }
    }
    const auto observeShoreOrTradeResident=[&](const std::array<float,3>& foot,
            std::optional<Tuple>& source,bool& inReach) {
        if(!c.exterior || !c.kitSeated || !playerSupport_.grounded || !chapterTerrain_ ||
           chapterTerrainGeneration_!=worldRoute_.generation)return;
        auto mouth=foot;
        mouth[1]+=kCompanionSpeechHeight;
        const float distance=std::hypot(mouth[0]-c.player.x,mouth[2]-c.player.z);
        const float height=std::min(mouth[1],c.listener.y);
        if(distance<=4.f && std::abs(mouth[1]-c.listener.y)<=1.2f &&
           chapterTerrain_->NonWalkableVolumeClear(c.player.x,c.player.z,mouth[0],mouth[2],
               height-.025f,height+.025f,.025f)) {
            source=Tuple{mouth[0],mouth[1],mouth[2]};
            inReach=distance<=2.1f &&
                std::sin(c.yaw)*(mouth[0]-c.player.x)-std::cos(c.yaw)*(mouth[2]-c.player.z)>=.65f*distance;
        }
    };
    if(salvager_ && IsBellwetherResidentReady(traversal::kSalvagerResident))
        observeShoreOrTradeResident(salvager_->foot,c.salvagerSource,c.salvagerInReach);
    if(relicDealer_ && IsBellwetherResidentReady(traversal::kRelicDealerResident))
        observeShoreOrTradeResident(relicDealer_->foot,c.relicDealerSource,c.relicDealerInReach);
    const auto& mark=bellwether::kWaysideMechanism;
    if(config_.combatFoundation1_7 && !legacyCombatCheckpoint_ && c.alive && c.claimed && c.exterior &&
       !c.combat && !c.traversing && c.kitSeated && playerSupport_.grounded && chapterTerrain_ &&
       chapterTerrainGeneration_==worldRoute_.generation && chapterTerrain_->HingedContact() &&
       chapterDialogue_.State().hearthLinesResolved==kHearthLines.size() &&
       std::hypot(c.player.x-mark[0],c.player.z-mark[2])<=4.f && std::abs(c.listener.y-mark[1])<=1.5f)
        c.shrineLanternSource=HeldRewardLanternSource();
    if(config_.combatFoundation1_7 && !legacyCombatCheckpoint_ && config_.developmentRescueJourney &&
       config_.authoredChapterTerrain && c.exterior && c.kitSeated && playerSupport_.grounded &&
       chapterTerrain_ && chapterTerrainGeneration_==worldRoute_.generation) {
        const auto& listen=bellwether::kBellwetherGrateListenPoint;
        const float dx=c.player.x-listen[0],dz=c.player.z-listen[2];
        const float distance=std::hypot(dx,dz);
        if(dz>=.21f && distance<=2.5f && std::abs(c.listener.y-listen[1])<=1.2f &&
           chapterTerrain_->NonWalkableVolumeClear(c.player.x,c.player.z,listen[0],listen[2],
               listen[1]-.025f,listen[1]+.025f,.025f)) {
            const auto& mouth=bellwether::kBellwetherGrateMouth;
            c.gardenGrateSource=Tuple{mouth[0],mouth[1],mouth[2]};
            c.gardenGrateInReach=distance<=1.35f && -std::sin(c.yaw)*dx+std::cos(c.yaw)*dz>=.65f*distance;
        }
    }
    const auto privyLine=chapterDialogue_.State().line;
    const bool privyCueRelevant=privyRagState_==bellwether::PrivyRagState::Available ||
        privyLine==dialogue::Line::PrivyRagRequest || privyLine==dialogue::Line::PrivyRagThanks;
    if(config_.combatFoundation1_7 && !legacyCombatCheckpoint_ && config_.developmentRescueJourney &&
       config_.authoredChapterTerrain && privyCueRelevant &&
       chapterDialogue_.State().tavernConversationLinesResolved==kTavernConversation.size() &&
       c.claimed && c.alive && c.exterior && !c.paused && !c.combat && !c.traversing && c.kitSeated &&
       playerSupport_.grounded && chapterTerrain_ && chapterTerrainGeneration_==worldRoute_.generation) {
        const auto& mouth=bellwether::kBellwetherPrivyMouth;
        const auto& listen=bellwether::kBellwetherPrivyListenPoint;
        const auto& front=bellwether::kBellwetherPrivyFront;
        const float dx=c.player.x-listen[0],dz=c.player.z-listen[2];
        const float distance=std::hypot(dx,dz);
        const float frontDistance=dx*front[0]+dz*front[1];
        if(distance<=2.8f && std::abs(c.listener.y-listen[1])<=1.5f &&
           chapterTerrain_->NonWalkableVolumeClear(c.player.x,c.player.z,listen[0],listen[2],
               listen[1]-.025f,listen[1]+.025f,.025f)) {
            c.privySource=dialogue::Tuple{mouth[0],mouth[1],mouth[2]};
            c.privyInReach=privyRagState_==bellwether::PrivyRagState::Available &&
                frontDistance>=.65f*distance &&
                std::sin(c.yaw)*(listen[0]-c.player.x)-
                    std::cos(c.yaw)*(listen[2]-c.player.z)>=.65f*distance;
        }
    }
    return c;
}

void GameSimulation::UpdateChapterDialogue(const InputSnapshot& input,float seconds) {
    // Historical replay/capture fixtures keep their old timing and semantics.
    if(!config_.combatFoundation1_7 || legacyCombatCheckpoint_) return;
    using namespace horde::gameplay::dialogue;
    const auto* terrain=chapterTerrainGeneration_==worldRoute_.generation?chapterTerrain_.get():nullptr;
    const auto arrival=chapterCompanion_.State().bellwetherArrival;
    if(IsBellwetherTavernKeeperReady() && terrain && !input.paused &&
       rescueTraversal_.Snapshot().exteriorSide && !rescueTraversal_.IsActive() &&
       playerVitals_.Snapshot().phase==PlayerLifePhase::Alive && arrival!=BellwetherArrivalStage::AtLookout)
        tavernKeeper_.Step(seconds,arrival==BellwetherArrivalStage::Seated,*terrain);
    EnsureBellwetherResidentPoses();
    auto c=ChapterInteractionContext(input.paused);
    if(tavernKeeper_.Pose() && c.keeperAtHearth && c.companionSource && !c.paused && c.alive &&
       !c.traversing && !c.combat &&
       chapterDialogue_.State().tavernConversationLinesResolved==kTavernConversation.size())
        tavernKeeper_.TurnAtHearth(seconds,c.tavernFittingInReach,c.player.x,c.player.z);
    if(salvager_ && c.salvagerSource && !c.paused && c.alive && !c.traversing) {
        auto& pose=*salvager_;
        pose.idleSeconds+=seconds;
        const auto line=chapterDialogue_.State().line;
        const bool attending=!c.combat &&
            ((line==Line::None && c.salvagerInReach) || IsSalvagerLine(line));
        const float dx=c.player.x-pose.foot[0],dz=c.player.z-pose.foot[2];
        const float target=attending && std::hypot(dx,dz)>.001f?
            std::atan2(dx,-dz):bellwether::kSalvagerRestYaw;
        pose.yaw=TurnResidentYaw(pose.yaw,target,seconds,1.5f);
    }
    if(recordKeeper_ && !c.paused && c.alive && !c.traversing) {
        auto& keeper=*recordKeeper_;
        if(c.recordKeeperSource)keeper.idleSeconds+=seconds;
        const auto line=chapterDialogue_.State().line;
        const bool attending=c.recordKeeperSource && !c.combat &&
            ((line==Line::None && c.recordKeeperInReach) || IsRecordKeeperLine(line));
        const float dx=c.player.x-keeper.foot[0],dz=c.player.z-keeper.foot[2];
        const float target=attending && std::hypot(dx,dz)>.001f?std::atan2(dx,-dz):kRecordKeeperRestYaw;
        keeper.yaw=TurnResidentYaw(keeper.yaw,target,seconds,1.5f);
    }
    if(terrain && c.tavernKeeperSource && tavernKeeper_.Pose()->stage==TavernKeeperStage::AtDoor &&
       !c.paused && c.alive && !c.traversing && !c.combat) {
        const auto door=traversal::kBellwetherTavernRoute[traversal::kBellwetherSouthDoorStation];
        const auto& kit=chapterCompanion_.State();
        const bool approaching=arrival==BellwetherArrivalStage::ProtectiveApproach &&
            kit.bellwetherStation>traversal::kBellwetherSouthApproachStation &&
            kit.bellwetherStation<=traversal::kBellwetherSouthDoorStation+1 &&
            std::hypot(kit.x-door.x,kit.z-door.z)<=2.5f && c.companionSource;
        const auto audibleFromDoor=[&](Tuple other) {
            return TavernWelcomeAudible(*terrain,*c.tavernKeeperSource,other);
        };
        if(approaching || chapterDialogue_.State().line==Line::TavernWelcome)
            c.tavernDoorInReach=playerSupport_.grounded && audibleFromDoor(c.listener);
        c.kitAtSouthDoor=c.tavernDoorInReach && approaching && audibleFromDoor(*c.companionSource);
    }
    if(c.exterior&&!c.traversing&&!chapterFirstExterior_) {
        chapterFirstExterior_=true;
        interactionState_.heldLightPose=interactions::HeldLightPose::Low;
        interactionState_.heldLightPoseProgress=0;
        chapterVisibleRaise_=0;
        ResolveHeldItems();
    }
    c.lanternLow=interactionState_.heldLightPose==interactions::HeldLightPose::Low;
    c.lanternHigh=interactionState_.heldLightPose==interactions::HeldLightPose::High;
    c.flaskGuideEnabled=config_.healingFlask1_7;
    c.flaskCharges=healingFlask_.Snapshot().resources.charges;
    c.manualRaiseSequence=lastConsumedToggleHeldLightPoseSequence_;
    // Platform feedback can only match a consumed manual edge. The internal
    // acknowledgement is the preferred path and never trusts a stale sequence.
    c.visibleRaiseSequence=chapterVisibleRaise_;
    if(!input.paused && input.dialogueCompletionGeneration)
        chapterDialogue_.Complete(static_cast<Line>(input.dialogueCompletionLine),input.dialogueCompletionGeneration);
    if(input.commands.dialogueSkip>chapterSkipFloor_) {
        chapterSkipFloor_=input.commands.dialogueSkip;
        if(!input.paused) chapterDialogue_.Skip();
    }
    const bool proofWasAccepted=chapterDialogue_.State().proofAccepted;
    const auto bindCompanion=[&] {
        const auto& kit=chapterCompanion_.State();
        const bool assessing=kit.bellwetherArrival==BellwetherArrivalStage::AnkleAssessment;
        c.kitCaughtAnkle=assessing && kit.motion.anklePlaybackSeconds>=audio::kKitAnkleBeats.back().seconds;
        c.kitReadyForProtectiveWalk=assessing && kit.motion.anklePlaybackSeconds>=kAnkleAssessmentDurationSeconds;
        c.roadInReach=(assessing || kit.bellwetherArrival==BellwetherArrivalStage::ProtectiveApproach) &&
            playerSupport_.grounded && std::abs(playerSupport_.worldY-kit.y)<=1.6f &&
            std::hypot(playerX_-kit.x,playerZ_-kit.z)<=5.f;
        const auto basin=kWorldRoutePoints[2];
        c.companionAtBasin=kit.basinGuide && kit.basinArrived && kit.grounded;
        c.playerNearBasin=c.exterior &&
            std::hypot(playerX_-basin.x,playerZ_-basin.z)<=3.f;
    };
    bindCompanion();
    chapterDialogue_.Step(seconds,c);
    const bool guidePending=config_.healingFlask1_7 && chapterDialogue_.BasinGuidePending();
    const bool readyToLead=chapterDialogue_.State().line!=Line::ReunionProof &&
        chapterDialogue_.State().line!=Line::ReunionFirstPiece;
    if(c.exterior && c.alive && !c.traversing && !c.paused && terrain &&
       campaignBoundary_ && campaignBoundary_->id==campaign::CheckpointId::ForestEndpoint &&
       chapterDialogue_.State().line==Line::None && playerSupport_.grounded)
        chapterCompanion_.TryStartBellwetherArrival(playerX_,playerZ_,*terrain);
    const float previousAnkleTime=chapterCompanion_.State().motion.anklePlaybackSeconds;
    chapterCompanion_.Step(seconds,playerX_,playerZ_,c.exterior,
        chapterDialogue_.State().proofAccepted && (!guidePending || readyToLead),c.paused,
        terrain,
        config_.authoredChapterTerrain,c.alive&&!c.traversing,
        guidePending && readyToLead,chapterDialogue_.HoldingAnkleAssessment());
    const auto& kit=chapterCompanion_.State();
    if(kit.grounded && kit.bellwetherArrival==BellwetherArrivalStage::AnkleAssessment &&
       kit.motion.anklePlaybackSeconds>previousAnkleTime) {
        const float headingX=std::sin(kit.yaw), headingZ=std::cos(kit.yaw);
        for(const auto& beat:audio::kKitAnkleBeats) {
            if(previousAnkleTime<beat.seconds && kit.motion.anklePlaybackSeconds>=beat.seconds)
                Emit(GameplayEventType::KitAnkleFoley,EntityId::Kit,EntityId::Invalid,
                    kit.x+headingX*beat.forward+headingZ*beat.right,
                    kit.z-headingZ*beat.forward+headingX*beat.right,beat.gain,
                    static_cast<std::int32_t>(beat.cue),kit.y+beat.height);
        }
    }
    chapterDialogue_.RefreshCompanionSpatial(ChapterInteractionContext(input.paused));
    const auto& speech=chapterDialogue_.State();
    const auto lanternPosition=speech.line==Line::TavernIntroduce || IsTavernLanternLine(speech.line)?
        HeldRewardLanternSource():std::nullopt;
    chapterCompanion_.AdvanceActing(seconds,speech,c.exterior,c.alive,
        !proofWasAccepted && speech.proofAccepted,lanternPosition);
}
bool GameSimulation::AcknowledgeChapterPresentation(std::uint64_t tick,std::uint64_t generation) {
    if(tick!=snapshot_.tickIndex||generation!=chapterDialogue_.State().generation||
        snapshot_.paused||!snapshot_.rescue.exteriorSide||snapshot_.rescue.equipmentStowed||
        snapshot_.interaction.heldLightPose!=interactions::HeldLightPose::High) return false;
    chapterVisibleRaise_=snapshot_.lastConsumedToggleHeldLightPoseSequence;
    return true;
}
} // namespace horde::gameplay::simulation
