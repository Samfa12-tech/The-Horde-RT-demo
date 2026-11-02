package com.samfa12.hordelanternrt;

import android.graphics.drawable.Drawable;
import android.view.Gravity;
import android.widget.Button;

/** Presentation only; the existing named action listener and native guards own drinking. */
final class FlaskDrinkControl {
    static void apply(Button button) {
        final int target=HordeUiTokens.dp(button.getContext(),56);
        final int iconSize=HordeUiTokens.dp(button.getContext(),32);
        final int padding=(target-iconSize)/2;
        button.setMinWidth(target);button.setMinimumWidth(target);
        button.setMinHeight(target);button.setMinimumHeight(target);
        button.setPadding(padding,padding,padding,padding);
        button.setGravity(Gravity.CENTER);button.setText("");
        // Preserve the current HUD publication's charge/reason description.
        if(button.getContentDescription()==null || button.getContentDescription().length()==0)
            button.setContentDescription(button.getContext().getString(R.string.flask_drink));
        final Drawable current=button.getCompoundDrawables()[0];
        if(current==null || current.getBounds().width()!=iconSize) {
            final Drawable icon=button.getContext().getDrawable(R.drawable.ic_flask_drink);
            if(icon!=null)icon.setBounds(0,0,iconSize,iconSize);
            button.setCompoundDrawables(icon,null,null,null);button.setCompoundDrawablePadding(0);
        }
    }
    private FlaskDrinkControl() { }
}
