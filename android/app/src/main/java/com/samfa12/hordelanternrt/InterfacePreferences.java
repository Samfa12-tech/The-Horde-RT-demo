package com.samfa12.hordelanternrt;
import android.content.SharedPreferences;

/** UI presentation only: no renderer, audio, consent or input mapping keys. */
final class InterfacePreferences {
    static final String COMPACT="ui_compact", SCALE="ui_control_scale", OPACITY="ui_backing_opacity",
            STRONG="ui_stronger_backing", STATUS="ui_routine_rt_status", REDUCED_MOTION="ui_reduced_motion";
    static final class Values {
        final boolean compact, strongerBacking, routineStatus, reducedMotion;
        final int scale, opacity;
        Values(boolean compact, int scale, int opacity, boolean strongerBacking, boolean routineStatus) {
            this(compact, scale, opacity, strongerBacking, routineStatus, false);
        }
        Values(boolean compact, int scale, int opacity, boolean strongerBacking, boolean routineStatus,
                boolean reducedMotion) {
            this.compact=compact; this.scale=scale; this.opacity=opacity;
            this.strongerBacking=strongerBacking; this.routineStatus=routineStatus;
            this.reducedMotion=reducedMotion;
        }
        boolean valid() { return scale>=85 && scale<=110 && opacity>=55 && opacity<=95; }
    }
    static Values defaults() { return new Values(false,100,70,false,false); }
    static boolean flag(SharedPreferences p,String key,boolean fallback) {
        try { return p.getBoolean(key,fallback); } catch(ClassCastException invalid) { return fallback; }
    }
    static Values read(SharedPreferences p) {
        Values v=new Values(flag(p,COMPACT,false),GraphicsPreferences.integer(p,SCALE,100),
                GraphicsPreferences.integer(p,OPACITY,70),flag(p,STRONG,false),flag(p,STATUS,false),
                flag(p,REDUCED_MOTION,false));
        return v.valid()?v:defaults();
    }
    static boolean reducedMotionEnabled(Values values, boolean systemReducedMotion) {
        return values.reducedMotion || systemReducedMotion;
    }
    static boolean save(SharedPreferences p,Values v) {
        if(!v.valid()) return false;
        return p.edit().putBoolean(COMPACT,v.compact).putInt(SCALE,v.scale).putInt(OPACITY,v.opacity)
                .putBoolean(STRONG,v.strongerBacking).putBoolean(STATUS,v.routineStatus)
                .putBoolean(REDUCED_MOTION,v.reducedMotion).commit();
    }
    static boolean reset(SharedPreferences p) {
        return p.edit().putBoolean(COMPACT,false).putInt(SCALE,100).putInt(OPACITY,70)
                .putBoolean(STRONG,false).putBoolean(STATUS,false).putBoolean(REDUCED_MOTION,false)
                .putBoolean("show_hud",true).commit();
    }
    static void saveLive(SharedPreferences p,Values v) {
        if(!v.valid()) throw new IllegalArgumentException("Invalid interface presentation");
        p.edit().putBoolean(COMPACT,v.compact).putInt(SCALE,v.scale).putInt(OPACITY,v.opacity)
                .putBoolean(STRONG,v.strongerBacking).putBoolean(STATUS,v.routineStatus)
                .putBoolean(REDUCED_MOTION,v.reducedMotion).apply();
    }
    private InterfacePreferences() { }
}
