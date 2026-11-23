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
        if(afterRun && gLookoutInspectKitTurns) {
            if(name=="windows-kit-turn-control.png" || name=="windows-kit-turn-restored.png")continue;
            bool allowed=false;
            if(gLookoutInspectKitPartials) {
                for(const char* side:{"left","right"})for(const char* angle:{"2","10","22_5","45"})
                    for(unsigned tick=0;tick<=18;++tick)if(gLookoutInspectKitPartialsDense || tick==0 || tick==6 || tick==9 || tick==12 || tick==18)
                        allowed|=name==std::string("windows-kit-turn-partial-")+side+"-"+angle+"-"+std::to_string(tick)+".png";
                if(!gLookoutInspectKitPartialsDense)for(unsigned frame=0;frame<=72;++frame)
                    allowed|=name=="windows-kit-turn-large-"+std::to_string(10000+frame).substr(1)+".png";
                if(allowed)continue;
            }
            for(const char* side:{"left","right"})for(const char* step:{"idle","blend-0","blend-4","blend-8","blend-11",
                "turn-20","turn-30","turn-40","turn-end","walk-0","walk-4","walk-8","walk-11","walk-20",
                "pause-a","pause-b","resume","reset"})
                allowed|=name==std::string("windows-kit-turn-")+side+"-"+step+".png";
            if(allowed)continue;
        }
        if(afterRun && gLookoutInspectKitMotion) {
            if(gLookoutInspectKitTiming && (name=="windows-kit-timing-walk.png" || name=="windows-kit-timing-turn.png"))continue;
            bool allowed=false;
            for(const char* step:{"start-0","start-1","start-4","start-8","walk-20","walk-35","walk-50","walk-65","walk-80","walk-100",
                "stop-0","stop-1","stop-4","stop-8","stop-11","turn-0","turn-10","turn-20","turn-30","turn-40",
                "turn-60","turn-80","turn-100","depart-0","depart-1","depart-4","depart-8","depart-20","depart-40","depart-60","depart-80",
                "waypoint-before","waypoint-tick","waypoint-after"})
                allowed|=name==std::string("windows-kit-motion-")+step+".png";
            if(allowed)continue;
        }
        if(afterRun && gLookoutInspectKitController) {
            bool allowed=false;
            for(unsigned frame=0;frame<=121;++frame)
                allowed|=name=="windows-kit-controller-depart-"+std::to_string(10000+frame).substr(1)+".png";
            for(unsigned frame=0;frame<=32;++frame)
                allowed|=name=="windows-kit-controller-ordinary-partial-"+std::to_string(10000+frame).substr(1)+".png";
            for(const char* stage:{"control","pause-a","pause-b","resume","stop","partial-begin","partial-mid",
                "opposite-request","opposite-mid","opposite-end","speech","speech-restored","slope",
                "corner-before","corner-at","corner-after","corner-walk-4","corner-walk-12",
                "corner-walk-24","reset","restored"})
                allowed|=name==std::string("windows-kit-controller-")+stage+".png";
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
    std::string loadedEvidence,entryEvidence;std::ostringstream kitEvidence,motionEvidence,timingEvidence,turnEvidence,controllerEvidence;
    unsigned controllerTicks=0,controllerCalls=0,controllerDepartureFrames=0,controllerFirstWalkTick=0;
    float controllerLegacyDepartureError=0,controllerDepartureError=0;
    bool controllerPauseIdentical=false,controllerResetIdentical=false,controllerActorReleased=false;
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
    if(proceed && gLookoutInspectKit && !gLookoutInspectKitController) {
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
    if(proceed && gLookoutInspectKitTurns) {
        // Explicit immutable playback on a separate CPU actor. This validates
        // RT joins and inspection cleanup, not a new gameplay turn state.
        using Inspection=horde::vulkan::raytracing::ChapterActorInspectionPose;
        using Image=horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture;
        struct ClearInspection {
            VulkanSurfaceContext& ctx;
            ~ClearInspection(){ctx.lookoutKitInspectionTurn.reset();ctx.lookoutKitInspectionMotion.reset();
                ctx.lookoutKitInspectionPose.reset();ctx.lookoutKitInspectionActing={};}
        } clear{ctx};
        const auto terrain=ctx.rtScene.ChapterTerrainSurface();
        dialogue::CompanionRouteFixture quiet;
        auto player=quiet.State();
        for(unsigned tick=0;tick<60;++tick)
            quiet.Step(1.f/60,player.x+3,player.z+1,true,false,false,terrain.get(),true);
        auto control=quiet.State();control.visualAdmitted=true;
        control.generation=ctx.simulation.Snapshot().chapterCompanion.generation+1;
        const auto destination=simulation::kWorldRoutePoints[4];
        const float heading=std::atan2(destination.x+1-control.x,control.z-destination.z);
        control.yaw=heading;control.animationSeconds=0;control.acting={};control.locomotion={};
        const auto capture=[&](const std::string& stage,dialogue::CompanionSnapshot pose,
                               std::optional<Inspection> inspection,Image* retained=nullptr) {
            ctx.lookoutKitInspectionMotion=pose;ctx.lookoutKitInspectionTurn=inspection;
            ctx.lookoutKitInspectionPose=horde::vulkan::raytracing::EnvironmentSurveyPose{
                pose.x+3.f,pose.z,pose.y,-1.57079632679f,-.15f,0};
            ctx.lookoutKitInspectionActing={};std::string evidence,error;Image image;
            const bool ok=check(pose.grounded && render() && completed(false,evidence) &&
                ctx.rtScene.ChapterActorInspectionPublishedPose()==inspection &&
                ctx.rtScene.CaptureStorageImage(image,error) &&
                WriteRgbaPng(gCampaignValidationDirectory/("windows-kit-turn-"+stage+".png"),image,error) &&
                storySame() && neutral(),"explicit turn playback publishes requested pose through completed hardware RT without campaign mutation");
            if(!error.empty())std::cerr<<"HORDE_KIT_TURN_ERROR "<<error<<'\n';
            if(!turnEvidence.str().empty())turnEvidence<<',';
            turnEvidence<<"{\"stage\":\""<<stage<<"\",\"root\":["<<pose.x<<','<<pose.y<<','<<pose.z
                <<"],\"logicalYaw\":"<<pose.yaw<<",\"inspection\":";
            if(inspection)turnEvidence<<"{\"clip\":"<<static_cast<unsigned>(inspection->clip)
                <<",\"seconds\":"<<inspection->seconds<<",\"phase\":"<<inspection->turnPhase
                <<",\"hasBlend\":"<<(inspection->hasBlend?"true":"false")
                <<",\"blendClip\":"<<static_cast<unsigned>(inspection->blendClip)
                <<",\"blendSeconds\":"<<inspection->blendSeconds<<",\"weight\":"<<inspection->weight<<'}';
            else turnEvidence<<"null";
            turnEvidence<<",\"rtEvidence\":"<<(evidence.empty()?"null":evidence)<<'}';
            if(retained)*retained=std::move(image);
            return ok;
        };
        Image original,restored;
        proceed=capture("control",control,std::nullopt,&original);
        std::string error;
        if(proceed)proceed=check(ctx.rtScene.LoadChapterActorInspection(
            std::filesystem::path(HORDE_RT_SOURCE_DIR)/"assets/models/characters/source/kit-step-turn-proof-v1/native",error),
            "separate Debug candidate matches original topology UV material and primitive GPU addressing");
        if(!error.empty())std::cerr<<"HORDE_KIT_TURN_ERROR "<<error<<'\n';
        const auto sameImage=[](const Image& a,const Image& b){return a.width==b.width && a.height==b.height && a.rgba==b.rgba;};
        const auto yawCurve=[](float phase){const float u=std::clamp((phase-.04f)/.92f,0.f,1.f);
            return .7853981633974483f*u*u*(3-2*u);};
        if(gLookoutInspectKitPartials) {
            constexpr std::array<float,4> angles{2,10,22.5f,45};
            constexpr std::array<const char*,4> labels{"2","10","22_5","45"};
            for(unsigned side=0;side<2 && proceed;++side)for(unsigned angle=0;angle<angles.size() && proceed;++angle) {
                const float amplitude=angles[angle]/45.f;
                const float target=kit_turn_contact::Yaw(side!=0,1)*amplitude;
                for(unsigned tick=0;tick<=18;++tick) {
                    if(!gLookoutInspectKitPartialsDense && tick!=0 && tick!=6 && tick!=9 && tick!=12 && tick!=18)continue;
                    if(!proceed)break;
                    const float phase=tick/18.f;
                    auto pose=control;pose.yaw=heading+target-kit_turn_contact::Yaw(side!=0,phase)*amplitude;
                    Inspection selected;selected.clip=Inspection::Clip::Idle;selected.turnPhase=phase;
                    selected.hasBlend=true;selected.blendClip=side?Inspection::Clip::TurnRight:Inspection::Clip::TurnLeft;
                    selected.weight=amplitude;
                    proceed=capture(std::string("partial-")+(side?"right":"left")+"-"+labels[angle]+"-"+std::to_string(tick),pose,selected);
                }
            }
            // Four original 45-degree cycles, sampled at 24 Hz: 3 seconds of
            // authored time. Image playback rate is not observed engine FPS.
            if(!gLookoutInspectKitPartialsDense)for(unsigned frame=0;frame<=72 && proceed;++frame) {
                const unsigned cycle=std::min(frame/18,3u);const float phase=(frame-cycle*18)/18.f;
                auto pose=control;pose.yaw=heading+kit_turn_contact::Yaw(false,1)*(4-cycle)-kit_turn_contact::Yaw(false,phase);
                Inspection selected;selected.clip=Inspection::Clip::TurnLeft;selected.turnPhase=phase;
                proceed=capture("large-"+std::to_string(10000+frame).substr(1),pose,selected);
            }
        }
        if(!gLookoutInspectKitPartials)for(unsigned side=0;side<2 && proceed;++side) {
            const std::string name=side?"right":"left";const float sign=side?-1.f:1.f;
            const auto turnClip=side?Inspection::Clip::TurnRight:Inspection::Clip::TurnLeft;
            auto pose=control;pose.yaw=heading+sign*.7853981633974483f;pose.animationSeconds=1.f;
            proceed=capture(name+"-idle",pose,std::nullopt);
            // Existing .18-second gait blend envelope; no production pacing
            // change. Nonzero Idle phase intentionally exercises the entry join.
            for(unsigned tick:{0u,4u,8u,11u,20u,30u,40u,45u}) {
                if(!proceed)break;
                const float seconds=tick/60.f,phase=seconds/.75f;
                pose.yaw=heading+sign*(.7853981633974483f-yawCurve(phase));
                Inspection selected;selected.turnPhase=phase;
                if(tick<11) {selected.clip=Inspection::Clip::Idle;selected.seconds=1+seconds;
                    selected.hasBlend=true;selected.blendClip=turnClip;selected.weight=std::min(seconds/.18f,1.f);}
                else selected.clip=turnClip;
                const auto stage=tick<=11?"blend-"+std::to_string(tick):
                    tick==45?std::string("turn-end"):"turn-"+std::to_string(tick);
                proceed=capture(name+"-"+stage,pose,selected);
            }
            dialogue::CompanionRouteFixture walking=quiet;
            for(unsigned tick=0;tick<=20 && proceed;++tick) {
                if(tick) {const auto before=walking.State();walking.Step(1.f/60,before.x+3,before.z+1,
                    true,true,false,terrain.get(),true);}
                if(tick!=0 && tick!=4 && tick!=8 && tick!=11 && tick!=20)continue;
                pose=walking.State();pose.visualAdmitted=true;pose.generation=control.generation;pose.yaw=heading;
                Inspection selected;selected.clip=turnClip;selected.turnPhase=1;selected.hasBlend=true;
                selected.blendClip=Inspection::Clip::Walk;
                selected.blendSeconds=pose.locomotion.walkPhase*(5.f/6.f);
                selected.weight=std::min((tick/60.f)/.18f,1.f);
                proceed=capture(name+"-walk-"+std::to_string(tick),pose,selected);
            }
            // Frozen diagnostic clocks remain frozen across completed frames.
            // Resuming advances once; clearing must restore original pose bytes.
            pose=control;float phase=.5f;bool paused=true;
            const auto advance=[&]{if(!paused)phase+=1.f/60/.75f;
                pose.yaw=heading+sign*(.7853981633974483f-yawCurve(phase));
                Inspection selected;selected.clip=turnClip;selected.turnPhase=phase;return selected;};
            Image pauseA,pauseB;
            if(proceed) {auto selected=advance();proceed=capture(name+"-pause-a",pose,selected,&pauseA);}
            if(proceed) {auto selected=advance();proceed=capture(name+"-pause-b",pose,selected,&pauseB);}
            if(proceed)proceed=check(phase==.5f && sameImage(pauseA,pauseB),"paused diagnostic turn clocks and actual RT image remain unchanged");
            paused=false;
            if(proceed) {auto selected=advance();proceed=capture(name+"-resume",pose,selected) &&
                check(phase>.5f,"resumed diagnostic turn advances explicit phase once");}
            if(proceed)proceed=capture(name+"-reset",control,std::nullopt,&restored) &&
                check(sameImage(original,restored),"cleared candidate renders byte-identical original actor and scene");
        }
        if(proceed)proceed=capture("restored",control,std::nullopt,&restored) &&
            check(sameImage(original,restored) && ctx.rtScene.ReleaseChapterActorInspection(error),
                "original actor restored before candidate CPU data and worker reference are released");
        ctx.lookoutKitInspectionTurn.reset();ctx.lookoutKitInspectionMotion.reset();
        ctx.lookoutKitInspectionPose.reset();ctx.lookoutKitInspectionActing={};
        if(proceed)proceed=check(render() && storySame(),"ordinary saved camera restored before existing Resume and scene retirement checks");
    }
    if(proceed && gLookoutInspectKitController) {
        // The loaded campaign remains paused and immutable. Only this local
        // shared controller advances on actual admitted terrain. Proof is an
        // explicit diagnostic input, not earned dialogue or campaign progress.
        using Mode=dialogue::CompanionMotionMode;
        using Image=horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture;
        struct ClearControllerInspection {
            VulkanSurfaceContext& ctx;
            ~ClearControllerInspection(){ctx.lookoutKitInspectionTurn.reset();ctx.lookoutKitInspectionMotion.reset();
                ctx.lookoutKitInspectionPose.reset();ctx.lookoutKitInspectionActing={};}
        } clear{ctx};
        const auto terrain=ctx.rtScene.ChapterTerrainSurface();
        dialogue::CompanionRouteFixture route(true),legacy;
        dialogue::Snapshot inspectionSpeech;
        float playerX=route.State().x-7,playerZ=route.State().z;
        float cameraAzimuth=0;
        bool controllerProof=true;
        std::uint64_t previousSubmission=0;
        const auto sameImage=[](const Image& a,const Image& b){return a.width==b.width && a.height==b.height && a.rgba==b.rgba;};
        const auto heading=[](const dialogue::CompanionSnapshot& pose) {
            const auto destination=simulation::kWorldRoutePoints[std::min(pose.waypoint,simulation::kWorldRoutePoints.size()-1)];
            return std::atan2(destination.x+1-pose.x,pose.z-destination.z);
        };
        const auto headingError=[](float target,float yaw){return std::remainder(target-yaw,6.28318530718f);};
        const auto step=[&](bool paused=false) {
            if(!terrain || !safe("shared-controller-fixed-step",false))return false;
            ++controllerCalls;
            route.Step(1.f/60,playerX,playerZ,true,controllerProof,paused,terrain.get(),true,true);
            if(!paused) {++controllerTicks;route.AdvanceActing(1.f/60,inspectionSpeech,true,true,false);}
            const auto& p=route.State();const auto& m=p.motion;
            return p.grounded && m.turnStartEnabled && std::isfinite(p.yaw) &&
                std::isfinite(m.speed) && m.speed>=0 && m.speed<=1.800001f &&
                m.turnPhase>=0 && m.turnPhase<=1 && m.entryWeight>=0 && m.entryWeight<=1 &&
                m.idleZeroWeight>=0 && m.idleZeroWeight<=1;
        };
        const auto capture=[&](const std::string& stage,dialogue::CompanionSnapshot pose,Image* retained=nullptr) {
            const auto sourceGeneration=pose.generation;
            pose.visualAdmitted=true;pose.generation+=ctx.simulation.Snapshot().chapterCompanion.generation;
            const float cameraX=pose.x+3*std::sin(cameraAzimuth),cameraZ=pose.z-3*std::cos(cameraAzimuth);
            const auto cameraGround=terrain->Query(cameraX,cameraZ);
            const float pitch=std::clamp(.05f+(pose.y+.9f-simulation::PlayerEyeWorldY(cameraGround.worldY))/3.f,-.32f,.28f);
            const auto support=traversal::ChapterExteriorStepSupport(*terrain,pose.x,pose.z,.24f);
            ctx.lookoutKitInspectionMotion=pose;ctx.lookoutKitInspectionTurn.reset();ctx.lookoutKitInspectionActing=pose.acting;
            ctx.lookoutKitInspectionPose=horde::vulkan::raytracing::EnvironmentSurveyPose{
                cameraX,cameraZ,cameraGround.worldY,cameraAzimuth+3.14159265359f,pitch,0};
            std::string evidence,error;Image image;
            bool ok=pose.grounded && cameraGround.found && cameraGround.walkable && support.grounded && support.worldY==pose.y &&
                render() && completed(false,evidence);
            const auto published=ctx.rtScene.ChapterCompanionPublishedMotion();
            const auto submission=ctx.rtFrameEvidence.PublishedStateByValue().completedEvidence.identity.submitted.submissionSerial;
            ok=ok && submission>previousSubmission && published==pose.motion &&
                !ctx.rtScene.ChapterActorInspectionPublishedPose() &&
                ctx.rtScene.CaptureStorageImage(image,error) &&
                WriteRgbaPng(gCampaignValidationDirectory/("windows-kit-controller-"+stage+".png"),image,error) &&
                ctx.simulationPaused && storySame() && neutral();
            if(ok)previousSubmission=submission;
            if(!error.empty())std::cerr<<"HORDE_KIT_CONTROLLER_ERROR "<<stage<<' '<<error<<'\n';
            std::cerr<<"HORDE_KIT_CONTROLLER_FRAME stage="<<stage<<" tick="<<controllerTicks
                <<" mode="<<static_cast<unsigned>(pose.motion.mode)<<" pass="<<ok<<'\n';
            if(!controllerEvidence.str().empty())controllerEvidence<<',';
            const auto& m=pose.motion;
            controllerEvidence<<std::setprecision(9)<<"{\"stage\":\""<<stage<<"\",\"pass\":"<<(ok?"true":"false")<<",\"fixedStepCalls\":"<<controllerCalls
                <<",\"activeFixedTicks\":"<<controllerTicks<<",\"authoredSeconds\":"<<controllerTicks/60.0
                <<",\"sourceGeneration\":"<<sourceGeneration<<",\"renderGeneration\":"<<pose.generation
                <<",\"root\":["<<pose.x<<','<<pose.y<<','<<pose.z<<"],\"logicalYaw\":"<<pose.yaw
                <<",\"walking\":"<<(pose.walking?"true":"false")<<",\"waypoint\":"<<pose.waypoint
                <<",\"wait\":"<<static_cast<unsigned>(pose.wait)<<",\"playerIntent\":["<<playerX<<','<<playerZ<<"]"
                <<",\"proofInput\":"<<(controllerProof?"true":"false")<<",\"idleSeconds\":"<<pose.animationSeconds<<",\"walkPhase\":"<<pose.locomotion.walkPhase
                <<",\"walkWeight\":"<<pose.locomotion.walkWeight<<",\"motion\":{\"turnStartEnabled\":"<<(m.turnStartEnabled?"true":"false")
                <<",\"mode\":"<<static_cast<unsigned>(m.mode)<<",\"speed\":"<<m.speed<<",\"turnPhase\":"<<m.turnPhase
                <<",\"turnPlaybackSeconds\":"<<m.turnPlaybackSeconds<<",\"signedTurnAngleRadians\":"<<m.turnAngleRadians<<",\"entryWeight\":"<<m.entryWeight
                <<",\"idleZeroWeight\":"<<m.idleZeroWeight<<"},\"speechMetadataLine\":"<<static_cast<unsigned>(inspectionSpeech.line)
                <<",\"acting\":["<<pose.acting.headPitch<<','<<pose.acting.headYaw<<','<<pose.acting.chestPitch
                <<"],\"camera\":["<<cameraX<<','<<cameraGround.worldY<<','<<cameraZ<<','<<cameraAzimuth+3.14159265359f<<','<<pitch
                <<"],\"supportId\":"<<static_cast<unsigned>(support.id)<<",\"rtEvidence\":"<<(evidence.empty()?"null":evidence)<<'}';
            if(retained)*retained=std::move(image);
            return check(ok,"shared-controller snapshot uses the normal actor and its published motion through a unique completed RT receipt");
        };
        const auto origin=route.State();
        unsigned waiting=0;
        while(proceed && waiting++<240) {
            proceed=step();
            const auto& pose=route.State();
            if(!proceed || (pose.motion.mode==Mode::Idle && std::abs(headingError(-1.57079632679f,pose.yaw))<.01745329252f))break;
        }
        // Matched timing cohorts start from the same active clock, heading and
        // ordinary Idle. Advance naturally; never seed yaw or a pose clock.
        for(unsigned tick=0;tick<240 && controllerTicks<240 && proceed;++tick)proceed=step();
        proceed=proceed && check(waiting<=240 && route.State().grounded && !route.State().walking &&
            route.State().wait==dialogue::CompanionWait::PlayerLagging && route.State().motion.mode==Mode::Idle &&
            route.State().motion.idleZeroWeight==0 && route.State().animationSeconds>0 &&
            route.State().x==origin.x && route.State().z==origin.z,
            "proof-true seven-metre tether wait finishes facing and ordinary Idle without seeded yaw or root travel");
        auto ordinary=route.State();ordinary.motion={};
        // One fixed world azimuth across departure. Orbiting with logical yaw
        // would hide turns/contact motion against the environment. This bisects
        // the actual shortest arrival/departure facing arc at three metres.
        cameraAzimuth=route.State().yaw+.5f*headingError(heading(route.State()),route.State().yaw);
        Image control,restored;
        if(proceed)proceed=capture("control",ordinary,&control);
        std::string error;
        if(proceed)proceed=check(!ctx.rtScene.ChapterActorInspectionPublishedPose(),
            "shared turn validation uses the packaged normal actor without loading a Debug inspection actor");
        if(proceed) {
            for(unsigned tick=0;tick<240;++tick) {
                const auto old=legacy.State();legacy.Step(1.f/60,old.x-7,old.z,true,true,false,terrain.get(),true);
            }
            const auto old=legacy.State();legacy.Step(1.f/60,old.x-3,old.z,true,true,false,terrain.get(),true);
            controllerLegacyDepartureError=std::abs(headingError(heading(old),legacy.State().yaw));
            proceed=check(legacy.State().grounded && legacy.State().walking,
                "unchanged legacy controller records its first moving departure-facing error on the same terrain and tether context");
        }
        playerX=route.State().x-3;playerZ=route.State().z;
        const auto departureOrigin=route.State();const auto departureStart=controllerTicks;
        bool pausedOnce=false;
        if(proceed) {proceed=capture("depart-0000",route.State());controllerDepartureFrames=1;}
        for(unsigned tick=1;tick<=240 && proceed;++tick) {
            const auto before=route.State();
            proceed=step();
            const auto& now=route.State();
            if(!controllerFirstWalkTick && now.walking) {
                controllerFirstWalkTick=controllerTicks;
                controllerDepartureError=std::abs(headingError(heading(before),now.yaw));
                proceed=proceed && check(controllerDepartureError<=.01745329252f &&
                    controllerDepartureError<controllerLegacyDepartureError,
                    "first accepted controller travel starts within its existing one-degree departure heading band");
            }
            if(!controllerFirstWalkTick)proceed=proceed && check(now.x==departureOrigin.x && now.z==departureOrigin.z,
                "departure entry and chained turns retain the exact stationary route root");
            if(proceed && (tick%2==0 || (now.walking && controllerTicks==controllerFirstWalkTick))) {
                proceed=check(controllerDepartureFrames<=121,"continuous departure remains within its bounded image roster") &&
                    capture("depart-"+std::to_string(10000+controllerDepartureFrames).substr(1),now);
                ++controllerDepartureFrames;
            }
            if(proceed && !pausedOnce && now.motion.mode==Mode::Turning && now.motion.entryWeight==1 &&
               now.motion.turnPhase>=.4f && now.motion.turnPhase<.8f) {
                const auto frozen=now;Image a,b;
                proceed=step(true) && capture("pause-a",route.State(),&a) && step(true) && step(true) &&
                    capture("pause-b",route.State(),&b);
                const auto& p=route.State();
                controllerPauseIdentical=proceed && sameImage(a,b);
                proceed=proceed && check(controllerPauseIdentical && p.motion==frozen.motion && p.locomotion==frozen.locomotion &&
                    p.animationSeconds==frozen.animationSeconds && p.x==frozen.x && p.y==frozen.y && p.z==frozen.z && p.yaw==frozen.yaw,
                    "paused shared controller freezes exact clocks root yaw gait and two original RT images");
                if(proceed)proceed=step() && capture("resume",route.State()) &&
                    check(route.State().motion.turnPhase>frozen.motion.turnPhase,"resume advances the shared turn exactly through its normal fixed step");
                pausedOnce=true;
            }
            if(controllerFirstWalkTick && controllerTicks-controllerFirstWalkTick>=24)break;
        }
        proceed=proceed && check(controllerFirstWalkTick>departureStart && controllerTicks-controllerFirstWalkTick>=24 && pausedOnce,
            "continuous thirty-Hz-authored departure reaches first walking plus four tenths of a second within 240 ticks");
        const auto setBearing=[&](float yaw){playerX=route.State().x+7*std::sin(yaw);playerZ=route.State().z-7*std::cos(yaw);};
        const auto settle=[&](unsigned limit) {
            for(unsigned tick=0;tick<limit;++tick) {
                if(!step())return false;
                if(route.State().motion.mode==Mode::Idle && route.State().locomotion.walkWeight==0)return true;
            }
            return false;
        };
        if(proceed) {
            setBearing(route.State().yaw);
            proceed=check(settle(90),"tether stop settles the same controller without root relocation") && capture("stop",route.State());
        }
        if(proceed) {
            // Retiming shortens the earlier departure. Equalize ordinary Idle
            // naturally so the partial's initial pose clock matches both builds.
            for(unsigned tick=0;tick<600 && controllerTicks<600 && proceed;++tick)proceed=step();
            proceed=proceed && check(controllerTicks==600 && route.State().motion.mode==Mode::Idle &&
                route.State().motion.idleZeroWeight==0 && route.State().locomotion.walkWeight==0,
                "ordinary partial starts from the matched six-hundred-tick Idle clock");
        }
        if(proceed) {
            // One uninterrupted ordinary partial, sampled on its real shared
            // fixed-step clock. The later opposite-intent witness stays intact.
            controllerProof=false; // Existing non-travel conversational facing context.
            const auto base=route.State();cameraAzimuth=base.yaw+.6f;
            setBearing(base.yaw+.3490658504f);
            unsigned frames=1;bool finished=false;
            proceed=capture("ordinary-partial-0000",base);
            for(unsigned tick=1;tick<=90 && proceed;++tick) {
                proceed=step();const auto& pose=route.State();
                finished=tick>1 && pose.motion.mode==Mode::Idle;
                proceed=proceed && check(pose.x==base.x && pose.z==base.z && !pose.walking &&
                    pose.locomotion.walkPhase==base.locomotion.walkPhase,
                    "ordinary partial retains its supported root and accepted gait phase");
                if(proceed && (tick%2==0 || finished)) {
                    proceed=check(frames<=32,"ordinary partial stays within its finite image roster") &&
                        capture("ordinary-partial-"+std::to_string(10000+frames).substr(1),pose);
                    ++frames;
                }
                if(finished)break;
            }
            proceed=proceed && check(finished && std::abs(headingError(base.yaw+.3490658504f,route.State().yaw))<.01745329252f,
                "ordinary twenty-degree turn completes its full supported exchange before returning to Idle");
        }
        if(proceed) {
            const auto base=route.State();cameraAzimuth=base.yaw+.6f;
            setBearing(base.yaw+.3490658504f);
            proceed=step() && capture("partial-begin",route.State());
            for(unsigned tick=0;tick<90 && proceed && route.State().motion.turnPhase<.45f;++tick)proceed=step();
            proceed=proceed && check(route.State().motion.mode==Mode::Turning && route.State().motion.turnAngleRadians>0 &&
                route.State().motion.turnAngleRadians<dialogue::companion_turn::maxAngle &&
                route.State().x==base.x && route.State().z==base.z,"bounded twenty-degree player intent produces a partial stationary shared turn") &&
                capture("partial-mid",route.State());
            setBearing(base.yaw-.1745329252f);
            if(proceed)proceed=step() && capture("opposite-request",route.State());
            bool opposite=false;
            for(unsigned tick=0;tick<120 && proceed;++tick) {
                proceed=step();
                if(route.State().motion.mode==Mode::Turning && route.State().motion.turnAngleRadians<0 && route.State().motion.turnPhase>=.45f) {
                    opposite=true;break;
                }
            }
            proceed=proceed && check(opposite && route.State().x==base.x && route.State().z==base.z,
                "changed opposite intent schedules the next partial turn without moving the planted route root") && capture("opposite-mid",route.State());
            if(proceed)proceed=check(settle(90),"opposite intent settles through the same controller") && capture("opposite-end",route.State());
        }
        if(proceed) {
            inspectionSpeech.line=dialogue::Line::ForestNight;inspectionSpeech.duration=dialogue::Spec(inspectionSpeech.line).duration;
            for(unsigned tick=0;tick<30 && proceed;++tick) {inspectionSpeech.elapsed=(tick+1)/60.f;proceed=step();}
            if(proceed)proceed=capture("speech",route.State());
            inspectionSpeech={};
            for(unsigned tick=0;tick<60 && proceed;++tick)proceed=step();
            if(proceed)proceed=capture("speech-restored",route.State()) && check(route.State().acting==dialogue::CompanionActingPose{},
                "diagnostic speech metadata decays without modifying loaded completed dialogue");
        }
        if(proceed) {
            // Continue the actual local route to its first corner; no pose or
            // waypoint import. This is a controller snapshot fixture, not a
            // second campaign/rope/save journey or an earned speech assertion.
            controllerProof=true; // Resume the existing travel intent at its next exchange.
            cameraAzimuth=heading(route.State())+.6f;unsigned ticks=0;bool slope=false;
            const auto target=simulation::kWorldRoutePoints[4];
            while(proceed && route.State().waypoint==4 && std::hypot(route.State().x-target.x-1,route.State().z-target.z)>=.06f && ticks++<1200) {
                playerX=route.State().x-3;playerZ=route.State().z;proceed=step();
                if(proceed && ticks==120){slope=true;proceed=capture("slope",route.State());}
            }
            proceed=proceed && check(ticks<=1200 && slope && route.State().waypoint==4 &&
                std::hypot(route.State().x-target.x-1,route.State().z-target.z)<.06f,
                "actual supported local route reaches the first corner within its finite CPU tick budget") && capture("corner-before",route.State());
            const auto before=route.State();
            if(proceed)proceed=step() && check(route.State().waypoint==5 && route.State().x==before.x && route.State().z==before.z &&
                route.State().yaw==before.yaw && route.State().motion.mode==Mode::Walking && !route.State().walking &&
                route.State().locomotion.walkPhase==before.locomotion.walkPhase && route.State().locomotion.walkWeight>.8f,
                "corner bookkeeping retains Walking mode, travel facing and nonzero gait without moving the root") && capture("corner-at",route.State());
            for(unsigned tick=1;tick<=24 && proceed;++tick) {
                const auto old=route.State();
                playerX=route.State().x-3;playerZ=route.State().z;proceed=step();
                const auto& pose=route.State();
                if(!proceed)break;
                const float distance=std::hypot(pose.x-old.x,pose.z-old.z);
                const auto destination=simulation::kWorldRoutePoints[old.waypoint];
                const float expectedTravel=std::min(std::hypot(destination.x+1-old.x,destination.z-old.z),1.8f/60);
                const float expectedYaw=old.yaw+std::clamp(headingError(heading(old),old.yaw),-2.4f/60,2.4f/60);
                const float expectedPhase=std::fmod(old.locomotion.walkPhase+distance/1.56f,1.f);
                const float expectedWeight=old.locomotion.walkWeight+
                    std::clamp(1.f-old.locomotion.walkWeight,-1.f/(60*.18f),1.f/(60*.18f));
                proceed=check(pose.motion.mode==Mode::Walking && pose.walking && distance>0 && pose.waypoint==5 &&
                    std::abs(distance-expectedTravel)<=4e-6f && std::abs(headingError(expectedYaw,pose.yaw))<1e-6f &&
                    std::abs(std::remainder(pose.locomotion.walkPhase-expectedPhase,1.f))<2e-5f &&
                    std::abs(pose.locomotion.walkWeight-expectedWeight)<1e-6f,
                    "routine first corner keeps legacy travel, bounded yaw and accepted-distance gait without a planted restart");
                if(proceed && tick==1)proceed=capture("corner-after",pose);
                if(proceed && (tick==4 || tick==12 || tick==24))proceed=capture("corner-walk-"+std::to_string(tick),pose);
            }
        }
        if(proceed) {
            const auto generation=route.State().generation;route.Reset();
            const auto& reset=route.State();
            proceed=check(reset.generation>generation && reset.motion.turnStartEnabled && reset.motion.mode==Mode::Idle &&
                reset.motion.speed==0 && reset.motion.turnPhase==0 && reset.motion.turnAngleRadians==0 &&
                reset.motion.entryWeight==0 && reset.motion.idleZeroWeight==0 && reset.animationSeconds==0 &&
                reset.locomotion==dialogue::CompanionLocomotionPose{} && reset.acting==dialogue::CompanionActingPose{},
                "local controller reset revokes transient turn start gait and acting state without touching the campaign");
            playerX=reset.x-7;playerZ=reset.z;
            if(proceed)proceed=step() && capture("reset",route.State());
            cameraAzimuth=ordinary.yaw+.5f*headingError(heading(ordinary),ordinary.yaw);
            if(proceed)proceed=capture("restored",ordinary,&restored);
            controllerResetIdentical=proceed && sameImage(control,restored);
            if(proceed)proceed=check(controllerResetIdentical && !ctx.rtScene.ChapterActorInspectionPublishedPose(),
                "cleared controller renders byte-identical ordinary pose using the same normal actor before scene retirement");
            controllerActorReleased=proceed;
        }
        ctx.lookoutKitInspectionTurn.reset();ctx.lookoutKitInspectionMotion.reset();
        ctx.lookoutKitInspectionPose.reset();ctx.lookoutKitInspectionActing={};
        if(proceed)proceed=check(render() && storySame() && !ctx.rtScene.ChapterActorInspectionPublishedPose(),
            "ordinary loaded camera and actor are restored before existing native Resume and menu replacement checks");
    }
    if(proceed && gLookoutInspectKitMotion) {
        // A sequential production-route visual fixture. It samples real fixed
        // steps/support and renders their immutable snapshots, without changing
        // the paused loaded campaign, earning progression or touching records.
        const auto terrain=ctx.rtScene.ChapterTerrainSurface();
        dialogue::CompanionRouteFixture route;
        std::array<std::vector<dialogue::CompanionSnapshot>,2> timingPoses;
        auto player=route.State();
        for(unsigned tick=0;tick<60;++tick)
            route.Step(1.f/60,player.x+3,player.z+1,true,false,false,terrain.get(),true);
        const auto capture=[&](const char* stage) {
            auto pose=route.State();pose.visualAdmitted=true;
            if(gLookoutInspectKitTiming) {
                const std::string_view name=stage;
                if(name.starts_with("walk-"))timingPoses[0].push_back(pose);
                if(name.starts_with("turn-") && name!="turn-0" && name!="turn-100")timingPoses[1].push_back(pose);
            }
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
                <<",\"waypoint\":"<<pose.waypoint<<",\"wait\":"<<static_cast<unsigned>(pose.wait)
                <<",\"idleSeconds\":"<<pose.animationSeconds<<",\"walkPhase\":"<<pose.locomotion.walkPhase
                <<",\"walkWeight\":"<<pose.locomotion.walkWeight<<",\"rtEvidence\":"<<(evidence.empty()?"null":evidence)<<'}';
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
        if(proceed && !gLookoutInspectKitTiming) {
            // Establish the real tether-wait branch before resuming the same
            // supported route. Proof remains true across wait and departure;
            // the earlier pose-inspection proof toggles are not this witness.
            // Never teleport, modify the motor, or change the timing cohort.
            for(unsigned tick=0;tick<100 && proceed;++tick) {
                const auto before=route.State();
                route.Step(1.f/60,before.x-7,before.z,true,true,false,terrain.get(),true);
                proceed=check(route.State().wait==dialogue::CompanionWait::PlayerLagging &&
                    !route.State().walking && route.State().grounded && route.State().x==before.x && route.State().z==before.z,
                    "tether wait retains proof and grounded root before departure inspection");
            }
            if(proceed)proceed=capture("depart-0");
            for(unsigned tick=1;tick<=80 && proceed;++tick) {
                const auto before=route.State();
                route.Step(1.f/60,before.x-3,before.z,true,true,false,terrain.get(),true);
                const char* name=tick==1?"depart-1":tick==4?"depart-4":tick==8?"depart-8":
                    tick==20?"depart-20":tick==40?"depart-40":tick==60?"depart-60":tick==80?"depart-80":nullptr;
                if(name)proceed=capture(name);
            }
            // A player on Kit's +X side stays within the accepted tether. At
            // the first waypoint, retain the last movement, bookkeeping-only
            // tick and next movement separately; never seed a completion flag.
            route.Reset();
            for(unsigned tick=0;tick<60;++tick) {
                const auto before=route.State();
                route.Step(1.f/60,before.x+1,before.z,true,false,false,terrain.get(),true);
            }
            const auto target=horde::gameplay::simulation::kWorldRoutePoints[4];
            unsigned ticks=0;
            while(proceed && route.State().waypoint==4 &&
                  std::hypot(route.State().x-(target.x+1),route.State().z-target.z)>=.06f && ticks++<600) {
                const auto before=route.State();
                route.Step(1.f/60,before.x+1,before.z,true,true,false,terrain.get(),true);
                proceed=check(route.State().grounded && route.State().walking,
                    "waypoint inspection advances the supported production route within its finite tick budget");
            }
            proceed=proceed && check(ticks<=600 && route.State().waypoint==4 &&
                std::hypot(route.State().x-(target.x+1),route.State().z-target.z)<.06f,
                "waypoint inspection reaches the actual first landmark without teleport or completion edits");
            if(proceed)proceed=capture("waypoint-before");
            if(proceed) {
                const auto before=route.State();
                route.Step(1.f/60,before.x+1,before.z,true,true,false,terrain.get(),true);
                proceed=check(route.State().waypoint==5 && !route.State().walking &&
                    route.State().x==before.x && route.State().z==before.z,
                    "waypoint bookkeeping advances identity without translating the actor");
                if(proceed)proceed=capture("waypoint-tick");
            }
            if(proceed) {
                const auto before=route.State();
                route.Step(1.f/60,before.x+1,before.z,true,true,false,terrain.get(),true);
                proceed=capture("waypoint-after");
            }
        }
        if(proceed && gLookoutInspectKitTiming) {
            // One finite profiling workload. Both builds replay identical
            // immutable roots/clocks/poses; only the owned additive leg layer
            // differs. Keep the existing 180-frame and 90-second limits.
            for(unsigned group=0;group<2 && proceed;++group) {
                proceed=check(timingPoses[group].size()==6,"timing workload has exactly six unchanged-walk or stationary-turn snapshots");
                for(unsigned sample=0;sample<72 && proceed;++sample) {
                    const auto& pose=timingPoses[group][sample%6];
                    ctx.lookoutKitInspectionMotion=pose;
                    ctx.lookoutKitInspectionPose=horde::vulkan::raytracing::EnvironmentSurveyPose{
                        pose.x+3.f,pose.z,pose.y,-1.57079632679f,-.15f,0};
                    std::string evidence;
                    proceed=render() && completed(false,evidence) && storySame() && neutral();
                    if(proceed && sample>=12) {
                        const auto utcMs=std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count();
                        if(!timingEvidence.str().empty())timingEvidence<<',';
                        timingEvidence<<"{\"group\":\""<<(group?"turn":"walk")<<"\",\"sample\":"<<sample-12
                            <<",\"pose\":"<<sample%6<<",\"utcUnixMs\":"<<utcMs<<",\"rtEvidence\":"<<evidence<<'}';
                    }
                }
                std::string error;horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
                proceed=check(proceed && ctx.rtScene.CaptureStorageImage(image,error) &&
                    WriteRgbaPng(gCampaignValidationDirectory/(group?"windows-kit-timing-turn.png":"windows-kit-timing-walk.png"),image,error),
                    "twelve warmups and sixty completed profiling samples retain native story/save/foreground guards and an original RT image");
            }
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
        <<",\"kitPoseInspection\":{\"requested\":"<<(gLookoutInspectKit && !gLookoutInspectKitController?"true":"false")
        <<",\"evidenceClass\":\"seeded render-only pose; not live fresh-raise acceptance\",\"cameraOffset\":[-3,0,0],\"poses\":["<<kitEvidence.str()<<"]}"
        <<",\"kitMotionInspection\":{\"requested\":"<<(gLookoutInspectKitMotion?"true":"false")
        <<",\"evidenceClass\":\"sequential fixed-step production-route render inputs; not live earned gameplay\",\"frames\":["<<motionEvidence.str()<<"]}"
        <<",\"kitTimingInspection\":{\"requested\":"<<(gLookoutInspectKitTiming?"true":"false")
        <<",\"evidenceClass\":\"bounded desktop profiling of repeated immutable poses; not sustained or phone acceptance\",\"samples\":["<<timingEvidence.str()<<"]}"
        <<",\"kitTurnInspection\":{\"requested\":"<<(gLookoutInspectKitTurns?"true":"false")
        <<",\"partialAnglesRequested\":"<<(gLookoutInspectKitPartials?"true":"false")
        <<",\"densePartialAnglesRequested\":"<<(gLookoutInspectKitPartialsDense?"true":"false")
        <<",\"largeSequenceAuthoredHz\":"<<(gLookoutInspectKitPartials && !gLookoutInspectKitPartialsDense?24:0)
        <<",\"evidenceClass\":\"isolated Debug candidate playback and RT joins; not gameplay turn-state or owner acceptance\",\"frames\":["<<turnEvidence.str()<<"]}"
        <<",\"kitControllerInspection\":{\"requested\":"<<(gLookoutInspectKitController?"true":"false")
        <<",\"authoredTurnSeconds\":"<<dialogue::companion_turn::duration<<",\"playbackTurnSeconds\":"<<dialogue::companion_turn::playbackDuration
        <<",\"evidenceClass\":\"isolated shared diagnostic controller snapshots on actual terrain; not earned journey or owner contact acceptance\""
        <<",\"departureNominalAuthoredHz\":30,\"cameraPolicy\":\"fixed world azimuth per case; three-metre translated offset on queried terrain\""
        <<",\"speechMetadataOnly\":true,\"actualRtCornerCoverage\":\"first route corner only\",\"fixedStepCalls\":"<<controllerCalls
        <<",\"activeFixedTicks\":"<<controllerTicks<<",\"departureFrames\":"<<controllerDepartureFrames
        <<",\"firstWalkActiveTick\":"<<controllerFirstWalkTick<<",\"legacyFirstWalkHeadingErrorRadians\":"<<controllerLegacyDepartureError
        <<",\"controllerFirstWalkHeadingErrorRadians\":"<<controllerDepartureError
        <<",\"pauseImagesIdentical\":"<<(controllerPauseIdentical?"true":"false")
        <<",\"resetOrdinaryImageIdentical\":"<<(controllerResetIdentical?"true":"false")
        <<",\"normalActorRestored\":"<<(controllerActorReleased?"true":"false")<<",\"inspectionActorUsed\":false,\"frames\":["<<controllerEvidence.str()<<"]}"
        <<",\"physicalVisibleAcceptance\":false,\"performanceEvidence\":false,\"ownerSettingsReadOrWritten\":false";
    return proceed && !failures?0:1;
}

int FinishHiddenLookoutValidation(HWND owner,int result)
{
    if(!ObserveHiddenLookoutForeground(owner,"owner-destroyed"))result=1;
    const int baseTimeLimit = gBellwetherPreparationsValidation ? 360 : gBellwetherExpeditionValidation ? 230 :
        gBellwetherCombinedCampaignActors && gBellwetherEarnTavernValidation ? 180 :
        gBellwetherAnkleRestoreValidation ? 180 : gBellwetherHearthRestValidation ? 110 : 90;
    const bool verifiesSilver = gBellwetherCombinedRelicDealer && gBellwetherEarnTavernValidation &&
        (!gBellwetherExpeditionValidation || gBellwetherPreparationsValidation);
    const int westernAllowance = verifiesSilver ? (gBellwetherPreparationsValidation ? 225 : 135) : 0;
    const int timeLimit = baseTimeLimit + westernAllowance;
    const auto elapsed = std::chrono::steady_clock::now()-gHiddenLookout.started;
    const bool inBudget = elapsed < std::chrono::seconds(timeLimit);
    if(!inBudget || gHiddenLookout.receipt.str().empty())result=1;
    auto& receipt=gHiddenLookout.receipt;
    if(receipt.str().empty())receipt<<"{\"schema\":1,\"checks\":[],\"stagesComplete\":false";
    receipt<<",\"foregroundSampleCount\":"<<gHiddenLookout.foregroundSamples
        <<",\"foregroundStoredSampleCount\":"<<gHiddenLookout.storedForegroundSamples
        <<",\"foregroundSamples\":["<<gHiddenLookout.foreground.str()<<"],\"foregroundContinuous\":"
        <<(!gHiddenLookout.foregroundChanged?"true":"false")<<",\"ownerObservedForeground\":"
        <<(gHiddenLookout.ownerObserved?"true":"false")<<",\"ownerShown\":"<<(gHiddenLookout.ownerShown?"true":"false")
        <<",\"withinNinetySeconds\":"<<(elapsed<std::chrono::seconds(90)?"true":"false")
        <<",\"timeLimitSeconds\":"<<timeLimit<<",\"withinTimeLimit\":"<<(inBudget?"true":"false")
        <<",\"complete\":"<<(result==0?"true":"false")<<"}\n";
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
int RunHiddenBellwetherCampaignValidation(VulkanSurfaceContext& ctx, horde::vulkan::DeviceCapabilities& capabilities)
{
    using horde::gameplay::campaign::CampaignLoadBarrier;
    using horde::gameplay::dialogue::BellwetherArrivalStage;
    using horde::telemetry::RtPresentationOutcome;
    using horde::vulkan::raytracing::RtSceneProfile;
    // The scripted arrival owns every movement and look input. Rendering that
    // snapshot must not also step the desktop input owner with its saved camera.
    struct RestoreInputOwner {
        VulkanSurfaceContext& context;
        bool previouslyScripted;
        ~RestoreInputOwner() { context.chapterRoundTripCaptureActive = previouslyScripted; }
    } restoreInputOwner{ctx,ctx.chapterRoundTripCaptureActive};
    if (gBellwetherEarnTavernValidation) ctx.chapterRoundTripCaptureActive = true;
    std::ostringstream newGameReceipt;
    auto& receipt = gLookoutHiddenValidation ? gHiddenLookout.receipt : newGameReceipt;
    receipt << "{\"schema\":1,\"checks\":[";
    unsigned checks = 0, failures = 0;
    const auto check = [&](bool pass, const char* name) {
        if (checks++) receipt << ',';
        if (!pass) ++failures;
        receipt << "{\"name\":\"" << name << "\",\"pass\":" << (pass ? "true" : "false") << '}';
        std::cerr << "HORDE_BELLWETHER_CAMPAIGN " << name << " pass=" << pass << '\n';
        return pass;
    };
    const HWND foreground = GetForegroundWindow();
    const auto hidden = [&] { return GetForegroundWindow() == foreground && !IsWindowVisible(ctx.windowHandle); };
    const auto click = [&](int id) {
        const auto control = GetDlgItem(ctx.windowHandle, id);
        if (!hidden() || !control || !IsWindowEnabled(control) || !(GetWindowLongPtrA(control, GWL_STYLE) & WS_VISIBLE)) return false;
        SendMessageA(ctx.windowHandle, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), reinterpret_cast<LPARAM>(control));
        return hidden();
    };
    const auto frame = [&] {
        const auto tick = ctx.simulation.Snapshot().tickIndex;
        bool presented = false;
        if (!RenderFrame(ctx, VkClearColorValue{{0,0,0,1}}, presented)) {
            std::cerr << "Bellwether render at tick " << tick << " failed: " << ctx.lastRtFrameError << '\n';
            return false;
        }
        if (gBellwetherEarnTavernValidation && ctx.simulation.Snapshot().tickIndex != tick) {
            std::cerr << "Bellwether presentation advanced the scripted campaign tick.\n";
            return false;
        }
        FinishGraphicsFrame(ctx, presented);
        ObserveCampaignPresentation(ctx, presented);
        return presented && ctx.lastFramePresentation == RtPresentationOutcome::Presented && hidden();
    };
    const int baseTimeLimit =
        gBellwetherPreparationsValidation ? 325 : gBellwetherExpeditionValidation ? 205 :
        gBellwetherCombinedRelicDealer ? 205 : gBellwetherCombinedCampaignActors ? 150 :
        gBellwetherAnkleRestoreValidation ? 150 : gBellwetherHearthRestValidation ? 85 : 65;
    const bool verifiesSilver = gBellwetherCombinedRelicDealer && gBellwetherEarnTavernValidation &&
        (!gBellwetherExpeditionValidation || gBellwetherPreparationsValidation);
    // The mixed campaign walks and reloads both western and preparation routes.
    const int westernAllowance = verifiesSilver ? (gBellwetherPreparationsValidation ? 200 : 110) : 0;
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::seconds(baseTimeLimit + westernAllowance);
    const auto until = [&](auto done, bool render) {
        while (std::chrono::steady_clock::now() < deadline && hidden()) {
            PumpCampaign(ctx);
            if (ctx.campaignPhase == CampaignMenu::Phase::Failed || ctx.campaignRetirementUnproved) return false;
            if (done()) return true;
            if (render) { if (!frame()) return false; }
            else std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    };
    const bool lookout = gLookoutHiddenValidation;
    ApplyOverlayState(ctx);
    const bool chooser = ctx.campaign && ctx.campaignMenu &&
        (lookout ? click(kEntryMoreButtonId) && click(kCampaignLoadButtonId) : click(kResumeButtonId));
    const bool loaded = chooser && ctx.campaignMenu->Visible() && until([&] {
        return std::none_of(ctx.campaign->Catalogue().begin(), ctx.campaign->Catalogue().end(),
            [](const auto& entry) { return entry.status == campaign::CatalogueStatus::Checking; });
    }, false) && click(CampaignMenu::FirstChoiceId) && (lookout || click(CampaignMenu::ConfirmId)) && until([&] {
        return ctx.campaignBarrier.State().phase == CampaignLoadBarrier::Phase::Complete;
    }, true);
    const auto& state = ctx.simulation.Snapshot();
    const bool entered = loaded && !ctx.campaignMenu->Visible() && !ctx.simulationPaused && !ctx.campaignPause &&
        ctx.campaign->State().activeSlot == 0 && ctx.rtScene.Profile() == RtSceneProfile::Showcase &&
        state.playerAlive && state.developmentRescueJourney && ctx.rtScene.ChapterResidentSourceCount() ==
            horde::vulkan::raytracing::kBellwetherResidentCount;
    if (lookout) {
        const bool atLookout = check(entered && state.rescue.exteriorSide && state.chapterDialogue.forestVillageConsumed &&
            state.chapterCompanion.bellwetherArrival == BellwetherArrivalStage::AtLookout,
            "ordinary Lookout load prepares both Bellwether residents with its presented scene");
        if (gBellwetherEarnTavernValidation) {
            using namespace horde::gameplay::dialogue;
            using horde::gameplay::traversal::BellwetherStation;
            using horde::gameplay::traversal::kBellwetherTavernRoute;
            const auto catalogueReady = [&] {
                return std::none_of(ctx.campaign->Catalogue().begin(), ctx.campaign->Catalogue().end(),
                    [](const auto& entry) { return entry.status == campaign::CatalogueStatus::Checking; });
            };
            const auto atHearth = [&] {
                return state.playerGrounded && state.chapterCompanion.bellwetherArrival == BellwetherArrivalStage::Seated &&
                    state.tavernKeeper && state.tavernKeeper->stage == TavernKeeperStage::AtHearth;
            };
            auto input = ctx.simulationInput;
            input.damageEnabled = input.tutorialEnabled = false;
            input.paused = false;
            input.moveForward = input.moveStrafe = 0;
            input.pitchRadians = -.15f;
            unsigned ticks = 0, exchangeLines = 0, presentationInterval = 60;
            bool assessmentObserved = false, ankleReloaded = false, ankleReplayed = false;
            std::optional<campaign::Checkpoint> lookoutBeforeAssessment;
            const auto step = [&] {
                if (std::chrono::steady_clock::now() >= deadline) {
                    std::cerr << "Bellwether campaign validation deadline at tick " << state.tickIndex << '\n';
                    return false;
                }
                if (!hidden() || ctx.simulationPaused ||
                    ctx.campaignRetirementUnproved || ctx.campaignPhase == CampaignMenu::Phase::Failed)
                    return false;
                ctx.simulationInput = input;
                ctx.inputPublicationSequence = std::max(ctx.inputPublicationSequence, state.inputPublicationSequence) + 1;
                ctx.simulation.StepFixed(input, 1.f / 60.f, ctx.inputPublicationSequence);
                if (gBellwetherAnkleRestoreValidation) {
                    const bool assessing = state.chapterCompanion.bellwetherArrival == BellwetherArrivalStage::AnkleAssessment;
                    assessmentObserved |= assessing;
                    if (assessing && !ankleReloaded)
                        lookoutBeforeAssessment = ctx.simulation.LatestCampaignBoundary();
                    if (ankleReloaded) {
                        const auto events = ctx.simulation.Events().Events();
                        ankleReplayed |= assessing || IsRoadLine(state.chapterDialogue.line) ||
                            std::any_of(events.begin(), events.end(), [](const auto& event) {
                                return event.type == horde::gameplay::simulation::GameplayEventType::KitAnkleFoley;
                            });
                    }
                }
                if (exchangeLines < kTavernConversation.size() &&
                    state.chapterDialogue.line == kTavernConversation[exchangeLines])
                    ++exchangeLines;
                PumpCampaign(ctx);
                return state.playerAlive && (++ticks % presentationInterval != 0 || frame());
            };
            const auto walkTo = [&](BellwetherStation target) {
                input.runHeld = true;
                for (unsigned tick = 0; tick < 1200; ++tick) {
                    const float dx = target.x - state.playerX, dz = target.z - state.playerZ;
                    const float distance = std::hypot(dx, dz);
                    if (distance < .135f) {
                        input.moveForward = 0;
                        input.runHeld = false;
                        return frame();
                    }
                    input.yawRadians = std::atan2(dx, -dz);
                    input.moveForward = std::min(distance / .20f, 1.f);
                    if (!step()) break;
                }
                std::cerr << "Bellwether walk stopped at " << state.playerX << ',' << state.playerZ
                          << " en route to " << target.x << ',' << target.z << '\n';
                return false;
            };
            const auto waitFor = [&](auto ready, unsigned maxTicks) {
                for (unsigned tick = 0; tick < maxTicks && !ready(); ++tick)
                    if (!step()) return false;
                return ready();
            };
            const auto reloadCheckpoint = [&] {
                const auto generation = state.worldRoute.generation;
                const auto operation = ctx.campaign->State().operationRevision;
                ShowPauseMenu(ctx, true);
                const bool reloaded = click(kCampaignLoadButtonId) && until(catalogueReady, false) &&
                    click(CampaignMenu::FirstChoiceId) && until([&] {
                        const auto play = ctx.campaign->State();
                        return play.operationRevision > operation && play.operation == campaign::PlayOperation::Load &&
                            play.outcome == campaign::PlayOutcome::Succeeded &&
                            ctx.campaignBarrier.State().phase == CampaignLoadBarrier::Phase::Complete;
                    }, true);
                // Rebase the diagnostic input at the native chooser's present barrier.
                input = ctx.simulationInput;
                input.paused = false;
                return reloaded && state.worldRoute.generation > generation &&
                    ctx.campaign->State().activeSlot == 0 && !ctx.campaignMenu->Visible() &&
                    !ctx.campaignPause && !ctx.simulationPaused;
            };
            const auto readDirections = [&](campaign::PauseObjective objective, std::string_view requiredText = {}) {
                const auto operation = ctx.campaign->State().operationRevision;
                const auto dialogueGeneration = state.chapterDialogue.generation;
                const auto breathTicks = state.shorePracticeBreathTicksRemaining;
                const float x = state.playerX, z = state.playerZ;
                ShowPauseMenu(ctx, true);
                if (!click(kLookoutObjectiveButtonId) || !ctx.lookoutObjective ||
                    !ctx.lookoutObjective->Visible() || !ctx.simulationPaused) {
                    std::cerr << "Bellwether directions did not open for objective " << static_cast<int>(objective) << '\n';
                    return false;
                }
                const auto body = GetDlgItem(ctx.windowHandle, LookoutPanel::BodyId);
                std::string text(static_cast<std::size_t>(GetWindowTextLengthA(body)) + 1, '\0');
                const int copied = GetWindowTextA(body, text.data(), static_cast<int>(text.size()));
                text.resize(static_cast<std::size_t>(copied));
                const bool correct = text == campaign::PauseObjectiveText(objective, campaign::CurrentBellwetherDirections(state)) &&
                    text.find(requiredText) != std::string::npos;
                const bool closed = click(LookoutPanel::BackId) && !ctx.lookoutObjective->Visible() &&
                    ctx.pauseMenuVisible && ctx.simulationPaused;
                const bool resumed = closed && click(kResumeButtonId) && !ctx.simulationPaused;
                input.paused = false;
                const bool unchanged = operation == ctx.campaign->State().operationRevision &&
                    dialogueGeneration == state.chapterDialogue.generation && x == state.playerX && z == state.playerZ &&
                    state.shorePracticeBreathTicksRemaining == breathTicks;
                if (!correct || !resumed || !unchanged)
                    std::cerr << "Bellwether directions objective=" << static_cast<int>(objective)
                              << " textCorrect=" << correct << " resumed=" << resumed
                              << " progressUnchanged=" << unchanged << '\n';
                return correct && resumed && unchanged && step();
            };
            const auto target = campaign::kBellwetherCheckpointPosition;
            // The scripted player runs ahead. Let Kit complete his independent
            // walk, sit and hearth lines on the same fixed-step clock.
            constexpr unsigned hearthWaitTicks = 120 * 60;
            bool progressed = atLookout && step();
            if (progressed && gBellwetherAnkleRestoreValidation) {
                constexpr auto station = horde::gameplay::traversal::kBellwetherAnkleStation;
                const auto assessmentSaved = [&] {
                    const auto& boundary = ctx.simulation.LatestCampaignBoundary();
                    return boundary && boundary->bellwetherAnkleAssessed &&
                        boundary->id == campaign::CheckpointId::ForestEndpoint;
                };
                campaign::storage::StoredCheckpoint assessed;
                const bool autosaved = std::all_of(kBellwetherTavernRoute.begin(),
                    kBellwetherTavernRoute.begin() + station + 1, walkTo) &&
                    waitFor(assessmentSaved, hearthWaitTicks) && until([&] {
                        return campaign::storage::Read(gCampaignValidationDirectory, "slot-1-auto.hrd", assessed).status ==
                            campaign::storage::Status::Read && assessed.checkpoint.bellwetherAnkleAssessed &&
                            ctx.campaign->State().outcome == campaign::PlayOutcome::Succeeded;
                    }, false);
                auto expected = lookoutBeforeAssessment.value_or(gHiddenLookout.source.checkpoint);
                expected.flaskRecorded = campaign::HasFlaskResources(expected);
                expected.schema = campaign::kBellwetherAnkleSchema;
                expected.bellwetherAnkleAssessed = true;
                expected.savedAtUnixSeconds = assessed.checkpoint.savedAtUnixSeconds;
                std::vector<std::byte> expectedBytes;
                campaign::codec::Error codecError{};
                progressed = check(autosaved && assessmentObserved && lookoutBeforeAssessment &&
                    campaign::codec::Encode(expected, expectedBytes, codecError) && assessed.encoded == expectedBytes,
                    "the completed ankle assessment updates the existing lookout autosave and preserves its safe state");
                ankleReloaded = progressed && reloadCheckpoint();
                const auto& kit = state.chapterCompanion;
                const auto& mark = kBellwetherTavernRoute[station];
                progressed = check(ankleReloaded && state.playerX == expected.position.x &&
                    state.playerZ == expected.position.z && kit.grounded && kit.x == mark.x && kit.z == mark.z &&
                    kit.bellwetherArrival == BellwetherArrivalStage::ProtectiveApproach &&
                    kit.bellwetherStation == station + 1 && state.chapterDialogue.line == Line::None && frame(),
                    "native Load presents Kit after the injury and retains the player's safe lookout spawn");
            }
            const auto afterRoadDirections = kBellwetherTavernRoute.begin() + 3;
            if (progressed) {
                progressed = check(std::all_of(kBellwetherTavernRoute.begin(), afterRoadDirections, walkTo) &&
                    readDirections(campaign::PauseObjective::BellwetherArrival, "south door"),
                    "the ordinary pause menu points to Bellwether's south tavern door on the road");
            }
            progressed = progressed &&
                std::all_of(afterRoadDirections, kBellwetherTavernRoute.end(), walkTo) &&
                walkTo({target.x, target.z}) && waitFor([&] { return state.tavernConversationAvailable; }, hearthWaitTicks);
            if (gBellwetherAnkleRestoreValidation)
                progressed = check(progressed && ankleReloaded && !ankleReplayed,
                    "the resumed walk reaches the hearth without replaying the injury, captions or foley");
            progressed = check(progressed && atHearth() && frame(),
                "fixed-step movement from the loaded lookout reaches the staffed hearth through the south door");
            if (!progressed)
                std::cerr << "Bellwether arrival steps=" << ticks << " player=" << state.playerX << ',' << state.playerZ
                          << " kit=" << state.chapterCompanion.x << ',' << state.chapterCompanion.z
                          << " kitStation=" << state.chapterCompanion.bellwetherStation
                          << " kitArrival=" << static_cast<unsigned>(state.chapterCompanion.bellwetherArrival)
                          << " hearthLines=" << state.chapterDialogue.hearthLinesResolved
                          << " currentLine=" << static_cast<unsigned>(state.chapterDialogue.line)
                          << " talk=" << state.tavernConversationAvailable << '\n';
            if (progressed) {
                const auto& lifetime = ctx.rtScene.BellwetherFurnitureLifetimeEvidence();
                const auto tier = ctx.rtScene.DebugRecordedBellwetherFurnitureDetail();
                const bool active = ctx.rtScene.PackagedBellwetherFurnitureActive();
                progressed = check(active && lifetime.retirements > 0 && lifetime.restorations > 0 &&
                    tier == horde::scene::BellwetherFurnitureDetail::Near,
                    "normal campaign approach retires and restores packaged near furniture before the hearth");
                if (!progressed)
                    std::cerr << "Bellwether furniture active=" << active << " retirements=" << lifetime.retirements
                              << " restorations=" << lifetime.restorations << " deferred=" << lifetime.deferredNearFrames
                              << " failures=" << lifetime.preparationFailures
                              << " tier=" << (tier ? static_cast<int>(*tier) : -1) << '\n';
            }
            if (progressed) {
                progressed = check(readDirections(campaign::PauseObjective::BellwetherHearth, "Choose Talk"),
                    "the ordinary pause menu points to Talk before the first hearth exchange");
            }
            if (progressed) {
                input.commands.interact = state.lastConsumedInteractSequence + 1;
                const auto conversationComplete = [&] {
                    return state.chapterDialogue.tavernConversationLinesResolved == kTavernConversation.size() &&
                        state.chapterDialogue.line == Line::None;
                };
                const auto savedTavern = [&] {
                    const auto& boundary = ctx.simulation.LatestCampaignBoundary();
                    return boundary && boundary->id == campaign::CheckpointId::BellwetherTavern &&
                        boundary->dialogue.tavernConversationLinesResolved == kTavernConversation.size();
                };
                progressed = check(step() && waitFor(conversationComplete, 1800) &&
                    exchangeLines == kTavernConversation.size() && savedTavern(),
                    "one Talk input earns a tavern boundary carrying the complete three-line exchange");
            }
            campaign::storage::StoredCheckpoint stored;
            if (progressed) {
                progressed = check(until([&] {
                    return campaign::storage::Read(gCampaignValidationDirectory, "slot-1-auto.hrd", stored).status ==
                        campaign::storage::Status::Read && stored.checkpoint.id == campaign::CheckpointId::BellwetherTavern &&
                        stored.checkpoint.dialogue.tavernConversationLinesResolved == kTavernConversation.size() &&
                        ctx.campaign->State().outcome == campaign::PlayOutcome::Succeeded;
                }, false) && stored.checkpoint.vitality == gHiddenLookout.source.checkpoint.vitality &&
                    !gHiddenCampaignFaults.rejectedWrites,
                    "ordinary autosave persists the earned tavern and conserves loaded vitality");
            }
            if (progressed) {
                const auto deferredBeforeReload = ctx.rtScene.BellwetherFurnitureLifetimeEvidence().deferredNearFrames;
                const bool reloaded = reloadCheckpoint();
                const bool nearPresentedOnLoad = reloaded &&
                    ctx.rtScene.DebugRecordedBellwetherFurnitureDetail()==horde::scene::BellwetherFurnitureDetail::Near &&
                    ctx.rtScene.BellwetherFurnitureLifetimeEvidence().deferredNearFrames==deferredBeforeReload;
                progressed = check(reloaded && step() && atHearth() &&
                    state.chapterDialogue.line == Line::None && !state.tavernConversationAvailable &&
                    state.tavernDirectionsAvailable && exchangeLines == kTavernConversation.size() &&
                    frame(), "native Load presents and resumes the earned tavern with completed Talk");
                if (progressed) {
                    check(nearPresentedOnLoad && ctx.rtScene.PackagedBellwetherFurnitureActive() &&
                        ctx.rtScene.BellwetherNearOwnerBytes()>0 &&
                        ctx.rtScene.DebugRecordedBellwetherFurnitureDetail()==horde::scene::BellwetherFurnitureDetail::Near,
                        "loading a tavern save presents ready near furniture without a coarse interim frame");
                    campaign::storage::StoredCheckpoint resumed;
                    check(campaign::storage::Read(gCampaignValidationDirectory, "slot-1-auto.hrd", resumed).status ==
                        campaign::storage::Status::Read && resumed.encoded == stored.encoded,
                        "resuming the tavern preserves the earned autosave bytes");
                }
            }
            if (progressed && gBellwetherPreparationsValidation) {
                progressed = check(state.shorePracticeServiceAvailable && state.millServiceAvailable &&
                    readDirections(campaign::PauseObjective::BellwetherShrine, "For a breathing rig, follow the MILL signs east"),
                    "Bellwether Directions offers both eastern preparations before learning the church lead");
            }
            if (progressed && gBellwetherCombinedCampaignActors && !gBellwetherHearthRestValidation &&
                (!gBellwetherExpeditionValidation || gBellwetherPreparationsValidation)) {
                using namespace horde::gameplay::traversal;
                using namespace horde::gameplay::bellwether;
                using Scene = horde::vulkan::raytracing::PresentableTinyRtScene;
                const auto junction = kBellwetherTavernRoute.begin() + kBellwetherMillJunction;
                auto sceneEpoch = ctx.rtFrameEvidence.PublishedStateByValue().sceneEpoch;
                auto geometryGeneration = ctx.rtScene.StaticGeometryGeneration();
                const auto checkRoster = [&](bool posesReady, const std::array<std::uint32_t, 2>& expected) {
                    const auto active = ctx.rtScene.ChapterResidentSourceIds();
                    if (posesReady && active == expected) return true;
                    std::cerr << "Combined actors at " << state.playerX << ',' << state.playerZ
                              << " ready=" << posesReady << " active=" << active[0] << ',' << active[1]
                              << " expected=" << expected[0] << ',' << expected[1]
                              << " pending=" << ctx.rtScene.PrefetchedChapterResidentId(0) << ','
                              << ctx.rtScene.PrefetchedChapterResidentId(1) << '\n';
                    return false;
                };
                const auto town = [&] {
                    return checkRoster(state.tavernKeeper && state.recordKeeper &&
                        !state.artificer && !state.salvager && !state.relicDealer,
                        {kTavernKeeperActorId, kRecordKeeperActorId});
                };
                const auto east = [&] {
                    return checkRoster(state.artificer && state.salvager &&
                        !state.tavernKeeper && !state.recordKeeper && !state.relicDealer,
                        {kArtificerActorId, kSalvagerActorId});
                };
                const auto fork = [&] {
                    return checkRoster(state.tavernKeeper && !state.recordKeeper && !state.artificer &&
                        (gBellwetherCombinedRelicDealer ? state.relicDealer && !state.salvager :
                            state.salvager && !state.relicDealer),
                        {kTavernKeeperActorId,
                            gBellwetherCombinedRelicDealer ? kRelicDealerActorId : kSalvagerActorId});
                };
                const auto capture = [&](const char* name, bool requireResidents = true) {
                    if (!CompleteRtEvidenceAfterDeviceIdle(ctx, vkDeviceWaitIdle(ctx.device))) return false;
                    const auto publication = ctx.rtFrameEvidence.PublishedStateByValue();
                    const auto& completed = publication.completedEvidence;
                    const auto& identity = completed.identity.submitted;
                    if (!publication.hasCompletedEvidence || identity.frame.sceneEpoch != sceneEpoch ||
                        identity.frame.simulationTick != state.tickIndex ||
                        completed.presentation.outcome != RtPresentationOutcome::Presented ||
                        completed.presentation.lastSuccessfulPresentSubmissionSerial != identity.submissionSerial ||
                        ctx.rtScene.StaticGeometryGeneration() != geometryGeneration ||
                        (requireResidents && (ctx.rtScene.SubmittedInstanceMask(Scene::kFirstResidentMetadataIndex) != 1u ||
                         ctx.rtScene.SubmittedInstanceMask(Scene::kFirstResidentMetadataIndex + 1u) != 1u))) {
                        std::cerr << "Combined resident evidence mismatch in " << name
                                  << ": completed=" << publication.hasCompletedEvidence
                                  << " epoch=" << identity.frame.sceneEpoch << '/' << sceneEpoch
                                  << " tick=" << identity.frame.simulationTick << '/' << state.tickIndex
                                  << " present=" << static_cast<int>(completed.presentation.outcome)
                                  << " serial=" << completed.presentation.lastSuccessfulPresentSubmissionSerial
                                  << '/' << identity.submissionSerial
                                  << " geometry=" << ctx.rtScene.StaticGeometryGeneration() << '/' << geometryGeneration
                                  << " masks=" << static_cast<int>(ctx.rtScene.SubmittedInstanceMask(Scene::kFirstResidentMetadataIndex))
                                  << ',' << static_cast<int>(ctx.rtScene.SubmittedInstanceMask(Scene::kFirstResidentMetadataIndex + 1u))
                                  << '\n';
                        return false;
                    }
                    Scene::StorageImageCapture image;
                    std::string error;
                    const bool saved = ctx.rtScene.CaptureStorageImage(image, error) &&
                        WriteRgbaPng(gCampaignValidationDirectory / name, image, error);
                    if (!saved) std::cerr << "Combined resident capture failed for " << name << ": " << error << '\n';
                    return saved;
                };
                const auto face = [&](float x, float z) {
                    input.yawRadians = std::atan2(x - state.playerX, state.playerZ - z);
                    return step() && frame();
                };
                const auto interact = [&] {
                    input.commands.interact = state.lastConsumedInteractSequence + 1;
                    return step();
                };
                const auto& landing = kBellwetherRiverLanding;
                const auto& mill = kBellwetherMillApproach;
                const auto approachShallows = [&] {
                    return std::all_of(landing.begin(), landing.end() - 2, walkTo) &&
                        walkTo(kBellwetherRiverDryMark) && !state.waterContact.wetGround &&
                        walkTo(landing[landing.size() - 2]) && walkTo(landing.back()) &&
                        state.waterContact.wetGround;
                };
                const auto backFromShore = [&] {
                    return std::all_of(landing.rbegin() + 2, landing.rend(), walkTo) &&
                        walkTo(kBellwetherSalvagerMark);
                };
                const auto reachWorkbench = [&] {
                    return walkTo(mill[1]) && std::all_of(mill.begin() + 2, mill.end(), walkTo) &&
                        walkTo(kBellwetherMillWorkMark);
                };
                const auto leaveWorkbench = [&] {
                    return walkTo(mill.back()) && std::all_of(mill.rbegin() + 1, mill.rend() - 1, walkTo);
                };
                const auto useMill = [&](MillAction action, const std::array<float, 3>& mark) {
                    return face(mark[0], mark[2]) && state.millAction == action && interact();
                };
                if (gBellwetherCombinedRelicDealer) {
                    // Keep the renderer serviced throughout the shortened test
                    // clock so that worker imports see more than the waypoints.
                    presentationInterval = 10;
                    const auto square = kBellwetherTavernRoute.begin() + kBellwetherWestJunction;
                    const auto westBranch = kBellwetherWestApproach.front();
                    const auto rampToe = kBellwetherTavernRoute.begin() + kBellwetherSouthDoorStation;
                    const BellwetherStation coveredHandoff{rampToe->x, rampToe->z + 1.5f};
                    using horde::gameplay::interactions::HeldLightPose;
                    const auto setInspectionLight = [&](HeldLightPose pose) {
                        if (state.interaction.heldLightPose == pose) return true;
                        input.commands.toggleHeldLightPose = state.lastConsumedToggleHeldLightPoseSequence + 1;
                        return step() && waitFor([&] { return state.interaction.heldLightPose == pose; }, 125);
                    };
                    const auto presentWell = [&] {
                        const BellwetherStation view{horde::gameplay::bellwether::kBellwetherWellView[0],
                            horde::gameplay::bellwether::kBellwetherWellView[1]};
                        if (!walkTo(view)) return false;
                        const float previousPitch = input.pitchRadians;
                        input.pitchRadians = -.025f;
                        // Keep the well clear of the held sword in the capture.
                        const bool presented = face(horde::gameplay::bellwether::kBellwetherWellCentre[0] + .32f,
                            horde::gameplay::bellwether::kBellwetherWellCentre[2]) &&
                            capture("windows-combined-well.png");
                        input.pitchRadians = previousPitch;
                        return presented && walkTo(*square);
                    };
                    const auto presentSmith = [&] {
                        const auto station = [](const auto& point) { return BellwetherStation{point[0], point[1]}; };
                        const auto walkSmith = [&](const auto& point) { return walkTo(station(point)); };
                        const auto& approach = kBellwetherSmithApproachXZ;
                        const auto& door = kBellwetherSmithDoorApproachXZ;
                        const auto viewEnd = std::prev(approach.end(), 2);
                        if (!std::all_of(approach.begin(), viewEnd, walkSmith))
                            return false;
                        const auto boundary = ctx.simulation.CampaignBoundaryRevision();
                        const float previousPitch = input.pitchRadians;
                        input.pitchRadians = -.08f;
                        const auto& anvil = kBellwetherSmithAnvilXZ;
                        const auto wick = horde::gameplay::effects::kBellwetherSmithLampEmitterId;
                        const auto wickPresented = [&] {
                            const auto& ids = ctx.rtScene.UploadedFireEmitters().selectedStableIds;
                            return state.fireEmitterCount == 4 && state.fireEmitters[3].stableId == wick &&
                                ctx.rtScene.HasUploadedFireEmitters() && std::find(ids.begin(), ids.end(), wick) != ids.end();
                        };
                        const bool presented = face(anvil[0], anvil[1]) && wickPresented() &&
                            capture("windows-combined-smith-approach.png", false) &&
                            std::all_of(viewEnd, approach.end(), walkSmith) && face(97.15f, 151.31f) &&
                            wickPresented() && capture("windows-combined-smith-yard.png", false);
                        input.pitchRadians = previousPitch;
                        return presented &&
                            std::all_of(door.begin(), door.end(), walkSmith) &&
                            walkSmith(door.front()) && walkSmith(approach.back()) &&
                            std::all_of(std::next(approach.rbegin()), approach.rend(), walkSmith) &&
                            walkTo(*square) && ctx.simulation.CampaignBoundaryRevision() == boundary;
                    };
                    const auto presentGarden = [&] {
                        const auto& approach = kBellwetherGardenApproach;
                        if (!walkTo(kBellwetherWestApproach[1]) || !std::all_of(approach.begin(), approach.end(), walkTo) ||
                            !walkTo(kBellwetherGardenAisle) || !walkTo(approach.back()) || !walkTo(kBellwetherGardenView))
                            return false;
                        const auto previousPose = state.interaction.heldLightPose;
                        const float previousPitch = input.pitchRadians;
                        input.pitchRadians = -.34f;
                        const bool presented = face(kBellwetherGardenCentre[0], kBellwetherGardenCentre[1]) &&
                            setInspectionLight(HeldLightPose::High) && frame() && capture("windows-combined-garden.png", false);
                        input.pitchRadians = previousPitch;
                        const auto& listen = kBellwetherGrateListenPoint;
                        const auto journalBoundary = ctx.simulation.CampaignBoundaryRevision();
                        if (!check(presented && walkTo(approach.back()) && walkTo(kBellwetherGardenGrateApproach) &&
                            face(listen[0],listen[2]+2.f) && !state.gardenGrateTalkAvailable &&
                            state.chapterDialogue.line != Line::GardenGrateTalk,
                            "the dry garden route approaches the grate without starting speech when facing away"))
                            return false;
                        input.pitchRadians = -.41f;
                        if (!check(face(listen[0],listen[2]) && state.playerGrounded &&
                            state.gardenGrateTalkAvailable && state.HasChapterInteractPrompt() && frame() &&
                            ctx.tavernInteractionPromptPresented,"facing the grate shows the shared talk action")) {
                            std::cerr << "Garden grate mark " << state.playerX << ',' << state.playerZ
                                      << " offered=" << state.gardenGrateTalkAvailable
                                      << " hud=" << ctx.tavernInteractionPromptPresented << '\n';
                            return false;
                        }
                        std::array<char, 180> caption{};
                        const auto& fire = ctx.rtScene.UploadedFireEmitters().selectedStableIds;
                        const auto grateLamp = horde::gameplay::effects::kBellwetherGrateLampEmitterId;
                        const bool talked = check(state.fireEmitterCount == 4 && ctx.rtScene.HasUploadedFireEmitters() &&
                            std::find(fire.begin(), fire.end(), grateLamp) != fire.end() && interact() && frame() && state.chapterDialogue.line == Line::GardenGrateTalk &&
                            std::abs(state.chapterDialogue.source.y - kBellwetherGrateMouth[1]) < .01f &&
                            GetWindowTextA(GetDlgItem(ctx.windowHandle,kChapterSubtitleId),caption.data(),
                                static_cast<int>(caption.size())) > 0 &&
                            std::string_view(caption.data()).starts_with("Man below the grate:") &&
                            capture("windows-combined-garden-grate.png", false),"Interact presents the grate and caption from below");
                        const auto& stock=kBellwetherStockApproach;
                        input.pitchRadians=-.28f;
                        const bool visitedStock=check(talked && walkTo(approach.back()) &&
                            state.chapterDialogue.line!=Line::GardenGrateTalk &&
                            std::all_of(stock.begin(),stock.end(),walkTo) && state.playerGrounded &&
                            face(kBellwetherStockPigXZ[0],kBellwetherStockPigXZ[1]) &&
                            capture("windows-combined-stock-pig.png",false) &&
                            walkTo(kBellwetherStockRoostWitness) && state.playerGrounded && !state.waterContact.wetGround &&
                            face(kBellwetherStockRoostXZ[0],kBellwetherStockRoostXZ[1]) &&
                            capture("windows-combined-stock-roost.png",false) && walkTo(stock.back()) &&
                            std::all_of(std::next(stock.rbegin()),stock.rend(),walkTo) && walkTo(approach.back()) &&
                            ctx.simulation.CampaignBoundaryRevision()==journalBoundary,
                            "the fenced stock and roost are reachable from the garden on the dry path");
                        input.pitchRadians = previousPitch;
                        const bool restored = setInspectionLight(previousPose);
                        return visitedStock && restored &&
                            std::all_of(std::next(approach.rbegin()), approach.rend(), walkTo) && walkTo(kBellwetherWestApproach[1]);
                    };
                    const auto presentPrivy = [&] {
                        const auto& path = kBellwetherServiceYardApproach;
                        const auto& mouth = kBellwetherPrivyMouth;
                        using Line = horde::gameplay::dialogue::Line;
                        if (!walkTo(westBranch) || !std::all_of(path.begin(), std::prev(path.end()), walkTo) ||
                            state.chapterDialogue.line == Line::PrivyRagRequest || !walkTo(path.back()) ||
                            !walkTo(kBellwetherServiceYardView)) return false;
                        const auto previousPose = state.interaction.heldLightPose;
                        const float previousPitch = input.pitchRadians;
                        input.pitchRadians = -.10f;
                        std::array<char, 96> caption{};
                        const bool presented = face(mouth[0], mouth[2] + .18f) &&
                            setInspectionLight(HeldLightPose::High) && frame() &&
                            state.chapterDialogue.line == Line::PrivyRagRequest &&
                            std::abs(state.chapterDialogue.source.x - mouth[0]) < .01f &&
                            GetWindowTextA(GetDlgItem(ctx.windowHandle,kChapterSubtitleId),caption.data(),
                                static_cast<int>(caption.size())) > 0 &&
                            std::string_view(caption.data()) == "Privy occupant: Can someone PLEASE get me some rag?!" &&
                            capture("windows-combined-service-yard.png", false);
                        input.pitchRadians = previousPitch;
                        const bool restored = setInspectionLight(previousPose);
                        return presented && restored &&
                            std::all_of(std::next(path.rbegin()), path.rend(), walkTo) &&
                            state.chapterDialogue.line != Line::PrivyRagRequest && walkTo(westBranch) && walkTo(*square);
                    };
                    const auto leaveTavern = [&] {
                        if (!std::all_of(kBellwetherTavernRoute.rbegin(), std::make_reverse_iterator(rampToe), walkTo) ||
                            !walkTo(coveredHandoff)) return false;
                        const auto handoffDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(300);
                        while (!state.relicDealer && std::chrono::steady_clock::now() < handoffDeadline)
                            if (!frame()) return false;
                        return fork() &&
                            std::all_of(std::make_reverse_iterator(rampToe), std::make_reverse_iterator(square), walkTo);
                    };
                    const auto boundary = ctx.simulation.CampaignBoundaryRevision();
                    const auto purse = state.treasurePurse;
                    const auto stallWickSelected = [&] {
                        const auto& ids = ctx.rtScene.UploadedFireEmitters().selectedStableIds;
                        return ctx.rtScene.HasUploadedFireEmitters() &&
                            std::find(ids.begin(), ids.end(), horde::gameplay::effects::kBellwetherRelicStallLampEmitterId)
                                != ids.end();
                    };
                    progressed = check(town() && leaveTavern() &&
                        fork() && walkTo(westBranch) && fork() && walkTo(*square) &&
                        std::all_of(std::next(square), kBellwetherTavernRoute.end(), walkTo) &&
                        walkTo({target.x, target.z}) && atHearth() && town() &&
                        leaveTavern() && fork() &&
                        walkTo(westBranch) && fork() && face(kBellwetherRelicDealerStand.x, kBellwetherRelicDealerStand.z) &&
                        capture("windows-combined-west-stall-entry.png") &&
                        std::all_of(std::next(kBellwetherWestApproach.begin()), kBellwetherWestApproach.end(), walkTo) &&
                        face(kBellwetherRelicDealerStand.x, kBellwetherRelicDealerStand.z) &&
                        state.relicDealerTradeAvailable && interact() &&
                        state.chapterDialogue.line == Line::RelicDealerOrdinarySale && frame() && fork() &&
                        stallWickSelected() &&
                        ctx.rtScene.SubmittedInstanceMask(Scene::kKeeperMetadataIndex) == 1u &&
                        capture("windows-combined-west-trade.png") && presentGarden() &&
                        std::all_of(std::next(kBellwetherWestApproach.rbegin()), kBellwetherWestApproach.rend(), walkTo) &&
                        walkTo(*square) && fork() && presentWell() && fork() &&
                        check(presentSmith() && fork(), "the southeast smith yard presents its wick and the west door approach is walkable from the square and back") &&
                        check(presentPrivy() && fork(), "the west service yard presents the local closed-privy caption and rejoins the square") &&
                        std::all_of(std::next(square), kBellwetherTavernRoute.end(), walkTo) &&
                        walkTo({target.x, target.z}) && atHearth() && town() &&
                        !state.relicDealerTradeAvailable && state.chapterDialogue.line != Line::RelicDealerOrdinarySale &&
                        ctx.simulation.CampaignBoundaryRevision() == boundary && state.treasurePurse == purse,
                        "the west lane survives reversal, presents dealer trade and restores the same tavern visit");
                    using campaign::OrdinarySilverAction;
                    using campaign::OrdinarySilverProgress;
                    const auto savedSilver = [&](OrdinarySilverProgress progress, std::uint32_t coins, bool fitting) {
                        return until([&] {
                            campaign::storage::StoredCheckpoint updated;
                            return campaign::storage::Read(gCampaignValidationDirectory, "slot-1-auto.hrd", updated).status ==
                                campaign::storage::Status::Read && updated.checkpoint.schema == campaign::kOrdinarySilverSchema &&
                                updated.checkpoint.position.x == target.x && updated.checkpoint.position.z == target.z &&
                                updated.checkpoint.treasurePurse.ordinarySilver == progress &&
                                updated.checkpoint.treasurePurse.coins == coins &&
                                updated.checkpoint.treasurePurse.lanternFittingPurchased == fitting &&
                                !updated.checkpoint.treasurePurse.registerRewardClaimed;
                        }, false);
                    };
                    const auto reloadWithTownResidents = [&] {
                        if (!reloadCheckpoint() || !atHearth()) return false;
                        const auto readyBy=std::chrono::steady_clock::now()+std::chrono::milliseconds(750);
                        while (!state.recordKeeper && std::chrono::steady_clock::now() < readyBy)
                            if (!frame()) return false;
                        if (!town()) return false;
                        // A successful native Load begins a new presentation
                        // epoch. Subsequent walks must retain that new scene.
                        sceneEpoch=ctx.rtFrameEvidence.PublishedStateByValue().sceneEpoch;
                        geometryGeneration=ctx.rtScene.StaticGeometryGeneration();
                        return true;
                    };
                    if (progressed && verifiesSilver) {
                        const auto church = kBellwetherTavernRoute.begin() + kBellwetherChurchJunction;
                        const auto& path = kBellwetherGraveyardApproach;
                        const auto& arrival = kBellwetherNorthApproach;
                        const auto& marker = kBellwetherGraveyardMarkers.front();
                        const BellwetherStation plotView{94.18f,56.43f};
                        const auto boundary = ctx.simulation.CampaignBoundaryRevision();
                        const float previousPitch = input.pitchRadians;
                        const auto inspect = [&](float x, float z, const char* image) {
                            input.pitchRadians = -.23f;
                            return face(x,z) && setInspectionLight(HeldLightPose::High) &&
                                state.playerGrounded && frame() && capture(image,false);
                        };
                        progressed = check(
                            std::all_of(kBellwetherTavernRoute.rbegin(),std::make_reverse_iterator(church),walkTo) &&
                            std::all_of(arrival.begin(),arrival.end(),walkTo) &&
                            walkTo(kBellwetherChurchApproach.front()) && walkTo(kBellwetherRecordKeeperApproach) &&
                            std::all_of(path.begin(),path.begin()+6,walkTo) &&
                            inspect(path[6].x,path[6].z,"windows-combined-graveyard-gateway.png") &&
                            std::all_of(path.begin()+6,path.end(),walkTo) &&
                            walkTo(plotView) &&
                            inspect(marker[0],marker[1],"windows-combined-graveyard-plots.png") &&
                            walkTo(path.back()) &&
                            std::all_of(std::next(path.rbegin()),path.rend(),walkTo) &&
                            walkTo(kBellwetherRecordKeeperApproach) && walkTo(kBellwetherChurchApproach.front()) &&
                            std::all_of(arrival.rbegin(),arrival.rend(),walkTo) &&
                            ctx.simulation.CampaignBoundaryRevision()==boundary,
                            "the active graveyard is reachable from the church and returns to the fork without creating a save");
                        input.pitchRadians = previousPitch;
                    }
                    if (progressed && verifiesSilver) {
                        // The graveyard walk has already brought the player back to this fork.
                        progressed = check(
                            walkTo({kRoadSilverCacheApproach[0], kRoadSilverCacheApproach[1]}) &&
                            face(kRoadSilverCacheReach[0], kRoadSilverCacheReach[2]) &&
                            state.ordinarySilverAction == OrdinarySilverAction::Take &&
                            capture("windows-combined-road-silver.png") && interact() &&
                            state.treasurePurse.ordinarySilver == OrdinarySilverProgress::Carried &&
                            state.ordinarySilverAction == OrdinarySilverAction::None &&
                            savedSilver(OrdinarySilverProgress::Carried, 0, false) &&
                            reloadWithTownResidents() &&
                            state.treasurePurse.ordinarySilver == OrdinarySilverProgress::Carried,
                            "the ordinary road silver is collected once and reloads at the earned tavern boundary");
                    }
                    if (progressed && verifiesSilver) {
                        constexpr auto price = campaign::TreasurePurse::kOrdinarySilverSaleCoins;
                        const std::string sellText="SELL ORDINARY SILVER (+"+std::to_string(price)+" COINS) | LEFT-CLICK / A";
                        std::array<char, 96> quote{};
                        progressed = check(leaveTavern() && fork() &&
                            std::all_of(kBellwetherWestApproach.begin(), kBellwetherWestApproach.end(), walkTo) &&
                            face(kBellwetherRelicDealerStand.x, kBellwetherRelicDealerStand.z) &&
                            state.ordinarySilverAction == OrdinarySilverAction::Sell && state.treasurePurse.coins == 0 &&
                            GetWindowTextA(GetDlgItem(ctx.windowHandle,kChestPromptControlId),quote.data(),
                                static_cast<int>(quote.size())) > 0 && std::string_view(quote.data()) == sellText &&
                            interact() && state.ordinarySilverAction == OrdinarySilverAction::Sold &&
                            !state.HasChapterInteractPrompt() && frame() && !ctx.tavernInteractionPromptPresented &&
                            interact() && state.treasurePurse.coins == price &&
                            savedSilver(OrdinarySilverProgress::Sold, price, false) &&
                            reloadWithTownResidents() && state.treasurePurse.coins == price,
                            "the quoted sale credits the purse exactly once and survives the native checkpoint reload");
                    }
                    if (progressed && verifiesSilver) {
                        progressed = check(walkTo(kBellwetherLanternFittingApproach) && state.tavernKeeper &&
                            face(state.tavernKeeper->foot[0], state.tavernKeeper->foot[2]) &&
                            state.tavernLanternFittingAvailable && interact() && state.treasurePurse.lanternFittingPurchased &&
                            state.treasurePurse.coins == 0 && savedSilver(OrdinarySilverProgress::Sold, 0, true) &&
                            reloadWithTownResidents() && state.treasurePurse == campaign::TreasurePurse{0,false,true,OrdinarySilverProgress::Sold} &&
                            campaign::storage::Read(gCampaignValidationDirectory,"slot-1-auto.hrd",stored).status == campaign::storage::Status::Read &&
                            stored.checkpoint.treasurePurse == state.treasurePurse,
                            "the silver proceeds pay for the existing hinge fitting without taking the register reward");
                    }
                    if (progressed && verifiesSilver) {
                        const auto visitYard = [&] {
                            return leaveTavern() && walkTo(westBranch) &&
                                std::all_of(kBellwetherServiceYardApproach.begin(), kBellwetherServiceYardApproach.end(), walkTo);
                        };
                        const auto inspectRag = [&](const std::array<float,3>& mark) {
                            input.pitchRadians = -.32f;
                            return face(mark[0],mark[2]) && setInspectionLight(HeldLightPose::High) && frame();
                        };
                        const auto ragVisible = [&](bool visible) {
                            return ctx.rtScene.SubmittedInstanceMask(Scene::kPrivyRagMetadataIndex) == (visible ? 1u : 0u);
                        };
                        const auto controlIs = [&](int id, std::string_view expected) {
                            std::array<char,96> prompt{};
                            return GetWindowTextA(GetDlgItem(ctx.windowHandle,id),prompt.data(),
                                static_cast<int>(prompt.size())) > 0 && std::string_view(prompt.data()) == expected;
                        };
                        const auto storedDelivery = [&](bool delivered) {
                            campaign::storage::StoredCheckpoint stored;
                            return campaign::storage::Read(gCampaignValidationDirectory,"slot-1-auto.hrd",stored).status ==
                                campaign::storage::Status::Read && stored.checkpoint.privyRagDelivered == delivered &&
                                stored.checkpoint.treasurePurse == campaign::TreasurePurse{0,false,true,OrdinarySilverProgress::Sold} &&
                                (!delivered || stored.checkpoint.schema==campaign::kPrivyRagSchema);
                        };
                        const campaign::TreasurePurse westernPurse{0,false,true,OrdinarySilverProgress::Sold};
                        const auto initialPurse = state.treasurePurse;
                        const auto initialFlask = state.healingFlask.resources;
                        const auto initialRevision = ctx.simulation.CampaignBoundaryRevision();
                        const auto takeRag = [&] {
                            return state.privyRagAction == PrivyRagAction::Take &&
                                controlIs(kChestPromptControlId,"LEFT-CLICK / A TO TAKE A CLEAN RAG") && interact() && frame() &&
                                state.privyRag == PrivyRagState::Carried && ragVisible(false);
                        };
                        const float previousPitch = input.pitchRadians;
                        const bool ragProgress = visitYard() && inspectRag(kPrivyRagBasket) &&
                            state.privyRag == PrivyRagState::Available && ragVisible(true) &&
                            capture("windows-combined-rag-basket.png",false) && takeRag() &&
                            ctx.simulation.CampaignBoundaryRevision() == initialRevision && storedDelivery(false) &&
                            reloadWithTownResidents() && state.privyRag == PrivyRagState::Available && ragVisible(true) &&
                            state.treasurePurse == westernPurse &&
                            visitYard() && inspectRag(kPrivyRagBasket) && takeRag() && inspectRag(kPrivyRagShelf) &&
                            state.privyRagAction == PrivyRagAction::Pass &&
                            controlIs(kChestPromptControlId,"LEFT-CLICK / A TO PASS THE RAG");
                        if (!ragProgress) std::cerr << "Service-yard rag before delivery: position=" << state.playerX
                            << ',' << state.playerZ << " rag=" << static_cast<int>(state.privyRag)
                            << " action=" << static_cast<int>(state.privyRagAction)
                            << " caption=" << static_cast<int>(state.chapterDialogue.line) << '\n';
                        const auto deliveryRevision = ctx.simulation.CampaignBoundaryRevision();
                        const bool delivered = ragProgress && interact() && frame() &&
                            state.privyRag == PrivyRagState::Delivered && state.privyRagAction == PrivyRagAction::None &&
                            state.chapterDialogue.line == Line::PrivyRagThanks && ragVisible(true) &&
                            controlIs(kChapterSubtitleId,"Privy occupant: Thank you.") &&
                            ctx.simulation.CampaignBoundaryRevision() == deliveryRevision + 1;
                        const bool deliverySaved = delivered && until([&] { return storedDelivery(true); },false);
                        if (delivered && !deliverySaved) std::cerr << "Service-yard rag delivery never reached the autosave.\n";
                        progressed = check(deliverySaved &&
                            capture("windows-combined-rag-shelf.png",false) && interact() &&
                            ctx.simulation.CampaignBoundaryRevision() == deliveryRevision + 1 &&
                            state.treasurePurse == initialPurse && state.healingFlask.resources == initialFlask &&
                            reloadWithTownResidents() && state.privyRag == PrivyRagState::Delivered && ragVisible(true) &&
                            state.treasurePurse == westernPurse && storedDelivery(true) &&
                            campaign::storage::Read(gCampaignValidationDirectory,"slot-1-auto.hrd",stored).status == campaign::storage::Status::Read &&
                            stored.checkpoint.treasurePurse == westernPurse && stored.checkpoint.privyRagDelivered,
                            "the clean rag returns after an interrupted pickup and persists on the shelf after one delivery");
                        input.pitchRadians = previousPitch;
                    }
                }
                if (progressed) progressed = check(town() &&
                    std::all_of(kBellwetherTavernRoute.rbegin(), std::make_reverse_iterator(junction), walkTo) &&
                    fork() && walkTo(kBellwetherMillApproach[1]) && east() &&
                    state.chapterCompanion.bellwetherArrival == BellwetherArrivalStage::Seated,
                    "the authored south route loads both eastern residents while Kit stays at the hearth");
                if (progressed && gBellwetherPreparationsValidation) {
                    constexpr BellwetherStation approach{
                        std::lerp(kBellwetherMillApproach[1].x, kBellwetherSalvagerMark.x, .60f),
                        std::lerp(kBellwetherMillApproach[1].z, kBellwetherSalvagerMark.z, .60f)};
                    const auto lampSelected = [&] {
                        const auto& ids = ctx.rtScene.UploadedFireEmitters().selectedStableIds;
                        return ctx.rtScene.HasUploadedFireEmitters() &&
                            std::find(ids.begin(), ids.end(), horde::gameplay::effects::kBellwetherRiverForkLampEmitterId)
                                != ids.end();
                    };
                    progressed = check(walkTo(approach) &&
                        face((kRiverForkLampFlame[0] + kBellwetherSalvagerStand.x) * .5f,
                            (kRiverForkLampFlame[2] + kBellwetherSalvagerStand.z) * .5f) &&
                        east() && lampSelected() && capture("windows-combined-fork-lamp.png"),
                        "the fork lamp reaches the normal exterior RT fire budget and presents beside the salvager");
                }
                if (progressed) {
                    const bool reached = walkTo(kBellwetherSalvagerMark) && east() &&
                        face(kBellwetherSalvagerStand.x, kBellwetherSalvagerStand.z);
                    const auto offer = state.salvagerAction;
                    const bool started = reached && offer == SalvagerAction::StartPractice &&
                        interact() && state.shorePracticeStage == ShorePracticeStage::FindDryMark &&
                        state.chapterDialogue.line == Line::SalvagerShoreTask;
                    if (!started) std::cerr << "Combined shore: player=" << state.playerX << ',' << state.playerZ
                        << " grounded=" << state.playerGrounded << " offer=" << static_cast<int>(offer)
                        << " stage=" << static_cast<int>(state.shorePracticeStage)
                        << " line=" << static_cast<int>(state.chapterDialogue.line) << '\n';
                    progressed = check(started && frame() && capture("windows-combined-shore.png"),
                        "the salvager starts shore practice with both eastern actors in the same presented scene");
                }
                if (progressed && gBellwetherPreparationsValidation) {
                    const bool reachedWetRail = approachShallows();
                    const auto breathTicks = state.shorePracticeBreathTicksRemaining;
                    std::array<char, 96> breathCue{};
                    const std::string expectedCue = "RETURN TO DRY STONE | BREATH " +
                        std::to_string(ShorePracticeSecondsRemaining(breathTicks)) + "s";
                    const bool presentedWetRail = reachedWetRail && breathTicks > 0 &&
                        breathTicks <= kShorePracticeBreathTicks && frame() &&
                        GetWindowTextA(GetDlgItem(ctx.windowHandle,kChestPromptControlId),
                            breathCue.data(),static_cast<int>(breathCue.size())) > 0 &&
                        std::string_view(breathCue.data()) == expectedCue &&
                        capture("windows-combined-shore-breath.png");
                    progressed = check(presentedWetRail && state.shorePracticeStage == ShorePracticeStage::Retreat &&
                        readDirections(campaign::PauseObjective::BellwetherShrine, "Return from the shallows") &&
                        state.shorePracticeBreathTicksRemaining + 1 == breathTicks &&
                        !state.salvagerShorePracticeCompleted && walkTo(kBellwetherRiverDryMark) &&
                        state.shorePracticeStage == ShorePracticeStage::ReportBack &&
                        state.shorePracticeBreathTicksRemaining == 0 && backFromShore() &&
                        face(kBellwetherSalvagerStand.x, kBellwetherSalvagerStand.z) &&
                        state.salvagerAction == SalvagerAction::ReportPractice && interact() && frame() &&
                        state.salvagerShorePracticeCompleted && !state.waysideArchive.airWardLearned &&
                        state.millPreparation == MillPreparation::Unexamined &&
                        state.chapterDialogue.line == Line::SalvagerShoreReturn,
                        "the player observes the timed breath cue, pauses it, returns to dry stone and reports the lesson");
                    if (!progressed)
                        std::cerr << "Combined shore breath beginning=" << breathTicks
                                  << " cue=" << breathCue.data() << " wetFrame=" << presentedWetRail
                                  << " stage=" << static_cast<int>(state.shorePracticeStage)
                                  << " remaining=" << state.shorePracticeBreathTicksRemaining << '\n';
                }
                if (progressed) {
                    progressed = check(reachWorkbench() && face(kMillComparison[0], kMillComparison[2]) && east() &&
                        state.millAction == MillAction::InspectGearing && capture("windows-combined-mill.png") &&
                        interact() && state.millPreparation == MillPreparation::BearingNeeded,
                        "the mill workbench advances preparation with both eastern actors in the same presented scene");
                }
                if (progressed && gBellwetherPreparationsValidation) {
                    progressed = check(leaveWorkbench() && walkTo(kBellwetherBearingBinApproach) &&
                        useMill(MillAction::RecoverBearing, kMillBearingBin) &&
                        state.millPreparation == MillPreparation::BearingRecovered && reachWorkbench() &&
                        useMill(MillAction::CheckFit, kMillComparison) && state.millPreparation == MillPreparation::FitChecked &&
                        useMill(MillAction::SeatBearing, kMillDriveWork) && state.millPreparation == MillPreparation::BearingSeated &&
                        useMill(MillAction::TurnDrive, kMillDriveWork) &&
                        waitFor([&] { return state.millPreparation == MillPreparation::GearingVerified; }, 75) &&
                        walkTo(mill.back()) && walkTo(kBellwetherMillArtificerView) &&
                        face(kBellwetherMillArtificerMark.x, kBellwetherMillArtificerMark.z) &&
                        state.millAction == MillAction::TakeRig && interact() && frame() &&
                        state.millPreparation == MillPreparation::RigTrialReady && !state.waysideArchive.airWardLearned &&
                        capture("windows-combined-mill-handoff.png"),
                        "recovering and testing the bearing starts the mill and earns the rig without an air ward");
                }
                if (progressed && gBellwetherPreparationsValidation) {
                    progressed = check(leaveWorkbench() && walkTo(kBellwetherSalvagerMark) &&
                        approachShallows() && state.millAction == MillAction::StartLeakTest && interact() &&
                        state.millLeakTestPhase == MillLeakTestPhase::Testing &&
                        waitFor([&] { return state.millLeakTestPhase == MillLeakTestPhase::ReturnToDry; }, 130) &&
                        state.waterContact.wetGround && state.millAction == MillAction::None &&
                        face(kBellwetherRiverDryMark.x, kBellwetherRiverDryMark.z) &&
                        capture("windows-combined-rig-trial.png") &&
                        walkTo(kBellwetherRiverDryMark) && state.millAction == MillAction::ConfirmSeal && interact() &&
                        state.millPreparation == MillPreparation::RigTested && state.salvagerShorePracticeCompleted &&
                        backFromShore() && walkTo(mill[1]),
                        "the earned rig only seals after a full wet hold and deliberate confirmation on dry stone");
                }
                const auto settleResidentPair = [&](std::uint32_t second) {
                    const std::array expected{kTavernKeeperActorId, second};
                    // The replay advances whole sections between frames. Give
                    // the asynchronous prefetch a bounded wall-clock window
                    // at the witness mark before checking publication.
                    const auto readyBy = std::chrono::steady_clock::now() + std::chrono::milliseconds(750);
                    while (ctx.rtScene.ChapterResidentSourceIds() != expected &&
                           std::chrono::steady_clock::now() < readyBy)
                        if (!frame()) return false;
                    return ctx.rtScene.ChapterResidentSourceIds()==expected && step();
                };
                if (progressed) {
                    const bool complete = gBellwetherPreparationsValidation;
                    const bool backAtJunction = complete ? walkTo(*junction) :
                        std::all_of(mill.rbegin(), mill.rend(), walkTo);
                    const auto forkResident = gBellwetherCombinedRelicDealer ? kRelicDealerActorId : kSalvagerActorId;
                    progressed = check(
                        backAtJunction && settleResidentPair(forkResident) && fork() &&
                        std::all_of(std::next(junction), kBellwetherTavernRoute.end(), walkTo) &&
                        walkTo({target.x, target.z}) && settleResidentPair(kRecordKeeperActorId) &&
                        atHearth() && town() && frame() &&
                        state.millPreparation == (complete ? MillPreparation::RigTested : MillPreparation::BearingNeeded) &&
                        state.shorePracticeStage == (complete ? ShorePracticeStage::Idle : ShorePracticeStage::FindDryMark) &&
                        capture("windows-combined-return.png"),
                        "returning to the same tavern restores both town residents and retains the two eastern activities");
                }
                if (progressed && verifiesSilver && !gBellwetherPreparationsValidation) {
                    using campaign::MillTallyAction;
                    using campaign::MillTallyProgress;
                    using campaign::TreasurePurse;
                    const auto depart = [&] {
                        return std::all_of(kBellwetherTavernRoute.rbegin(),
                            std::make_reverse_iterator(junction),walkTo);
                    };
                    const auto walkAt = [&](const auto& point) {return walkTo({point[0],point[1]});};
                    const auto offer = [&](const auto& mark,MillTallyAction action) {
                        if(!face(mark[0],mark[2]) || state.millTallyAction!=action ||
                           !state.HasChapterInteractPrompt() || !ctx.tavernInteractionPromptPresented) {
                            std::cerr << "Mill tally mark " << state.playerX << ',' << state.playerZ
                                      << " offered=" << static_cast<int>(state.millTallyAction)
                                      << " expected=" << static_cast<int>(action) << '\n';
                            return false;
                        }
                        return true;
                    };
                    const auto tray = [&] {
                        return depart() && std::all_of(kBellwetherWestApproach.begin(),
                            kBellwetherWestApproach.end(),walkTo) && walkAt(kMillTallyTrayView) &&
                            settleResidentPair(kRelicDealerActorId);
                    };
                    const auto board = [&] {
                        return depart() && std::all_of(std::next(mill.begin()),mill.end(),walkTo) &&
                            walkAt(kMillTallyBoardView);
                    };
                    const auto reloadTally = [&](MillTallyProgress progress,std::uint32_t expectedCoins) {
                        campaign::storage::StoredCheckpoint saved;
                        if(!until([&] {
                            return campaign::storage::Read(gCampaignValidationDirectory,"slot-1-auto.hrd",saved).status==
                                campaign::storage::Status::Read && saved.checkpoint.schema==campaign::kMillTallySchema &&
                                saved.checkpoint.treasurePurse.millTally==progress &&
                                saved.checkpoint.treasurePurse.coins==expectedCoins &&
                                ctx.campaign->State().outcome==campaign::PlayOutcome::Succeeded;
                        },false) || !reloadCheckpoint() ||
                            !settleResidentPair(kRecordKeeperActorId) || !atHearth() || !frame()) {
                            std::cerr << "Tally reload witness failed for progress " << static_cast<int>(progress)
                                      << " at " << state.playerX << ',' << state.playerZ << '\n';
                            return false;
                        }
                        sceneEpoch=ctx.rtFrameEvidence.PublishedStateByValue().sceneEpoch;
                        geometryGeneration=ctx.rtScene.StaticGeometryGeneration();
                        return state.treasurePurse==saved.checkpoint.treasurePurse &&
                            state.privyRag==PrivyRagState::Delivered &&
                            saved.checkpoint.id==campaign::CheckpointId::BellwetherTavern;
                    };
                    const auto before=state.treasurePurse;
                    const auto quoted=before.coins+TreasurePurse::kMillTallySaleCoins;
                    const float previousPitch=input.pitchRadians;
                    input.pitchRadians=-.72f;
                    const auto wetSteps=state.waterContact.stepCount;
                    const auto dry=[&](BellwetherStation station) {
                        return walkTo(station) && state.playerGrounded && !state.waterContact.wetGround &&
                            state.waterContact.stepCount==wetSteps;
                    };
                    progressed=check(depart() && walkTo(mill[1]) && walkTo(kBellwetherSalvagerMark) &&
                        std::all_of(landing.begin(),landing.end()-2,dry) &&
                        std::all_of(kBellwetherMillCrossing.begin(),kBellwetherMillCrossing.end(),dry) &&
                        walkAt(kMillTallyCaseView) && offer(kMillTallyCaseReach,MillTallyAction::Take) &&
                        capture("windows-tally-case.png",false) && interact() &&
                        state.millTallyAction==MillTallyAction::None && reloadTally(MillTallyProgress::Carried,before.coins),
                        "the dry mill crossing reaches the tally case and persists the recovered stamp at the tavern");
                    input.pitchRadians=previousPitch;
                    if(progressed) {
                        using campaign::OrdinarySilverAction;
                        std::array<char,96> text{};
                        progressed=check(tray() && face(kBellwetherRelicDealerStand.x,kBellwetherRelicDealerStand.z) &&
                            state.ordinarySilverAction==OrdinarySilverAction::Sold && state.millTallyAction==MillTallyAction::None &&
                            offer(kMillTallyTrayReach,MillTallyAction::Sell) &&
                            GetWindowTextA(GetDlgItem(ctx.windowHandle,kChestPromptControlId),text.data(),static_cast<int>(text.size()))>0 &&
                            std::string_view(text.data()).starts_with("SELL TALLY STAMP (+2 COINS)") &&
                            capture("windows-tally-tray.png") && interact() && state.treasurePurse.coins==quoted &&
                            state.millTallyAction==MillTallyAction::None && reloadTally(MillTallyProgress::Sold,quoted),
                            "the dealer tray quotes and credits a single named sale without replacing the ordinary trade mark");
                    }
                    if(progressed) {
                        progressed=check(board() && offer(kMillTallyBoardReach,MillTallyAction::RecordClaim) &&
                            capture("windows-tally-board.png",false) && interact() && state.millTallyAction==MillTallyAction::None &&
                            reloadTally(MillTallyProgress::ClaimRecorded,quoted) &&
                            readDirections(campaign::PauseObjective::BellwetherShrine,"Recover the tally stamp at the dealer"),
                            "the mill mark records a recoverable claim and Directions points to the dealer");
                    }
                    if(progressed) {
                        progressed=check(tray() && offer(kMillTallyTrayReach,MillTallyAction::Recover) &&
                            interact() && state.millTallyAction==MillTallyAction::None &&
                            reloadTally(MillTallyProgress::Recovered,quoted) && board() &&
                            offer(kMillTallyBoardReach,MillTallyAction::Return) && interact() &&
                            state.millTallyAction==MillTallyAction::None && reloadTally(MillTallyProgress::ReturnedAfterSale,quoted) &&
                            state.treasurePurse.ordinarySilver==before.ordinarySilver &&
                            state.treasurePurse.lanternFittingPurchased==before.lanternFittingPurchased &&
                            readDirections(campaign::PauseObjective::BellwetherShrine,"The mill's tally stamp has been returned."),
                            "the paid stamp can be recovered free and returned once without changing other purchases");
                    }
                }
                if (progressed && gBellwetherPreparationsValidation) {
                    campaign::storage::StoredCheckpoint eastern;
                    progressed = check(until([&] {
                        return campaign::storage::Read(gCampaignValidationDirectory, "slot-1-auto.hrd", eastern).status ==
                            campaign::storage::Status::Read && eastern.checkpoint.salvagerShorePracticeCompleted &&
                            eastern.checkpoint.millPreparation == MillPreparation::RigTested &&
                            ctx.campaign->State().outcome == campaign::PlayOutcome::Succeeded;
                    }, false) && eastern.checkpoint.id == stored.checkpoint.id &&
                        eastern.checkpoint.position.x == stored.checkpoint.position.x &&
                        eastern.checkpoint.position.y == stored.checkpoint.position.y &&
                        eastern.checkpoint.position.z == stored.checkpoint.position.z &&
                        eastern.checkpoint.vitality == stored.checkpoint.vitality &&
                        eastern.checkpoint.treasurePurse == stored.checkpoint.treasurePurse &&
                        eastern.checkpoint.privyRagDelivered == stored.checkpoint.privyRagDelivered &&
                        !eastern.checkpoint.airWardLearned && reloadCheckpoint() && step() && atHearth() &&
                        state.salvagerShorePracticeCompleted && state.millPreparation == MillPreparation::RigTested &&
                        state.millLeakTestPhase == MillLeakTestPhase::Idle &&
                        state.treasurePurse == stored.checkpoint.treasurePurse &&
                        state.privyRag == (stored.checkpoint.privyRagDelivered ? PrivyRagState::Delivered : PrivyRagState::Available) && frame(),
                        "the native tavern reload preserves both eastern preparations, western purse and rag at the earned return position");
                    if (progressed) {
                        stored = std::move(eastern);
                        progressed = check(readDirections(campaign::PauseObjective::BellwetherShrine, "Shore practice: Complete."),
                            "Bellwether Directions shows the saved shore result after the tavern reload");
                    }
                }
            }
            if (progressed && gBellwetherHearthRestValidation) {
                using namespace horde::gameplay::healing;
                constexpr auto hearth = static_cast<std::uint32_t>(campaign::CheckpointId::BellwetherTavern);
                progressed = check(state.healingFlaskEnabled && state.flaskRestAnchor == hearth &&
                    state.flaskNearbyRestAnchor == hearth && state.healingFlask.resources.charges == 0 &&
                    stored.checkpoint.flaskRecorded && stored.checkpoint.flask == state.healingFlask.resources,
                    "the rendered tavern reload offers hearth Rest with the exact empty flask");
                if (progressed) {
                    const auto restGeneration = state.completedRestGeneration;
                    ++input.commands.rest;
                    progressed = check(step() && state.healingFlask.action == FlaskAction::Resting &&
                        state.healingFlask.resources.charges == 0 && waitFor([&] {
                            return state.completedRestGeneration > restGeneration;
                        }, 123) && state.completedRestGeneration == restGeneration + 1 &&
                        state.healingFlask.resources == FlaskResources{false,2} && frame(),
                        "a deliberate fixed-step Rest completes once and presents the refilled flask state");
                }
                if (progressed) {
                    campaign::storage::StoredCheckpoint rested;
                    progressed = check(until([&] {
                        return campaign::storage::Read(gCampaignValidationDirectory, "slot-1-auto.hrd", rested).status ==
                            campaign::storage::Status::Read && rested.checkpoint.flask == FlaskResources{false,2} &&
                            ctx.campaign->State().outcome == campaign::PlayOutcome::Succeeded;
                    }, false) && rested.checkpoint.id == stored.checkpoint.id &&
                        rested.checkpoint.position.x == stored.checkpoint.position.x &&
                        rested.checkpoint.position.z == stored.checkpoint.position.z &&
                        rested.checkpoint.dialogue.tavernConversationLinesResolved == kTavernConversation.size(),
                        "native autosave persists the refilled flask at the same earned tavern boundary");
                    if (progressed) {
                        const auto restGeneration = state.completedRestGeneration;
                        progressed = check(reloadCheckpoint() && step() && atHearth() && frame() &&
                            state.healingFlask.resources == rested.checkpoint.flask &&
                            state.healingFlask.action == FlaskAction::Idle && state.completedRestGeneration == restGeneration,
                            "native Load presents the rested save without replaying Rest");
                        if (progressed) stored = std::move(rested);
                    }
                }
            }
            if (progressed && gBellwetherExpeditionValidation) {
                using namespace horde::gameplay::traversal;
                using namespace horde::gameplay::bellwether;
                using horde::gameplay::interactions::HeldLightPose;
                using campaign::PauseObjective;
                const auto junction = kBellwetherTavernRoute.begin() + kBellwetherChurchJunction;
                const auto walk = [&](auto first, auto last) { return std::all_of(first, last, walkTo); };
                const auto face = [&](float yaw, float pitch) {
                    input.yawRadians = yaw;
                    input.pitchRadians = pitch;
                    return step();
                };
                const auto interact = [&] {
                    input.commands.interact = state.lastConsumedInteractSequence + 1;
                    return step();
                };
                const auto dialogue = [&](std::span<const Line> lines) {
                    for (const auto line : lines) {
                        if (state.chapterDialogue.line != line) return false;
                        ++input.commands.dialogueSkip;
                        if (!step()) return false;
                    }
                    return state.chapterDialogue.line == Line::None;
                };
                const auto approachKeeper = [&] {
                    return walkTo(kBellwetherChurchApproach.front()) &&
                        walkTo(kBellwetherRecordKeeperApproach) && face(1.57079632679f, -.12f);
                };
                progressed = check(readDirections(PauseObjective::BellwetherShrine),
                    "the restored tavern pause menu recalls the shrine lead without advancing play");
                const bool reachedKeeper = progressed &&
                    walk(kBellwetherTavernRoute.rbegin(), std::make_reverse_iterator(junction)) &&
                    walk(kBellwetherNorthApproach.begin(), kBellwetherNorthApproach.end()) && approachKeeper();
                const auto offered = state.recordKeeperAction;
                const bool presentedLead = reachedKeeper && offered == RecordKeeperAction::ShowRoadMark &&
                    interact() && frame();
                progressed = check(presentedLead && dialogue(kRecordKeeperLead),
                    "the staffed church gives the wayside lead after the player leaves the earned tavern");
                if (!progressed)
                    std::cerr << "Bellwether church lead reached=" << reachedKeeper
                              << " offered=" << static_cast<int>(offered) << " presented=" << presentedLead
                              << " currentLine=" << static_cast<int>(state.chapterDialogue.line)
                              << " hearthLines=" << state.chapterDialogue.hearthLinesResolved
                              << " keeperAdmitted=" << state.recordKeeper.has_value()
                              << " player=" << state.playerX << ',' << state.playerSupportWorldY << ',' << state.playerZ
                              << " grounded=" << state.playerGrounded << " yaw=" << state.playerYawRadians
                              << " paused=" << state.paused << '\n';
                if (progressed) {
                    constexpr std::array help{Line::LanternFirstHelp};
                    constexpr std::array reveal{Line::LanternFirstIdentity, Line::LanternKingName, Line::LanternFirstRequest};
                    constexpr std::array pickup{Line::LanternRegisterReturn};
                    progressed = check(walkTo(kBellwetherChurchApproach.front()) && walkTo(kBellwetherNorthApproach.back()) &&
                        walk(kBellwetherWaysideApproach.begin(), kBellwetherWaysideApproach.end()) && face(0.f, -.12f) &&
                        state.waysideArchiveAction == ArchiveAction::Inspect && interact() && dialogue(help) &&
                        readDirections(PauseObjective::BellwetherLowerStone) &&
                        state.waysideArchiveAction == ArchiveAction::Open && interact() &&
                        waitFor([&] { return state.waysideArchive.stage == ArchiveStage::Open; }, 90) && frame() &&
                        dialogue(reveal) && readDirections(PauseObjective::BellwetherArchive) &&
                        state.waysideArchiveAction == ArchiveAction::TakeRegister && interact() &&
                        state.waysideArchive.registerRecovered && !state.waysideArchive.registerInterpreted && dialogue(pickup),
                        "physical archive opening earns the shrine reveal and recoverable register before interpretation");
                    if (!progressed)
                        std::cerr << "Bellwether shrine stage=" << static_cast<int>(state.waysideArchive.stage)
                                  << " action=" << static_cast<int>(state.waysideArchiveAction)
                                  << " line=" << static_cast<int>(state.chapterDialogue.line)
                                  << " recovered=" << state.waysideArchive.registerRecovered << '\n';
                }
                if (progressed)
                    progressed = check(readDirections(PauseObjective::BellwetherRegister),
                        "away from the tavern the pause directions point the recovered register back to the record keeper");
                if (progressed) {
                    progressed = check(walk(std::next(kBellwetherWaysideApproach.rbegin()), kBellwetherWaysideApproach.rend()) &&
                        walkTo(kBellwetherNorthApproach.back()) && approachKeeper() &&
                        state.recordKeeperAction == RecordKeeperAction::ShowRegister && interact() &&
                        state.waysideArchive.registerInterpreted && frame() && dialogue(kRecordKeeperFirstReading) &&
                        state.recordKeeperAction == RecordKeeperAction::CollectReward && state.treasurePurse.coins == 0 &&
                        state.chapterDialogue.tavernLanternLinesStarted == 0,
                        "the real register handoff teaches both routes independently of Kit's firsthand knowledge");
                }
                if (progressed)
                    progressed = check(readDirections(PauseObjective::BellwetherExpeditions),
                        "the church pause directions show both interpreted routes as future expeditions");
                if (progressed) {
                    campaign::storage::StoredCheckpoint restored;
                    progressed = check(walkTo(kBellwetherChurchApproach.front()) && walkTo(kBellwetherNorthApproach.back()) &&
                        walk(kBellwetherWaysideApproach.begin(), kBellwetherWaysideApproach.end()) && face(0.f,-.12f) &&
                        state.waysideArchiveAction == ArchiveAction::RestoreBinding && interact() &&
                        state.waysideArchive.stage == ArchiveStage::Closing && waitFor([&] {
                            return state.waysideArchive.stage == ArchiveStage::Restored;
                        }, 90) && state.waysideArchive.opening == 0.f && !state.waysideArchive.airWardLearned && frame() &&
                        until([&] {
                            return campaign::storage::Read(gCampaignValidationDirectory,"slot-1-auto.hrd",restored).status ==
                                campaign::storage::Status::Read &&
                                restored.checkpoint.waysideArchive == campaign::WaysideArchiveProgress::Restored &&
                                !restored.checkpoint.airWardLearned && restored.checkpoint.waysideRegisterRecovered;
                        },false) && readDirections(PauseObjective::BellwetherExpeditions),
                        "the rendered lower stone closes and saves the restored binding while the ward remains unlearned");
                }
                const bool westernSilverPaid = gBellwetherCombinedRelicDealer && gBellwetherPreparationsValidation;
                const campaign::TreasurePurse westernPurse{0,false,true,campaign::OrdinarySilverProgress::Sold};
                if (progressed) {
                    campaign::storage::StoredCheckpoint taught;
                    progressed = check(walk(std::next(kBellwetherWaysideApproach.rbegin()), kBellwetherWaysideApproach.rend()) &&
                        walkTo(kBellwetherNorthApproach.back()) && approachKeeper() &&
                        state.recordKeeperAction == RecordKeeperAction::LearnAirWard && interact() &&
                        state.waysideArchive.airWardLearned &&
                        state.treasurePurse == (westernSilverPaid ? westernPurse : campaign::TreasurePurse{}) && frame() &&
                        dialogue(kRecordKeeperWard) && state.recordKeeperAction == RecordKeeperAction::CollectReward &&
                        until([&] {
                            return campaign::storage::Read(gCampaignValidationDirectory,"slot-1-auto.hrd",taught).status ==
                                campaign::storage::Status::Read && taught.checkpoint.airWardLearned &&
                                taught.checkpoint.treasurePurse == (westernSilverPaid ? westernPurse : campaign::TreasurePurse{}) &&
                                taught.checkpoint.privyRagDelivered == stored.checkpoint.privyRagDelivered;
                        },false) && readDirections(PauseObjective::BellwetherExpeditions),
                        "the return lesson saves the air ward independently of the purse and register payout");
                }
                const campaign::TreasurePurse earned{campaign::TreasurePurse::kRegisterRewardCoins,true,
                    westernSilverPaid,westernSilverPaid ? campaign::OrdinarySilverProgress::Sold :
                        campaign::OrdinarySilverProgress::Unfound};
                if (progressed) {
                    campaign::storage::StoredCheckpoint rewarded;
                    progressed = check(interact() && state.treasurePurse == earned && frame() &&
                        dialogue(kRecordKeeperReward) && state.recordKeeperAction == RecordKeeperAction::RecallRoutes &&
                        until([&] {
                            return campaign::storage::Read(gCampaignValidationDirectory, "slot-1-auto.hrd", rewarded).status ==
                                campaign::storage::Status::Read && rewarded.checkpoint.treasurePurse == earned &&
                                ctx.campaign->State().operation == campaign::PlayOperation::Autosave &&
                                ctx.campaign->State().outcome == campaign::PlayOutcome::Succeeded;
                        }, false) && readDirections(PauseObjective::BellwetherExpeditions),
                        "collecting the register reward saves a single payment and shows it in the pause menu");
                }
                if (progressed) {
                    progressed = check(walkTo(kBellwetherChurchApproach.front()) &&
                        walk(kBellwetherNorthApproach.rbegin(), kBellwetherNorthApproach.rend()) &&
                        walk(std::next(junction), kBellwetherTavernRoute.end()) &&
                        walkTo({kConversation[0], kConversation[2]}) && face(-1.40f, -.22f) && atHearth(),
                        "the player returns through the south door to the same staffed hearth");
                }
                const campaign::TreasurePurse fitted{westernSilverPaid ? earned.coins :
                    earned.coins-campaign::TreasurePurse::kLanternFittingPrice,true,true,earned.ordinarySilver};
                if (progressed) {
                    constexpr std::array fittingLine{Line::TavernLanternFitting};
                    std::array<char,256> quote{};
                    const bool offered = walkTo(kBellwetherLanternFittingApproach) &&
                        face(std::atan2(kKeeperHearth[0]-state.playerX,state.playerZ-kKeeperHearth[2]),-.12f) &&
                        state.treasurePurse == earned && frame();
                    if (westernSilverPaid) {
                        const auto revision = ctx.simulation.CampaignBoundaryRevision();
                        campaign::storage::StoredCheckpoint retained;
                        progressed = check(offered && !state.tavernLanternFittingAvailable && interact() &&
                            state.treasurePurse == earned && !state.tavernLanternFittingAvailable &&
                            state.chapterDialogue.line == Line::None &&
                            ctx.simulation.CampaignBoundaryRevision() == revision &&
                            campaign::storage::Read(gCampaignValidationDirectory,"slot-1-auto.hrd",retained).status ==
                                campaign::storage::Status::Read && retained.checkpoint.treasurePurse == earned &&
                            readDirections(PauseObjective::BellwetherExpeditions) &&
                            walkTo({kConversation[0],kConversation[2]}) && face(-1.40f,-.22f),
                            "the fitted hinge cannot be purchased a second time after the register payout");
                    } else {
                        const bool quoted = offered && state.tavernLanternFittingAvailable &&
                            ctx.tavernInteractionPromptPresented &&
                            GetWindowTextA(GetDlgItem(ctx.windowHandle,kChestPromptControlId),quote.data(),static_cast<int>(quote.size()))>0 &&
                            std::string_view(quote.data()).find(std::to_string(campaign::TreasurePurse::kLanternFittingPrice)+" COINS")!=
                                std::string_view::npos;
                        campaign::storage::StoredCheckpoint purchase;
                        progressed = check(quoted && interact() && state.treasurePurse == fitted && frame() &&
                            dialogue(fittingLine) && interact() && !state.tavernLanternFittingAvailable && state.treasurePurse == fitted &&
                            until([&] {
                                return campaign::storage::Read(gCampaignValidationDirectory,"slot-1-auto.hrd",purchase).status ==
                                    campaign::storage::Status::Read && purchase.checkpoint.treasurePurse == fitted &&
                                    ctx.campaign->State().operation == campaign::PlayOperation::Autosave &&
                                    ctx.campaign->State().outcome == campaign::PlayOutcome::Succeeded;
                            },false) && readDirections(PauseObjective::BellwetherExpeditions) &&
                            walkTo({kConversation[0],kConversation[2]}) && face(-1.40f,-.22f),
                            "the keeper quotes the hinge price and saves the single purchase before the player returns to Kit");
                    }
                    if (!progressed)
                        std::cerr << "Bellwether fitting offered=" << offered << " prompt=" << quote.data()
                                  << " available=" << state.tavernLanternFittingAvailable << " coins=" << state.treasurePurse.coins << '\n';
                }
                if (progressed) {
                    const auto settle = [&](HeldLightPose pose) {
                        return waitFor([&] { return state.interaction.heldLightPose == pose; }, 125);
                    };
                    const auto toggle = [&] {
                        input.commands.toggleHeldLightPose = state.lastConsumedToggleHeldLightPoseSequence + 1;
                        return step();
                    };
                    const bool lowered = state.interaction.heldLightPose == HeldLightPose::Low ||
                        (toggle() && settle(HeldLightPose::Low));
                    progressed = check(lowered && state.NeedsRaiseLanternPrompt() && toggle() &&
                        settle(HeldLightPose::High) && state.chapterDialogue.line == Line::None &&
                        frame() && ctx.lastChapterPresentationAckTick == state.tickIndex && step() &&
                        dialogue(kTavernLanternExchange),
                        "a newly presented high lantern starts all six firsthand lines at Kit's chair");
                }
                if (progressed) {
                    campaign::storage::StoredCheckpoint expedition;
                    const auto original = stored.checkpoint;
                    progressed = check(until([&] {
                        return campaign::storage::Read(gCampaignValidationDirectory, "slot-1-auto.hrd", expedition).status ==
                            campaign::storage::Status::Read && expedition.checkpoint.waysideRegisterInterpreted &&
                            expedition.checkpoint.dialogue.tavernLanternLinesStarted == kTavernLanternExchange.size() &&
                            ctx.campaign->State().operation == campaign::PlayOperation::Autosave &&
                            ctx.campaign->State().outcome == campaign::PlayOutcome::Succeeded;
                    }, false) && expedition.checkpoint.id == original.id &&
                        expedition.checkpoint.position.x == original.position.x &&
                        expedition.checkpoint.position.y == original.position.y &&
                        expedition.checkpoint.position.z == original.position.z &&
                        expedition.checkpoint.vitality == original.vitality &&
                        expedition.checkpoint.flask == original.flask && expedition.checkpoint.treasurePurse == fitted &&
                        expedition.checkpoint.privyRagDelivered == original.privyRagDelivered &&
                        expedition.checkpoint.airWardLearned &&
                        expedition.checkpoint.dialogue.activeLine == Line::None,
                        "the expedition autosave retains the earned tavern position, resources and both discoveries");
                    if (progressed) {
                        campaign::storage::StoredCheckpoint resumed;
                        progressed = check(reloadCheckpoint() && step() && atHearth() && state.waysideArchive.registerInterpreted &&
                            state.chapterDialogue.tavernLanternLinesStarted == kTavernLanternExchange.size() &&
                            state.treasurePurse == fitted && state.waysideArchive.airWardLearned &&
                            state.waysideArchive.stage == ArchiveStage::Restored &&
                            (!state.healingFlaskEnabled || state.healingFlask.resources == expedition.checkpoint.flask) &&
                            state.privyRag == (expedition.checkpoint.privyRagDelivered ? PrivyRagState::Delivered : PrivyRagState::Available) &&
                            state.chapterDialogue.line == Line::None && frame() &&
                            campaign::storage::Read(gCampaignValidationDirectory, "slot-1-auto.hrd", resumed).status ==
                                campaign::storage::Status::Read && resumed.encoded == expedition.encoded,
                            "native reload preserves both discoveries and does not replay the speech or rewrite the save");
                        if (progressed)
                            check(readDirections(PauseObjective::BellwetherExpeditions),
                                "the reloaded tavern pause menu restores the earned expedition directions");
                    }
                }
            }
            if (progressed && gBellwetherPreparationsValidation) {
                const auto& combined = ctx.simulation.LatestCampaignBoundary();
                check(combined && campaign::Validate(*combined) && combined->airWardLearned &&
                    combined->salvagerShorePracticeCompleted &&
                    combined->millPreparation == horde::gameplay::bellwether::MillPreparation::RigTested &&
                    combined->treasurePurse.lanternFittingPurchased && state.waysideArchive.airWardLearned &&
                    (!gBellwetherCombinedRelicDealer ||
                        (combined->treasurePurse == campaign::TreasurePurse{campaign::TreasurePurse::kRegisterRewardCoins,
                            true,true,campaign::OrdinarySilverProgress::Sold} && combined->privyRagDelivered &&
                            state.treasurePurse == combined->treasurePurse && state.privyRag ==
                                horde::gameplay::bellwether::PrivyRagState::Delivered)) &&
                    state.salvagerShorePracticeCompleted &&
                    state.millPreparation == horde::gameplay::bellwether::MillPreparation::RigTested &&
                    state.shorePracticeStage == horde::gameplay::bellwether::ShorePracticeStage::Idle &&
                    state.millLeakTestPhase == horde::gameplay::bellwether::MillLeakTestPhase::Idle,
                    "the same tavern save retains Magic, Tech and Constitution preparation after the final native reload");
            }
        } else if (atLookout) {
            SendMessageA(ctx.windowHandle, WM_COMMAND, MAKEWPARAM(kMenuPauseId,0), 0);
            std::vector<double> timings;
            check(click(kLookoutObjectiveButtonId) && click(LookoutPanel::MenuId) &&
                ApplyPendingSceneReplacement(ctx, capabilities, timings) && frame() &&
                ctx.rtScene.Profile() == RtSceneProfile::EntryMenu && ctx.rtScene.ChapterResidentSourceCount() == 0,
                "title replacement drops chapter resident resources");
        }
        if (!gBellwetherEarnTavernValidation) {
            campaign::storage::StoredCheckpoint current;
            check(campaign::storage::Read(gCampaignValidationDirectory,"slot-1-auto.hrd",current).status ==
                campaign::storage::Status::Read && current.encoded == gHiddenLookout.source.encoded &&
                !gHiddenCampaignFaults.rejectedWrites, "source lookout autosave bytes preserved");
        }
    } else {
        check(entered && !state.rescue.exteriorSide && state.playerVitals.vitality == 3,
            "New Game presents with the same Bellwether resident resource admission");
    }
    check(hidden(), "isolated hidden campaign retains foreground ownership");
    receipt << "],\"count\":" << checks << ",\"failures\":" << failures
        << ",\"stagesComplete\":" << (failures ? "false" : "true");
    if (!lookout) {
        receipt << "}\n";
        if (!WriteReportFile(gCampaignValidationDirectory / "windows-validation.json", receipt.str())) return 1;
    }
    return failures ? 1 : 0;
}

#include "windows_route_validation.inl"
#endif

bool ConfigureHiddenCampaignValidation()
{
    int count=0;auto args=CommandLineToArgvW(GetCommandLineW(),&count);if(!args)return false;
    bool requested=false,lookoutRequested=false,earnTavernRequested=false,routeRequested=false,routeTurnStartRequested=false,routeNaturalWalkRequested=false,returnRequested=false,inspectKit=false,inspectMotion=false,inspectTiming=false,inspectTurns=false,inspectPartials=false,inspectPartialsDense=false,inspectController=false,benchmarkMenuRequested=false,detachedCancelRequested=false,detachedCloseRequested=false;std::filesystem::path directory;
    bool expeditionRequested = false, preparationsRequested = false, hearthRestRequested = false, ankleRestoreRequested = false;
    for(int i=1;i<count;++i) {
        const std::wstring_view arg=args[i];
        if(arg==L"--campaign-validate-hidden")requested=true;
        if(arg==L"--benchmark-menu-validate")benchmarkMenuRequested=true;
        if(arg==L"--benchmark-menu-detached-cancel")detachedCancelRequested=true;
        if(arg==L"--benchmark-menu-detached-close")detachedCloseRequested=true;
        if(arg==L"--lookout-validate-hidden")lookoutRequested=true;
        if(arg==L"--bellwether-earn-tavern")earnTavernRequested=true;
        if(arg==L"--bellwether-verify-ankle-restore"){earnTavernRequested=true;ankleRestoreRequested=true;}
        if(arg==L"--bellwether-verify-expedition-leads"){earnTavernRequested=true;expeditionRequested=true;}
        if(arg==L"--bellwether-verify-preparations"){earnTavernRequested=true;expeditionRequested=true;preparationsRequested=true;}
        if(arg==L"--bellwether-verify-hearth-rest"){earnTavernRequested=true;hearthRestRequested=true;}
        if(arg==L"--route-validate-hidden")routeRequested=true;
        if(arg==L"--route-kit-turn-start")routeTurnStartRequested=true;
        if(arg==L"--route-natural-walk")routeNaturalWalkRequested=true;
        if(arg==L"--route-return-recovery-hidden")returnRequested=true;
        if(arg==L"--lookout-inspect-kit")inspectKit=true;
        if(arg==L"--lookout-inspect-kit-motion"){inspectKit=true;inspectMotion=true;}
        if(arg==L"--lookout-inspect-kit-timing"){inspectKit=true;inspectMotion=true;inspectTiming=true;}
        if(arg==L"--lookout-inspect-kit-turns"){inspectKit=true;inspectTurns=true;}
        if(arg==L"--lookout-inspect-kit-partials"){inspectKit=true;inspectTurns=true;inspectPartials=true;}
        if(arg==L"--lookout-inspect-kit-partials-dense"){inspectKit=true;inspectTurns=true;inspectPartials=true;inspectPartialsDense=true;}
        if(arg==L"--lookout-inspect-kit-controller"){inspectKit=true;inspectController=true;}
        if(arg==L"--campaign-test-root" && i+1<count)directory=args[++i];
    }
    LocalFree(args);
    if (hearthRestRequested && !HORDE_RT_FLASK_ADMITTED) {
        std::cerr << "Hidden hearth Rest requires the flask-enabled campaign validation build.\n";
        return false;
    }
    if (preparationsRequested && hearthRestRequested) {
        std::cerr << "Select the preparation route or the separate hearth Rest check.\n";
        return false;
    }
    if (earnTavernRequested && (!lookoutRequested || inspectKit)) {
        std::cerr << "Earning a tavern save requires the isolated hidden lookout fixture without actor inspection.\n";
        return false;
    }
    if((routeTurnStartRequested || routeNaturalWalkRequested) && !routeRequested) {
        std::cerr<<"Kit turn/start opt-in requires the isolated hidden route validator.\n";return false;
    }
    if(inspectTurns && inspectMotion) {std::cerr<<"Select one bounded Kit motion or authored-turn inspection.\n";return false;}
    if(inspectController && (inspectMotion || inspectTurns)) {
        std::cerr<<"Shared Kit controller inspection cannot combine motion, timing or manual turn modes.\n";return false;
    }
    if(detachedCancelRequested && detachedCloseRequested) {
        std::cerr<<"Select one detached benchmark input validation action.\n";return false;
    }
    if((detachedCancelRequested || detachedCloseRequested) && !benchmarkMenuRequested) {
        std::cerr<<"Detached benchmark input validation requires isolated --benchmark-menu-validate.\n";return false;
    }
    if(!requested && !lookoutRequested && !routeRequested && !returnRequested && !inspectKit && !benchmarkMenuRequested && directory.empty())return true;
#if defined(_DEBUG) && defined(HORDE_RT_SOURCE_DIR)
    std::error_code error;
    const auto absolute=std::filesystem::absolute(directory,error).lexically_normal();
    const auto reports=std::filesystem::canonical(std::filesystem::path(HORDE_RT_SOURCE_DIR)/"reports",error);
    if(unsigned(requested)+unsigned(lookoutRequested)+unsigned(routeRequested)+unsigned(returnRequested)+unsigned(benchmarkMenuRequested)!=1 || (inspectKit && !lookoutRequested) || directory.empty() || error || !directory.is_absolute() ||
        !absolute.filename().string().starts_with("horde-campaign-fixtures-") ||
        std::filesystem::canonical(absolute.parent_path(),error)!=reports || error ||
        std::filesystem::canonical(absolute,error)!=absolute || error || !std::filesystem::is_directory(absolute,error) || error ||
        !std::filesystem::is_regular_file(absolute/"manifest.json",error) || error){
        std::cerr<<"Hidden campaign validation requires a caller-created exported fixture directory directly in this checkout's reports.\n";return false;
    }
    for(const auto& item:std::filesystem::directory_iterator(absolute))if(item.is_symlink())return false;
    if(benchmarkMenuRequested) {
        std::string manifest;
        if(!ReadHiddenLookoutManifest(absolute/"manifest.json",manifest) ||
           manifest!="{\"evidenceClass\":\"isolated opening-menu benchmark; no campaign progress\"}\n")return false;
        for(const auto& item:std::filesystem::directory_iterator(absolute))
            if(item.path().filename()!="manifest.json")return false;
        gOpeningBenchmarkMenuValidation=true;
        gOpeningBenchmarkDetachedCancelValidation=detachedCancelRequested;
        gOpeningBenchmarkDetachedCloseValidation=detachedCloseRequested;
        gCampaignValidationDirectory=absolute;return true;
    }
    if(lookoutRequested && !PrepareHiddenLookoutFixture(absolute)){
        std::cerr<<"Hidden lookout validation requires the unchanged caller-exported single ForestEndpoint autosave and canonical manifest only.\n";return false;
    }
    gBellwetherEarnTavernValidation = earnTavernRequested;
    gBellwetherAnkleRestoreValidation = ankleRestoreRequested;
    gBellwetherExpeditionValidation = expeditionRequested;
    gBellwetherPreparationsValidation = preparationsRequested;
    gBellwetherHearthRestValidation = hearthRestRequested;
    if (earnTavernRequested) gHiddenCampaignFaults.lookoutReadOnly = false;
    gRouteReturnRecovery=returnRequested;
    if((routeRequested || returnRequested) && !PrepareHiddenRouteFixture(absolute)){
        std::cerr<<"Hidden route validation requires the unchanged exported single LowerClaimed manual record and canonical manifest only.\n";return false;
    }
    gLookoutHiddenValidation=lookoutRequested;
    gRouteHiddenValidation=routeRequested || returnRequested;
    gRouteKitTurnStartValidation=routeTurnStartRequested;
    gRouteNaturalWalkValidation=routeNaturalWalkRequested;
    gLookoutInspectKit=inspectKit;gLookoutInspectKitMotion=inspectMotion;
    gLookoutInspectKitTiming=inspectTiming;
    gLookoutInspectKitTurns=inspectTurns;
    gLookoutInspectKitPartials=inspectPartials;
    gLookoutInspectKitPartialsDense=inspectPartialsDense;
    gLookoutInspectKitController=inspectController;
    gCampaignHiddenValidation=true;gCampaignValidationDirectory=absolute;return true;
#else
    std::cerr<<"Hidden campaign validation is Debug-only.\n";return false;
#endif
}
