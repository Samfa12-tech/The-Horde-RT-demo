package com.samfa12.hordelanternrt;

import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import java.util.function.BooleanSupplier;

/** One edge on touch-down; the accessibility click remains a separate input route. */
final class PressActionButton implements View.OnTouchListener, View.OnClickListener {
    private final Button button;
    private final BooleanSupplier allowed;
    private final Runnable action;
    private boolean active, suppressReleaseClick;

    PressActionButton(Button button, BooleanSupplier allowed, Runnable action) {
        this.button = button; this.allowed = allowed; this.action = action;
        button.setOnTouchListener(this); button.setOnClickListener(this);
    }
    @Override public boolean onTouch(View view, MotionEvent event) {
        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                if (!active) {
                    active = true; suppressReleaseClick = true; view.setPressed(true);
                    if (allowed.getAsBoolean()) action.run();
                }
                return true;
            case MotionEvent.ACTION_UP:
                view.setPressed(false);
                if (active) view.performClick();
                active = false; suppressReleaseClick = false;
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
    void cancel() {
        active = false; suppressReleaseClick = false; button.setPressed(false);
    }
}
