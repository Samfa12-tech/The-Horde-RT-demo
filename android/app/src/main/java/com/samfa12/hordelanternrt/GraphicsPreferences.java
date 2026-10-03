package com.samfa12.hordelanternrt;

import android.content.SharedPreferences;

/** Thin graphics-only storage adapter. GPU application belongs to the shared native owner. */
final class GraphicsPreferences {
    static final String PENDING = "graphics_pending";
    static final String FIRE = "fire_detail";
    static final String CAP = "graphics_preview_cap";
    static final String GLASS = "graphics_glass_enabled";
    static final String PENDING_GLASS = "graphics_pending_glass";
    static final int SCHEMA = 2;
    static final int MIN_RENDER_SCALE_PERCENT = BuildConfig.MIN_RENDER_SCALE_PERCENT;
    static boolean validScale(int scale, int minimum) {
        return (minimum == 50 || minimum == 33) && ((scale >= 50 && scale <= 100) ||
                (minimum == 33 && (scale == 33 || scale == 40)));
    }
    static final class Values {
        final int scale, water, fire, cap;
        final boolean glassEnabled;
        Values(int scale, int water, int fire, int cap) {
            this(scale, water, fire, cap, true);
        }
        Values(int scale, int water, int fire, int cap, boolean glassEnabled) {
            this.scale = scale; this.water = water; this.fire = fire; this.cap = cap;
            this.glassEnabled = glassEnabled;
        }
        boolean valid() { return valid(MIN_RENDER_SCALE_PERCENT); }
        boolean valid(int minimum) { return validScale(scale, minimum) && water >= 0 && water <= 2 &&
                fire >= 0 && fire <= 1 && cap >= 15 && cap <= 60; }
        boolean same(Values other) { return other != null && scale == other.scale && water == other.water &&
                fire == other.fire && cap == other.cap && glassEnabled == other.glassEnabled; }
    }
    static Values baseline() { return new Values(75, 1, 0, 30); }
    static int integer(SharedPreferences prefs, String key, int fallback) {
        try { return prefs.getInt(key, fallback); } catch (ClassCastException invalid) { return fallback; }
    }
    static boolean hasPending(SharedPreferences prefs) {
        try { return prefs.getBoolean(PENDING, false); } catch (ClassCastException invalid) { return true; }
    }
    static Values confirmed(SharedPreferences prefs) {
        return confirmed(prefs, MIN_RENDER_SCALE_PERCENT);
    }
    static Values confirmed(SharedPreferences prefs, int minimum) {
        final int schema = integer(prefs, "graphics_schema", prefs.contains("graphics_schema") ? -1 : 1);
        if (schema != 1 && schema != SCHEMA) return baseline();
        final boolean glass;
        try { glass = schema == 1 || prefs.getBoolean(GLASS, true); }
        catch (ClassCastException invalid) { return baseline(); }
        final int water = integer(prefs, "water_quality", 1);
        final Values values = new Values(integer(prefs, "render_scale", 75), water,
                integer(prefs, FIRE, water == 2 ? 1 : 0), integer(prefs, CAP, 30), glass);
        return values.valid(minimum) ? values : baseline();
    }
    static boolean markPending(SharedPreferences prefs, Values candidate) {
        if (!candidate.valid()) return false;
        // Synchronous, bounded local metadata write must complete before native Apply.
        return prefs.edit().putBoolean(PENDING, true).putInt("graphics_pending_scale", candidate.scale)
                .putInt("graphics_pending_water", candidate.water).putInt("graphics_pending_fire", candidate.fire)
                .putInt("graphics_pending_cap", candidate.cap).putBoolean(PENDING_GLASS, candidate.glassEnabled)
                .putInt("graphics_pending_schema", SCHEMA).commit();
    }
    static boolean confirm(SharedPreferences prefs, Values values) {
        if (!values.valid()) return false;
        return prefs.edit().putInt("render_scale", values.scale).putInt("water_quality", values.water)
                .putInt(FIRE, values.fire).putInt(CAP, values.cap).putBoolean(GLASS, values.glassEnabled)
                .putInt("graphics_schema", SCHEMA)
                .putBoolean(PENDING, false).commit();
    }
    static boolean clearAfterRestore(SharedPreferences prefs) {
        // Candidate keys remain retained for recovery diagnostics; they never become effective silently.
        return prefs.edit().putBoolean(PENDING, false).commit();
    }
    static Values retainedCandidate(SharedPreferences prefs) {
        final int pendingSchema = integer(prefs, "graphics_pending_schema", integer(prefs, "graphics_schema", 1));
        if (pendingSchema != 1 && pendingSchema != SCHEMA) return baseline();
        boolean glass = true;
        if (pendingSchema == SCHEMA) {
            try { glass = prefs.getBoolean(PENDING_GLASS, true); }
            catch (ClassCastException invalid) { return baseline(); }
        }
        return new Values(integer(prefs, "graphics_pending_scale", 75), integer(prefs, "graphics_pending_water", 1),
                integer(prefs, "graphics_pending_fire", 0), integer(prefs, "graphics_pending_cap", 30), glass);
    }
    static boolean presented(long[] snapshot, long generation) {
        return snapshot != null && snapshot.length == 22 && generation > 0 && snapshot[1] == generation &&
                snapshot[13] == 1 && (snapshot[12] == 1 || snapshot[12] == 2) && snapshot[7] > 0 && snapshot[8] > 0 &&
                snapshot[9] > 0 && snapshot[10] > 0 && (snapshot[20] == 0 || snapshot[20] == 1) &&
                (snapshot[21] == 0 || snapshot[21] == 1);
    }
    static boolean matchesEffective(long[] snapshot, Values values) {
        return values != null && snapshot != null && snapshot.length == 22 &&
                snapshot[20] == (values.glassEnabled ? 1 : 0) && values.scale == snapshot[3] &&
                values.water == snapshot[4] && values.fire == snapshot[5] && values.cap == snapshot[6];
    }
    static boolean matchesRequested(long[] snapshot, Values values) {
        return values != null && snapshot != null && snapshot.length == 22 &&
                snapshot[21] == (values.glassEnabled ? 1 : 0) && values.scale == snapshot[15] &&
                values.water == snapshot[16] && values.fire == snapshot[17] && values.cap == snapshot[18];
    }
    private GraphicsPreferences() {}
}
