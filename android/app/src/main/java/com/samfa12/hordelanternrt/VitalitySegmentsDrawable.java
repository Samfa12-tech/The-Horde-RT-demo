package com.samfa12.hordelanternrt;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.drawable.Drawable;

/** Three fixed segments supplement the real current/maximum text, without animation. */
final class VitalitySegmentsDrawable extends Drawable {
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final int filled;
    VitalitySegmentsDrawable(Context context,int vitality) {
        filled=Math.max(0,Math.min(3,vitality));
        setBounds(0,0,HordeUiTokens.dp(context,42),HordeUiTokens.dp(context,12));
    }
    @Override public void draw(Canvas c) {
        float gap=getBounds().width()/21f, width=(getBounds().width()-2*gap)/3f;
        for(int i=0;i<3;i++) {
            float x=getBounds().left+i*(width+gap);
            paint.setStyle(Paint.Style.FILL); paint.setColor(i<filled?HordeUiTokens.BRASS:HordeUiTokens.SLATE);
            c.drawRect(x,getBounds().top,x+width,getBounds().bottom,paint);
            paint.setStyle(Paint.Style.STROKE); paint.setStrokeWidth(1); paint.setColor(HordeUiTokens.PARCHMENT);
            c.drawRect(x+.5f,getBounds().top+.5f,x+width-.5f,getBounds().bottom-.5f,paint);
        }
    }
    @Override public void setAlpha(int alpha) { paint.setAlpha(alpha); invalidateSelf(); }
    @Override public void setColorFilter(ColorFilter filter) { paint.setColorFilter(filter); invalidateSelf(); }
    @Override public int getOpacity() { return PixelFormat.TRANSLUCENT; }
}
