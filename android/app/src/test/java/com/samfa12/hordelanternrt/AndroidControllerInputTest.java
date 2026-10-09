package com.samfa12.hordelanternrt;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.InputDeviceBuilder;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34)
public final class AndroidControllerInputTest {
    private static final int KEY_SOURCE = InputDevice.SOURCE_GAMEPAD;
    private static final int MOTION_SOURCE = InputDevice.SOURCE_JOYSTICK;

    @Test public void mapsFreshButtonEdgesAndLeavesNonControllerEventsUntouched() {
        AndroidControllerInput input = new AndroidControllerInput();
        AndroidControllerInput.Result a = input.key(key(1, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_BUTTON_A, 0, 10), 10);
        assertTrue(a.handled);
        assertTrue(a.meaningful);
        assertEquals(AndroidControllerInput.INTERACT | AndroidControllerInput.CONFIRM, a.actions);
        assertEquals(1, input.activeDeviceId());
        assertEquals(0, input.key(key(1, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_BUTTON_A, 1, 11), 11).actions);
        assertEquals(0, input.key(key(1, KeyEvent.ACTION_UP, KeyEvent.KEYCODE_BUTTON_A, 0, 12), 12).actions);

        AndroidControllerInput.Result b = input.key(key(1, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_BUTTON_B, 0, 13), 13);
        assertEquals(AndroidControllerInput.DODGE | AndroidControllerInput.CANCEL, b.actions);
        assertEquals(AndroidControllerInput.SWING, input.key(key(1, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_BUTTON_X, 0, 14), 14).actions);
        assertEquals(AndroidControllerInput.LANTERN, input.key(key(1, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_BUTTON_Y, 0, 15), 15).actions);
        assertEquals(AndroidControllerInput.PAUSE, input.key(key(1, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_BUTTON_START, 0, 16), 16).actions);
        assertEquals(0, input.key(key(1, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_BUTTON_START, 0, 17, InputDevice.SOURCE_KEYBOARD), 17).actions);
        assertFalse(input.key(key(1, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_BUTTON_THUMBL, 0, 18), 18).handled);
    }

    @Test public void triggerAxesUseHysteresisAndDeduplicateTheirDigitalRepresentations() {
        AndroidControllerInput input = new AndroidControllerInput();
        input.motion(motion(4, 1, 0, 0, 0, 0, 0, 0), 1); // neutral baseline
        assertEquals(0, input.motion(motion(4, 2, 0, 0, 0, 0, 0, 0.49f), 2).actions);
        assertEquals(AndroidControllerInput.SWING, input.motion(motion(4, 3, 0, 0, 0, 0, 0, 0.52f), 3).actions);
        assertEquals(0, input.motion(motion(4, 4, 0, 0, 0, 0, 0, 0.40f), 4).actions);
        assertEquals(0, input.key(key(4, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_BUTTON_R2, 0, 5), 5).actions);
        assertEquals(0, input.motion(motion(4, 6, 0, 0, 0, 0, 0, 0.30f), 6).actions);
        assertEquals(0, input.key(key(4, KeyEvent.ACTION_UP, KeyEvent.KEYCODE_BUTTON_R2, 0, 7), 7).actions);
        assertEquals(0, input.motion(motion(4, 8, 0, 0, 0, 0, 0, 0.0f), 8).actions);
        assertEquals(AndroidControllerInput.SWING, input.motion(motion(4, 9, 0, 0, 0, 0, 0, 0.60f), 9).actions);

        assertEquals(AndroidControllerInput.PARRY, input.key(key(4, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_BUTTON_L2, 0, 10), 10).actions);
        assertEquals(0, input.motion(motion(4, 11, 0, 0, 0, 0, 0.8f, 0.60f), 11).actions);
    }

