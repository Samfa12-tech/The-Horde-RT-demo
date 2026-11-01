package com.samfa12.hordelanternrt;

import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import java.util.function.BooleanSupplier;

/** One edge on touch-down; the accessibility click remains a separate input route. */
final class PressActionButton implements View.OnTouchListener, View.OnClickListener {
    private final Button button;
    private final BooleanSupplier allowed;
    private final Runnable action, holdChanged;
    private boolean active, suppressReleaseClick;
    private int pointerId=-1;

    PressActionButton(Button button, BooleanSupplier allowed, Runnable action) {
        this(button,allowed,action,()->{});
    }
    PressActionButton(Button button, BooleanSupplier allowed, Runnable action,Runnable holdChanged) {
        this.button = button; this.allowed = allowed; this.action = action;this.holdChanged=holdChanged;
        button.setOnTouchListener(this); button.setOnClickListener(this);
    }
    @Override public boolean onTouch(View view, MotionEvent event) {
        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                if (!active) {
                    pointerId=event.getPointerId(event.getActionIndex());
                    active = true; suppressReleaseClick = true; view.setPressed(true);
                    if (allowed.getAsBoolean()) action.run();
                    holdChanged.run();
                }
                return true;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP:
                if(!active || event.getPointerId(event.getActionIndex())!=pointerId)return true;
                pointerId=-1;view.setPressed(false);
                if (active) view.performClick();
                active = false; suppressReleaseClick = false;holdChanged.run();
                return true;
            case MotionEvent.ACTION_CANCEL:
                cancel(); return true;
            default: return true;
        }
    }
    @Override public void onClick(View view) {
        if (suppressReleaseClick) { suppressReleaseClick = false; return; }
        if (allowed.getAsBoolean()) action.run();
    }
    boolean held(){return active;}
    void cancel() {
        pointerId=-1;active = false; suppressReleaseClick = false; button.setPressed(false);holdChanged.run();
    }
}
