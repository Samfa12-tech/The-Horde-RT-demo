// Explicit Debug request, armed before any Surface. No ordinary lifecycle/input
// synchronisation, campaign service, preference publisher or report replacement.
extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_configureEnvironmentSurvey(
    JNIEnv* env,jclass,jstring requestId,jboolean disabled,jint scale,jint water,jint fire,jint cap,
    jboolean glass,jint shadow,jboolean mist,jint dust)
{
#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
    if(!requestId || env->GetStringUTFLength(requestId)>64)return JNI_FALSE;
    const char* text=env->GetStringUTFChars(requestId,nullptr);if(!text)return JNI_FALSE;
    const std::string id(text);env->ReleaseStringUTFChars(requestId,text);
    const auto settings=GraphicsTuple(scale,water,fire,cap,glass,shadow,mist,dust);
    if(!horde::graphics::ValidGraphicsSettings(settings))return JNI_FALSE;
    std::lock_guard admission(gDebugRequestAdmissionMutex);
    const auto state=ReadDebugRequestAdmissionStateLocked();
    if(gSwapchainRunning.load(std::memory_order_acquire) ||
       gSurfaceRetirementBlocked.load(std::memory_order_acquire) ||
       gCampaignPauseGate.load(std::memory_order_acquire) ||
       state.practicePending || state.checkpointPending || state.fixtureCheckpointActive ||
       state.capturePending || state.replayPending || state.benchmarkPendingOrActive ||
       state.motionPendingOrActive)return JNI_FALSE;
    std::lock_guard lock(gEnvironmentSurveyMutex);
    if(!gEnvironmentSurveyPolicy.Configure(id,disabled==JNI_TRUE))return JNI_FALSE;
    gEnvironmentSurveyGraphics=settings;
    gEnvironmentSurveyRequestedSurface.store(0,std::memory_order_release);
    return JNI_TRUE;
#else
    return JNI_FALSE;
#endif
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_startEnvironmentSurveySurface(
    JNIEnv* env,jclass,jobject surface,jstring baseDirectory,jstring requestId)
{
#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
    if(!surface || !baseDirectory || !requestId || env->GetStringUTFLength(requestId)>64)return 0;
    const char* text=env->GetStringUTFChars(requestId,nullptr);if(!text)return 0;
    const std::string id(text);env->ReleaseStringUTFChars(requestId,text);
    const char* base=env->GetStringUTFChars(baseDirectory,nullptr);if(!base)return 0;
    const std::string reports=BuildReportDirectory(base);env->ReleaseStringUTFChars(baseDirectory,base);
    std::uint64_t serial=0;
    {
        std::lock_guard lock(gEnvironmentSurveyMutex);
        if(gEnvironmentSurveyPolicy.Phase()!=1 || id!=gEnvironmentSurveyPolicy.Id() ||
           gEnvironmentSurveyPolicy.Surface() || gEnvironmentSurveyRequestedSurface.load(std::memory_order_acquire))return 0;
        serial=gEnvironmentSurveyPolicy.Serial();
    }
    const auto unused=[](const std::string& path) {
        struct stat status{};return lstat(path.c_str(),&status)!=0 && errno==ENOENT;
    };
    const auto prefix=reports+"/environment-survey-"+id;
    bool fresh=unused(prefix+".json");
    for(unsigned i=0;fresh && i<horde::platform::android::kEnvironmentSurveyImages;++i)
        fresh=unused(prefix+"-"+std::to_string(i)+".rgba");
    if(!fresh){FailEnvironmentSurvey(serial,"request-output-already-exists-or-unreadable");return 0;}
    std::unique_ptr<ANativeWindow,NativeWindowRelease> window(ANativeWindow_fromSurface(env,surface));
    if(!window){FailEnvironmentSurvey(serial,"missing-native-window");return 0;}
    try {
        (void)SurfaceOwner();
        // Serialize enqueue with cancellation; owner Bind takes this lock only
        // after the mailbox releases its own lock.
        std::lock_guard lock(gEnvironmentSurveyMutex);
        if(gEnvironmentSurveyPolicy.Phase()!=1 || serial!=gEnvironmentSurveyPolicy.Serial() ||
           gEnvironmentSurveyPolicy.Surface() || gEnvironmentSurveyRequestedSurface.load(std::memory_order_acquire))return 0;
        gEnvironmentSurveyPreserveStops.store(true,std::memory_order_release);
        const auto generation=gSurfaceSessions.Start({std::move(window),reports,true,serial});
        gEnvironmentSurveyRequestedSurface.store(generation,std::memory_order_release);
        return static_cast<jlong>(generation); // Accepted, never a presentation claim.
    } catch(...) {FailEnvironmentSurvey(serial,"surface-enqueue-failed");return 0;}
#else
    return 0;
#endif
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_stopEnvironmentSurveySurface(JNIEnv*,jclass,jlong generation)
{
#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
    if(generation<=0)return;
    std::lock_guard lock(gEnvironmentSurveyMutex);
    const auto expected=static_cast<std::uint64_t>(generation);
    if(expected!=gEnvironmentSurveyRequestedSurface.load(std::memory_order_acquire))return;
    gEnvironmentSurveyPolicy.Cancel();
    (void)gSurfaceSessions.Stop(expected);
#endif
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_getEnvironmentSurveyStatus(JNIEnv* env,jclass)
{
    std::array<jlong,3> state{};
#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
    {
        std::lock_guard lock(gEnvironmentSurveyMutex);
        state={static_cast<jlong>(gEnvironmentSurveyPolicy.Phase()),
            static_cast<jlong>(gEnvironmentSurveyPolicy.Frames()),
            static_cast<jlong>(gEnvironmentSurveyPolicy.Surface())};
        // Revocation is immediate; export waits for the owner image write/retire.
        if(state[0]==3 && gEnvironmentSurveyActiveSurface.load(std::memory_order_acquire)!=0)
            state[0]=1;
    }
#endif
    auto result=env->NewLongArray(state.size());
    if(result)env->SetLongArrayRegion(result,0,state.size(),state.data());
    return result;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_ProbeBridge_cancelEnvironmentSurvey(JNIEnv*,jclass)
{
#if defined(HORDE_RT_DEBUG_CHECKPOINTS)
    std::lock_guard lock(gEnvironmentSurveyMutex);gEnvironmentSurveyPolicy.Cancel();
#endif
}
