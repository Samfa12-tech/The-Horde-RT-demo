package com.samfa12.hordelanternrt;
import static org.junit.Assert.*;
import android.view.View;
import android.widget.*;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
@RunWith(RobolectricTestRunner.class) @Config(sdk=34)
public class ControllerNavigationTest {
    @Test public void disabledFocusIsRepairedAndSlidersUseNativeUserChangeRoute() {
        MainActivity a=Robolectric.buildActivity(MainActivity.class).get();LinearLayout root=new LinearLayout(a);root.setOrientation(LinearLayout.VERTICAL);
        Button first=new Button(a);root.addView(first);SeekBar slider=new SeekBar(a);slider.setMax(100);slider.setProgress(40);root.addView(slider);
        Button last=new Button(a);root.addView(last);a.setContentView(root);
        final boolean[] fromUser={false};slider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){
            public void onProgressChanged(SeekBar b,int p,boolean user){fromUser[0]=user;}
            public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){} });
        root.measure(View.MeasureSpec.makeMeasureSpec(500,View.MeasureSpec.EXACTLY),
                     View.MeasureSpec.makeMeasureSpec(500,View.MeasureSpec.EXACTLY));
        root.layout(0,0,500,500);
        ControllerNavigation.ensureFocus(root);assertTrue(first.hasFocus());
        first.setEnabled(false);ControllerNavigation.ensureFocus(root);assertTrue(slider.hasFocus());
        ControllerNavigation.navigate(root,1,0);assertTrue(slider.getProgress()>40);assertTrue(fromUser[0]);
        ControllerNavigation.navigate(root,0,1);assertTrue(last.hasFocus());
    }
    @Test public void rebuildUsesEnabledNativeControlsAndConfirmIsOneRealClick() {
        MainActivity a=Robolectric.buildActivity(MainActivity.class).get();FrameLayout root=new FrameLayout(a);a.setContentView(root);
        Button old=new Button(a);root.addView(old);ControllerNavigation.ensureFocus(root);root.removeAllViews();
        LinearLayout page=new LinearLayout(a);page.setOrientation(LinearLayout.VERTICAL);root.addView(page);
        Button disabled=new Button(a);disabled.setEnabled(false);page.addView(disabled);
        Button restored=new Button(a);int[] clicks={0};restored.setOnClickListener(v->clicks[0]++);page.addView(restored);
        assertEquals(restored,ControllerNavigation.ensureFocus(root));assertTrue(ControllerNavigation.confirm(root));assertEquals(1,clicks[0]);
    }
    private static void layout(FrameLayout root, Button button, int left, int top) {
        root.addView(button, new FrameLayout.LayoutParams(100, 60));
        button.layout(left, top, left + 100, top + 60);
    }
    @Test public void rightAtColumnEdgeDoesNotJumpDownByCreationOrder() {
        MainActivity a=Robolectric.buildActivity(MainActivity.class).get();
        FrameLayout root=new FrameLayout(a);a.setContentView(root);root.layout(0,0,500,500);
        Button current=new Button(a),below=new Button(a);
        layout(root,current,20,100);layout(root,below,20,220);
        ControllerNavigation.ensureFocus(root);
        ControllerNavigation.navigate(root,1,0);
        assertTrue("Right with no right-hand control must retain selection",current.hasFocus());
    }
    @Test public void bottomRowDownDoesNotSelectItsRightHandNeighbor() {
        MainActivity a=Robolectric.buildActivity(MainActivity.class).get();
        FrameLayout root=new FrameLayout(a);a.setContentView(root);root.layout(0,0,500,500);
        Button more=new Button(a),settings=new Button(a),play=new Button(a);
        layout(root,more,20,320);layout(root,settings,340,320);layout(root,play,180,200);
        ControllerNavigation.ensureFocus(root);
        ControllerNavigation.navigate(root,0,1);
        assertTrue("Down at the bottom cannot become Right",more.hasFocus());
        ControllerNavigation.navigate(root,1,0);assertTrue(settings.hasFocus());
        ControllerNavigation.navigate(root,0,-1);assertTrue(play.hasFocus());
    }
    @Test public void focusSearchIsScopedToTheMenuRatherThanUnderlyingControls() {
        MainActivity a=Robolectric.buildActivity(MainActivity.class).get();
        FrameLayout outer=new FrameLayout(a),menu=new FrameLayout(a);
        a.setContentView(outer);outer.addView(menu,new FrameLayout.LayoutParams(500,500));
        outer.layout(0,0,600,600);menu.layout(0,0,500,500);
        Button current=new Button(a),below=new Button(a),right=new Button(a),hud=new Button(a);
        layout(menu,current,20,100);layout(menu,below,20,250);layout(menu,right,320,100);
        layout(outer,hud,160,100);
        for(Button b:new Button[]{current,below,right,hud})b.setFocusableInTouchMode(true);
        current.requestFocus();ControllerNavigation.navigate(menu,1,0);
        assertTrue("Closer gameplay HUD must not divert menu direction",right.hasFocus());
    }

    @Test public void controllerSelectionPreservesNativeFirstTouchClickPolicy() {
        MainActivity a=Robolectric.buildActivity(MainActivity.class).get();
        FrameLayout root=new FrameLayout(a);a.setContentView(root);root.layout(0,0,500,500);
        Button first=new Button(a),right=new Button(a);
        layout(root,first,20,100);layout(root,right,240,100);
        assertFalse(first.isFocusableInTouchMode());assertFalse(right.isFocusableInTouchMode());
        ControllerNavigation.ensureFocus(root);ControllerNavigation.navigate(root,1,0);
        assertTrue(right.hasFocus());
        ControllerNavigation.restoreTouchPolicy(root);
        assertFalse("Touch fallback must restore ordinary one-tap button behavior",first.isFocusableInTouchMode());
        assertFalse(right.isFocusableInTouchMode());
    }

    @Test public void verticalNavigationRevealsOffscreenControlsWithoutHorizontalScroll() {
        MainActivity a=Robolectric.buildActivity(MainActivity.class).get();
        ScrollView root=new ScrollView(a);LinearLayout page=new LinearLayout(a);
        page.setOrientation(LinearLayout.VERTICAL);root.addView(page);a.setContentView(root);
        Button first=new Button(a),last=new Button(a);View explanation=new View(a);
        page.addView(first,new LinearLayout.LayoutParams(300,80));
        page.addView(explanation,new LinearLayout.LayoutParams(300,600));
        page.addView(last,new LinearLayout.LayoutParams(300,80));
        root.measure(View.MeasureSpec.makeMeasureSpec(300,View.MeasureSpec.EXACTLY),
                     View.MeasureSpec.makeMeasureSpec(200,View.MeasureSpec.EXACTLY));
        root.layout(0,0,300,200);
        ControllerNavigation.ensureFocus(root);assertTrue(first.hasFocus());
        ControllerNavigation.navigate(root,0,1);assertTrue(last.hasFocus());assertTrue(root.getScrollY()>0);
        int scroll=root.getScrollY();ControllerNavigation.navigate(root,1,0);
        assertTrue(last.hasFocus());assertEquals("Horizontal input cannot scroll a vertical page",scroll,root.getScrollY());
        ControllerNavigation.navigate(root,0,-1);assertTrue(first.hasFocus());assertEquals(0,root.getScrollY());
    }

}
