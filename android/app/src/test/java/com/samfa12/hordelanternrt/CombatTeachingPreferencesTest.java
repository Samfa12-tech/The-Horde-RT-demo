package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;

import android.content.Context;
import android.content.SharedPreferences;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk=34)
public final class CombatTeachingPreferencesTest {
    private SharedPreferences preferences;

    @Before public void setUp() {
        preferences = RuntimeEnvironment.getApplication().getSharedPreferences("teaching-test", Context.MODE_PRIVATE);
        preferences.edit().clear().commit();
    }

    @Test public void defaultsAreOptionalLessonOnAndSlowdownOffWithoutMutatingPreferences() {
        CombatTeachingPreferences.Values values = CombatTeachingPreferences.read(preferences);
        assertTrue(values.enabled);
        assertFalse(values.slowdown);
        assertTrue(preferences.getAll().isEmpty());
    }

    @Test public void savedSettingsRoundTripAndMalformedTypesUseSafeDefaults() {
        CombatTeachingPreferences.save(preferences, new CombatTeachingPreferences.Values(false, true));
        CombatTeachingPreferences.Values values = CombatTeachingPreferences.read(preferences);
        assertFalse(values.enabled);
        assertTrue(values.slowdown);
        preferences.edit().putString(CombatTeachingPreferences.ENABLED, "bad")
                .putString(CombatTeachingPreferences.SLOWDOWN, "bad").commit();
        values = CombatTeachingPreferences.read(preferences);
        assertTrue(values.enabled);
        assertFalse(values.slowdown);
    }

    @Test public void promptsUseShapeAndMotionCuesWithBoundedProgressAndSafeControls() {
        long parryNow = 2L | (2L << 3) | (1L << 10) | (0xffL << 16) | (0xffL << 40);
        String prompt = CombatTeachingPreferences.prompt(parryNow);
        assertTrue(prompt.contains("PRESS PARRY"));
        assertTrue(prompt.contains("raise guard before the strike"));
        assertTrue(prompt.contains("######"));
        assertTrue(prompt.contains("Move/look live"));
        assertTrue(prompt.contains("Retry safe"));
        long parryActive = 2L | (8L << 3) | (1L << 10) | (0xffL << 40);
        prompt = CombatTeachingPreferences.prompt(parryActive);
        assertTrue(prompt.contains("● PARRY WINDOW ACTIVE"));
        assertTrue(prompt.contains("face the incoming blade"));
        assertFalse(prompt.contains("PRESS PARRY"));

        long dodgeNow = 1L | (5L << 3) | (1L << 10) | (1L << 14) | (0xffL << 16) | (0xffL << 40);
        prompt = CombatTeachingPreferences.prompt(dodgeNow);
        assertTrue(prompt.contains("← DODGE NOW →"));
        assertTrue(prompt.contains("TIME EASED"));
        assertTrue(prompt.contains("######"));

        long keeperWindup = 1L | (4L << 3) | (1L << 10) | (0xffL << 40);
        prompt = CombatTeachingPreferences.prompt(keeperWindup);
        assertTrue(prompt.contains("KEEPER CHARGING"));
        assertTrue(prompt.contains("staff glow build"));
        assertFalse(prompt.contains("draw back"));
    }

    @Test public void nativeNormalizedProgressBytesMapAcrossTheSixMarkBar() {
        int[] nativeBytes = {0, 64, 128, 191, 255};
        String[] expectedBars = {"[------]", "[##----]", "[###---]", "[####--]", "[######]"};
        for (int i = 0; i < nativeBytes.length; ++i) {
            long packed = 1L | (4L << 3) | (1L << 10) |
                    ((long) nativeBytes[i] << 16) | (0xffL << 40);
            assertTrue("native normalized byte " + nativeBytes[i],
                    CombatTeachingPreferences.prompt(packed).contains(expectedBars[i]));
        }
    }

    @Test public void timelyRefreshIsLimitedToVisibleKeeperChargeAndRecoveryCues() {
        for (int cue : new int[]{4, 5, 6}) {
            long active = (cue << 3) | (1L << 10) | (0xffL << 40);
            assertTrue(CombatTeachingPreferences.needsTimelyRefresh(active));
        }
        assertFalse(CombatTeachingPreferences.needsTimelyRefresh(
                (2L << 3) | (1L << 10) | (0xffL << 40)));
        assertFalse(CombatTeachingPreferences.needsTimelyRefresh(
                (5L << 3) | (0xffL << 40)));
        assertFalse(CombatTeachingPreferences.needsTimelyRefresh(
                (5L << 3) | (1L << 10)));
    }

    @Test public void disabledAndSkippedStatesHaveNoActiveCuePrompt() {
        assertEquals("", CombatTeachingPreferences.prompt(2L));
        assertEquals("", CombatTeachingPreferences.prompt(3L | (1L << 10) | (0xffL << 40)));
        assertEquals("", CombatTeachingPreferences.prompt(2L | (1L << 10) | (0xffL << 40)));
    }

    @Test public void debugPracticeCheckpointNamesSelectTheSameOwnerRoutedPracticeKinds() {
        assertEquals(1, MainActivity.combatPracticeKind("combat-practice-parry"));
        assertEquals(2, MainActivity.combatPracticeKind("combat-practice-dodge"));
        assertEquals(-1, MainActivity.combatPracticeKind("combat-practice-parry-capture"));
        assertEquals(-1, MainActivity.combatPracticeKind(null));
    }

    @Test public void reducedMotionRemovesOpacityAnimationButKeepsVisibleState() {
        assertEquals(0.5f, CombatTeachingPreferences.alpha(128, false), 0.01f);
        assertEquals(1.0f, CombatTeachingPreferences.alpha(128, true), 0.0f);
        assertEquals(0.0f, CombatTeachingPreferences.alpha(0, true), 0.0f);
        assertEquals(1.0f, CombatTeachingPreferences.alpha(300, false), 0.0f);
        assertEquals(128, CombatTeachingPreferences.fadeAlpha(255, 60, false));
        assertEquals(0, CombatTeachingPreferences.fadeAlpha(255, 20, true));
        assertEquals(0, CombatTeachingPreferences.fadeAlpha(255, 120, false));
        assertEquals(0, CombatTeachingPreferences.fadeAlpha(255, 120, false));
    }
}
