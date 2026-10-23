// Included by DiagnosticWindow.cpp after its scene retirement helpers.
using CampaignMenu=horde::platform::windows::WindowsCampaignMenu;
namespace campaign=horde::gameplay::campaign;

std::filesystem::path WindowsCampaignDirectory()
{
#if defined(_DEBUG)
    if(gOpeningBenchmarkMenuValidation)return gCampaignValidationDirectory;
#endif
    PWSTR raw=nullptr;
    const auto result=SHGetKnownFolderPath(FOLDERID_LocalAppData,KF_FLAG_DONT_VERIFY,nullptr,&raw);
    std::filesystem::path root;
    if(SUCCEEDED(result) && raw)root=std::filesystem::path(raw)/"HordeLanternRT"/"campaign-v1";
    if(raw)CoTaskMemFree(raw);
    return root;
}

bool DrainCampaignResources(VulkanSurfaceContext& ctx)
{
    if(!ConsumePendingImageAcquire(ctx)){ctx.campaignRetirementUnproved=true;return false;}
    const auto idle=vkDeviceWaitIdle(ctx.device);
    bool ready=idle==VK_SUCCESS && CompleteRtEvidenceAfterDeviceIdle(ctx,idle) &&
        ctx.presentCompletionFences.Drain()==VK_SUCCESS;
    if(ready)for(auto buffer:ctx.commandBuffers)
        if(vkResetCommandBuffer(buffer,0)!=VK_SUCCESS){ready=false;break;}
    ctx.campaignRetirementUnproved=!ready;
    return ready;
}

bool PrepareCampaignResources(VulkanSurfaceContext& ctx)
{
    using Scene=horde::vulkan::raytracing::PresentableTinyRtScene;
    if(ctx.rtScene.Profile()==horde::vulkan::raytracing::RtSceneProfile::Showcase &&
       ctx.simulation.Snapshot().developmentRescueJourney && ctx.rtScene.IsReady() &&
       ctx.rtScene.StaticGeometryHasCompleteContributors())return true;
    if(ctx.campaignPreparedScene)return ctx.campaignPreparedScene->IsReady();
    if(!DrainCampaignResources(ctx))return false;
    ctx.campaignPreparedScene=std::make_unique<Scene>();
    auto& scene=*ctx.campaignPreparedScene;
    scene.SetDevelopmentWorldRoute(true,false);scene.SetDevelopmentRescueJourney(true);
    scene.SetDevelopmentSupportFixture(false);
    const auto root=ResolveAssetRoot();
    std::string diagnostic;
    const bool ready=scene.Initialise(ctx.instance,ctx.physicalDevice,ctx.device,ctx.graphicsQueue,
        ctx.commandPool,ScaledRenderExtent(ctx.swapchainExtent,ctx.renderScale),ctx.swapchainFormat,
        (root/"models/enemies/meshy/skeleton_biped_merged_animations_v01.glb").string(),
        (root/"models/enemies/meshy/lich_placeholder_merged_animations_v01.glb").string(),
        (root/"textures/polyhaven/mobile_1k").string(),(root/"textures/meshy/lich_placeholder_v01").string(),
        diagnostic,{},root.string(),ctx.executionBackend,
        horde::vulkan::raytracing::RtSceneProfile::Showcase,ctx.requestedGlassEnabled,ctx.pipelineCache);
    std::cerr<<"HORDE_WINDOWS_CAMPAIGN prepare ready="<<ready<<" previous_scene_retained=1 diagnostic="<<diagnostic<<'\n';
    if(!DrainCampaignResources(ctx))return false;
    if(!ready){ctx.campaignPreparedScene.reset();ctx.campaignMessage=diagnostic;return false;}
    return true;
}

