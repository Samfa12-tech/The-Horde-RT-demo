package com.samfa12.hordelanternrt;

import android.graphics.Rect;
import android.view.KeyEvent;
import android.view.FocusFinder;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Spinner;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Map;
import java.util.WeakHashMap;

/** Navigation of real native controls. Never changes game or graphics state directly. */
final class ControllerNavigation {
    private ControllerNavigation() {}
    // UI-thread-owned; detached pages are not retained by controller focus.
    private static final WeakHashMap<View,Boolean> originalTouchFocus = new WeakHashMap<>();
    private static boolean contained(View root,View view) {
        while(view!=null) {
            if(view==root)return true;
            view=view.getParent() instanceof View?(View)view.getParent():null;
        }
        return false;
    }
    static void restoreTouchPolicy(View root) {
        if(root==null)return;
        Iterator<Map.Entry<View,Boolean>> entries=originalTouchFocus.entrySet().iterator();
        while(entries.hasNext()) {
            Map.Entry<View,Boolean> entry=entries.next();
            if(contained(root,entry.getKey())) {
                entry.getKey().setFocusableInTouchMode(entry.getValue());
                entries.remove();
            }
        }
    }
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
    static View ensureFocus(View root) {
        if(root==null)return null;
        ArrayList<View> controls=new ArrayList<>(); collect(root,controls);
        View focus=root.findFocus();
        if(controls.contains(focus))return focus;
        if(controls.isEmpty())return null;
        return select(controls.get(0));
    }
    private static View select(View view) {
        // Consumed controller events do not leave Android touch mode automatically.
        originalTouchFocus.putIfAbsent(view,view.isFocusableInTouchMode());
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
        if(horizontal==0 && vertical==0)return;
        View focus=ensureFocus(root); if(focus==null)return;
        if(horizontal!=0 && focus instanceof SeekBar) {
            int key=horizontal<0?KeyEvent.KEYCODE_DPAD_LEFT:KeyEvent.KEYCODE_DPAD_RIGHT;
            // Native keyboard route emits fromUser=true, preserving saved audio/look sliders.
            focus.onKeyDown(key,new KeyEvent(KeyEvent.ACTION_DOWN,key)); return;
        }
        int direction=vertical<0?View.FOCUS_UP:vertical>0?View.FOCUS_DOWN:
                horizontal<0?View.FOCUS_LEFT:View.FOCUS_RIGHT;
        // Scope native geometric search to this menu/dialog. Activity-wide search
        // can choose a gameplay control behind it. Creation order is not direction.
        ArrayList<View> controls=new ArrayList<>(); collect(root,controls);
        // Other buttons must be eligible even while Android remains in touch mode.
        // Their original policy is restored before selecting or dispatching a click.
        boolean[] touchPolicy=new boolean[controls.size()];
        View next=null;
        try {
            for(int i=0;i<controls.size();i++) {
                touchPolicy[i]=controls.get(i).isFocusableInTouchMode();
                controls.get(i).setFocusableInTouchMode(true);
            }
            if(root instanceof ViewGroup)
                next=FocusFinder.getInstance().findNextFocus((ViewGroup)root,focus,direction);
        } finally {
            for(int i=0;i<controls.size();i++)controls.get(i).setFocusableInTouchMode(touchPolicy[i]);
        }
        if(controls.contains(next)) { select(next); return; }
        // Long explanatory text can be read without trapping focus or activating a button.
        View current=focus;
        while(current!=null) {
            if(current instanceof ScrollView && (direction==View.FOCUS_UP || direction==View.FOCUS_DOWN)) {
                ((ScrollView)current).arrowScroll(direction); return;
            }
            current=current.getParent() instanceof View?(View)current.getParent():null;
        }
    }
}
