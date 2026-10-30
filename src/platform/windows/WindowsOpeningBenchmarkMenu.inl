// Native menu presentation only. Renderer transitions are consumed by the outer
// owning loop; these helpers never reset a simulation or allocate RT resources.
horde::platform::windows::OpeningBenchmarkEligibility OpeningBenchmarkEligibilityFor(
    const VulkanSurfaceContext& c)
{
    using namespace horde::platform::windows;
    OpeningBenchmarkEligibility e;
    e.entryVisible=c.openingBenchmark && c.controlsEnabled && c.entryMenuVisible &&
        !c.settingsVisible && !c.graphicsVisible && !c.diagnosticsVisible && !c.benchmarkReportVisible &&
        !c.rtLabVisible && !c.openingBenchmarkReturnPending;
    e.entryScene=c.rtScene.Profile()==horde::vulkan::raytracing::RtSceneProfile::EntryMenu;
    e.pendingCampaign=c.campaignCommand.has_value() || c.campaignPreparation.has_value() ||
        c.campaignPreparedScene || c.campaignPause || c.campaignBarrier.HasPendingCommittedLoad() ||
        (c.campaignMenu && c.campaignMenu->Visible()) || c.campaignRetirementUnproved;
    if(c.campaign) {
        const auto state=c.campaign->State();
        e.activeCampaign=state.activeSlot.has_value();
        e.pendingCampaign=e.pendingCampaign || state.outcome==horde::gameplay::campaign::PlayOutcome::Pending ||
            (state.phase!=horde::gameplay::campaign::SessionPhase::Closed &&
             state.phase!=horde::gameplay::campaign::SessionPhase::Idle &&
             state.phase!=horde::gameplay::campaign::SessionPhase::Failed);
    }
    e.graphicsTransition=c.graphicsEdit || c.graphicsCommand || c.sceneProfileDirty ||
        c.glassGeometryDirty || c.renderScaleDirty;
    e.playHandoff=c.entryPlayQueued || c.entryPlayHandoffPending || c.entryLoadingVisible;
    e.benchmarkBusy=c.openingBenchmarkOwner || c.benchmark.IsRunning() || c.benchmarkProfileActive;
    return e;
}

void RefreshOpeningBenchmarkChooser(VulkanSurfaceContext& c)
{
    const bool standard=c.benchmarkChoice==OpeningBenchmarkChoice::WindowsProvisional;
    const bool custom=c.benchmarkChoice==OpeningBenchmarkChoice::Custom;
    SetWindowTextA(GetDlgItem(c.windowHandle,kBenchmarkStandardChoiceId),
        standard?"SELECTED: WINDOWS STANDARD - PROVISIONAL":"WINDOWS STANDARD - PROVISIONAL");
    SetWindowTextA(GetDlgItem(c.windowHandle,kBenchmarkCustomChoiceId),
        custom?"SELECTED: CUSTOM - UNRANKED":"CUSTOM - UNRANKED");
    std::string text="BENCHMARK | LOCAL REPORT ONLY\r\n\r\n"
        "Standard: provisional 1920 x 1080 internal and output, High RT, DRS off. "
        "High water/fire, Current shadows, glass/mist on, Low dust.\r\n"
        "Custom: your current graphics and window size. Both remain unranked.\r\n"
        "The scripted course speeds up or slows down with rendering speed. Normal gameplay does not.\r\n"
        "Your settings are restored afterwards. Esc cancels a run.";
    if(c.fullscreen)text+="\r\nStandard currently requires windowed mode; Custom keeps fullscreen.";
    if(c.openingBenchmark && !c.openingBenchmark->status.empty())
        text+="\r\n\r\n"+c.openingBenchmark->status;
    SetWindowTextA(GetDlgItem(c.windowHandle,kBenchmarkChoiceInfoId),text.c_str());
    EnableWindow(GetDlgItem(c.windowHandle,kBenchmarkChoiceRunId),
        (standard || custom) && !(standard && c.fullscreen) && OpeningBenchmarkEligibilityFor(c).Eligible());
}

void LayoutOpeningBenchmarkChooser(HWND window,int width,int height)
{
    const int inset=ScaleForDpi(window,16),gap=ScaleForDpi(window,8);
    const int available=std::max(1,height-2*inset);
    const int buttonHeight=std::clamp((available-ScaleForDpi(window,140)-4*gap)/4,
        ScaleForDpi(window,28),ScaleForDpi(window,44));
    const int infoHeight=std::max(1,available-4*buttonHeight-4*gap);
    const int panelWidth=std::max(1,std::min(ScaleForDpi(window,620),width-2*inset));
    const int x=(width-panelWidth)/2;
    int y=inset;
    MoveWindow(GetDlgItem(window,kBenchmarkChoiceInfoId),x,y,panelWidth,infoHeight,TRUE);
    y+=infoHeight+gap;
    for(int id:{kBenchmarkStandardChoiceId,kBenchmarkCustomChoiceId,kBenchmarkChoiceRunId,kBenchmarkChoiceBackId}) {
        MoveWindow(GetDlgItem(window,id),x,y,panelWidth,buttonHeight,TRUE);
        y+=buttonHeight+gap;
    }
}

// Preserve monotonically newer identities across complete renderer lifetimes.
// Failure blocks a successor; wrapping a token is never an admission fallback.
bool CarryOpeningBenchmarkSeeds(OpeningBenchmarkTransfer& transfer,const VulkanSurfaceContext& c)
{
    auto seeds=c.rtFrameEvidence.SeedsByValue();
    if(seeds.sceneEpoch==std::numeric_limits<std::uint64_t>::max() ||
       seeds.measurementGeneration==std::numeric_limits<std::uint64_t>::max())return false;
    ++seeds.sceneEpoch; ++seeds.measurementGeneration;
    transfer.seeds=seeds;return true;
}

void TraceOpeningMenuStage(const char* stage)
{
    if(gOpeningBenchmarkMenuValidation)std::cerr<<"HORDE_OPENING_MENU_STAGE "<<stage<<'\n';
}
