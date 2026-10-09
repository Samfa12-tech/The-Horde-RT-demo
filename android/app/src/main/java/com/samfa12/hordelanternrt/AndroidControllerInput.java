package com.samfa12.hordelanternrt;

import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

/**
 * Bounded Android controller input policy. This class translates controller events into
 * semantic edges and normalized axes; callers retain all menu and gameplay authority.
 */
final class AndroidControllerInput {
    static final int SWING = 1 << 0;
    static final int PARRY = 1 << 1;
    static final int DODGE = 1 << 2;
    static final int INTERACT = 1 << 3;
    static final int LANTERN = 1 << 4;
    static final int PAUSE = 1 << 5;
    static final int CONFIRM = 1 << 6;
    static final int CANCEL = 1 << 7;

    private static final float TRIGGER_PRESS = 0.50f;
    private static final float TRIGGER_RELEASE = 0.35f;
    private static final float STICK_NAV_PRESS = 0.65f;
    private static final float STICK_NAV_RELEASE = 0.45f;
    private static final long NAV_INITIAL_REPEAT_MS = 350L;
    private static final long NAV_REPEAT_MS = 120L;
    private static final int MAX_DEVICE_CANDIDATES = 4;

    private static final class DeviceCandidate {
        int deviceId;
        long sequence;
        boolean neutralReady;
    }

    static final class Result {
        final boolean handled;
        final boolean meaningful;
        final int actions;
        final int navigationHorizontal;
        final int navigationVertical;
        final int horizontal;
        final int vertical;
        final float moveStrafe;
        final float moveForward;
        final float lookX;
        final float lookY;

        Result(boolean handled, boolean meaningful, int actions, int navigationHorizontal,
                int navigationVertical, float moveStrafe, float moveForward, float lookX,
                float lookY) {
            this.handled = handled;
            this.meaningful = meaningful;
            this.actions = actions;
            this.navigationHorizontal = navigationHorizontal;
            this.navigationVertical = navigationVertical;
            this.horizontal = navigationHorizontal;
            this.vertical = navigationVertical;
            this.moveStrafe = moveStrafe;
            this.moveForward = moveForward;
            this.lookX = lookX;
            this.lookY = lookY;
        }
    }

    private int activeDeviceId = -1;
    private long lastObservedTimeMs = Long.MIN_VALUE;
    private long suspendBoundaryMs = Long.MIN_VALUE;
    private boolean suspended;
    private final boolean[] keyDown = new boolean[13];
    private final boolean[] blockedUntilUp = new boolean[13];
    private final DeviceCandidate[] candidates = new DeviceCandidate[MAX_DEVICE_CANDIDATES];
    private long candidateSequence;
    private boolean leftStickSeeded;
    private boolean leftStickArmed;
    private boolean rightStickSeeded;
    private boolean rightStickArmed;
    private boolean leftTriggerSeeded;
    private boolean leftTriggerArmed;
    private boolean leftTriggerAnalogDown;
    private boolean leftTriggerKeyDown;
    private boolean rightTriggerSeeded;
    private boolean rightTriggerArmed;
    private boolean rightTriggerAnalogDown;
    private boolean rightTriggerKeyDown;
    private float moveStrafe;
    private float moveForward;
    private float lookX;
    private float lookY;
    private boolean runHeld;
    private float leftFlatX = 0.12f;
    private float leftFlatY = 0.12f;
    private float rightFlatX = 0.12f;
    private float rightFlatY = 0.12f;
    private boolean dpadLeft;
    private boolean dpadRight;
    private boolean dpadUp;
    private boolean dpadDown;
    private int hatNavX;
    private int hatNavY;
    private boolean hatSeeded;
    private boolean hatArmed;
    private int stickNavX;
    private int stickNavY;
    private int lastNavX;
    private int lastNavY;
    private long nextNavRepeatMs;

    int activeDeviceId() { return activeDeviceId; }
    float moveStrafe() { return moveStrafe; }
    float moveForward() { return moveForward; }
    float lookX() { return lookX; }
    float lookY() { return lookY; }
    boolean runHeld() { return runHeld; }

