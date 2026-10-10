package com.samfa12.hordelanternrt;

import android.content.SharedPreferences;

/** Device-local presentation choices for the optional combat teaching overlay. */
final class CombatTeachingPreferences {
    static final String ENABLED = "combat_teaching_enabled";
    static final String SLOWDOWN = "combat_teaching_slowdown";
    static final long PROMPT_FADE_MILLISECONDS = 120L;

    static final class Values {
        final boolean enabled;
        final boolean slowdown;
        Values(boolean enabled, boolean slowdown) {
            this.enabled = enabled;
            this.slowdown = slowdown;
        }
    }

    static Values read(SharedPreferences preferences) {
        return new Values(flag(preferences, ENABLED, true), flag(preferences, SLOWDOWN, false));
    }

    static void save(SharedPreferences preferences, Values values) {
        preferences.edit().putBoolean(ENABLED, values.enabled)
                .putBoolean(SLOWDOWN, values.slowdown).apply();
    }

    static boolean flag(SharedPreferences preferences, String key, boolean fallback) {
        try { return preferences.getBoolean(key, fallback); }
        catch (ClassCastException invalid) { return fallback; }
    }

    static String prompt(long state) { return prompt(state, false); }

    static String prompt(long state, boolean controller) {
        final boolean enabled = (state & (1L << 10)) != 0;
        final int opacity = (int) ((state >>> 40) & 0xffL);
        if (!enabled || opacity == 0) return "";
        final int stage = (int) (state & 0x7L);
        final int cue = (int) ((state >>> 3) & 0xfL);
        final int source = (int) ((state >>> 7) & 0x7L);
        final boolean parryLearned = (state & (1L << 11)) != 0;
        final boolean dodgeLearned = (state & (1L << 12)) != 0;
        final String lesson = cueText(cue, stage, source, parryLearned, dodgeLearned);
        if (lesson.isEmpty()) return "";
        return lesson + (controller ? "\n[LT] Parry  ·  [B] + stick: Dodge"
                : "\n[Parry] Guard  ·  [Dodge] + move: Evade");
    }

    static boolean needsTimelyRefresh(long state) {
        final boolean enabled = (state & (1L << 10)) != 0;
        final int opacity = (int) ((state >>> 40) & 0xffL);
        final int cue = (int) ((state >>> 3) & 0xfL);
        return enabled && opacity > 0 && ((cue >= 1 && cue <= 6) || cue == 8);
    }

    static float alpha(int opacity, boolean reducedMotion) {
        final int bounded = Math.max(0, Math.min(255, opacity));
        if (reducedMotion) return bounded > 0 ? 1.0f : 0.0f;
        return bounded / 255.0f;
    }

    static int fadeAlpha(int previousOpacity, long elapsedMilliseconds, boolean reducedMotion) {
        final int opacity = Math.max(0, Math.min(255, previousOpacity));
        if (reducedMotion || elapsedMilliseconds >= PROMPT_FADE_MILLISECONDS) return 0;
        final long elapsed = Math.max(0L, elapsedMilliseconds);
        return Math.max(0, Math.min(255, Math.round(opacity *
                (PROMPT_FADE_MILLISECONDS - elapsed) / (float) PROMPT_FADE_MILLISECONDS)));
    }

    private static String cueText(int cue, int stage, int source, boolean parryLearned, boolean dodgeLearned) {
        // Source identity stays in diagnostics; the lesson names the visible action.
        switch (cue) {
            case 1: return "○ Watch the raised blade";
            case 2: return "○ PARRY NOW — raise your guard";
            case 8: return "● Guard up — face the blade";
            case 3: return "↩ Ready for the next strike";
            case 4: return "○ The Keeper gathers lightning — watch the staff";
            case 5: return "← DODGE NOW → — step sideways";
            case 6: return "↩ Close in before the next charge";
            case 7: return "● Well parried — strike back";
            default:
                if (stage == 2 || stage == 3) return "";
                if (parryLearned && !dodgeLearned) return "○ Dodge the Keeper's lightning";
                return "○ Watch the attacker";
        }
    }

    private CombatTeachingPreferences() { }
}
