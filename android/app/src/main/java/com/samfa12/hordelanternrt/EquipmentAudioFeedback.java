package com.samfa12.hordelanternrt;

/** Selection for a drained semantic event. Inactive events are discarded,
 * so opening a menu or returning from Home cannot postpone an equipment cue. */
final class EquipmentAudioFeedback {
    private EquipmentAudioFeedback() {}

    static String soundKey(int eventType, int cue, boolean gameplayActive) {
        if (!gameplayActive || eventType != 22) return null;
        if (cue == 1) return "sword_draw";
        if (cue == 2) return "sword_sheath";
        return null;
    }
}
