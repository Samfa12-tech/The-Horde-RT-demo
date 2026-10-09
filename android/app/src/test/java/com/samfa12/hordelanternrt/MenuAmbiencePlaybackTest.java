package com.samfa12.hordelanternrt;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.junit.Test;

public final class MenuAmbiencePlaybackTest {
    private static final class Start {
        final String key;
        final float gain;
        final boolean looping;
        final int stream;
        Start(String key, float gain, boolean looping, int stream) {
            this.key = key; this.gain = gain; this.looping = looping; this.stream = stream;
        }
    }

    private static final class FakeSink implements MenuAmbiencePlayback.Sink {
        final List<Start> starts = new ArrayList<>();
        final List<Integer> stops = new ArrayList<>();
        final Map<Integer, Float> gains = new HashMap<>();
        final Map<String, Integer> failedStartsRemaining = new HashMap<>();
        final Set<Integer> active = new HashSet<>();
        int nextStream = 1;

        @Override public int start(String key, float gain, boolean looping) {
            int failures = failedStartsRemaining.getOrDefault(key, 0);
            if (failures > 0) {
                failedStartsRemaining.put(key, failures - 1);
                starts.add(new Start(key, gain, looping, 0));
                return 0;
            }
            int stream = nextStream++;
            starts.add(new Start(key, gain, looping, stream));
            gains.put(stream, gain);
            active.add(stream);
            return stream;
        }

        @Override public void gain(int stream, float gain) {
            assertTrue("gain must target an owned active stream", active.contains(stream));
            gains.put(stream, gain);
        }

        @Override public void stop(int stream) {
            stops.add(stream);
            active.remove(stream);
        }

        int startsFor(String key) {
            int count = 0;
            for (Start start : starts) if (start.key.equals(key)) ++count;
            return count;
        }

        int successfulStartsFor(String key) {
            int count = 0;
            for (Start start : starts) if (start.key.equals(key) && start.stream > 0) ++count;
            return count;
        }

        Start lastStart(String key) {
            for (int i = starts.size() - 1; i >= 0; --i)
                if (starts.get(i).key.equals(key)) return starts.get(i);
            throw new AssertionError("No start recorded for " + key);
        }
    }

    private static long[] packet(long generation, long reset, long tick,
                                 long creak, long fade, long presented) {
        return new long[] {generation, reset, tick, creak, fade, presented};
    }

    @Test public void liveMixIsIndependentAndZeroRetiresOnlyItsOwnedVoice() {
        FakeSink sink = new FakeSink();
        MenuAmbiencePlayback playback = new MenuAmbiencePlayback(sink);
        assertTrue(playback.setMixPercent(7, 21));
        assertTrue(playback.update(packet(4, 2, 10, 0, 0, 1), 4, true, true, 0.7f));
        int room = sink.lastStart(MenuAmbiencePlayback.MENU_ROOM).stream;
        assertFalse(playback.setMixPercent(-1, 21));
        assertFalse(playback.setMixPercent(101, 21));
        assertTrue(playback.setMixPercent(0, 21));
        assertTrue(playback.update(packet(4, 2, 11, 1, 0, 1), 4, true, true, 0.7f));
        int chain = sink.lastStart(MenuAmbiencePlayback.MENU_CHAIN).stream;
        assertFalse(sink.active.contains(room));
        assertTrue(sink.active.contains(chain));
        assertEquals(0.147f, sink.gains.get(chain), 0.000001f);
        assertTrue(playback.setMixPercent(10, 0));
        assertTrue(playback.update(packet(4, 2, 12, 1, 0, 1), 4, true, true, 0.7f));
        assertEquals(2, sink.successfulStartsFor(MenuAmbiencePlayback.MENU_ROOM));
        assertFalse(sink.active.contains(chain));
        assertEquals(0.07f, sink.lastStart(MenuAmbiencePlayback.MENU_ROOM).gain, 0.000001f);
        assertTrue(playback.setMixPercent(10, 25));
        assertTrue(playback.update(packet(4, 2, 13, 2, 500, 1), 4, true, true, 0.7f));
        assertEquals(0.035f, sink.gains.get(sink.lastStart(MenuAmbiencePlayback.MENU_ROOM).stream), 0.000001f);
        assertEquals(0.0875f, sink.lastStart(MenuAmbiencePlayback.MENU_CHAIN).gain, 0.000001f);
        playback.close();
        assertFalse(playback.setMixPercent(10, 25));
        assertTrue(sink.active.isEmpty());
    }

