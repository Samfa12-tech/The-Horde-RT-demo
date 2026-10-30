package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34)
public final class PlayerDebugCheckpointTest {
    @Test public void torchParryUsesSharedCheckpointAndCapturedView() {
        assertEquals(160, MainActivity.checkpointId("player-torch-parry-clearance"));
        assertArrayEquals(new float[]{-1.561293f, -0.04f},
                MainActivity.developmentCheckpointViewPose(160), 0.000001f);
        assertEquals(-1, MainActivity.checkpointId("player-torch-parry-clearance-unknown"));
        assertNull(MainActivity.developmentCheckpointViewPose(164));
    }

    @Test public void verticalProofCheckpointsUseTheirSharedIds() {
        assertEquals(170, MainActivity.checkpointId("vertical-proof-ground"));
        assertEquals(171, MainActivity.checkpointId("vertical-proof-raised"));
        assertEquals(-1, MainActivity.checkpointId("vertical-proof-unknown"));
    }

    @Test public void verticalFeedbackProofIsDebugCheckpointGated() {
        assertTrue(MainActivity.enablesVerticalProofFeedback(true, 170));
        assertTrue(MainActivity.enablesVerticalProofFeedback(true, 171));
        assertFalse(MainActivity.enablesVerticalProofFeedback(false, 170));
        assertFalse(MainActivity.enablesVerticalProofFeedback(true, 160));
        assertFalse(MainActivity.enablesVerticalProofFeedback(true, -1));
    }

    @Test public void delayedFallForwardsTheCapturedVerticalTuple() throws IOException {
        Path sourcePath = Paths.get("src/main/java/com/samfa12/hordelanternrt/MainActivity.java");
        if (!Files.exists(sourcePath)) {
            sourcePath = Paths.get("android/app/src/main/java/com/samfa12/hordelanternrt/MainActivity.java");
        }
        final String source = new String(Files.readAllBytes(sourcePath), StandardCharsets.UTF_8);
        final int fallCase = source.indexOf("case PLATFORM_EVENT_ENEMY_DEFEATED:");
        assertTrue(fallCase >= 0);
        final int nextCase = source.indexOf("case PLATFORM_EVENT_LICH_CHARGE_STARTED:", fallCase);
        assertTrue(nextCase > fallCase);
        final String delayedFall = source.substring(fallCase, nextCase);
        assertTrue(delayedFall.contains("feedbackGeneration == delayedGameplayFeedbackGeneration"));
        assertTrue(delayedFall.contains("stereoGains, verticalMetadata"));
    }
}
