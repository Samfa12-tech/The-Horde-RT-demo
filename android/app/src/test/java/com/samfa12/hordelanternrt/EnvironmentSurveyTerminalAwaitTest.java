package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import org.junit.Test;

public final class EnvironmentSurveyTerminalAwaitTest {
    private static final class Fixture {
        long now;
        int reads, sleeps;
        long[] phases;
        Fixture(long... phases) { this.phases = phases; }
        long[] read() { return new long[]{phases[Math.min(reads++, phases.length - 1)]}; }
        void pause(long millis) { ++sleeps; now += millis; }
        int await(long timeout) {
            return EnvironmentSurveyTerminalAwait.await(this::read, () -> now, this::pause, timeout);
        }
    }

    @Test public void cancellationWaitsForNativeFailedManifestPublication() {
        Fixture f = new Fixture(1, 1, 3);
        assertEquals(3, f.await(1000));
        assertEquals(3, f.reads);
        assertEquals(2, f.sleeps);
        assertEquals(100, f.now);
    }

    @Test public void completionWinningCancellationUsesActualNativeCompletePhase() {
        Fixture f = new Fixture(1, 2);
        assertEquals(2, f.await(1000));
        assertEquals(2, f.reads);
        assertEquals(1, f.sleeps);
    }

    @Test public void alreadyCompleteOrFailedNeedsNoPause() {
        for (int phase : new int[]{2, 3}) {
            Fixture f = new Fixture(phase);
            assertEquals(phase, f.await(1000));
            assertEquals(1, f.reads);
            assertEquals(0, f.sleeps);
        }
    }

    @Test public void pendingTimeoutRemainsNonterminalAndUsesOnlyRemainingBudget() {
        Fixture f = new Fixture(1);
        assertEquals(1, f.await(125));
        assertEquals(125, f.now);
        assertEquals(3, f.reads);
        assertEquals(3, f.sleeps);
    }

    @Test public void unavailableAndMalformedStatusNeverBecomeFailureCompletion() {
        for (long[] state : new long[][]{null, {}, {-1}, {4}, {Long.MAX_VALUE}, {0}}) {
            assertEquals(0, EnvironmentSurveyTerminalAwait.await(() -> state, () -> 0,
                    ignored -> fail("Unavailable status must not sleep"), 100));
        }
    }

    @Test public void unavailableNativeRuntimeOrLinkageLeavesNonterminalStatus() {
        assertEquals(0, EnvironmentSurveyTerminalAwait.await(() -> { throw new IllegalStateException(); },
                () -> 0, ignored -> fail("Must not sleep"), 100));
        assertEquals(0, EnvironmentSurveyTerminalAwait.await(() -> { throw new UnsatisfiedLinkError(); },
                () -> 0, ignored -> fail("Must not sleep"), 100));
    }

    @Test public void clockRegressionDoesNotExtendDeadlineOrExportPayload() {
        Fixture f = new Fixture(1);
        f.now = 100;
        assertEquals(0, EnvironmentSurveyTerminalAwait.await(f::read, () -> f.now,
                ignored -> f.now = 99, 1000));
        assertEquals(1, f.reads);
    }

    @Test public void interruptionPreservesThreadFlagAndReturnsUnavailable() {
        try {
            assertEquals(0, EnvironmentSurveyTerminalAwait.await(() -> new long[]{1}, () -> 0,
                    ignored -> { throw new InterruptedException(); }, 100));
            assertTrue(Thread.currentThread().isInterrupted());
        } finally { Thread.interrupted(); }
    }

    @Test public void invalidBudgetOrClockDoesNotReadNativeStatus() {
        for (long budget : new long[]{0, -1, EnvironmentSurveyTerminalAwait.MAX_WAIT_MILLIS + 1}) {
            assertEquals(0, EnvironmentSurveyTerminalAwait.await(() -> { fail("No native read"); return null; },
                    () -> 0, ignored -> fail("No wait"), budget));
        }
        assertEquals(0, EnvironmentSurveyTerminalAwait.await(() -> { fail("No native read"); return null; },
                () -> -1, ignored -> fail("No wait"), 100));
        assertEquals(0, EnvironmentSurveyTerminalAwait.await(() -> { fail("No native read"); return null; },
                () -> { throw new IllegalStateException(); }, ignored -> fail("No wait"), 100));
    }
}