    @Test public void sticksApplyRadialDeadzoneAndRequireNeutralBeforeArming() {
        AndroidControllerInput input = new AndroidControllerInput();
        AndroidControllerInput.Result first = input.motion(motion(7, 1, 0.7f, 0, 0, 0, 0, 0), 1);
        assertFalse(first.meaningful);
        assertEquals(0.0f, first.moveStrafe, 0.0f);
        input.motion(motion(7, 2, 0.08f, 0.04f, 0, 0, 0, 0), 2);
        AndroidControllerInput.Result moved = input.motion(motion(7, 3, 0.8f, 0.6f, 0, 0, 0, 0), 3);
        assertTrue(moved.meaningful);
        assertEquals(0.8f, moved.moveStrafe, 0.03f);
        assertEquals(-0.6f, moved.moveForward, 0.03f);
        assertEquals(0.0f, input.motion(motion(7, 4, 0.05f, -0.04f, 0, 0, 0, 0), 4).moveStrafe, 0.0f);
        input.motion(motion(7, 5, 0, -0.8f, 0, 0, 0, 0), 5);
        assertEquals(-1, input.navigation(5).vertical); // negative=up even though forward is positive

        input.suspend();
        AndroidControllerInput.Result heldOnResume = input.motion(motion(7, 5, 0.75f, 0, 0, 0, 0, 0), 5);
        assertFalse(heldOnResume.meaningful);
        assertEquals(0.0f, heldOnResume.moveStrafe, 0.0f);
        input.motion(motion(7, 6, 0, 0, 0, 0, 0, 0), 6);
        assertTrue(input.motion(motion(7, 7, 0.4f, 0, 0, 0, 0, 0), 7).meaningful);
    }

    @Test public void navigationHasAnEdgeThenBoundedRepeatsAndReleaseStopsIt() {
        AndroidControllerInput input = new AndroidControllerInput();
        input.key(key(9, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DPAD_RIGHT, 0, 100), 100);
        AndroidControllerInput.Result edge = input.navigation(100);
        assertEquals(1, edge.navigationHorizontal);
        assertEquals(0, input.navigation(449).navigationHorizontal);
        assertEquals(1, input.navigation(450).navigationHorizontal);
        assertEquals(0, input.navigation(569).navigationHorizontal);
        assertEquals(1, input.navigation(570).navigationHorizontal);
        input.key(key(9, KeyEvent.ACTION_UP, KeyEvent.KEYCODE_DPAD_RIGHT, 0, 571), 571);
        assertEquals(0, input.navigation(571).navigationHorizontal);
    }

    @Test public void suspensionRejectsOldDownTimesAndRequiresHeldKeyRelease() {
        AndroidControllerInput input = new AndroidControllerInput();
        assertEquals(AndroidControllerInput.INTERACT | AndroidControllerInput.CONFIRM,
                input.key(key(2, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_BUTTON_A, 0, 100), 100).actions);
        input.suspend();
        assertEquals(0, input.key(key(2, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_BUTTON_A, 0, 90), 200).actions);
        assertEquals(0, input.key(key(2, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_BUTTON_A, 0, 150), 200).actions);
        input.key(key(2, KeyEvent.ACTION_UP, KeyEvent.KEYCODE_BUTTON_A, 0, 201), 201);
        assertEquals(AndroidControllerInput.INTERACT | AndroidControllerInput.CONFIRM,
                input.key(key(2, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_BUTTON_A, 0, 202), 202).actions);
    }

    @Test public void aMeaningfulSecondDeviceTakesOwnershipWithoutReplayingItsOldAxes() {
        AndroidControllerInput input = new AndroidControllerInput();
        input.motion(motion(1, 1, 0, 0, 0, 0, 0, 0), 1);
        input.motion(motion(1, 2, 0.5f, 0, 0, 0, 0, 0), 2);
        input.motion(motion(2, 3, 0.8f, 0, 0, 0, 0, 0), 3);
        assertEquals(1, input.activeDeviceId());
        input.motion(motion(2, 4, 0, 0, 0, 0, 0, 0), 4);
        assertEquals(1, input.activeDeviceId());
        input.motion(motion(2, 4, 0.65f, 0, 0, 0, 0, 0), 4);
        assertEquals(2, input.activeDeviceId());
        assertTrue(input.moveStrafe() > 0.0f);
        input.removeDevice(2);
        assertEquals(-1, input.activeDeviceId());
        assertEquals(0.0f, input.moveStrafe(), 0.0f);
    }

