package com.samfa12.hordelanternrt;

/** Consumes the native contact's semantic cue, never the UI's latest swing.
 * Inactive events are discarded while draining, not deferred until resume. */
final class CombatAudioFeedback {
    static final int GUARD_BLOCK_EVENT = 37;
    private CombatAudioFeedback() {}

    static String soundKey(int eventType, int cue, boolean gameplayActive) {
        if (!gameplayActive) return null;
        if (eventType == GUARD_BLOCK_EVENT && cue == 1) return "sword_hit_1";
        if (eventType == 6 && cue == 2) return "sword_hit_1";
        if (eventType == 6 && cue == 3) return "lich_hurt";
        return null;
    }

    static float gain(int eventType, int cue) {
        if (soundKey(eventType, cue, true) == null) return 0f;
        if (eventType == GUARD_BLOCK_EVENT) return .28f;
        return cue == 3 ? .82f : .32f;
    }
}
