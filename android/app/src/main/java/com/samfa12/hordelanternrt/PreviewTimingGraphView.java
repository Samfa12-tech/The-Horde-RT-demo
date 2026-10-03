package com.samfa12.hordelanternrt;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.view.View;

/** Native bounded timing trace; amber bars retain transition stalls. */
final class PreviewTimingGraphView extends View {
    private final Paint paint = new Paint();
    private double[] values = new double[0];
    PreviewTimingGraphView(Context context) { super(context); setContentDescription("Preview loop interval graph; amber marks transitions"); }
    void setSamples(double[] snapshot) {
        final int count = snapshot.length >= 9 ? Math.min(128, Math.max(0, (int)snapshot[8])) : 0;
        if (snapshot.length != 9 + count * 2) return;
        values = snapshot.clone(); invalidate();
    }
    @Override protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        canvas.drawColor(0xD9151719);
        if (values.length < 9) return;
        final int count = (int)values[8];
        double max = 33.333;
        for (int i=0;i<count;++i) max = Math.max(max, values[9+i*2]);
        final float width = getWidth() / 128.0f;
        for (int i=0;i<count;++i) {
            paint.setColor(values[10+i*2] != 0 ? 0xFFCFA96A : 0xFFF2E9D8);
            final float top = getHeight() * (1 - (float)(values[9+i*2] / max));
            canvas.drawRect(i*width, top, (i+1)*width, getHeight(), paint);
        }
    }
}
