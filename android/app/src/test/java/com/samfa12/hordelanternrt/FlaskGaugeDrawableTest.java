package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.widget.Button;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.GraphicsMode;

@RunWith(RobolectricTestRunner.class)
@Config(sdk=34)
@GraphicsMode(GraphicsMode.Mode.NATIVE)
public final class FlaskGaugeDrawableTest {
    private Bitmap render(int charges,int capacity) {
        Context c=RuntimeEnvironment.getApplication();
        FlaskGaugeDrawable d=new FlaskGaugeDrawable(c,charges,capacity,32);d.setBounds(0,0,320,400);
        Bitmap bitmap=Bitmap.createBitmap(320,400,Bitmap.Config.ARGB_8888);d.draw(new Canvas(bitmap));return bitmap;
    }
    @Test public void actualBodyPixelsShowFullHalfEmptyAndPersistentCorkOutline() {
        Bitmap full=render(2,2),half=render(1,2),empty=render(0,2);
        assertEquals(FlaskGaugeDrawable.LIQUID,full.getPixel(160,190));
        assertEquals(0,Color.alpha(half.getPixel(160,190)));
        assertEquals(FlaskGaugeDrawable.LIQUID,half.getPixel(160,300));
        assertEquals(0,Color.alpha(empty.getPixel(160,300)));
        assertTrue(Color.alpha(empty.getPixel(160,50))>0); // cork
        assertTrue(Color.alpha(empty.getPixel(60,270))>0); // bottle outline
        assertTrue(Color.alpha(empty.getPixel(160,255))>0); // half-capacity segment
    }
    @Test public void actualBodyPixelsRepresentThirdsRatherThanBinaryChargeIcons() {
        Bitmap one=render(1,3),two=render(2,3),full=render(3,3);
        assertEquals(0,Color.alpha(one.getPixel(160,240)));
        assertEquals(FlaskGaugeDrawable.LIQUID,one.getPixel(160,320));
        assertEquals(FlaskGaugeDrawable.LIQUID,two.getPixel(160,240));
        assertEquals(0,Color.alpha(two.getPixel(160,190)));
        assertEquals(FlaskGaugeDrawable.LIQUID,full.getPixel(160,190));
        assertTrue(Color.alpha(render(0,3).getPixel(160,223))>0); // upper third division
    }
    @Test public void repeatedPublicationReusesTheDrawableAndNamedAction() {
        Context c=RuntimeEnvironment.getApplication();Button button=new Button(c);
        button.setContentDescription("Drink. Flask 2 / 2, full");FlaskDrinkControl.apply(button,2,2);
        Drawable gauge=button.getCompoundDrawables()[0];FlaskDrinkControl.apply(button,1,2);
        assertSame(gauge,button.getCompoundDrawables()[0]);assertEquals("",button.getText().toString());
        assertEquals("Drink. Flask 2 / 2, full",button.getContentDescription().toString());
        gauge.setBounds(0,0,320,400);Bitmap bitmap=Bitmap.createBitmap(320,400,Bitmap.Config.ARGB_8888);
        gauge.draw(new Canvas(bitmap));assertEquals(0,Color.alpha(bitmap.getPixel(160,190)));
        assertEquals(FlaskGaugeDrawable.LIQUID,bitmap.getPixel(160,300));
        gauge.setAlpha(128);bitmap.eraseColor(Color.TRANSPARENT);gauge.draw(new Canvas(bitmap));
        assertEquals(128,Color.alpha(bitmap.getPixel(160,300)));
    }
}
