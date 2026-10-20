#if defined(_DEBUG)
class HiddenCampaignFaults final:public campaign::storage::FaultInjector {
public:
    std::atomic<bool> failNextRead=false;
    std::atomic<bool> lookoutReadOnly=false;
    std::atomic<unsigned> rejectedWrites=0;
    bool Fail(campaign::storage::Operation operation,std::size_t) override {
        if(lookoutReadOnly && operation!=campaign::storage::Operation::Read){++rejectedWrites;return true;}
        return operation==campaign::storage::Operation::Read && failNextRead.exchange(false);
    }
} gHiddenCampaignFaults;

// This mode has no relationship to the full campaign validation below. In
// particular it never seeds gameplay, opens Save/New Game or enters combat.
struct HiddenLookoutEvidence {
    HWND initialForeground=nullptr,previousForeground=nullptr;
    bool foregroundChanged=false,ownerObserved=false,ownerShown=false;
    unsigned foregroundSamples=0,storedForegroundSamples=0;
    std::ostringstream foreground,receipt;
    std::chrono::steady_clock::time_point started{};
    campaign::storage::StoredCheckpoint source;
    std::string manifest;
    bool sourceLockPresent=false;
} gHiddenLookout;

bool ObserveHiddenLookoutForeground(HWND owner,const char* stage,bool retain=true)
{
    const auto current=GetForegroundWindow();
    const bool ours=owner && (current==owner || IsChild(owner,current) || GetWindow(current,GW_OWNER)==owner);
    const bool shown=owner && IsWindowVisible(owner);
    const bool changed=current!=gHiddenLookout.previousForeground;
    gHiddenLookout.foregroundChanged|=changed;
    gHiddenLookout.ownerObserved|=ours;gHiddenLookout.ownerShown|=shown;
    ++gHiddenLookout.foregroundSamples;
    // Sample every owner pump, retain only bounded stage/frame/command points.
    // A changed HWND or ownership failure is always retained, including after
    // the ordinary entry cap; the runner stops immediately on that observation.
    if((retain && gHiddenLookout.storedForegroundSamples<256) || changed || ours || shown) {
        if(gHiddenLookout.storedForegroundSamples++)gHiddenLookout.foreground<<',';
        gHiddenLookout.foreground<<"{\"stage\":\""<<stage<<"\",\"sample\":"<<gHiddenLookout.foregroundSamples<<",\"initial\":"
            <<reinterpret_cast<std::uintptr_t>(gHiddenLookout.initialForeground)<<",\"previous\":"
            <<reinterpret_cast<std::uintptr_t>(gHiddenLookout.previousForeground)<<",\"current\":"
            <<reinterpret_cast<std::uintptr_t>(current)<<",\"owner\":"<<reinterpret_cast<std::uintptr_t>(owner)
            <<",\"ours\":"<<(ours?"true":"false")<<",\"shown\":"<<(shown?"true":"false")<<'}';
        std::cerr<<"HORDE_LOOKOUT_FOREGROUND stage="<<stage<<" sample="<<gHiddenLookout.foregroundSamples
            <<" initial="<<gHiddenLookout.initialForeground<<" previous="<<gHiddenLookout.previousForeground
            <<" current="<<current<<" owner="<<owner<<" ours="<<ours<<" shown="<<shown<<'\n';
    }
    if(changed && !ours)std::cerr<<"HORDE_LOOKOUT_FOREGROUND changed_external_hwnd=1 cause=unattributed\n";
    gHiddenLookout.previousForeground=current;
    return !gHiddenLookout.foregroundChanged && !gHiddenLookout.ownerObserved && !gHiddenLookout.ownerShown;
}

bool ReadHiddenLookoutManifest(const std::filesystem::path& path,std::string& bytes)
{
    std::ifstream file(path,std::ios::binary|std::ios::ate);if(!file)return false;
    const auto length=file.tellg();if(length<0 || length>4096)return false;
    std::string next(static_cast<std::size_t>(length),'\0');file.seekg(0);
    if(!file.read(next.data(),static_cast<std::streamsize>(next.size())) || file.peek()!=std::char_traits<char>::eof())return false;
    bytes=std::move(next);return true;
}

