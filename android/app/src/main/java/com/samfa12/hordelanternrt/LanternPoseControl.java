package com.samfa12.hordelanternrt;

import android.graphics.drawable.Drawable;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.Button;

/** Presentation only: the existing button listener remains the lantern-pose authority. */
final class LanternPoseControl {
    static void apply(Button button, boolean raised, String action) {
        final int target = HordeUiTokens.dp(button.getContext(), 56);
        final int iconSize = HordeUiTokens.dp(button.getContext(), 32);
        final int padding = (target - iconSize) / 2;
        final ViewGroup.LayoutParams layout = button.getLayoutParams();
        if (layout != null && (layout.width != target || layout.height != target)) {
            layout.width = target;
            layout.height = target;
            button.setLayoutParams(layout); // Preserve safe-area margins and the reserved context slot.
        }
        button.setMinWidth(target);
        button.setMinimumWidth(target);
        button.setMinHeight(target);
        button.setMinimumHeight(target);
        button.setPadding(padding, padding, padding, padding);
        button.setGravity(Gravity.CENTER);
        button.setText("");
        button.setActivated(raised);
        button.setContentDescription(button.getContext().getString(
                raised ? R.string.lantern_raised_action : R.string.lantern_lowered_action, action));
        final int drawableId = raised ? R.drawable.ic_lantern_lower : R.drawable.ic_lantern_raise;
        final Drawable current = button.getCompoundDrawables()[0];
        if (!(button.getTag(R.id.lantern_pose_icon_state) instanceof Integer) ||
                (Integer) button.getTag(R.id.lantern_pose_icon_state) != drawableId || current == null) {
            final Drawable icon = button.getContext().getDrawable(drawableId);
            if (icon != null) icon.setBounds(0, 0, iconSize, iconSize);
            button.setCompoundDrawables(icon, null, null, null);
            button.setCompoundDrawablePadding(0);
            button.setTag(R.id.lantern_pose_icon_state, drawableId);
        }
    }

    private LanternPoseControl() { }
}
