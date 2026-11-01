package com.samfa12.hordelanternrt;
import static org.junit.Assert.*;
import android.view.MotionEvent;
import android.widget.Button;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
@RunWith(RobolectricTestRunner.class) @Config(sdk=34)
public class PressActionButtonHoldTest {
    @Test public void downIsImmediateReleaseCancelAndAccessibilityDoNotInventHeldAttack() {
        Button button=new Button(RuntimeEnvironment.getApplication());int[] actions={0},changes={0};
        PressActionButton input=new PressActionButton(button,()->true,()->actions[0]++,()->changes[0]++);
        MotionEvent down=MotionEvent.obtain(1,1,MotionEvent.ACTION_DOWN,1,1,0);
        MotionEvent up=MotionEvent.obtain(1,2,MotionEvent.ACTION_UP,1,1,0);
        input.onTouch(button,down);assertEquals(1,actions[0]);assertTrue(input.held());
        input.onTouch(button,up);assertEquals(1,actions[0]);assertFalse(input.held());
        input.onTouch(button,down);assertTrue(input.held());input.cancel();assertFalse(input.held());
        input.onClick(button);assertEquals(3,actions[0]);assertFalse(input.held());assertEquals(4,changes[0]);
        down.recycle();up.recycle();
    }
}
