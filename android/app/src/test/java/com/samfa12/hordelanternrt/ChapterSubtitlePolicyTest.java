package com.samfa12.hordelanternrt;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class ChapterSubtitlePolicyTest {
    @Test public void automaticPlacementUsesTopWhenItFitsAndBottomWhenTopIsOccupied() {
        assertEquals(MainActivity.SUBTITLE_POSITION_TOP,
                MainActivity.resolveChapterSubtitlePosition(MainActivity.SUBTITLE_POSITION_AUTO,
                        240, 180, 120));
        assertEquals(MainActivity.SUBTITLE_POSITION_BOTTOM,
                MainActivity.resolveChapterSubtitlePosition(MainActivity.SUBTITLE_POSITION_AUTO,
                        80, 180, 120));
    }

    @Test public void explicitPositionNeverFlipsAndUnfitAutoStillReportsTopForOverflowHandling() {
        assertEquals(MainActivity.SUBTITLE_POSITION_TOP,
                MainActivity.resolveChapterSubtitlePosition(MainActivity.SUBTITLE_POSITION_TOP,
                        30, 240, 120));
        assertEquals(MainActivity.SUBTITLE_POSITION_BOTTOM,
                MainActivity.resolveChapterSubtitlePosition(MainActivity.SUBTITLE_POSITION_BOTTOM,
                        240, 30, 120));
        assertEquals(MainActivity.SUBTITLE_POSITION_TOP,
                MainActivity.resolveChapterSubtitlePosition(MainActivity.SUBTITLE_POSITION_AUTO,
                        30, 30, 120));
    }

    @Test public void subtitleSizeIsBoundedToTheSupportedAccessibilityRange() {
        assertEquals(80, MainActivity.clampChapterSubtitleSize(0));
        assertEquals(121, MainActivity.clampChapterSubtitleSize(121));
        assertEquals(160, MainActivity.clampChapterSubtitleSize(500));
    }

    @Test public void overflowViewportNeverExceedsSelectedSafeRegion() {
        assertEquals(96, MainActivity.chapterSubtitleViewportHeight(180, 84, 32));
        assertEquals(0, MainActivity.chapterSubtitleViewportHeight(115, 84, 32));
        assertEquals(0, MainActivity.chapterSubtitleViewportHeight(80, 84, 32));
    }

    @Test public void limitationNoticeIsShownOnlyWhenItsWholeLabelFits() {
        assertTrue(MainActivity.chapterSubtitleLimitationFits(48, 32, 8));
        assertFalse(MainActivity.chapterSubtitleLimitationFits(39, 32, 8));
    }
}
