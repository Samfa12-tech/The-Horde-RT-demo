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
        assertEquals(35, ChapterAmbienceFeedback.FOREST_FOLIAGE);
        assertEquals("forest_leaf_rustle", ChapterAmbienceFeedback.soundKey(35));
    }
    @Test public void stopAndUnknownEventsCannotSelectAudio() {
        for(int type: new int[]{-1,0,27,29,34,36,255})
            assertNull(ChapterAmbienceFeedback.soundKey(type));
        assertTrue(ChapterAmbienceFeedback.isSignal(29));
        assertTrue(ChapterAmbienceFeedback.isSignal(34));
        assertFalse(ChapterAmbienceFeedback.isSignal(27));
        assertTrue(ChapterAmbienceFeedback.isSignal(35));
        assertFalse(ChapterAmbienceFeedback.isSignal(36));
    }
    private static long[] batch(int... types) {
        long[] result=new long[types.length*3];
        for(int i=0;i<types.length;i++) {
            // High sequence/source/cue bits must not change the semantic type.
            result[i*3]=((long)(57+i)<<32)|0x00ab1200L|types[i];
            result[i*3+1]=0x3f0000003e800000L;
            result[i*3+2]=0x3f80000040000000L;
        }
        return result;
    }
    @Test public void laterStopsSuppressOlderFoliageAcrossTheCombinedFamily() {
        long[] forestStop=batch(35,0,29);
        assertEquals(6, ChapterAmbienceFeedback.latestSignalIndex(forestStop));
        assertNull(ChapterAmbienceFeedback.soundKey((int)(forestStop[6]&255)));
        assertEquals(6, ChapterAmbienceFeedback.latestSignalIndex(batch(35,32,34)));
        assertEquals(6, ChapterAmbienceFeedback.latestSignalIndex(batch(33,35,34)));
    }
    @Test public void laterEligibleStartsReplaceOldStopsAndOtherFamilyStarts() {
        long[] foliage=batch(29,0,35);
        assertEquals(6, ChapterAmbienceFeedback.latestSignalIndex(foliage));
        assertEquals("forest_leaf_rustle", ChapterAmbienceFeedback.soundKey((int)(foliage[6]&255)));
        assertEquals(6, ChapterAmbienceFeedback.latestSignalIndex(batch(34,35,30)));
        assertEquals(6, ChapterAmbienceFeedback.latestSignalIndex(batch(35,33,31)));
    }
    @Test public void nonAmbientAndIncompleteTuplesCannotReplaceAValidSignal() {
        assertEquals(0, ChapterAmbienceFeedback.latestSignalIndex(batch(35,0,36,255)));
        assertEquals(-1, ChapterAmbienceFeedback.latestSignalIndex(batch(0,27,36,255)));
        assertEquals(-1, ChapterAmbienceFeedback.latestSignalIndex(new long[0]));
        assertEquals(-1, ChapterAmbienceFeedback.latestSignalIndex(new long[]{35,0}));
        assertEquals(0, ChapterAmbienceFeedback.latestSignalIndex(new long[]{35,0,0,29,0}));
    }
}
