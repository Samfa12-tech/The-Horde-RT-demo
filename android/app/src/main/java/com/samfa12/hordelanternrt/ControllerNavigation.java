package com.samfa12.hordelanternrt;

import android.graphics.Rect;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Spinner;
import java.util.ArrayList;

/** Navigation of real native controls. Never changes game or graphics state directly. */
final class ControllerNavigation {
    private ControllerNavigation() {}
    private static boolean usable(View v) {
        return v != null && v.isEnabled() && v.getVisibility() == View.VISIBLE && v.isFocusable()
                && (!(v instanceof ViewGroup) || v instanceof Spinner);
    }
    private static void collect(View v, ArrayList<View> controls) {
        if (v.getVisibility() != View.VISIBLE || !v.isEnabled()) return;
        if (usable(v)) controls.add(v);
        if (v instanceof ViewGroup) {
            ViewGroup g=(ViewGroup)v;
            for (int i=0;i<g.getChildCount();i++) collect(g.getChildAt(i),controls);
        }
    }
    private static boolean contained(View root, View v) {
        while(v!=null) { if(v==root)return true; v=v.getParent() instanceof View?(View)v.getParent():null; }
        return false;
    }
    static View ensureFocus(View root) {
        if(root==null)return null;
        ArrayList<View> controls=new ArrayList<>(); collect(root,controls);
        View focus=root.findFocus();
        if(controls.contains(focus))return focus;
        if(controls.isEmpty())return null;
        return select(controls.get(0));
    }
    private static View select(View view) {
        // Consumed joystick events do not automatically leave Android touch mode.
        view.setFocusableInTouchMode(true);
        if(!view.requestFocus())return null;
        Rect rect=new Rect(0,0,view.getWidth(),view.getHeight());
        view.requestRectangleOnScreen(rect,true);
        return view;
    }
    static boolean confirm(View root) {
        View focus=ensureFocus(root);
        return focus!=null && focus.performClick();
    }
    static void navigate(View root,int horizontal,int vertical) {
        View focus=ensureFocus(root); if(focus==null)return;
        if(horizontal!=0 && focus instanceof SeekBar) {
            int key=horizontal<0?KeyEvent.KEYCODE_DPAD_LEFT:KeyEvent.KEYCODE_DPAD_RIGHT;
            // Native keyboard route emits fromUser=true, preserving saved audio/look sliders.
            focus.onKeyDown(key,new KeyEvent(KeyEvent.ACTION_DOWN,key)); return;
        }
        int direction=vertical<0?View.FOCUS_UP:vertical>0?View.FOCUS_DOWN:
                horizontal<0?View.FOCUS_LEFT:View.FOCUS_RIGHT;
        View next=focus.focusSearch(direction);
        if(usable(next) && contained(root,next)) { select(next); return; }
        ArrayList<View> controls=new ArrayList<>(); collect(root,controls);
        int index=controls.indexOf(focus), step=(vertical<0 || horizontal<0)?-1:1;
        int target=index+step;
        if(target>=0 && target<controls.size()) { select(controls.get(target)); return; }
        // Long explanatory text can be read without trapping focus or activating a button.
        View current=focus;
        while(current!=null) {
            if(current instanceof ScrollView) {
                ((ScrollView)current).arrowScroll(direction); return;
            }
            current=current.getParent() instanceof View?(View)current.getParent():null;
        }
    }
}
