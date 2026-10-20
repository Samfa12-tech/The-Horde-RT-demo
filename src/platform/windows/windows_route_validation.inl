// Included inside the Debug-only hidden campaign validator. This is a scoped
// native input/persistence harness, not a natural victory or physical playtest.
bool HiddenRouteFilesAllowed(const std::filesystem::path& directory,bool after)
{
    unsigned sources=0,workingAuto=0;std::error_code error;
    for(const auto& item:std::filesystem::directory_iterator(directory,error)) {
        if(error || item.is_symlink(error) || error || !item.is_regular_file(error) || error)return false;
        const auto name=item.path().filename().string();
        if(name=="manifest.json" || name=="slot-1-manual.hrd"){++sources;continue;}
        if(gRouteReturnRecovery && name=="slot-1-auto.hrd"){++workingAuto;continue;}
        if(name==".slot-1-manual.hrd.lock" || ((after || gRouteReturnRecovery) && name==".slot-1-auto.hrd.lock") || (after && name==".slot-2-manual.hrd.lock")) {
            if(item.file_size(error)!=0 || error)return false;continue;
        }
        if(after && (name=="slot-1-auto.hrd" || name=="slot-2-manual.hrd" || name==kTextReportFilename ||
            name==kJsonReportFilename || name=="windows-route-validation.json" || name=="route-frames.csv" ||
            name=="route-lower.png" || name=="route-lookout.png" || name=="route-return.png" || name=="route-recovered.png"))continue;
        return false;
    }
    return !error && sources==2 && (!gRouteReturnRecovery || workingAuto==1);
}

bool PrepareHiddenRouteFixture(const std::filesystem::path& directory)
{
    const std::string_view manifest=gRouteReturnRecovery?
        "{\"evidenceClass\":\"recovery of actual R3 lower-return autosave; original post-Keeper seed remains explicit\",\"record\":\"slot-1-auto.hrd\",\"sourceSha256\":\"031ced0a83a136861833ab4e9930ae6ea28c7c6e86d47c61c80ba24e8778472b\"}\n":
        "{\"evidenceClass\":\"seeded post-Keeper lower start; subsequent input-driven progression\",\"record\":\"slot-1-manual.hrd\",\"checkpoint\":\"LowerClaimed\",\"contentRevision\":1,\"hearts\":2}\n";
    if(!HiddenRouteFilesAllowed(directory,false) || !ReadHiddenLookoutManifest(directory/"manifest.json",gHiddenLookout.manifest) ||
        gHiddenLookout.manifest!=manifest || campaign::storage::Read(directory,"slot-1-manual.hrd",gHiddenLookout.source).status!=campaign::storage::Status::Read)return false;
    if(gRouteReturnRecovery) {
        campaign::storage::StoredCheckpoint working;
        const auto& c=gHiddenLookout.source.checkpoint;
        if(!horde::gameplay::validation::IsRecordedR3LowerReturn(gHiddenLookout.source.encoded) ||
            campaign::storage::Read(directory,"slot-1-auto.hrd",working).status!=campaign::storage::Status::Read ||
            working.encoded!=gHiddenLookout.source.encoded || !campaign::Validate(c) || c.schema!=1 || c.contentRevision!=2 ||
            c.id!=campaign::CheckpointId::LowerClaimed || c.vitality!=2 || !c.firstAscentCompleted || !campaign::CompletedForestProgress(c.dialogue))return false;
    } else {
    // Exact exporter value: two hearts, both victories and the claim seeded;
    // ascent, reunion, proof and every forest completion flag remain unset.
    campaign::Checkpoint expected;expected.vitality=2;
    expected.dialogue.grateConsumed=expected.dialogue.rescueConsumed=true;
    std::vector<std::byte> encoded;campaign::codec::Error error{};
    if(!campaign::codec::Encode(expected,encoded,error) || encoded!=gHiddenLookout.source.encoded)return false;
    }
    gHiddenLookout.started=std::chrono::steady_clock::now();
    gHiddenLookout.initialForeground=gHiddenLookout.previousForeground=GetForegroundWindow();
    return ObserveHiddenLookoutForeground(nullptr,"route-fixture-admitted");
}

