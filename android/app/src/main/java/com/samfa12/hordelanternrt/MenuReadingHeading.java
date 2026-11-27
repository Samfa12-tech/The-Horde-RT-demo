package com.samfa12.hordelanternrt;

import android.content.Context;
import android.content.res.ColorStateList;
import android.os.Build;
import android.view.KeyEvent;
import android.view.ViewParent;
import android.widget.ScrollView;
import android.widget.TextView;

/** Gives long menu copy a starting focus and scrolls it before moving into the actions. */
final class MenuReadingHeading extends TextView {
    MenuReadingHeading(Context context) {
        super(context);
        setFocusableInTouchMode(true);
        if (Build.VERSION.SDK_INT >= 28) setAccessibilityHeading(true);
        setTextColor(new ColorStateList(new int[][]{{android.R.attr.state_focused}, {}},
                new int[]{HordeUiTokens.BRASS, HordeUiTokens.PARCHMENT}));
    }

    boolean scrollForNavigation(int direction) {
        ViewParent parent = getParent();
        while (parent != null && !(parent instanceof ScrollView)) parent = parent.getParent();
        if (!(parent instanceof ScrollView)) return false;
        ScrollView scroll = (ScrollView) parent;
        if (!scroll.canScrollVertically(direction)) return false;
        scroll.scrollBy(0, direction * Math.max(1, scroll.getHeight() * 3 / 4));
        return true;
    }

    @Override public boolean onKeyDown(int code, KeyEvent event) {
        if (code == KeyEvent.KEYCODE_DPAD_UP && scrollForNavigation(-1)) return true;
        if (code == KeyEvent.KEYCODE_DPAD_DOWN && scrollForNavigation(1)) return true;
        return super.onKeyDown(code, event);
    }
}
