package com.samfa12.hordelanternrt;

import android.graphics.drawable.Drawable;
import android.view.Gravity;
import android.widget.Button;

/** The existing listeners/accessible actions keep ownership; no hit slop or overlay. */
final class CombatActionControl {
    static void apply(Button button,boolean sword) {
        int side=button.getLayoutParams()!=null?button.getLayoutParams().width:HordeUiTokens.dp(button.getContext(),80);
        int icon=Math.min(HordeUiTokens.dp(button.getContext(),44),Math.round(side*.56f));
        int padding=Math.max(0,(side-icon)/2);
        button.setPadding(padding,padding,padding,padding);
        button.setGravity(Gravity.CENTER);button.setText("");
        button.setContentDescription(button.getContext().getString(sword?R.string.swing:R.string.parry));
        int resource=sword?R.drawable.ic_combat_sword:R.drawable.ic_combat_shield;
        Drawable current=button.getCompoundDrawables()[0];
        if(!(button.getTag(R.id.combat_action_icon_state) instanceof Integer) ||
                (Integer)button.getTag(R.id.combat_action_icon_state)!=resource ||
                current==null || current.getBounds().width()!=icon) {
            Drawable glyph=button.getContext().getDrawable(resource);
            if(glyph!=null)glyph.setBounds(0,0,icon,icon);
            button.setCompoundDrawables(glyph,null,null,null);button.setCompoundDrawablePadding(0);
            button.setTag(R.id.combat_action_icon_state,resource);
        }
    }
    static void preview(Button button,boolean sword) {
        Drawable glyph=button.getContext().getDrawable(sword?R.drawable.ic_combat_sword:R.drawable.ic_combat_shield);
        int size=HordeUiTokens.dp(button.getContext(),24);
        if(glyph!=null)glyph.setBounds(0,0,size,size);
        button.setCompoundDrawables(glyph,null,null,null);
        button.setCompoundDrawablePadding(HordeUiTokens.dp(button.getContext(),6));
        // The existing accessible settings preview keeps full scaled text.
    }
    private CombatActionControl() { }
}
