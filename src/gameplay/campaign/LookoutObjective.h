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
    None, Lookout, BellwetherShrine, BellwetherArchive, BellwetherRegister, BellwetherExpeditions, BellwetherLowerStone
};

enum class AirWardDirections { RestoreBinding, LearnWard, Learned };

inline const char* BellwetherShorePracticeLine(bellwether::ShorePracticeStage stage,bool completed) {
    using bellwether::ShorePracticeStage;
    if(completed)return "Complete. Visit the salvager at the river fork to recall the lesson.";
    switch(stage) {
    case ShorePracticeStage::FindDryMark:return "Follow the causeway to the dry mark by the river rail.";
    case ShorePracticeStage::EnterShallows:return "From the dry mark, wade to the river rail.";
    case ShorePracticeStage::Retreat:return "Return from the shallows to the dry mark.";
    case ShorePracticeStage::ReportBack:return "Return to the salvager at the river fork to report back.";
    case ShorePracticeStage::Idle:break;
    }
    return "Meet the salvager at the river fork.";
}

inline AirWardDirections CurrentAirWardDirections(const bellwether::WaysideArchive& archive) {
    if(archive.airWardLearned)return AirWardDirections::Learned;
    return archive.stage==bellwether::ArchiveStage::Restored ?
        AirWardDirections::LearnWard:AirWardDirections::RestoreBinding;
}

inline PauseObjective CurrentPauseObjective(const simulation::SimulationSnapshot& s,
    const std::optional<Checkpoint>& boundary)
{
    if (boundary && boundary->contentRevision == kBellwetherContentRevision &&
        s.developmentRescueJourney && s.playerAlive && s.rescue.exteriorSide &&
        (s.chapterDialogue.tavernConversationLinesResolved == dialogue::kTavernConversation.size() ||
            s.waysideArchive.registerInterpreted))
    {
        if (s.waysideArchive.registerInterpreted) return PauseObjective::BellwetherExpeditions;
        if (s.waysideArchive.registerRecovered) return PauseObjective::BellwetherRegister;
        if (s.waysideArchive.lanternRevealStarted) return PauseObjective::BellwetherArchive;
        if (s.waysideArchive.stage != bellwether::ArchiveStage::Closed) return PauseObjective::BellwetherLowerStone;
        return PauseObjective::BellwetherShrine;
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
    case PauseObjective::BellwetherShrine:
        return "Ask the record keeper by the northern church about the wayside shrine. "
            "The shrine is on a short spur off the northern path.";
    case PauseObjective::BellwetherArchive:
        return "Recover the road register from the opened wayside archive.";
    case PauseObjective::BellwetherLowerStone:
        return "Open the lower stone at the wayside shrine. Stay close to hear the lantern.";
    case PauseObjective::BellwetherRegister:
        return dialogue::Spec(dialogue::Line::TavernRegisterDirections).text;
    case PauseObjective::BellwetherExpeditions:
        return "The Abbey lies downstream. The Foundry is east, across the gorge.\r\n\r\n"
            "Both are leads for future expeditions.";
    case PauseObjective::None: return "";
    }
    return "";
}

inline std::string PauseObjectiveText(PauseObjective objective, TreasurePurse purse, AirWardDirections ward,
    bellwether::MillPreparation mill={},bool shorePracticeAvailable=false,
    bellwether::ShorePracticeStage shorePracticeStage=bellwether::ShorePracticeStage::Idle,
    bool shorePracticeCompleted=false)
{
    std::string text=PauseObjectiveText(objective);
    if (objective==PauseObjective::BellwetherExpeditions) {
        text+="\r\n\r\nPurse: "+std::to_string(purse.coins)+(purse.coins==1?" coin.":" coins.");
        if (purse.lanternFittingPurchased) text+="\r\nLantern hinge fitted.";
        else text+="\r\nLantern fitting at the tavern: "+std::to_string(TreasurePurse::kLanternFittingPrice)+" coins.";
        switch(ward) {
        case AirWardDirections::RestoreBinding:text+="\r\n\r\nFor an air ward, return to the wayside shrine and restore its binding.";break;
        case AirWardDirections::LearnWard:text+="\r\n\r\nBinding restored. Ask the record keeper to teach you the air ward.";break;
        case AirWardDirections::Learned:text+="\r\n\r\nAir ward learned for the Abbey crossing.";break;
        }
    }
    if(objective!=PauseObjective::None && objective!=PauseObjective::Lookout) {
        using bellwether::MillPreparation;
        switch(mill) {
        case MillPreparation::Unexamined:
            if(objective==PauseObjective::BellwetherExpeditions)
                text+="\r\n\r\nFollow the MILL signs east to inspect the gearing at the workbench.";
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
    if(objective!=PauseObjective::None && objective!=PauseObjective::Lookout && shorePracticeAvailable)
        text+="\r\n\r\nShore practice: "+std::string(BellwetherShorePracticeLine(shorePracticeStage,shorePracticeCompleted));
    return text;
}
} // namespace horde::gameplay::campaign
