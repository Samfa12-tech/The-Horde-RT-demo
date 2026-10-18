package com.samfa12.hordelanternrt;

import android.content.Intent;
import android.os.Bundle;
import java.util.regex.Pattern;

/** Explicit, cold-start-only diagnostic request. Never a gameplay launch. */
final class EnvironmentSurveyRequest {
    static final String ACTION = "com.samfa12.hordelanternrt.ENVIRONMENT_SURVEY";
    static final String PROFILE = "river-bend";
    private static final Pattern ID = Pattern.compile("[A-Za-z0-9_-]{1,64}");
    final String requestId;
    final boolean waterDisabled;

    private EnvironmentSurveyRequest(String requestId, boolean waterDisabled) {
        this.requestId = requestId;
        this.waterDisabled = waterDisabled;
    }

    static boolean isSurvey(Intent intent) {
        return intent != null && ACTION.equals(intent.getAction());
    }

    static boolean validId(String id) {
        return id != null && ID.matcher(id).matches();
    }

    static EnvironmentSurveyRequest parse(Intent intent, boolean debug, boolean coldStart) {
        if (!debug || !coldStart || !isSurvey(intent)) return null;
        try {
            final Bundle extras = intent.getExtras();
            if (extras == null) return null;
            // A closed input roster prevents accidental reuse of checkpoint, motion,
            // benchmark, entry-play or campaign automation extras on this path.
            for (String key : extras.keySet()) {
                if (!"request_id".equals(key) && !"profile".equals(key) &&
                        !"water_disabled".equals(key)) return null;
            }
            final Object id = extras.get("request_id");
            final Object profile = extras.get("profile");
            final Object water = extras.get("water_disabled");
            if (!(id instanceof String) || !validId((String)id) ||
                    !PROFILE.equals(profile) || !(water instanceof Boolean)) return null;
            return new EnvironmentSurveyRequest((String)id, (Boolean)water);
        } catch (RuntimeException malformed) {
            return null;
        }
    }
}
