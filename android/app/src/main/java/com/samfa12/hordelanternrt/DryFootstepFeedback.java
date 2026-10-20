package com.samfa12.hordelanternrt;

/** Native simulation selects one type-scoped cue per grounded dry contact.
 * Surface choice and variation do not depend on Java polling or mute state. */
final class DryFootstepFeedback {
    private DryFootstepFeedback() {}

    static String soundKey(int cue) {
        switch (cue) {
            case 1: return "player_step_1";
            case 2: return "player_step_2";
            case 3: return "skeleton_step_1";
            case 4: return "skeleton_step_2";
            default: return null;
        }
    }
}
