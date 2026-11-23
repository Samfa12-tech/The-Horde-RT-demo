package com.samfa12.hordelanternrt;

/** Append-only semantic IDs; all cues use the same finite incidental voice. */
final class ChapterAmbienceFeedback {
    static final int TOMB_DRIP=32, ROPE_CONTACT=33, LOCAL_STOPPED=34, FOREST_FOLIAGE=35;
    static final int BELLWETHER_HEARTH=39;
    static final int BELLWETHER_PIG_GRUNT=40, BELLWETHER_HEN_CLUCK=41;
    static final int BELLWETHER_WATERWHEEL_CREAK=42;
    static boolean isSignal(int type) {
        return (type>=28 && type<=FOREST_FOLIAGE) || (type>=BELLWETHER_HEARTH && type<=BELLWETHER_WATERWHEEL_CREAK);
    }
    /** Return a tuple offset, including stops, so an older start cannot replay. */
    static int latestSignalIndex(long[] events) {
        int latest=-1;
        if(events==null) return latest;
        for(int i=0;i+2<events.length;i+=3)
            if(isSignal((int)(events[i]&0xffL))) latest=i;
        return latest;
    }
    static String soundKey(int type) {
        switch(type) {
            case 28: return "forest_night_owl";
            case 30: return "forest_evening_cricket";
            case 31: return "pond_frogs_crickets";
            case TOMB_DRIP: return "tomb_drip";
            case ROPE_CONTACT: return "rope_contact";
            case FOREST_FOLIAGE: return "forest_leaf_rustle";
            case BELLWETHER_HEARTH: return "bellwether_hearth";
            case BELLWETHER_PIG_GRUNT: return "bellwether_pig_grunt";
            case BELLWETHER_HEN_CLUCK: return "bellwether_hen_cluck";
            case BELLWETHER_WATERWHEEL_CREAK: return "bellwether_waterwheel_creak";
            default: return null;
        }
    }
}
