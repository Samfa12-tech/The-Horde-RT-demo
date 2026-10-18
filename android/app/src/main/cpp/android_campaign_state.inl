// Included inside the native bridge's private namespace after input ownership.
// JNI deposits immutable commands; only the serialized render owner touches
// PlaySession, simulation, resource preparation or the load barrier.
namespace campaign = horde::gameplay::campaign;
struct CampaignCommand {
    int action=0,slot=0,kind=0;
    std::uint64_t serial=0,generation=0;
};
std::mutex gCampaignMutex;
std::optional<CampaignCommand> gCampaignCommand;
std::string gCampaignConfiguredRoot;
std::array<jlong,33> gCampaignUi = [] {
    std::array<jlong,33> state{};state[0]=1;state[6]=-1;return state;
}();
std::uint64_t gCampaignRequestSerial=0;
std::atomic<bool> gCampaignPauseGate{false},gCampaignShowcaseOverride{false};
std::unique_ptr<campaign::PlaySession> gCampaignSession;
std::string gCampaignOwnerRoot;
std::optional<campaign::LoadPreparation> gCampaignPreparation;
std::unique_ptr<horde::vulkan::raytracing::PresentableTinyRtScene> gCampaignPreparedScene;
std::uint64_t gCampaignOwnerRequest=0,gCampaignOwnerGeneration=0,gCampaignPublishedRevision=0;
int gCampaignPhase=0,gCampaignError=0,gCampaignLastAction=0;
bool gCampaignIsolated=false;
bool gCampaignRetirementUnproved=false;
horde::platform::android::AndroidCampaignLoadBarrier gCampaignBarrier;
std::uint64_t gCampaignAckToken=0,gCampaignAckGeneration=0;
std::uint64_t gCampaignFrameTick=0,gCampaignFrameWorld=0;
std::uint64_t gCampaignLoadPublication=0,gCampaignAckPublication=0;
std::uint64_t gCampaignStartedNs=0;
float gCampaignNewGameYaw=0,gCampaignNewGamePitch=0;