void RefreshCampaignMenu(VulkanSurfaceContext& ctx)
{
    if(!ctx.campaignMenu || !ctx.campaign)return;
    CampaignMenu::View view;
    view.state=ctx.campaign->State();view.catalogue=ctx.campaign->Catalogue();
    view.catalogueRevision=view.state.catalogueRevision;view.phase=ctx.campaignPhase;
    view.message=ctx.campaignMessage.empty()?ctx.campaignStorageWarning:ctx.campaignMessage;
    view.canResumeLoaded=ctx.campaignBarrier.State().phase==campaign::CampaignLoadBarrier::Phase::Complete &&
        ctx.simulation.Snapshot().playerVitals.phase==horde::gameplay::PlayerLifePhase::Alive &&
        ctx.simulation.Snapshot().playerVitals.vitality>0;
    const bool unknown=view.state.result.status==campaign::storage::Status::PublishedDurabilityUnknown ||
        view.state.result.status==campaign::storage::Status::PublishOutcomeUnknown;
    bool checked=true;for(const auto& entry:view.catalogue)if(entry.status==campaign::CatalogueStatus::Checking || entry.status==campaign::CatalogueStatus::ReadError)checked=false;
    if(unknown && checked && ctx.campaignUncertaintyRecheck==view.state.operationRevision)ctx.campaignVerifiedUncertainty=view.state.operationRevision;
    view.writeUncertain=unknown && ctx.campaignVerifiedUncertainty!=view.state.operationRevision;
    ctx.campaignMenu->Refresh(view);
}

void OpenCampaignMenu(VulkanSurfaceContext& ctx,CampaignMenu::Page page)
{
    if(!ctx.campaignMenu || !ctx.campaign || ctx.benchmark.IsRunning() || ctx.graphicsVisible ||
       ctx.rtScene.Profile()==horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview)return;
    const auto origin=ctx.deathOverlayVisible?CampaignMenu::Origin::Death:
        ctx.entryMenuVisible?CampaignMenu::Origin::Entry:CampaignMenu::Origin::Pause;
    ctx.pauseMenuVisible=true;ctx.campaignPause=true;
    ctx.campaignPhase=CampaignMenu::Phase::Ready;ctx.campaignMessage.clear();
    ctx.campaign->RefreshCatalogue();RefreshCampaignMenu(ctx);ctx.campaignMenu->Open(page,origin);
    ApplyOverlayState(ctx);RECT rect{};GetClientRect(ctx.windowHandle,&rect);
    LayoutOverlayControls(ctx.windowHandle,rect.right,rect.bottom);
    FocusGameControl(GetDlgItem(ctx.windowHandle,CampaignMenu::BackId));
}

void QueueCampaignCommand(VulkanSurfaceContext& ctx,CampaignMenu::Command command)
{
    // Native messages can deliver Choose then Back before the owner's next IO
    // pump. Publish the pause/busy view immediately and let that cancellation
    // revoke the queued operation rather than silently dropping Back.
    if(command.action==CampaignMenu::Action::Cancel || command.action==CampaignMenu::Action::Close){
        if(!ctx.campaignBarrier.HasPendingCommittedLoad())ctx.campaignCommand=command;
    } else if(!ctx.campaignCommand){
        ctx.campaignCommand=command;
        if(command.action==CampaignMenu::Action::Load)ctx.campaignPhase=CampaignMenu::Phase::Reading;
        if(command.action==CampaignMenu::Action::SaveManual)ctx.campaignPhase=CampaignMenu::Phase::Saving;
        if(command.action==CampaignMenu::Action::BeginNewGame)ctx.campaignPhase=CampaignMenu::Phase::Preparing;
    }
    RefreshCampaignMenu(ctx);
}

