package com.samfa12.hordelanternrt;

import android.graphics.Rect;
import android.view.View;
import android.view.WindowInsets;

/** Convert window insets once into the actual HUD parent's coordinates. */
final class HudSafeArea {
    static Rect withinParent(Rect parent, Rect window, int left, int top, int right, int bottom) {
        return new Rect(Math.max(0,window.left+left-parent.left),
                Math.max(0,window.top+top-parent.top),
                Math.max(0,parent.right-(window.right-right)),
                Math.max(0,parent.bottom-(window.bottom-bottom)));
    }
    static Rect read(View parent) {
        if(parent==null || parent.getWidth()==0 || parent.getHeight()==0)return new Rect();
        View window=parent.getRootView();
        WindowInsets insets=window.getRootWindowInsets();
        if(insets==null)return new Rect();
        int left=insets.getStableInsetLeft(),top=insets.getStableInsetTop();
        int right=insets.getStableInsetRight(),bottom=insets.getStableInsetBottom();
        if(android.os.Build.VERSION.SDK_INT>=28 && insets.getDisplayCutout()!=null) {
            left=Math.max(left,insets.getDisplayCutout().getSafeInsetLeft());
            top=Math.max(top,insets.getDisplayCutout().getSafeInsetTop());
            right=Math.max(right,insets.getDisplayCutout().getSafeInsetRight());
            bottom=Math.max(bottom,insets.getDisplayCutout().getSafeInsetBottom());
        }
        int[] p=new int[2],w=new int[2];parent.getLocationOnScreen(p);window.getLocationOnScreen(w);
        return withinParent(new Rect(p[0],p[1],p[0]+parent.getWidth(),p[1]+parent.getHeight()),
                new Rect(w[0],w[1],w[0]+window.getWidth(),w[1]+window.getHeight()),left,top,right,bottom);
    }
    private HudSafeArea() {}
}
