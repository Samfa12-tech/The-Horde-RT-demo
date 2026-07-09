package com.samfa12.hordelanternrt;

public final class ProbeBridge {
    static {
        System.loadLibrary("horde_rt_probe_android");
    }

    private ProbeBridge() {}

    public static native String getTextReport();
    public static native String getJsonReport();
    public static native boolean writeReports(String baseDirectory);

    public static native boolean startDiagnosticSurface(android.view.Surface surface, String baseDirectory);
    public static native void stopDiagnosticSurface();
    public static native void setViewControls(float yaw, float pitch, float lanternStrength, float moveStrafe, float moveForward);
}
