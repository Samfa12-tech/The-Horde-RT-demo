package com.samfa12.hordelanternrt;

import java.util.function.LongSupplier;

/** Worker-only wait for native image/report publication and real owner retirement. */
final class EnvironmentSurveyTerminalAwait {
    static final long MAX_WAIT_MILLIS = 30000L;
    private static final long POLL_MILLIS = 50L;
    interface StatusSource { long[] read(); }
    interface Pause { void sleep(long millis) throws InterruptedException; }

    private EnvironmentSurveyTerminalAwait() {}

    // 2/3 are terminal native facts. Timeout stays 1; unavailable stays 0.
    // The exporter rejects both nonterminal values before reading any payload.
    static int await(StatusSource source, LongSupplier clock, Pause pause, long timeoutMillis) {
        if (source == null || clock == null || pause == null ||
                timeoutMillis <= 0 || timeoutMillis > MAX_WAIT_MILLIS) return 0;
        try {
            final long start = clock.getAsLong();
            if (start < 0) return 0;
            long previous = start;
            for (;;) {
                final long now = clock.getAsLong();
                if (now < previous) return 0;
                previous = now;
                final long elapsed = now - start;
                if (elapsed >= timeoutMillis) return 1;
                final long[] state = source.read();
                if (state == null || state.length == 0 || state[0] < 0 || state[0] > 3) return 0;
                final int phase = (int)state[0];
                if (phase == 0 || phase == 2 || phase == 3) return phase;
                pause.sleep(Math.min(POLL_MILLIS, timeoutMillis - elapsed));
            }
        } catch (InterruptedException interrupted) {
            Thread.currentThread().interrupt();
            return 0;
        } catch (RuntimeException | LinkageError unavailable) {
            return 0;
        }
    }
}
