package com.samfa12.hordelanternrt;

import java.util.Arrays;

/** Live preview choice/acknowledgement policy. No persistence or JNI side effects. */
final class GraphicsPreviewOptions {
    static final int RESOLUTION = 0, WATER = 1, FIRE = 2, CAP = 3;
    static String name(int choice) {
        switch (choice) {
            case WATER: return "Water";
            case FIRE: return "Fire";
            case CAP: return "Frame cap";
            default: return "Resolution";
        }
    }
    static int camera(int choice) {
        switch (choice) {
            case WATER: return 3;
            case CAP: return 4;
            case RESOLUTION: return 1;
            default: return 0;
        }
    }
    static int value(GraphicsPreferences.Values current, int choice) {
        switch (choice) {
            case RESOLUTION: return current.scale;
            case WATER: return current.water;
            case FIRE: return current.fire;
            case CAP: return current.cap;
            default: throw new IllegalArgumentException("Unknown live graphics choice");
        }
    }
    static GraphicsPreferences.Values withChoice(GraphicsPreferences.Values current, int choice, int value) {
        final GraphicsPreferences.Values result;
        switch (choice) {
            case RESOLUTION: result = new GraphicsPreferences.Values(value, current.water, current.fire, current.cap); break;
            case WATER: result = new GraphicsPreferences.Values(current.scale, value, current.fire, current.cap); break;
            case FIRE: result = new GraphicsPreferences.Values(current.scale, current.water, value, current.cap); break;
            case CAP: result = new GraphicsPreferences.Values(current.scale, current.water, current.fire, value); break;
            default: throw new IllegalArgumentException("Unknown live graphics choice");
        }
        if (!result.valid()) throw new IllegalArgumentException("Invalid live graphics value");
        return result;
    }
    static int[] choices(GraphicsPreferences.Values current, int choice) {
        int[] presets;
        switch (choice) {
            case RESOLUTION: presets = new int[]{50,63,75,100}; break;
            case WATER: return new int[]{0,1,2};
            case FIRE: return new int[]{0,1};
            case CAP: presets = new int[]{15,30,60}; break;
            default: throw new IllegalArgumentException("Unknown live graphics choice");
        }
        int selected = value(current,choice);
        for (int preset : presets) if (preset == selected) return presets;
        int[] values = Arrays.copyOf(presets,presets.length+1); values[presets.length]=selected;
        Arrays.sort(values); return values;
    }
    static boolean presented(long[] snapshot, long generation, long serial, GraphicsPreferences.Values selected) {
        return selected != null && GraphicsPreferences.presented(snapshot, generation) && snapshot[19] == 1 &&
                (serial == 0 || (snapshot[0] == serial && selected.same(new GraphicsPreferences.Values(
                    (int)snapshot[15], (int)snapshot[16], (int)snapshot[17], (int)snapshot[18])))) && selected.same(new GraphicsPreferences.Values(
                    (int)snapshot[3], (int)snapshot[4], (int)snapshot[5], (int)snapshot[6]));
    }
    static int maximumOverlayHeight(int viewportHeight, int bottomGap) {
        return Math.max(1, (int)((long)Math.max(0, viewportHeight) * 35 / 100) - Math.max(0, bottomGap));
    }
    static boolean currentPerformance(double[] snapshot, double epochFloor) {
        if (snapshot == null || snapshot.length < 9 || !Double.isFinite(snapshot[0]) || snapshot[0] <= epochFloor ||
                !Double.isFinite(snapshot[1]) || snapshot[1] < 0 || !Double.isFinite(snapshot[8]) ||
                snapshot[8] < 1 || snapshot[8] > 128 || snapshot[8] != (int)snapshot[8]) return false;
        return snapshot.length == 9 + (int)snapshot[8] * 2;
    }
    static double performanceEpochFloor(long previousGeneration, long currentGeneration, double[] snapshot) {
        // A new native surface context starts a new epoch sequence; the JNI getter filters its generation.
        return previousGeneration == currentGeneration && snapshot != null && snapshot.length >= 9 &&
                Double.isFinite(snapshot[0]) ? Math.max(0,snapshot[0]) : 0;
    }
    private GraphicsPreviewOptions() {}
}
