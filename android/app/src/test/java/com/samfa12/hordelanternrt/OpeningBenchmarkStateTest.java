package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import org.junit.Test;

public final class OpeningBenchmarkStateTest {
    @Test public void coherentShapeAndIdentifiersAreRequired() {
        assertNull(OpeningBenchmarkState.decode(null));
        assertNull(OpeningBenchmarkState.decode(new long[5]));
        assertNull(OpeningBenchmarkState.decode(new long[7]));
        for(long[] invalid:new long[][]{{-1,31,1,0,0,31},{7,-1,1,0,0,31},{7,31,7,0,0,31},
                {7,31,1,1,0,31},{7,31,1,0,2,31},{7,31,1,0,0,0},{0,31,1,0,0,31}})
            assertNull(OpeningBenchmarkState.decode(invalid));
        assertNotNull(OpeningBenchmarkState.decode(new long[]{0,0,0,0,0,0}));
    }
    @Test public void courseResultCannotReleaseUiBeforeCurrentEntryPresentation() {
        for(int phase:new int[]{OpeningBenchmarkState.QUEUED,OpeningBenchmarkState.STARTING,
                OpeningBenchmarkState.RUNNING,OpeningBenchmarkState.RETURNING,OpeningBenchmarkState.BLOCKED}) {
            OpeningBenchmarkState state=OpeningBenchmarkState.decode(new long[]{7,31,phase,2,1,31});
            assertNotNull(state); assertFalse(state.returnedToEntry(7,31,31));
        }
        OpeningBenchmarkState unpresented=OpeningBenchmarkState.decode(new long[]{7,31,5,2,0,31});
        assertFalse(unpresented.returnedToEntry(7,31,31));
        OpeningBenchmarkState noResult=OpeningBenchmarkState.decode(new long[]{7,31,5,0,1,31});
        assertFalse(noResult.returnedToEntry(7,31,31));
    }
    @Test public void bothIdentityAndNewSurfaceMustMatchAfterRotation() {
        OpeningBenchmarkState returned=OpeningBenchmarkState.decode(new long[]{7,42,5,3,1,31});
        assertTrue(returned.matches(7,31));
        assertTrue(returned.returnedToEntry(7,31,42));
        assertFalse(returned.returnedToEntry(7,31,31));
        assertFalse(returned.returnedToEntry(8,31,42));
        assertFalse(returned.returnedToEntry(7,30,42));
        assertFalse(returned.returnedToEntry(7,31,0));
    }
    @Test public void unavailableIsDistinctAndStillRequiresFreshReturnedEntry() {
        OpeningBenchmarkState returning=OpeningBenchmarkState.decode(new long[]{7,42,4,4,0,31});
        assertNotNull(returning);assertEquals(OpeningBenchmarkState.UNAVAILABLE,returning.result);
        assertFalse(returning.returnedToEntry(7,31,42));
        OpeningBenchmarkState complete=OpeningBenchmarkState.decode(new long[]{7,42,5,4,1,31});
        assertTrue(complete.returnedToEntry(7,31,42));
        assertFalse(complete.returnedToEntry(7,31,31));
    }
}