bool HiddenLookoutFilesAllowed(const std::filesystem::path& directory,bool afterRun)
{
    std::error_code error;unsigned sourceFiles=0;
    for(const auto& item:std::filesystem::directory_iterator(directory,error)) {
        if(error || item.is_symlink(error) || error || !item.is_regular_file(error) || error)return false;
        const auto name=item.path().filename().string();
        if(name=="manifest.json" || name=="slot-1-auto.hrd"){++sourceFiles;continue;}
        if(name==".slot-1-auto.hrd.lock") {if(item.file_size(error)!=0 || error)return false;continue;}
        if(afterRun && (name==kTextReportFilename || name==kJsonReportFilename ||
            name=="windows-lookout-validation.json" || name=="windows-loaded-lookout.png"))continue;
        if(afterRun && gLookoutInspectKit && (name=="windows-kit-neutral.png" ||
            name=="windows-kit-speaking.png" || name=="windows-kit-recognition.png"))continue;
        if(afterRun && gLookoutInspectKitMotion) {
            bool allowed=false;
            for(const char* step:{"start-0","start-1","start-4","start-8","walk-20","walk-35","walk-50","walk-65","walk-80","walk-100",
                "stop-0","stop-1","stop-4","stop-8","stop-11","turn-0","turn-10","turn-20","turn-30","turn-40",
                "turn-60","turn-80","turn-100"})
                allowed|=name==std::string("windows-kit-motion-")+step+".png";
            if(allowed)continue;
        }
        return false;
    }
    return !error && sourceFiles==2;
}

bool PrepareHiddenLookoutFixture(const std::filesystem::path& directory)
{
    if(!HiddenLookoutFilesAllowed(directory,false))return false;
    gHiddenLookout.sourceLockPresent=std::filesystem::exists(directory/".slot-1-auto.hrd.lock");
    // The create-only exporter deliberately emits this single, named fixture.
    // Matching its small canonical manifest avoids a second JSON parser or any
    // path selected by untrusted manifest text.
    constexpr std::string_view manifest="{\"evidenceClass\":\"seeded noncombat lookout recovery; not earned progression\","
        "\"record\":\"slot-1-auto.hrd\",\"checkpoint\":\"ForestEndpoint\",\"contentRevision\":2,\"hearts\":2}\n";
    if(!ReadHiddenLookoutManifest(directory/"manifest.json",gHiddenLookout.manifest) ||
        gHiddenLookout.manifest!=manifest || campaign::storage::Read(directory,"slot-1-auto.hrd",gHiddenLookout.source).status!=campaign::storage::Status::Read)return false;
    const auto& c=gHiddenLookout.source.checkpoint;
    const auto p=horde::gameplay::simulation::kWorldRoutePoints.back();
    if(!campaign::Validate(c) || c.id!=campaign::CheckpointId::ForestEndpoint || c.contentRevision!=2 ||
        c.zone!=horde::gameplay::simulation::WorldZoneId::Lookout || c.position.x!=p.x || c.position.z!=p.z ||
        c.yaw!=2.51f || c.pitch!=-.15f || c.vitality!=2 || !campaign::CompletedForestProgress(c.dialogue))return false;
    gHiddenCampaignFaults.lookoutReadOnly=true;gHiddenCampaignFaults.rejectedWrites=0;
    gHiddenLookout.started=std::chrono::steady_clock::now();
    gHiddenLookout.initialForeground=gHiddenLookout.previousForeground=GetForegroundWindow();
    return ObserveHiddenLookoutForeground(nullptr,"fixture-admitted");
}

