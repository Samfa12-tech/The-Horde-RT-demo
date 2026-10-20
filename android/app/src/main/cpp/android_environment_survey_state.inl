// Diagnostic request state only; no GPU, game input, campaign or saved settings.
std::mutex gEnvironmentSurveyMutex;
std::atomic<std::uint64_t> gEnvironmentSurveyActiveSurface{0};
std::atomic<std::uint64_t> gEnvironmentSurveyRequestedSurface{0};
// Keep delayed Activity stop callbacks isolated even after successful retirement.
std::atomic<bool> gEnvironmentSurveyPreserveStops{false};
horde::platform::android::AndroidEnvironmentSurveyPolicy gEnvironmentSurveyPolicy;
horde::graphics::GraphicsSettings gEnvironmentSurveyGraphics{};
bool gEnvironmentSurveyKeeperMist=false;
bool EnvironmentSurveyPending() {
    std::lock_guard lock(gEnvironmentSurveyMutex);return gEnvironmentSurveyPolicy.Phase()==1;
}
bool BindEnvironmentSurvey(SwapchainContext& context,std::uint64_t serial,std::uint64_t generation) {
    std::lock_guard lock(gEnvironmentSurveyMutex);
    if(!gEnvironmentSurveyPolicy.Bind(serial,generation))return false;
    context.environmentSurvey=true;context.environmentSurveySerial=serial;
    context.environmentSurveyId=gEnvironmentSurveyPolicy.Id();
    context.keeperMistRequested=gEnvironmentSurveyKeeperMist;
    context.environmentSurveyWaterDisabled=gEnvironmentSurveyPolicy.WaterDisabled();
    context.graphicsSettings=gEnvironmentSurveyGraphics;
    gEnvironmentSurveyActiveSurface.store(generation,std::memory_order_release);
    gEnvironmentSurveyPreserveStops.store(true,std::memory_order_release);
    return true;
}
void FailEnvironmentSurvey(std::uint64_t serial,const char* reason) {
    std::lock_guard lock(gEnvironmentSurveyMutex);
    if(serial==gEnvironmentSurveyPolicy.Serial())gEnvironmentSurveyPolicy.Cancel();
    if(!gSwapchainRunning.load(std::memory_order_acquire) && !gSurfaceRetirementBlocked.load(std::memory_order_acquire))
        gEnvironmentSurveyActiveSurface.store(0,std::memory_order_release);
    __android_log_print(ANDROID_LOG_ERROR,kTag,"HORDE_ENVIRONMENT_SURVEY failed serial=%llu reason=%s",
        static_cast<unsigned long long>(serial),reason);
}