int RunHiddenRouteValidation(VulkanSurfaceContext& ctx)
{
    using namespace horde::gameplay;
    using namespace horde::gameplay::validation;
    unsigned checks=0,failures=0,frames=0;
    std::ostringstream milestones;bool hasMilestone=false;
    auto& receipt=gHiddenLookout.receipt;
    receipt<<"{\"schema\":1,\"evidenceClass\":\""<<(gRouteReturnRecovery?
        "hidden native RT recovery and repeat ascent from actual R3 lower-return autosave; original post-Keeper seed remains explicit":
        "hidden native RT input-driven continuation from seeded post-Keeper lower checkpoint; not natural victory or physical play")<<"\",\"checks\":[";
    const auto check=[&](bool ok,const char* name){
        if(checks++)receipt<<',';if(!ok)++failures;
        receipt<<"{\"name\":\""<<name<<"\",\"pass\":"<<(ok?"true":"false")<<'}';
        std::cerr<<"HORDE_ROUTE_CHECK "<<name<<" pass="<<ok<<'\n';return ok;
    };
    const auto safe=[&](const char* stage,bool retain=true){
        return ObserveHiddenLookoutForeground(ctx.windowHandle,stage,retain) && !ctx.campaignRetirementUnproved &&
            ctx.campaignPhase!=CampaignMenu::Phase::Failed &&
            std::chrono::steady_clock::now()-gHiddenLookout.started<std::chrono::seconds(360);
    };
    const auto click=[&](int id){
        const auto control=GetDlgItem(ctx.windowHandle,id);
        if(!control || !IsWindowEnabled(control) || !(GetWindowLongPtrA(control,GWL_STYLE)&WS_VISIBLE) || !safe("route-before-command"))return false;
        SendMessageA(ctx.windowHandle,WM_COMMAND,MAKEWPARAM(id,BN_CLICKED),reinterpret_cast<LPARAM>(control));
        return safe("route-after-command");
    };
    // One owner advances four ordinary 60 Hz inputs per native rendered frame.
    // The normal platform controls must not advance this snapshot a second time.
    struct Restore {VulkanSurfaceContext& c;bool active;~Restore(){c.chapterRoundTripCaptureActive=active;}} restore{ctx,ctx.chapterRoundTripCaptureActive};
    ctx.chapterRoundTripCaptureActive=true;
    std::ofstream rows(gCampaignValidationDirectory/"route-frames.csv");
    rows<<"frame,tick,world_generation,x,y,z,grounded,modern_rules,rope_phase,exterior,stowed,proof,line,kit_grounded,kit_waypoint,boundary,boundary_revision,operation_revision,outcome,kit_x,kit_z,kit_wait,forest_night,forest_waystone,forest_clue,forest_village,hearts,lantern_high,saw_ascent,saw_pull_up\n";
    const auto render=[&]{
        if(++frames>6000 || !safe("route-before-frame",false))return false;
        const auto tick=ctx.simulation.Snapshot().tickIndex;
        bool presented=false;
        if(!RenderFrame(ctx,VkClearColorValue{{0,0,0,1}},presented))return false;
        FinishGraphicsFrame(ctx,presented);ObserveCampaignPresentation(ctx,presented);
        const auto& s=ctx.simulation.Snapshot();const auto state=ctx.campaign->State();const auto& b=ctx.simulation.LatestCampaignBoundary();
        rows<<frames<<','<<s.tickIndex<<','<<s.worldRoute.generation<<','<<s.playerX<<','<<s.playerSupportWorldY<<','<<s.playerZ
            <<','<<s.playerGrounded<<','<<s.modernCombatRules<<','<<unsigned(s.rescue.phase)<<','<<s.rescue.exteriorSide
            <<','<<s.rescue.equipmentStowed<<','<<s.chapterDialogue.proofAccepted<<','<<unsigned(s.chapterDialogue.line)
            <<','<<s.chapterCompanion.grounded<<','<<s.chapterCompanion.waypoint<<','<<(b?unsigned(b->id):0)
            <<','<<ctx.simulation.CampaignBoundaryRevision()<<','<<state.operationRevision<<','<<unsigned(state.outcome)
            <<','<<s.chapterCompanion.x<<','<<s.chapterCompanion.z<<','<<unsigned(s.chapterCompanion.wait)
            <<','<<s.chapterDialogue.forestNightConsumed<<','<<s.chapterDialogue.forestWaystoneConsumed
            <<','<<s.chapterDialogue.forestClueConsumed<<','<<s.chapterDialogue.forestVillageConsumed<<','<<s.playerVitals.vitality
            <<','<<(s.interaction.heldLightPose==interactions::HeldLightPose::High)<<','<<s.rescue.sawAscent<<','<<s.rescue.sawPullUp<<'\n';
        if(frames%120==0)rows.flush();
        return presented && ctx.lastFramePresentation==horde::telemetry::RtPresentationOutcome::Presented &&
            s.tickIndex==tick && safe("route-after-frame",false);
    };
    const auto until=[&](auto done,bool framesNeeded){
        do {if(!safe("route-owner-pump",false))return false;PumpCampaign(ctx);
            if(ctx.campaignPhase==CampaignMenu::Phase::Failed)return false;
            if(done())return true;
            if(framesNeeded){if(!render())return false;}else std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }while(safe("route-wait",false));return false;
    };
    const auto catalogue=[&]{for(const auto& e:ctx.campaign->Catalogue())if(e.status==campaign::CatalogueStatus::Checking)return false;return true;};
    const auto loadComplete=[&]{return ctx.campaignBarrier.State().phase==campaign::CampaignLoadBarrier::Phase::Complete;};
    const auto step=[&](simulation::InputSnapshot input){
        if(input.hasAuthoritativePlayerPose || input.damageEnabled || input.commands.attack || input.commands.parry || input.commands.dodge)return false;
        input.paused=false;ctx.simulationInput=input;
        ctx.inputPublicationSequence=std::max(ctx.inputPublicationSequence,ctx.simulation.Snapshot().inputPublicationSequence)+1;
        ctx.simulation.StepFixed(input,1.f/60.f,ctx.inputPublicationSequence);
        PumpCampaign(ctx);return ctx.simulation.Snapshot().playerAlive && !ctx.campaignRetirementUnproved &&
            ctx.campaignPhase!=CampaignMenu::Phase::Failed;
    };
    const auto neutral=[&]{auto i=ctx.simulationInput;i.moveForward=i.moveStrafe=0;i.runHeld=i.crouchHeld=false;
        i.damageEnabled=i.tutorialEnabled=false;i.hasAuthoritativePlayerPose=false;return i;};
    const auto capture=[&](const char* name){
        if(!safe("route-before-readback") || !DrainCampaignResources(ctx))return false;
        const auto p=ctx.rtFrameEvidence.PublishedStateByValue();const auto& e=p.completedEvidence;
        std::string json;horde::telemetry::RtEvidenceValidationError validation{};
        if(!p.presented || !p.hasCompletedEvidence || e.identity.submitted.frame.sceneEpoch!=p.sceneEpoch ||
            e.identity.submitted.frame.simulationTick!=ctx.simulation.Snapshot().tickIndex ||
            e.presentation.outcome!=horde::telemetry::RtPresentationOutcome::Presented ||
            e.identity.submitted.submissionSerial!=e.presentation.lastSuccessfulPresentSubmissionSerial ||
            !horde::telemetry::SerializeRtPerformanceEvidenceJson(e,json,validation))return false;
        std::string error;horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
        if(!ctx.rtScene.CaptureStorageImage(image,error) || !WriteRgbaPng(gCampaignValidationDirectory/name,image,error))return false;
        if(hasMilestone)milestones<<',';hasMilestone=true;
        milestones<<'\"'<<name<<"\":"<<json;return safe("route-after-readback");
    };
    bool proceed=check(ctx.campaign && ctx.campaignMenu && ctx.campaignIsolated &&
        (GetWindowLongPtrA(ctx.windowHandle,GWL_EXSTYLE)&(WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW))==(WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW) && safe("route-entered"),
        "isolated exported seed and hidden nonactivating native owner");
    if(proceed){ApplyOverlayState(ctx);proceed=check(click(kEntryMoreButtonId) && click(kCampaignLoadButtonId) && until(catalogue,false) &&
        click(CampaignMenu::FirstChoiceId+(gRouteReturnRecovery?1:0)) && until(loadComplete,true),"admitted record loads through ordinary native commands input rebase and fresh RT barrier");}
    if(gRouteReturnRecovery) {
        const auto& original=gHiddenLookout.source.checkpoint;
        const auto coherent=[&](bool upper){const auto& s=ctx.simulation.Snapshot();
            return s.modernCombatRules && s.playerAlive && s.playerVitals.vitality==original.vitality && s.playerGrounded &&
                s.rescue.exteriorSide==upper && !s.rescue.equipmentStowed && s.rescue.firstAscentCompleted &&
                s.rescue.claimCount==1 && s.rescue.deploymentCount==1 && s.rescue.ropeReady &&
                s.chapterDialogue.proofAccepted && !s.chapterDialogue.waitingForRaise && s.chapterDialogue.line==dialogue::Line::None &&
                s.chapterDialogue.forestNightConsumed && s.chapterDialogue.forestWaystoneConsumed &&
                s.chapterDialogue.forestClueConsumed && s.chapterDialogue.forestVillageConsumed &&
                s.chapterCompanion.grounded && s.chapterCompanion.waypoint==simulation::kWorldRoutePoints.size() &&
                (s.heldItems[1].parentMode==items::HeldItemParentMode::BodyStow)==original.swordStowed;};
        const auto originalPose=[&]{return (ctx.simulation.Snapshot().interaction.heldLightPose==interactions::HeldLightPose::High)==original.lanternHigh;};
        if(proceed)proceed=check(coherent(false) && originalPose() && ctx.campaignPause && ctx.simulationPaused &&
            capture("route-lower.png") && click(CampaignMenu::ResumeId),"actual lower-return auto restores saved health held pose completed story grounded Kit and single ownership");
        PumpCampaign(ctx);
        const auto initialOperation=ctx.campaign->State().operationRevision;
        for(unsigned tick=0;tick<120 && proceed;++tick){proceed=step(neutral());if(tick%4==3 && proceed)proceed=render();}
        if(proceed)proceed=check(coherent(false) && originalPose() && ctx.campaign->State().operationRevision==initialOperation,
            "resumed lower return remains quiet and does not heal or rewrite the loaded checkpoint");
        bool issued=false,stowed=false,ascent=false,pullUp=false;unsigned ticks=0;
        while(proceed && ticks<1800 && !(ctx.simulation.Snapshot().rescue.exteriorSide && !ctx.simulation.Snapshot().rescue.equipmentStowed)) {
            for(unsigned n=0;n<4;++n){auto input=neutral();
                if(!issued && ctx.simulation.Snapshot().rescuePrompt==traversal::RescuePrompt::Climb){++input.commands.interact;issued=true;}
                proceed=step(input);++ticks;const auto& s=ctx.simulation.Snapshot();
                stowed|=s.rescue.equipmentStowed;ascent|=s.rescue.phase==traversal::Phase::Ascent;pullUp|=s.rescue.phase==traversal::Phase::PullUp;
                if(!proceed)break;
            }
            if(proceed)proceed=render();
        }
        if(proceed)proceed=check(issued && stowed && ascent && pullUp && coherent(true) && capture("route-return.png"),
            "ordinary repeated climb traverses stow ascent pull-up and grounded upper release without replay or duplicate ownership");
        if(proceed)proceed=check(originalPose(),"repeat ascent restores the prior saved lantern configuration after the already completed reunion");
        for(unsigned tick=0;tick<120 && proceed;++tick){proceed=step(neutral());if(tick%4==3 && proceed)proceed=render();}
        campaign::storage::StoredCheckpoint upper;
        if(proceed)proceed=check(until([&]{return ctx.campaign->State().outcome!=campaign::PlayOutcome::Pending;},false) &&
            ctx.campaign->State().outcome==campaign::PlayOutcome::Succeeded &&
            campaign::storage::Read(gCampaignValidationDirectory,"slot-1-auto.hrd",upper).status==campaign::storage::Status::Read &&
            upper.checkpoint.id==campaign::CheckpointId::SafeExit && upper.checkpoint.contentRevision==2 &&
            upper.checkpoint.vitality==original.vitality && upper.checkpoint.lanternHigh==original.lanternHigh &&
            campaign::CompletedForestProgress(upper.checkpoint.dialogue) && upper.encoded!=gHiddenLookout.source.encoded,
            "actual repeat ascent durably replaces only working auto with coherent upper revision two and unchanged health equipment story");
        if(proceed){const auto generation=ctx.simulation.Snapshot().worldRoute.generation;
            ShowPauseMenu(ctx,true);proceed=click(kCampaignLoadButtonId) && until(catalogue,false) && click(CampaignMenu::FirstChoiceId+1) && until(loadComplete,true);
            proceed=check(proceed && coherent(true) && originalPose() && ctx.simulation.Snapshot().worldRoute.generation>generation &&
                capture("route-recovered.png") && click(CampaignMenu::ResumeId),"this-run upper autosave recovers through native Load fresh RT and explicit Resume");PumpCampaign(ctx);
        }
        if(proceed){const auto operation=ctx.campaign->State().operationRevision;
            for(unsigned tick=0;tick<120 && proceed;++tick){proceed=step(neutral());if(tick%4==3 && proceed)proceed=render();}
            proceed=check(proceed && coherent(true) && originalPose() && ctx.campaign->State().operationRevision==operation,
                "recovered repeat-ascent result remains quiet without repeated auto save healing or story restart");
        }
        if(!proceed && !failures)check(false,"bounded return recovery stopped before all stages");
        rows.flush();check(bool(rows),"bounded per-frame return recovery receipt written");
        receipt<<"],\"milestones\":{"<<milestones.str()<<"},\"count\":"<<checks<<",\"failures\":"<<failures<<",\"frames\":"<<frames
            <<",\"stagesComplete\":"<<(!failures?"true":"false")<<",\"deathRecoveryTested\":false,\"physicalPlayTested\":false,\"audioListened\":false";
        return failures?1:0;
    }
    if(proceed){const auto& s=ctx.simulation.Snapshot();proceed=check(s.modernCombatRules && s.playerVitals.vitality==2 &&
        !s.rescue.firstAscentCompleted && !s.rescue.exteriorSide && !s.chapterDialogue.proofAccepted && !s.chapterDialogue.forestVillageConsumed &&
        capture("route-lower.png") && click(CampaignMenu::ResumeId),"current rules two hearts and genuinely unset post-seed progression");PumpCampaign(ctx);}
    if(proceed){
        bool issued=false;unsigned ticks=0;
        while(proceed && ticks<1800 && !(ctx.simulation.Snapshot().rescue.exteriorSide && !ctx.simulation.Snapshot().rescue.equipmentStowed)) {
            for(unsigned n=0;n<4;++n){auto input=neutral();
                if(!issued && ctx.simulation.Snapshot().rescuePrompt==traversal::RescuePrompt::Climb){++input.commands.interact;issued=true;}
                proceed=step(input);++ticks;if(!proceed)break;
            }
            if(proceed)proceed=render();
        }
        proceed=check(proceed && issued && ctx.simulation.Snapshot().rescue.firstAscentCompleted &&
            ctx.simulation.Snapshot().rescue.exteriorSide && !ctx.simulation.Snapshot().rescue.equipmentStowed,
            "ordinary context interaction and current fixed steps finish first rope ascent");
    }
    ChapterRoundTripProbe probe;bool endpoint=false,endpointSaved=false;unsigned quietTicks=0;
    if(proceed)proceed=check(probe.Start(ctx.simulation.Snapshot()),"existing input-only route driver admits the actually reached low-lantern upper state");
    while(proceed && probe.Phase()!=ChapterProbePhase::Lower) {
        for(unsigned n=0;n<4 && probe.Phase()!=ChapterProbePhase::Lower;++n){
            auto input=neutral();
            const bool quietOutbound=probe.Phase()==ChapterProbePhase::Outbound && ctx.simulation.Snapshot().chapterDialogue.line!=dialogue::Line::None;
            const auto& boundary=ctx.simulation.LatestCampaignBoundary();
            endpoint=boundary && boundary->id==campaign::CheckpointId::ForestEndpoint;
            const bool waitEndpoint=probe.Phase()==ChapterProbePhase::Lookout && !endpoint;
            if(quietOutbound || waitEndpoint){if(++quietTicks>3000){proceed=false;break;}}
            else {quietTicks=0;input=probe.Next(ctx.simulation.Snapshot());}
            if(probe.Phase()==ChapterProbePhase::Outbound) {
                // Obey the current escort rule. The generic optimization probe
                // runs ahead; here ordinary walking waits for Kit within 3 m.
                const auto& s=ctx.simulation.Snapshot();
                const float dx=s.chapterCompanion.x-s.playerX,dz=s.chapterCompanion.z-s.playerZ;
                input.runHeld=false;
                if(std::hypot(dx,dz)>3.f && dx*std::sin(input.yawRadians)-dz*std::cos(input.yawRadians)<0)
                    input.moveForward=input.moveStrafe=0;
            }
            if(probe.Phase()==ChapterProbePhase::Failed || !step(input)){proceed=false;break;}
            // Stop at the real, quiet endpoint before return replaces its auto.
            if(endpoint && !endpointSaved)break;
        }
        if(proceed)proceed=render();
        if(proceed && endpoint && !endpointSaved){
            campaign::storage::StoredCheckpoint stored;
            proceed=check(until([&]{return ctx.campaign->State().outcome!=campaign::PlayOutcome::Pending;},false) &&
                campaign::storage::Read(gCampaignValidationDirectory,"slot-1-auto.hrd",stored).status==campaign::storage::Status::Read &&
                stored.checkpoint.id==campaign::CheckpointId::ForestEndpoint && stored.checkpoint.vitality==2 &&
                campaign::CompletedForestProgress(stored.checkpoint.dialogue) && capture("route-lookout.png"),
                "actual movement fresh presented raise and quiet Kit story produce durable endpoint autosave");
            if(proceed){ShowPauseMenu(ctx,true);proceed=click(kCampaignSaveButtonId) && until(catalogue,false) &&
                click(CampaignMenu::FirstChoiceId+1) && until([&]{return ctx.campaign->State().outcome!=campaign::PlayOutcome::Pending;},false);
                campaign::storage::StoredCheckpoint manual;
                proceed=check(proceed && ctx.campaign->State().operation==campaign::PlayOperation::ManualSave &&
                    ctx.campaign->State().outcome==campaign::PlayOutcome::Succeeded && ctx.campaign->State().activeSlot==0 &&
                    campaign::storage::Read(gCampaignValidationDirectory,"slot-2-manual.hrd",manual).status==campaign::storage::Status::Read &&
                    manual.encoded==stored.encoded,"ordinary native Save creates endpoint manual in empty slot two without retargeting slot one auto");
                if(proceed){proceed=click(CampaignMenu::BackId);PumpCampaign(ctx);proceed=proceed && click(kResumeButtonId);PumpCampaign(ctx);}
            }
            endpointSaved=proceed;
        }
    }
    if(proceed){const auto& s=ctx.simulation.Snapshot();
        proceed=check(endpointSaved && s.modernCombatRules && s.playerVitals.vitality==2 && !s.rescue.exteriorSide &&
            !s.rescue.equipmentStowed && s.rescue.sawDescent && s.rescue.sawLanding && s.rescue.claimCount==1 && s.rescue.deploymentCount==1 &&
            render() && capture("route-return.png"),"input-only forest return and real descent restore lower support and equipment without duplicate claim");
    }
    if(proceed){
        // Quiesce on the returned safe side, then recover only the endpoint
        // record written by THIS run. Loading retires the old probe generation.
        for(unsigned n=0;n<60 && proceed;++n)proceed=step(neutral());
        proceed=proceed && render() && until([&]{return ctx.campaign->State().outcome!=campaign::PlayOutcome::Pending;},false);
        campaign::storage::StoredCheckpoint returned;
        proceed=check(proceed && campaign::storage::Read(gCampaignValidationDirectory,"slot-1-auto.hrd",returned).status==campaign::storage::Status::Read &&
            returned.checkpoint.id==campaign::CheckpointId::LowerClaimed && returned.checkpoint.contentRevision==2 &&
            campaign::CompletedForestProgress(returned.checkpoint.dialogue),"actual lower return commits revision two autosave retaining completed forest story");
        const auto generation=ctx.simulation.Snapshot().worldRoute.generation;
        if(proceed){ShowPauseMenu(ctx,true);proceed=click(kCampaignLoadButtonId) && until(catalogue,false) &&
            click(CampaignMenu::FirstChoiceId+2) && until(loadComplete,true);
            const auto& s=ctx.simulation.Snapshot();
            proceed=check(proceed && s.worldRoute.generation>generation && s.modernCombatRules && s.playerVitals.vitality==2 &&
                s.playerGrounded && s.rescue.exteriorSide && s.chapterCompanion.grounded &&
                s.chapterDialogue.line==dialogue::Line::None && s.chapterDialogue.forestVillageConsumed &&
                ctx.campaignPause && ctx.simulationPaused && capture("route-recovered.png") && click(CampaignMenu::ResumeId),
                "this-run endpoint manual recovers through native load fresh generation matching RT and explicit Resume");PumpCampaign(ctx);
        }
        if(proceed){const auto operation=ctx.campaign->State().operationRevision;
            for(unsigned n=0;n<120 && proceed;++n){proceed=step(neutral());if(n%4==3 && proceed)proceed=render();}
            proceed=check(proceed && ctx.campaign->State().operationRevision==operation && !ctx.campaignPause && !ctx.simulationPaused &&
                ctx.simulation.Snapshot().chapterDialogue.line==dialogue::Line::None,"resumed endpoint stays quiet without repeated autosave or story restart");
        }
    }
    if(!proceed && !failures)check(false,"bounded route stopped before completing all stages");
    rows.flush();check(bool(rows),"bounded per-frame route receipt written");
    receipt<<"],\"milestones\":{"<<milestones.str()<<"},\"count\":"<<checks<<",\"failures\":"<<failures<<",\"frames\":"<<frames
        <<",\"stagesComplete\":"<<(!failures?"true":"false")<<",\"deathRecoveryTested\":false,\"physicalPlayTested\":false,\"audioListened\":false";
    return failures?1:0;
}

