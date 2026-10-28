package com.samfa12.hordelanternrt;

public final class ProbeBridge {
    static {
        System.loadLibrary("horde_rt_probe_android");
    }

    private ProbeBridge() {}

    /** Debug-only cold surface diagnostic; no gameplay or campaign authority. */
    public static native boolean configureEnvironmentSurvey(String requestId, boolean waterDisabled,
            int scale, int water, int fire, int cap, boolean glass, int shadow, boolean mist, int dust);
    public static native boolean configureKeeperMistFixture(String requestId,
            int scale, int water, int fire, int cap, boolean glass, int shadow, boolean mist, int dust);
    /** Explicit request identity prevents cancelled diagnostic starts falling into gameplay. */
    public static native long startEnvironmentSurveySurface(android.view.Surface surface,
            String baseDirectory, String requestId);
    public static native void stopEnvironmentSurveySurface(long generation);
    /** First element: 0 idle, 1 armed/running, 2 complete, 3 failed/cancelled. */
    public static native long[] getEnvironmentSurveyStatus();
    public static native void cancelEnvironmentSurvey();

    public static native String getTextReport();
    public static native String getJsonReport();
    /** Reset owned evidence before each Java asset attempt; no probe or renderer access. */
    public static native long beginCompatibilityStartupAttempt();
    /** Only the current pre-surface asset owner may publish its observed failure. */
    public static native boolean failCompatibilityAssetPreparation(long attemptToken);
    /** Owned startup evidence revision; zero means unavailable/stale, never renderer readiness. */
    public static native long getCompatibilityReportRevision();
    /** Local review/export only: status0 then strict UTF-8 JSON. No probe or renderer access. */
    public static native byte[] prepareCompatibilityReport(long expectedRevision, String reportId,
            String capturedAtUtc, boolean localConsent, String rawModel, String os);
    // One status byte (0=ready), then strict UTF-8 JSON/error text. No modified UTF-8.
    public static native byte[] preparePlaytestReport(String reportId, String capturedAtUtc,
            int category, int impact, String note, boolean exportConsent,
            boolean includeBasicContext, String rawModel);
    public static native byte[] preparePlaytestSubmission(String reportId, String capturedAtUtc,
            int category, int impact, String note, boolean submissionConsent,
            boolean includeBasicContext, String rawModel, boolean includeScreenshot,
            long captureToken, byte[] screenshotPng);
    // Consent BEFORE render-owner readback. status0 + LE width,height + RGBA8;
    // status1=pending,2=unavailable. This reads only the presented game RT target.
    public static native long requestPlaytestCapture(boolean screenshotConsent);
    public static native byte[] takePlaytestCapture(long captureToken);
    public static native void cancelPlaytestCapture(long captureToken);
    public static native String getDeveloperOverlayText();
    public static native byte[] getGitHubReleaseRequestContract();
    public static native byte[] evaluateGitHubReleaseUpdate(String installedVersion,
                                                             int httpStatus,
                                                             byte[] responseBodyUtf8);
    public static native boolean writeReports(String baseDirectory);

