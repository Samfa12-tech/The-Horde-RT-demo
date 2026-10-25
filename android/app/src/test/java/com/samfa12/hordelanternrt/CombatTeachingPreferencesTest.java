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
        long parryNow = 0L | (2L << 3) | (1L << 10) | (0xffL << 16) | (0xffL << 40);
        String prompt = CombatTeachingPreferences.prompt(parryNow);
        assertTrue(prompt.contains("PARRY NOW"));
        assertTrue(prompt.contains("fresh [Parry] press"));
        assertTrue(prompt.contains("[Parry]"));
        assertFalse("parry stage avoids an unrelated dodge legend",prompt.contains("[Dodge]"));
        assertFalse(prompt.contains("COMBAT TEACHING"));
        assertFalse(prompt.contains("######"));
        assertFalse(prompt.contains("Retry safe"));
        long parryActive = 0L | (8L << 3) | (1L << 10) | (0xffL << 40);
        prompt = CombatTeachingPreferences.prompt(parryActive);
        assertTrue(prompt.contains("Timed parry active"));
        assertTrue(prompt.contains("face the attacker"));
        assertFalse(prompt.contains("PARRY NOW"));

        long dodgeNow = 1L | (5L << 3) | (1L << 10) | (1L << 14) | (0xffL << 16) | (0xffL << 40);
        prompt = CombatTeachingPreferences.prompt(dodgeNow);
        assertTrue(prompt.contains("DODGE NOW"));
        assertFalse(prompt.contains("TIME EASED"));
        assertFalse(prompt.contains("######"));

        long keeperWindup = 1L | (3L << 7) | (4L << 3) | (1L << 10) | (0xffL << 40);
        prompt = CombatTeachingPreferences.prompt(keeperWindup);
        assertTrue(prompt.contains("Keeper lightning"));
        assertTrue(prompt.contains("[Dodge] + move"));
        assertFalse(prompt.contains("draw back"));
    }

    @Test public void shortPromptsWrapWithoutLosingGlyphsAtLargeFontScale() {
        android.text.TextPaint paint = new android.text.TextPaint();
        paint.setTextSize(28f); // 14sp at an explicit 2x host font scale.
        for (int width : new int[]{240, 480}) for (boolean controller : new boolean[]{false, true}) {
            for (int cue : new int[]{1,2,4,5,8,9,10,11,12,13}) {
                String text=CombatTeachingPreferences.prompt((cue<<3)|(1L<<10)|(0xffL<<40),controller);
                android.text.StaticLayout layout=android.text.StaticLayout.Builder.obtain(text,0,text.length(),paint,width)
                    .setAlignment(android.text.Layout.Alignment.ALIGN_CENTER).setIncludePad(true).build();
                assertEquals(text.length(),layout.getLineEnd(layout.getLineCount()-1));
                for(int line=0;line<layout.getLineCount();++line)
                    assertTrue("glyph line fits finite portrait/landscape host width",layout.getLineWidth(line)<=width+1);
            }
            String clearance=CombatTeachingPreferences.prompt(2L|(1L<<34),controller);
            android.text.StaticLayout layout=android.text.StaticLayout.Builder.obtain(clearance,0,clearance.length(),paint,width)
                    .setAlignment(android.text.Layout.Alignment.ALIGN_CENTER).setIncludePad(true).build();
            assertEquals(clearance.length(),layout.getLineEnd(layout.getLineCount()-1));
            for(int line=0;line<layout.getLineCount();++line)
                assertTrue("clearance hint remains complete at the existing portrait/landscape host widths",
                        layout.getLineWidth(line)<=width+1);
        }
    }

    @Test public void activeControllerGlyphsReplaceTouchActions() {
        long state = 1L | (5L << 3) | (1L << 10) | (0xffL << 40);
        String controller = CombatTeachingPreferences.prompt(state, true);
        assertTrue(controller.contains("[B] + stick"));
        assertFalse("dodge stage avoids an unrelated guard legend",controller.contains("[LT]"));
        assertTrue(CombatTeachingPreferences.prompt(4L | (1L<<10) | (0xffL<<40),true).contains("[LT]"));
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
            assertTrue(prompt.contains("[Dodge] + move"));
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

    @Test public void currentAttackCueSurvivesDisabledSkippedAndCompletedTeachingWithoutLessonSpam() {
        for (long lesson : new long[]{0L, 3L | (1L << 10), 2L | (1L << 10)}) {
            long yellow = lesson | (1L << 48), red = lesson | (2L << 48);
            assertEquals("\u25c7 YELLOW guard-break: parry/dodge", CombatTeachingPreferences.prompt(yellow));
            assertEquals("\u2715 RED: unblockable - DODGE", CombatTeachingPreferences.prompt(red,true));
            assertEquals(0xFFFFD266, CombatTeachingPreferences.promptColor(yellow));
            assertEquals(0xFFFF6B6B, CombatTeachingPreferences.promptColor(red));
            assertEquals(255, CombatTeachingPreferences.promptOpacity(yellow));
            assertEquals(255, CombatTeachingPreferences.promptOpacity(red));
            assertEquals(1f,CombatTeachingPreferences.alpha(CombatTeachingPreferences.promptOpacity(red),true),0f);
            assertTrue(CombatTeachingPreferences.description(red,"").contains("unblockable"));
            assertTrue(CombatTeachingPreferences.description(yellow,"").contains("guard-break"));
            assertTrue(CombatTeachingPreferences.needsTimelyRefresh(red));
            assertFalse(CombatTeachingPreferences.prompt(yellow).contains("[LT]"));
            assertFalse(CombatTeachingPreferences.prompt(red).contains("[Parry]"));
            assertEquals("",CombatTeachingPreferences.prompt(lesson));
        }
        assertEquals(0,CombatTeachingPreferences.attackReadability(3L << 48));
    }

    @Test public void disabledAndSkippedStatesHaveNoActiveCuePrompt() {
        assertEquals("", CombatTeachingPreferences.prompt(2L));
        assertEquals("", CombatTeachingPreferences.prompt(3L | (1L << 10) | (0xffL << 40)));
        assertEquals("", CombatTeachingPreferences.prompt(2L | (1L << 10) | (0xffL << 40)));
    }

    @Test public void fourSkillsAndFinishUseTheirPublishedStageWithoutInventingSuccess() {
        long practice=(1L<<10)|(1L<<13)|(0xffL<<40);
        for(int stage:new int[]{4,0,1,5,6}) {
            String text=CombatTeachingPreferences.prompt(practice|stage);
            assertTrue(text.startsWith("Practice: no damage\n"));
            assertEquals(stage==6?"Finish practice":"Skip practice",RuntimeEnvironment.getApplication().getString(CombatTeachingPreferences.practiceActionLabelResource(practice|stage)));
            assertFalse(CombatTeachingPreferences.blockLearned(practice|stage));
            assertFalse(CombatTeachingPreferences.attackFromGuardLearned(practice|stage));
        }
        assertTrue(CombatTeachingPreferences.prompt(practice|4).contains("block normal hits"));
        assertTrue(CombatTeachingPreferences.prompt(practice|5,true).contains("Hold [LT] + [RT]"));
        assertTrue(CombatTeachingPreferences.description(practice|5,"").contains("Attacking ends guard protection"));
        assertTrue(CombatTeachingPreferences.prompt(practice|6).contains("Settings: Finish practice"));
        assertTrue(CombatTeachingPreferences.prompt(practice|6).contains("enemies can hurt you"));
        assertTrue(CombatTeachingPreferences.blockLearned(practice|(1L<<15)));
        assertTrue(CombatTeachingPreferences.attackFromGuardLearned(practice|(1L<<32)));
        assertFalse(CombatTeachingPreferences.attackFromGuardLearned(practice|(1L<<15)));
        assertTrue(CombatTeachingPreferences.exitPending(practice|(1L<<33)));
        assertTrue(CombatTeachingPreferences.prompt(practice|(1L<<33)).contains("Leaving practice safely"));
    }
    @Test public void actualParrySuccessPrecedesCurrentSkillWithoutInventingSuccessFromAnOrdinaryHit() {
        long blockPractice=4L|(1L<<10)|(1L<<13)|(0xffL<<40);
        String parried=CombatTeachingPreferences.prompt(blockPractice|(7L<<3));
        assertTrue(parried.startsWith("Practice: no damage\n"));
        assertTrue(parried.contains("Well parried"));assertFalse(parried.contains("block normal hits"));
        assertFalse(CombatTeachingPreferences.prompt(blockPractice|(9L<<3)).contains("Well parried"));
    }
    @Test public void protectionAndCurrentSkillRemainDuringWarningsEvenWithNoLessonOpacity() {
        for(int stage:new int[]{4,0,1,5,6})for(int attack:new int[]{0,1,2}) {
            long state=(1L<<13)|stage|((long)attack<<48); // Protection publication is authoritative.
            String text=CombatTeachingPreferences.prompt(state);
            assertTrue(text.startsWith("Practice: no damage\n"));
            assertTrue(text.split("\n").length>=2);
            if(attack==1) {
                assertTrue(text.contains("YELLOW guard-break: parry/dodge"));
                assertTrue(CombatTeachingPreferences.description(state,text).contains("holding guard cannot block"));
                assertFalse(text.contains("half damage"));
            }
            if(attack==2)assertTrue(text.contains("RED: unblockable - DODGE"));
            assertEquals(255,CombatTeachingPreferences.promptOpacity(state));
            assertTrue(CombatTeachingPreferences.needsTimelyRefresh(state));
        }
    }
    @Test public void dodgeNamesOnlyTheActualPublishedAttackerAndReplayDoesNotClaimSuccess() {
        long practice=1L|(1L<<10)|(1L<<13)|(4L<<3);
        for(int source:new int[]{0,2,4})assertFalse(CombatTeachingPreferences.prompt(practice|((long)source<<7)).contains("Keeper"));
        assertTrue(CombatTeachingPreferences.prompt(practice|(2L<<7)).contains("Skeleton attack"));
        assertTrue(CombatTeachingPreferences.prompt(practice|(3L<<7)).contains("Keeper lightning"));
        assertTrue(CombatTeachingPreferences.prompt((12L<<3)|(1L<<13)).contains("Replay opening practice"));
        assertTrue(CombatTeachingPreferences.prompt((12L<<3)|(1L<<13)).contains("or Skip to continue"));
        assertFalse(CombatTeachingPreferences.prompt((12L<<3)|(1L<<13)).contains("Finish practice"));
    }

    @Test public void revivalClearanceUsesOnlyBit34WithoutClaimingProtectionOrNewLessonStages() {
        long completed=2L|(1L<<10),clearance=completed|(1L<<34);
        assertEquals("",CombatTeachingPreferences.prompt(completed));
        assertFalse(CombatTeachingPreferences.waitingForRevivalClearance(completed));
        assertTrue(CombatTeachingPreferences.waitingForRevivalClearance(clearance));
        assertEquals(2,CombatTeachingPreferences.stage(clearance));
        assertFalse(CombatTeachingPreferences.exitPending(clearance));
        assertFalse(CombatTeachingPreferences.blockLearned(clearance));
        assertFalse(CombatTeachingPreferences.attackFromGuardLearned(clearance));
        assertEquals("Step back from the skeletons.",CombatTeachingPreferences.prompt(clearance));
        assertEquals(255,CombatTeachingPreferences.promptOpacity(clearance));
        assertTrue(CombatTeachingPreferences.needsTimelyRefresh(clearance));
        assertTrue(CombatTeachingPreferences.description(clearance,CombatTeachingPreferences.prompt(clearance))
                .contains("Step back from the skeletons."));
        assertFalse(CombatTeachingPreferences.prompt(clearance).contains("Practice: no damage"));
        for(int adjacent:new int[]{32,33,35,39,40,48,49})
            assertFalse("neighboring fields must not alias clearance",CombatTeachingPreferences.waitingForRevivalClearance(1L<<adjacent));
        String withAttack=CombatTeachingPreferences.prompt(clearance|(2L<<48));
        assertTrue(withAttack.contains("Step back from the skeletons."));
        assertTrue(withAttack.contains("RED: unblockable - DODGE"));
        assertEquals(0xFFFF6B6B,CombatTeachingPreferences.promptColor(clearance|(2L<<48)));
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
