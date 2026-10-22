package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;

import android.content.Context;
import android.view.Gravity;
import android.widget.Button;
import android.widget.FrameLayout;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34)
public final class LanternPoseControlTest {
    @Test public void compactIconKeepsSafeMarginsAndExistingAction() {
        Context context = RuntimeEnvironment.getApplication();
        Button button = new Button(context);
        FrameLayout.LayoutParams slot = new FrameLayout.LayoutParams(128, 60);
        slot.gravity = Gravity.BOTTOM | Gravity.END;
        slot.setMarginEnd(164);
        slot.bottomMargin = 200;
        button.setLayoutParams(slot);
        AtomicInteger actions = new AtomicInteger();
        button.setOnClickListener(view -> actions.incrementAndGet());

        LanternPoseControl.apply(button, false, "Raise lantern");

        assertEquals(HordeUiTokens.dp(context, 56), slot.width);
        assertEquals(slot.width, slot.height);
        assertEquals(164, slot.getMarginEnd());
        assertEquals(200, slot.bottomMargin);
        assertEquals(Gravity.BOTTOM | Gravity.END, slot.gravity);
        assertEquals("", button.getText().toString());
        assertEquals("Raise lantern. Lantern lowered.", button.getContentDescription());
        assertFalse(button.isActivated());
        assertNotNull(button.getCompoundDrawables()[0]);
        assertEquals(HordeUiTokens.dp(context, 32), button.getCompoundDrawables()[0].getBounds().width());
        button.performClick();
        assertEquals(1, actions.get());
    }

    @Test public void stateChangeReplacesArrowAndAccessibleActionWithoutText() {
        Context context = RuntimeEnvironment.getApplication();
        Button button = new Button(context);
        button.setLayoutParams(new FrameLayout.LayoutParams(128, 60));
        LanternPoseControl.apply(button, false, "Raise lantern");
        Object loweredIcon = button.getCompoundDrawables()[0];

        LanternPoseControl.apply(button, true, "Lower lantern");

        assertTrue(button.isActivated());
        assertEquals("Lower lantern. Lantern raised.", button.getContentDescription());
        assertEquals("", button.getText().toString());
        assertNotSame(loweredIcon, button.getCompoundDrawables()[0]);
        Object raisedIcon = button.getCompoundDrawables()[0];
        LanternPoseControl.apply(button, true, "Lower lantern");
        assertSame(raisedIcon, button.getCompoundDrawables()[0]);
    }
}
