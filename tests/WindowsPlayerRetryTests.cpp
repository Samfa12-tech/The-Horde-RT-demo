#include <cstdint>
#include <iostream>

// Compile the actual platform transition bodies. GPU, OS focus and native
// retry outcomes are injected; this is not a second implementation of retry.
namespace horde::gameplay {
enum class PlayerLifePhase { Alive, Dead };
enum class EnemyKind { None };
struct ShowcaseCheckpoint {};
inline bool checkpointAvailable=true;
inline const ShowcaseCheckpoint* FindShowcaseCheckpoint(std::int32_t) {
    static ShowcaseCheckpoint checkpoint;return checkpointAvailable?&checkpoint:nullptr;
}
}
namespace horde::telemetry { enum class RtLifecycleEvent { Retry }; }
namespace {
using Phase=horde::gameplay::PlayerLifePhase;
struct Snapshot {
    bool healingFlaskEnabled=false,developmentRescueJourney=false;
    struct {unsigned claimCount=0;} rescue;
    struct {Phase phase=Phase::Dead;int vitality=0;} playerVitals;
    std::uint32_t retryGeneration=0;
};
struct Simulation {
    enum class Result { Success, Failure, AliveWithoutCommit, ZeroHealth } result=Result::Success;
    ::Snapshot state;unsigned requests=0,eventClears=0;
    const ::Snapshot& Snapshot()const{return state;}
    void RetryEncounter() {
        ++requests;
        if(result==Result::Failure)return;
        state.playerVitals={Phase::Alive,result==Result::ZeroHealth?0:3};
        if(result!=Result::AliveWithoutCommit)++state.retryGeneration;
    }
    void ClearEvents(){++eventClears;}
};
struct Counter {unsigned count=0;void Clear(){++count;}void Cancel(){++count;}
    void ApplyEvent(horde::telemetry::RtLifecycleEvent){++count;}};
struct Lookout {void Close(){}};
struct VulkanSurfaceContext {
    Lookout* lookoutObjective=nullptr;
    bool deathOverlayVisible=false,pauseMenuVisible=false,settingsVisible=false;
    bool diagnosticsVisible=false,benchmarkReportVisible=false;
    bool campaignConfirmedRetry=false,campaignPause=false,rtFrameEvidenceInitialised=true;
    bool retryControlVisible=false,heldAttack=true,heldGuard=true,heldMove=true;
    unsigned inputClears=0,audioStops=0,mirrors=0,hudUpdates=0;
    int windowHandle=1;
    Simulation simulation;
    struct {bool paused=true,hasAuthoritativePlayerPose=true;} simulationInput;
    Counter delayedFeedback,benchmarkEvidence,rtFrameEvidence;
    horde::gameplay::EnemyKind debugEnemyOverride=horde::gameplay::EnemyKind::None;
};
struct RECT {int left=0,top=0,right=640,bottom=360;};
constexpr int kResumeButtonId=101;
int focus=0,layoutCalls=0,failures=0,checks=0;
void ApplyOverlayState(VulkanSurfaceContext& c) {
    c.retryControlVisible=c.pauseMenuVisible && !c.settingsVisible &&
        !c.diagnosticsVisible && !c.benchmarkReportVisible;
}
void GetClientRect(int,RECT*){}
void LayoutOverlayControls(int,int,int){++layoutCalls;}
int GetDlgItem(int,int id){return id;}
void FocusGameControl(int control){focus=control;}
void ClearDesktopInput(VulkanSurfaceContext& c) {
    ++c.inputClears;c.heldAttack=c.heldGuard=c.heldMove=false;
}
void StopPrivateLichRevivalAudio(VulkanSurfaceContext& c){++c.audioStops;}
void MirrorSimulationSnapshot(VulkanSurfaceContext& c){++c.mirrors;}
void UpdateVitalityHud(VulkanSurfaceContext& c){++c.hudUpdates;}
#include "platform/windows/windows_player_retry.inl"
void Check(bool value,const char* message) {
    ++checks;if(!value){++failures;std::cerr<<message<<'\n';}
}
void Die(VulkanSurfaceContext& c) {
    c.simulation.state.playerVitals={Phase::Dead,0};ShowDeathMenu(c);
    Check(c.deathOverlayVisible && c.pauseMenuVisible && c.retryControlVisible && focus==kResumeButtonId,
        "each death presents the production death menu and focuses Retry");
}
}
int main() {
    VulkanSurfaceContext c;
    for(int death=0;death<3;++death) {
        Die(c);c.heldAttack=c.heldGuard=c.heldMove=true;
        Check(ApplyPlayerRetryCheckpoint(c,9),"committed living retry succeeds");
        Check(!c.deathOverlayVisible && !c.pauseMenuVisible && !c.retryControlVisible,
            "successful production retry retires both overlay latches before another death");
        Check(!c.heldAttack && !c.heldGuard && !c.heldMove && !c.simulationInput.paused && focus==c.windowHandle,
            "successful retry clears held input and returns gameplay focus");
    }
    Check(c.inputClears==3 && c.simulation.requests==3 && c.rtFrameEvidence.count==3,
        "each admitted retry owns one input retirement and lifecycle reset");
    Die(c);
    const auto before=c.simulation.requests;
    Check(!ApplyPlayerRetryCheckpoint(c,77) && c.simulation.requests==before && c.retryControlVisible,
        "invalid destination preserves existing death UI and performs no native retry");
    horde::gameplay::checkpointAvailable=false;
    Check(!ApplyPlayerRetryCheckpoint(c,9) && c.retryControlVisible,"missing checkpoint cannot discard death UI");
    horde::gameplay::checkpointAvailable=true;
    for(auto result:{Simulation::Result::Failure,Simulation::Result::AliveWithoutCommit,Simulation::Result::ZeroHealth}) {
        c.simulation.state.playerVitals={Phase::Dead,0};c.simulation.result=result;
        Check(!ApplyPlayerRetryCheckpoint(c,9) && c.deathOverlayVisible && c.pauseMenuVisible && c.retryControlVisible,
            "failed/uncommitted/zero-health retry keeps existing death menu paused");
    }
    // Repair a stale visible latch with a hidden pause menu idempotently.
    c.deathOverlayVisible=true;c.pauseMenuVisible=false;ApplyOverlayState(c);ShowDeathMenu(c);
    Check(c.pauseMenuVisible && c.retryControlVisible,"production death presentation repairs hidden-menu latch mismatch");
    const int layouts=layoutCalls;ShowDeathMenu(c);
    Check(layoutCalls==layouts,"an already presented death menu is idempotent");
    c.simulation.state.healingFlaskEnabled=c.simulation.state.developmentRescueJourney=true;
    c.simulation.state.rescue.claimCount=1;c.simulation.result=Simulation::Result::Failure;
    Check(ApplyPlayerRetryCheckpoint(c,9) && c.campaignPause && c.deathOverlayVisible && c.pauseMenuVisible &&
        c.simulationInput.paused,"confirmed-record request retains death pause until its separate commit/presentation barrier");
    c.campaignConfirmedRetry=true;const auto requests=c.simulation.requests;
    Check(!ApplyPlayerRetryCheckpoint(c,9) && c.simulation.requests==requests && c.retryControlVisible,
        "an in-flight confirmed retry cannot overlap or dismiss existing UI");
    std::cout<<"Production Windows death/retry checks="<<checks<<" failures="<<failures<<'\n';
    return failures?1:0;
}
