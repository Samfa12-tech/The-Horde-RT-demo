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
}
