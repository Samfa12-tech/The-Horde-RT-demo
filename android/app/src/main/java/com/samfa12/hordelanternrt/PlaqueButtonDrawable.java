package com.samfa12.hordelanternrt;

import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;

/** Static, stateful painted iron-and-brass plate for native Android buttons. */
final class PlaqueButtonDrawable extends Drawable {
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF rect = new RectF();
    private final int normalFill;
    private final float unit;
    private int alpha = 255;
    private boolean enabled = true, pressed, focused, selected;

    PlaqueButtonDrawable(int fill, float density) {
        normalFill = fill;
        unit = Math.max(0.75f, density);
        paint.setStrokeCap(Paint.Cap.SQUARE);
        paint.setStrokeJoin(Paint.Join.ROUND);
    }

    @Override public void draw(Canvas canvas) {
        final float left = getBounds().left;
        final float top = getBounds().top;
        final float right = getBounds().right;
        final float bottom = getBounds().bottom;
        final float u = unit;
        if (right - left < 4.0f * u || bottom - top < 4.0f * u) return;

        final int fill = enabled ? (pressed ? darken(normalFill, 0.70f) : normalFill) : 0xFF34383A;
        final int rim = enabled ? (pressed ? 0xFF8F7447 : HordeUiTokens.BRASS) : 0xFF8C9294;
        final float bottomLift = pressed ? 1.0f * u : 2.5f * u;
        final float faceTop = top + (pressed ? 2.5f : 1.8f) * u;
        final float faceBottom = bottom - (pressed ? 1.8f : 3.4f) * u;
        final float radius = 4.0f * u;

        // Dark lower edge gives the plate a compact raised profile without a large shadow.
        rect.set(left + 1.0f*u, top + 1.3f*u, right - 1.0f*u, bottom - 0.5f*u);
        fill(canvas, rect, radius, enabled ? 0xFF080A0B : 0xFF101112, Paint.Style.FILL, 0.0f);
        rect.set(left + 1.0f*u, top + 1.0f*u, right - 1.0f*u, bottom - bottomLift);
        fill(canvas, rect, radius, rim, Paint.Style.FILL, 0.0f);

        rect.set(left + 2.2f*u, faceTop, right - 2.2f*u, Math.max(faceTop + 1.0f*u, faceBottom));
        fill(canvas, rect, 3.0f*u, fill, Paint.Style.FILL, 0.0f);

        // Fine upper-left bevel and a darker lower-right bevel keep the face legible at small sizes.
        rect.inset(0.7f*u, 0.7f*u);
        fill(canvas, rect, 2.5f*u, enabled ? 0x70FFF0C8 : 0x607D8588, Paint.Style.STROKE, 0.7f*u);
        rect.inset(0.8f*u, 0.8f*u);
        fill(canvas, rect, 2.0f*u, enabled ? 0x903C2E1C : 0x80707476, Paint.Style.STROKE, 0.8f*u);

        drawRivet(canvas, left + 6.0f*u, top + 6.0f*u, enabled);
        drawRivet(canvas, right - 6.0f*u, top + 6.0f*u, enabled);
        drawRivet(canvas, left + 6.0f*u, bottom - 7.0f*u, enabled);
        drawRivet(canvas, right - 6.0f*u, bottom - 7.0f*u, enabled);

        if (focused || selected) {
            rect.set(left + 2.0f*u, top + 2.0f*u, right - 2.0f*u, bottom - 2.0f*u);
            fill(canvas, rect, 3.2f*u, 0xFFFFE8B7, Paint.Style.STROKE, 2.0f*u);
        } else if (pressed) {
            rect.set(left + 2.4f*u, top + 2.4f*u, right - 2.4f*u, bottom - 2.2f*u);
            fill(canvas, rect, 3.0f*u, 0xE7E9D5B7, Paint.Style.STROKE, 1.0f*u);
        }
    }

    private void drawRivet(Canvas canvas, float x, float y, boolean active) {
        paint.setStyle(Paint.Style.FILL);
        paint.setStrokeWidth(1.0f*unit);
        paint.setColor(color(active ? 0xFF352B1E : 0xFF262A2C));
        canvas.drawCircle(x, y, 2.4f*unit, paint);
        paint.setColor(color(active ? 0xFFD7B878 : 0xFFBEC2C2));
        canvas.drawCircle(x, y, 1.45f*unit, paint);
        paint.setColor(color(active ? 0xFF59452B : 0xFF717779));
        paint.setStrokeWidth(0.8f*unit);
        canvas.drawLine(x - 0.65f*unit, y, x + 0.65f*unit, y, paint);
    }

    private void fill(Canvas canvas, RectF bounds, float radius, int color, Paint.Style style, float strokeWidth) {
        paint.setColor(color(color));
        paint.setStyle(style);
        paint.setStrokeWidth(strokeWidth);
        canvas.drawRoundRect(bounds, radius, radius, paint);
    }

    private int color(int value) {
        final int sourceAlpha = (value >>> 24) & 0xFF;
        final int outAlpha = (sourceAlpha * alpha + 127) / 255;
        return (value & 0x00FFFFFF) | (outAlpha << 24);
    }

    private static int darken(int color, float factor) {
        final int a = color & 0xFF000000;
        final int r = Math.round(((color >>> 16) & 0xFF) * factor);
        final int g = Math.round(((color >>> 8) & 0xFF) * factor);
        final int b = Math.round((color & 0xFF) * factor);
        return a | (r << 16) | (g << 8) | b;
    }

    @Override protected boolean onStateChange(int[] state) {
        boolean nextEnabled = false, nextPressed = false, nextFocused = false, nextSelected = false;
        for (int value : state) {
            if (value == android.R.attr.state_enabled) nextEnabled = true;
            else if (value == android.R.attr.state_pressed) nextPressed = true;
            else if (value == android.R.attr.state_focused) nextFocused = true;
            else if (value == android.R.attr.state_selected) nextSelected = true;
        }
        if (enabled == nextEnabled && pressed == nextPressed && focused == nextFocused && selected == nextSelected) return false;
        enabled = nextEnabled;
        pressed = nextPressed;
        focused = nextFocused;
        selected = nextSelected;
        invalidateSelf();
        return true;
    }

    @Override public boolean isStateful() { return true; }
    @Override public void setAlpha(int value) { alpha = Math.max(0, Math.min(255, value)); invalidateSelf(); }
    @Override public void setColorFilter(ColorFilter filter) { paint.setColorFilter(filter); invalidateSelf(); }
    @Override public int getOpacity() { return PixelFormat.TRANSLUCENT; }
}
