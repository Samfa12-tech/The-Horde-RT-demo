package com.samfa12.hordelanternrt;

import android.content.Context;
import android.widget.ScrollView;

/** Keep the default preview controls at the bottom; large text scrolls rather than covers the scene. */
final class GraphicsPreviewControlsScrollView extends ScrollView {
    private final int maximumHeight;
    GraphicsPreviewControlsScrollView(Context context, int maximumHeight) {
        super(context); this.maximumHeight = Math.max(1, maximumHeight);
        setFillViewport(false);
    }
    @Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int available = maximumHeight;
        if (MeasureSpec.getMode(heightMeasureSpec) != MeasureSpec.UNSPECIFIED)
            available = Math.min(available, MeasureSpec.getSize(heightMeasureSpec));
        super.onMeasure(widthMeasureSpec, MeasureSpec.makeMeasureSpec(available, MeasureSpec.AT_MOST));
    }
}
