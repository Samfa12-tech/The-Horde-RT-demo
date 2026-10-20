package com.samfa12.hordelanternrt;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import org.junit.Test;

public final class DryFootstepFeedbackTest {
    @Test public void nativeCueByteSelectsOnlyExistingAdmittedSounds() {
        assertEquals("player_step_1", DryFootstepFeedback.soundKey(1));
        assertEquals("player_step_2", DryFootstepFeedback.soundKey(2));
        assertEquals("skeleton_step_1", DryFootstepFeedback.soundKey(3));
        assertEquals("skeleton_step_2", DryFootstepFeedback.soundKey(4));
    }

    @Test public void missingOrInvalidCueIsDiscardedWithoutFallbackOrLayer() {
        for (int cue : new int[]{Integer.MIN_VALUE, -1, 0, 5, 255, Integer.MAX_VALUE})
            assertNull(DryFootstepFeedback.soundKey(cue));
    }
}