    @Test public void inactiveDeviceMustBeNeutralThenMoveMeaningfullyBeforeTakingOwnership() {
        AndroidControllerInput input = new AndroidControllerInput();
        input.motion(motion(1, 1, 0, 0, 0, 0, 0, 0), 1);
        input.motion(motion(1, 2, 0.5f, 0, 0, 0, 0, 0), 2);
        float activeStrafe = input.moveStrafe();

        input.motion(motion(2, 3, 0, 0, 0, 0, 0, 0), 3); // candidate is neutral-ready
        input.motion(motion(2, 4, 0.06f, 0.04f, 0, 0, 0, 0), 4); // drift cannot steal focus
        assertEquals(1, input.activeDeviceId());
        assertEquals(activeStrafe, input.moveStrafe(), 0.0f);

        AndroidControllerInput.Result intentional = input.motion(motion(2, 5, 0.7f, 0, 0, 0, 0, 0), 5);
        assertEquals(2, input.activeDeviceId());
        assertTrue(intentional.meaningful);
        assertTrue(intentional.moveStrafe > 0.0f);
    }

    @Test public void dpadAndHatAreOneNavigationHoldAndPureDpadSourceRequiresGamepadDevice() {
        android.view.InputDevice controller = InputDeviceBuilder.newBuilder().setId(21).setName("Controller")
                .setSources(InputDevice.SOURCE_DPAD | InputDevice.SOURCE_GAMEPAD).build();
        android.view.InputDevice keyboard = InputDeviceBuilder.newBuilder().setId(22).setName("Keyboard")
                .setSources(InputDevice.SOURCE_DPAD | InputDevice.SOURCE_KEYBOARD).build();

        KeyEvent controllerDpad = key(21, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DPAD_UP, 0, 1,
                InputDevice.SOURCE_DPAD);
        assertTrue(AndroidControllerInput.isControllerKey(controllerDpad, controller));
        AndroidControllerInput input = new AndroidControllerInput();
        assertTrue(input.key(controllerDpad, 1, controller).handled);
        assertEquals(-1, input.navigation(1).vertical); // negative is up

        input.motion(motionAxes(21, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), 2); // neutral hat baseline
        input.motion(motionAxes(21, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1), 3);
        assertEquals(0, input.navigation(3).vertical); // hat+key remain one held direction
        input.key(key(21, KeyEvent.ACTION_UP, KeyEvent.KEYCODE_DPAD_UP, 0, 4, InputDevice.SOURCE_DPAD), 4, controller);
        assertEquals(0, input.navigation(4).vertical); // releasing one representation is not a new edge
        input.motion(motionAxes(21, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), 5);
        assertEquals(0, input.navigation(5).vertical);

        KeyEvent keyboardDpad = key(22, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DPAD_UP, 0, 5,
                InputDevice.SOURCE_DPAD);
        assertFalse(AndroidControllerInput.isControllerKey(keyboardDpad, keyboard));
        assertFalse(input.key(keyboardDpad, 5, keyboard).handled);
    }

    @Test public void digitalTriggerHeldAcrossSuspendCannotReplayThroughAnalogAxis() {
        AndroidControllerInput input = new AndroidControllerInput();
        assertEquals(AndroidControllerInput.SWING,
                input.key(key(31, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_BUTTON_R2, 0, 10), 10).actions);
        input.suspend(20);
        assertEquals(0, input.motion(motion(31, 21, 0, 0, 0, 0, 0, 0.8f), 21).actions);
        input.key(key(31, KeyEvent.ACTION_UP, KeyEvent.KEYCODE_BUTTON_R2, 0, 22), 22);
        assertEquals(0, input.motion(motion(31, 23, 0, 0, 0, 0, 0, 0.8f), 23).actions);
        input.motion(motion(31, 24, 0, 0, 0, 0, 0, 0), 24);
        assertEquals(AndroidControllerInput.SWING,
                input.motion(motion(31, 25, 0, 0, 0, 0, 0, 0.8f), 25).actions);
    }

