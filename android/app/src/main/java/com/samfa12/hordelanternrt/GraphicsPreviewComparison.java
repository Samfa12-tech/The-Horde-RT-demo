package com.samfa12.hordelanternrt;

/** One-field, real-renderer comparisons. This policy has no persistence or JNI side effects. */
final class GraphicsPreviewComparison {
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
    static GraphicsPreferences.Values selection(GraphicsPreferences.Values confirmed,
            GraphicsPreferences.Values draft, int choice, boolean after) {
        if (!after) return confirmed;
        switch (choice) {
            case RESOLUTION: return new GraphicsPreferences.Values(draft.scale, confirmed.water, confirmed.fire, confirmed.cap);
            case WATER: return new GraphicsPreferences.Values(confirmed.scale, draft.water, confirmed.fire, confirmed.cap);
            case FIRE: return new GraphicsPreferences.Values(confirmed.scale, confirmed.water, draft.fire, confirmed.cap);
            case CAP: return new GraphicsPreferences.Values(confirmed.scale, confirmed.water, confirmed.fire, draft.cap);
            default: throw new IllegalArgumentException("Unknown graphics comparison choice");
        }
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
    private GraphicsPreviewComparison() {}
}
