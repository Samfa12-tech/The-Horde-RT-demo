package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34)
public final class GrateDebugCheckpointTest {
    @Test public void grateCapturesPublishTheSharedLegalViewingAngles() {
        final String[] names = {"layout-c-wall-panel-upward", "wall-panel-bottom",
                "wall-panel-bottom-left", "wall-panel-bottom-right"};
        final int[] ids = {152, 161, 162, 163};
        final float[][] poses = {{3.1415927f, 0.28f}, {3.1415927f, -0.32f},
                {2.646f, -0.32f}, {3.637f, -0.32f}};
        for (int index = 0; index < names.length; ++index) {
            assertEquals(names[index], ids[index], MainActivity.checkpointId(names[index]));
            assertArrayEquals(names[index], poses[index],
                    MainActivity.developmentCheckpointViewPose(ids[index]), 0.000001f);
        }
        assertEquals(-1, MainActivity.checkpointId("wall-panel-bottom-unknown"));
        assertNull(MainActivity.developmentCheckpointViewPose(164));
    }
}