void RebaseCampaignInput(VulkanSurfaceContext& ctx)
{
    ClearDesktopInput(ctx);
    auto input=ctx.simulationInput;
    input.commands={ctx.attackSequence,ctx.parrySequence,ctx.dodgeSequence,ctx.routeResetSequence,
        ctx.retrySequence,ctx.interactSequence,ctx.toggleHeldLightPoseSequence,ctx.runToggleSequence,
        ctx.clearRunIntentSequence,ctx.combatTeachingSkipSequence,ctx.combatTeachingReplaySequence,ctx.dialogueSkipSequence};
    input.commands.crouchToggle=ctx.crouchToggleSequence;
    input.commands.clearCrouchIntent=ctx.clearCrouchIntentSequence;
    input.combatEdgeHistory={};input.dialogueCompletionLine=0;input.dialogueCompletionGeneration=0;
    input.paused=true;input.hasAuthoritativePlayerPose=false;
    input.moveForward=input.moveStrafe=0;input.runHeld=input.crouchHeld=false;
    input.authoritativePlayerX=input.authoritativePlayerZ=0;
    input.yawRadians=ctx.simulation.Snapshot().playerYawRadians;
    input.pitchRadians=ctx.simulation.Snapshot().playerPitchRadians;
    ctx.cameraYaw=input.yawRadians;ctx.cameraPitch=input.pitchRadians;
    ctx.simulationInput=input;
    ctx.simulation.SynchronizePausedInput(input,++ctx.inputPublicationSequence,
        horde::gameplay::simulation::PausedInputPolicy::DiscardAllCommands);
    // A real fresh paused owner publication releases the simulation restore floor.
    ctx.simulation.StepFixed(input,0,++ctx.inputPublicationSequence);
    const auto barrier=ctx.campaignBarrier.State();
    if(barrier.MatchesRebasePose(input.yawRadians,input.pitchRadians) &&
       ctx.campaignBarrier.Acknowledge(barrier.rebaseToken,ctx.campaignSurfaceGeneration))
        ctx.campaignPhase=CampaignMenu::Phase::Presenting;
}

void FinishCampaignCommit(VulkanSurfaceContext& ctx)
{
    if(ctx.campaignPreparedScene){
        std::swap(ctx.rtScene,*ctx.campaignPreparedScene);
        ctx.campaignPreparedScene.reset(); // Last reference was retired before commit.
    }
    ctx.sceneProfile=horde::vulkan::raytracing::RtSceneProfile::Showcase;
    ctx.developmentWorldRoute=ctx.developmentRescueJourney=true;ctx.stagedWorldPreparation=false;
    ctx.developmentVerticalProof=false;ctx.developmentCheckpoint.clear();
    ctx.entryMenuVisible=ctx.entryMoreVisible=ctx.entryPlayQueued=ctx.entryPlayHandoffPending=false;
    ctx.entryLoadingVisible=false;ctx.deathOverlayVisible=ctx.endingOverlayVisible=false;
    ctx.sceneProfileDirty=ctx.glassGeometryDirty=ctx.renderScaleDirty=false;
    ctx.delayedFeedback.Clear();ctx.simulation.ClearEvents();++ctx.musicResetToken;
    ++ctx.musicEventLifetimeToken; // Load/New Game replaced the semantic event queue.
    if(!gCampaignHiddenValidation) {
        SpatialAudioEngine().StopLoop("chapter_dialogue");
        SpatialAudioEngine().StopLoop("chapter_ambience");
    }
    ctx.playingDialogueLine=0;ctx.playingDialogueGeneration=0;
    ctx.dialogueCompletionLine=0;ctx.dialogueCompletionGeneration=0;
    ctx.campaignPause=true;ctx.pauseMenuVisible=true;ctx.simulationPaused=true;
    ctx.simulation.ResetTiming();ctx.lastControlSteadyNs=0;
    MirrorSimulationSnapshot(ctx);
    const auto& loaded=ctx.simulation.Snapshot();
    ctx.campaignBarrier.CommitLoaded(ctx.campaignBarrier.State().request,ctx.campaignSurfaceGeneration,
        loaded.playerYawRadians,loaded.playerPitchRadians,loaded.tickIndex,loaded.worldRoute.generation);
    ctx.campaignPhase=CampaignMenu::Phase::Rebase;
    if(ctx.rtFrameEvidenceInitialised)(void)ctx.rtFrameEvidence.Recreate(
        horde::telemetry::RtResourceResetReason::DiagnosticResourceReplacement,CurrentInitialGpuEvidenceStatus(ctx));
    ctx.gpuFrameTimer.ResetAfterDeviceIdle();
    RebaseCampaignInput(ctx);ApplyOverlayState(ctx);
}

