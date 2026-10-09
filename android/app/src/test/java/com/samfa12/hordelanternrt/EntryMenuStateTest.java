package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;

import org.junit.Test;

public final class EntryMenuStateTest {
    @Test public void debugAndAutomationLaunchesKeepTheirExistingRoute() {
        assertTrue(EntryMenuState.shouldShowAtLaunch(true, false, false,
                false, false, false, false, false));
        assertFalse(EntryMenuState.shouldShowAtLaunch(false, false, false,
                false, false, false, false, false));
        assertFalse(EntryMenuState.shouldShowAtLaunch(true, true, false,
                false, false, false, false, false));
        assertFalse(EntryMenuState.shouldShowAtLaunch(true, false, true,
                false, false, false, false, false));
        assertFalse(EntryMenuState.shouldShowAtLaunch(true, false, false,
                true, false, false, false, false));
        assertFalse(EntryMenuState.shouldShowAtLaunch(true, false, false,
                false, true, false, false, false));
        assertFalse(EntryMenuState.shouldShowAtLaunch(true, false, false,
                false, false, true, false, false));
        assertFalse(EntryMenuState.shouldShowAtLaunch(true, false, false,
                false, false, false, true, false));
        assertFalse(EntryMenuState.shouldShowAtLaunch(true, false, false,
                false, false, false, false, true));
    }

    @Test public void requiresExactCoherentSnapshotShapeAndRanges() {
        assertNull(EntryMenuState.decode(null));
        assertNull(EntryMenuState.decode(new long[4]));
        assertNull(EntryMenuState.decode(new long[6]));
        assertNull(EntryMenuState.decode(new long[]{1, 3, 0, 0, 1}));
        assertNull(EntryMenuState.decode(new long[]{1, 2, 5, 0, 1}));
        assertNull(EntryMenuState.decode(new long[]{1, 2, 0, 1001, 1}));
        assertNull(EntryMenuState.decode(new long[]{1, 2, 0, 0, 2}));
        assertNull(EntryMenuState.decode(new long[]{0, 2, 0, 0, 1}));
    }

    @Test public void onlyCurrentPresentedShowcaseCompletesPlay() {
        EntryMenuState idle = EntryMenuState.decode(new long[]{41, 2, 0, 0, 1});
        assertNotNull(idle);
        assertTrue(idle.isEntryPresented(41));
        assertFalse(idle.completedPlay(41));

        EntryMenuState loading = EntryMenuState.decode(new long[]{41, 0, 2, 850, 0});
        assertFalse(loading.isEntryPresented(41));
        assertFalse(loading.completedPlay(41));

        EntryMenuState stale = EntryMenuState.decode(new long[]{40, 0, 3, 1000, 1});
        assertFalse(stale.completedPlay(41));

        EntryMenuState current = EntryMenuState.decode(new long[]{41, 0, 3, 1000, 1});
        assertTrue(current.completedPlay(41));
    }

    @Test public void presentedFlagAndProfileAreBothRequired() {
        EntryMenuState notPresented = EntryMenuState.decode(new long[]{8, 0, 3, 1000, 0});
        EntryMenuState wrongProfile = EntryMenuState.decode(new long[]{8, 1, 3, 1000, 1});
        assertFalse(notPresented.completedPlay(8));
        assertFalse(wrongProfile.completedPlay(8));
    }

    @Test public void failedNativeAttemptIsResetOnceBeforeRetryAndBackCancelsPendingPlay() {
        assertTrue(EntryMenuState.shouldResetFailedAttempt(EntryMenuState.PHASE_FAILURE, false));
        assertFalse(EntryMenuState.shouldResetFailedAttempt(EntryMenuState.PHASE_FAILURE, true));
        assertFalse(EntryMenuState.shouldResetFailedAttempt(EntryMenuState.PHASE_LOADING, false));
        assertTrue(EntryMenuState.shouldCancelPendingPlayOnBack(true, true));
        assertFalse(EntryMenuState.shouldCancelPendingPlayOnBack(true, false));
        assertFalse(EntryMenuState.shouldCancelPendingPlayOnBack(false, true));
    }
}
