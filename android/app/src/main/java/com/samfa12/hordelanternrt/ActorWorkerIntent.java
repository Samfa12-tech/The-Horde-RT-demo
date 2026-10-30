package com.samfa12.hordelanternrt;

import android.content.Intent;

// Test-only renderer configuration, never gameplay input or saved preferences.
final class ActorWorkerIntent {
    static final String EXTRA = "horde.debug.actorWorker";
    static int mode(Intent intent, boolean debuggable, boolean experimentBuild) {
        if (!debuggable || !experimentBuild || intent == null || !intent.hasExtra(EXTRA)) return -1;
        Object value = intent.getExtras().get(EXTRA);
        return "serial".equals(value) ? 0 : "worker".equals(value) ? 1 : -1;
    }
    private ActorWorkerIntent() {}
}
