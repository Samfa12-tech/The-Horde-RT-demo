#pragma once

#include "gameplay/campaign/CampaignCheckpoint.h"
#include "gameplay/simulation/SimulationSnapshot.h"
#include <optional>
#include <string>

namespace horde::gameplay::campaign {
// Read-only presentation eligibility. Earning progress remains the simulation's
// checkpoint transaction; opening a panel never completes or saves the chapter.
inline bool LookoutObjectiveAvailable(const simulation::SimulationSnapshot& s,
    const std::optional<Checkpoint>& boundary) {
    const auto p=simulation::kWorldRoutePoints.back();
    const auto end=simulation::kPrivateChapterDemoEndpoint;
    const float dx=end.x-p.x,dz=end.z-p.z;
    const float t=std::clamp(((s.playerX-p.x)*dx+(s.playerZ-p.z)*dz)/(dx*dx+dz*dz),0.f,1.f);
    // The turnaround includes the visible timber stop beyond its centre.
    const bool onApproach=std::hypot(s.playerX-(p.x+dx*t),s.playerZ-(p.z+dz*t))<=2.f ||
        std::hypot(s.playerX-end.x,s.playerZ-end.z)<=3.8f;
    return boundary && boundary->contentRevision==kForestContentRevision && Validate(*boundary) &&
        s.developmentRescueJourney && s.playerAlive && s.playerGrounded &&
        s.rescue.exteriorSide && s.rescue.firstAscentCompleted && !s.rescue.equipmentStowed &&
        s.worldRoute.readiness[static_cast<std::size_t>(simulation::WorldZoneId::Lookout)]==simulation::ZoneReadiness::Ready &&
        onApproach &&
        s.chapterDialogue.line==dialogue::Line::None && s.chapterDialogue.proofAccepted &&
        s.chapterDialogue.forestVillageConsumed && s.chapterCompanion.grounded &&
        s.chapterCompanion.waypoint==simulation::kWorldRoutePoints.size();
}

// Values are carried by the Android pause UI publication. This is a view of
// existing progress, with no saved quest state or gameplay command of its own.
enum class PauseObjective {
    None, Lookout, BellwetherShrine, BellwetherArchive, BellwetherRegister, BellwetherExpeditions, BellwetherLowerStone,
    BellwetherArrival, BellwetherHearth, BellwetherLantern
};

enum class AirWardDirections { RestoreBinding, LearnWard, Learned };

struct BellwetherDirections {
    TreasurePurse purse{};
    AirWardDirections ward=AirWardDirections::RestoreBinding;
    bellwether::MillPreparation mill{};
    bellwether::ShorePracticeStage shorePracticeStage{};
    bool millServiceAvailable=false;
    bool shorePracticeAvailable=false;
    bool shorePracticeCompleted=false;
    bool operator==(const BellwetherDirections&) const = default;
};

inline const char* BellwetherShorePracticeLine(const BellwetherDirections& directions) {
    using bellwether::ShorePracticeStage;
    if(directions.shorePracticeCompleted)
        return directions.shorePracticeAvailable ?
            "Complete. Breath control learned. Visit the salvager at the river fork to recall the lesson." :
            "Complete. Breath control learned.";
    switch(directions.shorePracticeStage) {
    case ShorePracticeStage::FindDryMark:return "Follow the causeway to the dry mark by the river rail.";
    case ShorePracticeStage::EnterShallows:return "From the dry mark, wade to the river rail. Breath practice starts in the shallows.";
    case ShorePracticeStage::Retreat:return "Return from the shallows to the dry mark before your breath runs out.";
    case ShorePracticeStage::ReportBack:return "Return to the salvager at the river fork to report back.";
    case ShorePracticeStage::RecoverBreath:return "Catch your breath on the dry mark, then try again.";
    case ShorePracticeStage::Idle:break;
    }
    return "Meet the salvager at the river fork.";
}

inline AirWardDirections CurrentAirWardDirections(const bellwether::WaysideArchive& archive) {
    if(archive.airWardLearned)return AirWardDirections::Learned;
    return archive.stage==bellwether::ArchiveStage::Restored ?
        AirWardDirections::LearnWard:AirWardDirections::RestoreBinding;
}

inline BellwetherDirections CurrentBellwetherDirections(const simulation::SimulationSnapshot& snapshot) {
    return {.purse=snapshot.treasurePurse,.ward=CurrentAirWardDirections(snapshot.waysideArchive),
        .mill=snapshot.millPreparation,.shorePracticeStage=snapshot.shorePracticeStage,
        .millServiceAvailable=snapshot.millServiceAvailable,
        .shorePracticeAvailable=snapshot.shorePracticeServiceAvailable,
        .shorePracticeCompleted=snapshot.salvagerShorePracticeCompleted};
}

inline PauseObjective CurrentPauseObjective(const simulation::SimulationSnapshot& s,
    const std::optional<Checkpoint>& boundary)
{
    if (boundary && boundary->contentRevision >= kForestContentRevision &&
        s.developmentRescueJourney && s.playerAlive && s.rescue.exteriorSide && !s.rescue.equipmentStowed) {
        if (boundary->contentRevision == kBellwetherContentRevision &&
            (s.chapterDialogue.tavernConversationLinesResolved == dialogue::kTavernConversation.size() ||
                s.waysideArchive.registerInterpreted)) {
            if (s.waysideArchive.registerInterpreted) return PauseObjective::BellwetherExpeditions;
            if (s.waysideArchive.registerRecovered) return PauseObjective::BellwetherRegister;
            if (s.waysideArchive.lanternRevealStarted) return PauseObjective::BellwetherArchive;
            if (s.waysideArchive.stage == bellwether::ArchiveStage::Open) return PauseObjective::BellwetherLantern;
            if (s.waysideArchive.stage != bellwether::ArchiveStage::Closed) return PauseObjective::BellwetherLowerStone;
            return PauseObjective::BellwetherShrine;
        }
        using dialogue::BellwetherArrivalStage;
        const auto arrival = s.chapterCompanion.bellwetherArrival;
        if (arrival == BellwetherArrivalStage::Seated && s.tavernKeeper) return PauseObjective::BellwetherHearth;
        if (arrival != BellwetherArrivalStage::AtLookout) return PauseObjective::BellwetherArrival;
    }
    return LookoutObjectiveAvailable(s, boundary) ? PauseObjective::Lookout : PauseObjective::None;
}

inline constexpr char kLookoutObjectiveText[] =
    "Follow the road down to Bellwether. The tavern is across the square.\r\n\r\n"
    "You can also turn back toward the tomb.";

inline const char* PauseObjectiveText(PauseObjective objective)
{
    switch (objective)
    {
    case PauseObjective::Lookout: return kLookoutObjectiveText;
    case PauseObjective::BellwetherArrival:
        return "Follow the road down to Bellwether. Cross the square and enter the tavern through its south door.";
    case PauseObjective::BellwetherHearth:
        return "Meet Kit by the tavern hearth. Choose Talk when the keeper joins him and the prompt appears.";
    case PauseObjective::BellwetherShrine:
        return "Ask the record keeper by the northern church about the wayside shrine. "
            "The shrine is on a short spur off the northern path.";
    case PauseObjective::BellwetherArchive:
        return "Recover the road register from the opened wayside archive.";
    case PauseObjective::BellwetherLowerStone:
        return "Open the lower stone at the wayside shrine. Stay close to hear the lantern.";
    case PauseObjective::BellwetherLantern:
        return "The wayside archive is open. Stand near the opening and choose Listen to the lantern.";
    case PauseObjective::BellwetherRegister:
        return dialogue::Spec(dialogue::Line::TavernRegisterDirections).text;
    case PauseObjective::BellwetherExpeditions:
        return "The Abbey lies downstream. The Foundry is east, across the gorge.\r\n\r\n"
            "Both are leads for future expeditions.";
    case PauseObjective::None: return "";
    }
    return "";
}

inline std::string PauseObjectiveText(PauseObjective objective,const BellwetherDirections& directions)
{
    std::string text=PauseObjectiveText(objective);
    if(objective==PauseObjective::None || objective==PauseObjective::Lookout ||
       objective==PauseObjective::BellwetherArrival || objective==PauseObjective::BellwetherHearth)return text;
    const auto& purse=directions.purse;
    if(objective==PauseObjective::BellwetherExpeditions || purse.ordinarySilver!=OrdinarySilverProgress::Unfound ||
       purse.millTally!=MillTallyProgress::Unfound) {
        text+="\r\n\r\nPurse: "+std::to_string(purse.coins)+(purse.coins==1?" coin.":" coins.");
        if (purse.lanternFittingPurchased) text+="\r\nLantern hinge fitted.";
        else text+="\r\nLantern fitting at the tavern: "+std::to_string(TreasurePurse::kLanternFittingPrice)+" coins.";
        if(purse.ordinarySilver==OrdinarySilverProgress::Carried)
            text+="\r\nOrdinary silver: sell it at the stall west of the square.";
        if(purse.ordinarySilver==OrdinarySilverProgress::Sold)
            text+="\r\nOrdinary silver sold.";
        switch(purse.millTally) {
        case MillTallyProgress::Carried:
            text+="\r\nMill tally stamp: return it at the board by the mill's west door, or sell it at the dealer's tray.";
            break;
        case MillTallyProgress::Sold:
            text+="\r\nThe dealer holds the tally stamp. Check the mill's west-door board to record the claim.";
            break;
        case MillTallyProgress::ClaimRecorded:
            text+="\r\nMill claim recorded. Recover the tally stamp at the dealer's tray without paying.";
            break;
        case MillTallyProgress::Recovered:
            text+="\r\nTally stamp recovered. Return it at the board by the mill's west door.";
            break;
        case MillTallyProgress::Returned:
        case MillTallyProgress::ReturnedAfterSale:
            text+="\r\nThe mill's tally stamp has been returned.";
            break;
        case MillTallyProgress::Unfound: break;
        }
    }
    if (objective==PauseObjective::BellwetherExpeditions) {
        switch(directions.ward) {
        case AirWardDirections::RestoreBinding:text+="\r\n\r\nFor an air ward, return to the wayside shrine and restore its binding.";break;
        case AirWardDirections::LearnWard:text+="\r\n\r\nBinding restored. Ask the record keeper to teach you the air ward.";break;
        case AirWardDirections::Learned:text+="\r\n\r\nAir ward learned for the Abbey crossing.";break;
        }
    }
    using bellwether::MillPreparation;
    if(directions.millServiceAvailable || directions.mill!=MillPreparation::Unexamined) {
        switch(directions.mill) {
        case MillPreparation::Unexamined:
            text+="\r\n\r\nFor a breathing rig, follow the MILL signs east and inspect the gearing at the artificer's workbench.";
            break;
        case MillPreparation::BearingNeeded:
            text+="\r\n\r\nFind a spare bearing in the parts bin south of the mill path.";
            break;
        case MillPreparation::BearingRecovered:
            text+="\r\n\r\nTake the spare bearing to the mill workbench and check its fit.";
            break;
        case MillPreparation::FitChecked:
            text+="\r\n\r\nMill bearing matched. Seat it in the workbench drive when the artificer is present.";
            break;
        case MillPreparation::BearingSeated:
            text+="\r\n\r\nThe bearing is seated. Turn the drive at the workbench to check it moves freely.";
            break;
        case MillPreparation::GearingVerified:
            text+="\r\n\r\nThe drive turned freely. Collect the breathing rig from the artificer beside the bench.";
            break;
        case MillPreparation::RigTrialReady:
            text+="\r\n\r\nFollow the stone causeway south of the mill to the river rail. Start the leak test "
                  "in the shallows and stay there for two seconds. Return to dry stone to confirm the seal.";
            break;
        case MillPreparation::RigTested:
            text+="\r\n\r\nBreathing rig passed its shallow-water seal test.";
            break;
        }
    }
    if(directions.shorePracticeAvailable || directions.shorePracticeCompleted)
        text+="\r\n\r\nShore practice: "+std::string(BellwetherShorePracticeLine(directions));
    return text;
}
} // namespace horde::gameplay::campaign
