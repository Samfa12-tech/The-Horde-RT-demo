package com.samfa12.hordelanternrt;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.text.Layout;
import android.text.TextPaint;
import android.widget.TextView;

/** Draw an outline behind the same caption layout that TextView uses for its fill. */
final class OutlinedSubtitleView extends TextView {
    private final float strokeWidth;

    OutlinedSubtitleView(Context context) {
        super(context);
        strokeWidth = 3f * getResources().getDisplayMetrics().density;
    }

    @Override protected void onDraw(Canvas canvas) {
        final Layout layout = getLayout();
        if (layout != null && length() != 0) {
            final TextPaint paint = layout.getPaint();
            final int previousColor = paint.getColor();
            final Paint.Style previousStyle = paint.getStyle();
            final Paint.Join previousJoin = paint.getStrokeJoin();
            final float previousWidth = paint.getStrokeWidth();
            final int savedCanvas = canvas.save();
            canvas.translate(getTotalPaddingLeft(), getTotalPaddingTop());
            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeJoin(Paint.Join.ROUND);
            paint.setStrokeWidth(strokeWidth);
            paint.setColor(Color.BLACK);
            layout.draw(canvas);
            paint.setColor(previousColor);
            paint.setStrokeWidth(previousWidth);
            paint.setStrokeJoin(previousJoin);
            paint.setStyle(previousStyle);
            canvas.restoreToCount(savedCanvas);
        }
        super.onDraw(canvas);
    }
}
