package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.GraphicsMode;

@RunWith(RobolectricTestRunner.class) @Config(sdk=34)
@GraphicsMode(GraphicsMode.Mode.NATIVE)
public final class VitalityHeartsTest {
    @Test public void allHealthStatesRenderThreeFullOrEmptyHeartsAndKeepCurrentMaxText() {
        Context c=RuntimeEnvironment.getApplication();
        int previousFullPixels=-1;
        for(int health=0;health<=3;++health) {
            VitalityHeartsDrawable d=new VitalityHeartsDrawable(c,health);
            Rect bounds=d.getBounds();
            Bitmap b=Bitmap.createBitmap(bounds.width(),bounds.height(),Bitmap.Config.ARGB_8888);
            d.draw(new Canvas(b));
            int fullPixels=0;
            for(int y=0;y<b.getHeight();++y) for(int x=0;x<b.getWidth();++x)
                if(b.getPixel(x,y)==0xFFCF6557) ++fullPixels;
            assertTrue("each health point must add a visibly full heart",fullPixels>previousFullPixels);
            previousFullPixels=fullPixels;
            // The three empty silhouettes remain visible at zero health.
            for(int third=0;third<3;++third) {
                boolean visible=false;
                for(int y=0;y<b.getHeight();++y) for(int x=third*b.getWidth()/3;x<(third+1)*b.getWidth()/3;++x)
                    visible|=(b.getPixel(x,y)>>>24)!=0;
                assertTrue(visible);
            }
            assertEquals("Vitality "+health+" of 3",c.getString(R.string.vitality_accessibility,health));
        }
    }
}
