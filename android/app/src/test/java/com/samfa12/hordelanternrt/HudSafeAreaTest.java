package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.graphics.Rect;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class) @Config(sdk=34)
public final class HudSafeAreaTest {
    @Test public void windowCutoutAlreadyExcludedByContentIsNotAddedTwiceAcrossRotation() {
        Rect portrait=new Rect(0,149,1440,3120),landscape=new Rect(139,0,3120,1440);
        assertEquals(new Rect(0,0,0,168),HudSafeArea.withinParent(portrait,new Rect(0,0,1440,3120),0,149,0,168));
        assertEquals(new Rect(0,0,168,0),HudSafeArea.withinParent(landscape,new Rect(0,0,3120,1440),139,0,168,0));
        // The same absolute safe content with a fully inset parent has no residual inset.
        assertEquals(new Rect(),HudSafeArea.withinParent(new Rect(139,0,2952,1440),new Rect(0,0,3120,1440),139,0,168,0));
        assertEquals(new Rect(0,0,0,168),HudSafeArea.withinParent(portrait,new Rect(0,0,1440,3120),0,149,0,168));
    }
}
