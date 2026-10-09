package com.samfa12.hordelanternrt;

import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.HorizontalScrollView;
import android.widget.ScrollView;
import java.util.HashMap;
import java.util.Map;

/** View-only state: it never changes requested, effective or saved graphics settings. */
final class GraphicsViewportState {
    private final String page;
    private final Map<String, int[]> scrolls = new HashMap<>();
    private String focus, accessibilityFocus;
    private boolean touchFocusable;
    private int revision;

    GraphicsViewportState(String page) { this.page=page; }
    void reset() { scrolls.clear(); focus=null; accessibilityFocus=null; ++revision; }
    void remember(View root) {
        if (!page.equals(root.getTag()) || root.findViewWithTag(page + "-scroll") == null) return;
        scrolls.clear();
        visit(root, view -> {
            if (!(view.getTag() instanceof String)) return;
            String key=(String)view.getTag();
            if (view instanceof ScrollView || view instanceof HorizontalScrollView)
                scrolls.put(key,new int[]{view.getScrollX(),view.getScrollY()});
            if (view.hasFocus() && !(view instanceof ViewGroup)) {
                focus=key; touchFocusable=view.isFocusableInTouchMode();
            }
            if (view.isAccessibilityFocused()) accessibilityFocus=key;
        });
    }
    void restoreAfterLayout(View root) {
        root.setTag(page);
        final int current=++revision;
        root.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override public void onGlobalLayout() {
                root.getViewTreeObserver().removeOnGlobalLayoutListener(this);
                if (current!=revision || !page.equals(root.getTag()) || root.findViewWithTag(page + "-scroll") == null) return;
                restorePendingFocus(root);
                restoreScrolls(root,scrolls);
            }
        });
    }
    // A rebuilt live option is disabled until a current native RT acknowledgement.
    // Its focus is deferred to that acknowledgement, never used to enable editing early.
    void restorePendingFocus(View root) {
        if (!page.equals(root.getTag()) || root.findViewWithTag(page + "-scroll") == null) return;
        final Map<String,int[]> currentScrolls=new HashMap<>();
        visit(root, view -> {
            if (view.getTag() instanceof String && (view instanceof ScrollView || view instanceof HorizontalScrollView))
                currentScrolls.put((String)view.getTag(),new int[]{view.getScrollX(),view.getScrollY()});
            if (!view.isEnabled() || !(view.getTag() instanceof String)) return;
            final String key=(String)view.getTag();
            if (key.equals(focus)) {
                if (touchFocusable) view.setFocusableInTouchMode(true);
                if (view.requestFocus()) focus=null;
            }
            if (key.equals(accessibilityFocus) && view.performAccessibilityAction(
                    AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS,(Bundle)null)) accessibilityFocus=null;
        });
        restoreScrolls(root,currentScrolls);
    }
    private static void restoreScrolls(View root, Map<String,int[]> positions) {
        visit(root, view -> {
            int[] xy=positions.get(view.getTag());
            if(xy!=null) view.scrollTo(xy[0],xy[1]);
        });
    }
    private interface Visitor { void accept(View view); }
    private static void visit(View view, Visitor visitor) {
        visitor.accept(view);
        if(view instanceof ViewGroup) {
            ViewGroup g=(ViewGroup)view;
            for(int i=0;i<g.getChildCount();++i) visit(g.getChildAt(i),visitor);
        }
    }
}
