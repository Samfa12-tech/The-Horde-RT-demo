package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;

import android.content.Context;
import android.os.Looper;
import android.os.SystemClock;
import android.view.View;
import android.widget.TextView;
import java.lang.reflect.Field;
import java.time.Duration;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

/** Drives MainActivity's real recurring UI poll against a fake-clock native snapshot. */
@RunWith(RobolectricTestRunner.class)
@Config(sdk=34, shadows=CombatTeachingUiSchedulingTest.NativeTeaching.class)
public final class CombatTeachingUiSchedulingTest {
    @Implements(value=ProbeBridge.class, isInAndroidSdk=false)
    public static final class NativeTeaching {
        static long chargeStartedAtMs;
        static int reads;
        static int surfaceState = 1;

        @Implementation protected static void __staticInitializer__() {}
        @Implementation protected static int getSurfaceRuntimeState(long generation) { return surfaceState; }
        @Implementation protected static long getCombatTeachingState() {
            ++reads;
            final long elapsedMs = SystemClock.uptimeMillis() - chargeStartedAtMs;
            final double remainingMs = 1200.0 - elapsedMs;
            final int cue = elapsedMs >= 1320 ? 0 : elapsedMs >= 1200 ? 6 :
                    remainingMs > 10000.0 / 60.0 ? 4 :
                    remainingMs >= 2000.0 / 60.0 ? 5 : 6;
            final int opacity = elapsedMs >= 1320 ? 0 : 255;
            return 1L | ((long) cue << 3) | (1L << 10) | ((long) opacity << 40);
        }
        @Implementation protected static void setViewControls(float yaw, float pitch, float light, float x, float z) {}
        @Implementation protected static void setRunHeld(boolean held) {}
        @Implementation protected static void clearRunIntent() {}
        @Implementation protected static void setSimulationPaused(boolean paused) {}
        @Implementation protected static boolean setDiagnosticSurfaceSuspended(long generation, boolean suspended) { return true; }
        @Implementation protected static void stopDiagnosticSurface(long generation) {}
    }

    private static Field field(String name) throws Exception {
        final Field field = MainActivity.class.getDeclaredField(name);
        field.setAccessible(true);
        return field;
    }

    private static MainActivity activeActivity() throws Exception {
        final MainActivity activity = Robolectric.buildActivity(MainActivity.class).get();
        field("combatTeachingPrompt").set(activity, new TextView(activity));
        field("preferences").set(activity,
                activity.getSharedPreferences("combat-teaching-poll", Context.MODE_PRIVATE));
        field("resumed").setBoolean(activity, true);
        field("surfaceAvailable").setBoolean(activity, true);
        field("surfaceStarted").setBoolean(activity, true);
        field("surfaceRequestGeneration").setLong(activity, 1L);
        field("menuVisible").setBoolean(activity, false);
        field("interactButton").set(activity, new android.widget.Button(activity));
        field("toggleHeldLightPoseButton").set(activity, new android.widget.Button(activity));
        NativeTeaching.surfaceState = 1;
        return activity;
    }

    @Test public void defaultTeachingPollShowsActionableDodgeCueForEveryPollPhase() throws Exception {
        final StringBuilder missedOffsets = new StringBuilder();
        for (int offsetMs = 0; offsetMs < 180; offsetMs += 15) {
            final MainActivity activity = activeActivity();
            final TextView prompt = (TextView) field("combatTeachingPrompt").get(activity);

            final long pollStart = SystemClock.uptimeMillis();
            NativeTeaching.chargeStartedAtMs = pollStart - offsetMs;
            NativeTeaching.reads = 0;
            final Runnable poll = (Runnable) field("runtimePoll").get(activity);
            poll.run();
            final long cueStartMs = (long) Math.ceil(1200.0 - 10000.0 / 60.0) - offsetMs;
            final long cueEndMs = (long) Math.floor(1200.0 - 2000.0 / 60.0) - offsetMs;
            long firstDodgeShownAt = -1L;
            long lastDodgeShownAfterEndAt = -1L;
            boolean hiddenAfterRecoveryFade = false;
            for (int elapsed = 0; elapsed <= 1512; elapsed += 16) {
                Shadows.shadowOf(Looper.getMainLooper()).idleFor(Duration.ofMillis(16));
                final boolean dodgeShown = prompt.getVisibility() == View.VISIBLE &&
                        prompt.getText().toString().contains("DODGE NOW");
                if (dodgeShown && firstDodgeShownAt < 0L) firstDodgeShownAt = elapsed + 16L;
                if (dodgeShown && elapsed + 16L > cueEndMs)
                    lastDodgeShownAfterEndAt = elapsed + 16L;
                if (elapsed + 16L >= 1456 && prompt.getVisibility() == View.GONE)
                    hiddenAfterRecoveryFade = true;
            }
            final android.os.Handler handler = (android.os.Handler) field("handler").get(activity);
            handler.removeCallbacks(poll);
            handler.removeCallbacks((Runnable) field("combatTeachingPromptPoll").get(activity));
            handler.removeCallbacks((Runnable) field("runtimePoll").get(activity));
            if (firstDodgeShownAt < cueStartMs || firstDodgeShownAt - cueStartMs > 32L ||
                    lastDodgeShownAfterEndAt > cueEndMs + 32L ||
                    !hiddenAfterRecoveryFade) {
                if (missedOffsets.length() > 0) missedOffsets.append("; ");
                missedOffsets.append(offsetMs).append(" ms: shown=").append(firstDodgeShownAt)
                        .append("ms, cueStart=").append(cueStartMs)
                        .append("ms, lastDodgeAfterEnd=").append(lastDodgeShownAfterEndAt)
                        .append("ms, cueEnd=").append(cueEndMs)
                        .append("ms, hiddenByRecovery=").append(hiddenAfterRecoveryFade);
            }
        }
        assertTrue("32 ms prompt refresh failed cue timing across initial poll phases: " +
                missedOffsets, missedOffsets.length() == 0);
    }

    @Test public void pauseAndSurfaceDetachCancelTheTimelyPollAndClearTheCue() throws Exception {
        for (String boundary : new String[]{"onPause", "stopSurface"}) {
            final MainActivity activity = activeActivity();
            final TextView prompt = (TextView) field("combatTeachingPrompt").get(activity);
            NativeTeaching.chargeStartedAtMs = SystemClock.uptimeMillis() - 700L;
            ((Runnable) field("runtimePoll").get(activity)).run();
            final int readsBeforeBoundary = NativeTeaching.reads;
            assertTrue("fixture must schedule the cue poll", field("combatTeachingPromptPollScheduled").getBoolean(activity));

            final java.lang.reflect.Method lifecycle = MainActivity.class.getDeclaredMethod(boundary);
            lifecycle.setAccessible(true);
            try {
                lifecycle.invoke(activity);
            } catch (java.lang.reflect.InvocationTargetException failure) {
                throw new AssertionError("fixture could not execute " + boundary, failure.getCause());
            }
            assertFalse(boundary + " cancels the prompt timer",
                    field("combatTeachingPromptPollScheduled").getBoolean(activity));
            assertEquals(boundary + " clears the cue immediately", View.GONE, prompt.getVisibility());
            Shadows.shadowOf(Looper.getMainLooper()).idleFor(Duration.ofMillis(96));
            assertEquals(boundary + " prevents another native teaching read", readsBeforeBoundary,
                    NativeTeaching.reads);
            ((android.os.Handler) field("handler").get(activity)).removeCallbacks(
                    (Runnable) field("runtimePoll").get(activity));
        }
    }
}
