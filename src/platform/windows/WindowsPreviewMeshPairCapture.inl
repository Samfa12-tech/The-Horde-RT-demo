// Debug-only, explicit hidden mesh-pair fixture using the existing renderer,
// presentation/fence evidence and storage-image readback. No gameplay or audio.
int RunPreviewMeshPairCapture(VulkanSurfaceContext& context,
    horde::vulkan::DeviceCapabilities& capabilities,const std::filesystem::path& directory)
{
    using Mode=horde::platform::windows::PreviewMeshPairCaptureMode;
    const bool shared=context.previewMeshPair==Mode::Shared;
    std::string diagnostic,executableHash,masterHash;
    std::vector<std::string> records;
    bool sameOwnerRecreationProved=false;
    const auto manifest=[&](bool complete,const std::string& error) {
        std::ostringstream out;out.imbue(std::locale::classic());out<<std::fixed<<std::setprecision(8);
        out<<"{\"schema\":1,\"complete\":"<<(complete?"true":"false")
           <<",\"fixture\":\"Debug hidden quarter-scale pine pair\",\"mode\":\""<<(shared?"shared":"baked")
           <<"\",\"fullSizeForestOrRootAcceptance\":false,\"performanceAdmission\":false,\"buildId\":\""<<JsonEscape(HORDE_RT_BUILD_ID)
           <<"\",\"executableSha256\":\""<<executableHash<<"\",\"masterSha256\":\""<<masterHash
           <<"\",\"shaderBundleIdentity\":\""<<JsonEscape(context.rtScene.SelectedPipelineBundleIdentity())
           <<"\",\"expectedTlasDefinitions\":"<<(shared?9:8)
           <<",\"isolation\":{\"userPreferencesLoaded\":false,\"userPreferencesWritten\":false,\"audioStarted\":false,\"gameplayAdvanced\":false}"
           <<",\"timingScope\":\"unpaced RenderFrame wall time and current completed GPU duration; not scanout FPS or phone evidence\""
           <<",\"allocationScope\":\"tracked Vulkan allocations; UMA host/device flags overlap and are not additive; pair scratch is not total startup peak or residency\""
           <<",\"sameOwnerRecreationProved\":"<<(sameOwnerRecreationProved?"true":"false")
           <<",\"remainingGates\":[\"matched control/candidate visual and timing review\",\"process close readback\",\"unit-scale outdoor proof\",\"Android acceptance\"]"
           <<",\"error\":"<<(error.empty()?"null":"\""+JsonEscape(error)+"\"")<<",\"captures\":[";
        for(std::size_t i=0;i<records.size();++i){if(i)out<<',';out<<records[i];}
        out<<"]}\n";return WriteReportFile(directory/"preview-mesh-pair.json",out.str());
    };
    const auto fail=[&](const std::string& error){manifest(false,error);std::cerr<<"Preview mesh pair: "<<error<<'\n';return 1;};
    if(!gShowcaseCaptureHidden || !context.graphicsPreviewCapture || !context.useRtPath ||
       !context.rtScene.IsReady() || context.rtScene.Profile()!=horde::vulkan::raytracing::RtSceneProfile::GraphicsPreview ||
       context.previewMeshPair==Mode::None || !context.previewMeshPairMaster)
        return fail("Explicit hidden Debug pair and ready GraphicsPreview owner are required.");
    std::array<wchar_t,32768> executable{};const auto length=GetModuleFileNameW(nullptr,executable.data(),static_cast<DWORD>(executable.size()));
    if(!length || length>=executable.size() || !Sha256File(std::filesystem::path(executable.data()),executableHash,diagnostic) ||
       !Sha256File(ResolveAssetRoot()/"models/world/runtime/forest-tree-pair-v1/horde-irregular-pine-v1-lod1.glb",masterHash,diagnostic))
        return fail("Could not hash actual executable/master: "+diagnostic);
    const auto before=context.simulation.Snapshot();
    const auto eventSequence=context.simulation.Events().NextSequence();
    const auto eventCount=context.simulation.Events().Size();
    context.graphicsPreviewDelta=0.;
    const auto clear=ClearColorForMode(capabilities.rtMode);
    const auto stage=[&] {
        return horde::platform::windows::StageGraphicsPreviewCapturePose(context.graphicsPreview,
            {"pair",horde::graphics::GraphicsPreviewCamera::Overview,120u,false,true});
    };
    if(!stage())return fail("Could not freeze deterministic preview timeline.");
    const auto capture=[&](const char* name,horde::graphics::PreviewCameraPose camera) {
        context.previewMeshPairCamera=camera;std::vector<double> timings;unsigned retries=0;
        for(int i=0;i<kCaptureSettlingFrames;++i) {
            std::vector<double> replacements;
            if(!ApplyPendingSceneReplacement(context,capabilities,replacements) ||
               !ApplyPendingOutputResize(context,capabilities,replacements))return false;
            bool presented=false;const auto start=std::chrono::steady_clock::now();
            if(!RenderFrame(context,clear,presented))return false;
            if(context.lastFramePresentation==horde::telemetry::RtPresentationOutcome::PresentedNeedsRecreate ||
               context.lastFramePresentation==horde::telemetry::RtPresentationOutcome::NotPresentedNeedsRecreate) {
                if(++retries>8)return false;i=-1;timings.clear();continue;
            }
            if(!presented || context.lastFramePresentation!=horde::telemetry::RtPresentationOutcome::Presented)return false;
            timings.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
            FinishGraphicsFrame(context,presented);capabilities.rtScene.presented=true;
        }
        const auto idle=vkDeviceWaitIdle(context.device);
        if(idle!=VK_SUCCESS || !CompleteRtEvidenceAfterDeviceIdle(context,idle))return false;
        const auto publication=context.rtFrameEvidence.PublishedStateByValue();
        if(!CurrentCompletedGraphicsPreviewPresent(publication) ||
           publication.completedEvidence.identity.submitted.frame.simulationTick!=120u)return false;
        const auto pair=context.rtScene.DiagnosticPreviewMeshPair();
        const auto resources=context.rtScene.ResourceInventory();
        if(!pair.active || pair.sharedInstances!=shared || !pair.definitionsValid || pair.primitiveCount!=3u ||
           pair.physicalCount!=(shared?2u:1u) || resources.tlasInstanceCount!=(shared?9u:8u) ||
           !pair.referencesOwnedBlas[0] || (shared && !pair.referencesOwnedBlas[1]))return false;
        horde::vulkan::raytracing::PresentableTinyRtScene::StorageImageCapture image;
        if(!context.rtScene.CaptureStorageImage(image,diagnostic))return false;
        const auto filename=std::string(name)+".png";std::string pngHash;
        if(!WriteRgbaPng(directory/filename,image,diagnostic) || !Sha256File(directory/filename,pngHash,diagnostic) ||
           image.width!=context.rtScene.DispatchExtent().width || image.height!=context.rtScene.DispatchExtent().height)return false;
        const auto& complete=publication.completedEvidence;
        const auto graphics=CurrentGraphicsSettings(context);
        std::ostringstream out;out.imbue(std::locale::classic());out<<std::fixed<<std::setprecision(8);
        out<<"{\"name\":\""<<name<<"\",\"file\":\""<<filename<<"\",\"pngSha256\":\""<<pngHash
           <<"\",\"internalWidth\":"<<image.width<<",\"internalHeight\":"<<image.height
           <<",\"outputWidth\":"<<context.swapchainExtent.width<<",\"outputHeight\":"<<context.swapchainExtent.height
           <<",\"renderScalePercent\":"<<graphics.renderScalePercent<<",\"outputExposure\":"<<context.outputExposure
           <<",\"glassEnabled\":"<<(graphics.glassEnabled?"true":"false")
           <<",\"mistEnabled\":"<<(graphics.mistEnabled?"true":"false")
           <<",\"shadowQuality\":"<<static_cast<unsigned>(graphics.shadowQuality)
           <<",\"waterQuality\":"<<static_cast<unsigned>(graphics.waterQuality)
           <<",\"fireDetail\":"<<static_cast<unsigned>(graphics.fireDetail)
           <<",\"dustQuality\":"<<static_cast<unsigned>(graphics.dustQuality)
           <<",\"quality\":\""<<context.rtScene.SelectedDielectricQualityName()
           <<"\",\"backend\":\""<<(context.rtScene.ExecutionBackend()==horde::vulkan::RtExecutionBackend::RayTracingPipeline?"RayTracingPipeline":"RayQueryCompute")
           <<"\",\"camera\":["<<camera.x<<','<<camera.z<<','<<camera.yaw<<','<<camera.pitch
           <<"],\"tick\":120,\"sceneEpoch\":"<<publication.sceneEpoch
           <<",\"measurementGeneration\":"<<publication.measurementGeneration
           <<",\"submissionSerial\":"<<complete.identity.submitted.submissionSerial
           <<",\"completionSerial\":"<<complete.identity.completionSerial
           <<",\"metadataRow\":3,\"primitiveCount\":"<<pair.primitiveCount
           <<",\"pairVertexPayloadBytes\":"<<pair.vertexBytes<<",\"pairIndexPayloadBytes\":"<<pair.indexBytes
           <<",\"pairBlasAllocationBytes\":"<<pair.blasBytes
           <<",\"pairBuildScratchAllocationBytes\":"<<pair.buildScratchAllocationBytes
           <<",\"pairBuildWallMs\":"<<pair.buildMilliseconds<<",\"commonQuarterScalePlacementsColumnMajor\":[";
        for(unsigned i=0;i<2;++i){if(i)out<<',';out<<'[';for(unsigned k=0;k<16;++k){if(k)out<<',';out<<pair.authoredTransforms[i][k];}out<<']';}
        out<<"],\"physicalInstances\":[";
        for(unsigned i=0;i<pair.physicalCount;++i) {
            if(i)out<<',';out<<"{\"slot\":"<<7+i<<",\"customIndex\":3,\"referencesSingleOwnedBlas\":true,\"transform\":[";
            for(unsigned row=0;row<3;++row)for(unsigned col=0;col<4;++col){if(row||col)out<<',';out<<pair.transforms[i].matrix[row][col];}
            out<<"]}";
        }
        out<<"],\"materialTextureLayers\":[";
        for(unsigned i=0;i<3;++i){if(i)out<<',';out<<'[';for(unsigned k=0;k<4;++k){if(k)out<<',';out<<pair.textureLayers[i][k];}out<<']';}
        out<<"],\"resources\":{\"blasCount\":"<<resources.bottomLevelAccelerationStructureCount
           <<",\"tlasCount\":"<<resources.topLevelAccelerationStructureCount<<",\"tlasDefinitions\":"<<resources.tlasInstanceCount
           <<",\"buffers\":"<<resources.bufferCount<<",\"allocations\":"<<resources.memoryAllocationCount
           <<",\"deviceLocalBytes\":"<<resources.deviceLocalBytes<<",\"hostVisibleBytes\":"<<resources.hostVisibleBytes
           <<"},\"renderCallSamples\":"<<timings.size()<<",\"renderCallMedianMs\":"<<CaptureMedianMs(timings)
           <<",\"currentCompletedGpuMs\":";
        if(complete.gpu.hasDuration && complete.gpu.completedSubmissionSerial==complete.identity.submitted.submissionSerial)
            out<<double(complete.gpu.durationNanoseconds)/1e6;else out<<"null";
        out<<'}';records.push_back(out.str());std::cout<<"Captured hidden preview pair "<<name<<'\n';return true;
    };
    const horde::graphics::PreviewCameraPose direct{-3.6f,-13.4f,.8f,-.10f};
    if(!capture("direct",direct) || !capture("basis",{-2.5f,-13.8f,.65f,-.27f}) ||
       !capture("mirror",{-1.f,-16.5f,-1.81f,-.10f}))return fail("RT pair capture/ownership gate failed: "+diagnostic);
    const auto tick=context.graphicsPreview.Snapshot().tick;context.graphicsPreview.Advance(.25);
    if(context.graphicsPreview.Snapshot().tick!=tick || !capture("paused-direct",direct))
        return fail("Frozen pause gate failed: "+diagnostic);
    const auto retiredEpoch=context.rtFrameEvidence.PublishedStateByValue().sceneEpoch;
    const auto priorResources=context.rtScene.ResourceInventory();
    // Use the existing successful-idle retirement/recreate path, not a new GPU
    // owner or a manufactured restoration acknowledgement. Keep the five views.
    if(!RecreateSwapchain(context))
        return fail("Existing owner retirement/recreate failed: "+context.lastRtFrameError);
    if(!stage() || !capture("reset-direct",direct))
        return fail("Fresh owner reset/capture gate failed: "+diagnostic);
    const auto freshResources=context.rtScene.ResourceInventory();
    if(context.rtFrameEvidence.PublishedStateByValue().sceneEpoch<=retiredEpoch ||
       freshResources.bufferCount!=priorResources.bufferCount ||
       freshResources.memoryAllocationCount!=priorResources.memoryAllocationCount ||
       freshResources.deviceLocalBytes!=priorResources.deviceLocalBytes ||
       freshResources.hostVisibleBytes!=priorResources.hostVisibleBytes)
        return fail("Recreated pair did not preserve its fresh resource inventory.");
    sameOwnerRecreationProved=true;
    const auto after=context.simulation.Snapshot();
    if(after.tickIndex!=before.tickIndex || after.inputPublicationSequence!=before.inputPublicationSequence ||
       after.retryGeneration!=before.retryGeneration || after.playerX!=before.playerX || after.playerZ!=before.playerZ ||
       after.finaleComplete!=before.finaleComplete || context.simulation.Events().NextSequence()!=eventSequence ||
       context.simulation.Events().Size()!=eventCount)return fail("Diagnostic pair altered gameplay/event state.");
    context.previewMeshPairCamera.reset();
    if(!manifest(true,{}))return fail("Could not write complete pair receipt.");
    std::cout<<"Hidden preview pair complete; visual/performance admission and close/recreate proof remain separate.\n";return 0;
}