    static boolean isControllerKey(KeyEvent event) {
        return isControllerKey(event, event == null ? null : event.getDevice());
    }

    static boolean isControllerKey(KeyEvent event, InputDevice device) {
        if (event == null) return false;
        final int source = event.getSource();
        if ((source & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) return true;
        if ((source & InputDevice.SOURCE_DPAD) != InputDevice.SOURCE_DPAD) return false;
        if (device == null) return false;
        final int sources = device.getSources();
        return (sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
                || (sources & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK;
    }

    static boolean isControllerMotion(MotionEvent event) {
        return event != null && (event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK;
    }

    Result key(KeyEvent event, long nowMs) {
        return key(event, nowMs, event == null ? null : event.getDevice());
    }

    Result key(KeyEvent event, long nowMs, InputDevice device) {
        if (!isControllerKey(event, device)) return result(false, false, 0, 0, 0);
        final int slot = keySlot(event.getKeyCode());
        if (slot < 0) return result(false, false, 0, 0, 0);
        observe(nowMs);
        final int deviceId = event.getDeviceId();
        final int action = event.getAction();
        if (action == KeyEvent.ACTION_UP) {
            if (activeDeviceId == deviceId) {
                keyDown[slot] = false;
                blockedUntilUp[slot] = false;
                if (event.getKeyCode() == KeyEvent.KEYCODE_BUTTON_R1) runHeld = false;
                updateDpad(event.getKeyCode(), false);
                updateDigitalTrigger(event.getKeyCode(), false);
            }
            return result(true, false, 0, 0, 0);
        }
        if (action != KeyEvent.ACTION_DOWN) return result(true, false, 0, 0, 0);
        if (event.getRepeatCount() > 0) return result(true, false, 0, 0, 0);
        if (suspendBoundaryMs != Long.MIN_VALUE && event.getDownTime() <= suspendBoundaryMs) {
            if (activeDeviceId == deviceId) blockedUntilUp[slot] = true;
            return result(true, false, 0, 0, 0);
        }
        suspended = false;
        if (deviceId != activeDeviceId) {
            resetForDevice(deviceId);
        }
        if (blockedUntilUp[slot] || keyDown[slot]) return result(true, false, 0, 0, 0);
        keyDown[slot] = true;
        final int keyCode = event.getKeyCode();
        if (keyCode == KeyEvent.KEYCODE_BUTTON_R1) runHeld = true;
        updateDpad(keyCode, true);
        final int actions = updateDigitalTrigger(keyCode, true) | actionForKey(keyCode);
        return result(true, true, actions, 0, 0);
    }

    Result motion(MotionEvent event, long nowMs) {
        return motion(event, nowMs, event == null ? null : event.getDevice());
    }

    Result motion(MotionEvent event, long nowMs, InputDevice device) {
        if (!isControllerMotion(event)) return result(false, false, 0, 0, 0);
        observe(nowMs);
        final int deviceId = event.getDeviceId();
        final int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_CANCEL || action == MotionEvent.ACTION_UP) {
            if (deviceId == activeDeviceId) {
                neutralizeAxes(true);
                hatNavX = hatNavY = 0;
                hatSeeded = hatArmed = false;
            }
            return result(true, false, 0, 0, 0);
        }
        if (action != MotionEvent.ACTION_MOVE && action != MotionEvent.ACTION_HOVER_MOVE) {
            return result(true, false, 0, 0, 0);
        }
        if (deviceId != activeDeviceId) {
            if (activeDeviceId < 0) {
                resetForDevice(deviceId);
                if (hasNeutralBaseline(event, device)) armAfterNeutralBaseline();
            } else {
                DeviceCandidate candidate = candidateFor(deviceId);
                if (candidate == null) candidate = rememberCandidate(deviceId);
                final boolean neutral = hasNeutralBaseline(event, device);
                if (!candidate.neutralReady) {
                    if (neutral) candidate.neutralReady = true;
                    candidate.sequence = ++candidateSequence;
                    return result(true, false, 0, 0, 0);
                }
                if (!hasMeaningfulMotion(event, device)) {
                    candidate.sequence = ++candidateSequence;
                    return result(true, false, 0, 0, 0);
                }
                resetForDevice(deviceId);
                forgetCandidate(deviceId);
                armAfterNeutralBaseline();
            }
        }
        if (suspended) suspended = false;

        final float rawLX = axis(event, device, MotionEvent.AXIS_X);
        final float rawLY = axis(event, device, MotionEvent.AXIS_Y);
        final int[] rightAxes = rightStickAxes(event, device);
        final float rawRX = axis(event, device, rightAxes[0]);
        final float rawRY = axis(event, device, rightAxes[1]);
        final float flatLX = flat(device, MotionEvent.AXIS_X, leftFlatX);
        final float flatLY = flat(device, MotionEvent.AXIS_Y, leftFlatY);
        final float flatRX = flat(device, rightAxes[0], rightFlatX);
        final float flatRY = flat(device, rightAxes[1], rightFlatY);
        leftFlatX = flatLX;
        leftFlatY = flatLY;
        rightFlatX = flatRX;
        rightFlatY = flatRY;

        final float[] left = radial(rawLX, rawLY, flatLX, flatLY);
        final float[] right = radial(rawRX, rawRY, flatRX, flatRY);
        boolean meaningful = false;
        if (!leftStickSeeded) {
            leftStickSeeded = true;
            leftStickArmed = magnitude(rawLX, rawLY) <= deadzone(flatLX, flatLY);
        } else if (!leftStickArmed) {
            if (magnitude(rawLX, rawLY) <= deadzone(flatLX, flatLY)) leftStickArmed = true;
        } else {
            moveStrafe = left[0];
            moveForward = -left[1];
            meaningful |= Math.abs(moveStrafe) > 0.001f || Math.abs(moveForward) > 0.001f;
        }
        if (!rightStickSeeded) {
            rightStickSeeded = true;
            rightStickArmed = magnitude(rawRX, rawRY) <= deadzone(flatRX, flatRY);
        } else if (!rightStickArmed) {
            if (magnitude(rawRX, rawRY) <= deadzone(flatRX, flatRY)) rightStickArmed = true;
        } else {
            lookX = right[0];
            lookY = -right[1];
            meaningful |= Math.abs(lookX) > 0.001f || Math.abs(lookY) > 0.001f;
        }
        stickNavX = hysteresisDirection(moveStrafe, stickNavX);
        // MotionEvent Y grows downward; keep navigation's negative=up convention.
        stickNavY = hysteresisDirection(left[1], stickNavY);
        final float rawHatX = event.getAxisValue(MotionEvent.AXIS_HAT_X);
        final float rawHatY = event.getAxisValue(MotionEvent.AXIS_HAT_Y);
        if (!hatSeeded) {
            hatSeeded = true;
            hatArmed = Math.abs(rawHatX) < 0.5f && Math.abs(rawHatY) < 0.5f;
            hatNavX = hatNavY = 0;
        } else if (!hatArmed) {
            if (Math.abs(rawHatX) < 0.5f && Math.abs(rawHatY) < 0.5f) hatArmed = true;
            hatNavX = hatNavY = 0;
        } else {
            hatNavX = hatDirection(rawHatX, hatNavX);
            hatNavY = hatDirection(rawHatY, hatNavY);
        }

        int actions = 0;
        final float leftTrigger = trigger(event, device, true);
        final float rightTrigger = trigger(event, device, false);
        if (!leftTriggerSeeded) {
            leftTriggerSeeded = true;
            leftTriggerArmed = leftTrigger <= TRIGGER_RELEASE;
            leftTriggerAnalogDown = leftTrigger > TRIGGER_RELEASE;
        } else if (!leftTriggerArmed) {
            if (leftTrigger <= TRIGGER_RELEASE) {
                leftTriggerArmed = true;
                leftTriggerAnalogDown = false;
            }
        } else {
            final boolean down = leftTrigger > TRIGGER_PRESS;
            if (down && !leftTriggerAnalogDown && !leftTriggerKeyDown) actions |= PARRY;
            leftTriggerAnalogDown = down || (leftTriggerAnalogDown && leftTrigger > TRIGGER_RELEASE);
        }
        if (!rightTriggerSeeded) {
            rightTriggerSeeded = true;
            rightTriggerArmed = rightTrigger <= TRIGGER_RELEASE;
            rightTriggerAnalogDown = rightTrigger > TRIGGER_RELEASE;
        } else if (!rightTriggerArmed) {
            if (rightTrigger <= TRIGGER_RELEASE) {
                rightTriggerArmed = true;
                rightTriggerAnalogDown = false;
            }
        } else {
            final boolean down = rightTrigger > TRIGGER_PRESS;
            if (down && !rightTriggerAnalogDown && !rightTriggerKeyDown) actions |= SWING;
            rightTriggerAnalogDown = down || (rightTriggerAnalogDown && rightTrigger > TRIGGER_RELEASE);
        }
        meaningful |= actions != 0;
        meaningful |= hatNavX != 0 || hatNavY != 0;
        return result(true, meaningful, actions, 0, 0);
    }

    Result navigation(long nowMs) {
        observe(nowMs);
        final int horizontal = axisDirection(dpadLeft || hatNavX < 0,
                dpadRight || hatNavX > 0, stickNavX);
        final int vertical = axisDirection(dpadUp || hatNavY < 0,
                dpadDown || hatNavY > 0, stickNavY);
        if (horizontal != lastNavX || vertical != lastNavY) {
            lastNavX = horizontal;
            lastNavY = vertical;
            nextNavRepeatMs = nowMs + NAV_INITIAL_REPEAT_MS;
            return result(horizontal != 0 || vertical != 0, horizontal != 0 || vertical != 0,
                    0, horizontal, vertical);
        }
        if ((horizontal != 0 || vertical != 0) && nowMs >= nextNavRepeatMs) {
            nextNavRepeatMs = nowMs + NAV_REPEAT_MS;
            return result(true, true, 0, horizontal, vertical);
        }
        return result(false, false, 0, 0, 0);
    }

    void suspend() {
        suspend(lastObservedTimeMs);
    }

    void suspend(long nowMs) {
        suspendBoundaryMs = Math.max(nowMs, lastObservedTimeMs);
        suspended = true;
        runHeld = false;
        for (int i = 0; i < keyDown.length; ++i) {
            blockedUntilUp[i] |= keyDown[i];
            keyDown[i] = false;
        }
        dpadLeft = dpadRight = dpadUp = dpadDown = false;
        hatNavX = hatNavY = 0;
        hatSeeded = hatArmed = false;
        leftTriggerKeyDown = rightTriggerKeyDown = false;
        neutralizeAxes(true);
        leftStickSeeded = rightStickSeeded = false;
        leftTriggerSeeded = rightTriggerSeeded = false;
        leftStickArmed = rightStickArmed = false;
        leftTriggerArmed = rightTriggerArmed = false;
        stickNavX = stickNavY = lastNavX = lastNavY = 0;
    }

    void removeDevice(int deviceId) {
        if (deviceId == activeDeviceId) {
            resetForDevice(-1);
            suspended = false;
        }
        forgetCandidate(deviceId);
    }

    private void observe(long nowMs) {
        if (nowMs > lastObservedTimeMs) lastObservedTimeMs = nowMs;
    }

    private void resetForDevice(int deviceId) {
        activeDeviceId = deviceId;
        runHeld = false;
        for (int i = 0; i < keyDown.length; ++i) {
            keyDown[i] = false;
            blockedUntilUp[i] = false;
        }
        dpadLeft = dpadRight = dpadUp = dpadDown = false;
        hatNavX = hatNavY = 0;
        hatSeeded = hatArmed = false;
        leftStickSeeded = rightStickSeeded = false;
        leftStickArmed = rightStickArmed = false;
        leftTriggerSeeded = rightTriggerSeeded = false;
        leftTriggerArmed = rightTriggerArmed = false;
        leftTriggerAnalogDown = rightTriggerAnalogDown = false;
        leftTriggerKeyDown = rightTriggerKeyDown = false;
        stickNavX = stickNavY = lastNavX = lastNavY = 0;
        neutralizeAxes(false);
    }

    private boolean hasNeutralBaseline(MotionEvent event, InputDevice device) {
        final float lx = axis(event, device, MotionEvent.AXIS_X);
        final float ly = axis(event, device, MotionEvent.AXIS_Y);
        final int[] rightAxes = rightStickAxes(event, device);
        final float rx = axis(event, device, rightAxes[0]);
        final float ry = axis(event, device, rightAxes[1]);
        return magnitude(lx, ly) <= deadzone(flat(device, MotionEvent.AXIS_X, 0.12f), flat(device, MotionEvent.AXIS_Y, 0.12f))
                && magnitude(rx, ry) <= deadzone(flat(device, rightAxes[0], 0.12f), flat(device, rightAxes[1], 0.12f))
                && trigger(event, device, true) <= TRIGGER_RELEASE
                && trigger(event, device, false) <= TRIGGER_RELEASE
                && Math.abs(event.getAxisValue(MotionEvent.AXIS_HAT_X)) < 0.5f
                && Math.abs(event.getAxisValue(MotionEvent.AXIS_HAT_Y)) < 0.5f;
    }

    private boolean hasMeaningfulMotion(MotionEvent event, InputDevice device) {
        final float lx = axis(event, device, MotionEvent.AXIS_X);
        final float ly = axis(event, device, MotionEvent.AXIS_Y);
        final int[] rightAxes = rightStickAxes(event, device);
        final float rx = axis(event, device, rightAxes[0]);
        final float ry = axis(event, device, rightAxes[1]);
        return magnitude(lx, ly) > deadzone(flat(device, MotionEvent.AXIS_X, 0.12f), flat(device, MotionEvent.AXIS_Y, 0.12f))
                || magnitude(rx, ry) > deadzone(flat(device, rightAxes[0], 0.12f), flat(device, rightAxes[1], 0.12f))
                || trigger(event, device, true) > TRIGGER_PRESS
                || trigger(event, device, false) > TRIGGER_PRESS
                || Math.abs(event.getAxisValue(MotionEvent.AXIS_HAT_X)) >= 0.5f
                || Math.abs(event.getAxisValue(MotionEvent.AXIS_HAT_Y)) >= 0.5f;
    }

    private void armAfterNeutralBaseline() {
        leftStickSeeded = rightStickSeeded = true;
        leftStickArmed = rightStickArmed = true;
        leftTriggerSeeded = rightTriggerSeeded = true;
        leftTriggerArmed = rightTriggerArmed = true;
        hatSeeded = hatArmed = true;
    }

    private DeviceCandidate candidateFor(int deviceId) {
        for (DeviceCandidate candidate : candidates) {
            if (candidate != null && candidate.deviceId == deviceId) return candidate;
        }
        return null;
    }

    private DeviceCandidate rememberCandidate(int deviceId) {
        DeviceCandidate selected = null;
        for (DeviceCandidate candidate : candidates) {
            if (candidate == null) {
                selected = new DeviceCandidate();
                break;
            }
            if (selected == null || candidate.sequence < selected.sequence) selected = candidate;
        }
        if (selected == null) selected = new DeviceCandidate();
        selected.deviceId = deviceId;
        selected.sequence = ++candidateSequence;
        selected.neutralReady = false;
        for (int i = 0; i < candidates.length; ++i) {
            if (candidates[i] == null || candidates[i] == selected) {
                candidates[i] = selected;
                break;
            }
        }
        return selected;
    }

    private void forgetCandidate(int deviceId) {
        for (int i = 0; i < candidates.length; ++i) {
            if (candidates[i] != null && candidates[i].deviceId == deviceId) candidates[i] = null;
        }
    }

    private int updateDigitalTrigger(int keyCode, boolean down) {
        if (keyCode == KeyEvent.KEYCODE_BUTTON_L2) {
            final boolean wasDown = leftTriggerKeyDown || leftTriggerAnalogDown;
            leftTriggerKeyDown = down;
            return down && !wasDown ? PARRY : 0;
        }
        if (keyCode == KeyEvent.KEYCODE_BUTTON_R2) {
            final boolean wasDown = rightTriggerKeyDown || rightTriggerAnalogDown;
            rightTriggerKeyDown = down;
            return down && !wasDown ? SWING : 0;
        }
        return 0;
    }

    private void updateDpad(int keyCode, boolean down) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_DPAD_LEFT: dpadLeft = down; break;
            case KeyEvent.KEYCODE_DPAD_RIGHT: dpadRight = down; break;
            case KeyEvent.KEYCODE_DPAD_UP: dpadUp = down; break;
            case KeyEvent.KEYCODE_DPAD_DOWN: dpadDown = down; break;
            default: break;
        }
    }

    private void neutralizeAxes(boolean requireNeutral) {
        moveStrafe = moveForward = lookX = lookY = 0.0f;
        if (requireNeutral) {
            leftStickSeeded = rightStickSeeded = false;
            leftStickArmed = rightStickArmed = false;
            leftTriggerSeeded = rightTriggerSeeded = false;
            leftTriggerArmed = rightTriggerArmed = false;
            leftTriggerAnalogDown = rightTriggerAnalogDown = false;
        }
    }

    private Result result(boolean handled, boolean meaningful, int actions, int navX, int navY) {
        return new Result(handled, meaningful, actions, navX, navY,
                moveStrafe, moveForward, lookX, lookY);
    }

    private static int keySlot(int code) {
        switch (code) {
            case KeyEvent.KEYCODE_BUTTON_A: return 0;
            case KeyEvent.KEYCODE_BUTTON_B: return 1;
            case KeyEvent.KEYCODE_BUTTON_X: return 2;
            case KeyEvent.KEYCODE_BUTTON_Y: return 3;
            case KeyEvent.KEYCODE_BUTTON_L2: return 4;
            case KeyEvent.KEYCODE_BUTTON_R2: return 5;
            case KeyEvent.KEYCODE_BUTTON_START: return 6;
            case KeyEvent.KEYCODE_MENU: return 7;
            case KeyEvent.KEYCODE_DPAD_LEFT: return 8;
            case KeyEvent.KEYCODE_DPAD_RIGHT: return 9;
            case KeyEvent.KEYCODE_DPAD_UP: return 10;
            case KeyEvent.KEYCODE_DPAD_DOWN: return 11;
            case KeyEvent.KEYCODE_BUTTON_R1: return 12;
            default: return -1;
        }
    }

    private static int actionForKey(int code) {
        switch (code) {
            case KeyEvent.KEYCODE_BUTTON_A: return INTERACT | CONFIRM;
            case KeyEvent.KEYCODE_BUTTON_B: return DODGE | CANCEL;
            case KeyEvent.KEYCODE_BUTTON_X: return SWING;
            case KeyEvent.KEYCODE_BUTTON_Y: return LANTERN;
            case KeyEvent.KEYCODE_BUTTON_START:
            case KeyEvent.KEYCODE_MENU: return PAUSE;
            default: return 0;
        }
    }

    private static float axis(MotionEvent event, InputDevice device, int axis) {
        final float raw = event.getAxisValue(axis);
        if (device == null) return clamp(raw, -1.0f, 1.0f);
        final InputDevice.MotionRange range = device.getMotionRange(axis, event.getSource());
        if (range == null) return clamp(raw, -1.0f, 1.0f);
        final float center = (range.getMin() + range.getMax()) * 0.5f;
        final float span = raw >= center ? range.getMax() - center : center - range.getMin();
        return span <= 0.0001f ? 0.0f : clamp((raw - center) / span, -1.0f, 1.0f);
    }

    private static int[] rightStickAxes(MotionEvent event, InputDevice device) {
        final boolean primaryRanges = hasRange(device, MotionEvent.AXIS_Z, event.getSource())
                && hasRange(device, MotionEvent.AXIS_RZ, event.getSource());
        if (primaryRanges) return new int[] {MotionEvent.AXIS_Z, MotionEvent.AXIS_RZ};
        final boolean fallbackRanges = hasRange(device, MotionEvent.AXIS_RX, event.getSource())
                && hasRange(device, MotionEvent.AXIS_RY, event.getSource());
        if (fallbackRanges) return new int[] {MotionEvent.AXIS_RX, MotionEvent.AXIS_RY};
        if (Math.abs(event.getAxisValue(MotionEvent.AXIS_Z)) > 0.0001f
                || Math.abs(event.getAxisValue(MotionEvent.AXIS_RZ)) > 0.0001f
                || (Math.abs(event.getAxisValue(MotionEvent.AXIS_RX)) <= 0.0001f
                && Math.abs(event.getAxisValue(MotionEvent.AXIS_RY)) <= 0.0001f)) {
            return new int[] {MotionEvent.AXIS_Z, MotionEvent.AXIS_RZ};
        }
        return new int[] {MotionEvent.AXIS_RX, MotionEvent.AXIS_RY};
    }

    private static boolean hasRange(InputDevice device, int axis, int source) {
        return device != null && device.getMotionRange(axis, source) != null;
    }

    private static float flat(InputDevice device, int axis, float fallback) {
        if (device == null) return fallback;
        final InputDevice.MotionRange range = device.getMotionRange(axis, InputDevice.SOURCE_JOYSTICK);
        if (range == null) return fallback;
        final float center = (range.getMin() + range.getMax()) * 0.5f;
        final float span = Math.max(Math.abs(range.getMin() - center), Math.abs(range.getMax() - center));
        return span <= 0.0001f ? fallback : clamp(range.getFlat() / span, 0.0f, 0.95f);
    }

    private static float trigger(MotionEvent event, InputDevice device, boolean left) {
        int axis = left ? MotionEvent.AXIS_LTRIGGER : MotionEvent.AXIS_RTRIGGER;
        float value = triggerAxis(event, device, axis);
        if (value <= 0.0f) value = triggerAxis(event, device, left ? MotionEvent.AXIS_BRAKE : MotionEvent.AXIS_GAS);
        return clamp(value, 0.0f, 1.0f);
    }

    private static float triggerAxis(MotionEvent event, InputDevice device, int axis) {
        float raw = event.getAxisValue(axis);
        if (device == null) return clamp(raw, 0.0f, 1.0f);
        InputDevice.MotionRange range = device.getMotionRange(axis, event.getSource());
        if (range == null) return clamp(raw, 0.0f, 1.0f);
        float span = range.getMax() - range.getMin();
        return span <= 0.0001f ? clamp(raw, 0.0f, 1.0f)
                : clamp((raw - range.getMin()) / span, 0.0f, 1.0f);
    }

    private static float[] radial(float x, float y, float flatX, float flatY) {
        float length = magnitude(x, y);
        float dz = deadzone(flatX, flatY);
        if (length <= dz || length <= 0.0001f) return new float[] {0.0f, 0.0f};
        float scaled = clamp((length - dz) / (1.0f - dz), 0.0f, 1.0f) / length;
        return new float[] {x * scaled, y * scaled};
    }

    private static float deadzone(float x, float y) {
        return Math.max(0.12f, Math.min(0.45f, (float) Math.sqrt(x * x + y * y)));
    }

    private static float magnitude(float x, float y) { return (float) Math.sqrt(x * x + y * y); }

    private static int hysteresisDirection(float value, int prior) {
        if (prior == 0) return value >= STICK_NAV_PRESS ? 1 : value <= -STICK_NAV_PRESS ? -1 : 0;
        if (prior > 0) return value <= -STICK_NAV_PRESS ? -1 : value < STICK_NAV_RELEASE ? 0 : 1;
        return value >= STICK_NAV_PRESS ? 1 : value > -STICK_NAV_RELEASE ? 0 : -1;
    }

    private static int hatDirection(float value, int prior) {
        if (prior == 0) return value >= 0.5f ? 1 : value <= -0.5f ? -1 : 0;
        if (prior > 0) return value <= -0.5f ? -1 : value < 0.25f ? 0 : 1;
        return value >= 0.5f ? 1 : value > -0.25f ? 0 : -1;
    }

    private static int axisDirection(boolean negative, boolean positive, int stick) {
        if (negative == positive) return stick;
        return negative ? -1 : 1;
    }

    private static float clamp(float value, float min, float max) {
        return Math.max(min, Math.min(max, value));
    }
}
