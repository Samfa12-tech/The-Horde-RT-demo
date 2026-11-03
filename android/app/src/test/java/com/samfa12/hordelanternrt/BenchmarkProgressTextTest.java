package com.samfa12.hordelanternrt;
import org.junit.Test;
import static org.junit.Assert.*;
public final class BenchmarkProgressTextTest {
    @Test public void compactCourseOmitsLiveTimingWithoutInventingProgress() {
        String full="RT loop 42.0 FPS | mean 23.8ms\nlast 120 frames; not display Hz\nBENCHMARK 1/2  |  WAYPOINT 3/13  |  Shadow corridor";
        assertEquals("Lap 1/2 / Waypoint 3/13",BenchmarkProgressText.compact(full));
        assertTrue(full.contains("not display Hz"));
        assertEquals("Lap 2/2 / Waypoint 13/13",BenchmarkProgressText.compact("BENCHMARK 2/2 | WAYPOINT 13/13 | Finale"));
    }
    @Test public void otherWorkloadsAndStatusRemainTruthful() {
        assertEquals("Lantern 1/2 | FRAME 20/60",BenchmarkProgressText.compact("Lantern 1/2 | FRAME 20/60"));
        assertEquals("Returning to opening menu...",BenchmarkProgressText.compact("Returning to opening menu..."));
        assertEquals("",BenchmarkProgressText.compact(null));
    }
}
