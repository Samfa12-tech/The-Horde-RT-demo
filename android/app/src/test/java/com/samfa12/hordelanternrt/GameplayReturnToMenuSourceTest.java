package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import org.junit.Test;

/** Native scene handoff stays in the existing Entry presentation protocol. */
public final class GameplayReturnToMenuSourceTest {
    private static String source() throws Exception {
        return new String(Files.readAllBytes(java.nio.file.Paths.get("src/main/java/com/samfa12/hordelanternrt/MainActivity.java")), StandardCharsets.UTF_8);
    }
    private static String method(String source, String start, String next) {
        int a=source.indexOf(start), b=source.indexOf(next,a+start.length());
        assertTrue(start,a>=0 && b>a);
        return source.substring(a,b);
    }
    @Test public void gameplayActionsReturnButEntryMoreStillExits() throws Exception {
        String s=source();
        assertEquals(3,s.split(java.util.regex.Pattern.quote("getString(R.string.quit_to_main_menu), this::returnGameplayToMainMenu"),-1).length-1);
        assertTrue(s.contains("addEntryMenuButton(panel, getString(R.string.quit), this::finishAndRemoveTask, false)"));
    }
    @Test public void returnKeepsStorageAndRequiresFreshEntryPresentation() throws Exception {
        String body=method(source(),"private void returnGameplayToMainMenu()", "private boolean isReducedMotionEnabled()");
        assertTrue(body.contains("campaignBusy()"));
        assertTrue(body.indexOf("setGameplayPaused(true)")<body.indexOf("entryMenuEnabled=true"));
        assertTrue(body.contains("clearTouchState()"));
        assertTrue(body.contains("deathOverlayVisible=false;endingOverlayVisible=false"));
        assertTrue(body.contains("entryMenuAdmittedGeneration=0"));
        assertTrue(body.indexOf("publishEntryMenuState()")<body.indexOf("showEntryMenu(false)"));
        for(String forbidden:new String[]{"resetRoute()","restartAfterDeath()","finishAndRemoveTask()","configureCampaign(",".edit()"})
            assertFalse(forbidden,body.contains(forbidden));
    }
    @Test public void oldDeathAndEndingPollsCannotReplaceTheEntryPage() throws Exception {
        String s=source();
        assertTrue(method(s,"private void showDeathOverlay()", "private void renderDeathOverlay()").contains("if (entryMenuEnabled ||"));
        assertTrue(method(s,"private void showEndingOverlay()", "private boolean persistRtLabUnlockIfEligible()").contains("if (entryMenuEnabled ||"));
    }
}
