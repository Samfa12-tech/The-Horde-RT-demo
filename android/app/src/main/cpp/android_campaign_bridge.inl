// Included after RT retirement/initialisation helpers. Owner-thread methods.
using CampaignBarrier=horde::platform::android::AndroidCampaignLoadBarrier;

bool CampaignNeutralInput(const horde::gameplay::simulation::InputSnapshot& input,
    const CampaignBarrier::Status& barrier)
{
    return input.paused && input.moveForward==0 && input.moveStrafe==0 &&
        !input.runHeld && !input.crouchHeld && !input.hasAuthoritativePlayerPose &&
        barrier.MatchesRebasePose(input.yawRadians,input.pitchRadians);
}

int CampaignStorageError(const campaign::storage::Result& result)
{
    using S=campaign::storage::Status;
    switch(result.status) {
    case S::NotFound:return 4;
    case S::InvalidExisting:case S::InvalidCandidate:return 5;
    case S::PublishedDurabilityUnknown:case S::PublishOutcomeUnknown:return 10;
    case S::Busy:return 2;
    default:return 6;
    }
}

void PublishCampaignOnOwner(const SwapchainContext& context)
{
    std::array<jlong,33> ui{};ui[0]=1;ui[1]=context.surfaceGeneration;
    ui[3]=gCampaignOwnerRequest;ui[4]=gCampaignPhase;ui[5]=gCampaignError;ui[6]=-1;
    if(gCampaignSession) {
        const auto state=gCampaignSession->State();
        if(state.activeSlot)ui[6]=*state.activeSlot;
        ui[7]=state.hasLatestCheckpoint?1:0;
        ui[31]=static_cast<int>(state.result.durability);
        const auto& entries=gCampaignSession->Catalogue();
        for(std::size_t i=0;i<entries.size();++i) {
            ui[12+i*3]=static_cast<int>(entries[i].status)+1;
            if(entries[i].checkpoint) {
                ui[13+i*3]=static_cast<int>(entries[i].checkpoint->id);
                ui[14+i*3]=entries[i].checkpoint->vitality;
            }
        }
    }
    const auto barrier=gCampaignBarrier.State();
    ui[8]=barrier.rebaseToken;
    ui[9]=std::bit_cast<std::uint32_t>(barrier.yaw);
    ui[10]=std::bit_cast<std::uint32_t>(barrier.pitch);
    if(gCampaignLastAction==5 && gCampaignPhase==7) {
        ui[9]=std::bit_cast<std::uint32_t>(gCampaignNewGameYaw);
        ui[10]=std::bit_cast<std::uint32_t>(gCampaignNewGamePitch);
    }
    ui[11]=barrier.phase==CampaignBarrier::Phase::Complete;
    ui[30]=gCampaignLastAction;ui[32]=gCampaignIsolated?1:0;
    std::lock_guard lock(gCampaignMutex);
    ui[2]=gCampaignUi[2];
    if(ui!=gCampaignUi) {
        if(gCampaignPublishedRevision<static_cast<std::uint64_t>(INT64_MAX))++gCampaignPublishedRevision;
        ui[2]=gCampaignPublishedRevision;gCampaignUi=ui;
        __android_log_print(ANDROID_LOG_INFO,kTag,
            "HORDE_CAMPAIGN request=%lld generation=%lld phase=%lld error=%lld active_slot=%lld latest=%lld rebase=%lld presented=%lld action=%lld isolated=%lld durability=%lld",
            static_cast<long long>(ui[3]),static_cast<long long>(ui[1]),static_cast<long long>(ui[4]),
            static_cast<long long>(ui[5]),static_cast<long long>(ui[6]),static_cast<long long>(ui[7]),
            static_cast<long long>(ui[8]),static_cast<long long>(ui[11]),static_cast<long long>(ui[30]),
            static_cast<long long>(ui[32]),static_cast<long long>(ui[31]));
    }
}

