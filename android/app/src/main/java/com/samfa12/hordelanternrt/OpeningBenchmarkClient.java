package com.samfa12.hordelanternrt;

import android.os.Bundle;
import android.widget.FrameLayout;

/** Immutable recovery identity/policy only. Never retains an Activity, View, phase or runnable. */
final class OpeningBenchmarkClient {
    private static final String KEY="horde.opening-benchmark-client.v1";
    final long request,origin;
    final boolean provisional;
    private final FrameLayout.LayoutParams original;

    OpeningBenchmarkClient(long request,long origin,boolean provisional,FrameLayout.LayoutParams layout) {
        if(request<=0 || origin<=0 || (provisional && layout==null))throw new IllegalArgumentException("Incomplete benchmark client");
        this.request=request;this.origin=origin;this.provisional=provisional;
        original=layout==null?null:new FrameLayout.LayoutParams(layout);
    }
    FrameLayout.LayoutParams originalLayout() { return original==null?null:new FrameLayout.LayoutParams(original); }
    boolean matches(OpeningBenchmarkState state) { return state!=null && state.matches(request,origin); }
    static boolean saved(Bundle state) { return state!=null && state.containsKey(KEY); }
    void save(Bundle state) {
        Bundle value=new Bundle();value.putInt("schema",1);value.putLong("request",request);value.putLong("origin",origin);
        value.putBoolean("provisional",provisional);
        if(original!=null)value.putIntArray("layout",new int[]{original.width,original.height,original.gravity,
            original.leftMargin,original.topMargin,original.rightMargin,original.bottomMargin,
            relativeMargin(original,true),relativeMargin(original,false),original.getLayoutDirection()});
        state.putBundle(KEY,value);
    }
    static OpeningBenchmarkClient restore(Bundle state) {
        if(!saved(state))return null;
        try {
            Bundle value=state.getBundle(KEY);
            if(value==null || value.getInt("schema",0)!=1 || !value.containsKey("provisional"))return null;
            long request=value.getLong("request",0),origin=value.getLong("origin",0);
            boolean provisional=value.getBoolean("provisional");
            FrameLayout.LayoutParams layout=null;
            if(provisional) {
                int[] p=value.getIntArray("layout");
                if(p==null || p.length!=10 || (p[9]!=0 && p[9]!=1))return null;
                layout=new FrameLayout.LayoutParams(p[0],p[1],p[2]);layout.setMargins(p[3],p[4],p[5],p[6]);
                // Even assigning the undefined sentinel marks margins pending
                // resolution and would zero an absolute-only layout on read.
                if(p[7]!=Integer.MIN_VALUE)layout.setMarginStart(p[7]);
                if(p[8]!=Integer.MIN_VALUE)layout.setMarginEnd(p[8]);
                layout.setLayoutDirection(p[9]);
            }
            return new OpeningBenchmarkClient(request,origin,provisional,layout);
        } catch(RuntimeException invalid) { return null; }
    }
    // Public getters resolve pending margins, then fall back to an absolute
    // edge selected by layout direction. Resolve only a clone, then probe both
    // absolute edges so undefined start/end remain undefined in LTR and RTL.
    private static int relativeMargin(FrameLayout.LayoutParams source,boolean start) {
        FrameLayout.LayoutParams copy=new FrameLayout.LayoutParams(source);
        copy.resolveLayoutDirection(copy.getLayoutDirection());
        copy.leftMargin=copy.rightMargin=Integer.MIN_VALUE+1;
        int first=start?copy.getMarginStart():copy.getMarginEnd();
        copy.leftMargin=copy.rightMargin=Integer.MIN_VALUE+2;
        int second=start?copy.getMarginStart():copy.getMarginEnd();
        return first==second?first:Integer.MIN_VALUE;
    }
}
