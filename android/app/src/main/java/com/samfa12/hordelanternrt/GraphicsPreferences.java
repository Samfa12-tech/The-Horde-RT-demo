package com.samfa12.hordelanternrt;

import android.content.SharedPreferences;

/** Thin graphics-only storage adapter. GPU application belongs to the shared native owner. */
final class GraphicsPreferences {
    static final String PENDING = "graphics_pending";
    static final String FIRE = "fire_detail";
    static final String CAP = "graphics_preview_cap";
    static final String GLASS = "graphics_glass_enabled";
    static final String PENDING_GLASS = "graphics_pending_glass";
    static final String SHADOW = "graphics_shadow_quality";
    static final String PENDING_SHADOW = "graphics_pending_shadow";
    static final int SHADOW_LOWER = 0, SHADOW_CURRENT = 1, SHADOW_HIGHER = 2;
    static final int FIRE_MOBILE = 0, FIRE_HIGH = 1, FIRE_LOW = 2;
    static final int SCHEMA = 4;
    static final String MIST = "graphics_mist_enabled";
    static final String PENDING_MIST = "graphics_pending_mist";
    static final String UNSTORED_MOBILE_DEFAULTS = "graphics_unstored_mobile_defaults";
    static final int MIN_RENDER_SCALE_PERCENT = BuildConfig.MIN_RENDER_SCALE_PERCENT;
    static boolean validScale(int scale, int minimum) {
        return (minimum == 50 || minimum == 33) && ((scale >= 50 && scale <= 100) ||
                (minimum == 33 && (scale == 33 || scale == 40)));
    }
    static final class Values {
        final int scale, water, fire, cap, shadow;
        final boolean glassEnabled, mistEnabled;
        Values(int scale, int water, int fire, int cap) {
            this(scale, water, fire, cap, true);
        }
        Values(int scale, int water, int fire, int cap, boolean glassEnabled) {
            this(scale, water, fire, cap, glassEnabled, SHADOW_CURRENT);
        }
        Values(int scale, int water, int fire, int cap, boolean glassEnabled, int shadow) {
            this(scale, water, fire, cap, glassEnabled, shadow, true);
        }
        Values(int scale, int water, int fire, int cap, boolean glassEnabled, int shadow, boolean mistEnabled) {
            this.scale = scale; this.water = water; this.fire = fire; this.cap = cap;
            this.glassEnabled = glassEnabled; this.shadow = shadow; this.mistEnabled = mistEnabled;
        }
        boolean valid() { return valid(MIN_RENDER_SCALE_PERCENT); }
        boolean valid(int minimum) { return validScale(scale, minimum) && water >= 0 && water <= 2 &&
                fire >= FIRE_MOBILE && fire <= FIRE_LOW && cap >= 15 && cap <= 60 &&
                shadow >= SHADOW_LOWER && shadow <= SHADOW_HIGHER; }
        boolean same(Values other) { return other != null && scale == other.scale && water == other.water &&
                fire == other.fire && cap == other.cap && glassEnabled == other.glassEnabled &&
                shadow == other.shadow && mistEnabled == other.mistEnabled; }
    }
    static Values baseline() { return new Values(75, 1, 0, 30); }
    // Historical accepted appearance remains separate from the mobile startup/reset policy.
    static Values mobileDefaults() { return new Values(50, 1, FIRE_MOBILE, 30, false, SHADOW_CURRENT, true); }
    static boolean hasGraphicsSettings(SharedPreferences prefs) {
        for (String key : prefs.getAll().keySet()) {
            if (key.equals("render_scale") || key.equals("water_quality") || key.equals(FIRE) ||
                    key.startsWith("graphics_")) return true;
        }
        return false;
    }
    private static boolean usesUnstoredMobileDefaults(SharedPreferences prefs) {
        for (String key : new String[]{"render_scale", "water_quality", FIRE, CAP, GLASS, SHADOW, MIST, "graphics_schema"}) {
            if (prefs.contains(key)) return false;
        }
        // Only a transaction that began without graphics keys can retain the new mobile policy.
        // Historical pending-only records keep their original accepted baseline.
        try { return prefs.getBoolean(UNSTORED_MOBILE_DEFAULTS, false) &&
                integer(prefs, "graphics_pending_schema", 0) == SCHEMA; }
        catch (ClassCastException invalid) { return false; }
    }
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
        if (!hasGraphicsSettings(prefs) || usesUnstoredMobileDefaults(prefs)) return mobileDefaults();
        final int schema = integer(prefs, "graphics_schema", prefs.contains("graphics_schema") ? -1 : 1);
        if (schema < 1 || schema > SCHEMA) return baseline();
        final boolean glass;
        try { glass = schema == 1 || prefs.getBoolean(GLASS, true); }
        catch (ClassCastException invalid) { return baseline(); }
        final boolean mist;
        try { mist = schema < 4 || prefs.getBoolean(MIST, true); }
        catch (ClassCastException invalid) { return baseline(); }
        final int water = integer(prefs, "water_quality", 1);
        final Values values = new Values(integer(prefs, "render_scale", 75), water,
                integer(prefs, FIRE, water == 2 ? 1 : 0), integer(prefs, CAP, 30), glass,
                schema < 3 ? SHADOW_CURRENT : integer(prefs, SHADOW,
                        prefs.contains(SHADOW) ? -1 : SHADOW_CURRENT), mist);
        return values.valid(minimum) && (schema >= 3 || values.fire <= FIRE_HIGH) ? values : baseline();
    }
    static boolean markPending(SharedPreferences prefs, Values candidate) {
        if (!candidate.valid()) return false;
        final boolean unstoredMobileDefaults = !hasGraphicsSettings(prefs) || usesUnstoredMobileDefaults(prefs);
        // Synchronous, bounded local metadata write must complete before native Apply.
        return prefs.edit().putBoolean(UNSTORED_MOBILE_DEFAULTS, unstoredMobileDefaults)
                .putBoolean(PENDING, true).putInt("graphics_pending_scale", candidate.scale)
                .putInt("graphics_pending_water", candidate.water).putInt("graphics_pending_fire", candidate.fire)
                .putInt("graphics_pending_cap", candidate.cap).putBoolean(PENDING_GLASS, candidate.glassEnabled)
                .putInt(PENDING_SHADOW, candidate.shadow).putBoolean(PENDING_MIST, candidate.mistEnabled)
                .putInt("graphics_pending_schema", SCHEMA).commit();
    }
    static boolean confirm(SharedPreferences prefs, Values values) {
        if (!values.valid()) return false;
        return prefs.edit().putInt("render_scale", values.scale).putInt("water_quality", values.water)
                .putInt(FIRE, values.fire).putInt(CAP, values.cap).putBoolean(GLASS, values.glassEnabled)
                .putInt(SHADOW, values.shadow).putBoolean(MIST, values.mistEnabled)
                .putInt("graphics_schema", SCHEMA)
                .putBoolean(UNSTORED_MOBILE_DEFAULTS, false).putBoolean(PENDING, false).commit();
    }
    static boolean clearAfterRestore(SharedPreferences prefs) {
        // Candidate keys remain retained for recovery diagnostics; they never become effective silently.
        final boolean restoreUnstoredMobileDefaults = !hasGraphicsSettings(prefs) || usesUnstoredMobileDefaults(prefs);
        SharedPreferences.Editor editor = prefs.edit().putBoolean(PENDING, false)
                .putBoolean(UNSTORED_MOBILE_DEFAULTS, restoreUnstoredMobileDefaults);
        if (restoreUnstoredMobileDefaults) editor.putInt("graphics_pending_schema", SCHEMA);
        return editor.commit();
    }
    static Values retainedCandidate(SharedPreferences prefs) {
        final int pendingSchema = integer(prefs, "graphics_pending_schema", integer(prefs, "graphics_schema", 1));
        if (pendingSchema < 1 || pendingSchema > SCHEMA) return baseline();
        boolean glass = true;
        if (pendingSchema >= 2) {
            try { glass = prefs.getBoolean(PENDING_GLASS, true); }
            catch (ClassCastException invalid) { return baseline(); }
        }
        final boolean mist;
        try { mist = pendingSchema < 4 || prefs.getBoolean(PENDING_MIST, true); }
        catch (ClassCastException invalid) { return baseline(); }
        final Values values = new Values(integer(prefs, "graphics_pending_scale", 75), integer(prefs, "graphics_pending_water", 1),
                integer(prefs, "graphics_pending_fire", 0), integer(prefs, "graphics_pending_cap", 30), glass,
                pendingSchema < 3 ? SHADOW_CURRENT : integer(prefs, PENDING_SHADOW,
                        prefs.contains(PENDING_SHADOW) ? -1 : SHADOW_CURRENT), mist);
        // Retained intent may include the explicitly isolated benchmark scales even
        // in an ordinary build. It never becomes effective without owner recovery.
        return values.valid(33) && (pendingSchema >= 3 || values.fire <= FIRE_HIGH) ? values : baseline();
    }
    static boolean presented(long[] snapshot, long generation) {
        return snapshot != null && snapshot.length == 26 && generation > 0 && snapshot[1] == generation &&
                snapshot[13] == 1 && (snapshot[12] == 1 || snapshot[12] == 2) && snapshot[7] > 0 && snapshot[8] > 0 &&
                snapshot[9] > 0 && snapshot[10] > 0 && (snapshot[20] == 0 || snapshot[20] == 1) &&
                (snapshot[21] == 0 || snapshot[21] == 1) &&
                (snapshot[24] == 0 || snapshot[24] == 1) && (snapshot[25] == 0 || snapshot[25] == 1) &&
                snapshot[5] >= FIRE_MOBILE && snapshot[5] <= FIRE_LOW &&
                snapshot[17] >= FIRE_MOBILE && snapshot[17] <= FIRE_LOW &&
                snapshot[22] >= SHADOW_LOWER && snapshot[22] <= SHADOW_HIGHER &&
                snapshot[23] >= SHADOW_LOWER && snapshot[23] <= SHADOW_HIGHER;
    }
    static boolean matchesEffective(long[] snapshot, Values values) {
        return values != null && snapshot != null && snapshot.length == 26 &&
                snapshot[20] == (values.glassEnabled ? 1 : 0) && values.scale == snapshot[3] &&
                values.water == snapshot[4] && values.fire == snapshot[5] && values.cap == snapshot[6] &&
                values.shadow == snapshot[22] && snapshot[24] == (values.mistEnabled ? 1 : 0);
    }
    static boolean matchesRequested(long[] snapshot, Values values) {
        return values != null && snapshot != null && snapshot.length == 26 &&
                snapshot[21] == (values.glassEnabled ? 1 : 0) && values.scale == snapshot[15] &&
                values.water == snapshot[16] && values.fire == snapshot[17] && values.cap == snapshot[18] &&
                values.shadow == snapshot[23] && snapshot[25] == (values.mistEnabled ? 1 : 0);
    }
    private GraphicsPreferences() {}
}