bool DrainCampaignResources(SwapchainContext& context)
{
    if(!ConsumePendingImageAcquire(context)){gCampaignRetirementUnproved=true;return false;}
    const VkResult idle=vkDeviceWaitIdle(context.device);
    const bool retired=CompleteRtEvidenceAfterDeviceIdle(context,idle) &&
        vkResetCommandPool(context.device,context.commandPool,0)==VK_SUCCESS;
    gCampaignRetirementUnproved=!retired;return retired;
}

bool PrepareCampaignResources(SwapchainContext& context)
{
    using Scene=horde::vulkan::raytracing::PresentableTinyRtScene;
    if(context.sceneProfile==horde::vulkan::raytracing::RtSceneProfile::Showcase &&
       context.developmentRescueJourney && context.rtScene.IsReady() &&
       context.rtScene.StaticGeometryHasCompleteContributors())return true;
    if(gCampaignPreparedScene)return gCampaignPreparedScene->IsReady();
    if(!DrainCampaignResources(context))return false;
    gCampaignPreparedScene=std::make_unique<Scene>();
    auto& scene=*gCampaignPreparedScene;
    scene.SetDevelopmentWorldRoute(true,false);scene.SetDevelopmentRescueJourney(true);
    scene.SetDevelopmentSupportFixture(false);
    const auto extent=ScaledRenderExtent(context.swapchainExtent,context.renderScale);
    const auto root=context.reportDirectory+"/..";
    std::string diagnostic;
    const bool ready=scene.Initialise(context.instance,context.physicalDevice,context.device,
        context.graphicsQueue,context.commandPool,extent,context.swapchainFormat,
        root+"/skeleton_biped_merged_animations_v01.glb",root+"/lich_placeholder_merged_animations_v01.glb",
        root,root,diagnostic,{},root,context.executionBackend,
        horde::vulkan::raytracing::RtSceneProfile::Showcase,context.graphicsSettings.glassEnabled,
        context.pipelineCache,context.compiledPipelineCache?&context.compiledPipelineCache->cache:nullptr);
    __android_log_print(ready?ANDROID_LOG_INFO:ANDROID_LOG_ERROR,kTag,
        "HORDE_CAMPAIGN_RESOURCE_PREP request=%llu ready=%d previous_scene_retained=1 diagnostic=%s",
        static_cast<unsigned long long>(gCampaignOwnerRequest),ready?1:0,diagnostic.c_str());
    // Candidate uploads may have submitted work even on failure. Keep ownership
    // until the real retirement proof succeeds; never destroy the prior scene.
    if(!DrainCampaignResources(context))return false;
    if(!ready){gCampaignPreparedScene.reset();return false;}
    return true;
}

void FailCampaignOnOwner(int error)
{
    gCampaignError=error;gCampaignPhase=8;
    gCampaignBarrier.Fail(gCampaignBarrier.State().request);
    if(gCampaignSession)gCampaignSession->Cancel();
    gCampaignPreparation.reset();
    if(gCampaignPreparedScene && DrainCampaignResources(gSwapchainContext))gCampaignPreparedScene.reset();
    if(!gCampaignBarrier.HasPendingCommittedLoad())gCampaignPauseGate.store(false,std::memory_order_release);
}

