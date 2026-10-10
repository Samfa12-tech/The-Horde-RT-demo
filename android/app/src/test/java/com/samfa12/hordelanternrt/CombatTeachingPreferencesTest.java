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

    @Test public void promptsUseShortShapeAndMotionCuesWithTouchAndControllerControls() {
        long parryNow = 2L | (2L << 3) | (1L << 10) | (0xffL << 16) | (0xffL << 40);
        String prompt = CombatTeachingPreferences.prompt(parryNow);
        assertTrue(prompt.contains("PARRY NOW"));
        assertTrue(prompt.contains("raise your guard"));
        assertTrue(prompt.contains("[Parry]"));
        assertTrue(prompt.contains("[Dodge]"));
        assertFalse(prompt.contains("COMBAT TEACHING"));
        assertFalse(prompt.contains("######"));
        assertFalse(prompt.contains("Retry safe"));
        long parryActive = 2L | (8L << 3) | (1L << 10) | (0xffL << 40);
        prompt = CombatTeachingPreferences.prompt(parryActive);
        assertTrue(prompt.contains("● Guard up"));
        assertTrue(prompt.contains("face the blade"));
        assertFalse(prompt.contains("PARRY NOW"));

        long dodgeNow = 1L | (5L << 3) | (1L << 10) | (1L << 14) | (0xffL << 16) | (0xffL << 40);
        prompt = CombatTeachingPreferences.prompt(dodgeNow);
        assertTrue(prompt.contains("← DODGE NOW →"));
        assertFalse(prompt.contains("TIME EASED"));
        assertFalse(prompt.contains("######"));

        long keeperWindup = 1L | (4L << 3) | (1L << 10) | (0xffL << 40);
        prompt = CombatTeachingPreferences.prompt(keeperWindup);
        assertTrue(prompt.contains("The Keeper gathers lightning"));
        assertTrue(prompt.contains("watch the staff"));
        assertFalse(prompt.contains("draw back"));
    }

    @Test public void shortPromptsWrapWithoutLosingGlyphsAtLargeFontScale() {
        android.text.TextPaint paint = new android.text.TextPaint();
        paint.setTextSize(28f); // 14sp at an explicit 2x host font scale.
        for (int width : new int[]{240, 480}) for (boolean controller : new boolean[]{false, true}) {
            for (int cue : new int[]{1,2,4,5,8}) {
                String text=CombatTeachingPreferences.prompt((cue<<3)|(1L<<10)|(0xffL<<40),controller);
                android.text.StaticLayout layout=android.text.StaticLayout.Builder.obtain(text,0,text.length(),paint,width)
                    .setAlignment(android.text.Layout.Alignment.ALIGN_CENTER).setIncludePad(true).build();
                assertEquals(text.length(),layout.getLineEnd(layout.getLineCount()-1));
                for(int line=0;line<layout.getLineCount();++line)
                    assertTrue("glyph line fits finite portrait/landscape host width",layout.getLineWidth(line)<=width+1);
            }
        }
    }

    @Test public void activeControllerGlyphsReplaceTouchActions() {
        long state = (5L << 3) | (1L << 10) | (0xffL << 40);
        String controller = CombatTeachingPreferences.prompt(state, true);
        assertTrue(controller.contains("[LT]") && controller.contains("[B] + stick"));
        assertFalse(controller.contains("[Parry]"));
        assertFalse(CombatTeachingPreferences.prompt(state, false).contains("[LT]"));
    }

    @Test public void nativeDiagnosticProgressDoesNotLeakIntoTheGameplayPrompt() {
        int[] nativeBytes = {0, 64, 128, 191, 255};
        String[] expectedBars = {"[------]", "[##----]", "[###---]", "[####--]", "[######]"};
        for (int i = 0; i < nativeBytes.length; ++i) {
            long packed = 1L | (4L << 3) | (1L << 10) |
                    ((long) nativeBytes[i] << 16) | (0xffL << 40);
            String prompt = CombatTeachingPreferences.prompt(packed);
            assertTrue("native normalized byte " + nativeBytes[i], !prompt.contains(expectedBars[i]));
            assertTrue(prompt.contains("[Parry]") && prompt.contains("[Dodge]"));
        }
    }

    @Test public void timelyRefreshIncludesShortParryAndDodgeActionWindows() {
        for (int cue : new int[]{1, 2, 3, 4, 5, 6, 8}) {
            long active = (cue << 3) | (1L << 10) | (0xffL << 40);
            assertTrue(CombatTeachingPreferences.needsTimelyRefresh(active));
        }
        assertFalse(CombatTeachingPreferences.needsTimelyRefresh(
                (7L << 3) | (1L << 10) | (0xffL << 40)));
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
