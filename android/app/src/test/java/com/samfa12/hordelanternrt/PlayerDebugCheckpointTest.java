package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
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
        assertNull(MainActivity.developmentCheckpointViewPose(161));
    }
}
