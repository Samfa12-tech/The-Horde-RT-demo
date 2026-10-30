package com.samfa12.hordelanternrt;

import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;

/** In-game backing only. Never changes the View's hit target, label or input state. */
final class TouchButtonDrawable extends Drawable {
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF face = new RectF();
    private final int normalFill;
    private final float unit;
    private final boolean circular, combat;
    private int alpha = 255;
    private boolean enabled = true, pressed, focused, selected, activated;

    TouchButtonDrawable(int fill, float density, boolean circular) {
        this(fill,density,circular,false);
    }
    TouchButtonDrawable(int fill,float density,boolean circular,boolean combat) {
        this.combat=combat;
        normalFill = fill;
        unit = Math.max(.75f, density);
        this.circular = circular;
    }

    @Override public void draw(Canvas canvas) {
        final float inset = 1.5f * unit;
        face.set(getBounds().left + inset, getBounds().top + inset,
                getBounds().right - inset, getBounds().bottom - inset);
        if (face.width() <= 0 || face.height() <= 0) return;
        if (circular) {
            final float radius = Math.min(face.width(), face.height()) / 2;
            final float x = face.centerX(), y = face.centerY();
            face.set(x - radius, y - radius, x + radius, y + radius);
        }
        final int fill = enabled ? normalFill : (normalFill & 0xFF000000) | 0x0034383A;
        // Long contextual instructions retain the full text box; do not pinch
        // their first/last letters behind a semicircular end cap.
        final float radius = circular ? face.height() / 2 : Math.min(16 * unit, face.height() / 2);
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(color(pressed && enabled ? darken(fill) : fill));
        canvas.drawRoundRect(face, radius, radius, paint);

        // A thin solid rim keeps contrast; there is no opaque plate under the face.
        final boolean outlined = focused || selected;
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth((outlined ? (combat?3f:2.2f) : (combat?2f:1f)) * unit);
        paint.setColor(color(!enabled ? HordeUiTokens.IRON :
                outlined || pressed ? 0xFFFFE8B7 : combat ? HordeUiTokens.PARCHMENT : activated ? HordeUiTokens.BRASS : 0xC8CFA96A));
        canvas.drawRoundRect(face, radius, radius, paint);
        if (outlined) {
            face.inset(3.5f * unit, 3.5f * unit);
            paint.setStrokeWidth(.8f * unit);
            final float innerRadius = Math.max(0, radius - 3.5f * unit);
            canvas.drawRoundRect(face, innerRadius, innerRadius, paint);
        }
    }

    private int color(int value) {
        return (value & 0x00FFFFFF) | (((value >>> 24) * alpha + 127) / 255 << 24);
    }

    private static int darken(int color) {
        return (color & 0xFF000000) | (Math.round(((color >>> 16) & 255) * .65f) << 16) |
                (Math.round(((color >>> 8) & 255) * .65f) << 8) | Math.round((color & 255) * .65f);
    }

    @Override protected boolean onStateChange(int[] state) {
        boolean e = false, p = false, f = false, s = false, a = false;
        for (int value : state) {
            if (value == android.R.attr.state_enabled) e = true;
            else if (value == android.R.attr.state_pressed) p = true;
            else if (value == android.R.attr.state_focused) f = true;
            else if (value == android.R.attr.state_selected) s = true;
            else if (value == android.R.attr.state_activated) a = true;
        }
        if (enabled == e && pressed == p && focused == f && selected == s && activated == a) return false;
        enabled = e; pressed = p; focused = f; selected = s; activated = a;
        invalidateSelf();
        return true;
    }

    @Override public boolean isStateful() { return true; }
    @Override public void setAlpha(int value) { alpha = Math.max(0, Math.min(255, value)); invalidateSelf(); }
    @Override public void setColorFilter(ColorFilter filter) { paint.setColorFilter(filter); invalidateSelf(); }
    @Override public int getOpacity() { return PixelFormat.TRANSLUCENT; }
}
