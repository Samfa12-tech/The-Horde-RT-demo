// Present the authored repeat visit through one native scene. The shared
// checkpoint owns movement, interaction and dialogue ticks throughout.
bool RunBellwetherFurnitureRouteCapture(VulkanSurfaceContext& context, const std::filesystem::path& output,
    const VkClearColorValue& clear, std::string& error)
{
    using Scene=horde::vulkan::raytracing::PresentableTinyRtScene;
    using Tier=horde::scene::BellwetherFurnitureDetail;
    using horde::telemetry::RtPresentationOutcome;
    constexpr std::array expectedTiers{Tier::Far,Tier::Mid,Tier::Near,Tier::Mid,Tier::Near,Tier::Mid,Tier::Near};
    constexpr unsigned sampleTicks=12;
    const auto* checkpoint=horde::gameplay::FindDevelopmentCheckpoint("bellwether-repeat-visit");
    if(!checkpoint || !context.requestedGlassEnabled) {
        error="Moving Bellwether evidence requires the authored repeat route with Glass On.";return false;
    }
    std::ofstream frames(output/"furniture-route.csv");
    if(!frames) {error="Could not open the Bellwether route receipt.";return false;}
    frames<<"tick,submission,epoch,static_generation,x,y,z,yaw,door_opening,in_room,stage,tier,pane\n";
    const auto start=std::chrono::steady_clock::now();
    context.chapterRoundTripCaptureActive=true;
    context.simulationPaused=false;
    context.sfxVolumePercent=context.voiceVolumePercent=context.musicVolumePercent=0;
    (void)context.rtFrameEvidence.SetPaused(false);
    horde::vulkan::raytracing::PublishDevelopmentWorldReadiness(context.simulation,context.rtScene);
    const auto sceneEpoch=context.rtFrameEvidence.PublishedStateByValue().sceneEpoch;
    const auto staticGeneration=context.rtScene.StaticGeometryGeneration();
    const auto tlas=context.rtScene.CaptureResourceHandles().topLevelAccelerationStructures;
    unsigned frameCount=0,transitionIndex=0,entries=0,exits=0,captures=0;
    std::uint64_t lastTick=0,lastSerial=0;
    Tier tier=Tier::Near;
    bool inRoom=false;
    const auto presentStep=[&](bool finish) {
        const auto& state=context.simulation.Snapshot();
        if(frameCount && state.tickIndex==lastTick)return true;
        if(std::chrono::steady_clock::now()-start>std::chrono::seconds(180)) {
            error="Bellwether route exceeded its 180-second validation deadline.";return false;
        }
        const auto next=horde::scene::SelectBellwetherFurnitureDetail(state.playerX,state.playerZ,tier);
        const bool room=state.playerGrounded && horde::gameplay::traversal::IsOnBellwetherTavernRoomFloor(
            state.playerX,state.playerSupportWorldY,state.playerZ);
        const bool changedTier=!frameCount || next!=tier;
        const bool crossed=room!=inRoom;
        if(!finish && !changedTier && !crossed && state.tickIndex-lastTick<sampleTicks)return true;
        const auto tick=state.tickIndex;
        const auto frameSlot=context.currentFrame;
        bool presented=false;
        horde::telemetry::RtSubmittedFrameIdentity identity{};
        if(!RenderFrame(context,clear,presented) || !presented ||
           context.lastFramePresentation!=RtPresentationOutcome::Presented ||
           !context.rtFrameEvidence.TryGetCommittedIdentity(frameSlot,identity) ||
           identity.frame.simulationTick!=tick || identity.frame.sceneEpoch!=sceneEpoch ||
           identity.submissionSerial<=lastSerial || context.simulation.Snapshot().tickIndex!=tick ||
           context.rtScene.StaticGeometryGeneration()!=staticGeneration) {
            error="Bellwether lost a matched same-scene presentation at tick "+std::to_string(tick)+
                ": "+context.lastRtFrameError;
            return false;
        }
        const bool window=context.rtScene.DebugHasRecordedInstance(Scene::kChapterWindowMetadataIndex);
        if(context.rtScene.DebugRecordedBellwetherFurnitureDetail()!=next || !window ||
           !context.rtScene.DebugHasRecordedInstance(Scene::kTavernDoorMetadataIndex) ||
           ((room || crossed) && next!=Tier::Near) || (crossed && state.tavernDoor.opening<.95f)) {
            error="Bellwether lost its recorded furniture, pane or operable doorway at tick "+std::to_string(tick);
            return false;
        }
        if(changedTier && (transitionIndex==expectedTiers.size() || expectedTiers[transitionIndex++]!=next)) {
            error="Bellwether diverged from the expected approach and return tiers at tick "+std::to_string(tick);
            return false;
        }
        if(crossed)room ? ++entries : ++exits;
        ++frameCount;
        frames<<tick<<','<<identity.submissionSerial<<','<<sceneEpoch<<','<<staticGeneration<<','
            <<state.playerX<<','<<state.playerSupportWorldY<<','<<state.playerZ<<','<<state.playerYawRadians<<','
            <<state.tavernDoor.opening<<','<<room<<','<<entries+exits<<','<<static_cast<unsigned>(next)<<','<<window<<'\n';
        if(changedTier || crossed || finish) {
            if(!CompleteRtEvidenceAfterDeviceIdle(context,vkDeviceWaitIdle(context.device))) {
                error="Bellwether milestone did not complete its submitted GPU work.";return false;
            }
            const auto publication=context.rtFrameEvidence.PublishedStateByValue();
            const auto& complete=publication.completedEvidence;
            if(!publication.hasCompletedEvidence || complete.identity.submitted.frame.simulationTick!=tick ||
               complete.identity.submitted.frame.sceneEpoch!=sceneEpoch ||
               complete.identity.submitted.submissionSerial!=identity.submissionSerial ||
               complete.presentation.outcome!=RtPresentationOutcome::Presented ||
               complete.presentation.lastSuccessfulPresentSubmissionSerial!=identity.submissionSerial) {
                error="Bellwether milestone image lacks matching completed RT presentation.";return false;
            }
            const auto filename=std::to_string(++captures)+"-tick-"+std::to_string(tick)+
                "-lod"+std::to_string(static_cast<unsigned>(next))+".png";
            Scene::StorageImageCapture image;
            if(!context.rtScene.CaptureStorageImage(image,error) || !WriteRgbaPng(output/filename,image,error))return false;
        }
        frames.flush();
        if(!frames){error="Could not write the Bellwether route receipt.";return false;}
        lastTick=tick;lastSerial=identity.submissionSerial;tier=next;inRoom=room;
        return true;
    };
    auto present=[&] {return presentStep(false);};
    const horde::gameplay::DevelopmentCheckpointStepFixedObservation driver{&present,nullptr,nullptr,
        [](void* user) {return (*static_cast<decltype(present)*>(user))();}};
    horde::gameplay::DevelopmentCheckpointStageEvidence evidence;
    if(!horde::gameplay::StageDevelopmentCheckpointSimulation(context.simulation,*checkpoint,&evidence,&driver) ||
       !presentStep(true) || !evidence.bellwetherVisit || evidence.bellwetherVisit->quietReturns!=2 ||
       entries!=3 || exits!=2 || !inRoom || transitionIndex!=expectedTiers.size() ||
       tlas!=context.rtScene.CaptureResourceHandles().topLevelAccelerationStructures) {
        if(error.empty())error="Bellwether route did not finish three arrivals and two quiet returns on one TLAS.";
        return false;
    }
    std::ostringstream receipt;
    receipt<<"{\"complete\":true,\"executionBackend\":\""<<horde::vulkan::ToString(context.rtScene.ExecutionBackend())
        <<"\",\"frames\":"<<frameCount<<",\"captureCount\":"<<captures<<",\"sceneEpoch\":"<<sceneEpoch
        <<",\"staticGeneration\":"<<staticGeneration<<",\"baseSampleTicks\":"<<sampleTicks
        <<",\"tierSelections\":"<<transitionIndex<<",\"entries\":"<<entries<<",\"exits\":"<<exits
        <<",\"quietReturns\":"<<evidence.bellwetherVisit->quietReturns
        <<",\"sameTlas\":true,\"glassEnabled\":true}\n";
    if(!WriteReportFile(output/"furniture-route.json",receipt.str())) {
        error="Could not write the completed Bellwether route receipt.";return false;
    }
    return true;
}
