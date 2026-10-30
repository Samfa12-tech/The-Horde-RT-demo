package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;

import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.view.ViewGroup;
import java.io.File;
import java.io.FileOutputStream;
import java.nio.file.Files;
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
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.GraphicsMode;
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
        static long overrideState = Long.MIN_VALUE;

        @Implementation protected static void __staticInitializer__() {}
        @Implementation protected static int getSurfaceRuntimeState(long generation) { return surfaceState; }
        @Implementation protected static long getCombatTeachingState() {
            ++reads;
            if (overrideState != Long.MIN_VALUE) return overrideState;
            final long elapsedMs = SystemClock.uptimeMillis() - chargeStartedAtMs;
            final double remainingMs = 1200.0 - elapsedMs;
            final int cue = elapsedMs >= 1320 ? 0 : elapsedMs >= 1200 ? 6 :
                    remainingMs > 10000.0 / 60.0 ? 4 :
                    remainingMs >= 2000.0 / 60.0 ? 5 : 6;
            final int opacity = elapsedMs >= 1320 ? 0 : 255;
            return 1L | ((long) cue << 3) | (1L << 10) | ((long) opacity << 40);
        }
        @Implementation protected static void setViewControls(float yaw, float pitch, float light, float x, float z, boolean crouch,boolean analogueMovement) {}
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
        NativeTeaching.overrideState = Long.MIN_VALUE;
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

    @Test public void currentAttackPresentationSurvivesDisabledLessonAndClearsWithoutAStaleFade() throws Exception {
        final MainActivity activity=activeActivity();
        final TextView prompt=(TextView)field("combatTeachingPrompt").get(activity);
        final java.lang.reflect.Method update=MainActivity.class.getDeclaredMethod("updateCombatTeachingPrompt",int.class);
        update.setAccessible(true);
        NativeTeaching.overrideState=2L << 48; // Lesson disabled, no old opacity.
        assertTrue((Boolean)update.invoke(activity,1));
        assertEquals(View.VISIBLE,prompt.getVisibility());
        assertEquals("\u2715 RED: unblockable - DODGE",prompt.getText().toString());
        assertEquals(0xFFFF6B6B,prompt.getCurrentTextColor());
        assertEquals(1f,prompt.getAlpha(),0f);
        assertTrue(prompt.getContentDescription().toString().contains("unblockable"));
        NativeTeaching.overrideState=(1L << 48) | 2L | (1L << 10); // Lesson already complete.
        assertTrue((Boolean)update.invoke(activity,1));
        assertEquals("\u25c7 YELLOW guard-break: parry/dodge",prompt.getText().toString());
        assertEquals(0xFFFFD266,prompt.getCurrentTextColor());
        NativeTeaching.overrideState=2L | (1L << 10); // Current attack is neutral.
        assertFalse((Boolean)update.invoke(activity,1));
        assertEquals(View.GONE,prompt.getVisibility());
        assertEquals("",prompt.getText().toString());
        assertEquals("",prompt.getContentDescription().toString());
        assertEquals("",field("combatTeachingFadeText").get(activity));
        NativeTeaching.overrideState=2L << 48;
        field("menuVisible").setBoolean(activity,true);
        assertFalse((Boolean)update.invoke(activity,1));
        assertEquals(View.GONE,prompt.getVisibility());
    }

    @Test public void practiceProtectionSurvivesAttackEndAndClearsImmediatelyAtSafeExit()throws Exception {
        MainActivity activity=activeActivity();TextView prompt=(TextView)field("combatTeachingPrompt").get(activity);
        java.lang.reflect.Method update=MainActivity.class.getDeclaredMethod("updateCombatTeachingPrompt",int.class);update.setAccessible(true);
        NativeTeaching.overrideState=4L|(1L<<13)|(1L<<48);assertTrue((Boolean)update.invoke(activity,1));
        assertTrue(prompt.getText().toString().contains("Practice: no damage"));
        assertTrue(prompt.getText().toString().contains("block normal hits"));
        assertTrue(prompt.getText().toString().contains("YELLOW guard-break"));
        NativeTeaching.overrideState=4L|(1L<<13);assertTrue((Boolean)update.invoke(activity,1));
        assertTrue(prompt.getText().toString().startsWith("Practice: no damage"));
        NativeTeaching.overrideState=4L|(1L<<13)|(1L<<33);assertTrue((Boolean)update.invoke(activity,1));
        assertTrue(prompt.getText().toString().contains("Leaving practice safely"));
        NativeTeaching.overrideState=3L|(1L<<10);assertFalse((Boolean)update.invoke(activity,1));
        assertEquals("protection must never fade into live damage",View.GONE,prompt.getVisibility());
        assertEquals("",prompt.getText().toString());assertEquals("",field("combatTeachingFadeText").get(activity));
    }

    @Test public void actualExistingWidgetShowsAndClearsRevivalClearanceWithoutRestartingLesson()throws Exception {
        MainActivity activity=activeActivity();TextView prompt=(TextView)field("combatTeachingPrompt").get(activity);
        java.lang.reflect.Method update=MainActivity.class.getDeclaredMethod("updateCombatTeachingPrompt",int.class);
        update.setAccessible(true);
        NativeTeaching.overrideState=2L|(1L<<34); // Complete lesson; authoritative clearance only.
        assertTrue((Boolean)update.invoke(activity,1));
        assertEquals(View.VISIBLE,prompt.getVisibility());
        assertEquals("Step back from the skeletons.",prompt.getText().toString());
        assertEquals(1f,prompt.getAlpha(),0f);
        assertEquals("Step back from the skeletons.",prompt.getContentDescription().toString());
        field("menuVisible").setBoolean(activity,true);
        assertFalse((Boolean)update.invoke(activity,1));
        assertEquals(View.GONE,prompt.getVisibility());
        field("menuVisible").setBoolean(activity,false);
        assertTrue((Boolean)update.invoke(activity,1));
        Shadows.shadowOf(Looper.getMainLooper()).idleFor(Duration.ofMillis(1));
        NativeTeaching.overrideState=2L;
        assertTrue("the existing fade keeps timely polling until its final clear",
                (Boolean)update.invoke(activity,1));
        assertEquals("the non-protection hint may use the existing 120 ms fade",View.VISIBLE,prompt.getVisibility());
        assertEquals("Step back from the skeletons.",prompt.getText().toString());
        Shadows.shadowOf(Looper.getMainLooper()).idleFor(Duration.ofMillis(121));
        assertFalse((Boolean)update.invoke(activity,1));
        assertEquals(View.GONE,prompt.getVisibility());
        assertEquals("",prompt.getText().toString());
        assertEquals("",prompt.getContentDescription().toString());
    }

    @Test @GraphicsMode(GraphicsMode.Mode.NATIVE)
    public void actualPracticeWidgetKeepsProtectionSkillAndWarningReadableInBothOrientations()throws Exception {
        String originalQualifiers=RuntimeEnvironment.getQualifiers();
        String evidence=System.getenv("HORDE_PRACTICE_UI_EVIDENCE_DIR");
        try {
            for(int[] size:new int[][]{{320,568},{640,320}})for(float font:new float[]{1,2}) {
                RuntimeEnvironment.setQualifiers("w"+size[0]+"dp-h"+size[1]+"dp-"+(size[0]>size[1]?"land":"port")+"-mdpi");
                MainActivity activity=activeActivity();
                Configuration c=new Configuration(activity.getResources().getConfiguration());c.fontScale=font;
                activity.getResources().updateConfiguration(c,activity.getResources().getDisplayMetrics());
                activity.setContentView(R.layout.activity_main);
                ViewGroup root=activity.findViewById(R.id.root);
                TextView prompt=activity.findViewById(R.id.combat_teaching_prompt);field("combatTeachingPrompt").set(activity,prompt);
                for(int i=0;i<root.getChildCount();i++)if(root.getChildAt(i)!=prompt)root.getChildAt(i).setVisibility(View.GONE);
                java.lang.reflect.Method update=MainActivity.class.getDeclaredMethod("updateCombatTeachingPrompt",int.class);update.setAccessible(true);
                for(int stage:new int[]{4,5}) {
                    NativeTeaching.overrideState=stage|(1L<<13)|((stage==4?1L:2L)<<48);
                    assertTrue((Boolean)update.invoke(activity,1));
                    root.measure(View.MeasureSpec.makeMeasureSpec(size[0],View.MeasureSpec.EXACTLY),View.MeasureSpec.makeMeasureSpec(size[1],View.MeasureSpec.EXACTLY));
                    root.layout(0,0,size[0],size[1]);
                    String scenario=size[0]+"x"+size[1]+"-font"+(int)(font*100)+"-stage"+stage;
                    Rect bounds=new Rect(prompt.getLeft(),prompt.getTop(),prompt.getRight(),prompt.getBottom());
                    StringBuilder lineMetrics=new StringBuilder();
                    for(int line=0;line<prompt.getLayout().getLineCount();++line) {
                        Rect ink=new Rect();int start=prompt.getLayout().getLineStart(line),visibleEnd=prompt.getLayout().getLineVisibleEnd(line);
                        prompt.getPaint().getTextBounds(prompt.getText().toString(),start,visibleEnd,ink);
                        lineMetrics.append(line).append(":advance=").append(prompt.getLayout().getLineWidth(line))
                            .append(" visible=").append(prompt.getLayout().getLineMax(line)).append(" ink=").append(ink)
                            .append(" start=").append(start).append(" visibleEnd=").append(visibleEnd).append(" end=").append(prompt.getLayout().getLineEnd(line)).append(' ');
                    }
                    if(evidence!=null && !evidence.isEmpty()) {
                        File file=new File(evidence,"android-widget-fixture-practice-"+scenario+".png");
                        assertTrue(file.getParentFile().isDirectory() || file.getParentFile().mkdirs());
                        Bitmap image=Bitmap.createBitmap(size[0],size[1],Bitmap.Config.ARGB_8888);root.draw(new Canvas(image));
                        try(FileOutputStream out=new FileOutputStream(file)){assertTrue(image.compress(Bitmap.CompressFormat.PNG,100,out));}
                        Files.write(new File(evidence,file.getName()+".txt").toPath(),("HOST ANDROID WIDGET FIXTURE ONLY; no phone, RT world or combat acceptance.\nviewport="+scenario+
                            " prompt="+bounds+" layoutWidth="+prompt.getLayout().getWidth()+" contentWidth="+(prompt.getWidth()-prompt.getCompoundPaddingLeft()-prompt.getCompoundPaddingRight())+" lineWidths="+lineMetrics+" lines="+prompt.getLayout().getLineCount()+" textPx="+prompt.getTextSize()+"\n"+prompt.getText()).getBytes(java.nio.charset.StandardCharsets.UTF_8));
                    }
                    assertTrue(scenario+" actual prompt stays on viewport "+bounds,bounds.left>=0 && bounds.right<=size[0] && bounds.top>=0 && bounds.bottom<=size[1]);
                    assertFalse(prompt.isClickable());assertFalse(prompt.isFocusable());
                    assertTrue(prompt.getText().toString().startsWith("Practice: no damage"));
                    assertTrue(prompt.getText().toString().contains(stage==4?"block normal hits":"Hold [Parry] + [Swing]"));
                    assertEquals(prompt.getText().length(),prompt.getLayout().getLineEnd(prompt.getLayout().getLineCount()-1));
                    for(int line=0;line<prompt.getLayout().getLineCount();++line) {
                        assertEquals(0,prompt.getLayout().getEllipsisCount(line));
                        // getLineWidth includes trailing whitespace. Visible glyphs, not a newline's
                        // advance, must fit; retain the original one-pixel rounding allowance.
                        int start=prompt.getLayout().getLineStart(line),visibleEnd=prompt.getLayout().getLineVisibleEnd(line),end=prompt.getLayout().getLineEnd(line);
                        String all=prompt.getText().toString();
                        for(int offset=visibleEnd;offset<end;++offset)assertTrue(scenario+" only whitespace excluded at "+offset,Character.isWhitespace(all.charAt(offset)));
                        Rect ink=new Rect();prompt.getPaint().getTextBounds(all,start,visibleEnd,ink);
                        int contentWidth=prompt.getWidth()-prompt.getCompoundPaddingLeft()-prompt.getCompoundPaddingRight();
                        assertTrue(scenario+" full visible glyph line "+line+" bounds="+bounds+" allWidths="+lineMetrics,
                            prompt.getLayout().getLineMax(line)<=contentWidth+1 && ink.width()<=contentWidth+1);
                    }

                }
            }
        } finally {RuntimeEnvironment.setQualifiers(originalQualifiers);}
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