void PumpCampaign(VulkanSurfaceContext& ctx)
{
    if(!ctx.campaign)return;
    const auto fail=[&](const std::string& message){
        ctx.campaignMessage=message;ctx.campaignPhase=CampaignMenu::Phase::Failed;
        ctx.campaignBarrier.Fail(ctx.campaignBarrier.State().request);
        if(!ctx.campaignBarrier.HasPendingCommittedLoad()){
            ctx.campaign->Cancel();ctx.campaignPreparation.reset();
            if(ctx.campaignPreparedScene && DrainCampaignResources(ctx))ctx.campaignPreparedScene.reset();
        }
    };
    try {
        if(ctx.campaignCommand){
            const auto command=*ctx.campaignCommand;ctx.campaignCommand.reset();
            if(command.action==CampaignMenu::Action::Cancel || command.action==CampaignMenu::Action::Close){
                if(!ctx.campaignBarrier.HasPendingCommittedLoad()){
                    ctx.campaign->Cancel();ctx.campaignPreparation.reset();
                    ctx.campaignBarrier.Cancel(ctx.campaignBarrier.State().request);
                    if(ctx.campaignPreparedScene && !DrainCampaignResources(ctx)){fail("GPU retirement failed; game remains paused.");return;}
                    ctx.campaignPreparedScene.reset();ctx.campaignPhase=CampaignMenu::Phase::Ready;
                    ctx.campaignMenu->Close();RefreshCampaignMenu(ctx);ctx.campaignPause=false;
                    ApplyOverlayState(ctx);RECT r{};GetClientRect(ctx.windowHandle,&r);LayoutOverlayControls(ctx.windowHandle,r.right,r.bottom);
                }
            } else if(command.action==CampaignMenu::Action::Resume){
                if(ctx.campaignBarrier.State().phase==campaign::CampaignLoadBarrier::Phase::Complete &&
                   ctx.simulation.Snapshot().playerVitals.phase==horde::gameplay::PlayerLifePhase::Alive){
                    ctx.campaignMenu->Close();ctx.campaignPause=false;
                    ShowPauseMenu(ctx,false);
                }
            } else if(!ctx.campaignBarrier.HasPendingCommittedLoad()) {
                if(command.action==CampaignMenu::Action::Refresh){ctx.campaign->RefreshCatalogue();ctx.campaignUncertaintyRecheck=ctx.campaign->State().operationRevision;ctx.campaignPhase=CampaignMenu::Phase::Ready;}
                else if(command.action==CampaignMenu::Action::SaveManual){
                    if(ctx.campaign->SaveManual(command.slot)){ctx.campaignPhase=CampaignMenu::Phase::Saving;ctx.campaignMessage.clear();}
                    else fail("No safe checkpoint is available, or another operation is pending.");
                } else if(command.action==CampaignMenu::Action::Load){
                    ctx.campaignNewGame=false;
                    if(ctx.campaignBarrier.Begin(++ctx.campaignSerial,ctx.campaignSurfaceGeneration) && ctx.campaign->Load(command.slot,command.kind)){
                        ctx.campaignPhase=CampaignMenu::Phase::Reading;ctx.campaignStarted=std::chrono::steady_clock::now();ctx.campaignMessage.clear();
                    } else fail("Load could not start. Current game retained.");
                } else if(command.action==CampaignMenu::Action::BeginNewGame){
                    if(ctx.simulation.Snapshot().staticDetailLease.resourceGeneration){fail("World detail is restoring. Retry after it has completed.");}
                    else if(!ctx.campaignBarrier.Begin(++ctx.campaignSerial,ctx.campaignSurfaceGeneration) || !PrepareCampaignResources(ctx))fail("New game resources could not prepare. Current game retained.");
                    else {
                        auto candidate=std::make_unique<horde::gameplay::simulation::GameSimulation>(horde::gameplay::simulation::ProductionGameSimulationConfig());
                        candidate->StartLiveDevelopmentRescueJourney(true);
                        if(!DrainCampaignResources(ctx) || !ctx.campaign->BeginNewGame(command.slot))fail("New game could not start. Current game retained.");
                        else {ctx.simulation=std::move(*candidate);ctx.campaignNewGame=true;FinishCampaignCommit(ctx);}
                    }
                }
            }
        }
        ctx.campaign->Poll();
        if(const auto completion=ctx.campaign->TakeCompletion()){
            if(completion->outcome==campaign::PlayOutcome::Failed)fail("Storage operation failed. Recheck slots before another write.");
            else if(completion->operation!=campaign::PlayOperation::Load){ctx.campaignPhase=CampaignMenu::Phase::Succeeded;ctx.campaignMessage="Checkpoint saved.";}
        }
        const auto state=ctx.campaign->State();
        if(ctx.campaignPhase==CampaignMenu::Phase::Reading || ctx.campaignPhase==CampaignMenu::Phase::Preparing){
            if(std::chrono::steady_clock::now()-ctx.campaignStarted>std::chrono::seconds(60))fail("Load preparation timed out. Current game retained.");
            else if(!gCampaignValidationHoldPreparation && state.phase==campaign::SessionPhase::LoadReady && !ctx.simulation.Snapshot().staticDetailLease.resourceGeneration &&
                (ctx.rtScene.Profile()!=horde::vulkan::raytracing::RtSceneProfile::Showcase || ctx.rtScene.StaticGeometryHasCompleteContributors())){
                ctx.campaignPhase=CampaignMenu::Phase::Preparing;
                if(!PrepareCampaignResources(ctx))fail("RT resources could not prepare. Current game retained.");
                else {ctx.campaignPreparation=ctx.campaign->PrepareLoad();if(!ctx.campaignPreparation)fail("Checkpoint could not prepare. Current game retained.");}
            }
            if(ctx.campaignPreparation){
                auto& scene=ctx.campaignPreparedScene?*ctx.campaignPreparedScene:ctx.rtScene;
                const auto destination=ctx.campaignPreparation->destination;
                if(scene.WorldZoneReadiness(destination)==horde::gameplay::simulation::ZoneReadiness::Ready){
                    if(!DrainCampaignResources(ctx))fail("GPU retirement failed. Game remains paused.");
                    else if(ctx.campaign->CommitLoad(*ctx.campaignPreparation,destination,scene.ChapterTerrainSurface(),ctx.simulationInput,ctx.inputPublicationSequence)){
                        ctx.campaignPreparation.reset();FinishCampaignCommit(ctx);
                    } else fail("Checkpoint could not commit. Current game retained.");
                }
            }
        }
        if(ctx.campaignBarrier.HasPendingCommittedLoad()){
            const auto& current=ctx.simulation.Snapshot();
            if(!ctx.campaignBarrier.State().surfaceGeneration)ctx.campaignBarrier.ResumeOnSurface(ctx.campaignSurfaceGeneration);
            if(current.worldRoute.generation>ctx.campaignBarrier.State().loadedWorldGeneration)
                ctx.campaignBarrier.ResourcesRecreated(ctx.campaignSurfaceGeneration,current.tickIndex,current.worldRoute.generation);
            if(ctx.campaignBarrier.State().phase==campaign::CampaignLoadBarrier::Phase::AwaitingInputRebase)RebaseCampaignInput(ctx);
        }
        if(!ctx.campaignPause && ctx.rtScene.Profile()==horde::vulkan::raytracing::RtSceneProfile::Showcase)
            ctx.campaign->ObserveSafeBoundary();
        RefreshCampaignMenu(ctx);
    } catch(const std::exception& e){fail(std::string("Campaign operation failed: ")+e.what());RefreshCampaignMenu(ctx);}
}

void ObserveCampaignPresentation(VulkanSurfaceContext& ctx,bool presented)
{
    if(presented && ctx.lastFramePresentation==horde::telemetry::RtPresentationOutcome::Presented &&
       ctx.campaignBarrier.Presented(ctx.campaignSurfaceGeneration,ctx.campaignFrameTick,ctx.campaignFrameWorld)){
        ctx.campaignPhase=CampaignMenu::Phase::Succeeded;
        ctx.campaignMessage="Loaded game presented by hardware RT. Resume when ready.";
        RefreshCampaignMenu(ctx);
        if(ctx.campaignNewGame){ctx.campaignNewGame=false;ctx.campaignMenu->Close();ctx.campaignPause=false;ShowPauseMenu(ctx,false);}
    }
}
