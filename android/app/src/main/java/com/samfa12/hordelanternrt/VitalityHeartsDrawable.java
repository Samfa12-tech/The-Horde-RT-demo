package com.samfa12.hordelanternrt;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.graphics.drawable.Drawable;

/** Original vector artwork; all three full/empty hearts supplement current/max text. */
final class VitalityHeartsDrawable extends Drawable {
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path heart = new Path();
    private final int filled;
    private int alpha = 255;

    VitalityHeartsDrawable(Context context, int vitality) {
        filled = Math.max(0, Math.min(3, vitality));
        setBounds(0, 0, HordeUiTokens.dp(context, 72), HordeUiTokens.dp(context, 22));
        heart.moveTo(12, 21);
        heart.cubicTo(9, 18, 2, 12, 2, 7);
        heart.cubicTo(2, 1, 9, 0, 12, 5);
        heart.cubicTo(15, 0, 22, 1, 22, 7);
        heart.cubicTo(22, 12, 15, 18, 12, 21);
        heart.close();
    }
    @Override public void draw(Canvas canvas) {
        float gap = getBounds().width() / 18f;
        float width = (getBounds().width() - 2 * gap) / 3f;
        for (int i = 0; i < 3; ++i) {
            int saved = canvas.save();
            canvas.translate(getBounds().left + i * (width + gap), getBounds().top);
            canvas.scale(width / 24f, getBounds().height() / 24f);
            paint.setStyle(Paint.Style.FILL);
            paint.setColor(i < filled ? 0xFFCF6557 : HordeUiTokens.SLATE);
            paint.setAlpha(alpha); canvas.drawPath(heart, paint);
            paint.setStyle(Paint.Style.STROKE); paint.setStrokeWidth(1.5f);
            paint.setColor(HordeUiTokens.PARCHMENT); paint.setAlpha(alpha);
            canvas.drawPath(heart, paint);
            canvas.restoreToCount(saved);
        }
    }
    @Override public void setAlpha(int alpha) { this.alpha = Math.max(0, Math.min(255, alpha)); invalidateSelf(); }
    @Override public void setColorFilter(ColorFilter filter) { paint.setColorFilter(filter); invalidateSelf(); }
    @Override public int getOpacity() { return PixelFormat.TRANSLUCENT; }
}