int FinishHiddenRouteValidation(HWND owner,int result)
{
    campaign::storage::StoredCheckpoint source;std::string manifest;
    const bool preserved=HiddenRouteFilesAllowed(gCampaignValidationDirectory,true) &&
        ReadHiddenLookoutManifest(gCampaignValidationDirectory/"manifest.json",manifest) && manifest==gHiddenLookout.manifest &&
        campaign::storage::Read(gCampaignValidationDirectory,"slot-1-manual.hrd",source).status==campaign::storage::Status::Read &&
        source.encoded==gHiddenLookout.source.encoded;
    if(!preserved || !ObserveHiddenLookoutForeground(owner,"route-owner-destroyed") || gHiddenLookout.receipt.str().empty())result=1;
    auto& receipt=gHiddenLookout.receipt;
    if(receipt.str().empty())receipt<<"{\"schema\":1,\"stagesComplete\":false";
    receipt<<",\"seedAndManifestUnchanged\":"<<(preserved?"true":"false")<<",\"foregroundSamples\":["<<gHiddenLookout.foreground.str()
        <<"],\"foregroundSampleCount\":"<<gHiddenLookout.foregroundSamples<<",\"ownerObservedForeground\":"<<(gHiddenLookout.ownerObserved?"true":"false")
        <<",\"ownerShown\":"<<(gHiddenLookout.ownerShown?"true":"false")<<",\"complete\":"<<(result==0?"true":"false")<<"}\n";
    return WriteReportFile(gCampaignValidationDirectory/"windows-route-validation.json",receipt.str())?result:1;
}
