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
        ScrollView before=(ScrollView)scrim.getChildAt(0); before.scrollTo(0,1100);
        Button mist=button(scrim,"Mist:"); assertNotNull(mist);
        mist.setFocusableInTouchMode(true); assertTrue(mist.requestFocus());
        int y=before.getScrollY(); assertTrue(y>0);
        mist.performClick(); layout(scrim);
        ScrollView after=(ScrollView)scrim.getChildAt(0);
        assertEquals("a later setting must not jump back to the page title",y,after.getScrollY());
        assertTrue("the same control keeps focus after its label changes",button(scrim,"Mist:").hasFocus());
        assertNotNull(button(scrim,"Use these settings")); assertNotNull(button(scrim,"Keep and save"));
        assertNotNull(button(scrim,"Restore saved settings"));
    }
    @Test public void rebuiltPreviewRetainsBothHorizontalRowsAndFocusedControl() throws Exception {
        FrameLayout[] holder=new FrameLayout[1]; MainActivity a=activity(holder); FrameLayout scrim=holder[0];
        show(a,"showGraphicsPreviewPage"); layout(scrim);
        Button mist=button(scrim,"Mist "); assertNotNull(mist); mist.setEnabled(true);
        mist.setFocusableInTouchMode(true); assertTrue(mist.requestFocus());
        HorizontalScrollView options=(HorizontalScrollView)mist.getParent().getParent();
        HorizontalScrollView actions=(HorizontalScrollView)button(scrim,"Restore saved").getParent().getParent();
        options.scrollTo(650,0); actions.scrollTo(450,0);
        int ox=options.getScrollX(), ax=actions.getScrollX(); assertTrue(ox>0); assertTrue(ax>0);
        set(a,"graphicsDraft",GraphicsPreviewOptions.withChoice(GraphicsPreferences.mobileDefaults(),GraphicsPreviewOptions.MIST,0));
        show(a,"showGraphicsPreviewPage");
        Button replacement=button(scrim,"Mist "); replacement.setEnabled(true);
        layout(scrim);
        assertEquals(ox,((HorizontalScrollView)replacement.getParent().getParent()).getScrollX());
        assertEquals(ax,((HorizontalScrollView)button(scrim,"Restore saved").getParent().getParent()).getScrollX());
        // The native ACK temporarily disables options. Restore focus as soon as editing is available.
        assertTrue(replacement.hasFocus());
    }
}
