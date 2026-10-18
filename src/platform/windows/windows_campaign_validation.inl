#if defined(_DEBUG)
class HiddenCampaignFaults final:public campaign::storage::FaultInjector {
public:
    std::atomic<bool> failNextRead=false;
    bool Fail(campaign::storage::Operation operation,std::size_t) override {
        return operation==campaign::storage::Operation::Read && failNextRead.exchange(false);
    }
} gHiddenCampaignFaults;
// Explicit isolated hidden-HWND automation. It exercises real WM_COMMAND,
// worker IO, simulation restore and BLAS/TLAS RT presentation without activation.
int RunHiddenCampaignValidation(VulkanSurfaceContext& ctx)
{
    const auto foreground=GetForegroundWindow();
    std::ostringstream receipt;unsigned checks=0,failures=0;
    receipt<<"{\"schema\":1,\"evidenceClass\":\"hidden native HWND commands and hardware RT swapchain; seeded saves\",\"checks\":[";
    const auto check=[&](bool ok,const char* name){
        if(checks++)receipt<<',';if(!ok)++failures;
        receipt<<"{\"name\":\""<<name<<"\",\"pass\":"<<(ok?"true":"false")<<'}';
        std::cerr<<"HORDE_WINDOWS_CAMPAIGN_CHECK "<<name<<" pass="<<ok<<'\n';return ok;
    };
    const auto click=[&](int id){SendMessageA(ctx.windowHandle,WM_COMMAND,MAKEWPARAM(id,BN_CLICKED),reinterpret_cast<LPARAM>(GetDlgItem(ctx.windowHandle,id)));};
    const auto until=[&](auto done,bool render){
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(70);
        do {
            PumpCampaign(ctx);
            if(done())return true;
            if(ctx.campaignRetirementUnproved)return false;
            if(render){bool presented=false;if(!RenderFrame(ctx,VkClearColorValue{{0.0f,0.0f,0.0f,1.0f}},presented))return false;
                FinishGraphicsFrame(ctx,presented);ObserveCampaignPresentation(ctx,presented);}
            else std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }while(std::chrono::steady_clock::now()<deadline);
        return false;
    };
    const auto catalogueReady=[&]{for(const auto& entry:ctx.campaign->Catalogue())if(entry.status==campaign::CatalogueStatus::Checking)return false;return true;};
    const auto complete=[&]{return ctx.campaignBarrier.State().phase==campaign::CampaignLoadBarrier::Phase::Complete;};
    bool proceed=check(ctx.campaign && ctx.campaignMenu && ctx.campaignIsolated && !IsWindowVisible(ctx.windowHandle),"isolated root and hidden owner");
    if(proceed){
        ApplyOverlayState(ctx);
        click(kEntryMoreButtonId);click(kCampaignLoadButtonId);
        proceed=check(ctx.campaignMenu->Visible() && until(catalogueReady,false),"entry More opens actual six-record load panel");
    }
    if(proceed){
        click(CampaignMenu::FirstChoiceId);click(CampaignMenu::BackId);PumpCampaign(ctx);
        check(!ctx.campaignMenu->Visible() && !ctx.campaign->State().activeSlot,
            "Choose then Back before owner pump revokes queued Load");
        click(kCampaignLoadButtonId);proceed=until(catalogueReady,false);
        // Queue stale desktop intent before load. The saved yaw, neutral held
        // controls and consumed sequence floors must survive the actual frame.
        ctx.cameraYaw=2.0f;ctx.cameraPitch=.2f;ctx.forwardHeld=ctx.runHeld=ctx.crouchHeld=true;
        ++ctx.attackSequence;++ctx.retrySequence;++ctx.routeResetSequence;++ctx.dialogueSkipSequence;
        gCampaignValidationHoldPreparation=true;click(CampaignMenu::FirstChoiceId);PumpCampaign(ctx);
        check(ctx.campaignPhase==CampaignMenu::Phase::Reading && ctx.campaignPause,"load starts paused through WM_COMMAND");
        click(CampaignMenu::BackId);PumpCampaign(ctx);gCampaignValidationHoldPreparation=false;
        check(!ctx.campaignMenu->Visible() && !ctx.campaign->State().activeSlot,"precommit Back retains entry game and no active slot");
        click(kCampaignLoadButtonId);
        proceed=until(catalogueReady,false);
        click(CampaignMenu::FirstChoiceId);
        proceed=check(proceed && until(complete,true),"saved lower checkpoint reaches matching fresh hardware RT present");
    }
    if(proceed){
        const auto& saved=ctx.simulation.Snapshot();
        check(saved.playerVitals.vitality==1 && saved.developmentRescueJourney && ctx.campaign->State().activeSlot==0 &&
            ctx.campaignPause && ctx.simulationPaused && !ctx.deathOverlayVisible,"load adopts saved hearts and slot while result stays paused");
        check(!ctx.forwardHeld && !ctx.runHeld && !ctx.crouchHeld && ctx.cameraYaw==saved.playerYawRadians &&
            ctx.cameraPitch==saved.playerPitchRadians && ctx.simulationInput.moveForward==0 && !ctx.simulationInput.hasAuthoritativePlayerPose,
            "loaded camera and desktop input rebased without retained movement or pose");
        check(IsWindowEnabled(GetDlgItem(ctx.windowHandle,CampaignMenu::ResumeId)),"actual Resume control enabled only after matching RT frame");
        std::string imageError;
        if(DrainCampaignResources(ctx)){
            horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
            check(ctx.rtScene.CaptureStorageImage(image,imageError) && WriteRgbaPng(gCampaignValidationDirectory/"windows-loaded-lower.png",image,imageError),"loaded RT storage image read back for inspection");
        } else check(false,"loaded RT storage image read back for inspection");
        click(CampaignMenu::ResumeId);PumpCampaign(ctx);
        check(!ctx.campaignMenu->Visible() && !ctx.simulationPaused && !ctx.campaignPause,"explicit native Resume releases loaded game");
        ShowPauseMenu(ctx,true);click(kCampaignLoadButtonId);proceed=until(catalogueReady,false);
        click(CampaignMenu::FirstChoiceId+2);click(CampaignMenu::BackId);PumpCampaign(ctx);
        check(!ctx.campaignMenu->Visible() && ctx.campaign->State().activeSlot==0 &&
            ctx.simulation.Snapshot().playerVitals.vitality==1,
            "queued cancellation after retained Load success preserves current game and usable chooser");
        click(kCampaignLoadButtonId);proceed=proceed && until(catalogueReady,false);
        const auto retained=ctx.simulation.Snapshot();
        gHiddenCampaignFaults.failNextRead=true;click(CampaignMenu::FirstChoiceId+2);PumpCampaign(ctx);
        proceed=check(proceed && until([&]{return ctx.campaignPhase==CampaignMenu::Phase::Failed;},false) &&
            ctx.campaign->State().outcome==campaign::PlayOutcome::Failed && ctx.simulation.Snapshot().tickIndex==retained.tickIndex &&
            ctx.simulation.Snapshot().playerVitals.vitality==retained.playerVitals.vitality && ctx.campaign->State().activeSlot==0,
            "actual worker read failure retains live game and active slot with failed native result");
        char errorText[512]{};GetWindowTextA(GetDlgItem(ctx.windowHandle,CampaignMenu::StatusId),errorText,sizeof(errorText));
        check(std::string_view(errorText).find("failed")!=std::string_view::npos,"actual failure status text is visible in native panel");
        click(CampaignMenu::BackId);PumpCampaign(ctx);
        ShowPauseMenu(ctx,true);click(kCampaignSaveButtonId);
        proceed=check(ctx.campaignMenu->Visible() && until(catalogueReady,false),"pause opens actual Save panel after retained Load receipt");
        if(proceed){
            click(CampaignMenu::FirstChoiceId+2);
            check(IsWindowVisible(GetDlgItem(ctx.windowHandle,CampaignMenu::ConfirmId))==FALSE &&
                (GetWindowLongPtrA(GetDlgItem(ctx.windowHandle,CampaignMenu::ConfirmId),GWL_STYLE)&WS_VISIBLE),"occupied slot requires confirmation within hidden owner");
            click(CampaignMenu::ConfirmId);PumpCampaign(ctx);
            proceed=check(until([&]{return ctx.campaign->State().outcome!=campaign::PlayOutcome::Pending;},false) &&
                ctx.campaign->State().outcome==campaign::PlayOutcome::Succeeded && ctx.campaign->State().operationSlot==2 &&
                ctx.campaign->State().activeSlot==0,"confirmed manual save writes chosen slot without retargeting active autosave");
            campaign::storage::StoredCheckpoint stored;
            check(campaign::storage::Read(gCampaignValidationDirectory,campaign::SaveBasename(*campaign::SaveTarget(2,campaign::SaveKind::Manual)),stored).status==campaign::storage::Status::Read &&
                stored.checkpoint.vitality==1,"native manual save has an actual durable readable record");
            click(CampaignMenu::BackId);PumpCampaign(ctx);
        }
    }
    if(proceed){
        // Diagnostic death seed uses genuine shared combat damage at the existing
        // test encounter, not a mutated life/vitality field. This is deliberately
        // not evidence of an ordinary full-route death or earning this save.
        ctx.simulation=horde::gameplay::simulation::GameSimulation(horde::gameplay::simulation::ProductionGameSimulationConfig());
        horde::gameplay::simulation::InputSnapshot damage;
        damage.hasAuthoritativePlayerPose=true;damage.authoritativePlayerX=horde::gameplay::kWaterfallSkeletonPairCenter.x;
        damage.authoritativePlayerZ=horde::gameplay::kWaterfallSkeletonPairCenter.z;damage.damageEnabled=true;damage.tutorialEnabled=false;
        for(unsigned i=0;i<900 && ctx.simulation.Snapshot().playerVitals.phase!=horde::gameplay::PlayerLifePhase::Dead;++i)
            ctx.simulation.StepFixed(damage);
        MirrorSimulationSnapshot(ctx);ShowDeathMenu(ctx);
        std::cerr<<"HORDE_WINDOWS_CAMPAIGN death_seed phase="<<static_cast<int>(ctx.simulation.Snapshot().playerVitals.phase)
            <<" vitality="<<ctx.simulation.Snapshot().playerVitals.vitality<<'\n';
        proceed=check(ctx.deathOverlayVisible && ctx.simulation.Snapshot().playerVitals.phase==horde::gameplay::PlayerLifePhase::Dead,
            "diagnostic encounter produces genuine combat death and actual death card");
        if(proceed){
            click(kCampaignLoadButtonId);proceed=until(catalogueReady,false);
            gCampaignValidationHoldPreparation=true;click(CampaignMenu::FirstChoiceId+2);PumpCampaign(ctx);click(CampaignMenu::BackId);PumpCampaign(ctx);gCampaignValidationHoldPreparation=false;
            check(!ctx.campaignMenu->Visible() && ctx.deathOverlayVisible && ctx.simulation.Snapshot().playerVitals.vitality==0,
                "precommit death Load cancellation returns original dead game");
            click(kCampaignLoadButtonId);proceed=proceed && until(catalogueReady,false);
            click(CampaignMenu::FirstChoiceId+2);
            proceed=check(proceed && until(complete,true),"death-origin Load reaches fresh RT frame and saved alive state");
            if(proceed){
                check(ctx.simulation.Snapshot().playerVitals.vitality==2 && !ctx.deathOverlayVisible && ctx.simulationPaused,
                    "death-origin successful Load replaces death card but remains paused");
                click(CampaignMenu::ResumeId);PumpCampaign(ctx);
                check(!ctx.simulationPaused && !ctx.campaignMenu->Visible(),"death-origin explicit native Resume restores gameplay");
            }
        }
    }
    if(proceed){
        // Reopen the existing front-end origin diagnostically; all chooser,
        // confirmation and owner commands below are the production WM_COMMAND path.
        ctx.entryMenuVisible=true;ctx.entryMoreVisible=false;ctx.pauseMenuVisible=true;ApplyOverlayState(ctx);
        click(kResumeButtonId);proceed=until(catalogueReady,false);
        click(CampaignMenu::FirstChoiceId+1);click(CampaignMenu::ConfirmId);PumpCampaign(ctx);
        proceed=check(proceed && until(complete,true),"explicit New Game slot selection prepares and presents a fresh RT game");
        if(proceed)check(ctx.campaign->State().activeSlot==1 && ctx.simulation.Snapshot().playerVitals.vitality==3 &&
            ctx.simulation.Snapshot().playerYawRadians==0 && !ctx.simulationPaused && !ctx.campaignMenu->Visible(),
            "New Game uses selected autosave slot and doorway-facing neutral alive opening");
    }
    check(GetForegroundWindow()==foreground && !IsWindowVisible(ctx.windowHandle),"foreground unchanged and owner never shown");
    receipt<<"],\"count\":"<<checks<<",\"failures\":"<<failures
        <<",\"physicalVisibleAcceptance\":false,\"precommitCancellationPreparationHold\":true,\"newGameEntryReopenedByDiagnostic\":true,\"ownerSettingsReadOrWritten\":false}\n";
    if(!WriteReportFile(gCampaignValidationDirectory/"windows-validation.json",receipt.str()))return 1;
    return failures?1:0;
}
#endif