    @Test public void repeatedMenuPublicationsKeepExactlyOneLoop() {
        FakeSink sink = new FakeSink();
        MenuAmbiencePlayback playback = new MenuAmbiencePlayback(sink);
        long[] state = packet(4, 2, 10, 0, 0, 1);
        assertTrue(playback.update(state, 4, true, true, 0.5f));
        assertTrue(playback.update(state, 4, true, true, 0.5f));
        assertTrue(playback.update(packet(4, 2, 11, 0, 0, 1), 4, true, true, 0.5f));

        assertEquals(1, sink.successfulStartsFor(MenuAmbiencePlayback.MENU_ROOM));
        assertEquals(0, sink.startsFor(MenuAmbiencePlayback.MENU_CHAIN));
        assertTrue(sink.lastStart(MenuAmbiencePlayback.MENU_ROOM).looping);
        assertEquals(0.035f, sink.lastStart(MenuAmbiencePlayback.MENU_ROOM).gain, 0.000001f);
    }

    @Test public void fadeScalesAllOwnedSoundsAndPlayStopsThemIdempotently() {
        FakeSink sink = new FakeSink();
        MenuAmbiencePlayback playback = new MenuAmbiencePlayback(sink);
        playback.update(packet(2, 0, 1, 0, 0, 1), 2, true, true, 1.0f);
        playback.update(packet(2, 0, 2, 1, 250, 1), 2, true, true, 1.0f);

        assertEquals(0.0525f, sink.gains.get(sink.lastStart(MenuAmbiencePlayback.MENU_ROOM).stream), 0.000001f);
        assertEquals(0.1575f, sink.lastStart(MenuAmbiencePlayback.MENU_CHAIN).gain, 0.000001f);
        assertFalse(sink.lastStart(MenuAmbiencePlayback.MENU_CHAIN).looping);
        playback.update(packet(2, 0, 3, 1, 500, 1), 2, true, true, 1.0f);
        assertEquals(0.105f, sink.gains.get(sink.lastStart(MenuAmbiencePlayback.MENU_CHAIN).stream), 0.000001f);

        assertTrue(playback.update(packet(2, 0, 4, 1, 250, 1), 2, false, true, 1.0f));
        int stoppedAfterPlay = sink.stops.size();
        assertEquals(2, stoppedAfterPlay);
        assertTrue(sink.active.isEmpty());
        playback.stop();
        playback.stop();
        assertEquals(stoppedAfterPlay, sink.stops.size());
        assertTrue(sink.active.isEmpty());
    }

    @Test public void muteFocusLossAndCompletedFadeStopAndRebaseCreaks() {
        FakeSink sink = new FakeSink();
        MenuAmbiencePlayback playback = new MenuAmbiencePlayback(sink);
        playback.update(packet(1, 0, 0, 0, 0, 1), 1, true, true, 1.0f);
        assertTrue(playback.update(packet(1, 0, 1, 1, 0, 1), 1, true, false, 1.0f));
        assertTrue(sink.active.isEmpty());
        playback.update(packet(1, 0, 2, 2, 0, 1), 1, true, true, 1.0f);
        assertEquals(0, sink.startsFor(MenuAmbiencePlayback.MENU_CHAIN));

        playback.update(packet(1, 0, 3, 3, 0, 1), 1, true, true, 0.0f);
        assertTrue(sink.active.isEmpty());
        playback.update(packet(1, 0, 4, 4, 1000, 1), 1, true, true, 1.0f);
        assertTrue(sink.active.isEmpty());
        playback.update(packet(1, 0, 5, 5, 0, 1), 1, true, true, 1.0f);
        assertEquals(0, sink.startsFor(MenuAmbiencePlayback.MENU_CHAIN));
        playback.update(packet(1, 0, 6, 6, 0, 0), 1, true, true, 1.0f);
        assertTrue(sink.active.isEmpty());
        playback.update(packet(1, 0, 7, 7, 0, 1), 1, true, true, 1.0f);
        assertEquals(0, sink.startsFor(MenuAmbiencePlayback.MENU_CHAIN));
        playback.update(packet(1, 0, 8, 12, 0, 1), 1, true, true, 1.0f);
        assertEquals(1, sink.successfulStartsFor(MenuAmbiencePlayback.MENU_CHAIN));
        playback.update(packet(1, 0, 9, 12, 1000, 1), 1, true, true, 1.0f);
        assertTrue(sink.active.isEmpty());
        playback.update(packet(1, 0, 10, 13, 0, 1), 1, true, true, 1.0f);
        assertEquals(1, sink.startsFor(MenuAmbiencePlayback.MENU_CHAIN));
    }

