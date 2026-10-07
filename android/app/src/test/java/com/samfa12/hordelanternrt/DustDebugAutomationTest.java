package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;

import android.content.Intent;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34)
public final class DustDebugAutomationTest {
    private static final String DUST = "horde.debug.dust";

    @Test public void sharedDustFixturesResolveIdsAndCameraPoses() {
        final String[] names = {"dust-box-front", "dust-box-oblique", "dust-ellipsoid", "dust-box-wall"};
        final int[] ids = {156, 157, 158, 159};
        final float[][] poses = {{0.0f, 0.14f}, {0.06f, 0.14f},
                {-1.57079632679f, 0.14f}, {0.70f, -0.08f}};
        for (int index = 0; index < names.length; ++index) {
            assertEquals(names[index], ids[index], MainActivity.checkpointId(names[index]));
            final float[] actualPose = MainActivity.developmentCheckpointViewPose(ids[index]);
            assertNotNull(names[index], actualPose);
            assertEquals(names[index] + " yaw", poses[index][0], actualPose[0], 0.000001f);
            assertEquals(names[index] + " pitch", poses[index][1], actualPose[1], 0.000001f);
        }
    }

    @Test public void dustOverrideIsLimitedToDebugCheckpointCaptureOrMotionValidation() {
        for (int quality = 0; quality <= 2; ++quality) {
            final Intent checkpoint = new Intent().putExtra(DUST, quality);
            assertEquals(quality, MainActivity.admittedDebugDustQuality(
                    checkpoint, true, 156, true, false, false, false));
            final Intent motion = new Intent().putExtra(DUST, quality);
            assertEquals(quality, MainActivity.admittedDebugDustQuality(
                    motion, true, -1, false, false, false, true));
        }

        assertEquals(-1, MainActivity.admittedDebugDustQuality(
                new Intent().putExtra(DUST, 1), false, 156, true, false, false, false));
        assertEquals(-1, MainActivity.admittedDebugDustQuality(
                new Intent().putExtra(DUST, 1), true, -1, false, false, false, false));
        assertEquals(-1, MainActivity.admittedDebugDustQuality(
                new Intent().putExtra(DUST, 1), true, 156, false, false, false, false));
        assertEquals(-1, MainActivity.admittedDebugDustQuality(
                new Intent().putExtra(DUST, 1), true, 156, true, true, false, false));
        assertEquals(-1, MainActivity.admittedDebugDustQuality(
                new Intent().putExtra(DUST, 1), true, 156, true, false, true, false));
        assertEquals(-1, MainActivity.admittedDebugDustQuality(
                new Intent().putExtra(DUST, 1), true, 156, true, false, false, true));
        assertEquals(-1, MainActivity.admittedDebugDustQuality(
                new Intent().putExtra(DUST, 1).putExtra("horde.benchmark.run_id", "run"),
                true, 156, true, false, false, false));
        for (int quality : new int[]{-1, 3}) {
            assertEquals(-1, MainActivity.admittedDebugDustQuality(
                    new Intent().putExtra(DUST, quality), true, 156, true, false, false, false));
        }
        assertEquals(-1, MainActivity.admittedDebugDustQuality(
                new Intent().putExtra(DUST, "1"), true, 156, true, false, false, false));
        assertEquals(-1, MainActivity.admittedDebugDustQuality(
                new Intent(), true, 156, true, false, false, false));
    }

    @Test public void runtimeDustOverridePreservesEveryOtherActiveGraphicsSetting() {
        final GraphicsPreferences.Values current = new GraphicsPreferences.Values(
                73, 2, 1, 45, false, GraphicsPreferences.SHADOW_HIGHER, false, 1);
        final GraphicsPreferences.Values overridden = MainActivity.withDebugDustQuality(current, 2);
        assertNotNull(overridden);
        assertEquals(73, overridden.scale);
        assertEquals(2, overridden.water);
        assertEquals(1, overridden.fire);
        assertEquals(45, overridden.cap);
        assertFalse(overridden.glassEnabled);
        assertEquals(GraphicsPreferences.SHADOW_HIGHER, overridden.shadow);
        assertFalse(overridden.mistEnabled);
        assertEquals(2, overridden.dust);
        assertEquals(1, current.dust);
        assertNull(MainActivity.withDebugDustQuality(current, 3));
        assertNull(MainActivity.withDebugDustQuality(null, 1));
    }

    @Test public void dustTupleKeepsSavedScaleAndDebugScaleAdmissionRemainsBounded() {
        final GraphicsPreferences.Values saved = new GraphicsPreferences.Values(
                50, 2, 1, 45, false, GraphicsPreferences.SHADOW_HIGHER, false, 0);
        final GraphicsPreferences.Values dustTuple = MainActivity.withDebugDustQuality(saved, 2);
        assertEquals("Dust tuple must not replace the confirmed scale before the explicit scale override", 50,
                dustTuple.scale);
        assertEquals(2, dustTuple.dust);
        assertEquals(50, MainActivity.admittedDebugRenderScale(new Intent().putExtra("horde.debug.scale", 50)));
        assertEquals(100, MainActivity.admittedDebugRenderScale(new Intent().putExtra("horde.debug.scale", 100)));
        assertEquals(-1, MainActivity.admittedDebugRenderScale(new Intent().putExtra("horde.debug.scale", 49)));
        assertEquals(-1, MainActivity.admittedDebugRenderScale(new Intent().putExtra("horde.debug.scale", 101)));
        assertEquals(-1, MainActivity.admittedDebugRenderScale(new Intent()));
    }
}