int RunHiddenLookoutValidation(VulkanSurfaceContext& ctx,horde::vulkan::DeviceCapabilities& capabilities)
{
    using namespace horde::gameplay;
    using horde::telemetry::RtPresentationOutcome;
    unsigned checks=0,failures=0,renderCalls=0;
    auto& receipt=gHiddenLookout.receipt;
    receipt<<"{\"schema\":1,\"evidenceClass\":\"seeded noncombat hidden HWND commands and hardware RT swapchain; not earned progression\",\"checks\":[";
    const auto check=[&](bool ok,const char* name){
        if(checks++)receipt<<',';if(!ok)++failures;
        receipt<<"{\"name\":\""<<name<<"\",\"pass\":"<<(ok?"true":"false")<<'}';
        std::cerr<<"HORDE_LOOKOUT_CHECK "<<name<<" pass="<<ok<<'\n';return ok;
    };
    const auto safe=[&](const char* stage,bool retain=true){
        return ObserveHiddenLookoutForeground(ctx.windowHandle,stage,retain) &&
            std::chrono::steady_clock::now()-gHiddenLookout.started<std::chrono::seconds(90) &&
            !ctx.campaignRetirementUnproved && !gHiddenCampaignFaults.rejectedWrites;
    };
    const auto click=[&](int id){
        const auto control=GetDlgItem(ctx.windowHandle,id);
        if(!control || !IsWindowEnabled(control) || !(GetWindowLongPtrA(control,GWL_STYLE)&WS_VISIBLE) || !safe("before-native-command"))return false;
        SendMessageA(ctx.windowHandle,WM_COMMAND,MAKEWPARAM(id,BN_CLICKED),reinterpret_cast<LPARAM>(control));
        return safe("after-native-command");
    };
    const auto render=[&]{
        if(++renderCalls>180 || !safe("before-rt-frame"))return false;
        bool presented=false;
        if(!RenderFrame(ctx,VkClearColorValue{{0,0,0,1}},presented))return false;
        FinishGraphicsFrame(ctx,presented);ObserveCampaignPresentation(ctx,presented);
        return presented && ctx.lastFramePresentation==RtPresentationOutcome::Presented && safe("after-rt-frame");
    };
    const auto until=[&](auto done,bool frames){
        bool first=true;auto previousPhase=ctx.campaignPhase;
        do {
            if(!safe("owner-pump",first))return false;
            PumpCampaign(ctx);
            if(ctx.campaignPhase==CampaignMenu::Phase::Failed || !safe("after-owner-pump",first || ctx.campaignPhase!=previousPhase))return false;
            first=false;previousPhase=ctx.campaignPhase;
            if(done())return true;
            if(frames){if(!render())return false;}
            else std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }while(std::chrono::steady_clock::now()-gHiddenLookout.started<std::chrono::seconds(90));
        return false;
    };
    const auto noPanel=[&]{return !ctx.lookoutObjective || !ctx.lookoutObjective->Visible();};
    const auto neutral=[&]{return !ctx.forwardHeld && !ctx.backwardHeld && !ctx.leftHeld && !ctx.rightHeld &&
        !ctx.runHeld && !ctx.crouchHeld && !ctx.controllerRunHeld && !ctx.controllerCrouchHeld &&
        !ctx.mouseLookActive && ctx.controllerForward==0 && ctx.controllerStrafe==0 &&
        ctx.controllerLookHorizontal==0 && ctx.controllerLookVertical==0 &&
        ctx.simulationInput.moveForward==0 && ctx.simulationInput.moveStrafe==0 &&
        !ctx.simulationInput.hasAuthoritativePlayerPose && ctx.simulationInput.combatEdgeHistory.count==0 &&
        ctx.simulationInput.dialogueCompletionLine==0 && ctx.simulationInput.dialogueCompletionGeneration==0;};
    const auto boundarySame=[&]{
        const auto& boundary=ctx.simulation.LatestCampaignBoundary();std::vector<std::byte> encoded;
        campaign::codec::Error error{};
        return boundary && campaign::codec::Encode(*boundary,encoded,error) && encoded==gHiddenLookout.source.encoded;
    };
    const auto storySame=[&]{
        const auto& s=ctx.simulation.Snapshot();const auto& kit=s.chapterCompanion;
        const auto p=simulation::kWorldRoutePoints.back();const auto& c=gHiddenLookout.source.checkpoint;
        const auto& d=s.chapterDialogue;
        return s.playerAlive && s.playerVitals.vitality==c.vitality && s.playerGrounded &&
            std::abs(s.playerX-c.position.x)<.001f && std::abs(s.playerZ-c.position.z)<.001f &&
            std::abs(s.playerSupportWorldY-c.position.y)<.001f && s.playerYawRadians==c.yaw && s.playerPitchRadians==c.pitch &&
            s.rescue.exteriorSide && s.rescue.firstAscentCompleted && !s.rescue.equipmentStowed &&
            d.line==dialogue::Line::None && d.proofAccepted && d.forestNightConsumed && d.forestWaystoneConsumed &&
            d.forestClueConsumed && d.forestVillageConsumed && !d.waitingForRaise &&
            kit.grounded && !kit.walking && kit.waypoint==simulation::kWorldRoutePoints.size() &&
            std::hypot(kit.x-(p.x+1),kit.z-p.z)<.001f && boundarySame();
    };
    std::string loadedEvidence,entryEvidence;std::ostringstream kitEvidence,motionEvidence;
    const auto completed=[&](bool entry,std::string& json){
        if(!safe("before-gpu-drain") || !DrainCampaignResources(ctx) || !safe("after-gpu-drain"))return false;
        const auto publication=ctx.rtFrameEvidence.PublishedStateByValue();const auto& e=publication.completedEvidence;
        const auto& id=e.identity.submitted;
        horde::telemetry::RtEvidenceValidationError error{};
        return ctx.rtFrameEvidenceInitialised && publication.presented && publication.hasCompletedEvidence &&
            id.frame.sceneEpoch==publication.sceneEpoch &&
            id.frame.simulationTick==(entry?ctx.entryMenu.Snapshot().tick:ctx.campaignFrameTick) &&
            e.presentation.outcome==RtPresentationOutcome::Presented &&
            id.submissionSerial==e.presentation.lastSuccessfulPresentSubmissionSerial &&
            e.scene.dispatch.sceneReady && e.scene.dispatch.rtDispatchRecorded && e.scene.dispatch.swapchainCopyRecorded &&
            horde::telemetry::SerializeRtPerformanceEvidenceJson(e,json,error);
    };
    bool proceed=check(ctx.campaign && ctx.campaignMenu && ctx.campaignIsolated && gCampaignHiddenValidation &&
        (GetWindowLongPtrA(ctx.windowHandle,GWL_EXSTYLE)&(WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW))==(WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW) &&
        safe("runner-entered"),"isolated exported fixture and explicitly nonactivating hidden owner");
    if(proceed){
        ApplyOverlayState(ctx);
        proceed=check(click(kEntryMoreButtonId) && click(kCampaignLoadButtonId) && until([&]{
            for(const auto& e:ctx.campaign->Catalogue())if(e.status==campaign::CatalogueStatus::Checking)return false;
            return true;
        },false),"ordinary entry More and Load controls finish read-only catalogue");
    }
    if(proceed){
        bool catalogue=true;
        for(std::size_t i=0;i<ctx.campaign->Catalogue().size();++i)
            catalogue&=ctx.campaign->Catalogue()[i].status==(i==1?campaign::CatalogueStatus::Valid:campaign::CatalogueStatus::Empty);
        proceed=check(catalogue && click(CampaignMenu::FirstChoiceId+1) && until([&]{
            return ctx.campaignBarrier.State().phase==campaign::CampaignLoadBarrier::Phase::Complete;
        },true),"only slot one autosave loads through owner input rebase and matching successful RT presentation");
    }
    std::uint64_t loadedTick=0,operationRevision=0,boundaryRevision=0;
    float kitY=0;
    if(proceed){
        const auto& s=ctx.simulation.Snapshot();const auto terrain=ctx.rtScene.ChapterTerrainSurface();
        const auto support=traversal::ChapterExteriorSupport(terrain.get(),s.playerX,s.playerZ);
        const auto kitSupport=traversal::ChapterExteriorSupport(terrain.get(),s.chapterCompanion.x,s.chapterCompanion.z);
        loadedTick=s.tickIndex;kitY=s.chapterCompanion.y;
        operationRevision=ctx.campaign->State().operationRevision;boundaryRevision=ctx.simulation.CampaignBoundaryRevision();
        proceed=check(ctx.campaignPause && ctx.simulationPaused && ctx.pauseMenuVisible && ctx.campaignMenu->Visible() &&
            ctx.campaign->State().activeSlot==0 && IsWindowEnabled(GetDlgItem(ctx.windowHandle,CampaignMenu::ResumeId)) &&
            s.worldRoute.current==simulation::WorldZoneId::Lookout &&
            s.worldRoute.readiness[static_cast<std::size_t>(simulation::WorldZoneId::Lookout)]==simulation::ZoneReadiness::Ready &&
            support.grounded && support.id==s.playerSupportId && static_cast<unsigned>(support.id)>=1000 &&
            support.worldY==s.playerSupportWorldY && kitSupport.grounded && std::abs(kitSupport.worldY-kitY)<.001f &&
            storySame() && neutral() && noPanel() && !ctx.endingOverlayVisible && completed(false,loadedEvidence),
            "loaded player and Kit use actual grounded forest support with completed quiet story and no automatic panel");
    }
    if(proceed){
        std::string error;horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
        proceed=check(safe("before-lookout-image") && ctx.rtScene.CaptureStorageImage(image,error) &&
            WriteRgbaPng(gCampaignValidationDirectory/"windows-loaded-lookout.png",image,error) && safe("after-lookout-image"),
            "original paused lookout RT storage image read back separately from logical support evidence");
    }
    if(proceed && gLookoutInspectKit) {
        // A labelled render-only pose fixture, not proof of a live recognition
        // event. Shared-simulation tests separately own the fresh-raise gate.
        // Camera/quality/idle phase/lighting match across these three images.
        const auto& kit=ctx.simulation.Snapshot().chapterCompanion;
        const auto originalActing=kit.acting;
        ctx.lookoutKitInspectionPose=horde::vulkan::raytracing::EnvironmentSurveyPose{
            kit.x-3.f,kit.z,kit.y,1.57079632679f,-.15f,0};
        std::array<dialogue::CompanionActingPose,3> poses{};
        dialogue::CompanionActing speaking,recognition;dialogue::Snapshot line;
        line.line=dialogue::Line::ReunionQuestion;line.duration=dialogue::Spec(line.line).duration;
        for(int i=0;i<30;++i){line.elapsed=i/60.f;speaking.Step(1.f/60,line,true,true,true,false,false);}
        poses[1]=speaking.State();line={};line.proofAccepted=true;
        for(int i=0;i<36;++i)recognition.Step(1.f/60,line,true,true,true,false,i==0);
        poses[2]=recognition.State();
        constexpr std::array<const char*,3> names{"neutral","speaking","recognition"};
        for(unsigned i=0;i<poses.size() && proceed;++i) {
            ctx.lookoutKitInspectionActing=poses[i];std::string evidence,error;
            horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
            proceed=check(render() && completed(false,evidence) && ctx.rtScene.CaptureStorageImage(image,error) &&
                WriteRgbaPng(gCampaignValidationDirectory/(std::string("windows-kit-")+names[i]+".png"),image,error) &&
                ctx.simulation.Snapshot().chapterCompanion.acting==originalActing && storySame() && neutral(),
                "seeded Kit pose-only inspection has completed real RT and leaves logical story and acting unchanged");
            if(i)kitEvidence<<',';
            kitEvidence<<"{\"pose\":\""<<names[i]<<"\",\"angles\":["<<poses[i].headPitch<<','<<poses[i].headYaw<<','<<poses[i].chestPitch
                <<"],\"rtEvidence\":"<<(evidence.empty()?"null":evidence)<<'}';
        }
        ctx.lookoutKitInspectionPose.reset();ctx.lookoutKitInspectionActing={};
        if(proceed)proceed=check(render() && storySame() && ctx.simulation.Snapshot().chapterCompanion.acting==originalActing,
            "ordinary saved camera and neutral Kit pose return after diagnostic inspection");
    }
    if(proceed && gLookoutInspectKitMotion) {
        // A sequential production-route visual fixture. It samples real fixed
        // steps/support and renders their immutable snapshots, without changing
        // the paused loaded campaign, earning progression or touching records.
        const auto terrain=ctx.rtScene.ChapterTerrainSurface();
        dialogue::CompanionRouteFixture route;
        auto player=route.State();
        for(unsigned tick=0;tick<60;++tick)
            route.Step(1.f/60,player.x+3,player.z+1,true,false,false,terrain.get(),true);
        const auto capture=[&](const char* stage) {
            auto pose=route.State();pose.visualAdmitted=true;
            pose.generation=ctx.simulation.Snapshot().chapterCompanion.generation+1;
            ctx.lookoutKitInspectionMotion=pose;
            ctx.lookoutKitInspectionPose=horde::vulkan::raytracing::EnvironmentSurveyPose{
                pose.x+3.f,pose.z,pose.y,-1.57079632679f,-.15f,0};
            ctx.lookoutKitInspectionActing={};std::string evidence,error;
            horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
            const bool ok=check(pose.grounded && render() && completed(false,evidence) &&
                ctx.rtScene.CaptureStorageImage(image,error) &&
                WriteRgbaPng(gCampaignValidationDirectory/(std::string("windows-kit-motion-")+stage+".png"),image,error) &&
                storySame() && neutral(),"sequential production Kit motion snapshot has completed real RT and preserves loaded campaign");
            if(!motionEvidence.str().empty())motionEvidence<<',';
            motionEvidence<<"{\"stage\":\""<<stage<<"\",\"root\":["<<pose.x<<','<<pose.y<<','<<pose.z
                <<"],\"yaw\":"<<pose.yaw<<",\"walking\":"<<(pose.walking?"true":"false")
                <<",\"idleSeconds\":"<<pose.animationSeconds<<",\"walkPhase\":"<<pose.locomotion.walkPhase
                <<",\"walkWeight\":"<<pose.locomotion.walkWeight<<",\"leftTurnStep\":"<<pose.locomotion.leftTurnStep
                <<",\"rightTurnStep\":"<<pose.locomotion.rightTurnStep<<",\"rtEvidence\":"<<(evidence.empty()?"null":evidence)<<'}';
            return ok;
        };
        proceed=capture("start-0");
        for(unsigned tick=1;tick<=100 && proceed;++tick) {
            const auto before=route.State();
            route.Step(1.f/60,before.x+3,before.z+1,true,true,false,terrain.get(),true);
            const char* name=tick==1?"start-1":tick==4?"start-4":tick==8?"start-8":
                tick==20?"walk-20":tick==35?"walk-35":tick==50?"walk-50":
                tick==65?"walk-65":tick==80?"walk-80":tick==100?"walk-100":nullptr;
            if(name)proceed=capture(name);
        }
        if(proceed)proceed=capture("stop-0");
        for(unsigned tick=1;tick<=11 && proceed;++tick) {
            const auto before=route.State();
            route.Step(1.f/60,before.x+3,before.z+1,true,false,false,terrain.get(),true);
            const char* name=tick==1?"stop-1":tick==4?"stop-4":tick==8?"stop-8":tick==11?"stop-11":nullptr;
            if(name)proceed=capture(name);
        }
        if(proceed)proceed=capture("turn-0");
        for(unsigned tick=1;tick<=100 && proceed;++tick) {
            const auto before=route.State();
            route.Step(1.f/60,before.x-3,before.z,true,false,false,terrain.get(),true);
            const char* name=tick==10?"turn-10":tick==20?"turn-20":tick==30?"turn-30":tick==40?"turn-40":
                tick==60?"turn-60":tick==80?"turn-80":tick==100?"turn-100":nullptr;
            if(name)proceed=capture(name);
        }
        ctx.lookoutKitInspectionMotion.reset();ctx.lookoutKitInspectionPose.reset();ctx.lookoutKitInspectionActing={};
        if(proceed)proceed=check(render() && storySame(),"ordinary loaded camera and actor return after sequential motion inspection");
    }
    if(proceed){
        // Native notification queues owner work; the normal owner pump owns its
        // effects. Checking menu/pause state before that pump is not a Resume test.
        proceed=check(click(CampaignMenu::ResumeId) && ctx.campaignCommand &&
            ctx.campaignCommand->action==CampaignMenu::Action::Resume,
            "explicit native Resume queues exactly the production owner command");
        if(proceed){
            PumpCampaign(ctx);
            proceed=check(!ctx.campaignCommand && ctx.campaignPhase!=CampaignMenu::Phase::Failed && safe("after-resume-owner-pump"),
                "one normal owner pump consumes Resume without failure");
        }
        if(proceed)proceed=check(!ctx.campaignMenu->Visible(),"Resume owner command closes load chooser");
        if(proceed)proceed=check(!ctx.campaignPause && !ctx.simulationPaused,"Resume owner command releases both pause owners");
        if(proceed)proceed=check(noPanel() && neutral(),"Resume retains neutral input without automatically opening objective");
        if(proceed)proceed=check(render(),"first neutral live RT frame presents after Resume");
        if(proceed)proceed=check(render(),"second neutral live RT frame presents after Resume");
        if(proceed)proceed=check(ctx.simulation.Snapshot().tickIndex>loadedTick && storySame() && noPanel(),
            "neutral live simulation advances without replaying completed dialogue or opening objective");
        if(proceed){const auto current=ctx.simulation.CaptureCampaignCheckpoint(campaign::CheckpointId::ForestEndpoint);
            proceed=check(current && campaign::CompletedForestProgress(current->dialogue),
                "live endpoint capture independently confirms no active speech or queued continuation after Resume");}
    }
    const auto pause=[&]{
        if(!safe("before-pause-menu-command"))return false;
        // HMENU commands have no child HWND. Actual button commands above still
        // require their current native child handle and visible/enabled style.
        SendMessageA(ctx.windowHandle,WM_COMMAND,MAKEWPARAM(kMenuPauseId,0),0);
        return safe("after-pause-menu-command");
    };
    const auto open=[&]{return pause() && ctx.simulationPaused && noPanel() &&
        CurrentLookoutObjectiveView(ctx).available && click(kLookoutObjectiveButtonId) &&
        ctx.lookoutObjective && ctx.lookoutObjective->Visible() && ctx.simulationPaused;};
    if(proceed){
        proceed=check(open(),"explicit pause-menu Lookout objective opens the current native panel");
        if(proceed){
            // Only our own HWND receives these messages. A held key begun under
            // the panel must not leak movement or route reset after Continue.
            for(const int key:{static_cast<int>('W'),static_cast<int>('R'),static_cast<int>(VK_SHIFT)})SendMessageA(ctx.windowHandle,WM_KEYDOWN,key,0);
            proceed=check(click(LookoutPanel::ContinueId) && noPanel() && !ctx.simulationPaused && neutral(),
                "native Continue releases panel input and resumes existing game");
            if(proceed){
                for(const int key:{static_cast<int>('W'),static_cast<int>('R'),static_cast<int>(VK_SHIFT)}){
                    SendMessageA(ctx.windowHandle,WM_KEYDOWN,key,1ll<<30);
                    SendMessageA(ctx.windowHandle,WM_KEYUP,key,1ll<<31);
                }
                proceed=check(neutral() && render() && storySame() && ctx.simulation.CampaignBoundaryRevision()==boundaryRevision,
                    "held-key autorepeat cannot move or reset the recovered game after Continue");
            }
        }
    }
    if(proceed)proceed=check(open() && click(LookoutPanel::BackId) && noPanel() && !ctx.simulationPaused && neutral() && storySame(),
        "reopened native Back safely continues without replacing checkpoint");
    if(proceed){
        proceed=check(open(),"objective explicitly reopens for keyboard cancel");
        if(proceed){SendMessageA(ctx.windowHandle,WM_KEYDOWN,VK_ESCAPE,0);SendMessageA(ctx.windowHandle,WM_KEYUP,VK_ESCAPE,1ll<<31);
            proceed=check(noPanel() && !ctx.simulationPaused && neutral() && storySame() && safe("after-own-window-escape"),
                "own-window Escape uses production cancellation without focus or physical input");}
    }
    if(proceed){
        proceed=check(open(),"objective explicitly reopens for controller cancel");
        if(proceed){horde::platform::windows::ControllerMenuEdges edges{};edges.cancel=true;HandleControllerMenuEdges(ctx,edges);
            proceed=check(noPanel() && !ctx.simulationPaused && neutral() && storySame() && safe("after-controller-cancel"),
                "production controller cancel edge safely continues without polling a device");}
    }
    if(proceed){
        proceed=check(open() && click(LookoutPanel::MenuId) && noPanel() && ctx.entryMenuVisible &&
            ctx.pauseMenuVisible && ctx.simulationPaused && ctx.sceneProfileDirty && storySame() && neutral(),
            "explicit Return to menu requests branded entry profile while retaining paused campaign");
        if(proceed){
            std::vector<double> unusedTimings;
            proceed=check(safe("before-entry-replacement") && ApplyPendingSceneReplacement(ctx,capabilities,unusedTimings) &&
                ctx.rtScene.Profile()==horde::vulkan::raytracing::RtSceneProfile::EntryMenu &&
                !ctx.sceneProfileDirty && safe("after-entry-replacement") && render() && completed(true,entryEvidence) &&
                ctx.entryMenuVisible && ctx.simulationPaused && storySame() && neutral() &&
                std::abs(ctx.simulation.Snapshot().chapterCompanion.y-kitY)<.001f &&
                ctx.simulation.CampaignBoundaryRevision()==boundaryRevision && ctx.campaign->State().activeSlot==0 &&
                ctx.campaign->State().operationRevision==operationRevision && !gHiddenCampaignFaults.rejectedWrites,
                "actual EntryMenu scene replacement and completed RT present retain player Kit story and checkpoint without save operation");
        }
    }
    // This final read is independent of the logical assertions, and also runs
    // after a failed stage. No result is repaired, retried or auto-exported.
    campaign::storage::StoredCheckpoint after;std::string manifestAfter;
    check(campaign::storage::Read(gCampaignValidationDirectory,"slot-1-auto.hrd",after).status==campaign::storage::Status::Read &&
        after.encoded==gHiddenLookout.source.encoded && ReadHiddenLookoutManifest(gCampaignValidationDirectory/"manifest.json",manifestAfter) &&
        manifestAfter==gHiddenLookout.manifest && HiddenLookoutFilesAllowed(gCampaignValidationDirectory,true) &&
        std::filesystem::exists(gCampaignValidationDirectory/".slot-1-auto.hrd.lock")==gHiddenLookout.sourceLockPresent &&
        !gHiddenCampaignFaults.rejectedWrites,"source autosave and manifest bytes unchanged with zero attempted campaign writes");
    check(safe("runner-finished"),"foreground continuity and hidden owner preserved within ninety-second stage budget");
    receipt<<"],\"count\":"<<checks<<",\"failures\":"<<failures<<",\"stagesComplete\":"<<(proceed?"true":"false")
        <<",\"renderCalls\":"<<renderCalls<<",\"loadedTick\":"<<loadedTick<<",\"sourceBytes\":"<<gHiddenLookout.source.encoded.size()
        <<",\"loadedRtEvidence\":"<<(loadedEvidence.empty()?"null":loadedEvidence)
        <<",\"entryRtEvidence\":"<<(entryEvidence.empty()?"null":entryEvidence)
        <<",\"kitPoseInspection\":{\"requested\":"<<(gLookoutInspectKit?"true":"false")
        <<",\"evidenceClass\":\"seeded render-only pose; not live fresh-raise acceptance\",\"cameraOffset\":[-3,0,0],\"poses\":["<<kitEvidence.str()<<"]}"
        <<",\"kitMotionInspection\":{\"requested\":"<<(gLookoutInspectKitMotion?"true":"false")
        <<",\"evidenceClass\":\"sequential fixed-step production-route render inputs; not live earned gameplay\",\"frames\":["<<motionEvidence.str()<<"]}"
        <<",\"physicalVisibleAcceptance\":false,\"performanceEvidence\":false,\"ownerSettingsReadOrWritten\":false";
    return proceed && !failures?0:1;
}

