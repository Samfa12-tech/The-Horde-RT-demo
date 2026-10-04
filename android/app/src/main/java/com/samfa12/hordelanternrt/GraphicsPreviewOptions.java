package com.samfa12.hordelanternrt;

import java.util.Arrays;

/** Live preview choice/acknowledgement policy. No persistence or JNI side effects. */
final class GraphicsPreviewOptions {
    static final int RESOLUTION = 0, WATER = 1, FIRE = 2, CAP = 3, GLASS = 4, SHADOW = 5;
    static String name(int choice) {
        switch (choice) {
            case WATER: return "Water";
            case FIRE: return "Fire";
            case CAP: return "Frame cap";
            case GLASS: return "Glass";
            case SHADOW: return "Shadows";
            default: return "Resolution";
        }
    }
    static int camera(int choice) {
        switch (choice) {
            case WATER: return 3;
            case CAP: return 4;
            case RESOLUTION: return 1;
            case GLASS: return 2;
            default: return 0;
        }
    }
    static int value(GraphicsPreferences.Values current, int choice) {
        switch (choice) {
            case RESOLUTION: return current.scale;
            case WATER: return current.water;
            case FIRE: return current.fire;
            case CAP: return current.cap;
            case GLASS: return current.glassEnabled ? 1 : 0;
            case SHADOW: return current.shadow;
            default: throw new IllegalArgumentException("Unknown live graphics choice");
        }
    }
    static GraphicsPreferences.Values withChoice(GraphicsPreferences.Values current, int choice, int value) {
        final GraphicsPreferences.Values result;
        switch (choice) {
            case RESOLUTION: result = new GraphicsPreferences.Values(value, current.water, current.fire, current.cap, current.glassEnabled, current.shadow); break;
            case WATER: result = new GraphicsPreferences.Values(current.scale, value, current.fire, current.cap, current.glassEnabled, current.shadow); break;
            case FIRE: result = new GraphicsPreferences.Values(current.scale, current.water, value, current.cap, current.glassEnabled, current.shadow); break;
            case CAP: result = new GraphicsPreferences.Values(current.scale, current.water, current.fire, value, current.glassEnabled, current.shadow); break;
            case GLASS:
                if (value != 0 && value != 1) throw new IllegalArgumentException("Invalid glass choice");
                result = new GraphicsPreferences.Values(current.scale, current.water, current.fire, current.cap, value == 1, current.shadow); break;
            case SHADOW: result = new GraphicsPreferences.Values(current.scale, current.water, current.fire, current.cap, current.glassEnabled, value); break;
            default: throw new IllegalArgumentException("Unknown live graphics choice");
        }
        if (!result.valid()) throw new IllegalArgumentException("Invalid live graphics value");
        return result;
    }
    static int[] choices(GraphicsPreferences.Values current, int choice) {
        return choices(current, choice, GraphicsPreferences.MIN_RENDER_SCALE_PERCENT);
    }
    static int[] choices(GraphicsPreferences.Values current, int choice, int minimum) {
        int[] presets;
        switch (choice) {
            case RESOLUTION: presets = minimum == 33 ? new int[]{33,40,50,63,75,100} : new int[]{50,63,75,100}; break;
            case WATER: return new int[]{0,1,2};
            case FIRE: return new int[]{GraphicsPreferences.FIRE_LOW,GraphicsPreferences.FIRE_MOBILE,GraphicsPreferences.FIRE_HIGH};
            case SHADOW: return new int[]{GraphicsPreferences.SHADOW_LOWER,GraphicsPreferences.SHADOW_CURRENT,GraphicsPreferences.SHADOW_HIGHER};
            case GLASS: return new int[]{0,1};
            case CAP: presets = new int[]{15,30,60}; break;
            default: throw new IllegalArgumentException("Unknown live graphics choice");
        }
        int selected = value(current,choice);
        if (choice == RESOLUTION && !GraphicsPreferences.validScale(selected, minimum)) return presets;
        for (int preset : presets) if (preset == selected) return presets;
        int[] values = Arrays.copyOf(presets,presets.length+1); values[presets.length]=selected;
        Arrays.sort(values); return values;
    }
    static String resolutionLabel(int percent) {
        return resolutionLabel(percent, GraphicsPreferences.MIN_RENDER_SCALE_PERCENT);
    }
    static String resolutionLabel(int percent, int minimum) {
        return percent + "%" + (minimum == 33 && (percent == 33 || percent == 40) ? " (Experimental)" : "");
    }
    static String fireLabel(int value) {
        switch (value) {
            case GraphicsPreferences.FIRE_LOW: return "Low";
            case GraphicsPreferences.FIRE_MOBILE: return "Mobile";
            case GraphicsPreferences.FIRE_HIGH: return "High";
            default: throw new IllegalArgumentException("Unknown fire detail");
        }
    }
    static String shadowLabel(int value) {
        switch (value) {
            case GraphicsPreferences.SHADOW_LOWER: return "Lower";
            case GraphicsPreferences.SHADOW_CURRENT: return "Current";
            case GraphicsPreferences.SHADOW_HIGHER: return "Higher";
            default: throw new IllegalArgumentException("Unknown shadow quality");
        }
    }
    static boolean presented(long[] snapshot, long generation, long serial, GraphicsPreferences.Values selected) {
        return selected != null && GraphicsPreferences.presented(snapshot, generation) && snapshot[19] == 1 &&
                (serial == 0 || (snapshot[0] == serial && GraphicsPreferences.matchesRequested(snapshot,selected))) &&
                GraphicsPreferences.matchesEffective(snapshot,selected);
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