bool CampaignRootMatches(const std::string& root,const SwapchainContext& context)
{
    const auto path=std::filesystem::path(root).lexically_normal();
    const auto files=std::filesystem::path(context.reportDirectory).parent_path().lexically_normal();
    if(!path.is_absolute())return false;
    if(path.parent_path()==files && path.filename()=="campaign-v1")return true;
#if defined(HORDE_RT_DEBUG_CHECKPOINTS) && !defined(NDEBUG)
    const auto name=path.filename().string();
    return path.parent_path().parent_path()==files && path.parent_path().filename()=="campaign-tests" &&
        !name.empty() && name.size()<=48 && std::all_of(name.begin(),name.end(),[](unsigned char c){
            return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-';});
#else
    return false;
#endif
}

void PumpCampaignOnOwnerImpl(SwapchainContext& context)
{
    bool applyFailed=false;
    std::unique_ptr<horde::vulkan::raytracing::PresentableTinyRtScene> retiredScene;
    std::string configured;
    std::optional<CampaignCommand> command;
    {std::lock_guard lock(gCampaignMutex);configured=gCampaignConfiguredRoot;command=std::exchange(gCampaignCommand,{});}
    if(!configured.empty() && configured!=gCampaignOwnerRoot) {
        if(!CampaignRootMatches(configured,context)){gCampaignPhase=8;gCampaignError=11;PublishCampaignOnOwner(context);return;}
        // Configuration is admitted only between Activity-owned surface sessions.
        // No owner directory is scanned, migrated, removed or created here.
        gCampaignSession=std::make_unique<campaign::PlaySession>(gGameSimulation,configured);
        gCampaignOwnerRoot=configured;gCampaignPhase=1;gCampaignError=0;
        gCampaignIsolated=std::filesystem::path(configured).parent_path().filename()=="campaign-tests";
    }
    if(!gCampaignSession){PublishCampaignOnOwner(context);return;}
    if(gCampaignBarrier.State().loaded && gCampaignBarrier.KeepsPaused() &&
       gCampaignBarrier.State().surfaceGeneration==0) {
        gCampaignBarrier.ResumeOnSurface(context.surfaceGeneration);
        const auto& current=gGameSimulation.Snapshot();
        if(current.worldRoute.generation>gCampaignBarrier.State().loadedWorldGeneration)
            gCampaignBarrier.ResourcesRecreated(context.surfaceGeneration,current.tickIndex,current.worldRoute.generation);
        {std::lock_guard lock(gInputPublisherMutex);gCampaignLoadPublication=gInputMailbox.PublishedSequence();}
        gCampaignPhase=5;gCampaignPauseGate.store(true,std::memory_order_release);
    }
    if(command) {
        gCampaignOwnerRequest=command->serial;gCampaignOwnerGeneration=command->generation;
        if(command->generation!=context.surfaceGeneration || !gSurfaceSessions.IsCurrent(command->generation)) {
            gCampaignError=8;
        } else if(command->action==4) {
            if(!gCampaignBarrier.State().loaded || !gCampaignBarrier.KeepsPaused()) {
                gCampaignSession->Cancel();gCampaignPreparation.reset();
                gCampaignBarrier.Cancel(gCampaignBarrier.State().request);
                if(gCampaignPreparedScene && DrainCampaignResources(context))gCampaignPreparedScene.reset();
                gCampaignPhase=1;gCampaignError=8;gCampaignPauseGate.store(false,std::memory_order_release);
            }
            // A postcommit Back cannot undo the loaded game. Complete its
            // camera/presentation handshake while Java keeps its menu visible.
        } else if(gCampaignBarrier.State().loaded && gCampaignBarrier.KeepsPaused()) {
            gCampaignError=2;
        } else if(command->action==1) {
            gCampaignSession->RefreshCatalogue();gCampaignError=0;
            gCampaignLastAction=1;gCampaignPhase=1;
        } else if(context.captureActive || context.routeReplayActive || context.inAppBenchmark.IsRunning() ||
                  context.benchmarkSampling || context.sceneProfile==horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview) {
            gCampaignPhase=8;gCampaignError=2;gCampaignPauseGate.store(false,std::memory_order_release);
        } else if(command->action==2) {
            gCampaignLastAction=2;gCampaignError=0;
            if(gCampaignSession->SaveManual(command->slot))gCampaignPhase=2;
            else {gCampaignPhase=8;gCampaignError=gCampaignSession->State().hasLatestCheckpoint?2:3;}
        } else if(command->action==3) {
            gCampaignLastAction=3;gCampaignError=0;
            if(gCampaignBarrier.Begin(command->serial,command->generation) &&
               gCampaignSession->Load(command->slot,command->kind==0?campaign::SaveKind::Manual:campaign::SaveKind::Autosave)) {
                gCampaignPhase=3;gCampaignStartedNs=GraphicsSteadyNs();
                gCampaignPauseGate.store(true,std::memory_order_release);
            } else FailCampaignOnOwner(2);
        } else if(command->action==5) {
            gCampaignLastAction=5;gCampaignError=0;
            if(gGameSimulation.Snapshot().staticDetailLease.resourceGeneration) {
                std::lock_guard lock(gCampaignMutex);
                if(!gCampaignCommand)gCampaignCommand=command;
                gCampaignPhase=4;gCampaignPauseGate.store(true,std::memory_order_release);
            } else if(context.sceneProfile==horde::vulkan::raytracing::RtSceneProfile::Showcase &&
                      !PrepareCampaignResources(context))FailCampaignOnOwner(7);
            else {
                std::lock_guard lock(gInputPublisherMutex);
                std::lock_guard commandLock(gCampaignMutex);
                if(!gSurfaceSessions.IsCurrent(context.surfaceGeneration) ||
                   gSurfaceSessions.IsSuspended(context.surfaceGeneration) ||
                   (gCampaignCommand && gCampaignCommand->action==4)) {
                    gCampaignPhase=8;gCampaignError=8;gCampaignPauseGate.store(false,std::memory_order_release);
                } else if(gCampaignSession->BeginNewGame(command->slot)) {
                gGameSimulation= horde::gameplay::simulation::GameSimulation(
                    horde::gameplay::simulation::ProductionGameSimulationConfig());
                gGameSimulation.StartLiveDevelopmentRescueJourney(true);
                gGameSimulation.SynchronizePausedInput(gInputPublisherState,gInputMailbox.PublishedSequence());
                gCampaignNewGameYaw=gGameSimulation.Snapshot().playerYawRadians;
                gCampaignNewGamePitch=gGameSimulation.Snapshot().playerPitchRadians;
                if(gCampaignPreparedScene) {
                    std::swap(context.rtScene,*gCampaignPreparedScene);
                    retiredScene=std::move(gCampaignPreparedScene);
                    context.developmentWorldRoute=true;context.developmentRescueJourney=true;
                    context.developmentSupportFixture=false;context.capabilities.rtScene.presented=false;
                }
                horde::platform::android::ResetMusicSession(true);
                ClearPlatformGameplayEvents();gGameSimulation.ClearEvents();PublishSimulationUiState();
                gCampaignPhase=7;gCampaignPauseGate.store(false,std::memory_order_release);
                gCampaignShowcaseOverride.store(false,std::memory_order_release);
                } else {gCampaignPhase=8;gCampaignError=2;}
            }
        }
    }
    gCampaignSession->Poll();
    auto state=gCampaignSession->State();
    const auto completion=gCampaignSession->TakeCompletion();
    if(state.operation==campaign::PlayOperation::Autosave && state.outcome==campaign::PlayOutcome::Pending) {
        gCampaignLastAction=0;gCampaignPhase=2;
    }
    if(completion && completion->outcome==campaign::PlayOutcome::Failed) {
        // Keep the underlying failure at the owner completion boundary. The
        // public menu error intentionally groups IO failures; device evidence
        // still needs the actual operation, errno and publication certainty.
        const auto& result=completion->result;
        __android_log_print(ANDROID_LOG_ERROR,kTag,
            "HORDE_CAMPAIGN_STORAGE_FAILURE operation=%d slot=%u kind=%d status=%d system_error=%u codec_error=%d publication=%d durability=%d",
            static_cast<int>(completion->operation),completion->operationSlot,
            static_cast<int>(completion->operationKind),static_cast<int>(result.status),
            result.systemError,static_cast<int>(result.codecError),
            static_cast<int>(result.publication),static_cast<int>(result.durability));
        FailCampaignOnOwner(CampaignStorageError(completion->result));
    }
    else if(completion && completion->operation!=campaign::PlayOperation::Load &&
            completion->outcome==campaign::PlayOutcome::Succeeded) {
        gCampaignPhase=7;gCampaignError=0;
        if(completion->operation==campaign::PlayOperation::Autosave)gCampaignLastAction=0;
    }
    if(gCampaignPhase==3 || gCampaignPhase==4) {
        if(gCampaignLastAction==3 && GraphicsSteadyNs()-gCampaignStartedNs>60000000000ull) {
            FailCampaignOnOwner(7);PublishCampaignOnOwner(context);return;
        }
        // Existing reduced geometry restores through its bounded ordinary path.
        // Never clear a simulation lease manually to manufacture readiness.
        if(!gGameSimulation.Snapshot().staticDetailLease.resourceGeneration &&
           (context.sceneProfile!=horde::vulkan::raytracing::RtSceneProfile::Showcase ||
            context.rtScene.StaticGeometryHasCompleteContributors()) &&
           state.phase==campaign::SessionPhase::LoadReady) {
            gCampaignPhase=4;
            if(!PrepareCampaignResources(context))FailCampaignOnOwner(7);
            else if(!gCampaignPreparation) {
                gCampaignPreparation=gCampaignSession->PrepareLoad();
                if(!gCampaignPreparation)FailCampaignOnOwner(9);
            }
        }
        if(gCampaignPreparation) {
            auto& scene=gCampaignPreparedScene?*gCampaignPreparedScene:context.rtScene;
            const auto destination=gCampaignPreparation->destination;
            if(scene.WorldZoneReadiness(destination)==horde::gameplay::simulation::ZoneReadiness::Ready) {
                if(gCampaignPreparedScene && !DrainCampaignResources(context)) {
                    FailCampaignOnOwner(7);PublishCampaignOnOwner(context);return;
                }
                std::lock_guard inputLock(gInputPublisherMutex);
                std::lock_guard commandLock(gCampaignMutex);
                // Linearize against accepted Home/Stop/Cancel. No IO or GPU work
                // occurs inside this admission gate.
                if(gSurfaceSessions.IsCurrent(context.surfaceGeneration) &&
                   !gSurfaceSessions.IsSuspended(context.surfaceGeneration) &&
                   !(gCampaignCommand && gCampaignCommand->action==4)) {
                    auto input=gInputPublisherState;
                    if(gCampaignSession->CommitLoad(*gCampaignPreparation,destination,
                        scene.ChapterTerrainSurface(),input,gInputMailbox.PublishedSequence())) {
                        if(gCampaignPreparedScene) {
                            std::swap(context.rtScene,*gCampaignPreparedScene);
                            retiredScene=std::move(gCampaignPreparedScene);
                            context.sceneProfile=horde::vulkan::raytracing::RtSceneProfile::Showcase;
                            context.developmentWorldRoute=true;context.developmentRescueJourney=true;
                            context.developmentSupportFixture=false;
                        }
                        const auto& loaded=gGameSimulation.Snapshot();
                        gCampaignLoadPublication=gInputMailbox.PublishedSequence();
                        gCampaignBarrier.CommitLoaded(gCampaignBarrier.State().request,context.surfaceGeneration,
                            loaded.playerYawRadians,loaded.playerPitchRadians,loaded.tickIndex,loaded.worldRoute.generation);
                        gCampaignPhase=5;gCampaignPreparation.reset();
                        gCampaignShowcaseOverride.store(true,std::memory_order_release);
                        context.capabilities.rtScene.presented=false;
                        context.lastInputOwnerSteadyNs=GraphicsSteadyNs();gGameSimulation.ResetTiming();
                        horde::platform::android::ResetMusicSession(true);
                        ClearPlatformGameplayEvents();gGameSimulation.ClearEvents();PublishSimulationUiState();
                    } else applyFailed=true;
                }
            }
        }
    }
    if(applyFailed)FailCampaignOnOwner(9);
    if(gCampaignPhase==5) {
        std::lock_guard inputLock(gInputPublisherMutex);
        std::lock_guard commandLock(gCampaignMutex);
        const auto barrier=gCampaignBarrier.State();
        if(gCampaignAckToken==barrier.rebaseToken && gCampaignAckGeneration==context.surfaceGeneration &&
           gCampaignAckPublication>gCampaignLoadPublication &&
           CampaignNeutralInput(gInputPublisherState,barrier) &&
           gCampaignAckToken && gCampaignBarrier.Acknowledge(gCampaignAckToken,gCampaignAckGeneration)) {
            gCampaignAckToken=0;gCampaignPhase=6;
        }
    }
    if(context.sceneProfile==horde::vulkan::raytracing::RtSceneProfile::Showcase &&
       !gCampaignPauseGate.load(std::memory_order_acquire) && !context.captureActive &&
       !context.routeReplayActive && !context.inAppBenchmark.IsRunning() && !context.benchmarkSampling)
        gCampaignSession->ObserveSafeBoundary();
    PublishCampaignOnOwner(context);
}

void PumpCampaignOnOwner(SwapchainContext& context)
{
    try {PumpCampaignOnOwnerImpl(context);}
    catch(const std::exception& error) {
        __android_log_print(ANDROID_LOG_ERROR,kTag,"HORDE_CAMPAIGN_FAILURE diagnostic=%s",error.what());
        if(gCampaignBarrier.State().loaded && gCampaignBarrier.KeepsPaused()) {
            gCampaignPhase=gCampaignBarrier.State().phase==CampaignBarrier::Phase::AwaitingJavaRebase?5:6;
            gCampaignError=9;gCampaignPauseGate.store(true,std::memory_order_release);
        } else FailCampaignOnOwner(6);
        PublishCampaignOnOwner(context);
    }
}


void ObserveCampaignPresentationOnOwner(const SwapchainContext& context,bool actuallyPresented)
{
    if(actuallyPresented && gCampaignPhase==6 &&
       gGameSimulation.Snapshot().worldRoute.generation==gCampaignBarrier.State().loadedWorldGeneration &&
       gCampaignBarrier.Presented(context.surfaceGeneration,gCampaignFrameTick,gCampaignFrameWorld)) {
        gCampaignPhase=7;gCampaignError=0;gCampaignPauseGate.store(false,std::memory_order_release);
        PublishCampaignOnOwner(context);
    }
}

void RebindCampaignResourcesOnOwner(const SwapchainContext& context)
{
    const auto& current=gGameSimulation.Snapshot();
    if(gCampaignBarrier.ResourcesRecreated(context.surfaceGeneration,current.tickIndex,current.worldRoute.generation)) {
        std::lock_guard lock(gInputPublisherMutex);
        gCampaignLoadPublication=gInputMailbox.PublishedSequence();gCampaignAckToken=0;
        gCampaignPhase=5;gCampaignPauseGate.store(true,std::memory_order_release);
    }
    PublishCampaignOnOwner(context);
}

void SuspendCampaignOnOwner(const SwapchainContext& context)
{
    if(gCampaignBarrier.State().loaded && gCampaignBarrier.KeepsPaused()) {
        if(gCampaignBarrier.InputSuspended(context.surfaceGeneration)) {
            std::lock_guard lock(gInputPublisherMutex);
            gCampaignLoadPublication=gInputMailbox.PublishedSequence();gCampaignAckToken=0;
            gCampaignPhase=5;gCampaignPauseGate.store(true,std::memory_order_release);
        }
    } else if(gCampaignPhase==3 || gCampaignPhase==4) {
        gCampaignSession->Cancel();gCampaignPreparation.reset();
        gCampaignBarrier.Cancel(gCampaignBarrier.State().request);
        if(gCampaignPreparedScene && DrainCampaignResources(gSwapchainContext))gCampaignPreparedScene.reset();
        gCampaignPhase=8;gCampaignError=8;gCampaignPauseGate.store(false,std::memory_order_release);
    }
    PublishCampaignOnOwner(context);
}

bool RetireCampaignOnOwner(std::uint64_t generation)
{
    const bool loaded=gCampaignBarrier.State().loaded && gCampaignBarrier.KeepsPaused();
    gCampaignBarrier.SurfaceRetired(generation);
    if(!loaded) {
        if(gCampaignSession)gCampaignSession->Cancel();
        gCampaignPreparation.reset();gCampaignPauseGate.store(false,std::memory_order_release);
        if(gCampaignPhase>=3 && gCampaignPhase<=6){gCampaignPhase=8;gCampaignError=8;}
    }
    if(gCampaignPreparedScene) {
        if(!DrainCampaignResources(gSwapchainContext))return false;
        gCampaignPreparedScene.reset();
    }
    return true;
}
