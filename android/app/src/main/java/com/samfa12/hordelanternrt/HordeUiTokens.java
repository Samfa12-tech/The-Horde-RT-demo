package com.samfa12.hordelanternrt;
import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.StateListDrawable;

final class HordeUiTokens {
    static final int CHARCOAL=0xFF151719, SLATE=0xFF24272A, PARCHMENT=0xFFF2E9D8,
            MUTED=0xFFC9C4B8, BRASS=0xFFCFA96A, DANGER=0xFFD96F65,
            IRON=0xFF777E84, DISABLED=0xFFAEB0AC;
    static int dp(Context c,int value) { return Math.round(value*c.getResources().getDisplayMetrics().density); }
    static GradientDrawable plate(Context c,int fill,int border,int stroke) {
        GradientDrawable p=new GradientDrawable(); p.setColor(fill); p.setCornerRadius(dp(c,4));
        p.setStroke(dp(c,stroke),border); return p;
    }
    static StateListDrawable button(Context c,int fill) {
        StateListDrawable s=new StateListDrawable();
        s.addState(new int[]{-android.R.attr.state_enabled},plate(c,0xEB24272A,IRON,1));
        s.addState(new int[]{android.R.attr.state_pressed},plate(c,0xF224272A,PARCHMENT,2));
        s.addState(new int[]{android.R.attr.state_focused},plate(c,fill,PARCHMENT,2));
        s.addState(new int[]{android.R.attr.state_selected},plate(c,fill,PARCHMENT,2));
        s.addState(new int[]{},plate(c,fill,BRASS,1)); return s;
    }
    static ColorStateList label(int normal) {
        return new ColorStateList(new int[][]{{-android.R.attr.state_enabled},{}},new int[]{DISABLED,normal});
    }
    private HordeUiTokens() { }
}
