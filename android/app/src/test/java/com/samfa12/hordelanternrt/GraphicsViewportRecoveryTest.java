package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.HorizontalScrollView;
import android.widget.ScrollView;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.GraphicsMode;
import org.robolectric.shadows.ShadowLooper;

@RunWith(RobolectricTestRunner.class) @Config(sdk=34)
@GraphicsMode(GraphicsMode.Mode.NATIVE)
public final class GraphicsViewportRecoveryTest {
    private static void set(MainActivity activity, String name, Object value) throws Exception {
        Field f = MainActivity.class.getDeclaredField(name); f.setAccessible(true); f.set(activity,value);
    }
    private static void show(MainActivity activity, String name) throws Exception {
        Method m = MainActivity.class.getDeclaredMethod(name); m.setAccessible(true); m.invoke(activity);
    }
    private static void layout(View v) {
        v.measure(View.MeasureSpec.makeMeasureSpec(360,View.MeasureSpec.EXACTLY),
                View.MeasureSpec.makeMeasureSpec(640,View.MeasureSpec.EXACTLY));
        v.layout(0,0,360,640);
        v.getViewTreeObserver().dispatchOnGlobalLayout();
        ShadowLooper.idleMainLooper();
    }
    private static Button button(View root, String prefix) {
        if (root instanceof Button && ((Button)root).getText().toString().startsWith(prefix)) return (Button)root;
        if (root instanceof ViewGroup) {
            ViewGroup g=(ViewGroup)root;
            for (int i=0;i<g.getChildCount();++i) { Button b=button(g.getChildAt(i),prefix); if(b!=null)return b; }
        }
        return null;
    }
    private static View tagged(View root, String tag) {
        if(tag.equals(root.getTag())) return root;
        if(root instanceof ViewGroup) {
            ViewGroup group=(ViewGroup)root;
            for(int i=0;i<group.getChildCount();++i) {
                View found=tagged(group.getChildAt(i),tag);
                if(found!=null) return found;
            }
        }
        return null;
    }
    private static boolean insideHorizontalScroll(View view) {
        View current=view;
        while(current.getParent() instanceof View) {
            if(current.getParent() instanceof HorizontalScrollView) return true;
            current=(View)current.getParent();
        }
        return false;
    }
    private static MainActivity activity(FrameLayout[] holder) throws Exception {
        MainActivity a=Robolectric.buildActivity(MainActivity.class).get();
        holder[0]=new FrameLayout(a); holder[0].setFocusableInTouchMode(true);
        set(a,"menuScrim",holder[0]); set(a,"graphicsDraft",GraphicsPreferences.mobileDefaults());
        set(a,"graphicsConfirmed",GraphicsPreferences.mobileDefaults());
        return a;
    }
    @Test public void laterDraftChoiceRetainsViewportAndKeyboardFocus() throws Exception {
        FrameLayout[] holder=new FrameLayout[1]; MainActivity a=activity(holder); FrameLayout scrim=holder[0];
        show(a,"showGraphicsPage"); layout(scrim);
        ScrollView before=(ScrollView)tagged(scrim,"graphics-page-scroll"); assertNotNull(before); before.scrollTo(0,1100);
        Button mist=button(scrim,"Mist:"); assertNotNull(mist);
        mist.setFocusableInTouchMode(true); assertTrue(mist.requestFocus());
        int y=before.getScrollY(); assertTrue(y>0);
        mist.performClick(); layout(scrim);
        ScrollView after=(ScrollView)tagged(scrim,"graphics-page-scroll"); assertNotNull(after);
        assertEquals("a later setting must not jump back to the page title",y,after.getScrollY());
        assertTrue("the same control keeps focus after its label changes",button(scrim,"Mist:").hasFocus());
        Button use=button(scrim,"Use"), keep=button(scrim,"Keep"), restore=button(scrim,"Restore"), back=button(scrim,"Back");
        assertNotNull(use); assertNotNull(keep); assertNotNull(restore); assertNotNull(back);
        assertEquals("Use these settings for a temporary trial. Saved settings change only after a matching RT presentation and Keep and save.",use.getContentDescription());
        assertTrue(keep.getContentDescription().toString().contains("acknowledged trial"));
        for(Button action:new Button[]{use,keep,restore,back}) assertFalse(insideHorizontalScroll(action));
    }
    @Test public void freshGraphicsPageCanPublishPreviewDetailsToInitiallyNullDescription() throws Exception {
        FrameLayout[] holder=new FrameLayout[1]; MainActivity a=activity(holder); FrameLayout scrim=holder[0];
        show(a,"showGraphicsPage"); layout(scrim);
        Field telemetryField=MainActivity.class.getDeclaredField("graphicsTelemetry"); telemetryField.setAccessible(true);
        android.widget.TextView telemetry=(android.widget.TextView)telemetryField.get(a);
        assertNull("fresh ordinary Graphics page starts without an accessibility description",telemetry.getContentDescription());
        Method refresh=MainActivity.class.getDeclaredMethod("refreshGraphicsPreviewTelemetry",long[].class,String.class);
        refresh.setAccessible(true);
        refresh.invoke(a,new long[32],"Requested 75% / Saved 75%");
        assertTrue(telemetry.getContentDescription().toString().startsWith("Requested 75% / Saved 75%"));
        Method update=MainActivity.class.getDeclaredMethod("updateGraphicsTelemetryAccessibilityDetails",CharSequence.class);
        update.setAccessible(true);
        CharSequence current=telemetry.getContentDescription();
        assertEquals("unchanged details do not re-emit accessibility updates",false,update.invoke(a,current));
        assertEquals("changed details update once",true,update.invoke(a,"Saved 75%; backend pending"));
        assertEquals("Saved 75%; backend pending",telemetry.getContentDescription().toString());
        assertEquals("the same details do not churn on the next poll",false,update.invoke(a,"Saved 75%; backend pending"));
    }
    @Test public void rebuiltPreviewRetainsScrollableOptionsAndFocusedControlWithPersistentActions() throws Exception {
        FrameLayout[] holder=new FrameLayout[1]; MainActivity a=activity(holder); FrameLayout scrim=holder[0];
        show(a,"showGraphicsPreviewPage"); layout(scrim);
        Button mist=button(scrim,"Mist: "); assertNotNull(mist); mist.setEnabled(true);
        mist.setFocusableInTouchMode(true); assertTrue(mist.requestFocus());
        HorizontalScrollView options=(HorizontalScrollView)mist.getParent().getParent();
        options.scrollTo(650,0);
        int ox=options.getScrollX(); assertTrue(ox>0);
        set(a,"graphicsDraft",GraphicsPreviewOptions.withChoice(GraphicsPreferences.mobileDefaults(),GraphicsPreviewOptions.MIST,0));
        show(a,"showGraphicsPreviewPage");
        Button replacement=button(scrim,"Mist: "); replacement.setEnabled(true);
        layout(scrim);
        assertEquals(ox,((HorizontalScrollView)replacement.getParent().getParent()).getScrollX());
        Button use=button(scrim,"Use"), keep=button(scrim,"Keep"), restore=button(scrim,"Restore"), back=button(scrim,"Back");
        assertNotNull(use); assertNotNull(keep); assertNotNull(restore); assertNotNull(back);
        for(Button action:new Button[]{use,keep,restore,back}) assertFalse(insideHorizontalScroll(action));
        assertEquals("Save only the settings confirmed by a matching RT presentation.",keep.getContentDescription());
        // The native ACK temporarily disables options. Restore focus as soon as editing is available.
        assertTrue(replacement.hasFocus());
    }
}