int FinishHiddenLookoutValidation(HWND owner,int result)
{
    if(!ObserveHiddenLookoutForeground(owner,"owner-destroyed"))result=1;
    const bool inBudget=std::chrono::steady_clock::now()-gHiddenLookout.started<std::chrono::seconds(90);
    if(!inBudget || gHiddenLookout.receipt.str().empty())result=1;
    auto& receipt=gHiddenLookout.receipt;
    if(receipt.str().empty())receipt<<"{\"schema\":1,\"checks\":[],\"stagesComplete\":false";
    receipt<<",\"foregroundSampleCount\":"<<gHiddenLookout.foregroundSamples
        <<",\"foregroundStoredSampleCount\":"<<gHiddenLookout.storedForegroundSamples
        <<",\"foregroundSamples\":["<<gHiddenLookout.foreground.str()<<"],\"foregroundContinuous\":"
        <<(!gHiddenLookout.foregroundChanged?"true":"false")<<",\"ownerObservedForeground\":"
        <<(gHiddenLookout.ownerObserved?"true":"false")<<",\"ownerShown\":"<<(gHiddenLookout.ownerShown?"true":"false")
        <<",\"withinNinetySeconds\":"<<(inBudget?"true":"false")<<",\"complete\":"<<(result==0?"true":"false")<<"}\n";
    return WriteReportFile(gCampaignValidationDirectory/"windows-lookout-validation.json",receipt.str())?result:1;
}
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
#include "windows_route_validation.inl"
#endif

