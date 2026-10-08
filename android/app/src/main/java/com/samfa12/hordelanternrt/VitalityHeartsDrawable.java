package com.samfa12.hordelanternrt;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.graphics.drawable.Drawable;

/** Original heart artwork. Empty hearts have transparent interiors; health text is accessibility-only. */
final class VitalityHeartsDrawable extends Drawable {
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path heart = new Path();
    private final int filled;
    private final int count, columns, size, gap;
    private int alpha = 255;

    VitalityHeartsDrawable(Context context, int vitality, int maximum, int availableWidth) {
        count = Math.max(0, maximum);
        filled = Math.max(0, Math.min(count, vitality));
        size = HordeUiTokens.dp(context, 22);
        gap = HordeUiTokens.dp(context, 4);
        columns = Math.max(1, Math.min(count, (Math.max(size, availableWidth) + gap) / (size + gap)));
        int rows = count == 0 ? 0 : (count - 1) / columns + 1;
        setBounds(0, 0, count == 0 ? 0 : columns * (size + gap) - gap,
                rows == 0 ? 0 : rows * (size + gap) - gap);
        heart.moveTo(12, 21);
        heart.cubicTo(9, 18, 2, 12, 2, 7);
        heart.cubicTo(2, 1, 9, 0, 12, 5);
        heart.cubicTo(15, 0, 22, 1, 22, 7);
        heart.cubicTo(22, 12, 15, 18, 12, 21);
        heart.close();
    }
    @Override public void draw(Canvas canvas) {
        for (int i = 0; i < count; ++i) {
            int saved = canvas.save();
            canvas.translate(getBounds().left + (i % columns) * (size + gap),
                    getBounds().top + (i / columns) * (size + gap));
            canvas.scale(size / 24f, size / 24f);
            if (i < filled) {
                paint.setStyle(Paint.Style.FILL);
                paint.setColor(0xFFD64747);
                paint.setAlpha(alpha); canvas.drawPath(heart, paint);
            }
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
