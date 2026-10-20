package com.samfa12.hordelanternrt;

/** Append-only semantic IDs; all cues use the same finite incidental voice. */
final class ChapterAmbienceFeedback {
    static final int TOMB_DRIP=32, ROPE_CONTACT=33, LOCAL_STOPPED=34;
    static boolean isSignal(int type) { return type>=28 && type<=LOCAL_STOPPED; }
    static String soundKey(int type) {
        switch(type) {
            case 28: return "forest_night_owl";
            case 30: return "forest_evening_cricket";
            case 31: return "pond_frogs_crickets";
            case TOMB_DRIP: return "tomb_drip";
            case ROPE_CONTACT: return "rope_contact";
            default: return null;
        }
    }
}