bool ConfigureHiddenCampaignValidation()
{
    int count=0;auto args=CommandLineToArgvW(GetCommandLineW(),&count);if(!args)return false;
    bool requested=false;std::filesystem::path directory;
    for(int i=1;i<count;++i) {
        const std::wstring_view arg=args[i];
        if(arg==L"--campaign-validate-hidden")requested=true;
        if(arg==L"--campaign-test-root" && i+1<count)directory=args[++i];
    }
    LocalFree(args);
    if(!requested && directory.empty())return true;
#if defined(_DEBUG) && defined(HORDE_RT_SOURCE_DIR)
    std::error_code error;
    const auto absolute=std::filesystem::absolute(directory,error).lexically_normal();
    const auto reports=std::filesystem::canonical(std::filesystem::path(HORDE_RT_SOURCE_DIR)/"reports",error);
    if(!requested || directory.empty() || error || !directory.is_absolute() ||
        !absolute.filename().string().starts_with("horde-campaign-fixtures-") ||
        std::filesystem::canonical(absolute.parent_path(),error)!=reports || error ||
        std::filesystem::canonical(absolute,error)!=absolute || error || !std::filesystem::is_directory(absolute,error) || error ||
        !std::filesystem::is_regular_file(absolute/"manifest.json",error) || error){
        std::cerr<<"Hidden campaign validation requires a caller-created exported fixture directory directly in this checkout's reports.\n";return false;
    }
    for(const auto& item:std::filesystem::directory_iterator(absolute))if(item.is_symlink())return false;
    gCampaignHiddenValidation=true;gCampaignValidationDirectory=absolute;return true;
#else
    std::cerr<<"Hidden campaign validation is Debug-only.\n";return false;
#endif
}
