package com.samfa12.hordelanternrt;

import android.content.SharedPreferences;

/** Thin graphics-only storage adapter. GPU application belongs to the shared native owner. */
final class GraphicsPreferences {
    static final String PENDING = "graphics_pending";
    static final String FIRE = "fire_detail";
    static final String CAP = "graphics_preview_cap";
    static final class Values {
        final int scale, water, fire, cap;
        Values(int scale, int water, int fire, int cap) {
            this.scale = scale; this.water = water; this.fire = fire; this.cap = cap;
        }
        boolean valid() { return scale >= 50 && scale <= 100 && water >= 0 && water <= 2 &&
                fire >= 0 && fire <= 1 && cap >= 15 && cap <= 60; }
        boolean same(Values other) { return other != null && scale == other.scale && water == other.water &&
                fire == other.fire && cap == other.cap; }
    }
    static Values baseline() { return new Values(75, 1, 0, 30); }
    static int integer(SharedPreferences prefs, String key, int fallback) {
        try { return prefs.getInt(key, fallback); } catch (ClassCastException invalid) { return fallback; }
    }
    static boolean hasPending(SharedPreferences prefs) {
        try { return prefs.getBoolean(PENDING, false); } catch (ClassCastException invalid) { return true; }
    }
    static Values confirmed(SharedPreferences prefs) {
        if (prefs.contains("graphics_schema") && integer(prefs, "graphics_schema", -1) != 1) return baseline();
        final int water = integer(prefs, "water_quality", 1);
        final Values values = new Values(integer(prefs, "render_scale", 75), water,
                integer(prefs, FIRE, water == 2 ? 1 : 0), integer(prefs, CAP, 30));
        return values.valid() ? values : baseline();
    }
    static boolean markPending(SharedPreferences prefs, Values candidate) {
        if (!candidate.valid()) return false;
        // Synchronous, bounded local metadata write must complete before native Apply.
        return prefs.edit().putBoolean(PENDING, true).putInt("graphics_pending_scale", candidate.scale)
                .putInt("graphics_pending_water", candidate.water).putInt("graphics_pending_fire", candidate.fire)
                .putInt("graphics_pending_cap", candidate.cap).commit();
    }
    static boolean confirm(SharedPreferences prefs, Values values) {
        if (!values.valid()) return false;
        return prefs.edit().putInt("render_scale", values.scale).putInt("water_quality", values.water)
                .putInt(FIRE, values.fire).putInt(CAP, values.cap).putInt("graphics_schema", 1)
                .putBoolean(PENDING, false).commit();
    }
    static boolean clearAfterRestore(SharedPreferences prefs) {
        // Candidate keys remain retained for recovery diagnostics; they never become effective silently.
        return prefs.edit().putBoolean(PENDING, false).commit();
    }
    static Values retainedCandidate(SharedPreferences prefs) {
        return new Values(integer(prefs, "graphics_pending_scale", 75), integer(prefs, "graphics_pending_water", 1),
                integer(prefs, "graphics_pending_fire", 0), integer(prefs, "graphics_pending_cap", 30));
    }
    static boolean presented(long[] snapshot, long generation) {
        return snapshot != null && snapshot.length == 20 && generation > 0 && snapshot[1] == generation &&
                snapshot[13] == 1 && (snapshot[12] == 1 || snapshot[12] == 2) && snapshot[7] > 0 && snapshot[8] > 0 &&
                snapshot[9] > 0 && snapshot[10] > 0;
    }
    private GraphicsPreferences() {}
}
