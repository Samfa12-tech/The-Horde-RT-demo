package com.samfa12.hordelanternrt;

public final class ProbeBridge {
    static {
        System.loadLibrary("horde_rt_probe_android");
    }

    private ProbeBridge() {}

    public static native String getTextReport();
    public static native String getJsonReport();
    public static native boolean writeReports(String baseDirectory);
}