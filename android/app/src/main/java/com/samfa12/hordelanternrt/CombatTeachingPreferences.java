package com.samfa12.hordelanternrt;

import android.content.SharedPreferences;

/** Existing combat prompt: optional lesson plus authoritative current-attack readability. */
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

    static int stage(long state) { return (int)(state & 0x7L); }
    static boolean safePractice(long state) { return (state & (1L << 13)) != 0; }
    static boolean exitPending(long state) { return (state & (1L << 33)) != 0; }
    static boolean waitingForRevivalClearance(long state) { return (state & (1L << 34)) != 0; }
    static boolean blockLearned(long state) { return (state & (1L << 15)) != 0; }
    static boolean attackFromGuardLearned(long state) { return (state & (1L << 32)) != 0; }
    static int practiceActionLabelResource(long state) { return stage(state)==6 ? R.string.combat_teaching_finish : R.string.combat_teaching_skip; }

    static String prompt(long state) { return prompt(state, false); }

    static String prompt(long state, boolean controller) {
        final boolean protectedPractice=safePractice(state);
        final boolean enabled=(state & (1L << 10)) != 0;
        final int opacity=(int)((state >>> 40) & 0xffL);
        String text=protectedPractice ? "Practice: no damage" : "";
        if(protectedPractice || enabled && opacity>0) {
            String lesson=lessonText(state,controller);
            if(!lesson.isEmpty())text+=(text.isEmpty()?"":"\n")+lesson;
        }
        if(waitingForRevivalClearance(state))
            text+=(text.isEmpty()?"":"\n")+"Step back from the skeletons.";
        final int attack=attackReadability(state);
        if(attack!=0)text+=(text.isEmpty()?"":"\n")+(attack==1
            ? "\u25c7 YELLOW guard-break: parry/dodge" : "\u2715 RED: unblockable - DODGE");
        return text;
    }

    static int attackReadability(long state) {
        final int value = (int) ((state >>> 48) & 0x3L);
        return value == 1 || value == 2 ? value : 0;
    }
    static int promptColor(long state) {
        final int attack = attackReadability(state);
        return attack == 1 ? 0xFFFFD266 : attack == 2 ? 0xFFFF6B6B : HordeUiTokens.PARCHMENT;
    }
    static int promptOpacity(long state) {
        return safePractice(state) || waitingForRevivalClearance(state) || attackReadability(state) != 0
                ? 255 : (int) ((state >>> 40) & 0xffL);
    }
    static String description(long state, String visibleText) {
        String text=visibleText.replace('\n',' ');
        if(attackReadability(state)==1)text+=" Yellow guard-break attack: parry or dodge; holding guard cannot block it.";
        if(attackReadability(state)==2)text+=" Red unblockable attack: dodge.";
        if(stage(state)==5)text+=" Attacking ends guard protection. Keep guard held to guard again after recovery, without another parry.";
        return text;
    }

    static boolean needsTimelyRefresh(long state) {
        if (safePractice(state) || waitingForRevivalClearance(state) || attackReadability(state) != 0) return true;
        final boolean enabled = (state & (1L << 10)) != 0;
        final int opacity = (int) ((state >>> 40) & 0xffL);
        final int cue = (int) ((state >>> 3) & 0xfL);
        return enabled && opacity > 0 && ((cue >= 1 && cue <= 6) || cue == 8);
    }

    private static String lessonText(long state,boolean controller) {
        final int stage=stage(state),cue=(int)((state >>> 3) & 0xfL),source=(int)((state >>> 7) & 0x7L);
        final String guard=controller?"[LT]":"[Parry]",swing=controller?"[RT]":"[Swing]";
        final String dodge=controller?"[B] + stick":"[Dodge] + move";
        if(exitPending(state) || cue==13)return "Leaving practice safely...";
        if(cue==12)return "Settings: Replay opening practice or Skip to continue";
        if(stage==2 || stage==3)return "";
        if(stage==6 || cue==11)return "Settings: Finish practice - enemies can hurt you";
        if(cue==7)return "Well parried - strike back";
        if(stage==4 || cue==9)return "Hold "+guard+": block normal hits";
        if(stage==5 || cue==10)return "Hold "+guard+" + "+swing+"; keep held";
        if(stage==1) {
            if(cue==5)return "DODGE NOW - "+dodge;
            return (source==3?"Keeper lightning":source==2 || source==4?"Skeleton attack":"Attacker")+" - "+dodge;
        }
        switch(cue) {
            case 1: return "Watch the raised arm - fresh "+guard+" press";
            case 2: return "PARRY NOW - fresh "+guard+" press";
            case 8: return "Timed parry active - face the attacker";
            case 3: return "Release, then press "+guard+" to parry";
            default:return "Fresh "+guard+" press: timed parry";
        }
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

    private CombatTeachingPreferences() { }
}
