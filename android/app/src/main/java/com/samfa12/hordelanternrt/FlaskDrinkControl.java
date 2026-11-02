package com.samfa12.hordelanternrt;

import android.graphics.drawable.Drawable;
import android.view.Gravity;
import android.widget.Button;
import android.widget.TextView;

/** Presentation only; the existing named action listener and native guards own drinking. */
final class FlaskDrinkControl {
    static void apply(Button button) {apply(button,2,2);}
    static void apply(Button button,int charges,int capacity) {
        final int target=HordeUiTokens.dp(button.getContext(),56);
        if(button.getMinWidth()!=target)button.setMinWidth(target);
        if(button.getMinimumWidth()!=target)button.setMinimumWidth(target);
        if(button.getMinHeight()!=target)button.setMinHeight(target);
        if(button.getMinimumHeight()!=target)button.setMinimumHeight(target);
        final int horizontal=HordeUiTokens.dp(button.getContext(),12),vertical=HordeUiTokens.dp(button.getContext(),8);
        if(button.getPaddingLeft()!=horizontal || button.getPaddingTop()!=vertical ||
                button.getPaddingRight()!=horizontal || button.getPaddingBottom()!=vertical)
            button.setPadding(horizontal,vertical,horizontal,vertical);
        if(button.getGravity()!=Gravity.CENTER)button.setGravity(Gravity.CENTER);
        if(button.getText().length()!=0)button.setText("");
        if(button.getContentDescription()==null || button.getContentDescription().length()==0)
            button.setContentDescription(button.getContext().getString(R.string.flask_drink));
        applyGauge(button,charges,capacity,32);
    }
    static void applyGauge(TextView view,int charges,int capacity,int widthDp) {
        final Drawable existing=view.getCompoundDrawables()[0];
        if(existing instanceof FlaskGaugeDrawable && existing.getBounds().width()==HordeUiTokens.dp(view.getContext(),widthDp)) {
            ((FlaskGaugeDrawable)existing).updateCharges(charges,capacity);
        } else {
            view.setCompoundDrawables(new FlaskGaugeDrawable(view.getContext(),charges,capacity,widthDp),null,null,null);
        }
        final int gap=HordeUiTokens.dp(view.getContext(),4);
        if(view.getCompoundDrawablePadding()!=gap)view.setCompoundDrawablePadding(gap);
    }
    private FlaskDrinkControl() { }
}
