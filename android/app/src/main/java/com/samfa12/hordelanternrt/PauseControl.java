package com.samfa12.hordelanternrt;

import android.graphics.drawable.Drawable;
import android.view.Gravity;
import android.widget.Button;
import android.widget.FrameLayout;

/** Presentation only: the original upper-right menu button keeps its action and input ownership. */
final class PauseControl {
    static void apply(Button button, int safeRight, int safeTop) {
        final int target = HordeUiTokens.dp(button.getContext(), 48);
        final int iconSize = HordeUiTokens.dp(button.getContext(), 24);
        final FrameLayout.LayoutParams p = (FrameLayout.LayoutParams) button.getLayoutParams();
        p.width = p.height = target;
        p.gravity = Gravity.TOP | Gravity.END;
        p.setMarginEnd(HordeUiTokens.dp(button.getContext(), 16) + safeRight);
        p.topMargin = HordeUiTokens.dp(button.getContext(), 16) + safeTop;
        button.setLayoutParams(p);
        button.setMinWidth(target); button.setMinimumWidth(target);
        button.setMinHeight(target); button.setMinimumHeight(target);
        final int padding = (target - iconSize) / 2;
        button.setPadding(padding, padding, padding, padding);
        button.setGravity(Gravity.CENTER);
        button.setText("");
        button.setContentDescription(button.getContext().getString(R.string.pause_menu));
        final Drawable current = button.getCompoundDrawables()[0];
        if (current == null || current.getBounds().width() != iconSize) {
            final Drawable glyph = button.getContext().getDrawable(R.drawable.ic_pause);
            if (glyph != null) glyph.setBounds(0, 0, iconSize, iconSize);
            button.setCompoundDrawables(glyph, null, null, null);
            button.setCompoundDrawablePadding(0);
        }
    }
    private PauseControl() { }
}
