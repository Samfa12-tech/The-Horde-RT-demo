package com.samfa12.hordelanternrt;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.graphics.drawable.Drawable;

/** Original corked flask artwork. The body fill is charges/capacity, with outlined charge divisions. */
final class FlaskGaugeDrawable extends Drawable {
    static final int LIQUID=0xFFBCD8CC;
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path bottle=new Path();
    private final Path liquidBody=new Path();
    private int charges,capacity,alpha=255;

    FlaskGaugeDrawable(Context context,int charges,int capacity,int widthDp) {
        final int width=HordeUiTokens.dp(context,widthDp);
        setBounds(0,0,width,HordeUiTokens.dp(context,widthDp*40/32));
        bottle.moveTo(12,8);bottle.lineTo(12,12);bottle.cubicTo(12,14,6,15,6,20);
        bottle.lineTo(6,33);bottle.quadTo(6,37,10,37);bottle.lineTo(22,37);
        bottle.quadTo(26,37,26,33);bottle.lineTo(26,20);
        bottle.cubicTo(26,15,20,14,20,12);bottle.lineTo(20,8);bottle.close();
        liquidBody.addRoundRect(8,16,24,35,2,2,Path.Direction.CW);
        updateCharges(charges,capacity);
    }
    void updateCharges(int current,int maximum) {
        final int nextCapacity=Math.max(2,Math.min(3,maximum));
        final int nextCharges=Math.max(0,Math.min(nextCapacity,current));
        if(charges==nextCharges && capacity==nextCapacity)return;
        charges=nextCharges;capacity=nextCapacity;invalidateSelf();
    }
    @Override public void draw(Canvas canvas) {
        final int saved=canvas.save();
        canvas.translate(getBounds().left,getBounds().top);
        canvas.scale(getBounds().width()/32f,getBounds().height()/40f);
        paint.setStyle(Paint.Style.FILL);paint.setColor(LIQUID);paint.setAlpha(alpha);
        if(charges>0) {
            final int filled=canvas.save();
            canvas.clipRect(8,35-19f*charges/capacity,24,35);
            canvas.drawPath(liquidBody,paint);canvas.restoreToCount(filled);
        }
        // The cork and persistent outline make an empty flask recognizable without colour.
        paint.setColor(0xFFB49A77);paint.setAlpha(alpha);canvas.drawRect(11,3,21,8,paint);
        paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(1.4f);
        paint.setColor(HordeUiTokens.PARCHMENT);paint.setAlpha(alpha);
        canvas.drawRect(11,3,21,8,paint);canvas.drawPath(bottle,paint);
        canvas.drawLine(11,10,21,10,paint);
        paint.setStrokeWidth(.9f);
        for(int i=1;i<capacity;i++) {
            final float y=35-19f*i/capacity;canvas.drawLine(8,y,24,y,paint);
        }
        canvas.restoreToCount(saved);
    }
    @Override public void setAlpha(int value) {alpha=Math.max(0,Math.min(255,value));invalidateSelf();}
    @Override public void setColorFilter(ColorFilter filter) {paint.setColorFilter(filter);invalidateSelf();}
    @Override public int getOpacity() {return PixelFormat.TRANSLUCENT;}
}