bool ConfigureHiddenCampaignValidation()
{
    int count=0;auto args=CommandLineToArgvW(GetCommandLineW(),&count);if(!args)return false;
    bool requested=false,lookoutRequested=false,routeRequested=false,returnRequested=false,inspectKit=false,inspectMotion=false;std::filesystem::path directory;
    for(int i=1;i<count;++i) {
        const std::wstring_view arg=args[i];
        if(arg==L"--campaign-validate-hidden")requested=true;
        if(arg==L"--lookout-validate-hidden")lookoutRequested=true;
        if(arg==L"--route-validate-hidden")routeRequested=true;
        if(arg==L"--route-return-recovery-hidden")returnRequested=true;
        if(arg==L"--lookout-inspect-kit")inspectKit=true;
        if(arg==L"--lookout-inspect-kit-motion"){inspectKit=true;inspectMotion=true;}
        if(arg==L"--campaign-test-root" && i+1<count)directory=args[++i];
    }
    LocalFree(args);
    if(!requested && !lookoutRequested && !routeRequested && !returnRequested && !inspectKit && directory.empty())return true;
#if defined(_DEBUG) && defined(HORDE_RT_SOURCE_DIR)
    std::error_code error;
    const auto absolute=std::filesystem::absolute(directory,error).lexically_normal();
    const auto reports=std::filesystem::canonical(std::filesystem::path(HORDE_RT_SOURCE_DIR)/"reports",error);
    if(unsigned(requested)+unsigned(lookoutRequested)+unsigned(routeRequested)+unsigned(returnRequested)!=1 || (inspectKit && !lookoutRequested) || directory.empty() || error || !directory.is_absolute() ||
        !absolute.filename().string().starts_with("horde-campaign-fixtures-") ||
        std::filesystem::canonical(absolute.parent_path(),error)!=reports || error ||
        std::filesystem::canonical(absolute,error)!=absolute || error || !std::filesystem::is_directory(absolute,error) || error ||
        !std::filesystem::is_regular_file(absolute/"manifest.json",error) || error){
        std::cerr<<"Hidden campaign validation requires a caller-created exported fixture directory directly in this checkout's reports.\n";return false;
    }
    for(const auto& item:std::filesystem::directory_iterator(absolute))if(item.is_symlink())return false;
    if(lookoutRequested && !PrepareHiddenLookoutFixture(absolute)){
        std::cerr<<"Hidden lookout validation requires the unchanged caller-exported single ForestEndpoint autosave and canonical manifest only.\n";return false;
    }
    gRouteReturnRecovery=returnRequested;
    if((routeRequested || returnRequested) && !PrepareHiddenRouteFixture(absolute)){
        std::cerr<<"Hidden route validation requires the unchanged exported single LowerClaimed manual record and canonical manifest only.\n";return false;
    }
    gLookoutHiddenValidation=lookoutRequested;
    gRouteHiddenValidation=routeRequested || returnRequested;
    gLookoutInspectKit=inspectKit;gLookoutInspectKitMotion=inspectMotion;
    gCampaignHiddenValidation=true;gCampaignValidationDirectory=absolute;return true;
#else
    std::cerr<<"Hidden campaign validation is Debug-only.\n";return false;
#endif
}
