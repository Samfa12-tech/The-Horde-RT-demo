package com.samfa12.hordelanternrt;

final class KitAnkleFeedback {
    static final int EVENT_TYPE = 38;

    private KitAnkleFeedback() {}

    static String soundKey(int cue) {
        switch (cue) {
            case 1: return "kit_boot_and_gravel";
            case 2: return "kit_steady_breath";
            default: return null;
        }
    }
}
