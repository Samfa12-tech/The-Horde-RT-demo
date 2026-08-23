package com.samfa12.hordelanternrt;

public final class ProbeBridge {
    static {
        System.loadLibrary("horde_rt_probe_android");
    }

    private ProbeBridge() {}

    public static native String getTextReport();
    public static native String getJsonReport();
    public static native String getDeveloperOverlayText();
    public static native boolean writeReports(String baseDirectory);

    public static native boolean startDiagnosticSurface(android.view.Surface surface, String baseDirectory);
    public static native void stopDiagnosticSurface();
    public static native void setViewControls(float yaw, float pitch, float lanternStrength, float moveStrafe, float moveForward);
    public static native void requestAttack();
    public static native void requestParry();
    public static native void requestRouteReset();
    public static native int getPlayerVitality();
    public static native int getPlayerLifePhase();
    public static native int getFinaleEndingPhase();
    public static native int retryEncounter();
    public static native void setSimulationPaused(boolean paused);
    public static native void setRenderScale(float scale);
    public static native void setWaterQuality(int quality);
    public static native void setRtSceneTuning(float waterfallWidthScale,
                                                boolean roofOverrideEnabled, float roofOpen,
                                                boolean dawnOverrideEnabled, float dawnReveal,
                                                float fogDensityScale);
    public static native void setRtLightTuning(int group, float hueDegrees, float intensityScale);
    public static native void setRtWorkloadPreset(int preset);
    public static native void resetRtSceneTuning();
    public static native void markRtLabDebugAutomation();
    public static native boolean isRtLabUnlockEligible();
    public static native float getRtGpuFrameTimeMilliseconds();
    public static native long getRtGpuSampleCount();
    public static native int getCurrentRenderScalePercent();
    public static native int getCurrentWaterQuality();
    public static native void setGpuTimingEnabled(boolean enabled);
    public static native boolean requestDebugCheckpoint(int checkpointId);
    public static native boolean requestDebugCaptureCheckpoint(int checkpointId);
    public static native boolean requestDebugRouteReplay();
    public static native boolean requestBenchmark();
    public static native void cancelBenchmark();
    public static native int getBenchmarkStatus();
    public static native String getBenchmarkProgress();
    public static native String getBenchmarkReport();
    public static native int getRuntimeState();
    public static native long getWaterfallStereoGains();
    public static native long[] drainPlatformEvents();
}