    /** Returns an accepted request token, not renderer readiness; zero is failure. */
    public static native long startDiagnosticSurface(android.view.Surface surface, String baseDirectory);
    public static native void stopDiagnosticSurface(long generation);
    /** Suspend owner work on a still-valid surface; Stop still retires its resources. */
    public static native boolean setDiagnosticSurfaceSuspended(long generation, boolean suspended);
    public static native int getSurfaceRuntimeState(long generation);
    public static native void setViewControls(float yaw, float pitch, float torchLightStrength, float moveStrafe, float moveForward, boolean crouchHeld, boolean analogueMovement);
    public static native void requestToggleCrouch();
    public static native void clearCrouchIntent();
    public static native void setRunHeld(boolean held);
    public static native void requestToggleRun();
    public static native void clearRunIntent();
    public static native void requestAttack();
    public static native void requestParry();
    /** Coherent hold publication; only false-to-true opens a fresh parry window. */
    public static native void setGuardHeld(boolean held);
    public static native void requestDodge();
    public static native void setCombatTeachingOptions(boolean enabled, boolean slowdown);
    public static native void requestCombatTeachingSkip();
    public static native void requestCombatTeachingReplay();
    /** Coherent immutable chapter dialogue publication: line, generation, elapsed/duration ms, paused. */
    public static native long[] getChapterDialogueState();
    /** Coherent lookout availability: schema, surface generation, world generation, available. */
    public static native long[] getLookoutObjectiveState(long surfaceGeneration);
    /** Speaker and exact catalog text from the same render-owner publication. */
    public static native String getChapterDialogueText(long generation);
    /** Packed Float left/right planar gains from the shared spatial-audio model. */
    public static native long getChapterDialogueStereoGains(long generation);
    /** Distinct monotonic gameplay command; never aliases attack/interact. */
    public static native void requestChapterDialogueSkip();
    /** Packed immutable teaching snapshot published by the simulation owner. */
    public static native long getCombatTeachingState();
    /** Current shared envelope only; no Java duck clock or gameplay mutation. */
    public static native float getSfxDialogueBedGain();
    public static native void requestInteract();
    public static native void requestToggleHeldLightPose();
    public static native void requestRouteReset();
    public static native int getPlayerVitality();
    // Low 32 bits: current vitality; high 32 bits: maximum, from one simulation snapshot.
    public static native long getPlayerVitalityState();
    public static native int getPlayerLifePhase();
    public static native int getFinaleEndingPhase();
    public static native float getKeeperRevealTitleOpacity();
    public static native int getContextualControlState();
    /** Owner-published: earned 1.7 recovery uses confirmed campaign state, not a legacy camera. */
    public static native boolean getConfirmedRetryMode();
    /** Version1, eight words; exact current-surface owner state. */
    public static native long[] getHealingFlaskState();
    public static native void requestDrinkFlask();
    public static native void requestRest();
    public static native int retryEncounter();
    public static native void setSimulationPaused(boolean paused);
    public static native long setGraphicsSettings(int scale, int water, int fire, int cap, boolean glass, int shadow, boolean mistEnabled, int dustQuality);
    public static native void beginGraphicsEdit(int scale, int water, int fire, int cap, boolean glass, int shadow, boolean mistEnabled, int dustQuality);
    public static native long applyGraphicsSettings(int scale, int water, int fire, int cap, boolean glass, int shadow, boolean mistEnabled, int dustQuality, long generation);
    public static native long compareGraphicsPreview(int scale, int water, int fire, int cap, boolean glass, int shadow, boolean mistEnabled, int dustQuality, long generation);
    public static native long revertGraphicsSettings(long generation);
    public static native boolean confirmGraphicsSettings(long serial, long generation);
    public static native long advanceGraphicsConfirmation(double seconds, boolean foreground, long generation);
    // Coherent render-owner snapshot: serial, generation, edit state, effective
    // scale/water/fire/cap, internal W/H, output W/H, optics, backend, presented,
    // reason bits, requested scale/water/fire/cap, scene, effective/requested glass.
    // effective/requested shadow appended at22/23; mist24/25; dust26/27. Exactly28 fields; unknown extents stay zero.
    public static native long[] getGraphicsSnapshot();
    public static native void setGraphicsPreview(boolean enabled, boolean paused, boolean motion,
            int camera, boolean resetTimeline, long generation);
    // Entry menu requests are ephemeral. State is a coherent five-word snapshot:
    // generation, profile(Showcase/Preview/Entry), phase, fade permille, presented.
    public static native void setEntryMenu(boolean enabled, boolean sidePage,
            boolean reducedMotion, boolean play, long generation);
    public static native long[] getEntryMenuState();
    /** Publishes one app-private namespace; no save IO runs on the caller thread. */
    public static native boolean configureCampaign(String privateRoot);
    /** Admission only: actions Refresh=1, SaveManual=2, Load=3, Cancel=4, NewGame=5. */
    public static native long requestCampaign(int action, int slot, int kind, long lifecycleGeneration);
    /** Version 1, 34 words (33-word legacy accepted); decoded only by CampaignMenuState. */
    public static native long[] getCampaignState();
    /** Admit the paused neutral Java input/camera rebase; owner completion is asynchronous. */
    public static native boolean acknowledgeCampaignInput(long token, long surfaceGeneration);
    public static native long[] getMenuAmbienceState();
    // epoch,presents/s,loop ms,CPU ms,GPU ms (-1 unavailable),tracked device/host bytes,
    // transition count,sample count, then loop-ms/transition pairs (at most128).
    public static native double[] getGraphicsPreviewPerformance();
    public static native void setRenderScale(float scale);
    public static native void setWaterQuality(int quality);
    public static native void setRtSceneTuning(float waterfallWidthScale,
                                                boolean roofOverrideEnabled, float roofOpen,
                                                boolean dawnOverrideEnabled, float dawnReveal,
                                                float fogDensityScale);
    public static native void setRtLightTuning(int group, float hueDegrees, float intensityScale);
    public static native void setRtFireTuning(float strengthScale, float turbulenceScale, float smokeScale);
    public static native void setRtGlassTuning(boolean visible, float transmission, float ior, float roughness);
    public static native void setRtWorkloadPreset(int preset);
    public static native void resetRtSceneTuning();
    public static native void markRtLabDebugAutomation();
    public static native boolean isRtLabUnlockEligible();
    public static native float getRtGpuFrameTimeMilliseconds();
    public static native long getRtGpuSampleCount();
    public static native int getCurrentRenderScalePercent();
    public static native int getCurrentWaterQuality();
    public static native void setGpuTimingEnabled(boolean enabled);
    public static native void setRequiredRayQueryCompute(boolean required);
    public static native boolean requestActorWorkerExperiment(boolean enabled);
    public static native boolean requestDebugCheckpoint(int checkpointId);
    /** Debug-only live practice request; applied on the simulation owner thread. 1=parry, 2=Keeper. */
    public static native boolean requestDebugCombatPractice(int enemyKind);
    public static native boolean requestDebugCaptureCheckpoint(int checkpointId);
    public static native boolean requestDebugRouteReplay();
    public static native boolean requestDebugMotionEvidence(String scenario, String runId);
    public static native int getDebugMotionEvidenceStatus();
    public static native boolean finishDebugMotionEvidence();
    public static native boolean requestBenchmark();
    // Custom/unranked opening-menu owner. Course completion alone is not Entry restoration.
    public static native long requestOpeningBenchmark(long surfaceGeneration);
    public static native long requestOpeningProvisionalBenchmark(long surfaceGeneration);
    public static native void cancelOpeningBenchmark(long requestSerial, long requestSurfaceGeneration);
    // request, current generation, phase(0..6), result(0/2/3/4 unavailable), Entry presented, origin generation.
    public static native long[] getOpeningBenchmarkState();
    // request, origin, generation, sequence, command(0 none/1 fixed/2 layout), output width/height, phase.
    public static native long[] getOpeningBenchmarkOutputState();
    public static native void ackOpeningBenchmarkOutput(long request,long origin,long generation,long sequence,
            boolean success,int actualWidth,int actualHeight,boolean layoutRestoredOrAspectPreserved);
    // UUID.randomUUID() and Build.MODEL supplied by ordinary UI. Legacy run ID stays empty.
    public static native boolean requestBenchmarkWithSummaryId(String summaryRunUuid, String rawModel);
    public static native String getReadyBenchmarkSummaryRunId();
    // Local preparation only: first status byte then exact bounded UTF8 JSON on Ready(0).
    // Cooling: 0 unknown, 1 none declared, 2 external declared; hardware defaults off.
    public static native byte[] prepareBenchmarkSummaryReport(String expectedRunUuid, String reportUuid,
            String capturedAtUtc, boolean consentToPrepare, boolean includeBasicHardware, int declaredCooling);
    public static native boolean requestBenchmarkWithId(String runId);
    public static native boolean requestBenchmarkWithIdAndWorkload(String runId, String workload);
    public static native boolean requestBenchmarkMotionValidation(String runId, String scenario);
    public static native void finishBenchmarkMotionValidation();
    public static native void cancelBenchmark();
    public static native int getBenchmarkStatus();
    public static native String getBenchmarkProgress();
    public static native String getBenchmarkReport();
    public static native int getRuntimeState();
    public static native long getWaterfallStereoGains();
    public static native long[] drainPlatformEvents();
}
