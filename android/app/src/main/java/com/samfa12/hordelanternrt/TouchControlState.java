package com.samfa12.hordelanternrt;

/** Small, platform-independent cleanup for the two gameplay touch roles. */
final class TouchControlState {
    static void finishPointer(int[] activePointers, float[] controls, int pointerId) {
        if (pointerId == activePointers[0]) {
            activePointers[0] = -1;
            controls[7] = 0.0f;
            controls[8] = 0.0f;
        }
        if (pointerId == activePointers[1]) activePointers[1] = -1;
    }

    static void cancelGesture(int[] activePointers, float[] controls) {
        clear(activePointers, controls);
    }

    static void clear(int[] activePointers, float[] controls) {
        activePointers[0] = -1;
        activePointers[1] = -1;
        controls[7] = 0.0f;
        controls[8] = 0.0f;
    }

    private TouchControlState() {}
}