    @Test public void invalidAndStalePublicationsStopWithoutReplayingMissedCreaks() {
        FakeSink sink = new FakeSink();
        MenuAmbiencePlayback playback = new MenuAmbiencePlayback(sink);
        playback.update(packet(3, 0, 5, 0, 0, 1), 3, true, true, 1.0f);
        assertFalse(playback.update(new long[5], 3, true, true, 1.0f));
        assertTrue(sink.active.isEmpty());
        assertTrue(playback.update(packet(3, 0, 6, 4, 0, 1), 3, true, true, 1.0f));
        assertEquals(0, sink.startsFor(MenuAmbiencePlayback.MENU_CHAIN));

        assertFalse(playback.update(packet(3, 0, 4, 4, 0, 1), 3, true, true, 1.0f));
        assertTrue(sink.active.isEmpty());
        playback.update(packet(3, 0, 7, 5, 0, 1), 3, true, true, 1.0f);
        assertEquals(0, sink.startsFor(MenuAmbiencePlayback.MENU_CHAIN));
        assertFalse(playback.update(packet(3, 0, 8, 6, 0, 1), 4, true, true, 1.0f));
        assertTrue(sink.active.isEmpty());
        assertFalse(playback.update(packet(3, 0, 8, 6, 1001, 1), 3, true, true, 1.0f));
        assertFalse(playback.update(packet(3, -1, 9, 6, 0, 1), 3, true, true, 1.0f));
        assertFalse(playback.update(packet(3, 0, 9, 6, 0, 2), 3, true, true, 1.0f));
        assertFalse(playback.update(packet(3, 0, 9, 6, 0, 1), 3, true, true, Float.NaN));
        assertTrue(sink.active.isEmpty());

        playback.update(packet(4, 1, 0, 12, 0, 1), 4, true, true, 1.0f);
        assertEquals(0, sink.startsFor(MenuAmbiencePlayback.MENU_CHAIN));
        playback.update(packet(4, 1, 1, 13, 0, 1), 4, true, true, 1.0f);
        assertEquals(1, sink.successfulStartsFor(MenuAmbiencePlayback.MENU_CHAIN));
    }

    @Test public void delayedLoopStartRetriesOnlyMissingLoopAndDroppedCreakIsNotReplayed() {
        FakeSink sink = new FakeSink();
        sink.failedStartsRemaining.put(MenuAmbiencePlayback.MENU_ROOM, 1);
        sink.failedStartsRemaining.put(MenuAmbiencePlayback.MENU_CHAIN, 1);
        MenuAmbiencePlayback playback = new MenuAmbiencePlayback(sink);
        playback.update(packet(8, 0, 1, 0, 0, 1), 8, true, true, 1.0f);
        assertEquals(0, sink.successfulStartsFor(MenuAmbiencePlayback.MENU_ROOM));
        playback.update(packet(8, 0, 2, 0, 0, 1), 8, true, true, 1.0f);
        assertEquals(1, sink.successfulStartsFor(MenuAmbiencePlayback.MENU_ROOM));

        playback.update(packet(8, 0, 3, 1, 0, 1), 8, true, true, 1.0f);
        assertEquals(1, sink.startsFor(MenuAmbiencePlayback.MENU_CHAIN));
        playback.update(packet(8, 0, 4, 1, 0, 1), 8, true, true, 1.0f);
        assertEquals(1, sink.startsFor(MenuAmbiencePlayback.MENU_CHAIN));
        playback.update(packet(8, 0, 5, 2, 0, 1), 8, true, true, 1.0f);
        assertEquals(2, sink.startsFor(MenuAmbiencePlayback.MENU_CHAIN));
        assertEquals(1, sink.successfulStartsFor(MenuAmbiencePlayback.MENU_CHAIN));
    }

    @Test public void closeStopsEveryOwnedStreamAndIsIdempotent() {
        FakeSink sink = new FakeSink();
        MenuAmbiencePlayback playback = new MenuAmbiencePlayback(sink);
        playback.update(packet(6, 0, 1, 0, 0, 1), 6, true, true, 1.0f);
        playback.update(packet(6, 0, 2, 1, 0, 1), 6, true, true, 1.0f);
        assertEquals(2, sink.active.size());
        playback.close();
        int stops = sink.stops.size();
        playback.close();
        playback.stop();
        assertEquals(stops, sink.stops.size());
        assertTrue(sink.active.isEmpty());
        assertFalse(playback.update(packet(6, 0, 3, 2, 0, 1), 6, true, true, 1.0f));
        assertEquals(2, sink.starts.size());
    }
}