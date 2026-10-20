package com.samfa12.hordelanternrt;
import static org.junit.Assert.*;
import org.junit.Test;

public class ChapterAmbienceFeedbackTest {
    @Test public void finiteLocalAndExistingNatureCuesKeepTheirIdentity() {
        assertEquals("tomb_drip", ChapterAmbienceFeedback.soundKey(32));
        assertEquals("rope_contact", ChapterAmbienceFeedback.soundKey(33));
        assertEquals("forest_night_owl", ChapterAmbienceFeedback.soundKey(28));
        assertEquals("forest_evening_cricket", ChapterAmbienceFeedback.soundKey(30));
        assertEquals("pond_frogs_crickets", ChapterAmbienceFeedback.soundKey(31));
    }
    @Test public void stopAndUnknownEventsCannotSelectAudio() {
        for(int type: new int[]{-1,0,27,29,34,35,255})
            assertNull(ChapterAmbienceFeedback.soundKey(type));
        assertTrue(ChapterAmbienceFeedback.isSignal(29));
        assertTrue(ChapterAmbienceFeedback.isSignal(34));
        assertFalse(ChapterAmbienceFeedback.isSignal(27));
        assertFalse(ChapterAmbienceFeedback.isSignal(35));
    }
}
