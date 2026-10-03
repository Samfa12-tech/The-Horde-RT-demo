package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import java.util.ArrayList;
import org.junit.Test;

// Injected device writes verify queue ownership, not physical AudioTrack cadence.
public final class AmbienceOutputQueueTest {
    @Test public void partialAndZeroWritesRetainEverySampleExactlyOnce() {
        AmbienceOutputQueue queue = new AmbienceOutputQueue(1440);
        ArrayList<Integer> accepted = new ArrayList<>();
        for (int block = 0; block < 10; ++block) {
            queue.observeHead((int)queue.submitted());
            queue.rendered(queue.generated());
            int[] writes = {0, 2, 38, 0, 160, 760};
            for (int count : writes) {
                assertFalse(queue.canRender());
                int start = queue.offset();
                for (int sample = 0; sample < count; ++sample)
                    accepted.add(block * 960 + start + sample);
                queue.accepted(count);
            }
            assertEquals(0, queue.remaining());
            assertEquals((block + 1) * 480L, queue.submitted());
        }
        assertEquals(9600, accepted.size());
        for (int sample = 0; sample < accepted.size(); ++sample)
            assertEquals(sample, accepted.get(sample).intValue());
    }
    @Test public void exhaustionAndInjectedUnderflowKeepCursorOwnership() {
        AmbienceOutputQueue queue = new AmbienceOutputQueue(480);
        queue.rendered(0); queue.accepted(960);
        assertFalse(queue.canRender());
        queue.observeHead(480); // Simulated sink drains everything: refill next exact block.
        assertTrue(queue.canRender());
        queue.rendered(480); queue.accepted(0);
        assertEquals(960, queue.remaining());
        assertEquals(960, queue.generated());
        assertEquals(480, queue.submitted());
        queue.accepted(960);
        assertEquals(960, queue.submitted());
    }
    @Test public void lifecycleResetDiscardsOldPartialBlockAndHead() {
        AmbienceOutputQueue queue = new AmbienceOutputQueue(1440);
        queue.rendered(0); queue.accepted(100); queue.observeHead(30);
        queue.reset();
        assertEquals(0, queue.generated()); assertEquals(0, queue.submitted());
        assertEquals(0, queue.consumed()); assertEquals(0, queue.remaining());
        queue.rendered(0); queue.accepted(960);
        assertEquals(480, queue.submitted());
    }
    @Test(expected=IllegalStateException.class) public void oddPartialFrameIsRejected() {
        AmbienceOutputQueue queue = new AmbienceOutputQueue(1440);
        queue.rendered(0); queue.accepted(1);
    }
    @Test(expected=IllegalStateException.class) public void backendFailureIsRejected() {
        AmbienceOutputQueue queue = new AmbienceOutputQueue(1440);
        queue.rendered(0); queue.accepted(-6);
    }
    @Test(expected=IllegalStateException.class) public void unexpectedHeadResetIsRejected() {
        AmbienceOutputQueue queue = new AmbienceOutputQueue(1440);
        queue.rendered(0); queue.accepted(960); queue.observeHead(200); queue.observeHead(100);
    }
    @Test(expected=IllegalStateException.class) public void generatedPcmCannotReplacePartialBlock() {
        AmbienceOutputQueue queue = new AmbienceOutputQueue(1440);
        queue.rendered(0); queue.accepted(50); queue.rendered(480);
    }
    @Test(expected=IllegalStateException.class) public void acceptedSamplesCannotExceedDeviceBound() {
        AmbienceOutputQueue queue = new AmbienceOutputQueue(200);
        queue.rendered(0); queue.accepted(960);
    }
}
