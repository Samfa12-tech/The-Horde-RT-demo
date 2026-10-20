package com.samfa12.hordelanternrt;

import android.content.Intent;

/** Transient quality only for the explicitly requested Debug noncombat route. */
final class PerformanceProfileRequest {
    static final String EXTRA = "horde.debug.performance_profile";
    private PerformanceProfileRequest() {}
    static GraphicsPreferences.Values resolve(Intent intent, boolean debug, boolean motionRequested,
            boolean hasRtLabIntent, GraphicsPreferences.Values current) {
        if (intent == null || !debug || !motionRequested || hasRtLabIntent || current == null ||
                current.scale != 40 || !"chapter-performance-route".equals(intent.getStringExtra("horde.debug.motion"))) return null;
        try {
            for (String key : intent.getExtras().keySet()) {
                if (key.startsWith("horde.debug.") && !key.equals(EXTRA) &&
                        !key.equals("horde.debug.motion") && !key.equals("horde.debug.motion_id") &&
                        !key.equals("horde.debug.gpu_timing")) return null;
            }
            Object raw = intent.getExtras().get(EXTRA);
            if (!(raw instanceof String)) return null;
            boolean mist=current.mistEnabled; int fire=current.fire, dust=current.dust;
            int water=current.water, shadow=current.shadow;
            switch ((String)raw) {
                case "baseline": break;
                case "mist-off": mist=false; break;
                case "fire-low": fire=GraphicsPreferences.FIRE_LOW; break;
                case "dust-off": dust=0; break;
                case "combined": mist=false; fire=GraphicsPreferences.FIRE_LOW; dust=0;
                    water=1; shadow=GraphicsPreferences.SHADOW_CURRENT; break;
                default: return null;
            }
            return new GraphicsPreferences.Values(current.scale,water,fire,current.cap,
                    current.glassEnabled,shadow,mist,dust);
        } catch (RuntimeException malformed) { return null; }
    }
    /** Waits for the exact ordinary-owner presentation, without starting motion. */
    static final class PresentationGate {
        static final int WAIT = 0, READY = 1, FAILED = 2;
        final GraphicsPreferences.Values values;
        final long serial, requestedAt;
        private long generation;
        PresentationGate(GraphicsPreferences.Values values, long serial, long generation, long requestedAt) {
            this.values=values; this.serial=serial; this.generation=generation; this.requestedAt=requestedAt;
        }
        int poll(long[] snapshot, long currentGeneration, long now) {
            if (values==null || serial<=0 || now<requestedAt || now-requestedAt>=30000) return FAILED;
            // A cold launch binds its first surface once; the deadline never resets.
            if (generation==0 && currentGeneration>0) generation=currentGeneration;
            if (generation>0 && currentGeneration!=generation) return FAILED;
            if (generation<=0) return WAIT;
            if (snapshot==null || snapshot.length!=28) return WAIT;
            if (snapshot[0]>serial || (snapshot[0]==serial && snapshot[14]!=0)) return FAILED;
            return snapshot[0]==serial && snapshot[19]==0 &&
                    GraphicsPreferences.presented(snapshot,generation) &&
                    GraphicsPreferences.matchesRequested(snapshot,values) &&
                    GraphicsPreferences.matchesEffective(snapshot,values) ? READY : WAIT;
        }
    }

}
