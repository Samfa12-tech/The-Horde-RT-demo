// Included after the existing XAudio2 engine; never an additional audio engine.
void StopPrivateLichRevivalAudio(VulkanSurfaceContext& context)
{
    auto& state=context.privateRevivalAudio;
    if(state.voice.active)SpatialAudioEngine().StopLoop("lich_revival");
    state.voice.Cancel();
}
void UpdatePrivateLichRevivalAudio(VulkanSurfaceContext& context)
{
#if defined(_DEBUG)
    auto& state=context.privateRevivalAudio;
    if(!state.checked) {
        state.checked=true;wchar_t requested[2]{};
        state.opted=GetEnvironmentVariableW(L"HORDE_DEBUG_LICH_REVIVAL_AUDIO",requested,2)==1 && requested[0]==L'1';
        if(!state.opted)return;
        state.file=ExecutableDirectory()/"debug_audio/lich_revival.wav";
        std::error_code error;const auto bytes=std::filesystem::file_size(state.file,error);
        std::string hash,diagnostic;
        if(error || bytes!=horde::audio::kLichRevivalWaveBytes ||
           !Sha256File(state.file,hash,diagnostic) || hash!=horde::audio::kLichRevivalWaveSha256) {
            LogWindowsAudio("private revival WAV missing or differs from approved mono48k PCM16 draft; no substitute: "+diagnostic);return;
        }
        state.available=SpatialAudioEngine().PreloadMonoPcm48k16(state.file);
        if(!state.available)LogWindowsAudio("verified private revival WAV failed existing XAudio2 preload");
    }
    if(!state.opted || !state.available)return;
    const auto& snapshot=context.simulation.Snapshot();
    if(state.identitySeen && (state.retry!=snapshot.retryGeneration || state.routeReset!=snapshot.lastConsumedRouteResetSequence || state.musicReset!=context.musicResetToken)) {
        StopPrivateLichRevivalAudio(context);state.voice.lastSequence=0;
    }
    state.identitySeen=true;state.retry=snapshot.retryGeneration;state.routeReset=snapshot.lastConsumedRouteResetSequence;state.musicReset=context.musicResetToken;
    const bool eligible=context.controlsEnabled && !MeasurementPausedByUi(context) && !context.benchmark.IsRunning() &&
        !context.deathOverlayVisible && !context.endingOverlayVisible && GetForegroundWindow()==context.windowHandle && snapshot.playerAlive;
    const float gain=horde::audio::OwnedCueGain(context.sfxVolumePercent/100.f,horde::audio::DialogueBedGain(snapshot));
    if(!eligible || gain<=0.f){StopPrivateLichRevivalAudio(context);return;}
    if(state.voice.active)SpatialAudioEngine().SetOwnedVolume("lich_revival",gain);
#else
    (void)context;
#endif
}
void PlayPrivateLichRevivalAudio(VulkanSurfaceContext& context,const horde::gameplay::simulation::GameplayEvent& event)
{
#if defined(_DEBUG)
    UpdatePrivateLichRevivalAudio(context);
    auto& state=context.privateRevivalAudio;
    if(!state.opted || !state.available || !horde::audio::IsLichRevivalCry(event))return;
    const auto& snapshot=context.simulation.Snapshot();
    const float gain=horde::audio::OwnedCueGain(context.sfxVolumePercent/100.f,horde::audio::DialogueBedGain(snapshot));
    const bool eligible=context.controlsEnabled && !MeasurementPausedByUi(context) && !context.benchmark.IsRunning() &&
        !context.deathOverlayVisible && !context.endingOverlayVisible && GetForegroundWindow()==context.windowHandle && snapshot.playerAlive && gain>0.f;
    const auto stereo=horde::gameplay::CalculateSpatialAudio(horde::audio::LichRevivalEmitter(event),
        {event.listenerX,event.listenerZ,event.listenerYawRadians,event.listenerY});
    if(event.sequence==0 || event.sequence<=state.voice.lastSequence)return;
    StopPrivateLichRevivalAudio(context);
    if(!state.voice.Begin(event.sequence,stereo.left,stereo.right,eligible))return;
    if(!SpatialAudioEngine().Play(state.file,stereo.left*horde::audio::kLichRevivalMixGain,
        stereo.right*horde::audio::kLichRevivalMixGain,"lich_revival",0u,0u,gain))state.voice.Cancel();
#else
    (void)context;(void)event;
#endif
}
