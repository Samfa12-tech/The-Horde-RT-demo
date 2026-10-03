package com.samfa12.hordelanternrt;

public final class ProbeBridge {
    static {
        System.loadLibrary("horde_rt_probe_android");
    }

    private ProbeBridge() {}

    public static native String getTextReport();
    public static native String getJsonReport();
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
    public static native int getSurfaceRuntimeState(long generation);
    public static native void setViewControls(float yaw, float pitch, float torchLightStrength, float moveStrafe, float moveForward);
    public static native void requestAttack();
    public static native void requestParry();
    public static native void requestInteract();
    public static native void requestToggleHeldLightPose();
    public static native void requestRouteReset();
    public static native int getPlayerVitality();
    public static native int getPlayerLifePhase();
    public static native int getFinaleEndingPhase();
    public static native int getContextualControlState();
    public static native int retryEncounter();
    public static native void setSimulationPaused(boolean paused);
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
    public static native boolean requestDebugCheckpoint(int checkpointId);
    public static native boolean requestDebugCaptureCheckpoint(int checkpointId);
    public static native boolean requestDebugRouteReplay();
    public static native boolean requestBenchmark();
    public static native boolean requestBenchmarkWithId(String runId);
    public static native boolean requestBenchmarkWithIdAndWorkload(String runId, String workload);
    public static native void cancelBenchmark();
    public static native int getBenchmarkStatus();
    public static native String getBenchmarkProgress();
    public static native String getBenchmarkReport();
    public static native int getRuntimeState();
    public static native long getWaterfallStereoGains();
    public static native long[] drainPlatformEvents();
}
