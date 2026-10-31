package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import org.junit.Test;

public class CombatAudioFeedbackTest {
    @Test public void realOutcomeSelectsStableAdmittedCue() {
        assertEquals("sword_hit_1", CombatAudioFeedback.soundKey(37, 1, true));
        assertEquals("sword_hit_1", CombatAudioFeedback.soundKey(6, 2, true));
        assertEquals("lich_hurt", CombatAudioFeedback.soundKey(6, 3, true));
        assertEquals(.28f, CombatAudioFeedback.gain(37, 1), .0001f);
        assertEquals(.32f, CombatAudioFeedback.gain(6, 2), .0001f);
        assertEquals(.82f, CombatAudioFeedback.gain(6, 3), .0001f);
    }
    @Test public void noPreparationParryMissOrMalformedCueCanClaimImpact() {
        for (int type : new int[]{1, 2, 7, 12, 24, 36})
            for (int cue = 0; cue < 5; ++cue)
                assertNull(CombatAudioFeedback.soundKey(type, cue, true));
        for (int cue : new int[]{0, 2, 3, 255})
            assertNull(CombatAudioFeedback.soundKey(37, cue, true));
        assertNull(CombatAudioFeedback.soundKey(6, 1, true));
        assertEquals(0f, CombatAudioFeedback.gain(37, 2), 0f);
    }
    @Test public void inactiveDrainDoesNotPostponeCueUntilResume() {
        assertNull(CombatAudioFeedback.soundKey(37, 1, false));
        assertNull(CombatAudioFeedback.soundKey(6, 2, false));
        assertNull(CombatAudioFeedback.soundKey(6, 3, false));
        // The selector owns no queue or pending state to replay on resume.
        assertNull(CombatAudioFeedback.soundKey(0, 0, true));
    }
}