    @Test public void motionRangesNormalizeAxesAndRxRyBacksUpMissingZRzRanges() {
        android.view.InputDevice device = InputDeviceBuilder.newBuilder().setId(41).setName("RX fallback controller")
                .setSources(InputDevice.SOURCE_JOYSTICK | InputDevice.SOURCE_GAMEPAD)
                .addMotionRange(MotionEvent.AXIS_X, MOTION_SOURCE, -10, 10, 2, 0, 0)
                .addMotionRange(MotionEvent.AXIS_Y, MOTION_SOURCE, -10, 10, 2, 0, 0)
                .addMotionRange(MotionEvent.AXIS_RX, MOTION_SOURCE, -2, 2, 0.1f, 0, 0)
                .addMotionRange(MotionEvent.AXIS_RY, MOTION_SOURCE, -2, 2, 0.1f, 0, 0)
                .build();
        AndroidControllerInput input = new AndroidControllerInput();
        input.motion(motionAxes(41, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), 1, device);
        AndroidControllerInput.Result ranged = input.motion(
                motionAxes(41, 2, 4, 4, 0, 0, 0, 0, 1, 1, 0, 0), 2, device);
        assertEquals(0.279f, ranged.moveStrafe, 0.02f); // raw 4/10 with radial flat compensation
        assertEquals(-0.279f, ranged.moveForward, 0.02f);
        assertTrue(ranged.lookX > 0.40f);
        assertTrue(ranged.lookY < -0.40f);
    }

    private static KeyEvent key(int deviceId, int action, int keyCode, int repeat, long time) {
        return key(deviceId, action, keyCode, repeat, time, KEY_SOURCE);
    }

    private static KeyEvent key(int deviceId, int action, int keyCode, int repeat, long time, int source) {
        return new KeyEvent(time - 1, time, action, keyCode, repeat, 0, deviceId, 0, 0, source);
    }

    private static MotionEvent motion(int deviceId, long time, float x, float y,
            float z, float rz, float leftTrigger, float rightTrigger) {
        return motionAxes(deviceId, time, x, y, z, rz, leftTrigger, rightTrigger, 0, 0, 0, 0);
    }

    private static MotionEvent motionAxes(int deviceId, long time, float x, float y,
            float z, float rz, float leftTrigger, float rightTrigger,
            float rx, float ry, float hatX, float hatY) {
        MotionEvent.PointerProperties properties = new MotionEvent.PointerProperties();
        properties.id = 0;
        properties.toolType = MotionEvent.TOOL_TYPE_UNKNOWN;
        MotionEvent.PointerCoords coords = new MotionEvent.PointerCoords();
        coords.setAxisValue(MotionEvent.AXIS_X, x);
        coords.setAxisValue(MotionEvent.AXIS_Y, y);
        coords.setAxisValue(MotionEvent.AXIS_Z, z);
        coords.setAxisValue(MotionEvent.AXIS_RZ, rz);
        coords.setAxisValue(MotionEvent.AXIS_LTRIGGER, leftTrigger);
        coords.setAxisValue(MotionEvent.AXIS_RTRIGGER, rightTrigger);
        coords.setAxisValue(MotionEvent.AXIS_RX, rx);
        coords.setAxisValue(MotionEvent.AXIS_RY, ry);
        coords.setAxisValue(MotionEvent.AXIS_HAT_X, hatX);
        coords.setAxisValue(MotionEvent.AXIS_HAT_Y, hatY);
        return MotionEvent.obtain(time - 1, time, MotionEvent.ACTION_MOVE, 1,
                new MotionEvent.PointerProperties[] {properties}, new MotionEvent.PointerCoords[] {coords},
                0, 0, 1.0f, 1.0f, deviceId, 0, MOTION_SOURCE, 0);
    }
}
