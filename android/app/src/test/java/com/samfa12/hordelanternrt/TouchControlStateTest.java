package com.samfa12.hordelanternrt;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import org.junit.Test;

public final class TouchControlStateTest {
    @Test public void wholeCancelClearsBothRolesRegardlessOfLookFirstOrMoveFirstOrder() {
        assertWholeCancel(new int[]{7, 3}); // Look pointer arrived first; movement second.
        assertWholeCancel(new int[]{3, 7}); // Movement pointer arrived first; look second.
    }

    @Test public void pointerUpStillReleasesOnlyTheLiftedRole() {
        int[] pointers = {3, 7};
        float[] controls = controls();
        TouchControlState.finishPointer(pointers, controls, 7);
        assertArrayEquals(new int[]{3, -1}, pointers);
        assertEquals(0.8f, controls[7], 0.0f);
        assertEquals(-0.6f, controls[8], 0.0f);
    }

    @Test public void lifecycleAndMenuCleanupClearAssignmentsAndMovementAxes() {
        for (int[] order : new int[][]{{7,3}, {3,7}}) {
            int[] pointers = order.clone();
            float[] controls = controls();
            TouchControlState.cancelGesture(pointers, controls);
            assertArrayEquals(new int[]{-1,-1}, pointers);
            assertEquals(0.0f, controls[7], 0.0f);
            assertEquals(0.0f, controls[8], 0.0f);
        }
    }

    private static void assertWholeCancel(int[] order) {
        int[] pointers = order.clone();
        float[] controls = controls();
        TouchControlState.cancelGesture(pointers, controls);
        assertArrayEquals(new int[]{-1,-1}, pointers);
        assertEquals(0.0f, controls[7], 0.0f);
        assertEquals(0.0f, controls[8], 0.0f);
    }

    private static float[] controls() {
        float[] controls = new float[9];
        controls[7] = 0.8f;
        controls[8] = -0.6f;
        return controls;
    }
}
