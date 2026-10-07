package com.samfa12.hordelanternrt;

/** Validates the compact, generation-tagged native entry-menu snapshot. */
final class EntryMenuState {
    static final int PROFILE_SHOWCASE = 0;
    static final int PROFILE_PREVIEW = 1;
    static final int PROFILE_ENTRY = 2;

    static final int PHASE_IDLE = 0;
    static final int PHASE_FADING = 1;
    static final int PHASE_LOADING = 2;
    static final int PHASE_SHOWCASE_PRESENTED = 3;
    static final int PHASE_FAILURE = 4;

    final long surfaceGeneration;
    final int profile;
    final int phase;
    final int fadePermille;
    final boolean currentProfilePresented;

    private EntryMenuState(long surfaceGeneration, int profile, int phase,
            int fadePermille, boolean currentProfilePresented) {
        this.surfaceGeneration = surfaceGeneration;
        this.profile = profile;
        this.phase = phase;
        this.fadePermille = fadePermille;
        this.currentProfilePresented = currentProfilePresented;
    }

    static EntryMenuState decode(long[] values) {
        if (values == null || values.length != 5 || values[0] <= 0 ||
                values[1] < PROFILE_SHOWCASE || values[1] > PROFILE_ENTRY ||
                values[2] < PHASE_IDLE || values[2] > PHASE_FAILURE ||
                values[3] < 0 || values[3] > 1000 ||
                (values[4] != 0 && values[4] != 1)) return null;
        return new EntryMenuState(values[0], (int)values[1], (int)values[2],
                (int)values[3], values[4] == 1);
    }

    static boolean shouldShowAtLaunch(boolean firstLaunch, boolean debugAutostart,
            boolean debugCaptureUiSuppressed, boolean checkpointPending,
            boolean capturePending, boolean replayPending,
            boolean benchmarkAutomation, boolean debugRtLabAccess) {
        return firstLaunch && !debugAutostart && !debugCaptureUiSuppressed &&
                !checkpointPending && !capturePending && !replayPending &&
                !benchmarkAutomation && !debugRtLabAccess;
    }

    boolean isCurrent(long generation) {
        return generation > 0 && surfaceGeneration == generation;
    }

    boolean isEntryPresented(long generation) {
        return isCurrent(generation) && profile == PROFILE_ENTRY && currentProfilePresented;
    }

    boolean completedPlay(long generation) {
        return isCurrent(generation) && phase == PHASE_SHOWCASE_PRESENTED &&
                profile == PROFILE_SHOWCASE && currentProfilePresented;
    }

    static boolean shouldResetFailedAttempt(int phase, boolean alreadyHandled) {
        return phase == PHASE_FAILURE && !alreadyHandled;
    }

    static boolean shouldCancelPendingPlayOnBack(boolean entryEnabled, boolean playRequested) {
        return entryEnabled && playRequested;
    }

    private EntryMenuState() { throw new AssertionError(); }
}
